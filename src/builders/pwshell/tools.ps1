# TOOLS FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

#######################################################################################################
#   Build transcc, cserver, makedocs, launcher and the IDE Ted
#######################################################################################################

# TRANSCC
# This function basically just builds from the c++ sources.
# Any new or update version of the transcc c++ source should replace the ones in the src/transcc/transcc.build directory. 
function do_transcc() {

    do_info "BUILDING TransCC"

    # Create a variable to where the project files are located.
    [string]$cpptool_dir = "$SRC\transcc\transcc.build\cpptool"
    [string]$build_dir = ""

    # Check for an exisiting transcc and remove it.
    if (Test-Path("$BIN\transcc_winnt.exe")) { Remove-Item "$BIN\transcc_winnt.exe" }

    # Build transcc from the c++ source files using the tool chain selected. That is if it's installed.
    if (($global:msbuild -eq $true)-and($global:MSVC_SELECTED_IDX -ge 0)) {

        [string[]]$arguments = (
            "/p:OutDir=`"$BIN\`"",
            "/p:TargetName=`"transcc_winnt`"",
            "/p:Configuration=Release$msize",
            "/p:PlatformToolset=$platformtoolset",
            "/p:WindowsTargetPlatformVersion=$winsdk"
        )

        if ($msize -eq "64") {
            $arguments += (
                "/p:Platform=x64"
            )
        } else {
            $arguments += (
                "/p:Platform=x86"
            )
        }

        $arguments += ( "$cpptool_dir\msvc\msvc.sln" )

        $build_dir = "$cpptool_dir\msvc\Release$msize"
        execute "msbuild" $arguments
    } else {

        $build_dir = "$cpptool_dir\gcc_winnt\build"

        [string[]]$arguments = (
            "OUT_PATH=`"$BIN`"",
            "OUT=`"transcc_winnt`"",
            "BUILD_DIR=`"$build_dir`""
        )

        [string]$local:arch = "-m64"
        if ($msize -eq "32") { $local:arch = "-m32" }

        $arguments += (
            "CC_OPTS=`"-O3 -DNDEBUG $local:arch -Wno-free-nonheap-object`""
        )

        if ($mingwstatic -eq $true) {
            $arguments += (
                "STATIC_LINK=1"
            )
        } else {
             
        }

        New-Item "$build_dir" -Type Directory -Force
        Push-Location
        Set-Location "$cpptool_dir\gcc_winnt"
        execute "mingw32-make" $arguments
        Pop-Location

    }

    if ($global:EXITCODE -ne 0) {
        do_error "$global:MESSAGE`n"
        return
    }

    clean_build "$build_dir" $false

    do_success "BUILD SUCCESSFUL`n"
    $global:EXITCODE = 0
}

# CSERVER TOOL
function do_cserver() {
    
    # Call transcc to build cserver
    transcc "CServer" "Desktop_Game" "cserver"  
    if ($global:EXITCODE -ne 0) {
        do_error "$global:MESSAGE`n"
        return
    }

    # Set the relase directory based on the toolchain
    [string]$release_dir = "$SRC\cserver\cserver.build\glfw3\"
    if ($global:msbuild -eq $true) {
        $release_dir += "msvc\Release$msize"
    } else {
        $release_dir += "gcc_winnt\Release$msize"
    }

    # Remove the old version before moving the new one into the cerberus bin directory.
    if (Test-Path("$BIN\cserver_winnt.exe")) { Remove-Item "$BIN\cserver_winnt.exe" }
    Move-Item "$release_dir\CerberusGame.exe" "$BIN\cserver_winnt.exe"

    # Check for the data directory and copy over additional files.
    if (-not(Test-Path("$BIN\data"))) { Move-Item "$release_dir\data" "$BIN\data" }
    if (-not(Test-Path("$BIN\openal32.dll"))) { Move-Item "$release_dir\openal32.dll" "$BIN\OpenAL32.dll" }
    if (-not(Test-Path("$BIN\openal32_COPYING"))) { Move-Item "$release_dir\openal32_COPYING" "$BIN\openal32_COPYING" }
    if (-not(Test-Path("$BIN\openal32_LICENCE"))) { Move-Item "$release_dir\openal32_LICENCE" "$BIN\openal32_LICENCE" }
    
    clean_build "cserver"

    do_success "BUILD SUCCESSFUL`n"
}

# MAKEDOCS TOOL
function do_makedocs() {

    # Call transcc
    transcc "MakeDocs" "C++_Tool" "makedocs" "0"
    if ($global:EXITCODE -ne 0) {
        do_error "$global:MESSAGE`n"
        return
    }

    if (Test-Path("$BIN\makedocs_winnt.exe")) { Remove-Item "$BIN\makedocs_winnt.exe" }
    move-item "$SRC\makedocs\makedocs.build\cpptool\main_winnt.exe" "$BIN\makedocs_winnt.exe"

    clean_build "makedocs"

    do_success "BUILD SUCCESSFUL`n"
    $global:EXITCODE = 0
}

# LAUNCHER TOOL
# Due to some limitations with using the Cerberus launcher source file. The launcher is built from a custom c++ source file.
function do_launcher() {
    do_info "BUILDING Launcher"

    # Create a variable to where the project files are located.
    [string]$parent_dir = "$SRC\launcher\"
    [string]$project_dir = "$SRC\launcher\winnt"
    [string]$build_dir = "$project_dir\Release$msize"
    [string]$icon = "$SRC\launcher\cerberus.ico"
    
    # Remove the previous launcher if detected.
    if (Test-Path("$ROOT\Cerberus.exe")) { Remove-Item "$ROOT\Cerberus.exe" }

    if (($global:msbuild -eq $true)-and($global:MSVC_SELECTED_IDX -ge 0)) {

        [string[]]$arguments = (
            "/p:OutDir=`"$ROOT\`"",
            "/p:ApplicationIcon=`"$icon`"",
            "/p:TargetName=`"Cerberus`"",
            "/p:Configuration=Release$msize",
            "/p:PlatformToolset=$platformtoolset",
            "/p:WindowsTargetPlatformVersion=$winsdk"
        )

        if($msize -eq "64") {
            $arguments += (
                "/p:Platform=x64"
            )
        } else {
            $arguments += (
                "/p:Platform=x86"
            )
        }

        $arguments += (
            "$project_dir\msvc.sln"
        )
        execute "msbuild" $arguments
        clean_build "$build_dir" $false
        clean_build "$SRC\launcher\resource.o" $false

        if ($global:EXITCODE -ne 0) {
            do_error "$global:MESSAGE`n"
            return
        }
    } else {

        [string]$local:srcDir = $(Get-Location)
        
        [string]$local:arch = "-m64"
        if ($msize -eq "32") { $local:arch = "-m32" }

        $arguments = (
            "-f",
            "`"$project_dir\Makefile`"",
            "BUILD_DIR=`"$build_dir`"",
            "OUT_PATH=`"$ROOT`"",
            "OUT=`"Cerberus.exe`"",
            "CC_OPTS=`"-Os -DNDEBUG $local:arch -s`"",
            "MSIZE=$msize"
        )

        if ($mingwstatic -eq $true) {
            $arguments += (
                "STATIC_LINK=1"
            )
        }

        New-Item -ItemType Directory -Force -Path "$build_dir"
        Set-Location "$project_dir"
        execute "mingw32-make.exe" $arguments
        if ($global:EXITCODE -ne 0) {
            do_error "$global:MESSAGE`n"
            return
        }
        clean_build "$build_dir" $false
        Set-Location $local:srcDir
    }

    do_success "BUILD SUCCESSFUL`n"
    $global:EXITCODE = 0
}

# IDE TED
function do_ted() {
    do_info "BUILDING THE IDE TED"

    # If present, remove any previous build directory before creating a new one.
    if (Test-Path("$SRC\build-ted-Desktop-Release")) { Remove-Item -Force -Recurse "$SRC\build-ted-Desktop-Release" }
    New-Item "$SRC\build-ted-Desktop-Release" -Type Directory

    # Store the current directory and switch to the new build directory
    Push-Location
    Set-Location "$SRC\build-ted-Desktop-Release"

    # Get the select path for the Qt KIT chosen and run qmake to generate the build files.
    [string]$qtdir = $global:QT_INSTALLS[$global:QT_SELECTED_IDX]

    [string[]]$arguments = (
        "-config",
        "release",
        "$SRC\ted\ted.pro"
    )
    execute "$qtdir\bin\qmake.exe" $arguments
    if ($global:EXITCODE -ne 0) {
        Pop-Location
        Remove-Item -Force -Recurse "$SRC\build-ted-Desktop-Release"
        do_error "$global:MESSAGE"
        return
    }

    # Now run nmake to actuall build the IDE.
    execute "nmake" "-f Makefile.Release"
    if ($global:EXITCODE -ne 0) {
        Pop-Location
        Remove-Item -Force -Recurse "$SRC\build-ted-Desktop-Release"
        do_error "$global:MESSAGE"
        return
    }

    Pop-Location
    Remove-Item -Force -Recurse "$SRC\build-ted-Desktop-Release"

    do_success "BUILD SUCCESSFUL`n"
    $global:EXITCODE = 0
}

# BUILD ALL
# Builds all the above tools.
function do_all() {
    do_header "`n====== BUILDING ALL TOOLS ======"
    do_transcc
    if ($global:EXITCODE -eq 0) { do_cserver }
    if ($global:EXITCODE -eq 0) { do_makedocs }
    if ($global:EXITCODE -eq 0) { do_launcher }

    if ($global:EXITCODE -eq 0) {

        # Check that there is an MSVC tool chain and Qt Kit present before building. Else issue warning message and skip building. 
        if (($global:MSVC_INSTALLS.Count -gt 0) -and ($global:QT_INSTALLS.Count -gt 0)) {
            do_ted
        }
        else {
            [string]$msg = "The IDE Ted requires "
            if (($global:QT_SELECTED_IDX -lt 0) -and ($global:MSVC_SELECTED_IDX -lt 0)) {
                $msg += "both a Visual Studio MSVC and a Qt SDK kit"
            }
            elseif ($global:QT_SELECTED_IDX -lt 0) {
                $msg += "that a Qt SDK kit"
            }
            elseif ($global:MSVC_SELECTED_IDX -lt 0) {
                $msg += "that a version of Visual Studio MSVC"
            }
            $msg += " be installed."
            do_error "$msg"
        }
    }
}

################################################
# CLEAN THE WORK REPOSITORY OF BUILT FILES
################################################
function do_clearbuilds() {
    do_info "CLEARING OUT PREVIOUS BUILDS"

    # Remove all macOS applications. Ted and CServer
    Remove-Item "$BIN\*.app" -Recurse -Force

    # Remove transcc linux, macos winnt
    Remove-Item "$BIN\transcc_*" -Force

    # Remove the launchers linux, macOS and winnt.
    if(Test-Path("$ROOT\Cerberus")) { Remove-Item "$ROOT\Cerberus" -Force }
    if(Test-Path("$ROOT\Cerberus.exe")) { Remove-Item "$ROOT\Cerberus.exe" -Force }
    if(Test-Path("$ROOT\Cerberus.app")) { Remove-Item "$ROOT\Cerberus.app" -Recurse -Force }
    Remove-Item "$ROOT\*.desktop"

    # Remove CServer linux and winnt.
    Remove-Item "$BIN\cserver_*" -Force

    # Remove makedocs linux and winnt.
    Remove-Item "$BIN\makedocs_*" -Force

    # Remove Ted linus and winnt
    if(Test-Path("$BIN\Ted")) { Remove-Item "$BIN\Ted" -Force }
    if(Test-Path("$BIN\Ted.exe")) { Remove-Item "$BIN\Ted.exe" -Force }
    if(Test-Path("$BIN\platforms")) { Remove-Item "$BIN\platforms" -Recurse -Force }
    if(Test-Path("$BIN\qml")) { Remove-Item "$BIN\qml" -Recurse -Force }

    # Remove Qt Linux support files and directories.
    Remove-Item "$BIN\lib*" -Recurse -Force
    if(Test-Path("$BIN\plugins")) { Remove-Item "$BIN\plugins" -Recurse -Force }
    if(Test-Path("$BIN\resources")) { Remove-Item "$BIN\resources" -Recurse -Force }
    if(Test-Path("$BIN\translations")) { Remove-Item "$BIN\translations" -Recurse -Force }

    # Remove Qt WinNT support files and directories.
    if(Test-Path("$BIN\platforms")) { Remove-Item "$BIN\platforms" -Recurse -Force }
    if(Test-Path("$BIN\qt.conf")) { Remove-Item "$BIN\qt.conf" -Force }
    Remove-Item "$BIN\*.dll" -Force
    Remove-Item "$BIN\*.exe" -Force
    Remove-Item "$BIN\*.ilk" -Force
    Remove-Item "$BIN\*.pdb" -Force
    Remove-Item "$BIN\openal32_*" -Force
}
