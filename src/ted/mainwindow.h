/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "std.h"

class CodeEditor;
class ProjectTreeModel;
class DebugTreeModel;
class FindDialog;
class Process;
class FindInFilesDialog;

namespace Ui {
class MainWindow;
}

class Prefs;
class PrefsDialog;

class HelpView : public QWebView{
    Q_OBJECT
public:

protected:
    void keyPressEvent ( QKeyEvent * event );
};

class MainWindow : public QMainWindow{
    Q_OBJECT
public:

    MainWindow( QWidget *parent=0 );
    ~MainWindow();

    void cdebug( const QString &str );

    void updateHelp();
    void onShowHelp();

private:

    void parseAppArgs();
    void loadHelpIndex();

    bool isBuildable( CodeEditor *editor );
    QString widgetPath( QWidget *widget );
    CodeEditor *editorWithPath( const QString &path );

    QWidget *newFile( const QString &path );
    QWidget *openFile( const QString &path,bool addToRecent );
    bool saveFile( QWidget *widget,const QString &path );
    bool closeFile( QWidget *widget,bool remove=true );

    bool isValidCerberusPath( const QString &path );
    bool isValidBlitzmaxPath( const QString &path );
    QString defaultCerberusPath();
    void enumTargets();

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

public slots:

    //File menu
    void onFileNew();
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
    void onHelpAbout();
    void onHelpRebuild();

private slots:

    void onTargetChanged( int index );

    void onShowHelp( const QString &text );

    void onLinkClicked( const QUrl &url );

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

private:

    QMap<QString,QString> _helpUrls;

    Ui::MainWindow *_ui;

    QString _defaultDir;

    QString _blitzmaxPath;
    QString _cerberusPath;
    QString _monkey2Path;

    QString _transVersion;

    QTabWidget *_mainTabWidget;

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
