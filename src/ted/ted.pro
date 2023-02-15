#-------------------------------------------------
#
# Project created by QtCreator 2012-04-28T10:44:05
#
#-------------------------------------------------
# Change log
# 2023-02-11 - Dawlane
#                   Ted Qt Project file rewritten to be better organized.
# 2023-02-10 - Dawlane
#                   Updated MacOS deployment target to 10.13.
#                   Added QMAKE_APPLE_DEVICE_ARCH if the Qt Kit is 6.2.1 or greater.
#                   Updated the macdeployqt options and clean up.
# 2023-02-03 - Dawlane
#                   Fixed path separators for windeployqt executable.
#                   Updated the QMAKE_TARGET_BUNDLE_PREFIX to com.whiteskygames    
# 2022-12-26 - Dawlane
#                   Remove deployment files every time a build or clean action is preformed.
# 2022-12-24 - DawLane
#                   Fixed issue with windeployqt and macdeployqt not working.
#                   NOTE: Will still need refining to remove unwanted directories and files.
# 2022-10-21 - Dawlane
#                   Removed Linux stuff. There should be no need to copy over specific Qt libraries.
#                   The end user must ensure that they have the required Qt runtimes installed.
# 2018-08-14 - Dawlane
#                   Modified Info.plist to except companyid from the command line.
# 2018-08-06 - Dawlane
#                   Removed 'dependencies without needing windeployqt and macdeployqt' as there are issues.
#                   Added code the runs windeployqt and macdeployqt after a compiler. Currently not implemented yet.
# 2018-07-31 - Dawlane
#                   Windows and MacOS now uses install to filter out required libraries.
#                   Added a way to copy over dependencies without needing windeployqt and macdeployqt. Qt5.9.2 tested only.
#                   Don't forget to add a build step to install if using Qt creator!
# 2018-07-23 - Dawlane
#                   Updated to use with Qt5.6+
#                   Linux should now no longer rely on the distributions repositories for Qt. Qt5.9.2 tested only.

QT += core gui webenginewidgets

TEMPLATE = app

SOURCES += main.cpp\
    application.cpp \
    applicationguard.cpp \
    helpview.cpp \
    highlighter.cpp \
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
    application.h \
    applicationguard.h \
    codeeditor.h \
    colorswatch.h \
    helpview.h \
    highlighter.h \
    macros.h \
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

# Additional files to show in QtCreator.
DISTFILES += \
    .clang-format \
    contributors.txt \
    formating.astylerc \
    appicon.rc

# Target and destination.
TARGET	= Ted
DESTDIR = $$absolute_path(../../bin)

# Set the locations of where the main Qt SDK directories are.
QT_SDK_PLUGINS_DIR   = $$[QT_INSTALL_DATA]/plugins
QT_SDK_PLATFORMS_DIR = $${QT_SDK_PLUGINS_DIR}/platforms

# Function to show a message only once.
defineTest(print) {
  !build_pass:message($$1)
}

# NOTE: Currently for Qt Deployment it's best to use the Linux repositories instead of
#       Trying to distribute Qt Libraries. Ted should work now from versions 5.9+
linux{

    # If the GCC compiler it version 6+, then disable Position Independent Code (aka pie)
    GCC += $$system( expr `gcc -dumpversion | cut -f1 -d.` )
    greaterThan(GCC, 5) {
        QMAKE_LFLAGS+= -no-pie
    }

    # Compiler and QMAKE settings Linux
    CONFIG += warn_off silent       # Suppress warnings and silence
    QMAKE_CXXFLAGS_RELEASE *= -O3   # Set code generation to the fastest

    # Output all values to make sure that they are correct
    print("==== Linux ====")
    print("$$escape_expand(\\t)QT SDK VERSION: $${QT_VERSION}")
    print("$$escape_expand(\\t)Config: $${CONFIG}")
    print("$$escape_expand(\\t)CFLAGS: $${QMAKE_CFLAGS}")
    print("$$escape_expand(\\t)CXXFLAGS: $${QMAKE_CXXFLAGS}")
    print("$$escape_expand(\\t)LFLAGS: $${QMAKE_LFLAGS}")
    print("$$escape_expand(\\t)LIBS: $${LIBS}")
}

# Deal with MS Windows settings
win32{

    # Compiler and QMAKE settings for MS Windows
    CONFIG += warn_off silent       # Suppress warnings and silence
    QMAKE_CXXFLAGS += -MP           # All config targets need this option for multi file processing
    QMAKE_CXXFLAGS_RELEASE *= /O2   # Set code generation to the fastest
    LIBS += -luser32                # MS Windows requires the user32 is linked against
    RC_FILE = appicon.rc

    # Deployment
    # To run external commands on MS Windows correctly. The paths have to be converted to MS DOS format.
    WINDEPLOYQT = $$dirname(QMAKE_QMAKE)/windeployqt.exe
    WINDEPLOYQT ~= s,/,\\,g S

    DESTDIR_PATH = $${DESTDIR}
    DESTDIR_PATH ~= s,/,\\,g

    QT_SDK_PLATFORMS_DIR ~= s,/,\\,g

    WINDEPLOYQT_TARGET = $$shell_quote($$shell_path($${DESTDIR}/$${TARGET}.exe))
    WINDEPLOYQT_TARGET ~= s,/,\\,g

    # Set up the commandline to use with windeployqt
    WINDEPLOYQT_OPTS = "$${WINDEPLOYQT}"  --no-svg -core -webenginecore -webenginewidgets -webchannel
    WINDEPLOYQT_OPTS += --no-translations --no-system-d3d-compiler --no-compiler-runtime
    WINDEPLOYQT_OPTS += --no-opengl-sw --no-plugins --no-serialport

    # Append additional options based on the Qt version.
    lessThan(QT_MAJOR_VERSION, 6) {
        lessThan(QT_MINOR_VERSION, 14) {
            WINDEPLOYQT_OPTS += --no-angle
        } else {
            WINDEPLOYQT_OPTS += --no-angle --no-virtualkeyboard
        }
    } else {
        WINDEPLOYQT_OPTS += --no-virtualkeyboard
    }

    # Set up the debug or release
    CONFIG_TYPE = ""

    CONFIG(debug, debug|release) {
        CONFIG_TYPE = "d"
        WINDEPLOYQT_OPTS += --debug
    } else {
        WINDEPLOYQT_OPTS += --release
    }

    WINDEPLOYQT_OPTS += "$${WINDEPLOYQT_TARGET}"

    # Execute the windeployqt command, create the platforms directory and copy over the qwindows plugin.
    QMAKE_POST_LINK +=  $${WINDEPLOYQT_OPTS} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK +=  $$QMAKE_MKDIR $$quote($$DESTDIR_PATH\\platforms) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += 	$$QMAKE_COPY $$quote($${QT_SDK_PLATFORMS_DIR}\\qwindows$${CONFIG_TYPE}.dll) \
                        $$quote($${DESTDIR_PATH}\\platforms) $$escape_expand(\\n\\t)

    # Output all values to make sure that they are correct
    print("==== MS WINDOWS ====")
    print("$$escape_expand(\\t)QT SDK VERSION: $${QT_VERSION}")
    print("$$escape_expand(\\t)windeployqt.exe location: $${WINDEPLOYQT}")
    print("$$escape_expand(\\t)Application destinaton path: $${DESTDIR_PATH}")
    print("$$escape_expand(\\t)Qt Platforms path: $${QT_SDK_PLATFORMS_DIR}")
    print("$$escape_expand(\\t)Full application path: $${WINDEPLOYQT_TARGET}")
    print("$$escape_expand(\\t)windeployqt options: $${WINDEPLOYQT_OPTS}$$escape_expand(\\n\\t)")
    print("$$escape_expand(\\t)Post link commands: $${QMAKE_POST_LINK}$$escape_expand(\\n\\t)")
    print("$$escape_expand(\\t)Config: $${CONFIG}")
    print("$$escape_expand(\\t)CFLAGS: $${QMAKE_CFLAGS}")
    print("$$escape_expand(\\t)CXXFLAGS: $${QMAKE_CXXFLAGS}")
    print("$$escape_expand(\\t)LFLAGS: $${QMAKE_LFLAGS}")
    print("$$escape_expand(\\t)LIBS: $${LIBS}")

    # Remove deployment files every time a build or clean action is started. MS Windows commands.
    system(rmdir /Q /S $$shell_quote($${DESTDIR_PATH}\\platforms))
    system(rmdir /Q /S $$shell_quote($${DESTDIR_PATH}\\resources))
    system(rmdir /Q /S $$shell_quote($${DESTDIR_PATH}\\translations))
    system(del /Q $$shell_quote($${DESTDIR_PATH}\\Qt*.dll))
    system(del /Q $$shell_quote($${DESTDIR_PATH}\\Qt*.exe))
    system(del /Q $$shell_quote($${DESTDIR_PATH}\\Ted.exe))
    system(del /Q $$shell_quote($${DESTDIR_PATH}\\Ted.pdb))
    system(del /Q $$shell_quote($${DESTDIR_PATH}\\Ted.ilk))
}

# Deal with Apple macOS settings
macx{

    # Compiler and QMAKE settings for Apple macOS
    CONFIG += warn_off silent			# Suppress warnings and silence
    QMAKE_CXXFLAGS_RELEASE *= -O3		# Set code generation to the fastest

    # The base deployment target for macOS depends on the version supported by Qt and the macOS SDK used.
    equals(QT_MAJOR_VERSION, 5) {
        equals(QT_MINOR_VERSION, 9) { QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.10 }
        greaterThan(QT_MINOR_VERSION, 9):lessThan(QT_MINOR_VERSION, 12) { QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11 }
        greaterThan(QT_MINOR_VERSION, 11):lessThan(QT_MINOR_VERSION, 14) { QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12 }
        greaterThan(QT_MINOR_VERSION, 13):lessThan(QT_MINOR_VERSION, 16) { QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.13 }
    }
    equals(QT_MAJOR_VERSION, 6) {
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
    }

    QMAKE_BUNDLE = Ted          # Application bundle name.

    # If no QMAKE_TARGET_BUNDLE_PREFIX was passed to qmake, then set a default.
    isEmpty($(QMAKE_TARGET_BUNDLE_PREFIX)){
        QMAKE_TARGET_BUNDLE_PREFIX = com.whiteskygames
    }

    QMAKE_INFO_PLIST = Info.plist		# Name of the property list to use.
    ICON = ted.icns				# The icon file to set for the bundle.

    # Create a universal app bundle
    QMAKE_APPLE_DEVICE_ARCHS = x86_64
    greaterThan(QT_MAJOR_VERSION, 4) {
        # According to the documentation. Qt 5.9 onwards supports x86_64h (haswell).
        # Doesn't show up in the dylibs
        #equals(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 8)|equals(QT_MAJOR_VERSION, 6) {
        #        QMAKE_APPLE_DEVICE_ARCHS += x86_64h
        #}

        # Qt 5.15 onwards, according to the documentation, supports apple silicon.
        # equals(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 14)|
        equals(QT_MAJOR_VERSION, 6) {
            QMAKE_APPLE_DEVICE_ARCHS += arm64
        }
    }

    # Set up some basic path to save typing out the full paths.
    MACDEPLOYQT = $$dirname(QMAKE_QMAKE)/macdeployqt
    MACDEPLOYQT_TARGET = $$shell_path($${DESTDIR}/$${TARGET}.app)

    APP_CONTENTS_DIR = $${MACDEPLOYQT_TARGET}/Contents
    APP_PLUGINS_DIR = $${APP_CONTENTS_DIR}/Plugins
    APP_PLUGINS_PLATFORM = $${APP_PLUGINS_DIR}/platforms

    # Set up the commandline to use with macdeployqt
    MACDEPLOYQT_OPTS = "$${MACDEPLOYQT}" "$${MACDEPLOYQT_TARGET}"
    MACDEPLOYQT_OPTS += -no-plugins

    # Execute the macdeployqt command, create the platforms directory and copy over the libqcoca plugin.
    QMAKE_POST_LINK +=  $${MACDEPLOYQT_OPTS} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK +=  $$QMAKE_MKDIR $$quote($${APP_PLUGINS_PLATFORM}) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK +=  $$QMAKE_COPY $$quote($${QT_SDK_PLATFORMS_DIR}/libqcocoa.dylib) \
                        $$quote($${APP_PLUGINS_PLATFORM}) $$escape_expand(\\n\\t)

    # Output all values to make sure that they are correct
    print("==== MacOS ====")
    print("$$escape_expand(\\t)QT SDK VERSION: $${QT_VERSION}")
    print("$$escape_expand(\\t)Architectures supported: $${QMAKE_APPLE_DEVICE_ARCHS}")
    print("$$escape_expand(\\t)macdeployqt location: $${MACDEPLOYQT}")
    print("$$escape_expand(\\t)Application destinaton path: $${DESTDIR}")
    print("$$escape_expand(\\t)Qt Platforms path: $${QT_SDK_PLATFORMS_DIR}")
    print("$$escape_expand(\\t)Full application path: $${MACDEPLOYQT_TARGET}")
    print("$$escape_expand(\\t)windeployqt options: $${MACDEPLOYQT_OPTS}$$escape_expand(\\n\\t)")
    print("$$escape_expand(\\t)Post link commands: $${QMAKE_POST_LINK}$$escape_expand(\\n\\t)")

    print("$$escape_expand(\\t)Config: $${CONFIG}")
    print("$$escape_expand(\\t)CFLAGS: $${QMAKE_CFLAGS}")
    print("$$escape_expand(\\t)CXXFLAGS: $${QMAKE_CXXFLAGS}")
    print("$$escape_expand(\\t)LFLAGS: $${QMAKE_LFLAGS}")
    print("$$escape_expand(\\t)LIBS: $${LIBS}")

    # Remove deployment files every time a build or clean action is started.
    system(rm -rf $$shell_quote($$shell_path($${MACDEPLOYQT_TARGET})))
}
