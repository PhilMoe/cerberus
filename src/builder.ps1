# MAIN MS WINDOWS POWERSHELL SCRIPT FOR BUILDING CERBERUS X TOOLS

# NOTES:
# This script requires that the execution policy for the current user be set to unrestricted.
# Open power shell as administrator and use:
#    get-executionpolicy -list
#    set-executionpolicy -scope currentuser unrestricted
# If the file is still blocked use:
#    unblock-file -path "full_path_to_this_script"
# You should reset the execution policy back to it's original state e.g.:
#    set-executionpolicy -scope currentuser undefined
#
Param(
    [Alias("q")][string]$qtsdk = "C:\Qt",
    [Alias("v")][string]$qtver = "",
    [Alias("c")][string]$mingw = "C:\TDM-GCC-64",
    [Alias("y")][string]$vsver = "",
    [Alias("i")][string]$vsinstall = "$([System.Environment]::GetEnvironmentVariable('ProgramFiles(x86)'))\Microsoft Visual Studio\Installer",
    [Alias("m")][switch]$showmenu = $false,
    [Alias("h")][switch]$help = $false
)

[string]$SCRIPT_VER = "1.0.3"

Clear-Host

# Basic variable for common Cerberus directories.
[string]$SRC = "$PSScriptRoot"
[string]$ROOT = Resolve-Path "$SRC\.."
[string]$BIN = "$ROOT\bin"
[string]$MODULES = "$SRC\builders\pwshell"
[string]$MINGW_STORE = ""

# Import the additional scripts that contain core functions
."$MODULES\common.ps1"
."$MODULES\thirdparty.ps1"
."$MODULES\tools.ps1"

if ($help -eq $true) {
    do_info "CERBERUS X TOOLS VERSION 1.0.0"
    Write-Host "USEAGE: ./builder.ps1 [options]`n`t{-m|-showmenu}`t`t`t`t`t- run in menu mode.`n`t{-q|-qtsdk} `"QT_DIR_PATH`"`t`t`t- Set root Qt SDK directory."
    Write-Host "`t{-v|-qtver} `"DOT.VERSION.NUMBER`"`t`t- Set Qt SDK version.`n`t{-vsi|-vsinstall} `"VISUAL_INSTALLER_PATH`"`t- Set MS Visual Installer directory."
    Write-Host "`t{-y|-vsver} `"PRODUCT_YEAR`"`t`t`t- Set Visual Studio product year`n`t{-c|-mingw} `"MINGW_DIR`"`t`t`t`t- Set MiGW root directory.`n`t{-h|-help}`t`t`t`t`t- Show this quick help`n"
    Write-Host "EXAMPLE:`n`te.g: ./builder.ps1 -qtsdk C:\Qt -v 5.14.0"
    Write-Host "`te.g: ./builder.ps1 -qtsdk C:\Qt -qtver 5.14.0 -vsver `"2017`" -showmenu"
    exit 1
}

do_header "===== Cerberus X Tool Builder Version $SCRIPT_VER ====="

# The base requirments for building Cerberus is that a GCC compiler is present.
# If the test fails, then exit the script. The first thing
do_mingw($mingw)
if ($global:EXITCODE -ne 0) {
    do_error("NO MINGW COMPILER PRESENT")
    exit 1
}

# Store the current config.winnt.txt MINGW_PATH before updating it with the one the builder script is to use.
$MINGW_STORE = do_get_config_var "$BIN/config.winnt.txt" "MINGW_PATH"
do_set_config_var "$BIN/config.winnt.txt" "MINGW_PATH" "$mingw"

# Now get the Visual Studio installs and store them in an array.
# Any error here will not stop the build tool. It just means that the IDE Ted will not be an option.
do_msvc "$vsver" "$vsinstall"
if ($global:EXITCODE -ne 0) {
    do_error($global:MESSAGE)
    if ($global:EXITCODE -gt -2) { pause }
}

# To build Ted requires that both a Qt SDK is installed and that there is a Visual Studio compiler present.
# Get Qt installs first and store them in an array. Any error here will not stop the build tool. It just means that the IDE Ted will not be an option.
do_qtsdk_check "$qtver" "$qtsdk"
if ($global:EXITCODE -ne 0) {
    do_error($global:MESSAGE)
    if ($global:EXITCODE -gt -2) { pause }
}

# Construct the menu
[string[]]$menu = @("All", "Transcc", "CServer", "Makedocs", "Launcher")
if (($global:MSVC_INSTALLS.Count -gt 0) -and ($global:QT_INSTALLS.Count -gt 0)) { $menu += "Ted" }
$menu += "Quit"

function do_show_deps() {
    # Display the current MinGW being used
    do_success "MINGW: $(do_get_config_var "$BIN\config.winnt.txt" "MINGW_PATH")"

    # Display The MSVC compiler being used.
    if ($global:MSVC_SELECTED_IDX -ge 0) {
        $s = $global:MSVC_INSTALLS[$global:MSVC_SELECTED_IDX]
        do_success "MSVC: $s"
    }

    # Display The Qt SDK kit being used.
    if ($global:QT_SELECTED_IDX -ge 0) {
        $s = $global:QT_INSTALLS[$global:QT_SELECTED_IDX]
        do_success "Qt SDK: $s"
    } else {
        do_unknown "Qt SDK not installed."
    }
}

function do_menu() {
    Clear-Host
    do_header "===== Cerberus X Tool Builder Version $SCRIPT_VER ====="
    do_show_deps
    for ( $index = 0; $index -lt $menu.Count; $index++) {
        [int]$s = [int]$index + 1
        "${s}: {0}" -f $menu[$index]
    }
}

# Only show the menu if the showmenu option was set. Else, do a normal build all.
if ($showmenu -eq $true) {
    do {
        do_menu
        $in = Read-Host "Build Cerberus Tool: "

        # Only allow numbers
        if (-not(($in -ge "0") -and ($in -le "9"))) { continue }

        # Cast the value into an integer and only allow values within the menu array
        [int]$i = $in
        if (($i -ge 1) -or ($i -le ($menu.count))) {

            # Build all
            if ($i -eq 1) {
                do_all
            }

            # Build Transcc
            if ($i -eq 2) {
                do_transcc
            }

            # Build CServer
            if ($i -eq 3) {
                do_cserver
            }

            # Build Makedocs
            if ($i -eq 4) {
                do_makedocs
            }

            # Build the launcher
            if ($i -eq 5) {
                do_launcher
            }

            # Only allow Ted is all the criteria has been met.
            if (($global:MSVC_INSTALLS.count -gt 0) -and ($global:QT_INSTALLS.Count -gt 0) -and ($i -eq 6)) {
                do_ted
            }

            # Pause after an menu operation that's not the item to exit.
            if ($i -lt ($menu.Count)) { pause }
        }
    }
    until ($i -eq ($menu.Count))
}
else {
    do_info "`nBUILD TOOLS INSTALLED"
    do_show_deps
    do_all
}

# From this point on. Any changanges to config files that should not be part of the deployment should be restored.
do_set_config_var "$BIN/config.winnt.txt" "MINGW_PATH" "$MINGW_STORE" 