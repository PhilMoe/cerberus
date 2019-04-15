/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.

Change Log
--------------------------------------------------------------------------------
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "std.h"
#ifdef Q_OS_WIN
#include <QtPlatformHeaders\QWindowsWindowFunctions>
#endif

class CodeEditor;
class ProjectTreeModel;
class DebugTreeModel;
class FindDialog;
class Process;
class FindInFilesDialog;


namespace Ui {
class MainWindow;
}

// DAWLANE Qt 5.6+ supported
#if QT_VERSION>0x050501
class HelpView : public QWebEngineView{
    Q_OBJECT
public:
    HelpView(QWidget *parent=Q_NULLPTR): QWebEngineView( parent ){}
protected:
    void keyPressEvent ( QKeyEvent * event );
};

class WebEnginePage : public QWebEnginePage
{
    Q_OBJECT
public:
    WebEnginePage(QObject *parent=Q_NULLPTR): QWebEnginePage( parent ){}
    bool acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool);

signals:
    void linkClicked( const QUrl &url);
};
#else
class HelpView : public QWebView{
    Q_OBJECT
public:
protected:
    void keyPressEvent ( QKeyEvent * event );
};
#endif

class Prefs;
class PrefsDialog;

class MainWindow : public QMainWindow{
    Q_OBJECT
public:

    MainWindow( QWidget *parent=nullptr );
    ~MainWindow();

    void cdebug( const QString &str );

    void updateHelp();
    void onShowHelp();

    void setIcons();
    QIcon getThemeIcon(const QString &theme, const QString &ic, const QString &icd);
    QStringList _completeList;

// DAWLANE Qt 5.6+ supported. Function openFile moved here so that WebEnginePage can access it.
    QWidget *openFile( const QString &path,bool addToRecent );
// DAWLANE Move void parseAppArgs(); to public to pass new arguments if another instance of Ted was started.
    void parseAppArgs(QStringList &args);
private:

    void loadHelpIndex();

    bool isBuildable( CodeEditor *editor );
    QString widgetPath( QWidget *widget );
    CodeEditor *editorWithPath( const QString &path );

    QWidget *newFile( const QString &path );
    QWidget *newFileTemplate( const QString &path );

    bool saveFile( QWidget *widget,const QString &path );
    bool closeFile( QWidget *widget,bool remove=true );

    bool isValidCerberusPath( const QString &path );
    bool isValidBlitzmaxPath( const QString &path );
    QString defaultCerberusPath();
    void enumTargets();

    void loadTheme(QString &appPath, Prefs *prefs);
    void readSettings();
    void writeSettings();

    void updateWindowTitle();
    void updateTabLabel( QWidget *widget );
    void updateTargetsWidget( QString fileType );
    void updateActions();

    void print( const QString &str );
    void runCommand( QString cmd,QWidget *fileWidget );
    void build( QString mode );

    bool confirmQuit();
    void closeEvent( QCloseEvent *event );
    void showImage(const QString &path);
    void playAudio(const QString &path);
    void showDoc(const QString &path);

public slots:

    //File menu
    void onFileNew();
    void onFileNewTemplate();
    void onFileOpen();
    void onFileOpenRecent();
    void onFileClose();
    void onFileCloseAll();
    void onFileCloseOthers();
    void onFileSave();
    void onFileSaveAs();
    void onFileSaveAll();
    void onFileNext();
    void onFilePrevious();
    void onFilePrefs();
    void onFileQuit();

    //Edit menu
    void onEditUndo();
    void onEditRedo();
    void onEditCut();
    void onEditCopy();
    void onEditCommentUncommentBlock();
    void onEditPaste();
    void onEditDelete();
    void onEditSelectAll();
    void onEditFind();
    void onEditFindNext();
    void onFindReplace( int how );
    void onEditGoto();
    void onEditFindInFiles();

    //View menu
    void onViewToolBar();
    void onViewWindow();
    void onToggleBookmark();
    void onPreviousBookmark();
    void onNextBookmark();
    void onToggleFullscreen();

    //Build/Debug menu
    void onBuildBuild();
    void onBuildRun();
    void onBuildCheck();
    void onBuildUpdate();
    void onDebugStep();
    void onDebugStepInto();
    void onDebugStepOut();
    void onDebugKill();
    void onBuildTarget();
    void onBuildConfig();
    void onBuildLockFile();
    void onBuildUnlockFile();
    void onBuildAddProject();

    //Help menu
    void onHelpHome();
    void onHelpBack();
    void onHelpForward();
    void onHelpQuickHelp();
    void onHelpCerberusHomepage();
    void onHelpAbout();
    void onHelpRebuild();

private slots:

    void onTargetChanged( int index );

    void onShowHelp( const QString &text );
    void onShowHelpWithStatusbar( const QString &text );

// DAWLANE Qt 5.6+ supported -  onLinkClick is not required as QtWebEngine works differently.
    #if QT_VERSION<=0x050501
    void onLinkClicked( const QUrl &url );
#endif

    void onCloseMainTab( int index );
    void onMainTabChanged( int index );

    void onDockVisibilityChanged( bool visible );

    void onProjectMenu( const QPoint &pos );
    void onFileClicked( const QModelIndex &index );

    void onTextChanged();
    void onCursorPositionChanged();
    void onShowCode( const QString &path,int line );
    void onShowCode( const QString &path,int pos,int len );

    void onProcStdout();
    void onProcStderr();
    void onProcLineAvailable( int channel );
    void onProcFinished();
    void onEditorMenu(const QPoint &pos);

private:

    QMap<QString,QString> _helpUrls;
    QMap<QString,QString> _helpF1;

    Ui::MainWindow *_ui;

    QString _defaultDir;

    QString _blitzmaxPath;
    QString _cerberusPath;
    QString _monkey2Path;

    QString _transVersion;

    QTabWidget *_mainTabWidget;

    Qt::WindowStates _windowState;

    QTextEdit *_consoleTextWidget;
    QDockWidget *_consoleDockWidget;
    Process *_consoleProc;

    QTabWidget *_browserTabWidget;
    QDockWidget *_browserDockWidget;

    ProjectTreeModel *_projectTreeModel;
    QTreeView *_projectTreeWidget;

    QWidget *_emptyCodeWidget;

    DebugTreeModel *_debugTreeModel;
    QTreeView *_debugTreeWidget;

    CodeEditor *_codeEditor;
    CodeEditor *_lockedEditor;
    HelpView *_helpWidget;

    PrefsDialog *_prefsDialog;
    FindDialog *_findDialog;
    FindInFilesDialog *_findInFilesDialog;

    QMenu *_projectPopupMenu;
    QMenu *_dirPopupMenu;
    QMenu *_filePopupMenu;

    QMenu *_editorPopupMenu;

    QLabel *_statusWidget;

    QComboBox *_targetsWidget;
    QComboBox *_configsWidget;
    QComboBox *_indexWidget;

    QString _helpTopic;
    int _helpTopicId;

    bool _rebuildingHelp;

    QString _buildFileType;

    QString _activeCerberusTarget;
    QString _activeMonkey2Target;

    QVector<QString> _cerberusTargets;
    QVector<QString> _monkey2Targets;
};

#endif // MAINWINDOW_H
