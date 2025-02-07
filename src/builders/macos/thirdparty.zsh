#!/bin/zsh

# THIRD-PARTY FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILDER TOOL.

#######################################################################################################
#   DETECTION AND SET UP OF THIRD PARTY TOOLS, FRAME WORKS AND COMPILER TOOL CHAINS
#######################################################################################################

#######################################
# QT MAC OS CHECK
#######################################
# From Qt 6.1.2 the sdk compiler type directory changed to macos
do_check_version(){ printf "%03d%03d%03d%03d" $(echo "$1" | tr '.' ' '); }
do_macos_check(){
    [ $(do_check_version $1) -lt $(do_check_version "6.1.2") ] && { QMAKE_TYPE="clang_64"; } || { QMAKE_TYPE="macos"; }
}

############################
# QT FRAME WORK DETECTION
############################
# Scan the Qt directory passed as a parameter for Qt installations
do_qtsdk_scanner(){
    
    # Get all directories in the directory passed for Qt and sort in the reverse order.
    DIRS=($(printf "%s\n" `ls ${QTDIR}` | sort -s -t- -k 2,2nr | sort -t. -s -k 1,1nr -k 2,2nr -k 3,3nr -k 4,4nr));
    
    # Only process the DIRS array if there are elements present.
    [ ${#DIRS[@]} -gt 0 ] || { return; }
    
    # Use a regular expression to only store those elements with dot version numbers in to the QT_INSTALLS array.
    re='^[0-9]+(\.[0-9]+)*$';
    for d in "${DIRS[@]}"; do
        [[ $d =~ $re ]] && {
            do_macos_check "$d"
            execute "$QTDIR/$d/$QMAKE_TYPE/bin/qmake" -v
            [ $EXITCODE -eq 0 ] && { QT_INSTALLS+=("$QTDIR/$d/$QMAKE_TYPE"); };
        }
    done
}

# Function to test if there are any Qt SDK installations.
do_qtsdk_check(){
    EXITCODE=0
    
    do_header "CHECKING FOR QT INSTALLATION"
    # The first test is if arguments passed were for the Qt SDK root directory and version to use.
    # If this is successful, then the systems PATH variable is update to make sure that this
    # Qt SDK is the first before any other installed.
    # If the test failed; then try to find at least one Qt SDK installation in the Qt SDK root directory
    # if that was passed.
    [ -f "${QTDIR}/${QTVER}/${QMAKE_TYPE}/bin/qmake" ] && {
        
        # Update the QMAKE_TYPE directory root if Qt is greater the Qt 5.
        do_macos_check "$QTVER"
        
        execute "${QTDIR}/${QTVER}/${QMAKE_TYPE}/bin/qmake" --version
        [ $EXITCODE -eq 0 ] && {
            export PATH="${QTDIR}/${QTVER}/${QMAKE_TYPE}/bin:$PATH"
            QT_INSTALLS+=("`which qmake`")
            QT_SELECTED=$QTVER
            return $EXITCODE;
        };
    } || {
        
        # If the Qt root directory was passed; then test for the maintenance tool.
        # If the tool was successfully run; then proceed to get any installed Qt SDKs.
        # If there are any elements stored in the QT_INSTALLS array; then only pre-end
        # the PATH environment variable with either the one passed with option to get the version or help, or get
        # the first element in the QT_INSTALLS array.
        [ -n "$QTDIR" ] && {
            execute open -a "${QTDIR}/MaintenanceTool.app"  --args -h 2>/dev/null
            # Only scan the Qt directory if the maintenance tool was successfully run.
            [ $EXITCODE -eq 0 ] && {
                do_success "FOUND ${QTDIR}/MaintenanceTool.app"
                do_qtsdk_scanner
                [ ${#QT_INSTALLS[@]} -gt 0 ] && {
                    echo "QtT VER: $QTVER"
                    # If no version number was passed as a parameter; then use the first Qt SDK in the array.
                    [ -z "${QTVER}" ] && {
                        QT_SELECTED=${QT_INSTALLS[1]}
                        export PATH="${QT_INSTALLS[1]}/bin:$PATH";
                        echo "QT VER: $QTVER"
                        echo "QT SELECTED: $QT_SELECTED"
                    } || {
                        
                        # Match the version number.
                        for i in "${QT_INSTALLS[@]}"; do
                            [[ $i = *"/$QTVER/"* ]] && {
                                do_success "FOUND $i"
                                QT_SELECTED=$i
                                break;
                            }
                        done
                        [ -z "$QT_SELECTED" ] && {
                            do_unknown "Unknown Qt $QTVER\nSelecting highest version detected.";
                            QT_SELECTED=${QT_INSTALLS[1]}
                            return $EXITCODE;
                        } || {
                            export PATH="$QT_SELECTED/bin:$PATH";
                        }
                    };
                };
            }
        } || {
            do_error "Qt directory not found. Use -q or --qtsdk to set.";
        }
    }
    
    return $EXITCODE
}

############################
# COMPILER DETECTION
############################
setcompiler() {
    do_header "CHECKING FOR COMPILER INSTALLATION"

    # Check for xcode by running it.
    execute xcodebuild -h 2> /dev/null
    [ $EXITCODE -eq 0 ] || {
        do_error "XCODEBUILD NOT PRESENT.\nINSTALL XCODE AND THE XCODE COMMANDLINE TOOLS."
        return $EXITCODE;
    }
    do_success "FOUND: $(xcodebuild -version)"
}