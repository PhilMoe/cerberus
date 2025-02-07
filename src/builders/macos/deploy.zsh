#!/bin/zsh

# DEPLOYMENT BUILDER FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILDER TOOL.

########################################
# COMMON FUNCTION USE BY OTHER SCRIPTS
########################################

#############################################
# BUILD A DEPLOYMENT ARCHIVE
#############################################
# This function requires that git be installed.
# It works by cloning the the current repository, removing any git related items from the clone, and then running the 
# builder script in the clone with the basic set of parameters that were passed. After the cloned builder script has
# finished and control is returned to this function.
do_deploy(){
    local DEPLOYMENT_ROOT_DIR="$DEPLOY_PATH/cx_deploy_root"
    local DEPLOYMENT_BUILD_DIR="$DEPLOYMENT_ROOT_DIR/build"
    local DEPLOYMENT_TARGET_DIR="$DEPLOYMENT_BUILD_DIR/Cerberus"
    local DEPLOYMENT_TARGET_BIN_DIR="$DEPLOYMENT_TARGET_DIR/bin"
    local DEPLOYMENT_TARGET_SRC_DIR="$DEPLOYMENT_TARGET_DIR/src"

    # Test for a .git folder in the Cerberus directory.
    # If there isn't one, then it must be a normal set of sources files.
    [ ! -d "$CERBERUS_ROOT_DIR/.git" ] && {
        do_error "Deployment requires that the sources are contained in a git repository."
        return;
    }

    # Only do a deployment build if git is installed.
    execute git --version
    [ $EXITCODE -ne 0 ] && {
        do_error "git is required to build."
        return;
    }

    # Set up the deploy directories
    [ -d "$DEPLOYMENT_ROOT_DIR" ] && { rm -rf "$DEPLOYMENT_ROOT_DIR"; }
    execute mkdir -p "$DEPLOYMENT_TARGET_DIR"
    [ $EXITCODE -ne 0 ] && {
        do_error "Failed to create deployment directory: $DEPLOYMENT_TARGET_DIR"
        return;
    }

    ## Test if the git repository is in a clean state. Else issue a error message and return back
    # to the menu. If everything is okay, then clone the git repository.
    local STATUS=$(execute git status 2>&1)
    echo "$STATUS"
    if [[ $STATUS == *"nothing to commit, working tree clean"* ]]; then
        execute git -C $DEPLOYMENT_BUILD_DIR clone $CERBERUS_ROOT_DIR Cerberus
        [ $EXITCODE -ne 0 ] && {
            do_error "Failed to clone to directory: $DEPLOYMENT_BUILD_DIR"
            return;
        }
    else
        do_error "Repository is not clean. Check for untracked and uncommitted files."
        return
    fi

    # Clean up any git stuff that's been copied over.
    # Additional file and directories should be added here.
    local RM_FILES=("$DEPLOYMENT_TARGET_DIR/.git" "$DEPLOYMENT_TARGET_DIR/.gitignore" "$DEPLOYMENT_TARGET_DIR/.gitattributes")
    for r in "${RM_FILES[@]}"; do
        [ -d "$r" ] && {
            rm -rf "$r";
        } || {
            [ -f "$r" ] && { rm -f "$r"; };
        }
    done

    # Generate parameters to pass on to the cloned builder script.
    # Only the basic parameters need to be passed on.
    local PARAMS=("$DEPLOYMENT_TARGET_SRC_DIR/builder.zsh")
    if [ -n "$QTDIR" ]; then PARAMS+=("-q" "$QTDIR"); fi
    if [ -n "$QTVER" ]; then PARAMS+=("-k" "$QTVER"); fi
    if [ -n "$MACOS_BUNDLE_PREFIX" ]; then PARAMS+=("-p" "$MACOS_BUNDLE_PREFIX"); fi
    if [ -n "$CODESIGN_CERT" ]; then PARAMS+=("-c" "$CODESIGN_CERT"); fi

    #if [ -n "$ARCHIVER" ]; then PARAMS+=("-a" "$ARCHIVER"); fi

    # Start the cloned builder script with the parameter that were passed to the main builder script.
    chmod +x $DEPLOYMENT_TARGET_SRC_DIR/builder.zsh
    do_info "Execuiting:\n${PARAMS[@]}"
    execute ${PARAMS[@]}

    # Check that the default files have been created before compressing to an archive. NOTE: Qt SDK's are not included in the check.
    CHECK_FILES=("$DEPLOYMENT_TARGET_BIN_DIR/transcc_macos" "$DEPLOYMENT_TARGET_BIN_DIR/makedocs_macos")
    CHECK_FILES+=("$DEPLOYMENT_TARGET_BIN_DIR/cserver_macos.app/Contents/MacOS/CerberusGame" "$DEPLOYMENT_TARGET_BIN_DIR/Ted.app/Contents/MacOS/Ted" "$DEPLOYMENT_TARGET_DIR/Cerberus.app/Contents/MacOS/Cerberus")

    CHECK_COUNT=0
    for filecheck in ${(@)CHECK_FILES}; do
        do_info "CHECKING FOR $filecheck"
        [ -f "$filecheck" ] && {
            do_success "Found: $filecheck"
            ((CHECK_COUNT++));
        }
    done

    # Only compress if the file count match that of the array.
    [ $CHECK_COUNT -ne ${#CHECK_FILES[@]} ] && {
        do_error "Deployment file error."
        EXITCODE=1
        return $EXITCODE
    }

    # TODO: ARCHIVE FILES
    do_unknown "\nDEPLOYMENT OF CERBERUS NOW REQUIRES THE USUAL APPLE CERTIFICATE AND  NOTARIZING PROCESS"
}

