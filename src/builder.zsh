#!/bin/zsh
# MAC OS X ZSHELL SCRIPT FOR BUILDING CERBERUS X TOOLS
# THIS IS BASICALLY THE SAME AS THE BASH VERSION, BUT WITH THE LINUX STUFF REMOVED AND EXTRA STUFF TO DEAL WITH XCODE.

# Tried and trusted way to get the absolute path from any bourne type shell.
# $( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )
CERBERUS_ROOT_DIR="$( cd -- "$(dirname "$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )")" >/dev/null 2>&1 ; pwd -P )"
CERBERUS_BIN_DIR="$CERBERUS_ROOT_DIR/bin"
CERBERUS_SRC_DIR="$CERBERUS_ROOT_DIR/src"
SCRIPT_VER="2.0.0"      # Version 2. Because it's just about the same as the Linux version.

# Import the dependencies that this script relies on.
source "$CERBERUS_SRC_DIR/builders/macos/common.zsh"        # Common functions and variables.
source "$CERBERUS_SRC_DIR/builders/macos/thirdparty.zsh"    # Third-party functions. Used to get information from compilers and Qt SDKs.

source "$CERBERUS_SRC_DIR/builders/macos/deploy.zsh"        # Script to create a deployment archive.
source "$CERBERUS_SRC_DIR/builders/macos/tools.zsh"         # Functions to build Cerberus.

##################################
# COMMAND LINE ARGUMENT PROCESSING
##################################
POSITIONAL_ARGS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--prefix)
            MACOS_BUNDLE_PREFIX="$2"
            shift; shift
        ;;
        -c|--codesign)
            CODESIGN_CERT="$2"
            shift; shift
            ;;
        -t|--tedsign)
            FORCE_CODESIGN_TED=1
            shift
        ;;
        -a|--archiver)
            ARCHIVER="$2"
            shift; shift
        ;;
        --clearbuilds)
            do_header "===== Cerberus X Tool Builder Version $SCRIPT_VER ====="
            do_clearbuilds
            do_success "Cerberus X Builder script terminated."
            exit 0
        ;;
        -d|--deploy)
            DEPLOY_PATH="$2"
            shift; shift
        ;;
        -q|--qtdir)
            QTDIR="$2"
            shift; shift
        ;;
        -k|--qtkit)
            QTVER="$2"
            shift; shift
        ;;
        -m|--showmenu)
            SHOW_MENU=1
            shift
        ;;
        -h|--help)
            do_info "CERBERUS X TOOLS VERSION $SCRIPT_VER"
            echo "USAGE: ./builder.sh [options]"
            echo -e "\t{-m|--showmenu}\t\t\t\t\t- run in menu mode."
            echo -e "\t{-t|--tedsign}\t\t\t\t\t- Force local codesign of Ted. Only use this as a last resort."
            echo -e "\t{-p|--prefix}\t\t\t\t\t- Set the application bundle identifier prefix. Default is com.cerberus-x." 
            echo -e "\t{-q|--qtsdk} \"QT_DIR_PATH\"\t\t\t- Set Qt SDK root directory."
            echo -e "\t{-k|--qtkit) \"QT.VERSION.NUM\"\t\t\t- Set Qt SDK version."
            echo -e "\t{-d|--deploy} \"DEPLOY_DIR\"\t\t\t- Create a directory ready for deployment in the directory passed."
            echo -e "\t{-a|--archiver} \"ARCHIVE_TOOL\"\t\t\t- Set the archive tool. The default is to use hdiutil."
            echo -e "\t--clearbuilds\t\t\t\t\t- Removes all previous built binaries of Cerberus within local repository."
            echo -e "\t{-h|--help}\t\t\t\t\t- Show usage."
            echo "EXAMPLES:"
            echo -e "\te.g: ./builder.zsh -q $HOME/Qt -k 6.5.3 --showmenu"
            echo -e "\te.g: ./builder.zsh --qtsdk ~/Qt --qtkit 6.5.3"
            exit 0
        ;;
        -*|--*=) # unsupported flags
            do_error "Error: Unsupported flag $1" >&2
            exit 1
        ;;
        *) # preserve positional arguments
            POSITIONAL_ARGS+=("$1")
            shift
        ;;
    esac
done

set -- "${POSITIONAL_ARGS[@]}"  # This should be any command line arguments that were not processed.

# Check that there is a valid compiler present
setcompiler
[ $EXITCODE -eq 1 ] && { exit 1; }

# Check for a Qt installation. Ted will only become a build option if Qt is installed.
do_qtsdk_check

###############
# MENU/DISPLAY
###############
# Set up the menu items. The array DISPLAY_ITEMS, holds the human readable menu items.
# The array MENU_ITEMS, holds the function names to call.
do_items(){
    DISPLAY_ITEMS=("All" "Transcc")
    MENU_ITEMS=("do_all" "do_transcc")
    [ $TRANSCC_EXE -eq 1 ] && {
        DISPLAY_ITEMS+=("CServer" "Makedocs" "Launcher")
        MENU_ITEMS+=("do_cserver" "do_makedocs" "do_launcher");
    }
    [ ${#QT_INSTALLS[@]} -gt 0 ] && {
        DISPLAY_ITEMS+=("IDE Ted")
        MENU_ITEMS+=("do_ted");
    }
    [ -n "$DEPLOY_PATH" ] && {
        DISPLAY_ITEMS+=("Deploy: $DEPLOY_PATH")
        MENU_ITEMS+=("do_deploy");
    }
    DISPLAY_ITEMS+=("Quit")
    MENU_ITEMS+=("do_quit")
}

do_show_deps() {
    [ ${#QT_INSTALLS[@]} -gt 0 ] && {
        do_info "QMAKE Location: $QT_SELECTED";
    } || {
        do_unknown "Qt SDK is not installed.";
    }
    [ ! -z "$CODESIGN_CERT" ] && {
        do_info "CODESIGN CERTIFICATE FOR TED: $CODESIGN_CERT";
    }
    do_info "APPLICATION BUNDLE PREFIX: $MACOS_BUNDLE_PREFIX"
}

do_title() {
    clear
    do_header "===== Cerberus X Tool Builder Version $SCRIPT_VER ====="
    do_show_deps
    
    for ((i = 1; i <= $#DISPLAY_ITEMS; i++)); do
        echo "$i: ${DISPLAY_ITEMS[$i]}"
    done
}

# Loop for selecting menu options.
[ $SHOW_MENU -eq 1 ] && {
    while true; do
        
        # Test to see if transcc has been built.
        echo $CERBERUS_BIN_DIR
        execute $CERBERUS_BIN_DIR/transcc_macos
        [ $EXITCODE -eq 0 ] && { TRANSCC_EXE=1; } || { TRANSCC_EXE=0; }

        # update the menu and wait for selection.
        do_items
        do_title
        read "REPLY?Select application to build: "
        
        # Only process numbers
        [ -z "${REPLY##*[!0-9]*}" ] || {
            
            # If the value passed is greater than the total size of the selection array; then skip.
            [ $REPLY -gt $((${#MENU_ITEMS[@]})) ] && { continue; }
            
            # Call the required functions based on the option selected.
            [ $REPLY -eq $((${#MENU_ITEMS[@]})) ] && { break; } || {
                do_info "BUILDING ${DISPLAY_ITEMS[$(($REPLY))]}"
                ${MENU_ITEMS[$(($REPLY))]}
                read -k1 -s "?Press any key to continue... ";
            }
        }
    done
    
    clear
    do_success "Cerberus X Builder script terminated."
} || {
    do_info "MENU MODE OFF"
    do_show_deps
    do_all
}