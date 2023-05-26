# TOOL BUILDER FUNCTIONS VERSION
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.
Function do_deploy() {
    $global:EXITCODE = 0

    [String]$cx_deploy_root = "$deploy\Cerberus"
    [String]$cx_deploy_bin = "$cx_deploy_root\bin"
    [String]$cx_deploy_src = "$cx_deploy_root\src"

    # Test for a .git folder
    if (-not(Test-Path("$ROOT\.git"))) {
        do_error "Delopyment requires that the sources are contained in a git repository."
        return
    }

    # Test for a git
    execute "git" "--version" | Out-Null
    if ($global:EXITCODE -ne 0) {
        do_error "git is required to build."
        return;
    }

    # Set up the deploy directory
    If (Test-Path("$cx_deploy_root")) { Remove-Item -Path "$cx_deploy_root" -Force -Recurse }
    if (-not(New-Item "$cx_deploy_root" -Type Directory)) {
        do_error "Failed to create deploy directory: $cx_deploy_root"
        return
    }

    # Test if the git repository is in a clean state. And if clean, clone it.
    execute "git" "status"  | Out-Null
    if (-not($global:MESSAGE.Contains("nothing to commit, working tree clean"))) {
        do_info "$global:MESSAGE"
        do_error "Repository is not clean. Check for untracked and uncommited files."
        return
    }

    execute "git" "-C `"$deploy`" clone `"$ROOT`" Cerberus" | Out-Null
    if ($global:EXITCODE -ne 0) {
        do_info "$global:MESSAGE"
        do_error "Failed to clone to directory: $deploy"
        return
    }

    # Clean up any git stuf that's been copied over.
    [string[]]$files=@("$cx_deploy_root\.git","$cx_deploy_root\.gitignore","$cx_deploy_root\.gitattributes")
    $files | ForEach-Object {
        if (Test-Path($_)) { Remove-Item -Force -Recurse $_ | Out-Null }
    }

    # Generate parameters to pass on.
    [string]$buildtype = "mingw"
    [string]$param = "-q `"$qtsdk`" -c `"$mingw`" -i `"$vsinstall`""
    if (-not([string]::IsNullOrEmpty($qtkit))) { $param += " -k $qtkit" }
    if (-not([string]::IsNullOrEmpty($vsver))) { $param += " -y $vsver" }
    if ($msbuild -eq $true) {
        $param += " -b"
        $buildtype = "msvc"
    }
    if ($stdout -eq $true) { $param += " -s" }

    # Start a fresh build script with all the parameters needed.
    execute "$DEPLOY/Cerberus/src/builder.ps1" "$param" | Out-Null
    if ($global:EXITCODE -eq 0) {
        do_error "Failed to build Cerberus-X"
        return
    }

    # Check that the default files have been created before compressing to an archive. NOTE: Qt SDK's are not included in the check.
    [int]$filecount=0
    [string[]]$filelist=@("$cx_deploy_bin\transcc_winnt.exe","$cx_deploy_bin\makedocs_winnt.exe","$cx_deploy_bin\cserver_winnt.exe","$cx_deploy_bin\Ted.exe","$CX_deploy_root\Cerberus.exe")
    $filelist | ForEach-Object {
        do_info "CHECKING FOR: $_"
        if (Test-Path($_)) {
            $filecount +=1
            do_success "Found: $_"
        }
    }

    # Only compress if the file cout match that of the array.
    if ($filecount -ne $filelist.Count) {
        do_error "Deployment file error."
        $global:EXITCODE=1
        return
    }

    # Get what should be the first line in the VERSION.TXT file to retrieve the version number.
    [string]$CX_VERSION = $(Get-Content "$cx_deploy_root\VERSIONS.TXT" -First 1) -Replace '[*v ]', ''
    
    # Clear out any old compressed files and recompress.
    if (Test-Path("$deploy\Cerberus_$CX_VERSION-$buildtype-qt$qtkit-winnt.zip")) { Remove-Item -Force "$deploy\Cerberus_$CX_VERSION-$buildtype-qt$qtkit-winnt.zip" }
    Compress-Archive -Path "$deploy\Cerberus" -DestinationPath "$deploy\Cerberus_$CX_VERSION-$buildtype-qt$qtkit-winnt.zip" -CompressionLevel Optimal
}
