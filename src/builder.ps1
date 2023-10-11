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
    [Alias("k")][string]$qtkit = "",
    [Alias("c")][string]$mingw = "C:\TDM-GCC-64",
    [Alias("y")][string]$vsver = "",
    [Alias("i")][string]$vsinstall = "$([System.Environment]::GetEnvironmentVariable('ProgramFiles(x86)'))\Microsoft Visual Studio\Installer",
    [Alias("m")][switch]$showmenu = $true,
    [Alias("h")][switch]$help = $false,
    [Alias("b")][switch]$msbuild = $false,
    [Alias("s")][switch]$stdout = $false,
    [Alias("d")][string]$deploy = "",
    [switch]$clearbuilds = $false
)

[string]$SCRIPT_VER = "1.2.0"

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
."$MODULES\deploy.ps1"
."$MODULES\tools.ps1"

if ($help -eq $true) {
    do_info "CERBERUS X TOOLS VERSION $SCRIPT_VER"
    Write-Host "USEAGE: ./builder.ps1 [options]`n`t{-m|-showmenu}`t`t`t`t`t- run in menu mode.`n`t{-q|-qtsdk} `"QT_DIR_PATH`"`t`t`t- Set root Qt SDK directory."
    Write-Host "`t{-k|-qtkit} `"DOT.VERSION.NUMBER`"`t`t- Set Qt SDK version.`n`t{-vsi|-vsinstall} `"VISUAL_INSTALLER_PATH`"`t- Set MS Visual Installer directory."
    Write-Host "`t{-y|-vsver} `"PRODUCT_YEAR`"`t`t`t- Set Visual Studio product year`n`t{-c|-mingw} `"MINGW_DIR`"`t`t`t`t- Set MiGW root directory."
    Write-Host "`t{-b|-msbuild}`t`t`t`t`t- Build using MSBuild. Requires Visual Studio.`n`t{-s|-stdout}`t`t`t`t`t- Show stdout after execution."
    Write-Host "`t{-d|-deploy} `"DEPLOY_DIR`"`t`t`t- Build a deployment archive in the directory passed."
    Write-Host "`t-clearbuilds`t`t`t`t`t- Removes all previous built binaries of Cerberus within local repository.`n`t{-h|-help}`t`t`t`t`t- Show this quick help`n"
    Write-Host "EXAMPLE:`n`te.g: ./builder.ps1 -qtsdk C:\Qt -k 5.14.0"
    Write-Host "`te.g: ./builder.ps1 -qtsdk C:\Qt -qtkit 5.14.0 -vsver `"2017`" -showmenu"
    exit 1
}

do_header "===== Cerberus X Tool Builder Version $SCRIPT_VER ====="

# If the clear builds flag is set, then call the function to remove all previously build items.
if($clearbuilds) {
    do_clearbuilds
    do_success "Cerberus X Builder script terminated."
    exit 0
}

# The base requirments for building Cerberus is that a GCC compiler is present.
do_mingw($mingw)
if ($global:EXITCODE -ne 0) {
    do_error("NO MINGW COMPILER PRESENT")
    if ($global:EXITCODE -gt -2) { pause }
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

# If the value of $global:COMPILER_INSTALLED is $false, then stop the script.
if ($global:COMPILER_INSTALLED -eq $false) {
    do_error("NO COMPILER PRESENT")
    exit 1
}

# To build Ted requires that both a Qt SDK is installed and that there is a Visual Studio compiler present.
# Get Qt installs first and store them in an array. Any error here will not stop the build tool. It just means that the IDE Ted will not be an option.
do_qtsdk_check "$qtkit" "$qtsdk"
if ($global:EXITCODE -ne 0) {
    do_error($global:MESSAGE)
    if ($global:EXITCODE -gt -2) { pause }
}

# Set up the menu items. The array DISPLAY_ITEMS, holds the human readable menu items.
# The array MENU_ITEMS, holds the function names to call.
function do_items() {
    $global:display_items = @("All", "Transcc")
    $global:menu_items = @("do_all", "do_transcc")
    if ($global:TRANSCC_EXE -eq $true) {
        $global:display_items += @("CServer", "Makedocs", "Launcher")
        $global:menu_items += @("do_cserver", "do_makedocs", "do_launcher")
    }
    if (($global:MSVC_INSTALLS.Count -gt 0) -and ($global:QT_INSTALLS.Count -gt 0)) {
        $global:display_items += "Ted"
        $global:menu_items += "do_ted"
    }
    if (-not([string]::IsNullOrEmpty($deploy))) {
        $global:display_items += "Deplopy: $deploy"
        $global:menu_items += "do_deploy"
    }
    $global:display_items += ("Quit")
    $global:menu_items += ("do_quit")
}

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

    if ($msbuild -eq $true) {
        do_info "Toolchain: MSBuild"
    } else {
        do_info "Toolchain: MinGW"
    }
}

function do_title() {
    Clear-Host
    do_header "===== Cerberus X Tool Builder Version $SCRIPT_VER ====="
    do_show_deps
    for ( $index = 0; $index -lt $global:display_items.Count; $index++) {
        [int]$s = [int]$index + 1
        "${s}: {0}" -f $global:display_items[$index]
    }
}

# Only show the menu if the showmenu option was set. Else, do a normal build all.
if ($showmenu -eq $true) {
    [bool]$loop = $true
    do {

        # Test to see if transcc has been built.
        execute "$BIN/transcc_winnt"
        if ($global:EXITCODE -eq 0) {
            $global:TRANSCC_EXE = 1
        } else {
            $global:TRANSCC_EXE = 0
        }
        
        # update the menu and wait for selection.
        do_items
        do_title
        $in = Read-Host "Select application to build"

        # Only allow numbers
        if (-not(($in -ge "0") -and ($in -le "9"))) { continue }

        # Cast the value into an integer and then subtract one.
        # All functions are stored in an array and arrays start at zero.
        [int]$i = $in - 1
        #$i = $i - 1

        # Get the function to call. If the function name is do_quit, then exit the script.
        [string]$call = $global:menu_items[$i]
        if ($call -eq "do_quit") {
            $loop = $false
        } else {
            & $call
            pause
        }
    }
    until ($loop -eq $false)
} else {
    do_info "`nBUILD TOOLS INSTALLED"
    do_show_deps
    do_all
}

# From this point on. Any changanges to config files that should not be part of the deployment should be restored.
do_set_config_var "$BIN/config.winnt.txt" "MINGW_PATH" "$MINGW_STORE" 

exit $global:EXITCODE