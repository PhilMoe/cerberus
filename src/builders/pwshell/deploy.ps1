# DEPLOYMENT BUILDER FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILDER TOOL.

#############################################
# BUILD A DEPLOYMENT ARCHIVE
#############################################
# This function requires that git be installed.
# It works by cloning the the current repository, removing any git related items from the clone, and then running the 
# builder script in the clone with the basic set of parameters that were passed. After the cloned builder script has
# finished and control is returned to this function. A archive is generated from the freshly built files in the clone.
Function do_deploy() {
    $global:EXITCODE = 0

    # Set some local variables. Just to make it a bit easier to follow.
    [String]$cx_deploy_root = "$deploy\cx_deploy_root"
    [string]$cx_deploy_build = "$cx_deploy_root\build"
    [string]$cx_deploy_target = "$cx_deploy_build\Cerberus"
    [String]$cx_deploy_bin = "$cx_deploy_target\bin"
    [String]$cx_deploy_src = "$cx_deploy_target\src"

    # Test for a .git folder. If there isn't one, then it must be a normal set of sources files.
    if (-not(Test-Path("$ROOT\.git"))) {
        do_error "Deployment requires that the sources contain in a git repository."
        return
    }

    # Only do a deployment build if git is installed.
    if(-not($global:GIT_INSTALLED)) {
        do_error "git is required to build a deployment package."
        return
    }

    # Restore the config.winnt.txt file back from the cached version.
    # As this function will return back to the menu. Any modifications to the config variables will need to be restored
    # for the builder script to build any of the other tools.
    execute "git" "restore $BIN/config.winnt.txt"

    # Set up the deploy directories
    If (Test-Path("$cx_deploy_root")) { Remove-Item -Path "$cx_deploy_root" -Force -Recurse }
    if (-not(New-Item "$cx_deploy_build" -Type Directory -Force)) {
        do_error "Failed to create deploy directory: $cx_deploy_build"
        return
    }

    # Test if the git repository is in a clean state. Else issue a error message and return back
    # to the menu.
    execute "git" "status"  | Out-Null
    if (-not($global:MESSAGE.Contains("nothing to commit, working tree clean"))) {
        do_info "$global:MESSAGE"
        do_error "Repository is not clean. Check for untracked and uncommitted files."
        return
    }

    # Clone the local work repository to where the deployment is to be builts.
    execute "git" "-C `"$cx_deploy_build`" clone `"$ROOT`" Cerberus" | Out-Null
    if ($global:EXITCODE -ne 0) {
        do_info "$global:MESSAGE"
        do_error "Failed to clone to directory: $deploy`nIf the $cx_deploy_build directory exists, then delete it manually."
        return
    }

    # Clean up any git related items in the clone.
    [string[]]$files=@("$cx_deploy_target\.git","$cx_deploy_target\.gitignore","$cx_deploy_target\.gitattributes")
    $files | ForEach-Object {
        if (Test-Path($_)) { Remove-Item -Force -Recurse $_ | Out-Null }
    }

    # Generate parameters to pass on to the cloned builder script.
    # Only the basic parameters need to be passed on.
    [string]$buildtype = "mingw"    # Used from part of the final archive name
    [string]$param = "-q `"$qtsdk`" -c `"$mingw`" -i `"$vsinstall`" -w `"$winsdk`" -p `"$platformtoolset`""
    if (-not([string]::IsNullOrEmpty($qtkit))) { $param += " -k $qtkit" }
    if (-not([string]::IsNullOrEmpty($vsver))) { $param += " -y $vsver" }
    if ($msbuild -eq $true) {
        $param += " -b"
        $buildtype = "msvc"
    }
    if ($stdout -eq $true) { $param += " -s" }

    # Start the cloned builder script with the parameter that were passed to the main builder script.
    execute "$cx_deploy_src/builder.ps1" "$param" | Out-Null
    if ($global:EXITCODE -ne 0) {
        do_error "Failed deployment build of Cerberus-X`nIf the $cx_deploy_build directory exists, then delete it manually."
        return
    }

    # Check that the default files have been created before compressing to an archive. NOTE: Qt SDK's are not included in the check.
    [int]$filecount=0
    [string[]]$filelist=@("$cx_deploy_bin\transcc_winnt.exe","$cx_deploy_bin\makedocs_winnt.exe","$cx_deploy_bin\cserver_winnt.exe","$cx_deploy_bin\Ted.exe","$cx_deploy_target\Cerberus.exe")
    $filelist | ForEach-Object {
        do_info "CHECKING FOR: $_"
        if (Test-Path($_)) {
            $filecount +=1
            do_success "Found: $_"
        }
    }

    # Only compress if the file count match that of the array.
    if ($filecount -ne $filelist.Count) {
        do_error "Deployment file error.`nIf the $cx_deploy_build directory exists, then delete it manually."
        $global:EXITCODE=1
        return
    }

    # Get what should be the first line in the VERSION.TXT file to retrieve the version number.
    [string]$cx_version = $(Get-Content "$cx_deploy_target\VERSIONS.TXT") | ForEach-Object {
        if($_.ToString().Substring(0,1)) {
           $_.ToString() -Replace '[*v ]', ''
        }
    } | Select-Object -First 1
    
    # Clear out any old compressed files and recompress.
    if (Test-Path("$cx_deploy_root\Cerberus_$cx_version-$buildtype-qt$qtkit-winnt.zip")) { Remove-Item -Force "$cx_deploy_root\Cerberus_$cx_version-$buildtype-qt$qtkit-winnt.zip" }
    Compress-Archive -Path "$cx_deploy_target" -DestinationPath "$cx_deploy_root\Cerberus_$cx_version-$buildtype-qt$qtkit-winnt.zip" -CompressionLevel Optimal

    # As the chances are that the script will not be exiting, then the config.winnt.txt file will need the MinGW and MSBUILD values to be reset.
    do_set_config_var "$BIN/config.winnt.txt" "MINGW_PATH" "$global:MINGW_STORE"
    do_set_config_var "$BIN/config.winnt.txt" "MSBUILD_PATH" "$($global:MSVC_INSTALLS[$global:MSVC_SELECTED_IDX])\MSBuild\Current\Bin\MSBuild.exe"
}
