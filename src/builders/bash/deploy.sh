#!/bin/bash

# TOOL BUILDER FUNCTIONS VERSION
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.
do_deploy(){
    EXITCODE=0
    # Setup local varaibles that point to the deploy location
    local CX_DEPLOY_ROOT="$DEPLOY/Cerberus"
    local CX_DEPLOY_BIN="$CX_DEPLOY_ROOT/bin"
    local CX_DEPLOY_SRC="$CX_DEPLOY_ROOT/src"
    
    # Test for a .git folder
    [ ! -d "$CERBERUS_ROOT_DIR/.git" ] && {
        do_error "Delopyment requires that the sources are contained in a git repository."
        return;
    }

    # Test for a git
    execute git --version
    [ $EXITCODE -ne 0 ] && {
        do_error "git is required to build."
        return;
    }

    # Set up the deploy directory
    [ -d "$CX_DEPLOY_ROOT" ] && { rm -rf "$CX_DEPLOY_ROOT"; }
    execute mkdir -p "$CX_DEPLOY_ROOT"
    [ $EXITCODE -ne 0 ] && {
        do_error "Failed to create deploy directory: $CX_DEPLOY_ROOT"
        return;
    }

    # Test if the git repository is in a clean state. And if clean, clone it.
    STATUS=$(execute git status 2>&1)
    echo "$STATUS"
    if [[ $STATUS == *"nothing to commit, working tree clean"* ]]; then
        execute git -C $DEPLOY clone $ROOT Cerberus
        [ $EXITCODE -ne 0 ] && {
            do_error "Failed to clone to directory: $DEPLOY"
            return;
        }
    else
        do_error "Repository is not clean. Check for untracked and uncommited files."
        return
    fi

    # Clean up any git stuf that's been copied over.
    # Additional file and directories should be added here.
    local RM_FILES=("$CX_DEPLOY_ROOT/.git" "$CX_DEPLOY_ROOT/.gitignore" "$CX_DEPLOY_ROOT/.gitattributes")
    for r in "${RM_FILES[@]}"; do
        [ -d "$r" ] && {
            rm -rf "$r";
        } || {
            [ -f "$r" ] && { rm -f "$r"; };
        }
    done

    # Generate parameters to pass on.
    PARAMS=("$CX_DEPLOY_SRC/builder.sh")
    if [ -n "$QTDIR" ]; then PARAMS+=("-q" "$QTDIR"); fi
    if [ -n "$QTVER" ]; then PARAMS+=("-k" "$QTVER"); fi
    if [ -n "$ARCHIVER" ]; then PARAMS+=("-a" "$ARCHIVER"); fi

    # Linux or macos specific
    [ $HOST = "linux" ] && {
        if [[ $GEN_ICONS -eq 1 ]]; then PARAMS+=("-i" "$SVG_APP_ICON" "$SVG_MIME_ICON"); fi
        if [ -n "$GCC_VER" ]; then PARAMS+=("-g" "$GCC_VER"); fi
    } || {
        if [ -n "$MACOS_BUNDLE_PREFIX" ]; then PARAMS+=("-p" "$MACOS_BUNDLE_PREFIX"); fi
        if [ -n "$CERT" ]; then PARAMS+=("-c" "$CERT"); fi
    }

    # Start a fresh build script with all the parameters needed.
    chmod +x $CX_DEPLOY_SRC/builder.sh
    do_info "Execuiting: 
    ${PARAMS[@]}"
    execute ${PARAMS[@]}

    # Check that the default files have been created before compressing to an archive.
    CHECK_FILES=("$CX_DEPLOY_BIN/transcc_$HOST" "$CX_DEPLOY_BIN/makedocs_$HOST")
    [ $HOST = "linux" ] && {
        CHECK_FILES+=("$CX_DEPLOY_BIN/cserver_$HOST" "$CX_DEPLOY_BIN/Ted" "$CX_DEPLOY_ROOT/Cerberus");
    } || {
        CHECK_FILES+=("$CX_DEPLOY_BIN/cserver_$HOST.app/Contents/MacOS/CerberusGame" "$CX_DEPLOY_BIN/Ted.app/Contents/MacOS/Ted" "$CX_DEPLOY_ROOT/Cerberus.app/Contents/MacOS/Cerberus");
    }
    CHECK_COUNT=0
    for i in "${CHECK_FILES[@]}"; do
        do_info "CHECKING FOR $i"
        [ -f "$i" ] && {
            do_success "Found: $i"
            CHECK_COUNT=$((CHECK_COUNT + 1));
        }
    done

    [ $CHECK_COUNT -ne ${#CHECK_FILES[@]} ] && {
        do_error "Deployment file error."
        EXITCODE=1
        return $EXITCODE
    }

    # Make sure that the binaries will execute on the end users machine before compressing.
    # TODO: Check on macOS deployment etc.
    [ $HOST = "linux " ] && {
        for i in "${CHECK_FILES[@]}"; do
            do_info "Setting excute permission for $i"
            excute "chmod +x $i"
        done;
    }
    
    # Get the version from the Cerberus versions text file.
    CX_VERSION="$(head -n 1 "$CX_DEPLOY_ROOT/VERSIONS.TXT" | sed 's/[* ]//g')"

    ARCHIVE_CMD=()

    # If there is a Qt version number, then set the file name with the Qt version.
    [[ $QTVER =~ ^[0-9]+(\.[0-9]+){2,3}$ ]] && {
        FILE_NAME="$DEPLOY/Cerberus_$CX_VERSION-qt$QTVER-$HOST."
    } || {
        FILE_NAME="$DEPLOY/Cerberus_$CX_VERSION-$HOST."
    };

    [ ! "$ARCHIVER" = "tar" ] && {
        # TODO: Add other archive tools.
        do_unknown "Currently only tar is supported.\nSuggest manual deployment with tool of your choice."
        return
    } || {
        FILE_NAME+="tar.gz"
        ARCHIVE_CMD+=("tar" "-czvf" "$FILE_NAME" "Cerberus");
    }
    
    pushd "$DEPLOY"
    if [ -f "$FILE_NAME" ]; then rm -f "$FILE_NAME"; fi
    execute ${ARCHIVE_CMD[@]}
    popd;
}
