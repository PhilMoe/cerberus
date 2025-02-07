#!/bin/zsh

# COMMON FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILDER TOOL.

#######################################################################################################
#   Build transcc, cserver, makedocs, launcher and the IDE Ted
#######################################################################################################

# TRANSCC
# This function basically just builds from the c++ sources.
# Any new or update version of the transcc c++ source should replace the ones in the src/transcc/transcc.build directory. 
do_transcc(){
    EXITCODE=0
    do_info "BUILDING TRANSCC WITH $COMPILER"
    
    # Check for an existing transcc binary
    [ -f "$CERBERUS_BIN_DIR/transcc_macos" ] && { rm -f "$CERBERUS_BIN_DIR/transcc_macos"; }
    
    local PROJECT_DIR="$CERBERUS_SRC_DIR/transcc/transcc.build/cpptool"
    local DERIVED_DATA="$PROJECT_DIR/xcode/DerivedData"
    
    local ARG=("xcodebuild" "-scheme" "main_macos")
    ARG+=("-configuration" "Release" "-derivedDataPath" "$DERIVED_DATA")
    ARG+=("TARGET_NAME=transcc_macos" "TARGET_BUILD_DIR=$CERBERUS_BIN_DIR")
    
    pushd "$PROJECT_DIR/xcode"
    execute ${ARG[@]}
    popd
    clean_build "$DERIVED_DATA"

    do_build_result
}

# CSERVER TOOL
do_cserver(){
    EXITCODE=0
    
    # Call transcc
    transcc "CServer" "Desktop_Game" "cserver"
    
    local PROJECT_DIR="$CERBERUS_SRC_DIR/cserver/cserver.build/glfw3/$TARGET_COMPILER"

    # If transcc execution was successful; then update cerver.
    [ $EXITCODE -eq 0 ] && {
        # Clean out the olds and move new associated CServer files into the Cerberus bin directory.
        # If the host system is Linux; then add the data directory if one is not present.
        [ -d "$CERBERUS_BIN_DIR/cserver_macos.app" ] && { rm -rf "$CERBERUS_BIN_DIR/cserver_macos.app"; }
        
        # Move the newly built CServer into the Cerberus bin directory.
        mv "$PROJECT_DIR/Release/CerberusGame.app" "$CERBERUS_BIN_DIR/cserver_macos.app"
        
        # Clean up the .build directory.
        clean_build "cserver" "dotbuild"
        return $EXITCODE;
    }
    
    # Clean up the .build directory.
    clean_build "cserver" "dotbuild"
    return $EXITCODE
}

# LAUNCHER TOOL
do_launcher(){
    EXITCODE=0
    #xcodebuild -derivedDataPath /custom/path -IDECustomBuildProductsPath="" -IDECustomBuildIntermediatesPath="" ...
    [ -d "$CERBERUS_ROOT_DIR/Cerberus.app" ] && { rm -rf "$CERBERUS_ROOT_DIR/Cerberus.app"; }

    # Save and change directory to the xcode project
    local PROJECT_DIR="$CERBERUS_SRC_DIR/launcher/xcode/"
    local DERIVED_DATA="$PROJECT_DIR/DerivedData"

    pushd "$PROJECT_DIR"
    local ARG=("xcodebuild" "PRODUCT_BUNDLE_IDENTIFIER=$MACOS_BUNDLE_PREFIX.launcher")
    ARG+=("-scheme" "Cerberus" "-configuration" "Release")
    ARG+=("-derivedDataPath" "$DERIVED_DATA")
    ARG+=("PRODUCT_NAME=Cerberus")
    ARG+=("TARGET_BUILD_DIR=$CERBERUS_ROOT_DIR")

    # Execute xcodebuild
    execute ${ARG[@]}
    popd

    clean_build "$DERIVED_DATA"
    return $EXITCODE
}

# MAKEDOCS TOOL
do_makedocs(){
    EXITCODE=0
    
    # Call transcc to build makedocs.
    transcc "Makedocs" "C++_Tool" "makedocs"
    
    # Only update the makedocs if the build was successful.
    [ $EXITCODE -eq 0 ] && {
        [ -f "$CERBERUS_BIN_DIR/makedocs_macos" ] && { rm -f "$CERBERUS_BIN_DIR/makedocs_macos"; }
        mv "$CERBERUS_SRC_DIR/makedocs/makedocs.build/cpptool/main_macos" "$CERBERUS_BIN_DIR/makedocs_macos";
    }
    
    # Clean up the .build directory.
    clean_build "makedocs" "dotbuild"
    return $EXITCODE
}

# IDE TED
do_ted(){
    EXITCODE=0
    
    local PROJECT_DIR="$CERBERUS_SRC_DIR/build-ted-Desktop-Release"
    # As the qmake project expects there to be a directory with the name build-ted-Desktop-Release.
    # It's best to make sure any old version is removed and a new one created before running qmake
    [ -d "$PROJECT_DIR" ] && { rm -rf "$PROJECT_DIR"; }
    mkdir "$PROJECT_DIR"
    cd "$PROJECT_DIR"
    local MACOS_OPTS="QMAKE_TARGET_BUNDLE_PREFIX=$MACOS_BUNDLE_PREFIX"
    
    [ ! -z "$CODESIGN_CERT" ] && {
        CODESIGN=$CODESIGN_CERT
    }
    # Run qmake on the ted project file to create the makefile.
    execute qmake CONFIG+=release ../ted/ted.pro $MACOS_OPTS CODESIGN_CERT=$CODESIGN
    
    # If qmake was successfully executed; then proceed to build the IDE.
    [ $EXITCODE -eq 0 ] && {
        [ -d "$CERBERUS_BIN_DIR/Ted.app" ] && { rm -rf "$CERBERUS_BIN_DIR/Ted.app"; }
        execute "make"
        do_build_result;
    }

    # If there are still issues with Ted, then uncomment this to force local codesign
    [ $FORCE_CODESIGN_TED -eq 1 ] && { do_force_ted_codesign; }

    clean_build "$PROJECT_DIR"
    return $EXITCODE
}

# BUILD ALL
# Builds all the above tools.
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

################################################
# CLEAN THE WORK REPOSITORY OF BUILT FILES
################################################
do_clearbuilds(){
    do_info "CLEARING OUT PREVIOUS BUILDS"

    # Remove all macOS applications. Ted and CServer
    find "$CERBERUS_BIN_DIR" -type d -name '*.app' -exec rm -rf "{}" \;

    # Remove transcc linux, winnt and macos
    find "$CERBERUS_BIN_DIR" -type f -name 'transcc_*' -delete

    # Remove the launchers linux, winnt and macos
    find "$CERBERUS_ROOT_DIR" -type f -name 'Cerberus.exe' -delete
    find "$CERBERUS_ROOT_DIR" -type f -name 'Cerberus' -delete
    find "$CERBERUS_ROOT_DIR" -type d -name 'Cerberus.app' -exec rm -rf "{}" \;
    find "$CERBERUS_ROOT_DIR" -type f -name '*.desktop' -delete
  
    # Remove CServer linux and winnt
    find "$CERBERUS_BIN_DIR" -type f -name 'cserver_*' -delete

    # Remove makedocs linux, winnt and macos
    find "$CERBERUS_BIN_DIR" -type f -name 'makedocs_*' -delete

    # Remove Ted linux and winnt
    find "$CERBERUS_BIN_DIR" -type f -name 'Ted.exe' -delete
    find "$CERBERUS_BIN_DIR" -type f -name 'Ted' -delete

    # Remove Qt Linux support files and directories
    find "$CERBERUS_BIN_DIR" -type d -name 'lib*' -exec rm -rf "{}" \;
    find "$CERBERUS_BIN_DIR" -type d -name 'plugins' -exec rm -rf "{}" \;
    find "$CERBERUS_BIN_DIR" -type d -name 'resources' -exec rm -rf "{}" \;
    find "$CERBERUS_BIN_DIR" -type d -name 'translations' -exec rm -rf "{}" \;

    # Remove Qt WinNT support files and directories
    find "$CERBERUS_BIN_DIR" -type f -name 'qt.conf' -delete
    find "$CERBERUS_BIN_DIR" -type f -name '*.dll' -delete
    find "$CERBERUS_BIN_DIR" -type f -name '*.exe' -delete
    find "$CERBERUS_BIN_DIR" -type f -name '*.ilk' -delete
    find "$CERBERUS_BIN_DIR" -type f -name '*.pdb' -delete
    find "$CERBERUS_BIN_DIR" -type f -name 'openal32_*' -delete
    find "$CERBERUS_BIN_DIR" -type d -name 'platforms' -exec rm -rf "{}" \;

}
