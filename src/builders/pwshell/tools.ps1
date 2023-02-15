# TOOLS FUNCTIONS VERSION
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

    # Check for an exisiting transcc
    if (Test-Path("$BIN\transcc_winnt.exe")) { Remove-Item "$BIN\transcc_winnt.exe" }

    execute "g++.exe" "-O3 -DNDEBUG -o `"$BIN\transcc_winnt`" `"$SRC\transcc\transcc.build\cpptool\main.cpp`" -lpthread -s"
    If ($global:EXITCODE -ne 0) {
        do_error "$global:MESSAGE`n"
        return
    }

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

    # Remove the old version before moving the new one into the bin directory.
    if (Test-Path("$BIN\cserver_winnt.exe")) { Remove-Item "$BIN\cserver_winnt.exe" }
    Move-Item "$SRC\cserver\cserver.build\glfw3\gcc_winnt\Release64\CerberusGame.exe" "$BIN\cserver_winnt.exe"

    # Check for the data directory and copy over additional files.
    if (-not(Test-Path("$BIN\data"))) { Move-Item "$SRC\cserver\cserver.build\glfw3\gcc_winnt\Release64\data" "$BIN\data" }
    if (-not(Test-Path("$BIN\openal32.dll"))) { Move-Item "$SRC\cserver\cserver.build\glfw3\gcc_winnt\Release64\openal32.dll" "$BIN\OpenAL32.dll" }
    if (-not(Test-Path("$BIN\openal32_COPYING"))) { Move-Item "$SRC\cserver\cserver.build\glfw3\gcc_winnt\Release64\openal32_COPYING" "$BIN\openal32_COPYING" }
    if (-not(Test-Path("$BIN\openal32_LICENCE"))) { Move-Item "$SRC\cserver\cserver.build\glfw3\gcc_winnt\Release64\openal32_LICENCE" "$BIN\openal32_LICENCE" }
    
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

    if (Test-Path("$ROOT\Cerberus.exe")) { Remove-Item "$ROOT\Cerberus.exe" }
    if (Test-Path("$SRC\launcher\res.o")) { Remove-Item "$SRC\launcher\res.o" }

    execute "windres.exe" "`"$SRC\launcher\resource.rc`" -O coff -o `"$SRC\launcher\res.o`""
    if ($global:EXITCODE -ne 0) {
        do_error "$global:MESSAGE`n"
        return
    }

    execute "g++.exe" "-Os -DNDEBUG -o `"$ROOT\Cerberus.exe`" `"$SRC\launcher\codeblocks\launcher.cpp`" `"$SRC\launcher\res.o`" -ladvapi32 -s"
    if ($global:EXITCODE -ne 0) {
        do_error "$global:MESSAGE`n"
        return
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