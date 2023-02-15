# TOOL BUILDER FUNCTIONS VERSION
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

# Function to build transcc
do_transcc(){
    EXITCODE=0
    do_info "BUILDING TRANSCC WITH $COMPILER"
    
    # Check for an exisiting transcc
    [ -f "$BIN/transcc_$HOST" ] && { rm -f "$BIN/transcc_$HOST"; }
    
    # Host specific parameters to pass the the C++ compiler
    [ $HOST = "linux" ] && {
        ARG=("-Os" "-s" "-lpthread");
        } || {
        ARG=("-Wno-bitwise-op-parentheses" "-Wno-logical-op-parentheses" "-mmacosx-version-min=10.9" "-std=gnu++0x" "-stdlib=libc++");
    }
    
    # Execute the C++ compiler
    execute $COMPILER -DNDEBUG -o "$ROOT/bin/transcc_$HOST" "$ROOT/src/transcc/transcc.build/cpptool/main.cpp" ${ARG[@]}
    do_build_result
    
    return $EXITCODE
}

# Function to build CServer
do_cserver(){
    EXITCODE=0
    
    # Call transcc
    transcc "CServer" "Desktop_Game" "cserver"
    
    # If transcc execution was successful; then update cerver.
    [ $EXITCODE -eq 0 ] && {
        # Clean out the olds and move new associated CServer files into the Cerberus bin directory.
        # If the host system is Linux; then add the data directory if one is not present.
        [ $HOST = "linux" ] && {
            [ ! -d "$BIN/data" ] && {
                mv "$SRC/cserver/cserver.build/glfw3/$TARGET/Release/data" "$BIN/data";
            }
            [ -f "$BIN/cserver_$HOST" ] && { rm -f "$BIN/cserver_$HOST"; };
            } || {
            [ -d "$BIN/cserver_$HOST$EXTENSION" ] && { rm -rf "$BIN/cserver_$HOST$EXTENSION"; };
        }
        
        # Move the newly built CServer into the Cerberus bin directory.
        mv "$SRC/cserver/cserver.build/glfw3/$TARGET/Release/CerberusGame$EXTENSION" "$BIN/cserver_$HOST$EXTENSION"
        
        # Clean up the .build directory.
        clean_build "cserver"
        return $EXITCODE;
    }
    
    # Clean up the .build directory.
    clean_build "cserver"
    return $EXITCODE
}

# Function to build the launcher
do_launcher(){
    EXITCODE=0
    
    # Use transcc to build the launcher on a Linux host
    [ $HOST = "linux" ] && {
        transcc "Launcher" "C++_Tool" "launcher"
        
        # Only update the launcher if the build was successful.
        [ $EXITCODE -eq 0 ] && {
            [ -f "$ROOT/Cerberus" ] && { rm -f "$ROOT/Cerberus"; }
            mv "$SRC/launcher/launcher.build/cpptool/main_$HOST" "$ROOT/Cerberus";
        }
        
        # Clean up the .build directory.
        clean_build "launcher";
        return $EXITCODE
        
        } || {
        
        # Execute xcodebuild
        execute xcodebuild "PRODUCT_BUNDLE_IDENTIFIER=com.whiteskygames.launcher" -scheme Cerberus -configuration release -project $SRC/launcher/xcode/Cerberus.xcodeproj -derivedDataPath $SRC/launcher/launcher.build
        
        # Only update the launcher if the build was successful.
        [ $EXITCODE -eq 0 ] && {
            [ -d "$ROOT/Cerberus$EXTENSION" ] && { rm -rf "$ROOT/Cerberus$EXTENSION"; }
            mv "$SRC/launcher/launcher.build/Build/Products/Release/Cerberus.app" "$ROOT/Cerberus.app";
            clean_build "launcher";
            } || {
            clean_build "launcher";
        };
        
        do_build_result;
    }
    
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
    clean_build "makedocs"
    return $EXITCODE
}

# Function to build the IDE Ted.
do_ted(){
    EXITCODE=0
    
    # As the qmake project expects there to be a directory with the name build-ted-Desktop-Release.
    # It's best to make sure any old version is removed and a new one created before running qmake
    [ -d "$SRC/build-ted-Desktop-Release" ] && { rm -rf "$SRC/build-ted-Desktop-Release"; }
    mkdir "$SRC/build-ted-Desktop-Release"
    cd "$SRC/build-ted-Desktop-Release"
    
    # Run qmake on the ted project file to create the makefile.
    execute qmake CONFIG+=release ../ted/ted.pro
    
    # If qmake was successfully executed; then the old version of Ted should be removed before building.
    # If not successful; then remove the build directory.
    [ $EXITCODE -eq 0 ] && {
        [ $HOST = "linux" ] && {
            [ -f "$BIN/Ted" ] && { rm -f "$BIN/Ted"; };
            } || {
            [ -d "$BIN/Ted$EXTENSION" ] && { rm -rf "$BIN/Ted$EXTENSION"; };
        }
        execute "make"
        do_build_result
        rm -rf "$SRC/build-ted-Desktop-Release";
        } || {
        rm -rf "$SRC/build-ted-Desktop-Release";
    }
    
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
    
    [ $HOST = "linux " ] && {
        do_info "Generating Free Desktop Launcher"
        do_freedesktop;
    }
    
    [[ ${#QT_INSTALLS[@]} -gt 0 && $EXITCODE -eq 0 ]] && {
        do_info "BUILDING IDE Ted"
        do_ted;
        } || {
        do_error "NO QT SDK KITS INSTALLED";
    };
}
