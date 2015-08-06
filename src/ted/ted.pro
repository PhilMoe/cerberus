#-------------------------------------------------
#
# Project created by QtCreator 2012-04-28T10:44:05
#
#-------------------------------------------------

QT       += core gui webkit network widgets webkitwidgets

TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    codeeditor.cpp \
    colorswatch.cpp \
    projecttreemodel.cpp \
    std.cpp \
    debugtreemodel.cpp \
    finddialog.cpp \
    prefs.cpp \
    prefsdialog.cpp \
    process.cpp \
    findinfilesdialog.cpp

HEADERS  += mainwindow.h \
    codeeditor.h \
    colorswatch.h \
    projecttreemodel.h \
    std.h \
    debugtreemodel.h \
    finddialog.h \
    prefs.h \
    prefsdialog.h \
    process.h \
    findinfilesdialog.h

FORMS    += mainwindow.ui \
    finddialog.ui \
    prefsdialog.ui \
    findinfilesdialog.ui

RESOURCES += resources.qrc

TARGET = Ted
#OK, this seems to prevent latest Windows QtCreator from being able to run Ted (builds fine).
#Solved by using qtcreator-2.4.1
DESTDIR = ../../bin

win32{
        RC_FILE = appicon.rc
}

mac{
#        WTF..enabling this appears to *break* 10.6 compatibility!!!!!
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
        ICON = ted.icns
}
