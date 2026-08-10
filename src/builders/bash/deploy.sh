#!/bin/bash

# DEPLOYMENT BUILDER FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

#############################################
# BUILD A DEPLOYMENT ARCHIVE
#############################################
# This function requires that git be installed.
# It works by cloning the the current repository, removing any git related items from the clone, and then running the 
# builder script in the clone with the basic set of parameters that were passed. After the cloned builder script has
# finished and control is returned to this function. A archive is generated from the freshly built files in the clone.
do_deploy(){
    EXITCODE=0

    # Set some local variables. Just to make it a bit easier to follow.
    local CX_DEPLOY_ROOT="$DEPLOY/cx_deploy_root"
    local CX_DEPLOY_BUILD="$CX_DEPLOY_ROOT/build"
    local CX_DEPLOY_TARGET="$CX_DEPLOY_BUILD/Cerberus"
    local CX_DEPLOY_BIN="$CX_DEPLOY_TARGET/bin"
    local CX_DEPLOY_SRC="$CX_DEPLOY_TARGET/src"
    
    # Test for a .git folder. If there isn't one, then it must be a normal set of sources files.
    [ ! -d "$CERBERUS_ROOT_DIR/.git" ] && {
        do_error "Delopyment requires that the sources are contained in a git repository."
        return;
    }

    # Only do a deployment build if git is installed.
    execute git --version
    [ $EXITCODE -ne 0 ] && {
        do_error "git is required to build."
        return;
    }

    # Set up the deploy directories
    [ -d "$CX_DEPLOY_ROOT" ] && { rm -rf "$CX_DEPLOY_ROOT"; }
    execute mkdir -p "$CX_DEPLOY_TARGET"
    [ $EXITCODE -ne 0 ] && {
        do_error "Failed to create deploy directory: $CX_DEPLOY_TARGET"
        return;
    }

    ## Test if the git repository is in a clean state. Else issue a error message and return back
    # to the menu. If everything is okay, then clone the git repository.
    STATUS=$(execute git status 2>&1)
    echo "$STATUS"
    if [[ $STATUS == *"nothing to commit, working tree clean"* ]]; then
        execute git -C $CX_DEPLOY_BUILD clone $ROOT Cerberus
        [ $EXITCODE -ne 0 ] && {
            do_error "Failed to clone to directory: $CX_DEPLOY_BUILD"
            return;
        }
    else
        do_error "Repository is not clean. Check for untracked and uncommited files."
        return
    fi

    # Clean up any git stuf that's been copied over.
    # Additional file and directories should be added here.
    local RM_FILES=("$CX_DEPLOY_TARGET/.git" "$CX_DEPLOY_TARGET/.gitignore" "$CX_DEPLOY_TARGET/.gitattributes")
    for r in "${RM_FILES[@]}"; do
        [ -d "$r" ] && {
            rm -rf "$r";
        } || {
            [ -f "$r" ] && { rm -f "$r"; };
        }
    done

    # Generate parameters to pass on to the cloned builder script.
    # Only the basic parameters need to be passed on.
    PARAMS=("$CX_DEPLOY_SRC/builder.sh")
    if [ -n "$QTDIR" ]; then PARAMS+=("-q" "$QTDIR"); fi
    if [ -n "$QTVER" ]; then PARAMS+=("-k" "$QTVER"); fi
    if [ -n "$ARCHIVER" ]; then PARAMS+=("-a" "$ARCHIVER"); fi

    # Linux or macos specific parameters.
    [ $HOST = "linux" ] && {
        if [[ $GEN_ICONS -eq 1 ]]; then PARAMS+=("-i" "$SVG_APP_ICON" "$SVG_MIME_ICON"); fi
        if [ -n "$GCC_VER" ]; then PARAMS+=("-g" "$GCC_VER"); fi
    } || {
        if [ -n "$MACOS_BUNDLE_PREFIX" ]; then PARAMS+=("-p" "$MACOS_BUNDLE_PREFIX"); fi
        if [ -n "$CERT" ]; then PARAMS+=("-c" "$CERT"); fi
    }

    # Start the cloned builder script with the parameter that were passed to the main builder script.
    chmod +x $CX_DEPLOY_SRC/builder.sh
    do_info "Execuiting: 
    ${PARAMS[@]}"
    execute ${PARAMS[@]}

    # Check that the default files have been created before compressing to an archive. NOTE: Qt SDK's are not included in the check.
    CHECK_FILES=("$CX_DEPLOY_BIN/transcc_$HOST" "$CX_DEPLOY_BIN/makedocs_$HOST")
    [ $HOST = "linux" ] && {
        CHECK_FILES+=("$CX_DEPLOY_BIN/cserver_$HOST" "$CX_DEPLOY_BIN/Ted" "$CX_DEPLOY_TARGET/Cerberus");
    } || {
        CHECK_FILES+=("$CX_DEPLOY_BIN/cserver_$HOST.app/Contents/MacOS/CerberusGame" "$CX_DEPLOY_BIN/Ted.app/Contents/MacOS/Ted" "$CX_DEPLOY_TARGET/Cerberus.app/Contents/MacOS/Cerberus");
    }
    CHECK_COUNT=0
    for i in "${CHECK_FILES[@]}"; do
        do_info "CHECKING FOR $i"
        [ -f "$i" ] && {
            do_success "Found: $i"
            CHECK_COUNT=$((CHECK_COUNT + 1));
        } || {
            do_error "Not found: $i"
        }
    done

    # Only compress if the file count match that of the array.
    [ $CHECK_COUNT -ne ${#CHECK_FILES[@]} ] && {
        do_error "Deployment file error."
        EXITCODE=1
        return $EXITCODE
    }

    # Make sure that the binaries will execute permissions before compressing.
    # TODO: Check the permision requirements for macos.
    [ $HOST = "linux " ] && {
        for i in "${CHECK_FILES[@]}"; do
            do_info "Setting excute permission for $i"
            excute "chmod +x $i"
        done;
    }
    
   # Get what should be the first line in the VERSION.TXT file to retrieve the version number.
    while IFS= read -r line; do
        [ "${line:0:1}" = "*" ] && {
            CX_VERSION=$(echo "$line" | sed 's/[* ]//g')
            break;
        }
    done < "$CX_DEPLOY_TARGET/VERSIONS.TXT"

    ARCHIVE_CMD=()

    # If there is a Qt version number, this mean that there are Qt libraries to install.
    # Then set the file name with the Qt version.
    [[ $QTVER =~ ^[0-9]+(\.[0-9]+){2,3}$ ]] && {
        FILE_NAME="$CX_DEPLOY_ROOT/Cerberus_$CX_VERSION-qt$QTVER-$HOST."
    } || {
        FILE_NAME="$CX_DEPLOY_ROOT/Cerberus_$CX_VERSION-$HOST."
    };

    # Select the command to gnerate an archive package
    # NOTE: As Apple requires a specific way to do code signing. The resulting archive would be redundent. 
    case "$ARCHIVER" in
        "tar")
            FILE_NAME+="tar.gz"
            ARCHIVE_CMD+=("tar" "-czvf" "$FILE_NAME" "-C" "$CX_DEPLOY_BUILD" "Cerberus");
        ;;
        "hdiutil")
            FILE_NAME+="dmg"
            ARCHIVE_CMD+=("hdiutil" "create" "-volname" "CerberusX-$CX_VERSION-qt$QTVER" "-srcfolder" "$CX_DEPLOY_BUILD" "-ov" "-format" "UDBZ" "$FILE_NAME")
        ;;
        *)
            do_unknown "Currently only tar and hdiutil are supported.\nSuggest manual deployment with tool of your choice."
            ARCHIVER=""
    esac

    # Generate archive files
    [ $HOST = "linux" ] && {
        # If an archiver has been selected, then build the package.
        [ -z "$ARCHIVER" ] || {
            pushd "$CX_DEPLOY_ROOT"
            if [ -f "$FILE_NAME" ]; then rm -f "$FILE_NAME"; fi
            execute ${ARCHIVE_CMD[@]}
            popd;
        };
    } || {
        #[ $CODESIGN -eq 1 ] && {
        #    #pushd "$CX_DEPLOY_ROOT"
        #    do_codesign "$CX_DEPLOY_TARGET" "$CX_DEPLOY_BIN" "$ARCHIVE_CMD"
        #    #popd;
        ##} || {
            # If an archiver has been selected, then build the package.
            [ -z "$ARCHIVER" ] || {
            #    pushd "$CX_DEPLOY_ROOT"
            #    if [ -f "$FILE_NAME" ]; then rm -f "$FILE_NAME"; fi
            #    execute ${ARCHIVE_CMD[@]}
            #    popd;
                do_info "On macOS. Cerberus needs to be codesigned\nCurrently this has to be don manually."
                do_info "Once done. you can run the command below to generate a dmg image."
                do_info "${ARCHIVE_CMD[@]}"
            };
        #};
    }
}
