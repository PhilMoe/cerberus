# TOOLS FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

# Replace the first key/value pair that matched the key.
function do_set_config_var([string]$_path, [string]$_key, [string]$_value) {
    $edits = (Get-Content -Path $_path) |
    ForEach-Object {
        if ($_ -match '^' + $_key + '=\".*\"' ) {
            $_ -replace "($_key=)`".*`"", "`$1`"$_value`""
        }
        else {
            $_
        }
    }
    
    $edits -join "`n" | Out-File $_path -Encoding ascii

    # Running git with add stops it being scheduled to changes
    $global:EXITCODE = 0
    execute "git" "add $BIN\config.winnt.txt"
}

# Gets the first key=value pair from a config file that matches the key.
function do_get_config_var([string]$_path, [string]$_key) {
    $value = switch -Regex -File $_path { "^$_key=`"(.*)`"" { $Matches[1]; break } }
    return $value
}

# Build the transcc tool
function do_transcc() {

    do_info "BUILDING TransCC"

    # Create a variable to where the project files are located.
    [string]$cpptool_dir = "$SRC\transcc\transcc.build\cpptool"
    [string]$build_dir = ""

    # Check for an exisiting transcc
    if (Test-Path("$BIN\transcc_winnt.exe")) { Remove-Item "$BIN\transcc_winnt.exe" }

    # Call the build tool depending on the toolchain selected.
    if (($msbuild -eq $true)-and($global:MSVC_SELECTED_IDX -ge 0)) {
        $build_dir = "$cpptool_dir\msvc\Release64"
        execute "msbuild" "/p:OutDir=`"$BIN\`" /p:TargetName=`"transcc_winnt`" /p:Configuration=Release64 /p:Platform=x64 $cpptool_dir\msvc\msvc.sln"
    } else {
        $build_dir = "$cpptool_dir\gcc_winnt\build"
        New-Item "$build_dir" -Type Directory -Force
        Push-Location
        Set-Location "$cpptool_dir\gcc_winnt"
        execute "mingw32-make" "CCOPTS=`"-O3 -DNDEBUG -Wno-free-nonheap-object`" OUT_PATH=`"$BIN`" OUT=`"transcc_winnt`" BUILD_DIR=`"$build_dir`""
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

# Build the CServer tool.
function do_cserver() {
    
    # Call transcc
    transcc "CServer" "Desktop_Game" "cserver"
    
    if ($global:EXITCODE -ne 0) {
        do_error "$global:MESSAGE`n"
        return
    }

    # Set the relase directory based on the toolchain
    [string]$release_dir = "$SRC\cserver\cserver.build\glfw3\"
    if ($msbuild -eq $true) {
        $release_dir += "msvc\Release64"
    } else {
        $release_dir += "gcc_winnt\Release64"
    }

    # Remove the old version before moving the new one into the bin directory.
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

# Build the makedocs tool.
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

# Build the launcher tool
function do_launcher() {
    do_info "BUILDING Launcher"

    # Create a variable to where the project files are located.
    [string]$project_dir = "$SRC\launcher\winnt"
    [string]$build_dir = "$project_dir\Release64"
    [string]$icon = "$SRC\launcher\cerberus.ico"

    if (Test-Path("$ROOT\Cerberus.exe")) { Remove-Item "$ROOT\Cerberus.exe" }

    # Call the build tool depending on the toolchain selected.
    if (($msbuild -eq $true)-and($global:MSVC_SELECTED_IDX -ge 0)) {
        execute "msbuild" "/p:OutDir=`"$ROOT\`" /p:ApplicationIcon=`"$icon`" /p:TargetName=`"Cerberus`" /p:Configuration=Release64 /p:Platform=x64 $project_dir\msvc.sln"
        clean_build "$build_dir" $false

        if ($global:EXITCODE -ne 0) {
            do_error "$global:MESSAGE`n"
            return
        }
    } else {
        if (Test-Path("$SRC\launcher\res.o")) { Remove-Item "$SRC\launcher\res.o" }

        execute "windres.exe" "`"$SRC\launcher\resource.rc`" -O coff -o `"$SRC\launcher\res.o`""
        if ($global:EXITCODE -ne 0) {
            do_error "$global:MESSAGE`n"
            return
        }

        execute "g++.exe" "-Os -DNDEBUG -o `"$ROOT\Cerberus.exe`" `"$project_dir\launcher.cpp`" `"$SRC\launcher\res.o`" -ladvapi32 -s"
        if ($global:EXITCODE -ne 0) {
            do_error "$global:MESSAGE`n"
            return
        }
    }

    do_success "BUILD SUCCESSFUL`n"
    $global:EXITCODE = 0
}

# Build the IDE Ted
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
    execute "$qtdir\bin\qmake.exe" "-config release $SRC\ted\ted.pro"
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

# Function to do build all
function do_all() {
    do_header "`n====== BUILDING ALL TOOLS ======"
    do_transcc
    if ($global:EXITCODE -eq 0) { do_cserver }
    if ($global:EXITCODE -eq 0) { do_makedocs }
    if ($global:EXITCODE -eq 0) { do_launcher }

    if ($global:EXITCODE -eq 0) {
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

# Function to remove all previously built files.
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
    If(Test-Path("$BIN\Ted")) { Remove-Item "$BIN\Ted" -Force }
    If(Test-Path("$BIN\Ted.exe")) { Remove-Item "$BIN\Ted.exe" -Force }

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