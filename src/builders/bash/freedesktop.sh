#!/bin/bash

# BASH SCRIPT TO SET UP FOR LINUX FREE DESKTOP VERSION 1.0.1
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.
# THIS SCRIPT REQUIRE THE USE OF TWO SVG FILES FOR THE LAUNCHER ICON AND THE SOURCE FILE MIME ICON.

# THIS SCRIPT RELIES ON THE XDG-UTILS BEING INSTALLED.
# SEE https://www.freedesktop.org/wiki/Software/xdg-utils/

# QUICK NOTES:
# xdg-mime - The mime xml file name should be ${vendor_prefix}-$app_name.xml
#            The element named mime-type within that file should be application/x-${app_name}, thus the png file name
#            should be ${app_name}-mime-icon-${sise}.png and the xdg-icon-resource "icon-name"
#            should be application-x-${app_name}
#
# The desktop launcher icon file name should match the launchers name. but with a postfix of -icon.

# KNOWN ISSUES - The MIME icon may not update in some file browsers.

FREE_DESKTOP_SCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
CERBERUS_ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../../../" >/dev/null 2>&1 && pwd )"
TEMP_DIR="$FREE_DESKTOP_SCRIPT/temp"

# ICON_STORE_DIR is the location where coverted svg image are stored.
ICON_STORE_DIR="$CERBERUS_ROOT_DIR/src/builders/images/hicolor"

# VENDOR_PREFIX is use of the naming of the mime xml file and the .desktop file
VENDOR_PREFIX="cerberusxdev"

# APPLICATION_NAME is the name of the actual application.
APPLICATION_NAME="cerberusx"

# DESKTOP_NAME is the file name of the .desktop
DESKTOP_NAME="$VENDOR_PREFIX-$APPLICATION_NAME"

# MIME_ICON_NAME and APP_ICON_NAME are used as the icon-name identifier when using the xdg-icon-resorce application.
MIME_ICON_NAME="application-x-$APPLICATION_NAME"
APP_ICON_NAME="$DESKTOP_NAME-icon"

# MIME_TYPE is use for the mime-type element in the mime xml file.
MIME_TYPE="application/x-$APPLICATION_NAME"

# MIME_ICON_FILE and APP_ICON_FILE are the real base names used for the png files.
MIME_ICON_FILE="$APPLICATION_NAME-mimetype-icon"
APP_ICON_FILE="$APPLICATION_NAME-app-icon"

# MIME releated information
MIME_COMMENT_LANG=("en" "Cerberus X source")
MIME_GLOB=("*.cxs" "*.CXS")

# Default locations for storing Cerberus X XDG files.
USER_APPLICATION_DIR="$HOME/.local/share/applications"
USER_MIME_PACKAGES="$HOME/.local/share/mime/packages"
USER_ICON_DIR="$HOME/.local/share/icons"

# Common icon sizes
ICON_SIZES=("16" "22" "24" "32" "48" "64" "128")

# Default svg file names.
SVG_APP_ICON=
SVG_MIME_ICON=

do_freedesktop_info() {
    echo -e "\033[36m$1\033[0m\n"
}

do_freedesktop_header(){
    echo -e "\033[33m$1\033[0m"
}

do_freedesktop_success(){
    echo -e "\033[32m$1\033[0m"
}

do_freedesktop_error(){
    echo -e "\033[31m$1\033[0m"
}

do_freedesktop_unknown(){
    echo -e "\033[35m$1\033[0m"
}

# Make sure that the required directories are ready for the file to be placed into.
do_xdg_setup(){
    do_freedesktop_header "Checking XDG locations"
    mkdir -p $TEMP_DIR
    mkdir -p $USER_APPLICATION_DIR
    mkdir -p $USER_ICON_DIR
    mkdir -p $USER_MIME_PACKAGES

    # The latest specs for freedesktop state the the mimeapp.list in .local/share/applications is depreciated
    # and has been move to .config. If it doesn't already exist in the .local/shared/applications, then make a
    # link to the on in .config.
    [ -f "$HOME/.config/mimeapps.list" ] && {
        [ ! -f "$USER_APPLICATION_DIR/mimeapps.list" ] && {
            ln -s "$HOME/.config/mimeapps.list" "$USER_APPLICATION_DIR/mimeapps.list";
        };
    }
}

# Function to generate the mime type xml package file.
do_generate_mime_package() {
    do_freedesktop_header "Creating MIME package data"

    # Generate a new mime package xml data file.
    local MIME_XML=(
                    "<?xml version=\"1.0\"?>"
                    "<mime-info xmlns='http://www.freedesktop.org/standards/shared-mime-info'>"
                    "   <mime-type type=\"$MIME_TYPE\">"
    )

    # Add language specific comments.
    for ((i=0;i< ${#MIME_COMMENT_LANG[@]} ;i+=2)); do
        MIME_XML+=("       <comment xml:lang=\"${MIME_COMMENT_LANG[i]}\">${MIME_COMMENT_LANG[i+1]}</comment>")
    done

    # Appends globs
    for glob in "${MIME_GLOB[@]}"; do
        MIME_XML+=("       <glob pattern=\"$glob\"/>")
    done

    MIME_XML+=( "       <icon name=\"$MIME_ICON_NAME\"/>"
                "       <generic-icon name=\"$MIME_ICON_NAME\"/>"
                "   </mime-type>"
                "</mime-info>"
    )

    printf "%s\n" "${MIME_XML[@]}" > "$TEMP_DIR/$DESKTOP_NAME.xml"
    [ $? -eq 0 ] && { do_freedesktop_success "MIME extension created."; } || { do_freedesktop_error "Failed to create MIME extension."; }
}

# Generate the .desktop launcher that is to be used as a first run setup.
# This launcher will be place in the Cerberus X root directory and call this script using the --install options to set up
# a menu item, icons, mime association and desktop launcher.
do_generate_setup_launcher(){
    do_freedesktop_header "Creating Install desktop launcher"

    # kung fu line!
    # bash -c '_PWD=\"\$PWD\"; cd \"\`echo \$0 | sed s/"$DESKTOP_NAME.desktop"//\`\"; chmod +x ./src/builders/bash/freedesktop.sh; ./src/builders/bash/freedesktop.sh --setup; ./Cerberus %F; cd \"\$_PWD\"' %k"
    # store the current PWD, get the .desktop file path and use sed to remove the desktop name, start the installation of the icons, mime and menu, set Cerberus X and restore the PWD
    local DESKTOP_FILE=(
        "#!/usr/bin/env xdg-open"
        "[Desktop Entry]"
        ""
        "Version=1.0"
        "Type=Application"
        "Encoding=UTF-8"
        "Terminal=false"
        "Exec=bash -c '_PWD=\"\$PWD\"; cd \"\`echo \$0 | sed s/"$DESKTOP_NAME-install.desktop"//\`\"; chmod +x ./src/builders/bash/freedesktop.sh; ./src/builders/bash/freedesktop.sh --setup; ./Cerberus %F; cd \"\$_PWD\"' %k"
        "Icon=$APP_ICON_NAME"
        "Name=Linux Install"
        "Categories=Development"
        "Comment[en_GB.UTF-8]=Linux Desktop Install Launcher"
    )

    printf "%s\n" "${DESKTOP_FILE[@]}" > "$CERBERUS_ROOT_DIR/$DESKTOP_NAME-install.desktop"
    [ $? -eq 0 ] && { do_freedesktop_success "Created install desktop launcher $DESKTOP_NAME."; } || { do_freedesktop_error "Failed to create install desktop launcher $DESKTOP_NAME."; };

}

# Generate the .desktop launcher that is to be used as a clean up.
# This launcher will be place in the Cerberus X root directory and call this script using the --uninstall options to set up
# a menu item, icons, mime association and desktop launcher.
do_generate_remove_launcher(){
    do_freedesktop_header "Creating Uninstall desktop launcher"

    # kung fu line!
    # bash -c '_PWD=\"\$PWD\"; cd \"\`echo \$0 | sed s/"$DESKTOP_NAME.desktop"//\`\"; chmod +x ./src/builders/bash/freedesktop.sh; ./src/builders/bash/freedesktop.sh --uninstall; ./Cerberus %F; cd \"\$_PWD\"' %k"
    # store the current PWD, get the .desktop file path and use sed to remove the desktop name, start the installation of the icons, mime and menu, set Cerberus X and restore the PWD
    local DESKTOP_FILE=(
        "#!/usr/bin/env xdg-open"
        "[Desktop Entry]"
        ""
        "Version=1.0"
        "Type=Application"
        "Encoding=UTF-8"
        "Terminal=false"
        "Exec=bash -c '_PWD=\"\$PWD\"; cd \"\`echo \$0 | sed s/"$DESKTOP_NAME-uninstall.desktop"//\`\"; chmod +x ./src/builders/bash/freedesktop.sh; ./src/builders/bash/freedesktop.sh --uninstall; cd \"\$_PWD\"' %k"
        "Icon=$APP_ICON_NAME"
        "Name=Linux Uninstall"
        "Categories=Development"
        "Comment=Linux Desktop Uninstall Launcher"
    )

    printf "%s\n" "${DESKTOP_FILE[@]}" > "$CERBERUS_ROOT_DIR/$DESKTOP_NAME-uninstall.desktop"
    [ $? -eq 0 ] && { do_freedesktop_success "Created uninstall desktop launcher $DESKTOP_NAME."; } || { do_freedesktop_error "Failed to create uninstall desktop launcher $DESKTOP_NAME."; };

}

# Function to generate the .desktop file to be used in ./loval/shared/applications and the user desktop.
do_generate_menu_launcher(){
    
    do_freedesktop_header "Creating Cerberus X memu desktop launcher"

    local DESKTOP_FILE_USER=(
        "#!/usr/bin/env xdg-open"
        "[Desktop Entry]"
        ""
        "Version=1.0"
        "Type=Application"
        "Encoding=UTF-8"
        "Terminal=false"
        "Exec=$CERBERUS_ROOT_DIR/Cerberus %F"
        "Icon=$APP_ICON_NAME"
        "Name=Cerberus X"
        "MimeType=$MIME_TYPE;"
        "Categories=Development"
        "Comment[en_GB.UTF-8]=Cerberus X Integrated Development Environment"
        "GenericName[en_GB.UTF-8]=Create Games for HTML5 web browsers, PC desktop and Android."
        "Name[en_GB]=Cerberus X"
    )

    printf "%s\n" "${DESKTOP_FILE_USER[@]}" > "$TEMP_DIR/$DESKTOP_NAME.desktop"
    [ $? -eq 0 ] && { do_freedesktop_success "Created Desktop launcher $DESKTOP_NAME."; } || { do_freedesktop_error "Failed to create desktop launcher $DESKTOP_NAME."; };
}

# Function to call inkscape to convert svg files into png files.
# $1= size, $2= exorted file, $3= input file
do_convert_svg(){
    do_freedesktop_info "Convering $3 to a ${1}x$1 png as $2"
    inkscape -w $1 -h $1 --export-filename="$2" --export-type="png" "$3"
}

# Generate the png images for use with the .dektop and mime types.
# These are stored in the builder/images directory. Generating the freedesktop launcher copies these files to the
# users local directory.
do_generate_icons(){

    # Before cleaning out old icons. Check that inkscape is installed.
    command -v inkscape >/dev/null 2>&1 || {
        do_freedesktop_unknown "Inkscape not installed.\nSkipping creation of icons."
        return;
    }

    [ ! -f "$SVG_APP_ICON" ] && {
        do_freedesktop_unknown "SVG FILE: $SVG_APP_ICON is missing\nSkipping icon creation."
        return;
    }

    [ ! -f "$SVG_MIME_ICON" ] && {
        do_freedesktop_unknown "SVG FILE: $SVG_MIME_ICON is missing\nSkipping icon creation."
        return;
    }

    # Clean out any old icons before generating new ones.
    [ -d "$ICON_STORE_DIR" ] && { rm -rf "$ICON_STORE_DIR"; }

    for icon_size in "${ICON_SIZES[@]}"; do
       mkdir -p "$ICON_STORE_DIR/${icon_size}x$icon_size/apps"
       do_convert_svg $icon_size "$ICON_STORE_DIR/${icon_size}x$icon_size/apps/$APP_ICON_FILE-$icon_size.png" $SVG_APP_ICON
       mkdir -p "$ICON_STORE_DIR/${icon_size}x$icon_size/mimetypes"
       do_convert_svg $icon_size "$ICON_STORE_DIR/${icon_size}x$icon_size/mimetypes/$MIME_ICON_FILE-$icon_size.png" $SVG_MIME_ICON
    done
}

# Function to uninstall the MIME xml file
do_uninstall_mime_check(){
    # Uninstall the MIME association
    [ -f "$USER_MIME_PACKAGES/$DESKTOP_NAME.xml" ] && {
        do_freedesktop_info "Removing: $USER_MIME_PACKAGES/$DESKTOP_NAME.xml"
        xdg-mime uninstall "$USER_MIME_PACKAGES/$DESKTOP_NAME.xml"
    }
}

#Function to uninstall the menu entry
do_uninstall_menu_check(){
[ -f "$USER_APPLICATION_DIR/$DESKTOP_NAME.desktop" ] && {
        do_freedesktop_info "Removing: $USER_APPLICATION_DIR/$DESKTOP_NAME.desktop"
        xdg-desktop-menu uninstall "$USER_APPLICATION_DIR/$DESKTOP_NAME.desktop";
    }
}

# Check to see if the MIME type association file has be created
do_install_xdg_mime_xml_check(){
    do_uninstall_mime_check
    do_generate_mime_package
    cp -u "$TEMP_DIR/$DESKTOP_NAME.xml" "$USER_MIME_PACKAGES/$DESKTOP_NAME.xml";
    do_freedesktop_info "xdg-mime install $USER_MIME_PACKAGES/$DESKTOP_NAME.xml"
    xdg-mime install "$USER_MIME_PACKAGES/$DESKTOP_NAME.xml";
}

# Check the the desktop launcher has been created
do_install_xdg_memu_check(){
    do_uninstall_menu_check
    do_generate_menu_launcher
    do_freedesktop_info "xdg-desktop-menu install $TEMP_DIR/$DESKTOP_NAME.desktop"
    xdg-desktop-menu install "$TEMP_DIR/$DESKTOP_NAME.desktop";
}

# Install all generates file that are meant for XDG
do_install_xdg_items(){
    do_freedesktop_header "Installing Ceberus X XDG items"
    do_xdg_setup                    # Make sure that the directories are correct.
    do_install_xdg_mime_xml_check   # Make sure that there is a mime xml association file.
    do_install_xdg_memu_check       # Make sure that there is a desktop menu file.

    # Finally, install register the XDG icon associations.
    for icon_size in "${ICON_SIZES[@]}"; do
        ICN_1=$ICON_STORE_DIR/${icon_size}x$icon_size/apps/$APP_ICON_FILE-$icon_size.png
        ICN_2=$ICON_STORE_DIR/${icon_size}x$icon_size/mimetypes/$MIME_ICON_FILE-$icon_size.png

        [ -f $ICN_1 ] && {
            do_freedesktop_info "Installing application icon: ${icon_size}x$icon_size $ICN_1 $APP_ICON_NAME"
            xdg-icon-resource install --noupdate --size $icon_size $ICN_1 $APP_ICON_NAME;
        } || { do_freedesktop_error "NO $ICN_1"; }

        [ -f $ICN_2 ] && {
             do_freedesktop_info "Installing mime icon: ${icon_size}x$icon_size $ICN_2 $MIME_ICON_NAME"
             xdg-icon-resource install --noupdate --context mimetypes --size $icon_size $ICN_2 $MIME_ICON_NAME;
        } || { do_freedesktop_error "NO $MIME_ICON_FILE-$icon_size.png"; }
    done

    # Refresh the icon resources.
    xdg-icon-resource forceupdate
}

do_uninstall_xdg_items(){
    do_freedesktop_header "Unisntalling Ceberus X XDG items"

    # Un register the XDG icon associations
    for icon_size in "${ICON_SIZES[@]}"; do
        ICN_1="$ICON_STORE_DIR/${icon_size}x$icon_size/apps/$APP_ICON_FILE-$icon_size.png"
        ICN_2="$ICON_STORE_DIR/${icon_size}x$icon_size/mimetypes/$MIME_ICON_FILE-$icon_size.png"

        [ -f $ICN_1 ] && {
            do_freedesktop_info "Removing: --noupdate --novendor -size $icon_size $ICN_1 $APP_ICON_NAME"
            xdg-icon-resource uninstall --noupdate --novendor --size $icon_size $ICN_1 $APP_ICON_NAME;
        } || { do_freedesktop_error "$APP_ICON_FILE-$icon_size.png"; }

        [ -f $ICN_2 ] && {
            do_freedesktop_info "Removing: --noupdate --context mimetypes --size $icon_size $ICN_2 $MIME_ICON_NAME"
            xdg-icon-resource uninstall --noupdate --context mimetypes --size $icon_size $ICN_2 $MIME_ICON_NAME;
        } || { do_freedesktop_error "$MIME_ICON_FILE-$icon_size.png"; }
    done

    # Refresh the icon resources.
    xdg-icon-resource forceupdate

    # Remove mime and menu entries.
    do_uninstall_mime_check
    do_uninstall_menu_check
}

do_init_linux_desktop(){
    # If freedesktop.sh is part of the builder.sh process, then the actions
    [ -n "$GEN_ICONS" ] && {
        do_generate_icons;
    } || {
        [ -n "$BULDER_SCRIPT" ] && {
            do_freedesktop_unknown "WARNING: ICONS WILL NOT BE GENERATED\nRerun builder.sh with option -i default, or -i icon1.svg icon2.svg"
        }
    }

    # The only file that needs to be processed here is the one to generate the Cerberus X setup
    do_generate_setup_launcher
    do_generate_remove_launcher
}

# LINUX ONLY
# This allows for the creation of new icon files and removel of the freedesktop files.

if [ "$(uname -s)" = "Linux" ]; then
    [ -z "$BULDER_SCRIPT" ] && {

        # Standalone script will allow for the stored icons to be updated.
        # To generate new icons inkscape must be installed.
        while [[ $# -gt 0 ]]; do
            case $1 in
                # Generate launcher and icons
                -i|--icons)
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
                        do_generate_icons
                    fi
                    shift
                    ;;
                -u|--uninstall)
                    [ -d $TEMP_DIR ] && { rm -rf $TEMP_DIR; }
                    do_uninstall_xdg_items
                    break
                    ;;
                -s|--setup)
                    do_install_xdg_items
                    [ -d $TEMP_DIR ] && { rm -rf $TEMP_DIR; }
                    shift
                    break
                    ;;
                -l|--launcher)
                    do_generate_setup_launcher
                    do_generate_remove_launcher
                    shift
                    ;;
                -h|--help)
                do_freedesktop_info "CERBERUS X LINUX LAUNCHER"
                    echo "To generate new icons. Pass option {-i|--icon} with full paths to svg files."
                    echo "USAGE: ./freedesktop.sh [options]"
                    echo -e "\t{-s|--setup}\t\tInstall Cerberus XDG items."
                    echo -e "\t{-l|--launcher}\t\tGenerate the Ceberus X setup launcher."
                    echo -e "\t{-u|--uninstall}\tUninstall Cerberus XDG items."
                    echo -e "\t{-i|--icon}\t\tPATH_TO_APPLICATION_ICON.svg PATH_TO_MIME_ICON.svg"
                    echo -e "\t{-h|--help}\t\tShow usage."
                    echo "EXAMPLES:"
                    echo -e "\te.g: ./freedesktop.sh -i $HOME/app.svg $HOME/mime.svg -l"
                    echo -e "\te.g: ./freedesktop.sh --setup"
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
        
    };
else
    do_freedesktop_error "FREE DESKTOP SCRIPT: THIS SCRIPT IS FOR LINUX USE ONLY";
fi