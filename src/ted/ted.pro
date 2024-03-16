#-------------------------------------------------
#
# Project created by QtCreator 2012-04-28T10:44:05
#
#-------------------------------------------------
# Change log
# 2023-08-20 - Dawlane
#                   Changed to how information is displayed.
#                   Renaming of a few variables.
#                   Add iconengine and imageformats for Linux to fix issue with no, or incorrect icons theme
#                   in project browser.
# 2023-04-27 - Dawlane
#                   Fixed issue with platform pluging not being deployed on Linux builds.
# 2023-04-12 - Dawlane
#                   Updated macOS to take into account changes to Qt 6.5.0 minimum target.
#                   Reinstated Linux distribution of Qt dependencies if not built with the Linux repository.
#                   FIXED an issue with QMAKE_TARGET_BUNDLE_PREFIX not being overridden via command line.
#                   Updated WINDEPLOYQT to take into account changes to Qt 6.5.0.
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

# Function to show a message spaced only once.
defineTest(tabPrint) {
  !build_pass:message("$$escape_expand(\\t)$$1")
}

defineTest(output) {
    # Qt information
    print("Qt Information:")

    # Linux specific, else general qt.io installs
    contains( QMAKE_QMAKE, ^/usr/.*) {
        tabPrint("Qt Version: $${QT_VERSION} (local)")
    } else {
        tabPrint("Qt Version: $${QT_VERSION}")
    }
    tabPrint("Qt root directory: $$[QT_INSTALL_DATA]")
    tabPrint("Qt plugins directory: $$[QT_INSTALL_PLUGINS]")

    # Build information
    print("Build:")
    CONFIG(debug, debug|release) {
        tabPrint("Config: Debug")
    } else {
        tabPrint("Config: Release")
    }

    # Print the target output
    !isEmpty(DESTDIR_PATH) {
        tabPrint("Target output path: $${DESTDIR_PATH}")
    } else {
        tabPrint("Target output path: $${DESTDIR}")
    }
    !isEmpty(INSTALLS) { tabPrint("Installs: $${INSTALLS}") }
    !isEmpty(QMAKE_TARGET_BUNDLE_PREFIX) { tabPrint("Application bundle prefix: $${QMAKE_TARGET_BUNDLE_PREFIX}") }
    !isEmpty(QMAKE_APPLE_DEVICE_ARCHS) { tabPrint("Target CPU architectures: $${QMAKE_APPLE_DEVICE_ARCHS}") }

    # For windeployqt and macdeployqt
    !isEmpty(DEPLOYQT_APP) {
        print("Deployment Application Information:")
        tabPrint("Application: $${DEPLOYQT_APP}")
        tabPrint("Application options: $${DEPLOYQT_OPTS}")
        tabPrint("Application input: $${DEPLOYQT_TARGET}")
    }
    !isEmpty(QMAKE_POST_LINK) {
        print("Post link commands:$$escape_expand(\\n)$${QMAKE_POST_LINK}")
    }
}

# NOTE: Currently for Qt Deployment it's best to use the Linux repositories instead of
#       Trying to distribute Qt Libraries. If a version other than the distributions default is chosen, then
#       try to set things up for that Qt kit.
linux{

    # If the GCC compiler is version 6+, then disable Position Independent Code (aka pie)
    GCC += $$system( expr `gcc -dumpversion | cut -f1 -d.` )
    greaterThan(GCC, 5) {
        QMAKE_LFLAGS+= -no-pie
    }

    # Compiler and QMAKE settings Linux
    CONFIG += warn_off silent       # Suppress warnings and silence
    QMAKE_CXXFLAGS_RELEASE *= -O3   # Set code generation to the fastest

    # Check if qmake is a local or installer.
    # Do this by checking if QMAKE_QMAKE contains /usr/
    !contains( QMAKE_QMAKE, ^/usr/.*) {
        QMAKE_STRIP = echo

        # Make a linux application search for libraries here.
        QMAKE_RPATHDIR = $ORIGIN/lib

        # Copy over all the required libraries
        # Common
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libicudata.so.56
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libicui18n.so.56
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libicuuc.so.56

        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}Core.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}DBus.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}Gui.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}Network.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}Positioning.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}PrintSupport.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}Qml.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}Quick.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}QuickWidgets.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}WebChannel.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}WebEngineCore.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}WebEngineWidgets.so.$${QT_MAJOR_VERSION}
        QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt$${QT_MAJOR_VERSION}Widgets.so.$${QT_MAJOR_VERSION}

        # Plugins
        platforms.files += $$[QT_INSTALL_PLUGINS]/platforms/libqxcb.so
        platforms.path += $(DESTDIR)/plugins/platforms

        xcbglintegrations.files += $$[QT_INSTALL_PLUGINS]/xcbglintegrations/libqxcb-glx-integration.so
        xcbglintegrations.path += $(DESTDIR)/plugins/xcbglintegrations

        INSTALLS += platforms xcbglintegrations

        # Version Specific
        equals(QT_MAJOR_VERSION, 5) {
            QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt5XcbQpa.so.5
            greaterThan(QT_MINOR_VERSION, 13) {
                QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt5Svg.so.5
                iconengines.files += $$[QT_INSTALL_PLUGINS]/iconengines/libqsvgicon.so
                iconengines.path += $(DESTDIR)/plugins/iconengines

                imageformats.files += $$[QT_INSTALL_PLUGINS]/imageformats/lib*.so
                imageformats.path += $(DESTDIR)/plugins/imageformats
                INSTALLS += iconengines imageformats
            }
        }

        equals(QT_MAJOR_VERSION, 6) {
            QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt6OpenGL.so.6
            QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt6QmlModels.so.6
            QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt6XcbQpa.so.6

            QTLIBS += $$[QT_INSTALL_DATA]/lib/libQt6Svg.so.6

            iconengines.files += $$[QT_INSTALL_PLUGINS]/iconengines/libqsvgicon.so
            iconengines.path += $(DESTDIR)/plugins/iconengines

            imageformats.files += $$[QT_INSTALL_PLUGINS]/imageformats/lib*.so
            imageformats.path += $(DESTDIR)/plugins/imageformats
            INSTALLS += iconengines imageformats
        }

        libs.files += $$QTLIBS
        libs.path += $(DESTDIR)/lib

        # translations. Don't need the qml stuff
        translations.files += $$[QT_INSTALL_TRANSLATIONS]/qtwebengine_locales/*.pak
        translations.path += $(DESTDIR)/translations/qtwebengine_locales

        # resources
        resources.files += $$[QT_INSTALL_DATA]/resources/*
        resources.path += $(DESTDIR)/resources

        # WebEngineProcess
        webengineprocess.files += $$[QT_INSTALL_DATA]/libexec/QtWebEngineProcess
        webengineprocess.path += $(DESTDIR)/libexec

        ted_qcfg.files += $(_PRO_FILE_PWD_)/configs/linux/bin/qt.conf
        ted_qcfg.path += $(DESTDIR)

        libexec_qcfg.files += $(_PRO_FILE_PWD_)/configs/linux/libexec/qt.conf
        libexec_qcfg.path += $(DESTDIR)/libexec

        INSTALLS += libs translations resources webengineprocess ted_qcfg libexec_qcfg
        install:   $(INSTALLS)
    }

    # Output all values to make sure that they are correct
    print("==== Linux ====")
    output()

    # Remove deployment files every time a build or clean action is started. Linux commands.
    system(rm -rf $$shell_quote($${DESTDIR}/plugins))
    system(rm -rf $$shell_quote($${DESTDIR}/resources))
    system(rm -rf /S $$shell_quote($${DESTDIR}/translations))
    system(rm -rf /S $$shell_quote($${DESTDIR}/lib))
    system(rm -rf /S $$shell_quote($${DESTDIR}/libexec))
    system(rm -f $$shell_quote($${DESTDIR}/Ted))
    system(rm -f $$shell_quote($${DESTDIR}/qt.conf))
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
    DEPLOYQT_APP = $$dirname(QMAKE_QMAKE)/windeployqt.exe
    DEPLOYQT_APP ~= s,/,\\,g S

    DESTDIR_PATH = $${DESTDIR}
    DESTDIR_PATH ~= s,/,\\,g

    QT_SDK_PLATFORMS_DIR ~= s,/,\\,g

    DEPLOYQT_TARGET = $$shell_quote($$shell_path($${DESTDIR}/$${TARGET}.exe))
    DEPLOYQT_TARGET ~= s,/,\\,g

    # Set up the commandline to use with windeployqt
    DEPLOYQT_OPTS =  --no-svg -core -webenginecore -webenginewidgets -webchannel
    DEPLOYQT_OPTS += --no-translations --no-system-d3d-compiler --no-compiler-runtime
    DEPLOYQT_OPTS += --no-opengl-sw --no-plugins

    # Append additional options based on the Qt version.
    lessThan(QT_MAJOR_VERSION, 6) {
        DEPLOYQT_OPTS+= --no-serialport
        lessThan(QT_MINOR_VERSION, 14) {
            DEPLOYQT_OPTS += --no-angle
        } else {
            DEPLOYQT_OPTS += --no-angle --no-virtualkeyboard
        }
    } else {
        lessThan(QT_MINOR_VERSION, 5) {
            DEPLOYQT_OPTS += --no-serialport
        }
        DEPLOYQT_OPTS += --no-virtualkeyboard
    }

    # Set up the debug or release
    CONFIG_TYPE = ""

    CONFIG(debug, debug|release) {
        CONFIG_TYPE = "d"
        DEPLOYQT_OPTS += --debug
    } else {
        DEPLOYQT_OPTS += --release
    }

    DEPLOY_CMDLINE = "$${DEPLOYQT_APP}" $${DEPLOYQT_OPTS} "$${DEPLOYQT_TARGET}"

    # Execute the windeployqt command, create the platforms directory and copy over the qwindows plugin.
    QMAKE_POST_LINK +=  $${DEPLOY_CMDLINE} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK +=  $$QMAKE_MKDIR $$quote($$DESTDIR_PATH\\platforms) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += 	$$QMAKE_COPY $$quote($${QT_SDK_PLATFORMS_DIR}\\qwindows$${CONFIG_TYPE}.dll) \
                        $$quote($${DESTDIR_PATH}\\platforms) $$escape_expand(\\n\\t)

    # Output all values to make sure that they are correct
    print("==== MS WINDOWS ====")
    output()

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
        lessThan(QT_MINOR_VERSION, 5) { QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14 }
        equals(QT_MINOR_VERSION, 5) { QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.00 }
    }

    QMAKE_BUNDLE = Ted          # Application bundle name.

    # If no QMAKE_TARGET_BUNDLE_PREFIX was passed to qmake, then set a default.
    isEmpty(QMAKE_TARGET_BUNDLE_PREFIX){
        QMAKE_TARGET_BUNDLE_PREFIX = com.whiteskygames
    }

    QMAKE_INFO_PLIST = Info.plist		# Name of the property list to use.
    ICON = ted.icns				# The icon file to set for the bundle.

    # Create a universal app bundle
    QMAKE_APPLE_DEVICE_ARCHS = x86_64
    greaterThan(QT_MAJOR_VERSION, 5) {
        QMAKE_APPLE_DEVICE_ARCHS += arm64
    }

    # Set up some basic path to save typing out the full paths.
    DEPLOYQT_APP = $$dirname(QMAKE_QMAKE)/macdeployqt
    DEPLOYQT_TARGET = $$shell_path($${DESTDIR}/$${TARGET}.app)

    APP_CONTENTS_DIR = $${DEPLOYQT_TARGET}/Contents
    APP_PLUGINS_DIR = $${APP_CONTENTS_DIR}/Plugins
    APP_PLUGINS_PLATFORM = $${APP_PLUGINS_DIR}/platforms

    # Set up the commandline to use with macdeployqt
    DEPLOYQT_OPTS += -no-plugins

    DEPLOY_CMDLINE = "$${DEPLOYQT_APP}" "$${DEPLOYQT_TARGET}" $${DEPLOYQT_OPTS}

    # Execute the macdeployqt command, create the platforms directory and copy over the libqcoca plugin.
    QMAKE_POST_LINK +=  $${DEPLOY_CMDLINE} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK +=  $$QMAKE_MKDIR $$quote($${APP_PLUGINS_PLATFORM}) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK +=  $$QMAKE_COPY $$quote($${QT_SDK_PLATFORMS_DIR}/libqcocoa.dylib) \
                        $$quote($${APP_PLUGINS_PLATFORM}) $$escape_expand(\\n\\t)

    # Output all values to make sure that they are correct
    print("==== MacOS ====")
    output()

    # Remove deployment files every time a build or clean action is started.
    system(rm -rf $$shell_quote($$shell_path($${DEPLOYQT_TARGET})))
}
