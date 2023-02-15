#!/bin/bash
# LINUX AND MAC OS X BASH SCRIPT FOR BUILDING CERBERUS X TOOLS

# Get this script directory
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
TRANSCC_EXE=0
BULDER_SCRIPT=1
SCRIPT_VER="1.0.3"
QT_SELECTED=

# Import the dependencies that this script relies on.
source "$SCRIPTPATH/builders/bash/common.sh"
source "$SCRIPTPATH/builders/bash/thirdparty.sh"
[ $HOST = "linux" ] && { source "$SCRIPTPATH/builders/bash/freedesktop.sh"; }
source "$SCRIPTPATH/builders/bash/tools.sh"

# Check that there is a valid compiler present
setcompiler
[ $EXITCODE -eq 1 ] && { exit 1; }

# Process command line arguments.
# The only options are for the Qt SDK root directory and the version to use.
# NOTE: Linux distributions can use these, but you are on your own to deploy
#       Qt dependencies if they are not already part of the run time libraries.
#       of the target distribution. So for Linux, it's best to use the distributions
#       default repositories for Qt to build against.
POSITIONAL_ARGS=()

while [[ $# -gt 0 ]]; do
    case $1 in
        -q|--qtdir)
            QTDIR="$2"
            shift; shift
        ;;
        -v|--qtver)
            QTVER="$2"
            shift; shift
        ;;
        -m|--showmenu)
            SHOW_MENU=1
            shift
        ;;
        -i|--icons)
            
            # Only do the generation of icons on a Linux host.
            [ $HOST = "linux" ] && {
                if [ "$2" != "default" ]; then
                    SVG_APP_ICON="$3"
                    SVG_MIME_ICON="$4";
                    shift; shift
                else
                    SVG_APP_ICON=$CERBERUS_ROOT_DIR/src/builders/images/svg/Logo.svg
                    SVG_MIME_ICON=$CERBERUS_ROOT_DIR/src/builders/images/svg/AppIcon.svg
                fi
                
                # Only generate the icons if both icon files are passed as parameters.
                if [ -f "$SVG_APP_ICON" ] && [ -f "$SVG_MIME_ICON" ]; then
                    GEN_ICONS=1
                fi
            }
            shift
        ;;
        -h|--help)
            do_info "CERBERUS X TOOLS VERSION $SCRIPT_VER"
            echo "USAGE: ./builder.sh [options]"
            echo -e "\t{-m|--showmenu}\t\t\t\t\t- run in menu mode."
            echo -e "\t{-q|--qtsdk} \"QT_DIR_PATH\"\t\t\t- Set Qt SDK root directory."
            echo -e "\t{-v|--qtver) \"QT.VERSION.NUM\"\t\t\t- Set Qt SDK version."
            [ $HOST = "linux" ] && {
                echo -e "\t{-i|--icons} \"APP_ICON.svg\" \"MIME_ICON.svg\"\t- Generate desktop icons.";
            }
            echo -e "\t{-h|--help}\t\t\t\t\t- Show usage."
            echo "EXAMPLES:"
            echo -e "\te.g: ./builder.sh -q $HOME/Qt -v 5.14.0 --showmenu"
            echo -e "\te.g: ./builder.sh --qtsdk ~/Qt --qtver 5.14.0"
            exit 0
        ;;
        -*|--*=) # unsupported flags
            echo "Error: Unsupported flag $1" >&2
            exit 1
        ;;
        *) # preserve positional arguments
            POSITIONAL_ARGS+=("$1")
            shift
        ;;
    esac
done

set -- "${POSITIONAL_ARGS[@]}"

# Check for a Qt installation. Ted will only become a build option if Qt is installed.
do_qtsdk_check

# Set up the menu items. The array DISPLAY_ITEMS, holds the human readable menu items.
# The array MENU_ITEMS, holds the function names to call.
do_items(){
    DISPLAY_ITEMS=("All" "Transcc")
    MENU_ITEMS=("do_all" "do_transcc")
    [ $TRANSCC_EXE -eq 1 ] && {
        DISPLAY_ITEMS+=("CServer" "Makedocs" "Launcher")
        MENU_ITEMS+=("do_cserver" "do_makedocs" "do_launcher")
        
        [ $HOST = "linux" ] && {
            DISPLAY_ITEMS+=("Generate Free Desktop Launcher")
            MENU_ITEMS+=("do_freedesktop");
        };
    }
    [ ${#QT_INSTALLS[@]} -gt 0 ] && {
        DISPLAY_ITEMS+=("IDE Ted")
        MENU_ITEMS+=("do_ted");
    }
    DISPLAY_ITEMS+=("Quit")
    MENU_ITEMS+=("do_quit")
}

do_show_deps() {
    [ ${#QT_INSTALLS[@]} -gt 0 ] && {
        do_info "QMAKE Location: $QT_SELECTED";
        } || {
        do_unknown "Qt SDK is not installed."
    }
}

do_title() {
    clear
    do_header "===== Cerberus X Tool Builder Version $SCRIPT_VER ====="
    do_show_deps
    
    for i in "${!DISPLAY_ITEMS[@]}"; do
        echo "$(($i+1)): ${DISPLAY_ITEMS[$i]}"
    done
}

# Loop for selecting menu options.
[ $SHOW_MENU -eq 1 ] && {
    while true; do
        
        # Test to see if transcc has been built.
        execute $BIN/transcc_$HOST
        [ $EXITCODE -eq 0 ] && { TRANSCC_EXE=1; } || { TRANSCC_EXE=0; }
        
        # update the menu and wait for selection.
        do_items
        do_title
        read -p "Select application to build: "
        
        # Only process numbers
        [ -z "${REPLY##*[!0-9]*}" ] || {
            
            # If the value passed is greater than the total size of the selection array; then skip.
            [ $REPLY -gt $((${#MENU_ITEMS[@]})) ] && { continue; }
            
            # Call the required functions based on the option selected.
            [ $REPLY -eq $((${#MENU_ITEMS[@]})) ] && { break; } || {
                do_info "BUILDING ${DISPLAY_ITEMS[$(($REPLY-1))]}"
                ${MENU_ITEMS[$(($REPLY-1))]}
                read -p "Press any key to continue... " -n1 -s;
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