/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.

Change Log
--------------------------------------------------------------------------------
2018-07-21 - dawlane
        Fetched in changes to vanilla Ted from git hub
        Added a work-around for Linux file path issues when using the Project context windows.
        Surpressed context menu from main menu toolbars
        Fixed image viewer and added additional image extensions.
        Double click asset file will open either the viewer or default application.
2018-07-31 - dawlane
        Heavy changes to allow single instance running of Ted. Lets us open multiple files from Explorer.
        All zero/null pointer updated to the nullptr as per C++11 standard.
        All QApplication static members now using derived CerberusApplication class.
        All mainWindow references are now set to the CerberusApplication member mainWindow.
        Work-around to Mac OS invisible QTabBar close button. Themes need to be set and load on first run.
2018-07-23 - dawlane
        Update Ted to use QtWebEngine and QtWebEnginePage and minor fixes.
        Should now build with later versions of Qt. Qt 5.9.2 tested.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "codeeditor.h"
#include "prefsdialog.h"
#include "projecttreemodel.h"
#include "debugtreemodel.h"
#include "finddialog.h"
#include "prefs.h"
#include "process.h"
#include "findinfilesdialog.h"
#include "cerberusapplication.h"

#include <QHostInfo>

#define TED_VERSION "2019-05-04"

#define SETTINGS_VERSION 2

#ifdef Q_OS_WIN
#define HOST QString("_winnt")
#elif defined( Q_OS_MAC )
#define HOST QString("_macos")
#elif defined( Q_OS_LINUX )
#define HOST QString("_linux")
#else
#define HOST QString("")
#endif

// DAWLANE - Not needed with versions of Qt that supports a name string.
#if QT_VERSION<=0x050001
#define _QUOTE(X) #X
//xxxxxxx
#define _STRINGIZE( X ) _QUOTE(X)
#endif

// DAWLANE - mainWindow is a static member of CerberusApplication.
MainWindow *CerberusApplication::mainWindow;

void cdebug( const QString &q ){
    if( CerberusApplication::mainWindow ) CerberusApplication::mainWindow->cdebug( q );
}

//***** MainWindow *****
//
MainWindow::MainWindow(QWidget *parent) : QMainWindow( parent ),_ui( new Ui::MainWindow ){

    CerberusApplication::mainWindow=this;

    //Untested fix for QT5 ala dawlane - Changed this from <= to just plain <
#if QT_VERSION<0x050001
    QTextCodec::setCodecForCStrings( QTextCodec::codecForName( "UTF-8" ) );
#endif

#ifdef Q_OS_MAC
    QCoreApplication::instance()->setAttribute( Qt::AA_DontShowIconsInMenus );
#endif

    QCoreApplication::setOrganizationName( "Cerberus X" );
    QCoreApplication::setOrganizationDomain( "cerberus-x.com" );
    QCoreApplication::setApplicationName( "Ted" );

    QString comp=QHostInfo::localHostName();

    QString cfgPath=QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    cfgPath=extractDir(extractDir(extractDir(cfgPath)))+"/ted_macos_"+comp+".ini";
#elif defined(Q_OS_WIN)
    cfgPath+="/ted_winnt_"+comp+".ini";
#elif defined(Q_OS_LINUX)
    cfgPath+="/ted_linux_"+comp+".ini";
#endif
    QSettings::setDefaultFormat( QSettings::IniFormat );
    QSettings::setPath( QSettings::IniFormat,QSettings::UserScope,cfgPath );

// DAWLANE Qt 5.6+ supported
#if QT_VERSION>0x050501
    /*
     * TODO:
     * Get WebEngineView working with pdf files.
     * Get WebEngineView working with full screen for youtube.
     */
    QWebEngineSettings::defaultSettings()->setAttribute( QWebEngineSettings::PluginsEnabled,true);
    //QWebEngineSettings::defaultSettings()->setAttribute( QWebEngineSettings::FullScreenSupportEnabled,true);
#else
    //Enables pdf viewing!
    QWebSettings::globalSettings()->setAttribute( QWebSettings::PluginsEnabled,true );
#endif
    //setIconSize( QSize(20,20) );

    _ui->setupUi( this );

    _codeEditor=nullptr;
    _lockedEditor=nullptr;
    _helpWidget=nullptr;
    _helpTopicId=0;
    _rebuildingHelp=false;

    //docking options
    setCorner( Qt::TopLeftCorner,Qt::LeftDockWidgetArea );
    setCorner( Qt::BottomLeftCorner,Qt::LeftDockWidgetArea );
    setCorner( Qt::TopRightCorner,Qt::RightDockWidgetArea );
    setCorner( Qt::BottomRightCorner,Qt::RightDockWidgetArea );

    //status bar widget
    _statusWidget=new QLabel;
    statusBar()->addPermanentWidget( _statusWidget );

    //targets combobox
    _targetsWidget=new QComboBox;
    _targetsWidget->setSizeAdjustPolicy( QComboBox::AdjustToContents );
#ifdef Q_OS_MAC
    _targetsWidget->setObjectName("cbTarget");
    _targetsWidget->view()->setObjectName("cbTargetView");
#endif
    _ui->buildToolBar->addWidget( _targetsWidget );

    _configsWidget=new QComboBox;
    _configsWidget->addItem( "Debug" );
    _configsWidget->addItem( "Release" );
#ifdef Q_OS_MAC
    _configsWidget->setObjectName("cbConfig");
    _configsWidget->view()->setObjectName("cbConfigView");
#endif
    _ui->buildToolBar->addWidget( _configsWidget );

    _indexWidget=new QComboBox;
    _indexWidget->setEditable( true );
    _indexWidget->setInsertPolicy( QComboBox::NoInsert );
    _indexWidget->setMinimumSize( 80,_indexWidget->minimumHeight() );
    _indexWidget->setMaximumSize( 240,_indexWidget->maximumHeight() );
//    _indexWidget->setSizePolicy( QSizePolicy::Expanding,QSizePolicy::Preferred );
#ifdef Q_OS_MAC
    _indexWidget->setObjectName("cbIndex");
    _indexWidget->view()->setObjectName("cbIndexView");
#endif
    _ui->helpToolBar->addWidget( _indexWidget );

    //init central tab widget
    _mainTabWidget=new QTabWidget;
    _mainTabWidget->setMovable( true );
    _mainTabWidget->setTabsClosable( true );
    //_mainTabWidget->tabBar()->setUsesScrollButtons(true);

#ifdef Q_OS_MAC
//    _mainTabWidget->setDocumentMode( true );
#endif

    setCentralWidget( _mainTabWidget );
    connect( _mainTabWidget,SIGNAL(currentChanged(int)),SLOT(onMainTabChanged(int)) );
    connect( _mainTabWidget,SIGNAL(tabCloseRequested(int)),SLOT(onCloseMainTab(int)) );

    //init console widgets
    _consoleProc=nullptr;
    _consoleTextWidget=new QTextEdit;
    _consoleTextWidget->setReadOnly( true );
//    _consoleTextWidget->setFocusPolicy( Qt::NoFocus );    //Oops, disables copy...
#ifdef Q_OS_WIN
    _consoleTextWidget->setFrameStyle( QFrame::NoFrame );
#endif

    _consoleDockWidget=new QDockWidget;
    _consoleDockWidget->setObjectName( "consoleDockWidget" );
    _consoleDockWidget->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea );
    _consoleDockWidget->setWindowTitle( "Console" );
    _consoleDockWidget->setWidget( _consoleTextWidget );
    addDockWidget( Qt::BottomDockWidgetArea,_consoleDockWidget );
    connect( _consoleDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );

    //init browser widgets
    _projectTreeModel=new ProjectTreeModel;
    _projectTreeWidget=new QTreeView;
    _projectTreeWidget->setHeaderHidden( true );
    _projectTreeWidget->setModel( _projectTreeModel );
    _projectTreeWidget->hideColumn( 1 );
    _projectTreeWidget->hideColumn( 2 );
    _projectTreeWidget->hideColumn( 3 );
    _projectTreeWidget->setContextMenuPolicy( Qt::CustomContextMenu );
#ifdef Q_OS_WIN
    _projectTreeWidget->setFrameStyle( QFrame::NoFrame );
#endif

    _projectTreeWidget->setFocusPolicy( Qt::NoFocus );

    connect( _projectTreeWidget,SIGNAL(doubleClicked(const QModelIndex&)),SLOT(onFileClicked(const QModelIndex&)) );
    connect( _projectTreeWidget,SIGNAL(customContextMenuRequested(const QPoint&)),SLOT(onProjectMenu(const QPoint&)) );

    _emptyCodeWidget=new QWidget;
    _emptyCodeWidget->setFocusPolicy( Qt::NoFocus );

    _debugTreeModel=nullptr;
    _debugTreeWidget=new QTreeView;
    _debugTreeWidget->setHeaderHidden( true );
    _debugTreeWidget->setFocusPolicy( Qt::NoFocus );
#ifdef Q_OS_WIN
    _debugTreeWidget->setFrameStyle( QFrame::NoFrame );
#endif

    _browserTabWidget=new QTabWidget;
    _browserTabWidget->addTab( _projectTreeWidget,"Projects" );
    _browserTabWidget->addTab( _emptyCodeWidget,"Code" );
    _browserTabWidget->addTab( _debugTreeWidget,"Debug" );
#ifdef Q_OS_MAC
    //_browserTabWidget->setDocumentMode( true );
#endif

    _browserDockWidget=new QDockWidget;
    _browserDockWidget->setObjectName( "browserDockWidget" );
    _browserDockWidget->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    _browserDockWidget->setWindowTitle( "Browser" );
    _browserDockWidget->setWidget( _browserTabWidget );

    addDockWidget( Qt::RightDockWidgetArea,_browserDockWidget );
    connect( _browserDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );

#ifdef Q_OS_WIN
    //_ui->actionFileNext->setShortcut( QKeySequence( "Ctrl+Tab" ) );
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL + Qt::Key_Tab));
    shortcuts.append(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    _ui->actionFileNext->setShortcuts(shortcuts);
    //_ui->actionFilePrevious->setShortcut( QKeySequence( "Ctrl+Shift+Tab" ) );
    QList<QKeySequence> shortcuts2;
    shortcuts2.append(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab));
    shortcuts2.append(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    _ui->actionFilePrevious->setShortcuts(shortcuts2);
#else
    //_ui->actionFileNext->setShortcut( QKeySequence( "Meta+Tab" ) );
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::META + Qt::Key_Tab));
    shortcuts.append(QKeySequence(Qt::META + Qt::Key_PageDown));
    _ui->actionFileNext->setShortcuts(shortcuts);

    //_ui->actionFilePrevious->setShortcut( QKeySequence( "Meta+Shift+Tab" ) );
    QList<QKeySequence> shortcuts2;
    shortcuts2.append(QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_Tab));
    shortcuts2.append(QKeySequence(Qt::META + Qt::Key_PageUp));
    _ui->actionFilePrevious->setShortcuts(shortcuts2);
#endif

    _projectPopupMenu=new QMenu;
    _projectPopupMenu->addAction( _ui->actionNewFile );
    _projectPopupMenu->addAction( _ui->actionNewFolder );
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction( _ui->actionEditFindInFiles );
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction( _ui->actionOpen_on_Desktop );
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction( _ui->actionCloseProject );

    _filePopupMenu=new QMenu;
    _filePopupMenu->addAction( _ui->actionOpen_on_Desktop );
    _filePopupMenu->addAction( _ui->actionOpen_in_Help );
    _filePopupMenu->addSeparator();
    _filePopupMenu->addAction( _ui->actionRenameFile );
    _filePopupMenu->addAction( _ui->actionDeleteFile );

    _dirPopupMenu=new QMenu;
    _dirPopupMenu->addAction( _ui->actionNewFile );
    _dirPopupMenu->addAction( _ui->actionNewFolder );
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction( _ui->actionEditFindInFiles );
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction( _ui->actionOpen_on_Desktop );
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction( _ui->actionRenameFile );
    _dirPopupMenu->addAction( _ui->actionDeleteFile );

    _editorPopupMenu = new QMenu;
    _editorPopupMenu->addAction( _ui->actionEditUndo );
    _editorPopupMenu->addAction( _ui->actionEditRedo );
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction( _ui->actionEditUn_Comment_block);
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction( _ui->actionEditFind);
    _editorPopupMenu->addSeparator();
    QMenu *bm = new QMenu("Bookmarks",_editorPopupMenu);
    bm->addAction( _ui->actionToggleBookmark );
    bm->addAction( _ui->actionPreviousBookmark );
    bm->addAction( _ui->actionNextBookmark );
    _editorPopupMenu->addMenu(bm);
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction( _ui->actionEditCut );
    _editorPopupMenu->addAction( _ui->actionEditCopy );
    _editorPopupMenu->addAction( _ui->actionEditPaste );
    _editorPopupMenu->addAction( _ui->actionEditDelete );
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction( _ui->actionEditSelectAll );


    connect( _ui->actionFileQuit,SIGNAL(triggered()),SLOT(onFileQuit()) );

    readSettings();

    if( _buildFileType.isEmpty() ){
        updateTargetsWidget( "cerberus" );
    }

    QString home2=_cerberusPath+"/docs/html/Home2.html";
    // DAWLANE EDIT -- local file Url
    //if( QFile::exists( home2 ) ) openFile( "file:///"+home2,false );
#ifdef Q_OS_WIN
    if( QFile::exists( home2 ) ) openFile( "file:/"+home2,false );
#else
    if( QFile::exists( home2 ) ) openFile( "file://"+home2,false );
#endif

    _prefsDialog=new PrefsDialog( this );
    _prefsDialog->readSettings();

    _findDialog=new FindDialog( this );
    _findDialog->readSettings();
    connect( _findDialog,SIGNAL(findReplace(int)),SLOT(onFindReplace(int)) );

    _findInFilesDialog=new FindInFilesDialog( nullptr );
    _findInFilesDialog->readSettings();
    connect( _findInFilesDialog,SIGNAL(showCode(QString,int,int)),SLOT(onShowCode(QString,int,int)) );

    // DAWLANE -- Fix menu context
    //_ui->fileToolBar->setWindowTitle( QString( "File" ) );
    //_ui->editToolBar->setWindowTitle(( QString( "Edit" )));
    //_ui->buildToolBar->setWindowTitle(( QString( "Build" )));
    //_ui->helpToolBar->setWindowTitle(( QString( "Help" )));
    setContextMenuPolicy(Qt::NoContextMenu); // Uncomment this to disable the main context popup

    //loadHelpIndex();
    // DAWLANE - Cannot directly pass arguments() to parseAppArgs
    QStringList args = CerberusApplication::arguments();
    parseAppArgs(args);

    updateWindowTitle();

    updateActions();

    statusBar()->showMessage( "Ready." );
}

MainWindow::~MainWindow(){

    delete _ui;
}

//***** private methods *****

void MainWindow::onTargetChanged( int index ){
//xxxxxxx
    //QString target=_targetsWidget->currentText();
    QString target=_targetsWidget->itemText(index);

    if( _buildFileType=="cerberus" || _buildFileType=="monkey" ){
        _activeCerberusTarget=target;
    }else if( _buildFileType=="mx2" || _buildFileType=="monkey2" ){
        _activeMonkey2Target=target;
    }
}

void MainWindow::loadHelpIndex(){
    if( _cerberusPath.isEmpty() ) return;

    QFile file( _cerberusPath+"/docs/html/index.txt" );
    if( !file.open( QIODevice::ReadOnly ) ) return;

    QTextStream stream( &file );

    stream.setCodec( "UTF-8" );

    QString text=stream.readAll();

    file.close();

    QStringList lines=text.split('\n');

    _indexWidget->disconnect();

    _indexWidget->clear();

    _completeList.clear();

    for( int i=0;i<lines.count();++i ){

        QString line=lines.at( i );

        int j=line.indexOf( ':' );
        if( j==-1 ) continue;

        QString topic=line.left(j);

        // DAWLANE -- URL fix for Linux
#ifdef Q_OS_WIN
        QString url="file:/"+_cerberusPath+"/docs/html/"+line.mid(j+1);
#else
        QString url="file://"+_cerberusPath+"/docs/html/"+line.mid(j+1);
#endif
        QString status = line;
        _indexWidget->addItem( topic );
        _completeList.append(topic);

        _helpUrls.insert( topic,url );
    }


    // Read in declarations for the F! Status bar help
    QFile file2( _cerberusPath+"/docs/html/decls.txt" );
    if( !file2.open( QIODevice::ReadOnly ) ) return;

    QTextStream stream2( &file2 );

    stream2.setCodec( "UTF-8" );

    QString text2=stream2.readAll();

    file2.close();

    QStringList lines2=text2.split('\n');

    for( int i=0;i<lines2.count();++i ){

        QString line2=lines2.at( i );

        if ( line2.startsWith("Class ") ) continue;
        if ( line2.startsWith("Property ") ) continue;
        if ( line2.startsWith("Module ") ) continue;
        if ( line2.startsWith("Inherited_method  ") ) continue;
        if ( line2.startsWith("Const ") ) continue;

        line2 = line2.left(line2.length()-1);

        int j2=line2.indexOf( '#' );
        if( j2==-1 ) continue;

        QString topic2=line2.right(line2.length()-j2-1);

        int j3=line2.indexOf( ':' );
        int j4=line2.indexOf( ')' );
        QString status = topic2 + line2.mid(j3,j4-j3+1);
        //status = status.replace(":", " : ");
        status = status.replace("(", " (");
        status = status.replace(",", ", ");
        _helpF1.insert(topic2, status);
    }



    connect( _indexWidget,SIGNAL(currentIndexChanged(QString)),SLOT(onShowHelp(QString)) );
}

void MainWindow::parseAppArgs(QStringList &args){
    for( int i=1;i<args.size();++i ){
        QString arg=fixPath( args.at(i) );
        if( QFile::exists( arg ) ){
            openFile( arg,true );
        }
    }
}

bool MainWindow::isBuildable( CodeEditor *editor ){
    if( !editor ) return false;
    if( editor->fileType()=="cxs" ) return !_cerberusPath.isEmpty();
    if( editor->fileType()=="monkey" ) return !_cerberusPath.isEmpty();
    if( editor->fileType()=="bmx" ) return !_blitzmaxPath.isEmpty();
    if( editor->fileType()=="monkey2" ) return !_monkey2Path.isEmpty();
    return false;
}

QString MainWindow::widgetPath( QWidget *widget ){
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( widget ) ){
        return editor->path();
    }else if( HelpView *helpView=qobject_cast<HelpView*>( widget ) ){
        return helpView->url().toString();
    }
    return "";
}

CodeEditor *MainWindow::editorWithPath( const QString &path ){
    for( int i=0;i<_mainTabWidget->count();++i ){
        if( CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){
            if( editor->path()==path ){
                editor->_mainWnd = this;
                return editor;
            }
        }
    }
    return nullptr;
}

void MainWindow::showImage(const QString &path) {
    QLabel* label = new QLabel();
    QImage img;
    bool valid = img.load( path );
    if( valid ){
        int w = img.width();
        int h = img.height();
        QString imgSize = QString::number(w)+"x"+ QString::number(h);
        label->setScaledContents(false);
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumWidth(128);
        label->setMinimumHeight(128);
        label->setPixmap(QPixmap::fromImage(img).scaled(w,h,Qt::KeepAspectRatio));
        label->setWindowTitle("CX Image Viewer-> "+ imgSize+" : "+path);
        label->setAutoFillBackground(true);
        label->show();
    }else{
#ifdef Q_OS_WIN
        QDesktopServices::openUrl( "file:/"+path );
#else
        QDesktopServices::openUrl( "file://"+path );
#endif
    }
}

// DALWANE -- play audio file
void MainWindow::playAudio(const QString &path){
/* DAWLANE -- Open audio files
* For the time being open the files with the operating systems defaults
*/
#ifdef Q_OS_WIN
    QDesktopServices::openUrl( "file:/"+path );
#else
    QDesktopServices::openUrl( "file://"+path );
#endif
}

void MainWindow::showDoc(const QString &path){
    /* DAWLANE -- Open doc/pdf files
    * For the time being open the files with the operating systems defaults
    */
    #ifdef Q_OS_WIN
        QDesktopServices::openUrl( "file:/"+path );
    #else
        QDesktopServices::openUrl( "file://"+path );
    #endif
}


QWidget *MainWindow::newFile( const QString &cpath ){

    QString path=cpath;

    if( path.isEmpty() ){

        QString srcTypes="*.cxs *.monkey *.cpp *.h *.cs *.js *.as *.java *.txt";
        if( !_monkey2Path.isEmpty() ) srcTypes+=" *.mx2 *.monkey2";

        path=fixPath( QFileDialog::getSaveFileName( this,"New File",_defaultDir,"Source Files ("+srcTypes+")" ) );
        if( path.isEmpty() ) return nullptr;
    }

    QFile file( path );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ){
        QMessageBox::warning( this,"New file","Failed to create new file: "+path );
        return nullptr;
    }
    file.close();

    if( CodeEditor *editor=editorWithPath( path ) ) closeFile( editor );

    return openFile( path,true );
}

QWidget *MainWindow::newFileTemplate( const QString &cpath ){

    QString path="";

    if( path.isEmpty() ){

        QString srcTypes="*.cxs *.monkey *.cpp *.h *.cs *.js *.as *.java *.txt";
        if( !_monkey2Path.isEmpty() ) srcTypes+=" *.mx2 *.monkey2";

        path=fixPath( QFileDialog::getSaveFileName( this,"New File from template",_defaultDir,"Source Files ("+srcTypes+")" ) );
        if( path.isEmpty() ) return nullptr;
    }
    QFile::copy(cpath, path);

    QFile file( path );
    if( !file.open( QIODevice::ReadOnly ) ){
        QMessageBox::warning( this,"New file","Failed to create new file from template: "+path );
        return nullptr;
    }

    file.close();

    if( CodeEditor *editor=editorWithPath( path ) ) closeFile( editor );

    return openFile( path,true );
}


QWidget *MainWindow::openFile( const QString &cpath,bool addToRecent ){

    QString path=cpath;

    if( isUrl( path ) ){
/*
        if( path.startsWith( "file:" ) && path.endsWith( "/docs/html/Home.html" ) ){
            QString path2=_cerberusPath+"/docs/html/Home2.html";
            if( QFile::exists( path2 ) ) path="file:///"+path2;
        }
*/
        HelpView *helpView=nullptr;
        for( int i=0;i<_mainTabWidget->count();++i ){
            helpView=qobject_cast<HelpView*>( _mainTabWidget->widget( i ) );
            if( helpView ) break;
        }
        if( !helpView ){
            helpView=new HelpView(_mainTabWidget);
// DAWLANE Qt 5.6+ supported
#if QT_VERSION>0x050501
            // QWebEnginePage already uses links.
            // Uncomment these to set the QWebEnginePage so you can over-ride acceptNavigationRequest to open external browsers.
            helpView->setPage(new WebEnginePage(helpView));
            //connect( helpView,SIGNAL(linkClicked(QUrl)),SLOT(onLinkClicked(QUrl)) );
#else
            helpView->page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
            connect( helpView,SIGNAL(linkClicked(QUrl)),SLOT(onLinkClicked(QUrl)) );
#endif

            _mainTabWidget->addTab( helpView,"Help" );
        }

        helpView->setUrl( path );

        if( helpView!=_mainTabWidget->currentWidget() ){
            _mainTabWidget->setCurrentWidget( helpView );
        }else{
            updateWindowTitle();
        }

        return helpView;
    }

    if( path.isEmpty() ){

        QString srcTypes="*.cxs *.monkey *.cpp *.h *.cs *.js *.as *.java *.txt";
        if( !_monkey2Path.isEmpty() ) srcTypes+=" *.mx2 *.monkey2";

        path=fixPath( QFileDialog::getOpenFileName( this,"Open File",_defaultDir,"Source Files ("+srcTypes+");;Image Files(*.jpg *.png *.bmp);;All Files(*.*)" ) );
        if( path.isEmpty() ) return nullptr;

        _defaultDir=extractDir( path );
    }

// DAWLANE -- Fix for image, audio and doc/pdf file selection
    if (isImageFile(path)) {
        showImage(path);
        return nullptr;
    }

    if (isAudioFile(path)) {
        playAudio(path);
        return nullptr;
    }

    if (isDocFile(path)) {
        showDoc(path);
        return nullptr;
    }

    CodeEditor *editor=editorWithPath( path );
    if( editor ){
        editor->_mainWnd = this;
        _mainTabWidget->setCurrentWidget( editor );
        return editor;
    }

    editor=new CodeEditor(nullptr,this);

    if( !editor->open( path ) ){
        delete editor;
        QMessageBox::warning( this,"Open File Error","Error opening file: "+path );
        return nullptr;
    }

    editor->_mainWnd = this;
    connect( editor,SIGNAL(textChanged()),SLOT(onTextChanged()) );
    connect( editor,SIGNAL(cursorPositionChanged()),SLOT(onCursorPositionChanged()) );

    editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( editor,SIGNAL(customContextMenuRequested(const QPoint&)),SLOT(onEditorMenu(const QPoint&)) );

    _mainTabWidget->addTab( editor,stripDir( path ) );
    //int tabindex = _mainTabWidget->count() - 1;
    //_mainTabWidget->setTabToolTip(tabindex,path);
    _mainTabWidget->setCurrentWidget( editor );

    if( addToRecent ){
        QMenu *menu=_ui->menuRecent_Files;
        QList<QAction*> actions=menu->actions();
        bool found=false;
        for( int i=0;i<actions.size();++i ){
            if( actions[i]->text()==path ){
                found=true;
                break;
            }
        }
        if( !found ){
            for( int i=19;i<actions.size();++i ){
                menu->removeAction( actions[i] );
            }
            QAction *action=new QAction( path,menu );
            if( actions.size() ){
                menu->insertAction( actions[0],action );
            }else{
                menu->addAction( action );
            }
            connect( action,SIGNAL(triggered()),this,SLOT(onFileOpenRecent()) );
        }
    }
    if (_mainTabWidget->count() > 1)
        _mainTabWidget->tabBar()->setUsesScrollButtons(true);

    editor->_mainWnd = this;
    return editor;
}

bool MainWindow::saveFile( QWidget *widget,const QString &cpath ){

    QString path=cpath;

    CodeEditor *editor=qobject_cast<CodeEditor*>( widget );
    if( !editor ) return true;

    if( path.isEmpty() ){

        _mainTabWidget->setCurrentWidget( editor );

        QString srcTypes="*.cxs *.monkey *.cpp *.h *.cs *.js *.as *.java *.txt";
        if( !_monkey2Path.isEmpty() ) srcTypes+=" *.mx2 *.monkey2";

        path=fixPath( QFileDialog::getSaveFileName( this,"Save File As",editor->path(),"Source Files ("+srcTypes+")" ) );
        if( path.isEmpty() ) return false;

    }else if( !editor->modified() ){
        return true;
    }

    if( !editor->save( path ) ){
        QMessageBox::warning( this,"Save File Error","Error saving file: "+path );
        return false;
    }

    updateTabLabel( editor );

    updateWindowTitle();

    updateActions();

    return true;
}

bool MainWindow::closeFile( QWidget *widget,bool really ){
    if( !widget ) return true;

    CodeEditor *editor=qobject_cast<CodeEditor*>( widget );

    if( editor && editor->modified() ){

        _mainTabWidget->setCurrentWidget( editor );

        QMessageBox msgBox;
        msgBox.setText( editor->path()+" has been modified." );
        msgBox.setInformativeText( "Do you want to save your changes?" );
        msgBox.setStandardButtons( QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel );
        msgBox.setDefaultButton( QMessageBox::Save );

        int ret=msgBox.exec();

        if( ret==QMessageBox::Save ){
            if( !saveFile( editor,editor->path() ) ) return false;
        }else if( ret==QMessageBox::Cancel ){
            return false;
        }else if( ret==QMessageBox::Discard ){
        }
    }

    if( !really ) return true;

    if( widget==_codeEditor ){
        _codeEditor=nullptr;
    }else if( widget==_helpWidget ){
        _helpWidget=nullptr;
    }
    if( widget==_lockedEditor ){
        _lockedEditor=nullptr;
    }

    _mainTabWidget->removeTab( _mainTabWidget->indexOf( widget ) );

    delete widget;

    if (_mainTabWidget->count() <= 1)
        _mainTabWidget->tabBar()->setUsesScrollButtons(false);

    return true;
}

bool MainWindow::confirmQuit(){

    writeSettings();

    _prefsDialog->writeSettings();

    _findDialog->writeSettings();

    _findInFilesDialog->writeSettings();

    for( int i=0;i<_mainTabWidget->count();++i ){

        CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) );

        if( editor && !closeFile( editor,false ) ) return false;
    }

    return true;
}

void MainWindow::closeEvent( QCloseEvent *event ){

    if( confirmQuit() ){
        _findInFilesDialog->close();
        event->accept();
    }else{
        event->ignore();
    }
}

//Settings...
//
bool MainWindow::isValidCerberusPath( const QString &path ){
    QString transcc="transcc"+HOST;
#ifdef Q_OS_WIN
    transcc+=".exe";
#endif
    return QFile::exists( path+"/bin/"+transcc );
}

bool MainWindow::isValidBlitzmaxPath( const QString &path ){
#ifdef Q_OS_WIN
    QString bmk="bmk.exe";
#else
    QString bmk="bmk";
#endif
    return QFile::exists( path+"/bin/"+bmk );
}

QString MainWindow::defaultCerberusPath(){
    QString path=CerberusApplication::applicationDirPath();
    while( !path.isEmpty() ){
        if( isValidCerberusPath( path ) ) return path;
        path=extractDir( path );
    }
    return "";
}

void MainWindow::enumTargets(){
    if( _cerberusPath.isEmpty() ) return;

    _cerberusTargets.clear();
    _monkey2Targets.clear();

    QDir monkey2Dir( _cerberusPath+"/monkey2" );
    if( !monkey2Dir.exists() ) monkey2Dir=QDir( _cerberusPath+"/../monkey2" );
    if( monkey2Dir.exists() ){
        _monkey2Path=monkey2Dir.absolutePath();
        _monkey2Targets.push_back( "Desktop" );
        _monkey2Targets.push_back( "Emscripten" );
        _activeMonkey2Target="Desktop";
    }

    QString cmd="\""+_cerberusPath+"/bin/transcc"+HOST+"\"";

    Process proc;
    if( !proc.start( cmd ) ) return;

    QString sol="Valid targets: ";
    QString ver="TRANS cerberus compiler V";

    while( proc.waitLineAvailable( 0 ) ){
        QString line=proc.readLine( 0 );
        if( line.startsWith( ver ) ){
            _transVersion=line.mid( ver.length() );
        }else if( line.startsWith( sol ) ){
            line=line.mid( sol.length() );
            QStringList bits=line.split( ' ' );
            for( int i=0;i<bits.count();++i ){
                QString bit=bits[i];
                if( bit.isEmpty() ) continue;
                QString target=bit.replace( '_',' ' );
                if( target.contains( "Html5" ) ) _activeCerberusTarget=target;
                _cerberusTargets.push_back( target );
            }
        }
    }

}

// DAWLANE - Made theme loading into a method set correctly to avoid odd visuals.
void MainWindow::loadTheme(QString &appPath, Prefs *prefs)
{
    QString css = "";
    QString cssFile = "";
    cssFile = prefs->getString( "theme" );
    QFile f(appPath+"/themes/"+cssFile+"/"+cssFile+".css");
    if(f.open(QFile::ReadOnly)) {
        css = f.readAll();
    }
    f.close();

    //css += "QDockWidget::title{text-align:center;background: #ff0000; }";

    css.replace("url(:","url("+appPath+"/themes/"+cssFile);

    qApp->setStyleSheet(css);
}

void MainWindow::readSettings(){

    QSettings settings;

    Prefs *prefs=Prefs::prefs();


    QString appPath=QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    appPath = extractDir(extractDir(extractDir(appPath)));
#endif

    if( settings.value( "settingsVersion" ).toInt()<1 ){

        prefs->setValue( "fontFamily","Courier" );
        prefs->setValue( "fontSize",12 );
        prefs->setValue( "smoothFonts",true );
        prefs->setValue( "tabSize",4 );
        prefs->setValue( "tabs4spaces",true );
        prefs->setValue( "capitalizeAPI",false );
        prefs->setValue( "theme","default" );
        prefs->setValue( "backgroundColor",QColor( 255,255,255 ) );
        prefs->setValue( "console1Color",QColor( 70,70,70 ) );
        prefs->setValue( "console2Color",QColor( 70,170,70 ) );
        prefs->setValue( "console3Color",QColor( 70,70,170 ) );
        prefs->setValue( "console4Color",QColor( 70,170,170 ) );
        prefs->setValue( "defaultColor",QColor( 0,0,0 ) );
        prefs->setValue( "numbersColor",QColor( 0,0,255 ) );
        prefs->setValue( "stringsColor",QColor( 170,0,255 ) );
        prefs->setValue( "identifiersColor",QColor( 0,0,0 ) );
        prefs->setValue( "keywordsColor",QColor( 0,85,255 ) );
        prefs->setValue( "keywords2Color",QColor( 0,85,255 ) );
        prefs->setValue( "lineNumberColor",QColor( 0,85,255 ) );
        prefs->setValue( "commentsColor",QColor( 0,128,128 ) );
        prefs->setValue( "highlightColor",QColor( 255,255,128 ) );
        prefs->setValue( "highlightCurrLine",true );
        prefs->setValue( "highlightCurrWord",true );
        prefs->setValue( "highlightBrackets",true );
        prefs->setValue( "showLineNumbers",true );
        prefs->setValue( "sortCodeBrowser",true );

        _cerberusPath=defaultCerberusPath();
        prefs->setValue( "cerberusPath",_cerberusPath );

        _blitzmaxPath="";
        prefs->setValue( "blitzmaxPath",_blitzmaxPath );

        if( !_cerberusPath.isEmpty() ){
            _projectTreeModel->addProject( _cerberusPath );
        }

        // DAWLANE - Make sure that our theme is set correctly to avoid odd visuals on first run.
        writeSettings();
        loadTheme(appPath, prefs);

        enumTargets();

        onHelpHome();

        loadHelpIndex();
        return;
    }

    _cerberusPath=defaultCerberusPath();

    QString prefsCerberusPath=prefs->getString( "cerberusPath" );

    if( _cerberusPath.isEmpty() ){

        _cerberusPath=prefsCerberusPath;

        if( !isValidCerberusPath( _cerberusPath ) ){
            _cerberusPath="";
            prefs->setValue( "cerberusPath",_cerberusPath );
            QMessageBox::warning( this,"Cerberus Path Error","Invalid Cerberus path!\n\nPlease select correct path from the File..Options dialog" );
        }

    }else if( _cerberusPath!=prefsCerberusPath ){
        prefs->setValue( "cerberusPath",_cerberusPath );
        QMessageBox::information( this,"Cerberus Path Updated","Cerberus path has been updated to "+_cerberusPath );
    }

    _blitzmaxPath=prefs->getString( "blitzmaxPath" );
    if( !_blitzmaxPath.isEmpty() && !isValidBlitzmaxPath( _blitzmaxPath ) ){
        _blitzmaxPath="";
        prefs->setValue( "blitzmaxPath",_blitzmaxPath );
        QMessageBox::warning( this,"BlitzMax Path Error","Invalid BlitzMax path!\n\nPlease select correct path from the File..Options dialog" );
    }

    enumTargets();
    loadHelpIndex();
    settings.beginGroup( "mainWindow" );
    restoreGeometry( settings.value( "geometry" ).toByteArray() );
    restoreState( settings.value( "state" ).toByteArray() );
    settings.endGroup();

    int n=settings.beginReadArray( "openProjects" );
    for( int i=0;i<n;++i ){
        settings.setArrayIndex( i );
        QString path=fixPath( settings.value( "path" ).toString() );
        if( QFile::exists( path ) ) _projectTreeModel->addProject( path );
    }
    settings.endArray();

    n=settings.beginReadArray( "openDocuments" );
    for( int i=0;i<n;++i ){
        settings.setArrayIndex( i );
        QString path=fixPath( settings.value( "path" ).toString() );
        if( isUrl( path ) ){
            openFile( path,false );
        }else{
            if( QFile::exists( path ) ) openFile( path,false );
        }
    }
    settings.endArray();

    n=settings.beginReadArray( "recentFiles" );
    for( int i=0;i<n;++i ){
        settings.setArrayIndex( i );
        QString path=fixPath( settings.value( "path" ).toString() );
        if( QFile::exists( path ) ) _ui->menuRecent_Files->addAction( path,this,SLOT(onFileOpenRecent()) );
    }
    settings.endArray();

    settings.beginGroup( "buildSettings" );
    QString target=settings.value( "target" ).toString();

    _activeCerberusTarget=target;
    _buildFileType="";

    /*
    if( !target.isEmpty() ){
        for( int i=0;i<_targetsWidget->count();++i ){
            if( _targetsWidget->itemText(i)==target ){
                _targetsWidget->setCurrentIndex( i );
                break;
            }
        }
    }
    */

    QString config=settings.value( "config" ).toString();
    if( !config.isEmpty() ){
        for( int i=0;i<_configsWidget->count();++i ){
            if( _configsWidget->itemText(i)==config ){
                _configsWidget->setCurrentIndex( i );
                break;
            }
        }
    }

    QString locked=settings.value( "locked" ).toString();
    if( !locked.isEmpty() ){
        if( CodeEditor *editor=editorWithPath( locked ) ){
            _lockedEditor=editor;
            updateTabLabel( editor );
        }
    }
    settings.endGroup();

    if( settings.value( "settingsVersion" ).toInt()<2 ){
        return;
    }

    _defaultDir=fixPath( settings.value( "defaultDir" ).toString() );

    loadTheme(appPath, prefs);
/*
    QString css = "";
    QString cssFile = "";
    cssFile = prefs->getString( "theme" );
    QFile f(appPath+"/themes/"+cssFile+"/"+cssFile+".css");
    if(f.open(QFile::ReadOnly)) {
        css = f.readAll();
    }
    f.close();

    //css += "QDockWidget::title{text-align:center;background: #ff0000; }";

    css.replace("url(:","url("+appPath+"/themes/"+cssFile);

    qApp->setStyleSheet(css);
*/

    CerberusApplication::processEvents();
    QString tempPath ="";
    QDir recoredDir(appPath+"/templates/");
    QStringList allFiles = recoredDir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
    foreach (const QString &str, allFiles) {
        tempPath = appPath+"/templates/"+str;
        if( QFile::exists( tempPath ) ) _ui->menuNewTemplate->addAction( QFileInfo(tempPath).baseName(),this,SLOT(onFileNewTemplate())) ;
    }
    // Set the actions icons depending on the theme
    setIcons();

    _targetsWidget->adjustSize();
    _configsWidget->adjustSize();
}

QIcon MainWindow::getThemeIcon(const QString &theme, const QString &ic, const QString &icd){
    QString appPath=QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    appPath = extractDir(extractDir(extractDir(appPath)));
#endif
    QIcon icon = QIcon(QPixmap(appPath+"/themes/"+theme+"/icons/ui/"+ic));
    if (icd != "") icon.addPixmap(QPixmap(appPath+"/themes/"+theme+"/icons/ui/"+icd),QIcon::Disabled);
    return icon;
}

void MainWindow::setIcons(){
    QSettings settings;

    Prefs *prefs=Prefs::prefs();

    QString appPath=QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    appPath = extractDir(extractDir(extractDir(appPath)));
#endif

    QString css = "";
    QString theme = "";
    theme = prefs->getString( "theme" );

    _ui->actionNew->setIcon(getThemeIcon(theme, "New.png","New_off.png"));
    _ui->actionOpen->setIcon(getThemeIcon(theme, "Open.png","Open_off.png"));
    _ui->actionClose->setIcon(getThemeIcon(theme, "Close.png","Close_off.png"));
    _ui->actionSave->setIcon(getThemeIcon(theme, "Save.png","Save_off.png"));

    _ui->actionEditCopy->setIcon(getThemeIcon(theme, "Copy.png","Copy_off.png"));
    _ui->actionEditCut->setIcon(getThemeIcon(theme, "Cut.png","Cut_off.png"));
    _ui->actionEditPaste->setIcon(getThemeIcon(theme, "Paste.png","Paste_off.png"));
    _ui->actionEditFind->setIcon(getThemeIcon(theme, "Find.png","Find_off.png"));

    _ui->actionBuildBuild->setIcon(getThemeIcon(theme, "Build.png","Build_off.png"));
    _ui->actionBuildRun->setIcon(getThemeIcon(theme, "Build-Run.png","Build-Run_off.png"));

    _ui->actionStep->setIcon(getThemeIcon(theme, "Step.png","Step_off.png"));
    _ui->actionStep_In->setIcon(getThemeIcon(theme, "Step-In.png","Step-In_off.png"));
    _ui->actionStep_Out->setIcon(getThemeIcon(theme, "Step-Out.png","Step-Out_off.png"));
    _ui->actionKill->setIcon(getThemeIcon(theme, "Stop.png","Stop_off.png"));

    _ui->actionHelpHome->setIcon(getThemeIcon(theme, "Home.png","Home_off.png"));
    _ui->actionHelpBack->setIcon(getThemeIcon(theme, "Back.png","Back_off.png"));
    _ui->actionHelpForward->setIcon(getThemeIcon(theme, "Forward.png","Forward_off.png"));
}

void MainWindow::writeSettings(){
    QSettings settings;

    settings.setValue( "settingsVersion",SETTINGS_VERSION );

    settings.beginGroup( "mainWindow" );
    settings.setValue( "geometry",saveGeometry() );
    settings.setValue( "state",saveState() );
    settings.endGroup();

    settings.beginWriteArray( "openProjects" );
    QVector<QString> projs=_projectTreeModel->projects();
    for( int i=0;i<projs.size();++i ){
        settings.setArrayIndex(i);
        settings.setValue( "path",projs[i] );
    }
    settings.endArray();

    settings.beginWriteArray( "openDocuments" );
    int n=0;
    for( int i=0;i<_mainTabWidget->count();++i ){
        QString path=widgetPath( _mainTabWidget->widget( i ) );
        if( path.isEmpty() ) continue;
        settings.setArrayIndex( n++ );
        settings.setValue( "path",path );
    }
    settings.endArray();

    settings.beginWriteArray( "recentFiles" );
    QList<QAction*> rfiles=_ui->menuRecent_Files->actions();
    for( int i=0;i<rfiles.size();++i ){
        settings.setArrayIndex( i );
        settings.setValue( "path",rfiles[i]->text() );
    }
    settings.endArray();

    settings.beginGroup( "buildSettings" );
    settings.setValue( "target",_targetsWidget->currentText() );
    settings.setValue( "config",_configsWidget->currentText() );
    settings.setValue( "locked",_lockedEditor ? _lockedEditor->path() : "" );
    settings.endGroup();

    settings.setValue( "defaultDir",_defaultDir );
}

void MainWindow::updateTargetsWidget( QString fileType ){

    if( _buildFileType!=fileType ){

        disconnect( _targetsWidget,nullptr,nullptr,nullptr );
        _targetsWidget->clear();

        if( fileType=="cxs" || fileType=="monkey" ){
            for( int i=0;i<_cerberusTargets.size();++i ){
                _targetsWidget->addItem( _cerberusTargets.at(i) );
                if( _cerberusTargets.at(i)==_activeCerberusTarget ) _targetsWidget->setCurrentIndex( i );
            }
            _activeCerberusTarget=_targetsWidget->currentText();
            _configsWidget->setEnabled( true );
        }else if( fileType=="mx2" || fileType=="monkey2" ){
            for( int i=0;i<_monkey2Targets.size();++i ){
                _targetsWidget->addItem( _monkey2Targets.at(i) );
                if( _monkey2Targets.at(i)==_activeMonkey2Target ) _targetsWidget->setCurrentIndex( i );
            }
            _activeMonkey2Target=_targetsWidget->currentText();
            _configsWidget->setEnabled( true );
        }else if( fileType=="bmx" ){
            _targetsWidget->addItem( "BlitzMax App" );
            _configsWidget->setEnabled( false );
        }
        _buildFileType=fileType;
        connect( _targetsWidget,SIGNAL(currentIndexChanged(int)),SLOT(onTargetChanged(int)) );
    }
     _targetsWidget->adjustSize();
}

//Actions...
//
void MainWindow::updateActions(){

    bool ed=_codeEditor!=nullptr;
    bool db=_debugTreeModel!=nullptr;
    bool wr=ed && !_codeEditor->isReadOnly();
    bool sel=ed && _codeEditor->textCursor().hasSelection();

    bool saveAll=false;
    for( int i=0;i<_mainTabWidget->count();++i ){
        if( CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){
            if( editor->modified() ) saveAll=true;
        }
    }

    //file menu
    _ui->actionClose->setEnabled( ed || _helpWidget );
    _ui->actionClose_All->setEnabled( _mainTabWidget->count()>1 || (_mainTabWidget->count()==1 && !_helpWidget) );
    _ui->actionClose_Others->setEnabled( _mainTabWidget->count()>1 );
    _ui->actionSave->setEnabled( ed && _codeEditor->modified() );
    _ui->actionSave_As->setEnabled( ed );
    _ui->actionSave_All->setEnabled( saveAll );
    _ui->actionFileNext->setEnabled( _mainTabWidget->count()>1 );
    _ui->actionFilePrevious->setEnabled( _mainTabWidget->count()>1 );

    //edit menu
    _ui->actionEditUndo->setEnabled( wr && _codeEditor->document()->isUndoAvailable() );
    _ui->actionEditRedo->setEnabled( wr && _codeEditor->document()->isRedoAvailable() );
    _ui->actionEditCut->setEnabled( wr && sel );
    _ui->actionEditCopy->setEnabled( sel );
    _ui->actionEditPaste->setEnabled( wr );
    _ui->actionEditDelete->setEnabled( sel );
    _ui->actionEditSelectAll->setEnabled( ed );
    _ui->actionEditFind->setEnabled( ed );
    _ui->actionEditFindNext->setEnabled( ed );
    _ui->actionEditGoto->setEnabled( ed );
    _ui->actionEditUn_Comment_block->setEnabled( ed );

    //view menu - not totally sure why !isHidden works but isVisible doesn't...
    _ui->actionViewFile->setChecked( !_ui->fileToolBar->isHidden() );
    _ui->actionViewEdit->setChecked( !_ui->editToolBar->isHidden() );
    _ui->actionViewBuild->setChecked( !_ui->buildToolBar->isHidden() );
    _ui->actionViewHelp->setChecked( !_ui->helpToolBar->isHidden() );
    _ui->actionViewConsole->setChecked( !_consoleDockWidget->isHidden() );
    _ui->actionViewBrowser->setChecked( !_browserDockWidget->isHidden() );
    _ui->actionToggleBookmark->setEnabled( ed );
    _ui->actionNextBookmark->setEnabled( ed );
    _ui->actionPreviousBookmark->setEnabled( ed );

    //build menu
    CodeEditor *buildEditor=_lockedEditor ? _lockedEditor : _codeEditor;
    bool canBuild=!_consoleProc && isBuildable( buildEditor );
    bool canTrans=canBuild && ( buildEditor->fileType()=="cxs" || buildEditor->fileType()=="monkey" );
    _ui->actionBuildBuild->setEnabled( canBuild );
    _ui->actionBuildRun->setEnabled( canBuild || db );
    _ui->actionBuildCheck->setEnabled( canTrans );
    _ui->actionBuildUpdate->setEnabled( canTrans );
    _ui->actionStep->setEnabled( db );
    _ui->actionStep_In->setEnabled( db );
    _ui->actionStep_Out->setEnabled( db );
    _ui->actionKill->setEnabled( _consoleProc!=nullptr );
    _ui->actionLock_Build_File->setEnabled( _codeEditor!=_lockedEditor && isBuildable( _codeEditor ) );
    _ui->actionUnlock_Build_File->setEnabled( _lockedEditor!=nullptr );

    //targets widget
    if( isBuildable( buildEditor ) ){
        updateTargetsWidget( buildEditor->fileType() );
    }

    //help menu
    _ui->actionHelpBack->setEnabled( _helpWidget!=nullptr );
    _ui->actionHelpForward->setEnabled( _helpWidget!=nullptr );
    _ui->actionHelpQuickHelp->setEnabled( _codeEditor!=nullptr );
    _ui->actionHelpRebuild->setEnabled( _consoleProc==nullptr );
}

void MainWindow::updateWindowTitle(){
    QWidget *widget=_mainTabWidget->currentWidget();
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( widget ) ){
        setWindowTitle( editor->path() );
    }else if( HelpView *helpView=qobject_cast<HelpView*>( widget ) ){
        setWindowTitle( helpView->url().toString() );
    }else{
        setWindowTitle( "Ted V" TED_VERSION );
    }
}

//Main tab widget...
//
void MainWindow::updateTabLabel( QWidget *widget ){
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( widget ) ){
        QString text=stripDir( editor->path() );
        if( editor->modified() ) text=text+"*";
        if( editor==_lockedEditor ) text="+"+text;
        _mainTabWidget->setTabText( _mainTabWidget->indexOf( widget ),text );
    }
}

void MainWindow::onCloseMainTab( int index ){

    closeFile( _mainTabWidget->widget( index ) );
}

void MainWindow::onMainTabChanged( int index ){

    CodeEditor *_oldEditor=_codeEditor;

    QWidget *widget=_mainTabWidget->widget( index );

    _codeEditor=qobject_cast<CodeEditor*>( widget );

    _helpWidget=qobject_cast<HelpView*>( widget );

    if( _oldEditor ){

        disconnect( _oldEditor,SIGNAL(showCode(QString,int)),this,SLOT(onShowCode(QString,int)) );
    }

    if( _codeEditor ){

        replaceTabWidgetWidget( _browserTabWidget,1,_codeEditor->codeTreeView() );

        connect( _codeEditor,SIGNAL(showCode(QString,int)),SLOT(onShowCode(QString,int)) );

        _codeEditor->setFocus( Qt::OtherFocusReason );

        onCursorPositionChanged();

    }else{

        replaceTabWidgetWidget( _browserTabWidget,1,_emptyCodeWidget );
    }

    updateWindowTitle();

    updateActions();
}

void MainWindow::onDockVisibilityChanged( bool visible ){

    (void)visible;

    updateActions();
}

//Project browser...
//
void MainWindow::onProjectMenu( const QPoint &pos ){

    QModelIndex index=_projectTreeWidget->indexAt( pos );
    if( !index.isValid() ) return;

    QFileInfo info=_projectTreeModel->fileInfo( index );

    QMenu *menu=nullptr;

    if( _projectTreeModel->isProject( index ) ){
        menu=_projectPopupMenu;
    }else if( info.isFile() ){
        menu=_filePopupMenu;
        QString suffix=info.suffix().toLower();
        bool browsable=(suffix=="txt" || suffix=="htm" || suffix=="html");
        _ui->actionOpen_in_Help->setEnabled( browsable );
    }else{
        menu=_dirPopupMenu;
    }

    if( !menu ) return;

    QAction *action=menu->exec( _projectTreeWidget->mapToGlobal( pos ) );
    if( !action ) return;

    if( action==_ui->actionNewFile ){

        bool ok=false;
        QString name=QInputDialog::getText( this,"Create File","File name: "+info.filePath()+"/",QLineEdit::Normal,"",&ok );
        if( ok && !name.isEmpty() ){
            if( extractExt( name ).isEmpty() ) name+=".cxs";
            QString path=info.filePath()+"/"+name;
            if( QFileInfo( path ).exists() ){
                if( QMessageBox::question( this,"Create File","Okay to overwrite existing file: "+path+" ?",QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel )==QMessageBox::Ok ){
                    newFile( path );
                }
            }else{
                newFile( path );
            }
        }

    }else if( action==_ui->actionNewFolder ){

        bool ok=false;
        QString name=QInputDialog::getText( this,"Create Folder","Folder name: "+info.filePath()+"/",QLineEdit::Normal,"",&ok );
        if( ok && !name.isEmpty() ){
            if( !QDir( info.filePath() ).mkdir( name ) ){
                QMessageBox::warning( this,"Create Folder","Create folder failed" );
            }
        }

    }else if( action==_ui->actionRenameFile ){

        bool ok=false;
        QString newName=QInputDialog::getText( this,"Rename file","New name:",QLineEdit::Normal,info.fileName(),&ok );
        if( ok ){
            QString oldPath=info.filePath();
            QString newPath=info.path()+"/"+newName;
            if( QFile::rename( oldPath,newPath ) ){
                for( int i=0;i<_mainTabWidget->count();++i ){
                    if( CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){
                        if( editor->path()==oldPath ){
                            editor->rename( newPath );
                            updateTabLabel( editor );
                        }
                    }
                }
            }else{
                QMessageBox::warning( this,"Rename Error","Error renaming file: "+oldPath );
            }
        }
    }else if( action==_ui->actionOpen_on_Desktop ){

// DAWLANE -- Workaround for Qt file system issue on Linux
#ifdef Q_OS_WIN
        QDesktopServices::openUrl( "file:/"+info.filePath() );
#else
        QDesktopServices::openUrl( "file://"+info.filePath() );
#endif


    }else if( action==_ui->actionOpen_in_Help ){
// DAWLANE -- Workaround for Qt file system issue on Linux
#ifdef Q_OS_WIN
        openFile( "file:/"+info.filePath(),false );
#else
        //qDebug()<<info.filePath();
        openFile( "file://"+info.filePath(),false );      
#endif
    }else if( action==_ui->actionDeleteFile ){

        QString path=info.filePath();

        if( info.isDir() ){
            if( QMessageBox::question( this,"Delete file","Okay to delete directory: "+path+" ?\n\n*** WARNING *** all subdirectories will also be deleted!",QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel )==QMessageBox::Ok ){
                if( !removeDir( path ) ){
                    QMessageBox::warning( this,"Delete Error","Error deleting directory: "+info.filePath() );
                }
            }
        }else{
            if( QMessageBox::question( this,"Delete file","Okay to delete file: "+path+" ?",QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel )==QMessageBox::Ok ){
                if( QFile::remove( path ) ){
                    for( int i=0;i<_mainTabWidget->count();++i ){
                        if( CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){
                            if( editor->path()==path ){
                                closeFile( editor );
                                i=-1;
                            }
                        }
                    }
                }else{
                    QMessageBox::warning( this,"Delete Error","Error deleting file: "+info.filePath() );
                }
            }
        }
    }else if( action==_ui->actionCloseProject ){

// DAWLANE -- Workaround for Qt file system issue on Linux
#ifdef Q_OS_WIN
        _projectTreeModel->removeProject( info.filePath() );
#else
        QString f;
        if( info.filePath().startsWith( "//" ) ){
            f=info.filePath().replace( "//", "/" );
        }else{
            f=info.filePath();
        }
        _projectTreeModel->removeProject( f );
#endif
    }else if( action==_ui->actionEditFindInFiles ){

        _findInFilesDialog->show( info.filePath() );

        _findInFilesDialog->raise();

    }
}

void MainWindow::onFileClicked( const QModelIndex &index ){

    if( !_projectTreeModel->isDir( index ) ) openFile( _projectTreeModel->filePath( index ),true );
}

//Editor...
//
void MainWindow::onTextChanged(){
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( sender() ) ){
        if( editor->modified()<2 ){
            updateTabLabel( editor );
        }
    }
    updateActions();
}

void MainWindow::onCursorPositionChanged(){
    if( sender()==_codeEditor ){
        _statusWidget->setText( "Line: "+QString::number( _codeEditor->textCursor().blockNumber()+1 ) );
    }
    updateActions();
}

void MainWindow::onShowCode( const QString &path,int line ){
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( openFile( path,true ) ) ){
        //
        editor->gotoLine( line );
        editor->highlightLine( line );
        //
        if( editor==_codeEditor ) editor->setFocus( Qt::OtherFocusReason );
    }
}

void MainWindow::onShowCode( const QString &path,int pos,int len ){
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( openFile( path,true ) ) ){
        //
        QTextCursor cursor( editor->document() );
        cursor.setPosition( pos );
        cursor.setPosition( pos+len,QTextCursor::KeepAnchor );
        editor->setTextCursor( cursor );
        //
        if( editor==_codeEditor ) editor->setFocus( Qt::OtherFocusReason );
    }
}

//Console...
//
void MainWindow::print( const QString &str ){
    QTextCursor cursor=_consoleTextWidget->textCursor();
    cursor.insertText( str );
    cursor.insertBlock();
    cursor.movePosition( QTextCursor::End,QTextCursor::MoveAnchor );
    _consoleTextWidget->setTextCursor( cursor );
    //_consoleTextWidget->insertPlainText( str+"\n" );
    //_consoleTextWidget->append( str );
}

void MainWindow::cdebug( const QString &str ){
    _consoleTextWidget->setTextColor( Prefs::prefs()->getColor( "console3Color" ) );
    //_consoleTextWidget->setTextColor( QColor( 128,0,128 ) );
    print( str );
}

void MainWindow::runCommand( QString cmd,QWidget *fileWidget ){

    cmd=cmd.replace( "${TARGET}",_targetsWidget->currentText().replace( ' ','_' ) );
    cmd=cmd.replace( "${CONFIG}",_configsWidget->currentText() );
    cmd=cmd.replace( "${CERBERUSPATH}",_cerberusPath );
    cmd=cmd.replace( "${MONKEY2PATH}",_monkey2Path );
    cmd=cmd.replace( "${BLITZMAXPATH}",_blitzmaxPath );
    if( fileWidget ) cmd=cmd.replace( "${FILEPATH}",widgetPath( fileWidget ) );

    _consoleProc=new Process;

    connect( _consoleProc,SIGNAL(lineAvailable(int)),SLOT(onProcLineAvailable(int)) );//,Qt::QueuedConnection );
    connect( _consoleProc,SIGNAL(finished()),SLOT(onProcFinished()) );//,Qt::QueuedConnection );

    _consoleTextWidget->clear();
    _consoleDockWidget->show();
    _consoleTextWidget->setTextColor( Prefs::prefs()->getColor( "console1Color" ) );
    print( cmd );

    if( !_consoleProc->start( cmd ) ){
        delete _consoleProc;
        _consoleProc=nullptr;
        QMessageBox::warning( this,"Process Error","Failed to start process: "+cmd );
        return;
    }

    updateActions();
}

void MainWindow::onProcStdout(){

    static QString comerr=" : Error : ";
    static QString runerr="Cerberus Runtime Error : ";

    QString text=_consoleProc->readLine( 0 );

    _consoleTextWidget->setTextColor( Prefs::prefs()->getColor( "console2Color" ) );
    print( text );

    if( text.contains( comerr )){
        int i0=text.indexOf( comerr );
        QString info=text.left( i0 );
        int i=info.lastIndexOf( '<' );
        if( i!=-1 && info.endsWith( '>' ) ){
            QString path=info.left( i );
            int line=info.mid( i+1,info.length()-i-2 ).toInt()-1;
            QString err=text.mid( i0+comerr.length() );

            onShowCode( path,line );

            QMessageBox::warning( this,"Compile Error",err );
        }
    }else if( text.startsWith( runerr ) ){
        QString err=text.mid( runerr.length() );

        //not sure what this voodoo is for...!
        showNormal();
        raise();
        activateWindow();
        QMessageBox::warning( this,"Cerberus Runtime Error",err );
    }
}

void MainWindow::onProcStderr(){

    if( _debugTreeModel && _debugTreeModel->stopped() ) return;

    QString text=_consoleProc->readLine( 1 );

    if( text.startsWith( "{{~~" ) && text.endsWith( "~~}}" ) ){

        QString info=text.mid( 4,text.length()-8 );

        int i=info.lastIndexOf( '<' );
        if( i!=-1 && info.endsWith( '>' ) ){
            QString path=info.left( i );
            int line=info.mid( i+1,info.length()-i-2 ).toInt()-1;
            onShowCode( path,line );
        }else{
            _consoleTextWidget->setTextColor( Prefs::prefs()->getColor( "console3Color" ) );
            print( info );
        }

        if( !_debugTreeModel ){

            raise();

            _debugTreeModel=new DebugTreeModel( _consoleProc );
            connect( _debugTreeModel,SIGNAL(showCode(QString,int)),SLOT(onShowCode(QString,int)) );

            _debugTreeWidget->setModel( _debugTreeModel );
            connect( _debugTreeWidget,SIGNAL(clicked(const QModelIndex&)),_debugTreeModel,SLOT(onClicked(const QModelIndex&)) );

            _browserTabWidget->setCurrentWidget( _debugTreeWidget );

            _consoleTextWidget->setTextColor( QColor( 192,96,0 ) );
            print( "STOPPED" );
        }

        _debugTreeModel->stop();

        updateActions();

        return;
    }

    _consoleTextWidget->setTextColor( Prefs::prefs()->getColor( "stringsColor" ) );
    print( text );
}

void MainWindow::onProcLineAvailable( int channel ){

    (void)channel;

    while( _consoleProc ){
        if( _consoleProc->isLineAvailable( 0 ) ){
            onProcStdout();
        }else if( _consoleProc->isLineAvailable( 1 ) ){
            onProcStderr();
        }else{
            return;
        }
    }
/*
    if( channel==0 ){
        onProcStdout();
    }else if( channel==1 ){
        onProcStderr();
    }
*/
}

void MainWindow::onProcFinished(){

    while( _consoleProc->waitLineAvailable( 0,100 ) ){
        onProcLineAvailable( 0 );
    }

    _consoleTextWidget->setTextColor( QColor( Prefs::prefs()->getColor( "console4Color" ) ) );
    print( "Done." );

    if( _rebuildingHelp ){
        _rebuildingHelp=false;
        loadHelpIndex();
        for( int i=0;i<_mainTabWidget->count();++i ){
            HelpView *helpView=qobject_cast<HelpView*>( _mainTabWidget->widget( i ) );
// DAWLANE Qt 5.6+ supported
#if QT_VERSION>0x050501
            if( helpView ) helpView->triggerPageAction( QWebEnginePage::ReloadAndBypassCache );
#else
            if( helpView ) helpView->triggerPageAction( QWebPage::ReloadAndBypassCache );
#endif
        }
        onHelpHome();
    }

    if( _debugTreeModel ){
        _debugTreeWidget->setModel( nullptr );
        delete _debugTreeModel;
        _debugTreeModel=nullptr;
    }

    if( _consoleProc ){
        delete _consoleProc;
        _consoleProc=nullptr;
    }

    updateActions();

    statusBar()->showMessage( "Ready." );
}

void MainWindow::build( QString mode ){

    CodeEditor *editor=_lockedEditor ? _lockedEditor : _codeEditor;
    if( !isBuildable( editor ) ) return;

    QString filePath=editor->path();
    if( filePath.isEmpty() ) return;

    QString cmd,msg="Building: "+filePath+"...";

    if( editor->fileType()=="cxs" || editor->fileType()=="monkey" ){
        if( mode=="run" ){
            cmd="\"${CERBERUSPATH}/bin/transcc"+HOST+"\" -target=${TARGET} -config=${CONFIG} -run \"${FILEPATH}\"";
        }else if( mode=="build" ){
            cmd="\"${CERBERUSPATH}/bin/transcc"+HOST+"\" -target=${TARGET} -config=${CONFIG} \"${FILEPATH}\"";
        }else if( mode=="update" ){
            cmd="\"${CERBERUSPATH}/bin/transcc"+HOST+"\" -target=${TARGET} -config=${CONFIG} -update \"${FILEPATH}\"";
            msg="Updating: "+filePath+"...";
        }else if( mode=="check" ){
            cmd="\"${CERBERUSPATH}/bin/transcc"+HOST+"\" -target=${TARGET} -config=${CONFIG} -check \"${FILEPATH}\"";
            msg="Checking: "+filePath+"...";
        }
    }else if( editor->fileType()=="bmx" ){
        if( mode=="run" ){
            cmd="\"${BLITZMAXPATH}/bin/bmk\" makeapp -a -r -x \"${FILEPATH}\"";
        }else if( mode=="build" ){
            cmd="\"${BLITZMAXPATH}/bin/bmk\" makeapp -a -x \"${FILEPATH}\"";
        }
    }else if( editor->fileType()=="monkey2" ){
        if( mode=="run" ){
            QString mx2="mx2cc";
#ifdef Q_OS_WIN
            mx2+="_windows";
#else
            mx2+=HOST;
#endif
            cmd="\"${MONKEY2PATH}/bin/"+mx2+"\" makeapp -target=${TARGET} -config=${CONFIG} \"${FILEPATH}\"";
        }
    }

    if( !cmd.length() ) return;

    onFileSaveAll();

    statusBar()->showMessage( msg );

    runCommand( cmd,editor );
}

//***** File menu *****

void MainWindow::onFileNew(){
    newFile( "" );
}
void MainWindow::onFileNewTemplate(){
    QString appPath=QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    appPath = extractDir(extractDir(extractDir(appPath)));
#endif

    if( QAction *action=qobject_cast<QAction*>( sender() ) ){
        newFileTemplate(appPath+"/templates/"+action->text()+".cxs");
    }
}

void MainWindow::onFileOpen(){
    openFile( "",true );
}

void MainWindow::onFileOpenRecent(){
    if( QAction *action=qobject_cast<QAction*>( sender() ) ){
        openFile( action->text(),false );
    }

}

void MainWindow::onFileClose(){
    closeFile( _mainTabWidget->currentWidget() );
}

void MainWindow::onFileCloseAll(){
    for(;;){
        int i;
        CodeEditor *editor=nullptr;
        for( i=0;i<_mainTabWidget->count();++i ){
            editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) );
            if( editor ) break; 
        }
        if( !editor ) return;

        if( !closeFile( editor ) ) return;
    }
}

void MainWindow::onFileCloseOthers(){
    if( _helpWidget ) return onFileCloseAll();
    if( !_codeEditor ) return;

    for(;;){
        int i;
        CodeEditor *editor=nullptr;
        for( i=0;i<_mainTabWidget->count();++i ){
            editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) );
            if( editor && editor!=_codeEditor ) break;
            editor=nullptr;
        }
        if( !editor ) return;

        if( !closeFile( editor ) ) return;
    }
}

void MainWindow::onFileSave(){
    if( !_codeEditor ) return;

    saveFile( _codeEditor,_codeEditor->path() );
}

void MainWindow::onFileSaveAs(){
    if( !_codeEditor ) return;

    saveFile( _codeEditor,"" );
}

void MainWindow::onFileSaveAll(){
    for( int i=0;i<_mainTabWidget->count();++i ){
        CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) );
        if( editor && !saveFile( editor,editor->path() ) ) return;
    }
}

void MainWindow::onFileNext(){
    if( _mainTabWidget->count()<2 ) return;

    int i=_mainTabWidget->currentIndex()+1;
    if( i>=_mainTabWidget->count() ) i=0;

    _mainTabWidget->setCurrentIndex( i );
}

void MainWindow::onFilePrevious(){
    if( _mainTabWidget->count()<2 ) return;

    int i=_mainTabWidget->currentIndex()-1;
    if( i<0 ) i=_mainTabWidget->count()-1;

    _mainTabWidget->setCurrentIndex( i );
}

void MainWindow::onFilePrefs(){

    _prefsDialog->setModal( true );

    _prefsDialog->exec();

    Prefs *prefs=Prefs::prefs();

    QString path=prefs->getString( "cerberusPath" );
    if( path!=_cerberusPath ){
        if( isValidCerberusPath( path ) ){
            _cerberusPath=path;
            enumTargets();
        }else{
            prefs->setValue( "cerberusPath",_cerberusPath );
            QMessageBox::warning( this,"Tool Path Error","Invalid Cerberus Path" );
        }
    }

    path=prefs->getString( "blitzmaxPath" );
    if( path!=_blitzmaxPath ){
        if( isValidBlitzmaxPath( path ) ){
            _blitzmaxPath=path;
        }else{
            prefs->setValue( "blitzmaxPath",_blitzmaxPath );
            QMessageBox::warning( this,"Tool Path Error","Invalid BlitzMax Path" );
        }
    }

    updateActions();
    setIcons();
}

void MainWindow::onFileQuit(){
    if( confirmQuit() ) CerberusApplication::quit();
}

//***** Edit menu *****

void MainWindow::onEditUndo(){
    if( !_codeEditor ) return;

    _codeEditor->undo();
}

void MainWindow::onEditRedo(){
    if( !_codeEditor ) return;

    _codeEditor->redo();
}

void MainWindow::onEditCut(){
    if( !_codeEditor ) return;

    _codeEditor->cut();
}

void MainWindow::onEditCopy(){
    if( !_codeEditor ) return;

    _codeEditor->copy();
}

void MainWindow::onEditorMenu(const QPoint &pos) {
    _editorPopupMenu->exec( _codeEditor->mapToGlobal( pos ) );
}

void MainWindow::onEditPaste(){
    if( !_codeEditor ) return;

    _codeEditor->paste();
}

void MainWindow::onEditDelete(){
    if( !_codeEditor ) return;

    _codeEditor->textCursor().removeSelectedText();
}

void MainWindow::onEditCommentUncommentBlock() {
    if( !_codeEditor ) return;
    _codeEditor->commentUncommentBlock();
}

void MainWindow::onToggleBookmark() {
    if( _codeEditor )
        _codeEditor->bookmarkToggle();
}

void MainWindow::onPreviousBookmark() {
    if( _codeEditor )
        _codeEditor->bookmarkPrev();
}

void MainWindow::onNextBookmark() {
    if( _codeEditor )
        _codeEditor->bookmarkNext();
}

void MainWindow::onEditSelectAll(){
    if( !_codeEditor ) return;

    _codeEditor->selectAll();

    updateActions();
}

void MainWindow::onEditFind(){
    if( !_codeEditor ) return;
//qDebug() << _codeEditor->identAtCursor();
//qDebug() << _codeEditor->textCursor().selectedText();
    _findDialog->setModal( true );

    _findDialog->exec(_codeEditor->textCursor().selectedText());
}

void MainWindow::onEditFindNext(){
    if( !_codeEditor ) return;

    onFindReplace( 0 );
}

void MainWindow::onFindReplace( int how ){
    if( !_codeEditor ) return;

    QString findText=_findDialog->findText();
    if( findText.isEmpty() ) return;

    QString replaceText=_findDialog->replaceText();

    bool cased=_findDialog->caseSensitive();

    bool wrap=true;

    if( how==0 ){

        if( !_codeEditor->findNext( findText,cased,wrap ) ){
            CerberusApplication::beep();
//            QMessageBox::information( this,"Find Next","Text not found" );
//            _findDialog->activateWindow();
        }

    }else if( how==1 ){

        if( _codeEditor->replace( findText,replaceText,cased ) ){
            if( !_codeEditor->findNext( findText,cased,wrap ) ){
                CerberusApplication::beep();
//                QMessageBox::information( this,"Replace","Text not found" );
//                _findDialog->activateWindow();
            }
        }


    }else if( how==2 ){

        int n=_codeEditor->replaceAll( findText,replaceText,cased,wrap );

        QMessageBox::information( this,"Replace All",QString::number(n)+" occurences replaced" );
    }
}

void MainWindow::onEditGoto(){
    if( !_codeEditor ) return;

    bool ok=false;
    int line=QInputDialog::getInt( this,"Go to Line","Line number:",1,1,_codeEditor->document()->blockCount(),1,&ok );
    if( ok ){
        _codeEditor->gotoLine( line-1 );
        _codeEditor->highlightLine( line-1 );
    }
}

void MainWindow::onEditFindInFiles(){

    _findInFilesDialog->show();

    _findInFilesDialog->raise();
}

//***** View menu *****

void MainWindow::onViewToolBar(){
    if( sender()==_ui->actionViewFile ){
        _ui->fileToolBar->setVisible( _ui->actionViewFile->isChecked() );
    }else if( sender()==_ui->actionViewEdit ){
        _ui->editToolBar->setVisible( _ui->actionViewEdit->isChecked() );
    }else if( sender()==_ui->actionViewBuild ){
        _ui->buildToolBar->setVisible( _ui->actionViewBuild->isChecked() );
    }else if( sender()==_ui->actionViewHelp ){
        _ui->helpToolBar->setVisible( _ui->actionViewHelp->isChecked() );
    }
}

void MainWindow::onViewWindow(){
    if( sender()==_ui->actionViewBrowser ){
        _browserDockWidget->setVisible( _ui->actionViewBrowser->isChecked() );
    }else if( sender()==_ui->actionViewConsole ){
        _consoleDockWidget->setVisible( _ui->actionViewConsole->isChecked() );
    }
}

void MainWindow::onToggleFullscreen() {
    if(windowState() != Qt::WindowFullScreen){
        this->_windowState = windowState();
        this->setWindowState(Qt::WindowFullScreen);
    }
    else
        //this->setWindowState(Qt::WindowActive);
        this->setWindowState(this->_windowState);
}

//***** Build menu *****

void MainWindow::onBuildBuild(){
    build( "build" );
}

void MainWindow::onBuildRun(){
    if( _debugTreeModel ){
        _debugTreeModel->run();
    }else{
        build( "run" );
    }
}

void MainWindow::onBuildCheck(){
    build( "check" );
}

void MainWindow::onBuildUpdate(){
    build( "update" );
}

void MainWindow::onDebugStep(){
    if( !_debugTreeModel ) return;
    _debugTreeModel->step();
}

void MainWindow::onDebugStepInto(){
    if( !_debugTreeModel ) return;
    _debugTreeModel->stepInto();
}

void MainWindow::onDebugStepOut(){
    if( !_debugTreeModel ) return;
    _debugTreeModel->stepOut();
}

void MainWindow::onDebugKill(){
    if( !_consoleProc ) return;

    _consoleTextWidget->setTextColor( Prefs::prefs()->getColor( "console4Color" ) );
    print( "Killing process..." );

    _consoleProc->kill();
}

void MainWindow::onBuildTarget(){

    QStringList items;
    for( int i=0;i<_targetsWidget->count();++i ){
        items.push_back( _targetsWidget->itemText( i ) );
    }

    bool ok=false;
    QString item=QInputDialog::getItem( this,"Select build target","Build target:",items,_targetsWidget->currentIndex(),false,&ok );
    if( ok ){
        int index=items.indexOf( item );
        if( index!=-1 ) _targetsWidget->setCurrentIndex( index );
    }
}

void MainWindow::onBuildConfig(){

    QStringList items;
    for( int i=0;i<_configsWidget->count();++i ){
        items.push_back( _configsWidget->itemText( i ) );
    }

    bool ok=false;
    QString item=QInputDialog::getItem( this,"Select build config","Build config:",items,_configsWidget->currentIndex(),false,&ok );
    if( ok ){
        int index=items.indexOf( item );
        if( index!=-1 ) _configsWidget->setCurrentIndex( index );
    }
}

void MainWindow::onBuildLockFile(){
    if( _codeEditor && _codeEditor!=_lockedEditor ){
        CodeEditor *wasLocked=_lockedEditor;
        _lockedEditor=_codeEditor;
        updateTabLabel( _lockedEditor );
        if( wasLocked ) updateTabLabel( wasLocked );
    }
    updateActions();
}

void MainWindow::onBuildUnlockFile(){
    if( CodeEditor *wasLocked=_lockedEditor ){
        _lockedEditor=nullptr;
        updateTabLabel( wasLocked );
    }
    updateActions();
}

void MainWindow::onBuildAddProject(){

    QString dir=fixPath( QFileDialog::getExistingDirectory( this,"Select project directory",_defaultDir,QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks ) );
    if( dir.isEmpty() ) return;

    if( !_projectTreeModel->addProject( dir ) ){
        QMessageBox::warning( this,"Add Project Error","Error adding project: "+dir );
    }
}

//***** Help menu *****

void MainWindow::updateHelp(){
    QString home=_cerberusPath+"/docs/html/Home.html";
    if( !QFile::exists( home ) ){
        if( QMessageBox::question( this,"Rebuild cerberus docs","Cerberus documentation not found - rebuild docs?",QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Ok )==QMessageBox::Ok ){
            onHelpRebuild();
        }
    }
}

void MainWindow::onHelpHome(){
    QString home=_cerberusPath+"/docs/html/Home.html";
    if( !QFile::exists( home ) ) return;
#ifdef Q_OS_WIN
    openFile( "file:/"+home,false );
#else
    openFile( "file://"+home,false );
#endif

}

void MainWindow::onHelpBack(){
    if( !_helpWidget ) return;

    _helpWidget->back();
}

void MainWindow::onHelpForward(){
    if( !_helpWidget ) return;

    _helpWidget->forward();
}

void MainWindow::onHelpQuickHelp(){
    if( !_codeEditor ) return;

    QString ident=_codeEditor->identAtCursor();
    if( ident.isEmpty() ) return;

    onShowHelpWithStatusbar( ident );
}

void MainWindow::onHelpCerberusHomepage() {
    QString s = "https://www.cerberus-x.com/";
    QDesktopServices::openUrl( s );
}

void MainWindow::onHelpAbout(){

    QString CERBERUS_VERSION="?????";

    QFile file( _cerberusPath+"/VERSIONS.TXT" );
    if( file.open( QIODevice::ReadOnly ) ){
        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        QString text=stream.readAll();
        file.close();
        QStringList lines=text.split('\n');
        for( int i=0;i<lines.count();++i ){
            QString line=lines.at( i ).trimmed();
            if( line.startsWith( "***** v") ){
                QString v=line.mid( 7 );
                int j=v.indexOf( " *****" );
                if( j+6==v.length() ){
                    CERBERUS_VERSION=v.left( j );
                    break;
                }
            }
        }
    }
    QString webSite = "https://www.cerberus-x.com";
// DAWLANE - Fixed the Correct displaying of the Qt version.
#if QT_VERSION<0x050001
	QString ABOUT= "<html><head><style>a{color:#FFEE00;}</style></head><body>"
            "Ted V" TED_VERSION "<br><br>"
            "Cerberus V" +CERBERUS_VERSION+ "<br>"
            "Trans V"+ _transVersion +"<br>"
            "QT V" +_STRINGIZE(QT_VERSION)+ "<br><br>"
            "A simple editor/IDE for the Cerberus programming language.<br><br>"
            "Copyright Blitz Research Ltd for Monkey X.<br><br>"
            "Cerberus X is maintained by Michael Hartlef, Martin Leidel & Olivier Stucker.<br<br>"
            "Further additions done by serveral member of the Cerberus X community.<br>"
            "Please visit <a href=\""+webSite+"\">www.cerberus-x.com</a> for more information on Cerberus X."
            "</body></html>";   
#else
	QString ABOUT= "<html><head><style>a{color:#FFEE00;}</style></head><body>"
            "Ted V" TED_VERSION "<br><br>"
            "Cerberus V" +CERBERUS_VERSION+ "<br>"
            "Trans V"+ _transVersion +"<br>"
            "QT V" +(QT_VERSION_STR)+"<br><br>"
            "A simple editor/IDE for the Cerberus programming language.<br><br>"
            "Copyright Blitz Research Ltd for Monkey X.<br><br>"
            "Cerberus X is maintained by Michael Hartlef, Martin Leidel & Olivier Stucker.<br<br>"
            "Further additions done by serveral member of the Cerberus X community.<br>"
            "Please visit <a href=\""+webSite+"\">www.cerberus-x.com</a> for more information on Cerberus X."
            "</body></html>";
#endif

    QMessageBox::information( this,"About Ted",ABOUT );
}

void MainWindow::onShowHelp(){

    if( _helpTopic.isEmpty() ) return;

    if( !_helpTopicId ) _helpTopicId=1;
    ++_helpTopicId;

    QString tmp=_helpTopic+"("+QString::number( _helpTopicId )+")";
    QString url=_helpUrls.value( tmp );

    if( url.isEmpty() ){

        url=_helpUrls.value( _helpTopic );
        if( url.isEmpty() ){
            _helpTopic="";
            return;
        }
        _helpTopicId=0;
    }

    openFile( url,false );
}

void MainWindow::onShowHelp( const QString &topic ){
    QString url=_helpUrls.value( topic );
    QString status = _helpF1.value( topic );

    if( url.isEmpty() ){
        _helpTopic="";
        return;
    }

    _helpTopic=topic;
    _helpTopicId=0;

    openFile( url,false );
}

void MainWindow::onShowHelpWithStatusbar( const QString &topic ){
    QString url=_helpUrls.value( topic );
    QString status = _helpF1.value( topic );

    if ( !status.isEmpty() && _helpTopic!=topic) {
        statusBar()->showMessage( "Help->  " + status );
        _helpTopic=topic;
        return;
    }

    if( url.isEmpty() ){
        _helpTopic="";
        return;
    }

    _helpTopic=topic;
    _helpTopicId=0;

    openFile( url,false );
}

// DAWLANE - QtWebEnginge/Page uses a different way to call hypertext links.
#if QT_VERSION>0x050501
bool WebEnginePage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame )
{
    // Capture links that have been clicked only in the main frame. Embedded elements will be child frames within a QWebEnginePage.
    if ( (type == QWebEnginePage::NavigationTypeLinkClicked)  && (isMainFrame)  )
    {
        //  Convert the url all to lower case to deal with caputing local files.
        QString str=url.toString();
        if( (str.startsWith( "file:///", Qt::CaseInsensitive ))){

            // If our link leads to one of the recognised source or text files, then strip off the file tag
            // and call the open file method in the main window class to load it into the editor
            QString ext=";"+extractExt(str)+";";
            if( textFileTypes.contains( ext.toLower() ) || codeFileTypes.contains( ext.toLower() ) ){
#ifdef Q_OS_WIN
                CerberusApplication::mainWindow->openFile( str.mid(8),false );
#else
                CerberusApplication::mainWindow->openFile( str.mid(7),false );
#endif
                return false;
            }

            // Our file isn't one of the main types, so try to open it any way.
            CerberusApplication::mainWindow->openFile( str,false );
            return false;
        }

        // Our file isn't local, so try to open this is a web browser rather than in the QWebEngineView.
        QDesktopServices::openUrl( str );
        return false;
    }
    return true;
}
#else
void MainWindow::onLinkClicked( const QUrl &url ){

    QString str=url.toString();
    /* DAWLANE -- Trap files that need to be opened in the editor.
                  Replaced the sub string slicing with string replace due to inconsistent paths.
    */
    if( str.startsWith( "file:///", Qt::CaseInsensitive ) ){
        QString ext=";"+extractExt(str)+";";
        if( textFileTypes.contains( ext.toLower() ) || codeFileTypes.contains( ext.toLower() ) ){
#ifdef Q_OS_WIN
                CerberusApplication::mainWindow->openFile( str.mid(8),false );
#else
                CerberusApplication::mainWindow->openFile( str.mid(7),false );
#endif
            return;
         }
         CerberusApplication::mainWindow->openFile( str,false );
         return;
    }

            QDesktopServices::openUrl( str );
}
#endif

void MainWindow::onHelpRebuild(){
    if( _consoleProc || _cerberusPath.isEmpty() ) return;

    onFileSaveAll();

    QString cmd="\"${CERBERUSPATH}/bin/makedocs"+HOST+"\"";

    _rebuildingHelp=true;

    runCommand( cmd,nullptr );
}

void HelpView::keyPressEvent ( QKeyEvent * event ){
    if( event->key()==Qt::Key_F1 ){
        CerberusApplication::mainWindow->onShowHelp();
    }
}

