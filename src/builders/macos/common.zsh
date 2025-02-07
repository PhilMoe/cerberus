#!/bin/zsh

# COMMON FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILDER TOOL.

########################################
# COMMON FUNCTION USE BY OTHER SCRIPTS
########################################

# Various flags and variables
EXITCODE=-1                 # Used to store the exit code after any call to functions and applications.
COMPILER=g++                # The file name of the compiler to use.
QT_INSTALLS=()              # Holds the total number of Qt kits installed
QTVER=                      # Holds the Qt version number
SHOW_MENU=0                 # Flag used to show the menu
TRANSCC_EXE=0               # The flag use to check if transcc_macos has been built.
FORCE_CODESIGN_TED=0        # The flag to trigger a local code signing of Ted
DEPLOY_PATH=                # Holds the path that will be use to create the deployment build.

QTDIR="$HOME/Qt";           # Set the default Qt Installer directory location to the users home directory.
TARGET_COMPILER="xcode/build"
HOST="macos"                # Set the operating system type.

# From Qt 6.2.4 the directory is no longer clang_64, but macOS. So extra checks will be needed.
QMAKE_TYPE="clang_64";

CODESIGN_CERT=                          # Holds the path to the developer certificate. Empty will force codesign -s -
MACOS_BUNDLE_PREFIX="com.cerberus-x"    # Holds the default application bundle prefix.
QT_SELECTED=                            # Holds the selected/found Qt SDK.
DEPLOY_PATH=                            # Deployment path. See deploy.zsh
ARCHIVE_TOOL=hdiutil                    # Default archive tool to use.

#########################################
# Display colourized information
#########################################
do_info(){
    echo -e "\033[36m$1\033[0m"
}

do_header(){
    echo -e "\033[33m$1\033[0m"
}

do_build(){
    echo -e "\033[34m$1\033[0m"
}

do_error(){
    echo -e "\033[31m$1\033[0m"
}

do_success(){
    echo -e "\033[32m$1\033[0m"
}

do_unknown(){
    echo -e "\033[35m$1\033[0m"
}

###################################################
# General external application execution function.
###################################################
execute(){
    PARAM=
    for exec_param in $@; do
        PARAM+="$exec_param "
    done
    
    do_build "Executing:\n$PARAM"
    $@
    [[ $? -eq 0 ]] && {
        EXITCODE=0
        return $EXITCODE;
    } || {
        EXITCODE=1
        return $EXITCODE;
    }
}

###############################################
# Function to clean up after transcc builds.
###############################################
# Passing anything as the second parameter will allow a non .build directory to be deleted.
clean_build(){
    [ -z "$2" ] && {
        _dir="$1";
    } || {
        _dir="$CERBERUS_SRC_DIR/$1/$1.build";
    }

    [ -d "$_dir" ] && {
        echo "REMOVING BUILD $_dir";
        rm -rf "$_dir";
    }
}

# General function to call after a build
do_build_result(){
    [ $EXITCODE -eq 0 ] && {
        do_success "BUILD SUCCESSFUL"
        echo "";
        } || {
        do_error "BUILD FAILED"
        echo "";
    }
}

########################################
# Function to build with transcc
########################################
# The last parameter sets the garbage collection mode to use. The default is to use gc mode 1.
# See the Cerberus config documentation about garbage collection.
transcc(){
    EXITCODE=0
    [ ! -f "$CERBERUS_BIN_DIR/transcc_macos" ] && {
        do_error "NO TRANSCC PRESENT"
        EXITCODE=1
        return $EXITCODE;
    } || {
        local target=$2
        local srcpath="$CERBERUS_SRC_DIR/$3"
        local srcfile="$3"
        [ -z "$4" ] && { gc_mode="0"; } || { gc_mode="1"; }
        ARG=("$CERBERUS_BIN_DIR/transcc_macos" "-target=$target")
        ARG+=("-builddir=$srcfile.build" "-clean" "-config=release" "+CPP_GC_MODE=$gc_mode" "$srcpath/$srcfile.cxs")
        execute ${ARG[@]}
        do_build_result
        
        return $EXITCODE;
    }
}


########################################
#   LAST RESORT CODE SIGN LOCAL
########################################
# Force local use codesign
do_force_ted_codesign(){
    [ -z "$CODESIGN_CERT" ] && {
       array=( $(find "$CERBERUS_BIN_DIR/Ted.app") )
        local i=0;
        while [ $i -lt ${#array[@]} ]
        do
            execute codesign -s - ${array[$i]}
            ((i++))
        done;
    }
}