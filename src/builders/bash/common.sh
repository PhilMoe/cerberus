#!/bin/bash

# COMMON FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILDER TOOL.

########################################
# COMMON FUNCTION USE BY OTHER SCRIPTS
########################################

# Various flags and variables
ROOT="$( cd -- "$(dirname "$SCRIPTPATH")" >/dev/null 2>&1 ; pwd -P )"   # Get the root directory of the Cerberus installation.
CERBERUS_ROOT_DIR="$ROOT"                                               # Set a named Cerberus variable to that of the ROOT variable. 
BIN="$ROOT/bin"
SRC="$ROOT/src"

EXITCODE=-1                 # Used to store the exit code after any call to functions and applications.
COMPILER=g++                # The file name of the compiler to use.
QT_INSTALLS=()              # Holds the total number of Qt kits installed
QTVER=                      # Holds the Qt version number
SHOW_MENU=0                 # Flag used to show the menu

# Set up additional variable based on the host.
[ $(uname -s) = "Linux" ] && {
    QTDIR=
    EXTENSION=
    HOST="linux"
    TARGET="gcc_$HOST"
    QMAKE_TYPE="gcc_64";
    GCC_VER="10"                            # For Linux: Set the default version of GCC if available.
} || {
    QTDIR="$HOME/Qt";    # Set the default Qt Installer directory location to the users home directory.
    EXTENSION=".app"
    TARGET="xcode"
    HOST="macos"
    
    # From Qt 6.2.4 the directory is no longer clang_64, but macOS. So extra checks will be needed.
    QMAKE_TYPE="clang_64";

    CERT=                                   # For macOS: Holds the developer certificate for use with xip or pkg files.
    MACOS_BUNDLE_PREFIX="com.cerberus-x"    # For macOS: Holds the default application bundle prefix.
}

#########################################
# Display colourised information
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
        _dir="$SRC/$1/$1.build";
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
    [ ! -f "$BIN/transcc_$HOST" ] && {
        do_error "NO TRANSCC PRESENT"
        EXITCODE=1
        return $EXITCODE;
    } || {
        local target=$2
        local srcpath="$SRC/$3"
        local srcfile="$3"
        [ -z "$4" ] && { gc_mode="0"; } || { gc_mode="1"; }

        ARGUMENTS=("$BIN/transcc_$HOST")
        ARGUMENTS+=("-target=$target")
        ARGUMENTS+=("-builddir=$srcfile.build")
        ARGUMENTS+=("-clean" "-config=release")
        ARGUMENTS+=("+CPP_GC_MODE=$gc_mode")
        if [ "$HOST" = "linux" ]; then
            if [ "$target" = "C++_Tool" ]; then
                ARGUMENTS+=("+CC_GCC_MSIZE=$MSIZE")
            elif [ "$target" = "Desktop_Game" ]; then
                ARGUMENTS+=("+GLFW_MSIZE_LINUX=$MSIZE")
            fi
        fi

        ARGUMENTS+=("$srcpath/$srcfile.cxs")

        execute "${ARGUMENTS[@]}"
        do_build_result
        
        return $EXITCODE;
    }
}
