//----------------------------------------------------------------------------------------------------------------------
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
//          2022-12-26: DAWLANE
//                      Removed old variable for storing previous window state.
//          2022-12-23: DAWLANE
//                      Fixed A MS Windows issue with menus in full screen mode.
//                      Overrides the main window event to force a one pixel boarder around a full screen window.
//          2022-10-20: DAWLANE - Overhaul of code to remove Qt4 supported code.
//                      Updated to work with Qt5.9+ and Qt6.2+.
//                      Moved the WebEngineView and WebEnginePage into it's own file
//                      class module.
//                      All member pointer variables initialised here and not in the
//                      implementation code.

#pragma once
#include "std.h"

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QTextEdit>
#include <QTreeView>

class CodeEditor;
class ProjectTreeModel;
class DebugTreeModel;
class FindDialog;
class Process;
class FindInFilesDialog;
class HelpView;

namespace Ui {
    class MainWindow;
}

class Prefs;
class PrefsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void cdebug(const QString &str);
    void updateHelp();

    int CompleteListCount();
    QString CompleteListItem(const int item);

    QWidget *openFile(const QString &path, bool addToRecent);
    void parseAppArgs(QStringList &args);

private:
    void setIcons();
    QIcon getThemeIcon(const QString &theme, const QString &ic, const QString &icd);

    void loadHelpIndex();

    bool isBuildable(CodeEditor *editor);
    QString widgetPath(QWidget *widget);
    CodeEditor *editorWithPath(const QString &path);

    QWidget *newFile(const QString &path);
    QWidget *newFileTemplate(const QString &path);

    bool saveFile(QWidget *widget, const QString &path);
    bool closeFile(QWidget *widget, bool remove = true);

    bool isValidCerberusPath(const QString &path);
    bool isValidBlitzmaxPath(const QString &path);
    QString defaultCerberusPath();
    void enumTargets();

    void loadTheme(QString &appPath, Prefs *prefs);
    void readSettings();
    void writeSettings();

    void updateWindowTitle();
    void updateTabLabel(QWidget *widget);
    void updateTargetsWidget(QString fileType);
    void updateActions();

    void print(const QString &str);
    void runCommand(QString cmd, QWidget *fileWidget);
    void build(QString mode);

    bool confirmQuit();
    void closeEvent(QCloseEvent *event);
    void showImage(const QString &path);
    void playAudio(const QString &path);
    void showDoc(const QString &path);
    void updateStatusBar();

public slots:

    // File menu
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

    // Edit menu
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
    void onFindReplace(int how);
    void onEditGoto();
    void onEditFindInFiles();

    // View menu
    void onViewToolBar();
    void onViewWindow();
    void onToggleBookmark();
    void onPreviousBookmark();
    void onNextBookmark();
    void onToggleFullscreen();

    // Build/Debug menu
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

    // Help menu
    void onHelpHome();
    void onHelpBack();
    void onHelpForward();
    void onHelpQuickHelp();
    void onHelpCerberusHomepage();
    void onHelpAbout();
    void onHelpRebuild();
    void onShowHelpView();

private slots:

    void onTargetChanged(int index);
    void onShowHelp(int index);
    void onShowHelpWithStatusbar(const QString &text);

    void onCloseMainTab(int index);
    void onMainTabChanged(int index);

    void onDockVisibilityChanged(bool visible);

    void onProjectMenu(const QPoint &pos);
    void onFileClicked(const QModelIndex &index);

    void onTextChanged();
    void onCursorPositionChanged();
    void onShowCode(const QString &path, int line);
    void onShowCode(const QString &path, int pos, int len);

    void onProcStdout();
    void onProcStderr();
    void onProcLineAvailable(int channel);
    void onProcFinished();
    void onEditorMenu(const QPoint &pos);

#ifdef Q_OS_WIN
protected:
    bool event(QEvent *e);
#endif

private:
    QMap<QString, QString> _helpUrls;
    // QMap<QString,QString> _helpF1;    // NOTE: See loadHelpIndex in the mainwindow.cpp file.

    Ui::MainWindow *_ui = nullptr;

    QString _defaultDir;

    QString _blitzmaxPath;
    QString _cerberusPath;
    QString _monkey2Path;

    QString _transVersion;

    QTabWidget *_mainTabWidget = nullptr;

    QTextEdit *_consoleTextWidget = nullptr;
    QDockWidget *_consoleDockWidget = nullptr;
    Process *_consoleProc = nullptr;

    QTabWidget *_browserTabWidget = nullptr;
    QDockWidget *_browserDockWidget = nullptr;

    ProjectTreeModel *_projectTreeModel = nullptr;
    QTreeView *_projectTreeWidget = nullptr;

    QWidget *_emptyCodeWidget = nullptr;

    DebugTreeModel *_debugTreeModel = nullptr;
    QTreeView *_debugTreeWidget = nullptr;

    CodeEditor *_codeEditor = nullptr;
    CodeEditor *_lockedEditor = nullptr;
    HelpView *_helpWidget = nullptr;

    PrefsDialog *_prefsDialog = nullptr;
    FindDialog *_findDialog = nullptr;
    FindInFilesDialog *_findInFilesDialog = nullptr;

    QMenu *_projectPopupMenu = nullptr;
    QMenu *_dirPopupMenu = nullptr;
    QMenu *_filePopupMenu = nullptr;

    QMenu *_editorPopupMenu = nullptr;

    QLabel *_statusWidget = nullptr;

    QComboBox *_targetsWidget = nullptr;
    QComboBox *_configsWidget = nullptr;
    QComboBox *_indexWidget = nullptr;

    QString _helpTopic;
    int _helpTopicId;

    bool _rebuildingHelp;

    QString _buildFileType;

    QString _activeCerberusTarget;
    QString _activeMonkey2Target;

    QVector<QString> _cerberusTargets;
    QVector<QString> _monkey2Targets;
    QStringList _completeList;

    QAction *actionToggle_Fullscreen = nullptr;
};

extern MainWindow *MAIN_WINDOW;
