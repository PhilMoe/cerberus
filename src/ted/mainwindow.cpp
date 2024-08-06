///----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// CONTRIBUTORS: See contributors.txt
// Changes:
//          2023-02-10: DAWLANE
//                      Updated and fixed MacOS side so the it will build against later versions of Qt.
//          2022-12-26: DAWLANE
//                      Fixed issue with restoring previous state after exiting full screen mode?
//          2022-12-23: DAWLANE
//                      Fixed the crash when quitting via menu.
//                      Fixed A MS Windows issue with menus in full screen mode.
//                      Overrides the main window event to force a one pixel boarder around a full screen window.
//          2022-10-20: DAWLANE
//                      Overhaul of code to remove Qt4 supported code.
//                      Restructured member functions to match main window header order.
//                      Updated to work with Qt5.9+ and Qt6.2+.
//                      Moved the WebEngineView and WebEnginePage into it's own file class module.
//                      Updated where possible signal and slot connect to use the function pointer variant.

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "application.h"
#include "codeeditor.h"
#include "debugtreemodel.h"
#include "finddialog.h"
#include "findinfilesdialog.h"
#include "helpview.h"
#include "macros.h"
#include "prefs.h"
#include "prefsdialog.h"
#include "process.h"
#include "projecttreemodel.h"

#include <QDesktopServices>
#include <QDockWidget>
#include <QFileDialog>
#include <QHostInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#define TED_VERSION "2023-02-14"

#define SETTINGS_VERSION 2

//----------------------------------------------------------------------------------------------------------------------
//  Set up some macros for URL and HOST target applications
//----------------------------------------------------------------------------------------------------------------------
#ifdef Q_OS_WIN
    #define HOST QString("_winnt")
    #define FILE_URL "file:/"
#elif defined(Q_OS_MAC)
    #define HOST QString("_macos")
    #define FILE_URL "file://"
#elif defined(Q_OS_LINUX)
    #define HOST QString("_linux")
    #define FILE_URL "file://"
#else
    #define HOST QString("")
#endif

//----------------------------------------------------------------------------------------------------------------------
// The main window object is accessed via global variable. Include main window.h in any implementation file that needs
// access to the public member functions of the MainWindow class.
MainWindow *MAIN_WINDOW = nullptr;

void cdebug(const QString &q)
{
    if(MAIN_WINDOW)
        MAIN_WINDOW->cdebug(q);
}

//----------------------------------------------------------------------------------------------------------------------
//  MainWindow: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), _ui(new Ui::MainWindow)
{
    MAIN_WINDOW = this; // Set this instance to the global variable.
#ifdef Q_OS_MAC
    QCoreApplication::instance()->setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    QCoreApplication::setOrganizationName("Cerberus X");
    QCoreApplication::setOrganizationDomain("cerberus-x.com");
    QCoreApplication::setApplicationName("Ted");

    QString comp = QHostInfo::localHostName();

    // TODO: Figure out a way to store and change files when the Cerberus directory is located in a restricted area
    // for example on MacOS /Applications. The issue is when it comes to using makedocs expecting that it's to find
    // all files in the Cerberus directory.
    QString cfgPath;
#ifdef Q_OS_MAC
    cfgPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    cfgPath = QCoreApplication::applicationDirPath();
#endif
    cfgPath += "/ted" + HOST + "_" + comp + ".ini";

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, cfgPath);

    // NOTE: In Qt6 the static defaultSettings function was removed, but QWebEngineProfile is available in both Qt5 and
    // Qt6, so QWebEngineProfile::defaultProfile() should work for both. If it doesn't, then uncomment the lines to
    // reinstate.
    // #if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    //    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    // #else
    QWebEngineSettings *web_settings = QWebEngineProfile::defaultProfile()->settings();
    web_settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    // #endif
    _ui->setupUi(this);

    _helpTopicId = 0;
    _rebuildingHelp = false;

    //  Set the docking locations options
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Initialise the status bar widget
    _statusWidget = new QLabel;
    statusBar()->addPermanentWidget(_statusWidget);

    // Initialise the targets combo-box and append to the build toolbar.
    _targetsWidget = new QComboBox;
    _targetsWidget->setSizeAdjustPolicy(QComboBox::AdjustToContents);
#ifdef Q_OS_MAC
    _targetsWidget->setObjectName("cbTarget");
    _targetsWidget->view()->setObjectName("cbTargetView");
#endif
    _ui->buildToolBar->addWidget(_targetsWidget);

    _configsWidget = new QComboBox;
    _configsWidget->addItem("Debug");
    _configsWidget->addItem("Release");
#ifdef Q_OS_MAC
    _configsWidget->setObjectName("cbConfig");
    _configsWidget->view()->setObjectName("cbConfigView");
#endif
    _ui->buildToolBar->addWidget(_configsWidget);

    // initialise the lookup help combo-box and append to the help toolbar.
    _indexWidget = new QComboBox;
    _indexWidget->setEditable(true);
    _indexWidget->setInsertPolicy(QComboBox::NoInsert);
    _indexWidget->setMinimumSize(80, _indexWidget->minimumHeight());
    _indexWidget->setMaximumSize(240, _indexWidget->maximumHeight());
#ifdef Q_OS_MAC
    _indexWidget->setObjectName("cbIndex");
    _indexWidget->view()->setObjectName("cbIndexView");
#endif
    _ui->helpToolBar->addWidget(_indexWidget);

    // Initialise the central tab widget.
    _mainTabWidget = new QTabWidget;
    _mainTabWidget->setMovable(true);
    _mainTabWidget->setTabsClosable(true);
#ifdef Q_OS_MAC
    //    _mainTabWidget->setDocumentMode( true );
#endif
    setCentralWidget(_mainTabWidget);
    connect(_mainTabWidget, &QTabWidget::currentChanged, this, &MainWindow::onMainTabChanged);
    connect(_mainTabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onCloseMainTab);

    // Initialise console widget.
    _consoleProc = nullptr;
    _consoleTextWidget = new QTextEdit;
    _consoleTextWidget->setReadOnly(true);

#ifdef Q_OS_WIN
    _consoleTextWidget->setFrameStyle(QFrame::NoFrame);
#endif

    _consoleDockWidget = new QDockWidget;
    _consoleDockWidget->setObjectName("consoleDockWidget");
    _consoleDockWidget->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    _consoleDockWidget->setWindowTitle("Console");
    _consoleDockWidget->setWidget(_consoleTextWidget);
    addDockWidget(Qt::BottomDockWidgetArea, _consoleDockWidget);
    connect(_consoleDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::onDockVisibilityChanged);

    // Initialise browser widgets
    _projectTreeModel = new ProjectTreeModel;
    _projectTreeWidget = new QTreeView;
    _projectTreeWidget->setModel(_projectTreeModel);
    _projectTreeWidget->setHeaderHidden(true);

    _projectTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
#ifdef Q_OS_WIN
    _projectTreeWidget->setFrameStyle(QFrame::NoFrame);
#endif
    _projectTreeWidget->setFocusPolicy(Qt::NoFocus);
    connect(_projectTreeWidget, &QTreeView::doubleClicked, this, &MainWindow::onFileClicked);
    connect(_projectTreeWidget, &QTreeView::customContextMenuRequested, this, &MainWindow::onProjectMenu);

    _emptyCodeWidget = new QWidget;
    _emptyCodeWidget->setFocusPolicy(Qt::NoFocus);

    _debugTreeModel = nullptr;
    _debugTreeWidget = new QTreeView;
    _debugTreeWidget->setHeaderHidden(true);
    _debugTreeWidget->setFocusPolicy(Qt::NoFocus);
#ifdef Q_OS_WIN
    _debugTreeWidget->setFrameStyle(QFrame::NoFrame);
#endif

    _browserTabWidget = new QTabWidget;
    _browserTabWidget->addTab(_projectTreeWidget, "Projects");
    _browserTabWidget->addTab(_emptyCodeWidget, "Code");
    _browserTabWidget->addTab(_debugTreeWidget, "Debug");
#ifdef Q_OS_MAC
    //_browserTabWidget->setDocumentMode( true );
#endif

    _browserDockWidget = new QDockWidget;
    _browserDockWidget->setObjectName("browserDockWidget");
    _browserDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    _browserDockWidget->setWindowTitle("Browser");
    _browserDockWidget->setWidget(_browserTabWidget);

    addDockWidget(Qt::RightDockWidgetArea, _browserDockWidget);
    connect(_browserDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::onDockVisibilityChanged);

    // Set up keyboard short cuts for next/previous tab selection
    QList<QKeySequence> shortcuts;
    QList<QKeySequence> shortcuts2;
#ifdef Q_OS_MAC
    shortcuts.append(QKeySequence(Qt::META | Qt::Key_Tab));
    shortcuts.append(QKeySequence(Qt::META | Qt::Key_PageDown));

    shortcuts2.append(QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_Tab));
    shortcuts2.append(QKeySequence(Qt::META | Qt::Key_PageUp));
#else
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Tab));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_PageDown));

    shortcuts2.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    shortcuts2.append(QKeySequence(Qt::CTRL | Qt::Key_PageUp));

#endif
    _ui->actionFileNext->setShortcuts(shortcuts);
    _ui->actionFilePrevious->setShortcuts(shortcuts2);

    // Initialise pop-up menus.
    _projectPopupMenu = new QMenu;
    _projectPopupMenu->addAction(_ui->actionNewFile);
    _projectPopupMenu->addAction(_ui->actionNewFolder);
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction(_ui->actionEditFindInFiles);
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction(_ui->actionOpen_on_Desktop);
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction(_ui->actionCloseProject);

    _filePopupMenu = new QMenu;
    _filePopupMenu->addAction(_ui->actionOpen_on_Desktop);
    _filePopupMenu->addAction(_ui->actionOpen_in_Help);
    _filePopupMenu->addSeparator();
    _filePopupMenu->addAction(_ui->actionRenameFile);
    _filePopupMenu->addAction(_ui->actionDeleteFile);

    _dirPopupMenu = new QMenu;
    _dirPopupMenu->addAction(_ui->actionNewFile);
    _dirPopupMenu->addAction(_ui->actionNewFolder);
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction(_ui->actionEditFindInFiles);
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction(_ui->actionOpen_on_Desktop);
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction(_ui->actionRenameFile);
    _dirPopupMenu->addAction(_ui->actionDeleteFile);

    _editorPopupMenu = new QMenu;
    _editorPopupMenu->addAction(_ui->actionEditUndo);
    _editorPopupMenu->addAction(_ui->actionEditRedo);
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction(_ui->actionEditUn_Comment_block);
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction(_ui->actionEditFind);
    _editorPopupMenu->addSeparator();
    QMenu *bm = new QMenu("Bookmarks", _editorPopupMenu);
    bm->addAction(_ui->actionToggleBookmark);
    bm->addAction(_ui->actionPreviousBookmark);
    bm->addAction(_ui->actionNextBookmark);
    _editorPopupMenu->addMenu(bm);
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction(_ui->actionEditCut);
    _editorPopupMenu->addAction(_ui->actionEditCopy);
    _editorPopupMenu->addAction(_ui->actionEditPaste);
    _editorPopupMenu->addAction(_ui->actionEditDelete);
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction(_ui->actionEditSelectAll);
    connect(_ui->actionFileQuit, &QAction::triggered, this, &MainWindow::onFileQuit);

    // NOTE: AKA from Mac OS El Capitan (10.11). Full screen should automatically be added to the view menu.
    // So the full screen toggle is removed from the main UI and is added in the main window constructor. On Macs, Only
    // the action is defined to match that of the key combination of Command+CTRL+F, which appears to be the most common
    // key combination for entering and exiting full screen mode.
    _ui->menuView->addSeparator();
#ifndef Q_OS_MAC
    QAction *actionToggle_Fullscreen = new QAction(QString("Toggle Fullscreen"), this);
    actionToggle_Fullscreen->setShortcut(QKeySequence(Qt::Key_F11));
    _ui->menuView->addAction(actionToggle_Fullscreen);
    connect(actionToggle_Fullscreen, &QAction::triggered, this, &MainWindow::onToggleFullscreen);
#else
    QAction *actionToggle_Fullscreen = new QAction(this);
    actionToggle_Fullscreen->setShortcut(QKeySequence(Qt::META | Qt::CTRL | Qt::Key_F));
    connect(actionToggle_Fullscreen, &QAction::triggered, this, &MainWindow::onToggleFullscreen);
#endif

    // Initialise the settings.
    readSettings();

    // NOTE: This is used to determine the IDE's front end tool to use and the targets it supports.
    if(_buildFileType.isEmpty())
        updateTargetsWidget("cerberus");

    // Initialise the preference, find and find in files dialogues.
    _prefsDialog = new PrefsDialog(this);
    _prefsDialog->readSettings();

    _findDialog = new FindDialog(this);
    _findDialog->readSettings();
    connect(_findDialog, &FindDialog::findReplace, this, &MainWindow::onFindReplace);

    _findInFilesDialog = new FindInFilesDialog(nullptr);
    _findInFilesDialog->readSettings();

    connect(_findInFilesDialog, SIGNAL(showCode(QString, int, int)), SLOT(onShowCode(QString, int, int)));
    setContextMenuPolicy(Qt::NoContextMenu); // Uncomment this to disable the main context pop-up

    // NOTE: Application is a derived QApplication object and in combination with ApplicationGuard,
    // is used to ensure only one instance of Ted is running. All arguments are parsed from a new instance
    // are passed onto the already running instance.
    QStringList args = Application::arguments();
    parseAppArgs(args);

    // Update the UI
    updateWindowTitle();
    updateActions();
    statusBar()->showMessage("Ready.");
}

MainWindow::~MainWindow()
{
    if(actionToggle_Fullscreen)
        delete actionToggle_Fullscreen;
    delete _ui;
}

//----------------------------------------------------------------------------------------------------------------------
//  MainWindow: PUBLIC MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
void MainWindow::cdebug(const QString &str)
{
    _consoleTextWidget->setTextColor(Prefs::prefs()->getColor("console3Color"));
    print(str);
}

void MainWindow::updateHelp()
{
    QString home = _cerberusPath + "/docs/html/Home.html";
    if(!QFile::exists(home)) {
        if(QMessageBox::question(this, "Rebuild Cerberus docs", "Cerberus documentation not found - rebuild docs?",
                                 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
            onHelpRebuild();
    }
}

// Returns information from the QStringList for the variable _completeList. This is called by the CodeEditor class.
int MainWindow::CompleteListCount()
{
    return _completeList.count();
}

QString MainWindow::CompleteListItem(const int item)
{
    return _completeList.at(item);
}

// This is the main member function to open a file.
// It handles opening of help, media, document and source/text files.
QWidget *MainWindow::openFile(const QString &cpath, bool addToRecent)
{
    QString path = cpath;

    // If the path is either file:/, http:/ or https:/, then open a help view if one is not already open.
    if(isUrl(path)) {
        HelpView *helpView = nullptr;
        for(int i = 0; i < _mainTabWidget->count(); ++i) {
            helpView = qobject_cast<HelpView *>(_mainTabWidget->widget(i));
            if(helpView)
                break;
        }
        if(!helpView) {
            helpView = new HelpView(_mainTabWidget);
            helpView->setPage(new WebEnginePage(helpView));
            _mainTabWidget->addTab(helpView, "Help");
        }

        helpView->setUrl(path);

        if(helpView != _mainTabWidget->currentWidget())
            _mainTabWidget->setCurrentWidget(helpView);

        else
            updateWindowTitle();

        return helpView;
    }

    // If the path is empty, then use an open file dialogue to open a file.
    if(path.isEmpty()) {

        QString srcTypes = "*.cxs *.monkey *.cpp *.h *.cs *.js *.as *.java *.txt";
        if(!_monkey2Path.isEmpty())
            srcTypes += " *.mx2 *.monkey2";

        path = fixPath(QFileDialog::getOpenFileName(this, "Open File", _defaultDir,
                       "Source Files (" + srcTypes +
                       ");;Image Files(*.jpg *.png *.bmp);;All Files(*.*)"));
        if(path.isEmpty())
            return nullptr;

        _defaultDir = extractDir(path);
    }

    //  To avoid an issue with the IDE and symbolic links on Linux. The path needs to be converted to a full path.
#ifdef Q_OS_LINUX
    path = QFileInfo(path).canonicalFilePath();
#endif

    // If the path matches any media or document file, then either open the built-in viewer, or open the default
    // application for that file type.
    if(isImageFile(path)) {
        showImage(path);
        return nullptr;
    }

    if(isAudioFile(path)) {
        playAudio(path);
        return nullptr;
    }

    if(isDocFile(path)) {
        showDoc(path);
        return nullptr;
    }

    // Check to see if there is an editor already open with that path.
    CodeEditor *editor = editorWithPath(path);
    if(editor) {
        _mainTabWidget->setCurrentWidget(editor);
        return editor;
    }

    // Create and check to see if that the editor was created successfully.
    editor = new CodeEditor(nullptr);
    if(!editor->open(path)) {
        delete editor;
        QMessageBox::warning(this, "Open File Error", "Error opening file: " + path);
        return nullptr;
    }

    // Connect the code editor signals to the main window's member functions.
    connect(editor, &CodeEditor::textChanged, this, &MainWindow::onTextChanged);
    connect(editor, &CodeEditor::cursorPositionChanged, this, &MainWindow::onCursorPositionChanged);

    // Set the context menu policy and connect the context menu request to the main windows onEditorMenu member function.
    editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(editor, &CodeEditor::customContextMenuRequested, this, &MainWindow::onEditorMenu);

    // Add the editor to the main central widget and select it.
    _mainTabWidget->addTab(editor, stripDir(path));
    _mainTabWidget->setCurrentWidget(editor);

    // If the addToRecent parameter is true and that file isn't in the recent files menu,
    // then add the file path to the recent file menu.
    if(addToRecent) {
        QMenu *menu = _ui->menuRecent_Files;
        QList<QAction *> actions = menu->actions();
        bool found = false;
        for(int i = 0; i < actions.size(); ++i) {
            if(actions[i]->text() == path) {
                found = true;
                break;
            }
        }
        if(!found) {
            for(int i = 19; i < actions.size(); ++i)
                menu->removeAction(actions[i]);
            QAction *action = new QAction(path, menu);
            if(actions.size())
                menu->insertAction(actions[0], action);

            else
                menu->addAction(action);
            connect(action, &QAction::triggered, this, &MainWindow::onFileOpenRecent);
        }
    }

    // If there is more than one tab open in the central widget, then show the scroll tab buttons.
    if(_mainTabWidget->count() > 1)
        _mainTabWidget->tabBar()->setUsesScrollButtons(true);
    else
        _mainTabWidget->tabBar()->setUsesScrollButtons(false);

    return editor;
}

// Parse arguments passed if another instance of Ted is started.
void MainWindow::parseAppArgs(QStringList &args)
{
    for(int i = 1; i < args.size(); ++i) {
        QString arg = fixPath(args.at(i));
        if(QFile::exists(arg))
            openFile(arg, true);
    }
}

//----------------------------------------------------------------------------------------------------------------------
//  MainWindow: PRIVATE MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
void MainWindow::setIcons()
{
    QSettings settings;
    Prefs *prefs = Prefs::prefs();

    QString theme = "";
    theme = prefs->getString("theme");

    _ui->actionNew->setIcon(getThemeIcon(theme, "New.png", "New_off.png"));
    _ui->actionOpen->setIcon(getThemeIcon(theme, "Open.png", "Open_off.png"));
    _ui->actionClose->setIcon(getThemeIcon(theme, "Close.png", "Close_off.png"));
    _ui->actionSave->setIcon(getThemeIcon(theme, "Save.png", "Save_off.png"));

    _ui->actionEditCopy->setIcon(getThemeIcon(theme, "Copy.png", "Copy_off.png"));
    _ui->actionEditCut->setIcon(getThemeIcon(theme, "Cut.png", "Cut_off.png"));
    _ui->actionEditPaste->setIcon(getThemeIcon(theme, "Paste.png", "Paste_off.png"));
    _ui->actionEditFind->setIcon(getThemeIcon(theme, "Find.png", "Find_off.png"));

    _ui->actionBuildBuild->setIcon(getThemeIcon(theme, "Build.png", "Build_off.png"));
    _ui->actionBuildRun->setIcon(getThemeIcon(theme, "Build-Run.png", "Build-Run_off.png"));

    _ui->actionStep->setIcon(getThemeIcon(theme, "Step.png", "Step_off.png"));
    _ui->actionStep_In->setIcon(getThemeIcon(theme, "Step-In.png", "Step-In_off.png"));
    _ui->actionStep_Out->setIcon(getThemeIcon(theme, "Step-Out.png", "Step-Out_off.png"));
    _ui->actionKill->setIcon(getThemeIcon(theme, "Stop.png", "Stop_off.png"));

    _ui->actionHelpHome->setIcon(getThemeIcon(theme, "Home.png", "Home_off.png"));
    _ui->actionHelpBack->setIcon(getThemeIcon(theme, "Back.png", "Back_off.png"));
    _ui->actionHelpForward->setIcon(getThemeIcon(theme, "Forward.png", "Forward_off.png"));
}

QIcon MainWindow::getThemeIcon(const QString &theme, const QString &ic, const QString &icd)
{
#ifdef Q_OS_MAC
    QString appPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif
    QIcon icon = QIcon(QPixmap(appPath + "/themes/" + theme + "/icons/ui/" + ic));
    if(icd != "")
        icon.addPixmap(QPixmap(appPath + "/themes/" + theme + "/icons/ui/" + icd), QIcon::Disabled);
    return icon;
}

// Parse the help text files for use with F1 quick help.
void MainWindow::loadHelpIndex()
{
    if(_cerberusPath.isEmpty())
        return;

    QFile file(_cerberusPath + "/docs/html/index.txt");
    if(!file.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&file);

    // NOTE: Documentation for Qt6 states that QTextStrean uses UTF-8 as standard.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif
    QString text = stream.readAll();

    file.close();

    QStringList lines = text.split('\n');

    _indexWidget->disconnect();
    _indexWidget->clear();
    _completeList.clear();

    for(int i = 0; i < lines.count(); ++i) {

        QString line = lines.at(i);

        int j = line.indexOf(':');
        if(j == -1)
            continue;

        QString topic = line.left(j);
        QString url = FILE_URL + _cerberusPath + "/docs/html/" + line.mid(j + 1);

        _indexWidget->addItem(topic);
        _completeList.append(topic);

        _helpUrls.insert(topic, url);
    }

    /* Read in declarations for the F1 Status bar help
    // NOTE: The decls.txt generated appears to be broken for this to work.
    // I'm sure that the original release of Ted used this for keyword help
    // in the status bar.
    QFile file2( _cerberusPath+"/docs/html/decls.txt" );
    if( !file2.open( QIODevice::ReadOnly ) ) return;

    QTextStream stream2( &file2 );

    // NOTE: Documentation for Qt6 states that QTextStrean uses UTF-8 as standard.
    #if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    stream2.setCodec( "UTF-8" );
    #endif

    QString text2=stream2.readAll();

    file2.close();

    QStringList lines2=text2.split('\n');

    for( int i=0;i<lines2.count();++i ){

        QString line2=lines2.at( i );

        if ( line2.startsWith("Class ") ) continue;
        if ( line2.startsWith("Property ") ) continue;
        if ( line2.startsWith("Module ") ) continue;
        if ( line2.startsWith("Inherited_method ") ) continue;
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
    }*/

    connect(_indexWidget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onShowHelp);
}

// Checks to see if a toolchain is available for a given file type that's open in the editor.
bool MainWindow::isBuildable(CodeEditor *editor)
{
    if(!editor)
        return false;
    if(editor->fileType() == "cxs")
        return !_cerberusPath.isEmpty();
    if(editor->fileType() == "monkey")
        return !_cerberusPath.isEmpty();
    if(editor->fileType() == "bmx")
        return !_blitzmaxPath.isEmpty();
    if(editor->fileType() == "monkey2")
        return !_monkey2Path.isEmpty();
    return false;
}

// Return the path stored in either a editor or a help view.
QString MainWindow::widgetPath(QWidget *widget)
{
    if(CodeEditor *editor = qobject_cast<CodeEditor *>(widget))
        return editor->path();

    else if(HelpView *helpView = qobject_cast<HelpView *>(widget))
        return helpView->url().toString();
    return "";
}

// Returns a CodeEditor if the path passed matches the one stored in the editor.
CodeEditor *MainWindow::editorWithPath(const QString &path)
{
    for(int i = 0; i < _mainTabWidget->count(); ++i) {
        if(CodeEditor *editor = qobject_cast<CodeEditor *>(_mainTabWidget->widget(i))) {
            if(editor->path() == path)
                return editor;
        }
    }
    return nullptr;
}

// Creates a new file
QWidget *MainWindow::newFile(const QString &cpath)
{

    QString path = cpath;

    // If the file path is empty, then select a name and where the new file will be created.
    if(path.isEmpty()) {
        QString srcTypes = "*.cxs *.monkey *.cpp *.h *.cs *.js *.as *.java *.txt";
        if(!_monkey2Path.isEmpty())
            srcTypes += " *.mx2 *.monkey2";

        path = fixPath(QFileDialog::getSaveFileName(this, "New File", _defaultDir, "Source Files (" + srcTypes + ")"));
        if(path.isEmpty())
            return nullptr;
    }

    // Try to open the file. If it fails, then display an error.
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "New file", "Failed to create new file: " + path);
        return nullptr;
    }
    file.close();

    // If the file's already open, then close the editor before reopening it.
    if(CodeEditor *editor = editorWithPath(path))
        closeFile(editor);

    return openFile(path, true);
}

// Creates a new file from one of the templates.
QWidget *MainWindow::newFileTemplate(const QString &cpath)
{

    QString path = "";

    if(path.isEmpty()) {

        QString srcTypes = "*.cxs *.monkey *.cpp *.h *.cs *.js *.as *.java *.txt";
        if(!_monkey2Path.isEmpty())
            srcTypes += " *.mx2 *.monkey2";

        path = fixPath(QFileDialog::getSaveFileName(this, "New File from template", _defaultDir,
                       "Source Files (" + srcTypes + ")"));
        if(path.isEmpty())
            return nullptr;
    }
    QFile::copy(cpath, path);

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "New file", "Failed to create new file from template: " + path);
        return nullptr;
    }
    file.close();

    //  To avoid an issue with the IDE and symbolic links on Linux. The path needs to be converted to a full path.
#ifdef Q_OS_LINUX
    path = QFileInfo(path).canonicalFilePath();
#endif

    if(CodeEditor *editor = editorWithPath(path))
        closeFile(editor);

    return openFile(path, true);
}

// Save a file to disk.
bool MainWindow::saveFile(QWidget *widget, const QString &cpath)
{

    QString path = cpath;

    // Cast the widget passed to a CodeEditor, If it fails, then
    // it wasn't a a CodeEditor object, so exit the function.
    CodeEditor *editor = qobject_cast<CodeEditor *>(widget);
    if(!editor)
        return true;

    // If the path is empty, or wasn't modified, then open a QFileDialog to set the name and location to save to.
    if(path.isEmpty()) {

        _mainTabWidget->setCurrentWidget(editor);

        QString srcTypes = "*.cxs *.monkey *.cpp *.h *.cs *.js *.as *.java *.txt";
        if(!_monkey2Path.isEmpty())
            srcTypes += " *.mx2 *.monkey2";

        path = fixPath(QFileDialog::getSaveFileName(this,
                       "Save File As", editor->path(), "Source Files (" + srcTypes + ")"));
        if(path.isEmpty())
            return false;

    } else if(!editor->modified())
        return true;

    // Save the file in the editor and if there was an error, display a message.
    if(!editor->save(path)) {
        QMessageBox::warning(this, "Save File Error", "Error saving file: " + path);
        return false;
    }

    // Update the UI
    updateTabLabel(editor);
    updateWindowTitle();
    updateActions();

    return true;
}

// Close an open file.
bool MainWindow::closeFile(QWidget *widget, bool really)
{
    // Check that what was pass was a QWidget.
    if(!widget)
        return true;

    // Cast the widget passed to a CodeEditor and if successful, check if the file was modified.
    // If it was, then ask if the changes are to be saved.
    CodeEditor *editor = qobject_cast<CodeEditor *>(widget);
    if(editor && editor->modified()) {

        _mainTabWidget->setCurrentWidget(editor);

        QMessageBox msgBox;
        msgBox.setText(editor->path() + " has been modified.");
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);

        int ret = msgBox.exec();

        if(ret == QMessageBox::Save) {
            if(!saveFile(editor, editor->path()))
                return false;
        } else if(ret == QMessageBox::Cancel)
            return false;

        else if(ret == QMessageBox::Discard) {
        }
    }

    if(!really)
        return true;

    // Null out the internal variables based on the widget passed before removing the tab and deleting the object.
    if(widget == _codeEditor)
        _codeEditor = nullptr;

    else if(widget == _helpWidget)
        _helpWidget = nullptr;
    if(widget == _lockedEditor)
        _lockedEditor = nullptr;

    _mainTabWidget->removeTab(_mainTabWidget->indexOf(widget));
    delete widget;

    // If the number of open central widget tabs displayed drops down to 1 or less, then disable the tabs scroll button.
    if(_mainTabWidget->count() <= 1)
        _mainTabWidget->tabBar()->setUsesScrollButtons(false);

    return true;
}

// Check that there is a valid Cerberus X path.
bool MainWindow::isValidCerberusPath(const QString &path)
{
    QString transcc = "transcc" + HOST;
#ifdef Q_OS_WIN
    transcc += ".exe";
#endif
    return QFile::exists(path + "/bin/" + transcc);
}

// Check that there is a valid BlitzMax path.
bool MainWindow::isValidBlitzmaxPath(const QString &path)
{
#ifdef Q_OS_WIN
    QString bmk = "bmk.exe";
#else
    QString bmk = "bmk";
#endif
    return QFile::exists(path + "/bin/" + bmk);
}

// Search for a valid Cerberus X path.
QString MainWindow::defaultCerberusPath()
{
    QString path = Application::applicationDirPath();
    while(!path.isEmpty()) {
        if(isValidCerberusPath(path))
            return path;
        path = extractDir(path);
    }
    return "";
}

// Enumerate the targets available from the tool chains
void MainWindow::enumTargets()
{
    if(_cerberusPath.isEmpty())
        return;

    _cerberusTargets.clear();
    _monkey2Targets.clear();

    QDir monkey2Dir(_cerberusPath + "/monkey2");
    if(!monkey2Dir.exists())
        monkey2Dir = QDir(_cerberusPath + "/../monkey2");
    if(monkey2Dir.exists()) {
        _monkey2Path = monkey2Dir.absolutePath();
        _monkey2Targets.push_back("Desktop");
        _monkey2Targets.push_back("Emscripten");
        _activeMonkey2Target = "Desktop";
    }

    QString cmd = "\"" + _cerberusPath + "/bin/transcc" + HOST + "\"";

    Process proc;
    if(!proc.start(cmd))
        return;

    QString sol = "Valid targets: ";
    QString ver = "TRANS cerberus compiler V";

    while(proc.waitLineAvailable(0)) {
        QString line = proc.readLine(0);
        if(line.startsWith(ver))
            _transVersion = line.mid(ver.length());

        else if(line.startsWith(sol)) {
            line = line.mid(sol.length());
            QStringList bits = line.split(' ');
            for(int i = 0; i < bits.count(); ++i) {
                QString bit = bits[i];
                if(bit.isEmpty())
                    continue;
                QString target = bit.replace('_', ' ');
                if(target.contains("Html5"))
                    _activeCerberusTarget = target;
                _cerberusTargets.push_back(target);
            }
        }
    }
}

// Load and apply window themes
void MainWindow::loadTheme(QString &appPath, Prefs *prefs)
{
    QString css = "";
    QString cssFile = "";
    cssFile = prefs->getString("theme");
    QFile f(appPath + "/themes/" + cssFile + "/" + cssFile + ".css");
    if(f.open(QFile::ReadOnly))
        css = f.readAll();
    f.close();
    css.replace("url(:", "url(" + appPath + "/themes/" + cssFile);

    qApp->setStyleSheet(css);
}

// Read in the stored settings from disk.
void MainWindow::readSettings()
{
    QSettings settings;
    Prefs *prefs = Prefs::prefs();

#ifdef Q_OS_MAC
    QString appPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif

    if(settings.value("settingsVersion").toInt() < 1) {
        prefs->setValue("fontFamily", "Courier");
        prefs->setValue("fontSize", 12);
        prefs->setValue("smoothFonts", true);
        prefs->setValue("tabSize", 4);
        prefs->setValue("tabs4spaces", true);
        prefs->setValue("capitalizeAPI", false);
        prefs->setValue("theme", "default");
        prefs->setValue("backgroundColor", QColor(255, 255, 255));
        prefs->setValue("console1Color", QColor(70, 70, 70));
        prefs->setValue("console2Color", QColor(70, 170, 70));
        prefs->setValue("console3Color", QColor(70, 70, 170));
        prefs->setValue("console4Color", QColor(70, 170, 170));
        prefs->setValue("defaultColor", QColor(0, 0, 0));
        prefs->setValue("numbersColor", QColor(0, 0, 255));
        prefs->setValue("stringsColor", QColor(170, 0, 255));
        prefs->setValue("identifiersColor", QColor(0, 0, 0));
        prefs->setValue("keywordsColor", QColor(0, 85, 255));
        prefs->setValue("keywords2Color", QColor(0, 85, 255));
        prefs->setValue("lineNumberColor", QColor(0, 85, 255));
        prefs->setValue("commentsColor", QColor(0, 128, 128));
        prefs->setValue("highlightColor", QColor(255, 255, 128));
        prefs->setValue("highlightCurrLine", true);
        prefs->setValue("highlightCurrWord", true);
        prefs->setValue("highlightBrackets", true);
        prefs->setValue("showLineNumbers", true);
        prefs->setValue("sortCodeBrowser", true);

        _cerberusPath = defaultCerberusPath();
        prefs->setValue("cerberusPath", _cerberusPath);

        _blitzmaxPath = "";
        prefs->setValue("blitzmaxPath", _blitzmaxPath);

        if(!_cerberusPath.isEmpty())
            _projectTreeModel->addProject(_cerberusPath);

        // If any changes have take place, make sure that they have been written to disk.
        writeSettings();
        loadTheme(appPath, prefs);

        enumTargets();

        onHelpHome();

        loadHelpIndex();
        return;
    }

    _cerberusPath = defaultCerberusPath();

    // Check that the paths to the tool chains are correct.
    QString prefsCerberusPath = prefs->getString("cerberusPath");

    if(_cerberusPath.isEmpty()) {

        _cerberusPath = prefsCerberusPath;

        if(!isValidCerberusPath(_cerberusPath)) {
            _cerberusPath = "";
            prefs->setValue("cerberusPath", _cerberusPath);
            QMessageBox::warning(this, "Cerberus Path Error",
                                 "Invalid Cerberus path!\n\nPlease select correct path from the File..Options dialog");
        }

    } else if(_cerberusPath != prefsCerberusPath) {
        prefs->setValue("cerberusPath", _cerberusPath);
        QMessageBox::information(this, "Cerberus Path Updated", "Cerberus path has been updated to " + _cerberusPath);
    }

    _blitzmaxPath = prefs->getString("blitzmaxPath");
    if(!_blitzmaxPath.isEmpty() && !isValidBlitzmaxPath(_blitzmaxPath)) {
        _blitzmaxPath = "";
        prefs->setValue("blitzmaxPath", _blitzmaxPath);
        QMessageBox::warning(this, "BlitzMax Path Error",
                             "Invalid BlitzMax path!\n\nPlease select correct path from the File..Options dialog");
    }

    // Enumerate the targets, load in the help index file
    enumTargets();
    loadHelpIndex();

    // Restore the window layout.
    settings.beginGroup("mainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();

    // Restore the last session opened projects
    int n = settings.beginReadArray("openProjects");
    for(int i = 0; i < n; ++i) {
        settings.setArrayIndex(i);
        QString path = fixPath(settings.value("path").toString());
        if(QFile::exists(path))
            _projectTreeModel->addProject(path);
    }
    settings.endArray();

    // Restore the last sessions opened editors.
    n = settings.beginReadArray("openDocuments");
    for(int i = 0; i < n; ++i) {
        settings.setArrayIndex(i);
        QString path = fixPath(settings.value("path").toString());
        if(isUrl(path))
            openFile(path, false);

        else {
            if(QFile::exists(path))
                openFile(path, false);
        }
    }
    settings.endArray();

    // Populate the recent files menu from the settings.
    n = settings.beginReadArray("recentFiles");
    for(int i = 0; i < n; ++i) {
        settings.setArrayIndex(i);
        QString path = fixPath(settings.value("path").toString());
        if(QFile::exists(path))
            _ui->menuRecent_Files->addAction(path, this, SLOT(onFileOpenRecent()));
    }
    settings.endArray();

    // Restore the last sessions build settings.
    settings.beginGroup("buildSettings");
    QString target = settings.value("target").toString();

    _activeCerberusTarget = target;
    _buildFileType = "";

    // Restore the last sessions config settings.
    QString config = settings.value("config").toString();
    if(!config.isEmpty()) {
        for(int i = 0; i < _configsWidget->count(); ++i) {
            if(_configsWidget->itemText(i) == config) {
                _configsWidget->setCurrentIndex(i);
                break;
            }
        }
    }

    // Restore any editor that was locked as a build file.
    QString locked = settings.value("locked").toString();
    if(!locked.isEmpty()) {
        if(CodeEditor *editor = editorWithPath(locked)) {
            _lockedEditor = editor;
            updateTabLabel(editor);
        }
    }
    settings.endGroup();

    // If the settings version is less and two, then do not restore any theme's or default file templates.
    // Those are Cerberus X Ted build settings.
    if(settings.value("settingsVersion").toInt() < 2)
        return;

    // Load themes, default template files and icons.
    _defaultDir = fixPath(settings.value("defaultDir").toString());
    loadTheme(appPath, prefs);

    Application::processEvents();
    QString tempPath = "";
    QDir recoredDir(appPath + "/templates/");
    QStringList allFiles = recoredDir.entryList(
                               QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files,
                               QDir::DirsFirst);
    foreach(const QString &str, allFiles) {
        tempPath = appPath + "/templates/" + str;
        if(QFile::exists(tempPath))
            _ui->menuNewTemplate->addAction(QFileInfo(tempPath).baseName(), this, SLOT(onFileNewTemplate()));
    }

    // Set the actions icons depending on the theme
    setIcons();
    _targetsWidget->adjustSize();
    _configsWidget->adjustSize();
}

// Write the settings to disk.
void MainWindow::writeSettings()
{
    QSettings settings;

    // Ensure that the setting file is maked as a version two for Cerberus X settings.
    settings.setValue("settingsVersion", SETTINGS_VERSION);

    // Save the current window layout.
    settings.beginGroup("mainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();

    // Save all open projects.
    settings.beginWriteArray("openProjects");
    QVector<QString> projs = _projectTreeModel->projects();
    for(int i = 0; i < projs.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("path", projs[i]);
    }
    settings.endArray();

    // Save all opened editors
    settings.beginWriteArray("openDocuments");
    int n = 0;
    for(int i = 0; i < _mainTabWidget->count(); ++i) {
        QString path = widgetPath(_mainTabWidget->widget(i));
        if(path.isEmpty())
            continue;
        settings.setArrayIndex(n++);
        settings.setValue("path", path);
    }
    settings.endArray();

    // Save the recently opened files menu items.
    settings.beginWriteArray("recentFiles");
    QList<QAction *> rfiles = _ui->menuRecent_Files->actions();
    for(int i = 0; i < rfiles.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("path", rfiles[i]->text());
    }
    settings.endArray();

    // Save the default directory, build settings, target, config and currently locked file.
    settings.beginGroup("buildSettings");
    settings.setValue("target", _targetsWidget->currentText());
    settings.setValue("config", _configsWidget->currentText());
    settings.setValue("locked", _lockedEditor ? _lockedEditor->path() : "");
    settings.endGroup();

    settings.setValue("defaultDir", _defaultDir);
}

// Update the main window title bar based.
// Displays the file path to the currently selected editor.
void MainWindow::updateWindowTitle()
{
    QWidget *widget = _mainTabWidget->currentWidget();
    if(CodeEditor *editor = qobject_cast<CodeEditor *>(widget))
        setWindowTitle(editor->path());

    else if(HelpView *helpView = qobject_cast<HelpView *>(widget))
        setWindowTitle(helpView->url().toString());

    else
        setWindowTitle("Ted V" TED_VERSION);
}

// If the editor's text has been modified, then update the editors tab to indicate this.
void MainWindow::updateTabLabel(QWidget *widget)
{
    if(CodeEditor *editor = qobject_cast<CodeEditor *>(widget)) {
        QString text = stripDir(editor->path());
        if(editor->modified())
            text = text + "*";
        if(editor == _lockedEditor)
            text = "+" + text;
        _mainTabWidget->setTabText(_mainTabWidget->indexOf(widget), text);
    }
}

// Update the current targets widgets to reflect the tool chain based on the file type.
void MainWindow::updateTargetsWidget(QString fileType)
{

    if(_buildFileType != fileType) {

        disconnect(_targetsWidget, nullptr, nullptr, nullptr);
        _targetsWidget->clear();

        if(fileType == "cxs" || fileType == "monkey") {
            for(int i = 0; i < _cerberusTargets.size(); ++i) {
                _targetsWidget->addItem(_cerberusTargets.at(i));
                if(_cerberusTargets.at(i) == _activeCerberusTarget)
                    _targetsWidget->setCurrentIndex(i);
            }
            _activeCerberusTarget = _targetsWidget->currentText();
            _configsWidget->setEnabled(true);
        } else if(fileType == "mx2" || fileType == "monkey2") {
            for(int i = 0; i < _monkey2Targets.size(); ++i) {
                _targetsWidget->addItem(_monkey2Targets.at(i));
                if(_monkey2Targets.at(i) == _activeMonkey2Target)
                    _targetsWidget->setCurrentIndex(i);
            }
            _activeMonkey2Target = _targetsWidget->currentText();
            _configsWidget->setEnabled(true);
        } else if(fileType == "bmx") {
            _targetsWidget->addItem("BlitzMax App");
            _configsWidget->setEnabled(false);
        }
        _buildFileType = fileType;
        connect(_targetsWidget, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &MainWindow::onTargetChanged);
    }
    _targetsWidget->adjustSize();
}

// Update the UI interface based of the current state of the main widgets.
void MainWindow::updateActions()
{

    bool ed = _codeEditor != nullptr;
    bool db = _debugTreeModel != nullptr;
    bool wr = ed && !_codeEditor->isReadOnly();
    bool sel = ed && _codeEditor->textCursor().hasSelection();

    bool saveAll = false;
    for(int i = 0; i < _mainTabWidget->count(); ++i) {
        if(CodeEditor *editor = qobject_cast<CodeEditor *>(_mainTabWidget->widget(i))) {
            if(editor->modified())
                saveAll = true;
        }
    }

    // File Menu
    _ui->actionClose->setEnabled(ed || _helpWidget);
    _ui->actionClose_All->setEnabled(_mainTabWidget->count() > 1 || (_mainTabWidget->count() == 1 && !_helpWidget));
    _ui->actionClose_Others->setEnabled(_mainTabWidget->count() > 1);
    _ui->actionSave->setEnabled(ed && _codeEditor->modified());
    _ui->actionSave_As->setEnabled(ed);
    _ui->actionSave_All->setEnabled(saveAll);
    _ui->actionFileNext->setEnabled(_mainTabWidget->count() > 1);
    _ui->actionFilePrevious->setEnabled(_mainTabWidget->count() > 1);

    // Edit Menu
    _ui->actionEditUndo->setEnabled(wr && _codeEditor->document()->isUndoAvailable());
    _ui->actionEditRedo->setEnabled(wr && _codeEditor->document()->isRedoAvailable());
    _ui->actionEditCut->setEnabled(wr && sel);
    _ui->actionEditCopy->setEnabled(sel);
    _ui->actionEditPaste->setEnabled(wr);
    _ui->actionEditDelete->setEnabled(sel);
    _ui->actionEditSelectAll->setEnabled(ed);
    _ui->actionEditFind->setEnabled(ed);
    _ui->actionEditFindNext->setEnabled(ed);
    _ui->actionEditGoto->setEnabled(ed);
    _ui->actionEditUn_Comment_block->setEnabled(ed);

    // View Menu - not totally sure why !isHidden works but isVisible doesn't...
    _ui->actionViewFile->setChecked(!_ui->fileToolBar->isHidden());
    _ui->actionViewEdit->setChecked(!_ui->editToolBar->isHidden());
    _ui->actionViewBuild->setChecked(!_ui->buildToolBar->isHidden());
    _ui->actionViewHelp->setChecked(!_ui->helpToolBar->isHidden());
    _ui->actionViewConsole->setChecked(!_consoleDockWidget->isHidden());
    _ui->actionViewBrowser->setChecked(!_browserDockWidget->isHidden());
    _ui->actionToggleBookmark->setEnabled(ed);
    _ui->actionNextBookmark->setEnabled(ed);
    _ui->actionPreviousBookmark->setEnabled(ed);

    // Build Menu
    CodeEditor *buildEditor = _lockedEditor ? _lockedEditor : _codeEditor;

    // These are set depending on the tool chain status and file type.
    bool canBuild = !_consoleProc && isBuildable(buildEditor);
    bool canTrans = canBuild && (buildEditor->fileType() == "cxs" || buildEditor->fileType() == "monkey");

    _ui->actionBuildBuild->setEnabled(canBuild);
    _ui->actionBuildRun->setEnabled(canBuild || db);
    _ui->actionBuildCheck->setEnabled(canTrans);
    _ui->actionBuildUpdate->setEnabled(canTrans);
    _ui->actionStep->setEnabled(db);
    _ui->actionStep_In->setEnabled(db);
    _ui->actionStep_Out->setEnabled(db);
    _ui->actionKill->setEnabled(_consoleProc != nullptr);
    _ui->actionLock_Build_File->setEnabled(_codeEditor != _lockedEditor && isBuildable(_codeEditor));
    _ui->actionUnlock_Build_File->setEnabled(_lockedEditor != nullptr);

    // Targets Widget
    if(isBuildable(buildEditor))
        updateTargetsWidget(buildEditor->fileType());

    // Help Menu
    _ui->actionHelpBack->setEnabled(_helpWidget != nullptr);
    _ui->actionHelpForward->setEnabled(_helpWidget != nullptr);
    _ui->actionHelpQuickHelp->setEnabled(_codeEditor != nullptr);
    _ui->actionHelpRebuild->setEnabled(_consoleProc == nullptr);
}

// Output to the console view.
void MainWindow::print(const QString &str)
{
    QTextCursor cursor = _consoleTextWidget->textCursor();
    cursor.insertText(str);
    cursor.insertBlock();
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    _consoleTextWidget->setTextCursor(cursor);
}

// Execute a command. The file to process is stored in the QWidget.
void MainWindow::runCommand(QString cmd, QWidget *fileWidget)
{

    cmd = cmd.replace("${TARGET}", _targetsWidget->currentText().replace(' ', '_'));
    cmd = cmd.replace("${CONFIG}", _configsWidget->currentText());
    cmd = cmd.replace("${CERBERUSPATH}", _cerberusPath);
    cmd = cmd.replace("${MONKEY2PATH}", _monkey2Path);
    cmd = cmd.replace("${BLITZMAXPATH}", _blitzmaxPath);
    if(fileWidget)
        cmd = cmd.replace("${FILEPATH}", widgetPath(fileWidget));

    _consoleProc = new Process;

    connect(_consoleProc, &Process::lineAvailable, this, &MainWindow::onProcLineAvailable);
    connect(_consoleProc, &Process::finished, this, &MainWindow::onProcFinished);

    _consoleTextWidget->clear();
    _consoleDockWidget->show();
    _consoleTextWidget->setTextColor(Prefs::prefs()->getColor("console1Color"));
    print(cmd);

    if(!_consoleProc->start(cmd)) {
        delete _consoleProc;
        _consoleProc = nullptr;
        QMessageBox::warning(this, "Process Error", "Failed to start process: " + cmd);
        return;
    }

    updateActions();
}

// Execute a build based on the mode.
// This could be a full build and run, a build, an update or a file check.
void MainWindow::build(QString mode)
{
    // Set the editor based on the on stored in the variable _lockedEditor
    CodeEditor *editor = _lockedEditor ? _lockedEditor : _codeEditor;
    if(!isBuildable(editor))
        return;

    // Get the file path
    QString filePath = editor->path();
    if(filePath.isEmpty())
        return;

    QString cmd, msg = "Building: " + filePath + "...";

    // Create a build command based on the toolchain and the file type.
    if(editor->fileType() == "cxs" || editor->fileType() == "monkey") {
        if(mode == "run")
            cmd = "\"${CERBERUSPATH}/bin/transcc" + HOST +
                  "\" -target=${TARGET} -config=${CONFIG} -run \"${FILEPATH}\"";

        else if(mode == "build")
            cmd = "\"${CERBERUSPATH}/bin/transcc" + HOST + "\" -target=${TARGET} -config=${CONFIG} \"${FILEPATH}\"";

        else if(mode == "update") {
            cmd = "\"${CERBERUSPATH}/bin/transcc" + HOST +
                  "\" -target=${TARGET} -config=${CONFIG} -update \"${FILEPATH}\"";
            msg = "Updating: " + filePath + "...";
        } else if(mode == "check") {
            cmd = "\"${CERBERUSPATH}/bin/transcc" + HOST +
                  "\" -target=${TARGET} -config=${CONFIG} -check \"${FILEPATH}\"";
            msg = "Checking: " + filePath + "...";
        }
    } else if(editor->fileType() == "bmx") {
        if(mode == "run")
            cmd = "\"${BLITZMAXPATH}/bin/bmk\" makeapp -a -r -x \"${FILEPATH}\"";

        else if(mode == "build")
            cmd = "\"${BLITZMAXPATH}/bin/bmk\" makeapp -a -x \"${FILEPATH}\"";
    } else if(editor->fileType() == "monkey2") {
        if(mode == "run") {
            QString mx2 = "mx2cc";
#ifdef Q_OS_WIN
            mx2 += "_windows";
#else
            mx2 += HOST;
#endif
            cmd = "\"${MONKEY2PATH}/bin/" + mx2 + "\" makeapp -target=${TARGET} -config=${CONFIG} \"${FILEPATH}\"";
        }
    }

    if(!cmd.length())
        return;

    onFileSaveAll();
    statusBar()->showMessage(msg);

    runCommand(cmd, editor);
}

// If application termination has been confirmed, then save all settings and any modified editors.
bool MainWindow::confirmQuit()
{
    writeSettings();
    _prefsDialog->writeSettings();
    _findDialog->writeSettings();
    _findInFilesDialog->writeSettings();

    for(int i = 0; i < _mainTabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor *>(_mainTabWidget->widget(i));
        if(editor && !closeFile(editor, false))
            return false;
    }

    return true;
}

// If application termination was triggered, then do a confirmation before shutdown.
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(confirmQuit()) {
        _findInFilesDialog->close();
        event->accept();
    } else
        event->ignore();
}

// Shows an image if one was opened in the IDE.
void MainWindow::showImage(const QString &path)
{
    QImage img;
    bool valid = img.load(path);
    if(valid) {
        QLabel *label = new QLabel();
        int w = img.width();
        int h = img.height();
        QString imgSize = QString::number(w) + "x" + QString::number(h);
        label->setScaledContents(false);
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumWidth(128);
        label->setMinimumHeight(128);
        label->setPixmap(QPixmap::fromImage(img).scaled(w, h, Qt::KeepAspectRatio));
        label->setWindowTitle("CX Image Viewer-> " + imgSize + " : " + path);
        label->setAutoFillBackground(true);
        label->show();
    } else
        QDesktopServices::openUrl(FILE_URL + path);
}

// Opens the default audio player if an audio file is opened in the IDE.
void MainWindow::playAudio(const QString &path)
{
    QDesktopServices::openUrl(FILE_URL + path);
}

// Opens the default document viewer if a document file is opened in the IDE.
void MainWindow::showDoc(const QString &path)
{
    QDesktopServices::openUrl(FILE_URL + path);
}

// Update the status bar
void MainWindow::updateStatusBar()
{
    // Cast the current selected tab to a CodeEditor object.
    // If successful, then update the show Line item in the status bar.
    if(qobject_cast<CodeEditor *>(_mainTabWidget->currentWidget()))
        _statusWidget->setText("Line: " + QString::number(_codeEditor->textCursor().blockNumber() + 1));
    else
        _statusWidget->clear();
}

//----------------------------------------------------------------------------------------------------------------------
//  MainWindow: PUBLIC MEMBER SLOTS
//----------------------------------------------------------------------------------------------------------------------
// FILE SLOTS
// Start a new file
void MainWindow::onFileNew()
{
    newFile("");
}

// Start a new file from a template
void MainWindow::onFileNewTemplate()
{
#ifdef Q_OS_MAC
    QString appPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif
    if(QAction *action = qobject_cast<QAction *>(sender()))
        newFileTemplate(appPath + "/templates/" + action->text() + ".cxs");
}

// Open a file
void MainWindow::onFileOpen()
{
    openFile("", true);
}

// Open a recently opened file
void MainWindow::onFileOpenRecent()
{
    if(QAction *action = qobject_cast<QAction *>(sender()))
        openFile(action->text(), false);
}

// Close an open file.
void MainWindow::onFileClose()
{
    closeFile(_mainTabWidget->currentWidget());
}

// Close all open files.
void MainWindow::onFileCloseAll()
{
    for(;;) {
        int i;
        CodeEditor *editor = nullptr;
        for(i = 0; i < _mainTabWidget->count(); ++i) {
            editor = qobject_cast<CodeEditor *>(_mainTabWidget->widget(i));
            if(editor)
                break;
        }
        if(!editor)
            return;

        if(!closeFile(editor))
            return;
    }
}

// Close all other file, but the currently selected
void MainWindow::onFileCloseOthers()
{
    if(_helpWidget)
        return onFileCloseAll();
    if(!_codeEditor)
        return;

    for(;;) {
        int i;
        CodeEditor *editor = nullptr;
        for(i = 0; i < _mainTabWidget->count(); ++i) {
            editor = qobject_cast<CodeEditor *>(_mainTabWidget->widget(i));
            if(editor && editor != _codeEditor)
                break;
            editor = nullptr;
        }
        if(!editor)
            return;

        if(!closeFile(editor))
            return;
    }
}

// Save the current selected file.
void MainWindow::onFileSave()
{
    if(!_codeEditor)
        return;

    saveFile(_codeEditor, _codeEditor->path());
}

// Save the currently selected file under a different name and path.
void MainWindow::onFileSaveAs()
{
    if(!_codeEditor)
        return;

    saveFile(_codeEditor, "");
}

// Save all opened files.
void MainWindow::onFileSaveAll()
{
    for(int i = 0; i < _mainTabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor *>(_mainTabWidget->widget(i));
        if(editor && !saveFile(editor, editor->path()))
            return;
    }
}

// Select the next file to the right of the currently selected editor.
void MainWindow::onFileNext()
{
    if(_mainTabWidget->count() < 2)
        return;

    int i = _mainTabWidget->currentIndex() + 1;
    if(i >= _mainTabWidget->count())
        i = 0;

    _mainTabWidget->setCurrentIndex(i);
}

// Select the previous file to the left of the currently selected editor.
void MainWindow::onFilePrevious()
{
    if(_mainTabWidget->count() < 2)
        return;

    int i = _mainTabWidget->currentIndex() - 1;
    if(i < 0)
        i = _mainTabWidget->count() - 1;

    _mainTabWidget->setCurrentIndex(i);
}

// Open the preference dialogue for making changes to the IDE settings.
void MainWindow::onFilePrefs()
{

    _prefsDialog->setModal(true);

    _prefsDialog->exec();

    Prefs *prefs = Prefs::prefs();

    QString path = prefs->getString("cerberusPath");
    if(path != _cerberusPath) {
        if(isValidCerberusPath(path)) {
            _cerberusPath = path;
            enumTargets();
        } else {
            prefs->setValue("cerberusPath", _cerberusPath);
            QMessageBox::warning(this, "Tool Path Error", "Invalid Cerberus Path");
        }
    }

    path = prefs->getString("blitzmaxPath");
    if(path != _blitzmaxPath) {
        if(isValidBlitzmaxPath(path))
            _blitzmaxPath = path;

        else {
            prefs->setValue("blitzmaxPath", _blitzmaxPath);
            QMessageBox::warning(this, "Tool Path Error", "Invalid BlitzMax Path");
        }
    }

    updateActions();
    setIcons();
}

// Terminate the applications.
void MainWindow::onFileQuit()
{
    close();    // Trigger a close event.
    Application::quit();
}

//----------------------------------------------------------------------------------------------------------------------
// EDIT SLOTS
// Undo last text actions in the currently opened editor.
void MainWindow::onEditUndo()
{
    if(!_codeEditor)
        return;

    _codeEditor->undo();
}

// Redo previous undone text actions in the currently opened editor.
void MainWindow::onEditRedo()
{
    if(!_codeEditor)
        return;

    _codeEditor->redo();
}

// Cut the selected text in currently opened editor to the clipboard.
void MainWindow::onEditCut()
{
    if(!_codeEditor)
        return;

    _codeEditor->cut();
}

// Copy the selected text to the clipboard.
void MainWindow::onEditCopy()
{
    if(!_codeEditor)
        return;

    _codeEditor->copy();
}

// Block comment toggle all selected lines.
void MainWindow::onEditCommentUncommentBlock()
{
    if(!_codeEditor)
        return;
    _codeEditor->commentUncommentBlock();
}

// Paste the currently copied/cut text from the clipboard into the current text editor.
void MainWindow::onEditPaste()
{
    if(!_codeEditor)
        return;

    _codeEditor->paste();
}

// Delete the currently selected text.
void MainWindow::onEditDelete()
{
    if(!_codeEditor)
        return;

    _codeEditor->textCursor().removeSelectedText();
}

// Select all the text in a code editor.
void MainWindow::onEditSelectAll()
{
    if(!_codeEditor)
        return;

    _codeEditor->selectAll();

    updateActions();
}

// Open the find and replace dialogue for word searching.
void MainWindow::onEditFind()
{
    if(!_codeEditor)
        return;
    _findDialog->setModal(true);

    _findDialog->exec(_codeEditor->textCursor().selectedText());
}

// Find the next occurrence of a word search.
void MainWindow::onEditFindNext()
{
    if(!_codeEditor)
        return;

    onFindReplace(0);
}

// Find or replace searched text
void MainWindow::onFindReplace(int how)
{
    if(!_codeEditor)
        return;

    QString findText = _findDialog->findText();
    if(findText.isEmpty())
        return;

    QString replaceText = _findDialog->replaceText();

    bool cased = _findDialog->caseSensitive();

    bool wrap = true;

    switch(how) {
        case 1: {
            if(_codeEditor->replace(findText, replaceText, cased)) {
                if(!_codeEditor->findNext(findText, cased, wrap))
                    Application::beep();
            }
            break;
        }
        case 2: {
            int n = _codeEditor->replaceAll(findText, replaceText, cased, wrap);
            QMessageBox::information(this, "Replace All", QString::number(n) + " occurrences replaced");
            break;
        }
        default: {
            if(!_codeEditor->findNext(findText, cased, wrap))
                Application::beep();
        }
    }
}

// Go to a direct line number in the currently selected editor.
void MainWindow::onEditGoto()
{
    if(!_codeEditor)
        return;

    bool ok = false;
    int line =
        QInputDialog::getInt(this, "Go to Line", "Line number:", 1, 1, _codeEditor->document()->blockCount(), 1, &ok);
    if(ok) {
        _codeEditor->gotoLine(line - 1);
        _codeEditor->highlightLine(line - 1);
    }
}

// Open the Find In File dialogue
void MainWindow::onEditFindInFiles()
{
    _findInFilesDialog->show();
    _findInFilesDialog->raise();
}

//----------------------------------------------------------------------------------------------------------------------
// VIEW SLOTS
// Set the visibility state of the dockable tool bars
void MainWindow::onViewToolBar()
{
    if(sender() == _ui->actionViewFile)
        _ui->fileToolBar->setVisible(_ui->actionViewFile->isChecked());

    else if(sender() == _ui->actionViewEdit)
        _ui->editToolBar->setVisible(_ui->actionViewEdit->isChecked());

    else if(sender() == _ui->actionViewBuild)
        _ui->buildToolBar->setVisible(_ui->actionViewBuild->isChecked());

    else if(sender() == _ui->actionViewHelp)
        _ui->helpToolBar->setVisible(_ui->actionViewHelp->isChecked());
}

// Set the visibility state of the dockable windows
void MainWindow::onViewWindow()
{
    if(sender() == _ui->actionViewBrowser)
        _browserDockWidget->setVisible(_ui->actionViewBrowser->isChecked());

    else if(sender() == _ui->actionViewConsole) {
        // In the event that the current main tab widget is an editor. Then check to see if the completer pop-up menu is
        // visible. If it is, then the completer need to be closed and console visibility check skipped.
        CodeEditor *editor = qobject_cast<CodeEditor *>(_mainTabWidget->currentWidget());
        bool isCompleterVisible = false;
        if(editor)
            isCompleterVisible = editor->isCompleterVisible();

        if(!isCompleterVisible)
            _consoleDockWidget->setVisible(_ui->actionViewConsole->isChecked());
        else
            editor->closeCompleter();
    }
}

// Toggle the book mark of a line.
void MainWindow::onToggleBookmark()
{
    if(_codeEditor)
        _codeEditor->bookmarkToggle();
}

// Select the previous line in the book mark list
void MainWindow::onPreviousBookmark()
{
    if(_codeEditor)
        _codeEditor->bookmarkPrev();
}

// Select the next line in the book mark list
void MainWindow::onNextBookmark()
{
    if(_codeEditor)
        _codeEditor->bookmarkNext();
}

// Toggle full screen mode.
void MainWindow::onToggleFullscreen()
{
    setWindowState(windowState() ^ Qt::WindowFullScreen);
}

//----------------------------------------------------------------------------------------------------------------------
// BUILD SLOTS
void MainWindow::onBuildBuild()
{
    build("build");
}

void MainWindow::onBuildRun()
{
    if(_debugTreeModel)
        _debugTreeModel->run();

    else
        build("run");
}

void MainWindow::onBuildCheck()
{
    build("check");
}

void MainWindow::onBuildUpdate()
{
    build("update");
}

void MainWindow::onDebugStep()
{
    if(!_debugTreeModel)
        return;
    _debugTreeModel->step();
}

void MainWindow::onDebugStepInto()
{
    if(!_debugTreeModel)
        return;
    _debugTreeModel->stepInto();
}

void MainWindow::onDebugStepOut()
{
    if(!_debugTreeModel)
        return;
    _debugTreeModel->stepOut();
}

void MainWindow::onDebugKill()
{
    if(!_consoleProc)
        return;

    _consoleTextWidget->setTextColor(Prefs::prefs()->getColor("console4Color"));
    print("Killing process...");

    _consoleProc->kill();
}

// Show a dialogue to select the target to build for.
void MainWindow::onBuildTarget()
{
    QStringList items;
    for(int i = 0; i < _targetsWidget->count(); ++i)
        items.push_back(_targetsWidget->itemText(i));

    bool ok = false;
    QString item = QInputDialog::getItem(this, "Select build target", "Build target:", items,
                                         _targetsWidget->currentIndex(), false, &ok);
    if(ok) {
        int index = items.indexOf(item);
        if(index != -1)
            _targetsWidget->setCurrentIndex(index);
    }
}

// Show a dialogue to select the config to build for.
void MainWindow::onBuildConfig()
{
    QStringList items;
    for(int i = 0; i < _configsWidget->count(); ++i)
        items.push_back(_configsWidget->itemText(i));

    bool ok = false;
    QString item = QInputDialog::getItem(this, "Select build config", "Build config:", items,
                                         _configsWidget->currentIndex(), false, &ok);
    if(ok) {
        int index = items.indexOf(item);
        if(index != -1)
            _configsWidget->setCurrentIndex(index);
    }
}

// Lock an editor as the default source file to build.
void MainWindow::onBuildLockFile()
{
    if(_codeEditor && _codeEditor != _lockedEditor) {
        CodeEditor *wasLocked = _lockedEditor;
        _lockedEditor = _codeEditor;
        updateTabLabel(_lockedEditor);
        if(wasLocked)
            updateTabLabel(wasLocked);
    }
    updateActions();
}

// Free the locked build editor for building any editor file currently selected..
void MainWindow::onBuildUnlockFile()
{
    if(CodeEditor *wasLocked = _lockedEditor) {
        _lockedEditor = nullptr;
        updateTabLabel(wasLocked);
    }
    updateActions();
}

// Open a directory as a project folder.
void MainWindow::onBuildAddProject()
{
    QString dir = fixPath(QFileDialog::getExistingDirectory(this, "Select project directory", _defaultDir,
                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks));
    if(dir.isEmpty())
        return;

    if(!_projectTreeModel->addProject(dir))
        QMessageBox::warning(this, "Add Project Error", "Error adding project: " + dir);
}

//----------------------------------------------------------------------------------------------------------------------
// HELP SLOTS
void MainWindow::onHelpHome()
{
    QString home = _cerberusPath + "/docs/html/Home.html";
    if(!QFile::exists(home))
        return;

    openFile(FILE_URL + home, false);
}

void MainWindow::onHelpBack()
{
    if(!_helpWidget)
        return;

    _helpWidget->back();
}

void MainWindow::onHelpForward()
{
    if(!_helpWidget)
        return;

    _helpWidget->forward();
}

void MainWindow::onHelpQuickHelp()
{
    if(!_codeEditor)
        return;

    QString ident = _codeEditor->identAtCursor();
    if(ident.isEmpty())
        return;

    onShowHelpWithStatusbar(ident);
}

void MainWindow::onHelpCerberusHomepage()
{
    QString s = "https://www.cerberus-x.com/";
    QDesktopServices::openUrl(s);
}

// Show information about the application.
void MainWindow::onHelpAbout()
{

    QString CERBERUS_VERSION = "?????";

    QFile file(_cerberusPath + "/VERSIONS.TXT");
    if(file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        STREAMS_UTF8 // MACRO: Only used if Qt version is less than 6.0.0

        QString text = stream.readAll();
        file.close();
        QStringList lines = text.split('\n');
        for(int i = 0; i < lines.count(); ++i) {
            QString line = lines.at(i).trimmed();
            if(line.startsWith("***** v")) {
                QString v = line.mid(7);
                int j = v.indexOf(" *****");
                if(j + 6 == v.length()) {
                    CERBERUS_VERSION = v.left(j);
                    break;
                }
            }
        }
    }
    QString webSite = "https://www.cerberus-x.com";
    QString ABOUT = "<html><head><style>a{color:#FFEE00;}</style></head><body>"
                    "Ted V" TED_VERSION "<br><br>"
                    "Cerberus V" +
                    CERBERUS_VERSION +
                    "<br>"
                    "Trans V" +
                    _transVersion +
                    "<br>"
                    "QT V" +
                    (QT_VERSION_STR) +
                    "<br><br>"
                    "A simple editor/IDE for the Cerberus programming language.<br><br>"
                    "Copyright Blitz Research Ltd for Monkey X.<br><br>"
                    "Cerberus X is maintained by Michael Hartlef, Philipp Moeller & Olivier Stucker.<br<br>"
                    "Further additions done by several member of the Cerberus X community.<br>"
                    "Please visit <a href=\"" +
                    webSite +
                    "\">www.cerberus-x.com</a> for more information on Cerberus X."
                    "</body></html>";

    QMessageBox::information(this, "About Ted", ABOUT);
}

// Rebuild the help files.
void MainWindow::onHelpRebuild()
{
    if(_consoleProc || _cerberusPath.isEmpty())
        return;

    onFileSaveAll();
    _rebuildingHelp = true;
    QString cmd = "\"${CERBERUSPATH}/bin/makedocs" + HOST + "\"";
    runCommand(cmd, nullptr);
}

// Open the help menu
void MainWindow::onShowHelpView()
{
    if(_helpTopic.isEmpty())
        return;

    if(!_helpTopicId)
        _helpTopicId = 1;
    ++_helpTopicId;

    QString tmp = _helpTopic + "(" + QString::number(_helpTopicId) + ")";
    QString url = _helpUrls.value(tmp);

    if(url.isEmpty()) {

        url = _helpUrls.value(_helpTopic);
        if(url.isEmpty()) {
            _helpTopic = "";
            return;
        }
        _helpTopicId = 0;
    }

    openFile(url, false);
}

//----------------------------------------------------------------------------------------------------------------------
//  MainWindow: PRIVATE MEMEBR SLOTS
//----------------------------------------------------------------------------------------------------------------------
// If the _targetsWidget triggers a currentIndexChanged event and the build file.
// If different, then set the active backend based on the file type.
void MainWindow::onTargetChanged(int index)
{
    QString target = _targetsWidget->itemText(index);

    if(_buildFileType == "cerberus" || _buildFileType == "monkey")
        _activeCerberusTarget = target;

    else if(_buildFileType == "mx2" || _buildFileType == "monkey2")
        _activeMonkey2Target = target;
}

// Show the help view based on the index passed.
// This member function used to pass a QString, but the function signature was removed for Qt6.
void MainWindow::onShowHelp(int index)
{
    QString topic = _indexWidget->itemText(index);
    QString url = _helpUrls.value(topic);

    if(url.isEmpty()) {
        _helpTopic = "";
        return;
    }

    _helpTopic = topic;
    _helpTopicId = 0;

    openFile(url, false);
}

// Open a help file using a context string.
// NOTE: This member function at some point should be removed.
// If I remember, the original purpose was to show a function signature in the help file.
void MainWindow::onShowHelpWithStatusbar(const QString &topic)
{
    QString url = _helpUrls.value(topic);

    /* This is pretty much useless here. See the loadHelpIndex member function.
    //QString status = _helpF1.value( topic );

    if ( !status.isEmpty() && _helpTopic!=topic) {
        statusBar()->showMessage( "Help->  " + status );
        _helpTopic=topic;
        return;
    }*/

    if(url.isEmpty()) {
        _helpTopic = "";
        return;
    }

    _helpTopic = topic;
    _helpTopicId = 0;

    openFile(url, false);
}

// Close a tab and update the status bar
void MainWindow::onCloseMainTab(int index)
{
    closeFile(_mainTabWidget->widget(index));
    updateStatusBar();
    updateActions();
}

// Update the editor if tabs are reorder.
void MainWindow::onMainTabChanged(int index)
{
    CodeEditor *_oldEditor = _codeEditor;
    QWidget *widget = _mainTabWidget->widget(index);
    _codeEditor = qobject_cast<CodeEditor *>(widget);
    _helpWidget = qobject_cast<HelpView *>(widget);

    if(_oldEditor)
        disconnect(_oldEditor, SIGNAL(showCode(QString, int)), this, SLOT(onShowCode(QString, int)));

    if(_codeEditor) {
        replaceTabWidgetWidget(_browserTabWidget, 1, _codeEditor->codeTreeView());
        connect(_codeEditor, SIGNAL(showCode(QString, int)), SLOT(onShowCode(QString, int)));
        _codeEditor->setFocus(Qt::OtherFocusReason);

    } else
        replaceTabWidgetWidget(_browserTabWidget, 1, _emptyCodeWidget);
    updateStatusBar();
    updateWindowTitle();
    updateActions();
}

// Update the dockable windows and tool bars.
// NOTE: Visibility of dockable widgets is done in updateActions.
void MainWindow::onDockVisibilityChanged(bool visible)
{
    static_cast<void>(visible);
    updateActions();
}

// Show the context menu for the projects view.
void MainWindow::onProjectMenu(const QPoint &pos)
{
    QModelIndex index = _projectTreeWidget->indexAt(pos);
    if(!index.isValid())
        return;

    QFileInfo info = _projectTreeModel->fileInfo(index);

    QMenu *menu = nullptr;

    // If the current selected item is that of a project folder, then the variable menu is set to show menu options for
    // projects. If not, then the menu options are set for either files or creating files or directories etc.
    if(_projectTreeModel->isProject(index))
        menu = _projectPopupMenu;

    else if(info.isFile()) {
        menu = _filePopupMenu;
        QString suffix = info.suffix().toLower();
        bool browsable = (suffix == "txt" || suffix == "htm" || suffix == "html");
        _ui->actionOpen_in_Help->setEnabled(browsable);

    } else
        menu = _dirPopupMenu;

    if(!menu)
        return;

    // Set the action to preform based on the menu selected.
    QAction *action = menu->exec(_projectTreeWidget->mapToGlobal(pos));
    if(!action)
        return;

    // Preform creation of a new file.
    if(action == _ui->actionNewFile) {
        bool ok = false;
        QString name = QInputDialog::getText(this, "Create File", "File name: " + info.filePath() + "/",
                                             QLineEdit::Normal, "", &ok);
        if(ok && !name.isEmpty()) {
            if(extractExt(name).isEmpty())
                name += ".cxs";
            QString path = info.filePath() + "/" + name;
            if(QFileInfo(path).exists()) {
                if(QMessageBox::question(this, "Create File", "Okay to overwrite existing file: " + path + " ?",
                                         QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok)
                    newFile(path);
            } else
                newFile(path);
        }

        // Preform action of creating an new directory.
    } else if(action == _ui->actionNewFolder) {
        bool ok = false;
        QString name = QInputDialog::getText(this, "Create Folder", "Folder name: " + info.filePath() + "/",
                                             QLineEdit::Normal, "", &ok);
        if(ok && !name.isEmpty()) {
            if(!QDir(info.filePath()).mkdir(name))
                QMessageBox::warning(this, "Create Folder", "Create folder failed");
        }

        // Preform action of renaming a file or directory.
    } else if(action == _ui->actionRenameFile) {
        bool ok = false;
        QString newName = QInputDialog::getText(this, "Rename file", "New name:", QLineEdit::Normal,
                                                info.fileName(), &ok);
        if(ok) {
            QString oldPath = info.filePath();
            QString newPath = info.path() + "/" + newName;
            if(QFile::rename(oldPath, newPath)) {
                for(int i = 0; i < _mainTabWidget->count(); ++i) {
                    if(CodeEditor *editor = qobject_cast<CodeEditor *>(_mainTabWidget->widget(i))) {
                        if(editor->path() == oldPath) {
                            editor->evaluatefiletype(newPath);
                            updateTabLabel(editor);
                        }
                    }
                }
            } else
                QMessageBox::warning(this, "Rename Error", "Error renaming file: " + oldPath);

        }
        // Preform action of opening the file or directory with the operating systems own default applications.
    } else if(action == _ui->actionOpen_on_Desktop)
        QDesktopServices::openUrl(FILE_URL + info.filePath());

    // Open any help files in the operating systems own default web browser.
    else if(action == _ui->actionOpen_in_Help)
        openFile(FILE_URL + info.filePath(), false);

    // Preform action of deleting a file or directory.
    else if(action == _ui->actionDeleteFile) {
        QString path = info.filePath();

        if(info.isDir()) {
            if(QMessageBox::question(this, "Delete file",
                                     "Okay to delete directory: " + path +
                                     " ?\n\n*** WARNING *** all subdirectories will also be deleted!",
                                     QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
                if(!removeDir(path))
                    QMessageBox::warning(this, "Delete Error", "Error deleting directory: " + info.filePath());
            }
        } else {
            if(QMessageBox::question(this, "Delete file", "Okay to delete file: " + path + " ?",
                                     QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
                if(QFile::remove(path)) {
                    for(int i = 0; i < _mainTabWidget->count(); ++i) {
                        if(CodeEditor *editor = qobject_cast<CodeEditor *>(_mainTabWidget->widget(i))) {
                            if(editor->path() == path) {
                                closeFile(editor);
                                i = -1;
                            }
                        }
                    }
                } else
                    QMessageBox::warning(this, "Delete Error", "Error deleting file: " + info.filePath());
            }
        }
        // Preform the action of closing a project directory.
    } else if(action == _ui->actionCloseProject) {

        // Workaround for Qt file system issue on Linux
#ifdef Q_OS_WIN
        _projectTreeModel->removeProject(info.filePath());
#else
        QString f;
        if(info.filePath().startsWith("//"))
            f = info.filePath().replace("//", "/");

        else
            f = info.filePath();
        _projectTreeModel->removeProject(f);
#endif
        // Preform action of starting the find in files dialogue.
    } else if(action == _ui->actionEditFindInFiles) {

        _findInFilesDialog->show(info.filePath());

        _findInFilesDialog->raise();
    }
}

// Open a select file in the projects view.
void MainWindow::onFileClicked(const QModelIndex &index)
{
    if(!_projectTreeModel->isDir(index))
        openFile(_projectTreeModel->filePath(index), true);
}

// In the event of any editing, update the label of the tab to show a asterisk and update any UI widgets related to
// any changes. i.e. the save file actions.
void MainWindow::onTextChanged()
{
    if(CodeEditor *editor = qobject_cast<CodeEditor *>(sender())) {
        if(editor->modified() < 2)
            updateTabLabel(editor);
    }
    updateActions();
}

// Update the line cursor information text in the status bar.
void MainWindow::onCursorPositionChanged()
{
    if(sender() == _codeEditor)
        _statusWidget->setText("Line: " + QString::number(_codeEditor->textCursor().blockNumber() + 1));
    updateActions();
}

// Jump to a line in an editor
void MainWindow::onShowCode(const QString &path, int line)
{
    if(CodeEditor *editor = qobject_cast<CodeEditor *>(openFile(path, true))) {
        editor->gotoLine(line);
        editor->highlightLine(line);

        if(editor == _codeEditor)
            editor->setFocus(Qt::OtherFocusReason);
    }
}

// Used by find in files to open an editor to the location of a search text item.
void MainWindow::onShowCode(const QString &path, int pos, int len)
{
    if(CodeEditor *editor = qobject_cast<CodeEditor *>(openFile(path, true))) {
        QTextCursor cursor(editor->document());
        cursor.setPosition(pos);
        cursor.setPosition(pos + len, QTextCursor::KeepAnchor);
        editor->setTextCursor(cursor);

        if(editor == _codeEditor)
            editor->setFocus(Qt::OtherFocusReason);
    }
}

// If a running process outputs to the stdout stream, then process it.
void MainWindow::onProcStdout()
{
    static QString comerr = " : Error : ";
    static QString runerr = "Cerberus Runtime Error : ";

    QString text = _consoleProc->readLine(0);

    _consoleTextWidget->setTextColor(Prefs::prefs()->getColor("console2Color"));
    print(text);

    if(text.contains(comerr)) {
        int i0 = text.indexOf(comerr);
        QString info = text.left(i0);
        int i = info.lastIndexOf('<');
        if(i != -1 && info.endsWith('>')) {
            QString path = info.left(i);
            int line = info.mid(i + 1, info.length() - i - 2).toInt() - 1;
            QString err = text.mid(i0 + comerr.length());

            onShowCode(path, line);

            QMessageBox::warning(this, "Compile Error", err);
        }
    } else if(text.startsWith(runerr)) {
        QString err = text.mid(runerr.length());

        // Show the IDE window and bring it into the front.
        // NOTE: If this cause issues, then it will need to set the showNormal and changedEvent overridden and a method
        // used to preserve the state before calling showNormal.
        show();
        raise();
        activateWindow();
        QMessageBox::warning(this, "Cerberus Runtime Error", err);
    }
}

// If a running process output to the stderr stream, then process it.
void MainWindow::onProcStderr()
{
    if(_debugTreeModel && _debugTreeModel->stopped())
        return;

    QString text = _consoleProc->readLine(1);

    if(text.startsWith("{{~~") && text.endsWith("~~}}")) {
        QString info = text.mid(4, text.length() - 8);

        int i = info.lastIndexOf('<');
        if(i != -1 && info.endsWith('>')) {
            QString path = info.left(i);
            int line = info.mid(i + 1, info.length() - i - 2).toInt() - 1;
            onShowCode(path, line);
        } else {
            _consoleTextWidget->setTextColor(Prefs::prefs()->getColor("console3Color"));
            print(info);
        }

        if(!_debugTreeModel) {
            raise();

            _debugTreeModel = new DebugTreeModel(_consoleProc);
            connect(_debugTreeModel, SIGNAL(showCode(QString, int)), SLOT(onShowCode(QString, int)));

            _debugTreeWidget->setModel(_debugTreeModel);
            connect(_debugTreeWidget, &QTreeView::clicked, _debugTreeModel, &DebugTreeModel::onClicked);

            _browserTabWidget->setCurrentWidget(_debugTreeWidget);

            _consoleTextWidget->setTextColor(QColor(192, 96, 0));
            print("STOPPED");
        }

        _debugTreeModel->stop();
        updateActions();

        return;
    }

    _consoleTextWidget->setTextColor(Prefs::prefs()->getColor("stringsColor"));
    print(text);
}

// If a process signals that there are line for processing, then select the correct.
void MainWindow::onProcLineAvailable(int channel)
{
    static_cast<void>(channel);

    while(_consoleProc) {
        if(_consoleProc->isLineAvailable(0))
            onProcStdout();

        else if(_consoleProc->isLineAvailable(1))
            onProcStderr();

        else
            return;
    }
}

// Clean up after a process has finished running.
void MainWindow::onProcFinished()
{
    while(_consoleProc->waitLineAvailable(0, 100))
        onProcLineAvailable(0);

    _consoleTextWidget->setTextColor(QColor(Prefs::prefs()->getColor("console4Color")));
    print("Done.");

    // Rebuild the help files and reset to the home page.
    if(_rebuildingHelp) {
        _rebuildingHelp = false;
        loadHelpIndex();
        for(int i = 0; i < _mainTabWidget->count(); ++i) {
            HelpView *helpView = qobject_cast<HelpView *>(_mainTabWidget->widget(i));
            if(helpView)
                helpView->triggerPageAction(QWebEnginePage::ReloadAndBypassCache);
        }
        onHelpHome();
    }

    // Clean up the debug tree.
    if(_debugTreeModel) {
        _debugTreeWidget->setModel(nullptr);
        delete _debugTreeModel;
        _debugTreeModel = nullptr;
    }

    // Clean up the console process
    if(_consoleProc) {
        delete _consoleProc;
        _consoleProc = nullptr;
    }

    updateActions();
    statusBar()->showMessage("Ready.");
}

// Execute a pop-up window at the mouse cursor position.
void MainWindow::onEditorMenu(const QPoint &pos)
{
    _editorPopupMenu->exec(_codeEditor->mapToGlobal(pos));
}

//----------------------------------------------------------------------------------------------------------------------
//  MainWindow: PROTECTED MEMEBR SLOTS
//----------------------------------------------------------------------------------------------------------------------
// NOTE: MS Windows Only. Deal with the issues with full screen mode and hidden menus.
#ifdef Q_OS_WIN
bool MainWindow::event(QEvent *e)
{
    // Capture the window state change event
    if(e->type() == QEvent::WindowStateChange) {
        // Make sure that there is a valid window handle before doing any native operations on a window.
        if(windowHandle()) {
            // Add a one pixel boarder if the window state is that for full screen.
            HWND handle = reinterpret_cast<HWND>(winId());
            if(windowState() & Qt::WindowFullScreen)
                SetWindowLongPtr(handle, GWL_STYLE, GetWindowLongPtr(handle, GWL_STYLE) | WS_BORDER);
            else
                SetWindowLongPtr(handle, GWL_STYLE, GetWindowLongPtr(handle, GWL_STYLE));
        }
    }

    return QMainWindow::event(e);
}
#endif

