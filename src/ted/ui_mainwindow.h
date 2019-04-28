/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionOpen;
    QAction *actionSave;
    QAction *actionSave_All;
    QAction *actionClose;
    QAction *actionClose_All;
    QAction *actionFileQuit;
    QAction *actionBuildRun;
    QAction *actionNew;
    QAction *actionPrefs;
    QAction *actionSave_As;
    QAction *actionStep;
    QAction *actionKill;
    QAction *actionStep_In;
    QAction *actionStep_Out;
    QAction *actionEditFind;
    QAction *actionEditUndo;
    QAction *actionEditRedo;
    QAction *actionEditCut;
    QAction *actionEditCopy;
    QAction *actionEditPaste;
    QAction *actionEditDelete;
    QAction *actionEditSelectAll;
    QAction *actionEditGoto;
    QAction *actionHelpHome;
    QAction *actionHelpBack;
    QAction *actionHelpForward;
    QAction *actionViewFile;
    QAction *actionViewEdit;
    QAction *actionViewBuild;
    QAction *actionViewHelp;
    QAction *actionViewBrowser;
    QAction *actionViewConsole;
    QAction *actionAbout;
    QAction *actionBuildTarget;
    QAction *actionBuildConfig;
    QAction *actionOpenProject;
    QAction *actionDeleteFile;
    QAction *actionRenameFile;
    QAction *actionCloseProject;
    QAction *actionSetActiveProject;
    QAction *actionLock_Build_File;
    QAction *actionUnlock_Build_File;
    QAction *action_empty;
    QAction *actionFileNext;
    QAction *actionFilePrevious;
    QAction *actionEditFindNext;
    QAction *actionBuildBuild;
    QAction *actionEditFindInFiles;
    QAction *actionNewFile;
    QAction *actionNewFolder;
    QAction *actionOpen_on_Desktop;
    QAction *actionOpen_in_Help;
    QAction *actionHelpQuickHelp;
    QAction *actionBuildCheck;
    QAction *actionBuildUpdate;
    QAction *actionClose_Others;
    QAction *actionHelpRebuild;
    QAction *actionEditUn_Comment_block;
    QAction *actionNew_from_template;
    QAction *actionToggleBookmark;
    QAction *actionNextBookmark;
    QAction *actionPreviousBookmark;
    QAction *actionHelpCerberusHomepage;
    QAction *actionToggle_Fullscreen;
    QWidget *centralWidget;
    QGridLayout *gridLayout_3;
    QToolBar *fileToolBar;
    QStatusBar *statusBar;
    QToolBar *editToolBar;
    QToolBar *buildToolBar;
    QToolBar *helpToolBar;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuRecent_Files;
    QMenu *menuNewTemplate;
    QMenu *menuBuild;
    QMenu *menuEdit;
    QMenu *menuView;
    QMenu *menuToolbars;
    QMenu *menuWindows;
    QMenu *menuHelp;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(996, 783);
        QFont font;
        font.setStyleStrategy(QFont::PreferDefault);
        MainWindow->setFont(font);
        MainWindow->setContextMenuPolicy(Qt::DefaultContextMenu);
        MainWindow->setStyleSheet(QStringLiteral(""));
        MainWindow->setDocumentMode(false);
        MainWindow->setDockOptions(QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks|QMainWindow::AnimatedDocks);
        actionOpen = new QAction(MainWindow);
        actionOpen->setObjectName(QStringLiteral("actionOpen"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/Open.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpen->setIcon(icon);
        actionSave = new QAction(MainWindow);
        actionSave->setObjectName(QStringLiteral("actionSave"));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/Save.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave->setIcon(icon1);
        actionSave_All = new QAction(MainWindow);
        actionSave_All->setObjectName(QStringLiteral("actionSave_All"));
        actionClose = new QAction(MainWindow);
        actionClose->setObjectName(QStringLiteral("actionClose"));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/icons/Close.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionClose->setIcon(icon2);
        actionClose_All = new QAction(MainWindow);
        actionClose_All->setObjectName(QStringLiteral("actionClose_All"));
        actionFileQuit = new QAction(MainWindow);
        actionFileQuit->setObjectName(QStringLiteral("actionFileQuit"));
        actionBuildRun = new QAction(MainWindow);
        actionBuildRun->setObjectName(QStringLiteral("actionBuildRun"));
        QIcon icon3;
        icon3.addFile(QStringLiteral(":/icons/Build-Run.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionBuildRun->setIcon(icon3);
        actionNew = new QAction(MainWindow);
        actionNew->setObjectName(QStringLiteral("actionNew"));
        QIcon icon4;
        icon4.addFile(QStringLiteral(":/icons/New.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionNew->setIcon(icon4);
        actionPrefs = new QAction(MainWindow);
        actionPrefs->setObjectName(QStringLiteral("actionPrefs"));
        actionSave_As = new QAction(MainWindow);
        actionSave_As->setObjectName(QStringLiteral("actionSave_As"));
        actionStep = new QAction(MainWindow);
        actionStep->setObjectName(QStringLiteral("actionStep"));
        QIcon icon5;
        icon5.addFile(QStringLiteral(":/icons/Step.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStep->setIcon(icon5);
        actionKill = new QAction(MainWindow);
        actionKill->setObjectName(QStringLiteral("actionKill"));
        QIcon icon6;
        icon6.addFile(QStringLiteral(":/icons/Stop.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionKill->setIcon(icon6);
        actionStep_In = new QAction(MainWindow);
        actionStep_In->setObjectName(QStringLiteral("actionStep_In"));
        QIcon icon7;
        icon7.addFile(QStringLiteral(":/icons/Step-In.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStep_In->setIcon(icon7);
        actionStep_Out = new QAction(MainWindow);
        actionStep_Out->setObjectName(QStringLiteral("actionStep_Out"));
        QIcon icon8;
        icon8.addFile(QStringLiteral(":/icons/Step-Out.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStep_Out->setIcon(icon8);
        actionEditFind = new QAction(MainWindow);
        actionEditFind->setObjectName(QStringLiteral("actionEditFind"));
        QIcon icon9;
        icon9.addFile(QStringLiteral(":/icons/Find.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditFind->setIcon(icon9);
        actionEditUndo = new QAction(MainWindow);
        actionEditUndo->setObjectName(QStringLiteral("actionEditUndo"));
        actionEditRedo = new QAction(MainWindow);
        actionEditRedo->setObjectName(QStringLiteral("actionEditRedo"));
        actionEditCut = new QAction(MainWindow);
        actionEditCut->setObjectName(QStringLiteral("actionEditCut"));
        QIcon icon10;
        icon10.addFile(QStringLiteral(":/icons/Cut.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditCut->setIcon(icon10);
        actionEditCopy = new QAction(MainWindow);
        actionEditCopy->setObjectName(QStringLiteral("actionEditCopy"));
        QIcon icon11;
        icon11.addFile(QStringLiteral(":/icons/Copy.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditCopy->setIcon(icon11);
        actionEditPaste = new QAction(MainWindow);
        actionEditPaste->setObjectName(QStringLiteral("actionEditPaste"));
        QIcon icon12;
        icon12.addFile(QStringLiteral(":/icons/Paste.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditPaste->setIcon(icon12);
        actionEditDelete = new QAction(MainWindow);
        actionEditDelete->setObjectName(QStringLiteral("actionEditDelete"));
        actionEditSelectAll = new QAction(MainWindow);
        actionEditSelectAll->setObjectName(QStringLiteral("actionEditSelectAll"));
        actionEditGoto = new QAction(MainWindow);
        actionEditGoto->setObjectName(QStringLiteral("actionEditGoto"));
        actionHelpHome = new QAction(MainWindow);
        actionHelpHome->setObjectName(QStringLiteral("actionHelpHome"));
        QIcon icon13;
        icon13.addFile(QStringLiteral(":/icons/Home.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionHelpHome->setIcon(icon13);
        actionHelpBack = new QAction(MainWindow);
        actionHelpBack->setObjectName(QStringLiteral("actionHelpBack"));
        QIcon icon14;
        icon14.addFile(QStringLiteral(":/icons/Back.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionHelpBack->setIcon(icon14);
        actionHelpForward = new QAction(MainWindow);
        actionHelpForward->setObjectName(QStringLiteral("actionHelpForward"));
        QIcon icon15;
        icon15.addFile(QStringLiteral(":/icons/Forward.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionHelpForward->setIcon(icon15);
        actionViewFile = new QAction(MainWindow);
        actionViewFile->setObjectName(QStringLiteral("actionViewFile"));
        actionViewFile->setCheckable(true);
        actionViewEdit = new QAction(MainWindow);
        actionViewEdit->setObjectName(QStringLiteral("actionViewEdit"));
        actionViewEdit->setCheckable(true);
        actionViewBuild = new QAction(MainWindow);
        actionViewBuild->setObjectName(QStringLiteral("actionViewBuild"));
        actionViewBuild->setCheckable(true);
        actionViewHelp = new QAction(MainWindow);
        actionViewHelp->setObjectName(QStringLiteral("actionViewHelp"));
        actionViewHelp->setCheckable(true);
        actionViewBrowser = new QAction(MainWindow);
        actionViewBrowser->setObjectName(QStringLiteral("actionViewBrowser"));
        actionViewBrowser->setCheckable(true);
        actionViewConsole = new QAction(MainWindow);
        actionViewConsole->setObjectName(QStringLiteral("actionViewConsole"));
        actionViewConsole->setCheckable(true);
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionBuildTarget = new QAction(MainWindow);
        actionBuildTarget->setObjectName(QStringLiteral("actionBuildTarget"));
        actionBuildConfig = new QAction(MainWindow);
        actionBuildConfig->setObjectName(QStringLiteral("actionBuildConfig"));
        actionOpenProject = new QAction(MainWindow);
        actionOpenProject->setObjectName(QStringLiteral("actionOpenProject"));
        actionDeleteFile = new QAction(MainWindow);
        actionDeleteFile->setObjectName(QStringLiteral("actionDeleteFile"));
        actionRenameFile = new QAction(MainWindow);
        actionRenameFile->setObjectName(QStringLiteral("actionRenameFile"));
        actionCloseProject = new QAction(MainWindow);
        actionCloseProject->setObjectName(QStringLiteral("actionCloseProject"));
        actionSetActiveProject = new QAction(MainWindow);
        actionSetActiveProject->setObjectName(QStringLiteral("actionSetActiveProject"));
        actionLock_Build_File = new QAction(MainWindow);
        actionLock_Build_File->setObjectName(QStringLiteral("actionLock_Build_File"));
        actionUnlock_Build_File = new QAction(MainWindow);
        actionUnlock_Build_File->setObjectName(QStringLiteral("actionUnlock_Build_File"));
        action_empty = new QAction(MainWindow);
        action_empty->setObjectName(QStringLiteral("action_empty"));
        actionFileNext = new QAction(MainWindow);
        actionFileNext->setObjectName(QStringLiteral("actionFileNext"));
        actionFilePrevious = new QAction(MainWindow);
        actionFilePrevious->setObjectName(QStringLiteral("actionFilePrevious"));
        actionEditFindNext = new QAction(MainWindow);
        actionEditFindNext->setObjectName(QStringLiteral("actionEditFindNext"));
        actionBuildBuild = new QAction(MainWindow);
        actionBuildBuild->setObjectName(QStringLiteral("actionBuildBuild"));
        QIcon icon16;
        icon16.addFile(QStringLiteral(":/icons/Build.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionBuildBuild->setIcon(icon16);
        actionEditFindInFiles = new QAction(MainWindow);
        actionEditFindInFiles->setObjectName(QStringLiteral("actionEditFindInFiles"));
        actionNewFile = new QAction(MainWindow);
        actionNewFile->setObjectName(QStringLiteral("actionNewFile"));
        actionNewFolder = new QAction(MainWindow);
        actionNewFolder->setObjectName(QStringLiteral("actionNewFolder"));
        actionOpen_on_Desktop = new QAction(MainWindow);
        actionOpen_on_Desktop->setObjectName(QStringLiteral("actionOpen_on_Desktop"));
        actionOpen_in_Help = new QAction(MainWindow);
        actionOpen_in_Help->setObjectName(QStringLiteral("actionOpen_in_Help"));
        actionHelpQuickHelp = new QAction(MainWindow);
        actionHelpQuickHelp->setObjectName(QStringLiteral("actionHelpQuickHelp"));
        actionBuildCheck = new QAction(MainWindow);
        actionBuildCheck->setObjectName(QStringLiteral("actionBuildCheck"));
        actionBuildUpdate = new QAction(MainWindow);
        actionBuildUpdate->setObjectName(QStringLiteral("actionBuildUpdate"));
        actionClose_Others = new QAction(MainWindow);
        actionClose_Others->setObjectName(QStringLiteral("actionClose_Others"));
        actionHelpRebuild = new QAction(MainWindow);
        actionHelpRebuild->setObjectName(QStringLiteral("actionHelpRebuild"));
        actionEditUn_Comment_block = new QAction(MainWindow);
        actionEditUn_Comment_block->setObjectName(QStringLiteral("actionEditUn_Comment_block"));
        actionNew_from_template = new QAction(MainWindow);
        actionNew_from_template->setObjectName(QStringLiteral("actionNew_from_template"));
        actionToggleBookmark = new QAction(MainWindow);
        actionToggleBookmark->setObjectName(QStringLiteral("actionToggleBookmark"));
        actionNextBookmark = new QAction(MainWindow);
        actionNextBookmark->setObjectName(QStringLiteral("actionNextBookmark"));
        actionPreviousBookmark = new QAction(MainWindow);
        actionPreviousBookmark->setObjectName(QStringLiteral("actionPreviousBookmark"));
        actionHelpCerberusHomepage = new QAction(MainWindow);
        actionHelpCerberusHomepage->setObjectName(QStringLiteral("actionHelpCerberusHomepage"));
        actionToggle_Fullscreen = new QAction(MainWindow);
        actionToggle_Fullscreen->setObjectName(QStringLiteral("actionToggle_Fullscreen"));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        centralWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        gridLayout_3 = new QGridLayout(centralWidget);
        gridLayout_3->setSpacing(6);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        MainWindow->setCentralWidget(centralWidget);
        fileToolBar = new QToolBar(MainWindow);
        fileToolBar->setObjectName(QStringLiteral("fileToolBar"));
        fileToolBar->setContextMenuPolicy(Qt::DefaultContextMenu);
        fileToolBar->setIconSize(QSize(24, 24));
        MainWindow->addToolBar(Qt::TopToolBarArea, fileToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);
        editToolBar = new QToolBar(MainWindow);
        editToolBar->setObjectName(QStringLiteral("editToolBar"));
        editToolBar->setContextMenuPolicy(Qt::DefaultContextMenu);
        MainWindow->addToolBar(Qt::TopToolBarArea, editToolBar);
        buildToolBar = new QToolBar(MainWindow);
        buildToolBar->setObjectName(QStringLiteral("buildToolBar"));
        buildToolBar->setContextMenuPolicy(Qt::DefaultContextMenu);
        MainWindow->addToolBar(Qt::TopToolBarArea, buildToolBar);
        helpToolBar = new QToolBar(MainWindow);
        helpToolBar->setObjectName(QStringLiteral("helpToolBar"));
        helpToolBar->setContextMenuPolicy(Qt::DefaultContextMenu);
        MainWindow->addToolBar(Qt::TopToolBarArea, helpToolBar);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 996, 22));
        menuBar->setContextMenuPolicy(Qt::CustomContextMenu);
        menuBar->setStyleSheet(QStringLiteral(""));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        menuFile->setContextMenuPolicy(Qt::DefaultContextMenu);
        menuFile->setStyleSheet(QStringLiteral(""));
        menuRecent_Files = new QMenu(menuFile);
        menuRecent_Files->setObjectName(QStringLiteral("menuRecent_Files"));
        menuNewTemplate = new QMenu(menuFile);
        menuNewTemplate->setObjectName(QStringLiteral("menuNewTemplate"));
        menuNewTemplate->setTearOffEnabled(false);
        menuBuild = new QMenu(menuBar);
        menuBuild->setObjectName(QStringLiteral("menuBuild"));
        menuEdit = new QMenu(menuBar);
        menuEdit->setObjectName(QStringLiteral("menuEdit"));
        menuView = new QMenu(menuBar);
        menuView->setObjectName(QStringLiteral("menuView"));
        menuToolbars = new QMenu(menuView);
        menuToolbars->setObjectName(QStringLiteral("menuToolbars"));
        menuWindows = new QMenu(menuView);
        menuWindows->setObjectName(QStringLiteral("menuWindows"));
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName(QStringLiteral("menuHelp"));
        MainWindow->setMenuBar(menuBar);

        fileToolBar->addAction(actionNew);
        fileToolBar->addAction(actionOpen);
        fileToolBar->addAction(actionClose);
        fileToolBar->addAction(actionSave);
        editToolBar->addAction(actionEditCut);
        editToolBar->addAction(actionEditCopy);
        editToolBar->addAction(actionEditPaste);
        editToolBar->addAction(actionEditFind);
        buildToolBar->addAction(actionBuildBuild);
        buildToolBar->addAction(actionBuildRun);
        buildToolBar->addAction(actionStep);
        buildToolBar->addAction(actionStep_In);
        buildToolBar->addAction(actionStep_Out);
        buildToolBar->addAction(actionKill);
        helpToolBar->addAction(actionHelpHome);
        helpToolBar->addAction(actionHelpBack);
        helpToolBar->addAction(actionHelpForward);
        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuEdit->menuAction());
        menuBar->addAction(menuView->menuAction());
        menuBar->addAction(menuBuild->menuAction());
        menuBar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionNew);
        menuFile->addAction(menuNewTemplate->menuAction());
        menuFile->addAction(actionOpen);
        menuFile->addAction(menuRecent_Files->menuAction());
        menuFile->addSeparator();
        menuFile->addAction(actionClose);
        menuFile->addAction(actionClose_All);
        menuFile->addAction(actionClose_Others);
        menuFile->addSeparator();
        menuFile->addAction(actionSave);
        menuFile->addAction(actionSave_As);
        menuFile->addAction(actionSave_All);
        menuFile->addSeparator();
        menuFile->addAction(actionFileNext);
        menuFile->addAction(actionFilePrevious);
        menuFile->addSeparator();
        menuFile->addAction(actionPrefs);
        menuFile->addSeparator();
        menuFile->addAction(actionFileQuit);
        menuBuild->addAction(actionBuildBuild);
        menuBuild->addAction(actionBuildRun);
        menuBuild->addAction(actionBuildCheck);
        menuBuild->addAction(actionBuildUpdate);
        menuBuild->addSeparator();
        menuBuild->addAction(actionStep);
        menuBuild->addAction(actionStep_In);
        menuBuild->addAction(actionStep_Out);
        menuBuild->addAction(actionKill);
        menuBuild->addSeparator();
        menuBuild->addAction(actionBuildTarget);
        menuBuild->addAction(actionBuildConfig);
        menuBuild->addSeparator();
        menuBuild->addAction(actionLock_Build_File);
        menuBuild->addAction(actionUnlock_Build_File);
        menuBuild->addSeparator();
        menuBuild->addAction(actionOpenProject);
        menuEdit->addAction(actionEditUndo);
        menuEdit->addAction(actionEditRedo);
        menuEdit->addSeparator();
        menuEdit->addAction(actionEditCut);
        menuEdit->addAction(actionEditCopy);
        menuEdit->addAction(actionEditPaste);
        menuEdit->addAction(actionEditDelete);
        menuEdit->addSeparator();
        menuEdit->addAction(actionEditUn_Comment_block);
        menuEdit->addSeparator();
        menuEdit->addAction(actionEditSelectAll);
        menuEdit->addSeparator();
        menuEdit->addAction(actionEditFind);
        menuEdit->addAction(actionEditFindNext);
        menuEdit->addAction(actionEditGoto);
        menuEdit->addSeparator();
        menuEdit->addAction(actionEditFindInFiles);
        menuView->addAction(menuToolbars->menuAction());
        menuView->addAction(menuWindows->menuAction());
        menuView->addSeparator();
        menuView->addAction(actionToggleBookmark);
        menuView->addAction(actionNextBookmark);
        menuView->addAction(actionPreviousBookmark);
        menuView->addSeparator();
        menuView->addAction(actionToggle_Fullscreen);
        menuToolbars->addAction(actionViewFile);
        menuToolbars->addAction(actionViewEdit);
        menuToolbars->addAction(actionViewBuild);
        menuToolbars->addAction(actionViewHelp);
        menuWindows->addAction(actionViewBrowser);
        menuWindows->addAction(actionViewConsole);
        menuHelp->addAction(actionHelpHome);
        menuHelp->addAction(actionHelpBack);
        menuHelp->addAction(actionHelpForward);
        menuHelp->addSeparator();
        menuHelp->addAction(actionHelpQuickHelp);
        menuHelp->addSeparator();
        menuHelp->addAction(actionHelpRebuild);
        menuHelp->addSeparator();
        menuHelp->addAction(actionHelpCerberusHomepage);
        menuHelp->addAction(actionAbout);

        retranslateUi(MainWindow);
        QObject::connect(actionOpen, SIGNAL(triggered()), MainWindow, SLOT(onFileOpen()));
        QObject::connect(actionBuildRun, SIGNAL(triggered()), MainWindow, SLOT(onBuildRun()));
        QObject::connect(actionNew, SIGNAL(triggered()), MainWindow, SLOT(onFileNew()));
        QObject::connect(actionPrefs, SIGNAL(triggered()), MainWindow, SLOT(onFilePrefs()));
        QObject::connect(actionSave, SIGNAL(triggered()), MainWindow, SLOT(onFileSave()));
        QObject::connect(actionSave_As, SIGNAL(triggered()), MainWindow, SLOT(onFileSaveAs()));
        QObject::connect(actionSave_All, SIGNAL(triggered()), MainWindow, SLOT(onFileSaveAll()));
        QObject::connect(actionClose, SIGNAL(triggered()), MainWindow, SLOT(onFileClose()));
        QObject::connect(actionClose_All, SIGNAL(triggered()), MainWindow, SLOT(onFileCloseAll()));
        QObject::connect(actionClose_Others, SIGNAL(triggered()), MainWindow, SLOT(onFileCloseOthers()));
        QObject::connect(actionStep, SIGNAL(triggered()), MainWindow, SLOT(onDebugStep()));
        QObject::connect(actionKill, SIGNAL(triggered()), MainWindow, SLOT(onDebugKill()));
        QObject::connect(actionStep_In, SIGNAL(triggered()), MainWindow, SLOT(onDebugStepInto()));
        QObject::connect(actionStep_Out, SIGNAL(triggered()), MainWindow, SLOT(onDebugStepOut()));
        QObject::connect(actionEditFind, SIGNAL(triggered()), MainWindow, SLOT(onEditFind()));
        QObject::connect(actionEditGoto, SIGNAL(triggered()), MainWindow, SLOT(onEditGoto()));
        QObject::connect(actionViewBrowser, SIGNAL(triggered()), MainWindow, SLOT(onViewWindow()));
        QObject::connect(actionViewConsole, SIGNAL(triggered()), MainWindow, SLOT(onViewWindow()));
        QObject::connect(actionViewFile, SIGNAL(triggered()), MainWindow, SLOT(onViewToolBar()));
        QObject::connect(actionViewEdit, SIGNAL(triggered()), MainWindow, SLOT(onViewToolBar()));
        QObject::connect(actionViewBuild, SIGNAL(triggered()), MainWindow, SLOT(onViewToolBar()));
        QObject::connect(actionViewHelp, SIGNAL(triggered()), MainWindow, SLOT(onViewToolBar()));
        QObject::connect(actionHelpHome, SIGNAL(triggered()), MainWindow, SLOT(onHelpHome()));
        QObject::connect(actionHelpBack, SIGNAL(triggered()), MainWindow, SLOT(onHelpBack()));
        QObject::connect(actionHelpForward, SIGNAL(triggered()), MainWindow, SLOT(onHelpForward()));
        QObject::connect(actionBuildTarget, SIGNAL(triggered()), MainWindow, SLOT(onBuildTarget()));
        QObject::connect(actionBuildConfig, SIGNAL(triggered()), MainWindow, SLOT(onBuildConfig()));
        QObject::connect(actionOpenProject, SIGNAL(triggered()), MainWindow, SLOT(onBuildAddProject()));
        QObject::connect(actionLock_Build_File, SIGNAL(triggered()), MainWindow, SLOT(onBuildLockFile()));
        QObject::connect(actionUnlock_Build_File, SIGNAL(triggered()), MainWindow, SLOT(onBuildUnlockFile()));
        QObject::connect(actionEditCopy, SIGNAL(triggered()), MainWindow, SLOT(onEditCopy()));
        QObject::connect(actionEditCut, SIGNAL(triggered()), MainWindow, SLOT(onEditCut()));
        QObject::connect(actionEditDelete, SIGNAL(triggered()), MainWindow, SLOT(onEditDelete()));
        QObject::connect(actionEditPaste, SIGNAL(triggered()), MainWindow, SLOT(onEditPaste()));
        QObject::connect(actionEditRedo, SIGNAL(triggered()), MainWindow, SLOT(onEditRedo()));
        QObject::connect(actionEditSelectAll, SIGNAL(triggered()), MainWindow, SLOT(onEditSelectAll()));
        QObject::connect(actionEditUndo, SIGNAL(triggered()), MainWindow, SLOT(onEditUndo()));
        QObject::connect(actionAbout, SIGNAL(triggered()), MainWindow, SLOT(onHelpAbout()));
        QObject::connect(actionFileNext, SIGNAL(triggered()), MainWindow, SLOT(onFileNext()));
        QObject::connect(actionFilePrevious, SIGNAL(triggered()), MainWindow, SLOT(onFilePrevious()));
        QObject::connect(actionEditFindNext, SIGNAL(triggered()), MainWindow, SLOT(onEditFindNext()));
        QObject::connect(actionBuildBuild, SIGNAL(triggered()), MainWindow, SLOT(onBuildBuild()));
        QObject::connect(actionEditFindInFiles, SIGNAL(triggered()), MainWindow, SLOT(onEditFindInFiles()));
        QObject::connect(actionHelpQuickHelp, SIGNAL(triggered()), MainWindow, SLOT(onHelpQuickHelp()));
        QObject::connect(actionBuildCheck, SIGNAL(triggered()), MainWindow, SLOT(onBuildCheck()));
        QObject::connect(actionBuildUpdate, SIGNAL(triggered()), MainWindow, SLOT(onBuildUpdate()));
        QObject::connect(actionHelpRebuild, SIGNAL(triggered()), MainWindow, SLOT(onHelpRebuild()));
        QObject::connect(actionEditUn_Comment_block, SIGNAL(triggered()), MainWindow, SLOT(onEditCommentUncommentBlock()));
        QObject::connect(actionToggleBookmark, SIGNAL(triggered()), MainWindow, SLOT(onToggleBookmark()));
        QObject::connect(actionPreviousBookmark, SIGNAL(triggered()), MainWindow, SLOT(onPreviousBookmark()));
        QObject::connect(actionNextBookmark, SIGNAL(triggered()), MainWindow, SLOT(onNextBookmark()));
        QObject::connect(actionHelpCerberusHomepage, SIGNAL(triggered()), MainWindow, SLOT(onHelpCerberusHomepage()));
        QObject::connect(actionToggle_Fullscreen, SIGNAL(triggered()), MainWindow, SLOT(onToggleFullscreen()));

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "Big Ted", Q_NULLPTR));
        actionOpen->setText(QApplication::translate("MainWindow", "Open...", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionOpen->setShortcut(QApplication::translate("MainWindow", "Ctrl+O", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionSave->setText(QApplication::translate("MainWindow", "Save", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionSave->setShortcut(QApplication::translate("MainWindow", "Ctrl+S", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionSave_All->setText(QApplication::translate("MainWindow", "Save All", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionSave_All->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+S", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionClose->setText(QApplication::translate("MainWindow", "Close", Q_NULLPTR));
        actionClose_All->setText(QApplication::translate("MainWindow", "Close All", Q_NULLPTR));
        actionFileQuit->setText(QApplication::translate("MainWindow", "Quit", Q_NULLPTR));
        actionBuildRun->setText(QApplication::translate("MainWindow", "Build and Run", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionBuildRun->setToolTip(QApplication::translate("MainWindow", "Build/Run", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_SHORTCUT
        actionBuildRun->setShortcut(QApplication::translate("MainWindow", "F5", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionNew->setText(QApplication::translate("MainWindow", "New...", Q_NULLPTR));
        actionPrefs->setText(QApplication::translate("MainWindow", "Options...", Q_NULLPTR));
        actionSave_As->setText(QApplication::translate("MainWindow", "Save As...", Q_NULLPTR));
        actionStep->setText(QApplication::translate("MainWindow", "Step", Q_NULLPTR));
        actionKill->setText(QApplication::translate("MainWindow", "Kill", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionKill->setShortcut(QApplication::translate("MainWindow", "Shift+F5", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionStep_In->setText(QApplication::translate("MainWindow", "Step In", Q_NULLPTR));
        actionStep_Out->setText(QApplication::translate("MainWindow", "Step out", Q_NULLPTR));
        actionEditFind->setText(QApplication::translate("MainWindow", "Find...", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditFind->setShortcut(QApplication::translate("MainWindow", "Ctrl+F", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionEditUndo->setText(QApplication::translate("MainWindow", "Undo", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditUndo->setShortcut(QApplication::translate("MainWindow", "Ctrl+Z", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionEditRedo->setText(QApplication::translate("MainWindow", "Redo", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditRedo->setShortcut(QApplication::translate("MainWindow", "Ctrl+Y", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionEditCut->setText(QApplication::translate("MainWindow", "Cut", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditCut->setShortcut(QApplication::translate("MainWindow", "Ctrl+X", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionEditCopy->setText(QApplication::translate("MainWindow", "Copy", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditCopy->setShortcut(QApplication::translate("MainWindow", "Ctrl+C", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionEditPaste->setText(QApplication::translate("MainWindow", "Paste", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditPaste->setShortcut(QApplication::translate("MainWindow", "Ctrl+V", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionEditDelete->setText(QApplication::translate("MainWindow", "Delete", Q_NULLPTR));
        actionEditSelectAll->setText(QApplication::translate("MainWindow", "Select All", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditSelectAll->setShortcut(QApplication::translate("MainWindow", "Ctrl+A", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionEditGoto->setText(QApplication::translate("MainWindow", "Go to Line...", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditGoto->setShortcut(QApplication::translate("MainWindow", "Ctrl+G", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionHelpHome->setText(QApplication::translate("MainWindow", "Home", Q_NULLPTR));
        actionHelpBack->setText(QApplication::translate("MainWindow", "Back", Q_NULLPTR));
        actionHelpForward->setText(QApplication::translate("MainWindow", "Forward", Q_NULLPTR));
        actionViewFile->setText(QApplication::translate("MainWindow", "File", Q_NULLPTR));
        actionViewEdit->setText(QApplication::translate("MainWindow", "Edit", Q_NULLPTR));
        actionViewBuild->setText(QApplication::translate("MainWindow", "Build", Q_NULLPTR));
        actionViewHelp->setText(QApplication::translate("MainWindow", "Help", Q_NULLPTR));
        actionViewBrowser->setText(QApplication::translate("MainWindow", "Browser", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionViewBrowser->setShortcut(QApplication::translate("MainWindow", "Shift+Esc", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionViewConsole->setText(QApplication::translate("MainWindow", "Console", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionViewConsole->setShortcut(QApplication::translate("MainWindow", "Esc", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionAbout->setText(QApplication::translate("MainWindow", "About...", Q_NULLPTR));
        actionBuildTarget->setText(QApplication::translate("MainWindow", "Build Target...", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionBuildTarget->setToolTip(QApplication::translate("MainWindow", "Build Target", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        actionBuildConfig->setText(QApplication::translate("MainWindow", "Build Config...", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionBuildConfig->setToolTip(QApplication::translate("MainWindow", "Build Config", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        actionOpenProject->setText(QApplication::translate("MainWindow", "Open project...", Q_NULLPTR));
        actionDeleteFile->setText(QApplication::translate("MainWindow", "Delete...", Q_NULLPTR));
        actionRenameFile->setText(QApplication::translate("MainWindow", "Rename...", Q_NULLPTR));
        actionCloseProject->setText(QApplication::translate("MainWindow", "Close Project", Q_NULLPTR));
        actionSetActiveProject->setText(QApplication::translate("MainWindow", "Set as Active Project", Q_NULLPTR));
        actionLock_Build_File->setText(QApplication::translate("MainWindow", "Lock Build File", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionLock_Build_File->setShortcut(QApplication::translate("MainWindow", "Ctrl+L", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionUnlock_Build_File->setText(QApplication::translate("MainWindow", "Unlock Build File", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionUnlock_Build_File->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+L", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        action_empty->setText(QApplication::translate("MainWindow", "<empty>", Q_NULLPTR));
        actionFileNext->setText(QApplication::translate("MainWindow", "Next File", Q_NULLPTR));
        actionFilePrevious->setText(QApplication::translate("MainWindow", "Previous File", Q_NULLPTR));
        actionEditFindNext->setText(QApplication::translate("MainWindow", "Find Next", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditFindNext->setShortcut(QApplication::translate("MainWindow", "F3", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionBuildBuild->setText(QApplication::translate("MainWindow", "Build", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionBuildBuild->setShortcut(QApplication::translate("MainWindow", "F7", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionEditFindInFiles->setText(QApplication::translate("MainWindow", "Find in files...", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionEditFindInFiles->setToolTip(QApplication::translate("MainWindow", "Find in files", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        actionNewFile->setText(QApplication::translate("MainWindow", "New File...", Q_NULLPTR));
        actionNewFolder->setText(QApplication::translate("MainWindow", "New Folder...", Q_NULLPTR));
        actionOpen_on_Desktop->setText(QApplication::translate("MainWindow", "Open on Desktop", Q_NULLPTR));
        actionOpen_in_Help->setText(QApplication::translate("MainWindow", "Open in Help View", Q_NULLPTR));
        actionHelpQuickHelp->setText(QApplication::translate("MainWindow", "Quick Help", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionHelpQuickHelp->setShortcut(QApplication::translate("MainWindow", "F1", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionBuildCheck->setText(QApplication::translate("MainWindow", "Check Source", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionBuildCheck->setToolTip(QApplication::translate("MainWindow", "Check Source", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_SHORTCUT
        actionBuildCheck->setShortcut(QApplication::translate("MainWindow", "F6", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionBuildUpdate->setText(QApplication::translate("MainWindow", "Update Target Project", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionBuildUpdate->setToolTip(QApplication::translate("MainWindow", "Update Target Project", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_SHORTCUT
        actionBuildUpdate->setShortcut(QApplication::translate("MainWindow", "F8", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionClose_Others->setText(QApplication::translate("MainWindow", "Close Others", Q_NULLPTR));
        actionHelpRebuild->setText(QApplication::translate("MainWindow", "Rebuild Help", Q_NULLPTR));
        actionEditUn_Comment_block->setText(QApplication::translate("MainWindow", "Un/Comment block", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionEditUn_Comment_block->setShortcut(QApplication::translate("MainWindow", "Ctrl+E", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionNew_from_template->setText(QApplication::translate("MainWindow", "New from template", Q_NULLPTR));
        actionToggleBookmark->setText(QApplication::translate("MainWindow", "Toggle bookmark", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionToggleBookmark->setShortcut(QApplication::translate("MainWindow", "Ctrl+M", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionNextBookmark->setText(QApplication::translate("MainWindow", "Next bookmark", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionNextBookmark->setShortcut(QApplication::translate("MainWindow", "Ctrl+.", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionPreviousBookmark->setText(QApplication::translate("MainWindow", "Previous bookmark", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionPreviousBookmark->setShortcut(QApplication::translate("MainWindow", "Ctrl+,", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        actionHelpCerberusHomepage->setText(QApplication::translate("MainWindow", "Cerberus X Homepage", Q_NULLPTR));
        actionToggle_Fullscreen->setText(QApplication::translate("MainWindow", "Toggle Fullscreen", Q_NULLPTR));
#ifndef QT_NO_SHORTCUT
        actionToggle_Fullscreen->setShortcut(QApplication::translate("MainWindow", "F11", Q_NULLPTR));
#endif // QT_NO_SHORTCUT
        fileToolBar->setWindowTitle(QApplication::translate("MainWindow", "File", Q_NULLPTR));
        editToolBar->setWindowTitle(QApplication::translate("MainWindow", "Edit", Q_NULLPTR));
        buildToolBar->setWindowTitle(QApplication::translate("MainWindow", "Build", Q_NULLPTR));
        helpToolBar->setWindowTitle(QApplication::translate("MainWindow", "Help", Q_NULLPTR));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", Q_NULLPTR));
        menuRecent_Files->setTitle(QApplication::translate("MainWindow", "Recent Files...", Q_NULLPTR));
        menuNewTemplate->setTitle(QApplication::translate("MainWindow", "New from template", Q_NULLPTR));
        menuBuild->setTitle(QApplication::translate("MainWindow", "Build", Q_NULLPTR));
        menuEdit->setTitle(QApplication::translate("MainWindow", "Edit", Q_NULLPTR));
        menuView->setTitle(QApplication::translate("MainWindow", "View", Q_NULLPTR));
        menuToolbars->setTitle(QApplication::translate("MainWindow", "Toolbars", Q_NULLPTR));
        menuWindows->setTitle(QApplication::translate("MainWindow", "Windows", Q_NULLPTR));
        menuHelp->setTitle(QApplication::translate("MainWindow", "Help", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
