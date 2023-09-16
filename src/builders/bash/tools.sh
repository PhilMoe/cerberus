#!/bin/bash

# TOOL BUILDER FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

# Function to build transcc
do_transcc(){
    EXITCODE=0
    do_info "BUILDING TRANSCC WITH $COMPILER"
    
    # Check for an exisiting transcc
    [ -f "$BIN/transcc_$HOST" ] && { rm -f "$BIN/transcc_$HOST"; }
    
    PROJECT_DIR="$SRC/transcc/transcc.build/cpptool"
    # Host specific parameters to pass the the C++ compiler
    [ $HOST = "linux" ] && {
        BUILD_DIR="$PROJECT_DIR/gcc_linux/Release"

        ARG=("make")
        ARG+=("CXX_COMPILER=$COMPILER" "C_COMPILER=$C_COMPILER" "CCOPTS=-DNDEBUG" "CCOPTS+=-Os")
        ARG+=("BUILD_DIR=$BUILD_DIR")
        ARG+=("OUT=transcc_linux" "OUT_PATH=$BIN" "LIBOPTS=-lpthread" "LIBOPTS+=-ldl" "LDOPTS=-no-pie" "LDOPTS+=-s");

        mkdir -p $BUILD_DIR
        cd $SRC/transcc/transcc.build/cpptool/gcc_linux
        execute ${ARG[@]}
        cd $SRC

        clean_build "$BUILD_DIR";
    } || {
        BUILD_DIR="$PROJECT_DIR/xcode/build"
        
        ARG=("xcodebuild" "-project" "$ROOT/src/transcc/transcc.build/cpptool/xcode/main_macos.xcodeproj")
    	ARG+=("-configuration" "Release")
    	ARG+=("CONFIGURATION_BUILD_DIR=$ROOT/bin")
    	ARG+=("TARGET_NAME=transcc_macos")
     
        cd "$SRC/transcc/transcc.build/cpptool/xcode"
        execute ${ARG[@]}
        cd "$SRC"
        clean_build "$BUILD_DIR";
    }
    
    do_build_result
    
    return $EXITCODE
}

# Function to build CServer
do_cserver(){
    EXITCODE=0
    
    # Call transcc
    transcc "CServer" "Desktop_Game" "cserver"
    
    PROJECT_DIR="$SRC/cserver/cserver.build/glfw3/$TARGET"

    # If transcc execution was successful; then update cerver.
    [ $EXITCODE -eq 0 ] && {
        # Clean out the olds and move new associated CServer files into the Cerberus bin directory.
        # If the host system is Linux; then add the data directory if one is not present.
        [ $HOST = "linux" ] && {
            [ ! -d "$BIN/data" ] && {
                mv "$PROJECT_DIR/Release/data" "$BIN/data";
            }
            [ -f "$BIN/cserver_$HOST" ] && { rm -f "$BIN/cserver_$HOST"; };
        } || {
            [ -d "$BIN/cserver_$HOST$EXTENSION" ] && { rm -rf "$BIN/cserver_$HOST$EXTENSION"; };
        }
        
        # Move the newly built CServer into the Cerberus bin directory.
        mv "$PROJECT_DIR/Release/CerberusGame$EXTENSION" "$BIN/cserver_$HOST$EXTENSION"
        
        # Clean up the .build directory.
        clean_build "cserver" "dotbuild"
        return $EXITCODE;
    }
    
    # Clean up the .build directory.
    clean_build "cserver" "dotbuild"
    return $EXITCODE
}

# Function to build the launcher
do_launcher(){
    EXITCODE=0
    
    PROJECT_DIR="$SRC/launcher/launcher.build"
    # Use transcc to build the launcher on a Linux host
    if [ "$HOST" = "linux" ]; then
        transcc "Launcher" "C++_Tool" "launcher"
        
        # Only update the launcher if the build was successful.
        [ $EXITCODE -eq 0 ] && {
            [ -f "$ROOT/Cerberus" ] && { rm -f "$ROOT/Cerberus"; }
            mv "$PROJECT_DIR/cpptool/main_$HOST" "$ROOT/Cerberus";
        };
    else 
        # Execute xcodebuild
        execute xcodebuild "PRODUCT_BUNDLE_IDENTIFIER=$MACOS_BUNDLE_PREFIX.launcher" -scheme Cerberus -configuration release -project $SRC/launcher/xcode/Cerberus.xcodeproj -derivedDataPath $SRC/launcher/launcher.build
        
        # Only update the launcher if the build was successful.
        [ $EXITCODE -eq 0 ] && {
            [ -d "$ROOT/Cerberus$EXTENSION" ] && { rm -rf "$ROOT/Cerberus$EXTENSION"; }
            mv "$PROJECT_DIR/Build/Products/Release/Cerberus.app" "$ROOT/Cerberus.app";
        };
    fi

    # Clean up the .build directory.
    clean_build "launcher" "dotbuild"

    return $EXITCODE
}

# Function to build makedocs
do_makedocs(){
    EXITCODE=0
    
    # Call transcc to build makedocs.
    transcc "Makedocs" "C++_Tool" "makedocs"
    
    # Only update the makedocs if the build was successful.
    [ $EXITCODE -eq 0 ] && {
        [ -f "$BIN/makedocs_$HOST" ] && { rm -f "$BIN/makedocs_$HOST"; }
        mv "$SRC/makedocs/makedocs.build/cpptool/main_$HOST" "$BIN/makedocs_$HOST";
    }
    
    # Clean up the .build directory.
    clean_build "makedocs" "dotbuild"
    return $EXITCODE
}

# Function to build the IDE Ted.
do_ted(){
    EXITCODE=0
    
    PROJECT_DIR="$SRC/build-ted-Desktop-Release"
    # As the qmake project expects there to be a directory with the name build-ted-Desktop-Release.
    # It's best to make sure any old version is removed and a new one created before running qmake
    [ -d "$PROJECT_DIR" ] && { rm -rf "$PROJECT_DIR"; }
    mkdir "$PROJECT_DIR"
    cd "$PROJECT_DIR"
    
    [ $HOST = "macos" ] && {
        MACOS_OPTS="QMAKE_TARGET_BUNDLE_PREFIX=$MACOS_BUNDLE_PREFIX";
    }
    
    # Run qmake on the ted project file to create the makefile.
    execute qmake CONFIG+=release ../ted/ted.pro $MACOS_OPTS
    
    # If qmake was successfully executed; then proceed to build the IDE.
    [ $EXITCODE -eq 0 ] && {
        # Not realy required with qmake cleaning out all Qt related binaries.
        [ $HOST = "linux" ] && {
            [ -f "$BIN/Ted" ] && { rm -f "$BIN/Ted"; };
        } || {
            [ -d "$BIN/Ted$EXTENSION" ] && { rm -rf "$BIN/Ted$EXTENSION"; };
        }

        # For Linux. Either 
        [ $HOST = "linux" ] && {
            if [[ $QT_SELECTED != *"/usr/"* ]]; then
                execute "make" "install"
            else
                execute "make"
            fi;
        } || {
            execute "make"
        }
        do_build_result;
    }
    
    clean_build "$PROJECT_DIR"
    return $EXITCODE
}

do_freedesktop(){
    do_init_linux_desktop
}

do_all(){
    do_header "\n====== BUILDING ALL TOOLS ======"
    do_info "BUILDING TransCC"
    do_transcc;
    [ $EXITCODE -eq 0 ] && {
        do_info "BUILDING CServer"
        do_cserver;
    }
    [ $EXITCODE -eq 0 ] && {
        do_info "BUILDING Makedocs"
        do_makedocs;
    }
    [ $EXITCODE -eq 0 ] && {
        do_info "BUILDING Launcher"
        do_launcher;
    }
    
    [ $HOST = "linux" ] && {
        do_info "Generating Free Desktop Launcher"
        do_freedesktop;
    }
    
    [[ ${#QT_INSTALLS[@]} -gt 0 && $EXITCODE -eq 0 ]] && { 
        do_info "BUILDING IDE Ted"
        do_ted;
    } || {
        do_error "NO QT SDK KITS INSTALLED";
    }

    [ -n "$DEPLY" ] && {
        do_deploy;
        return $EXITCODE
    }
}

# Remove all previously built files.
do_clearbuilds(){
    do_info "CLEARING OUT PREVIOUS BUILDS"

    # Remove all macOS applications. Ted and CServer
    find "$BIN" -type d -name '*.app' -exec rm -rf "{}" \;

    # Remove transcc linux, winnt and macos
    find "$BIN" -type f -name 'transcc_*' -delete

    # Remove the launchers linux, winnt and macos
    find "$ROOT" -type f -name 'Cerberus.exe' -delete
    find "$ROOT" -type f -name 'Cerberus' -delete
    find "$ROOT" -type d -name 'Cerberus.app' -exec rm -rf "{}" \;
    find "$ROOT" -type f -name '*.desktop' -delete
  
    # Remove CServer linux and winnt
    find "$BIN" -type f -name 'Cerberus.*' -delete

    # Remove makedocs linux, winnt and macos
    find "$BIN" -type f -name 'makedocs_*' -delete

    # Remove Ted linux and winnt
    find "$BIN" -type f -name 'Ted.*' -delete
    find "$BIN" -type f -name 'Ted' -delete

    # Remove Qt Linux support files and directories
    find "$BIN" -type d -name 'lib*' -exec rm -rf "{}" \;
    find "$BIN" -type d -name 'plugins' -exec rm -rf "{}" \;
    find "$BIN" -type d -name 'resources' -exec rm -rf "{}" \;
    find "$BIN" -type d -name 'translations' -exec rm -rf "{}" \;

    # Remove Qt WinNT support files and directories
    find "$BIN" -type f -name 'qt.conf' -delete
    find "$BIN" -type f -name '*.dll' -delete
    find "$BIN" -type f -name '*.exe' -delete
    find "$BIN" -type f -name '*.ilk' -delete
    find "$BIN" -type f -name '*.pdb' -delete
    find "$BIN" -type f -name 'openal32_*' -delete
    find "$BIN" -type d -name 'platforms' -exec rm -rf "{}" \;

}
