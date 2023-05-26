#!/bin/bash

# COMMON FUNCTIONS VERSION
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

# Get the root directory of the Cerberus installation and set variable for BIN and SRC directories.
ROOT="$( cd -- "$(dirname "$SCRIPTPATH")" >/dev/null 2>&1 ; pwd -P )"
CERBERUS_ROOT_DIR="$ROOT"
BIN="$ROOT/bin"
SRC="$ROOT/src"

# Set controling varialbes
EXITCODE=-1
COMPILER=g++
QT_INSTALLS=()
QTVER=
SHOW_MENU=0

# Set up additional variable based on the host.
[ $(uname -s) = "Linux" ] && {
    QTDIR=
    EXTENSION=
    HOST="linux"
    TARGET="gcc_$HOST"
    QMAKE_TYPE="gcc_64";
    GCC_VER="10"                            # For Linux: Set the default version of GCC is available.
} || {
    QTDIR="$HOME/Qt";    # Set the default Qt Installer directory location to the users home directory.
    EXTENSION=".app"
    TARGET="xcode/build"
    HOST="macos"
    
    # From Qt 6.2.4 the directory is no longer clang_64, but macOS. So extra checks will be needed.
    QMAKE_TYPE="clang_64";

    CERT=                                   # For macOS: Holds the developer certiciate for use wikt xip or pkg files.
    MACOS_BUNDLE_PREFIX="com.cerberus-x"    # For macOS: Holds the default applcation bundle prefix.
}

# General coloured information output.
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

# Function to execute external applications.
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

# Function to clean .build directories.
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

# Function to execute transcc
transcc(){
    [ ! -f "$BIN/transcc_$HOST" ] && {
        do_error "NO TRANSCC PRESENT"
        EXITCODE=1
        return $EXITCODE;
        } || {
        local target=$2
        local srcpath="$SRC/$3"
        local srcfile="$3"
        [ -z "$4" ] && { mode="0"; } || { mode="1"; }
        
        execute "$BIN/transcc_$HOST -target=$target -builddir=$srcfile.build -clean -config=release +CPP_GC_MODE=$mode $srcpath/$srcfile.cxs"
        do_build_result
        
        return $EXITCODE;
    }
}
