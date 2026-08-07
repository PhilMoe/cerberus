# THIRD-PARTY FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

#######################################################################################################
#   DETECTION AND SET UP OF THIRD PARTY TOOLS, FRAME WORKS AND COMPILER TOOL CHAINS
#######################################################################################################

############################
# VISUAL STUDIO DETECTION
############################
# NOTE: Due to how a MSVC tool chain works. It is necessary to set the environment up correctly.
# As this can be an issue if the script is run multiple times. Then it is necessary to set an
# environment variable to indicate that the current session has already done this.
function do_msvc_env() {

    # Check to see if the the Visual Studio enviroment has already been set up.
    if ($env:CERBERUS -eq 1) { return }
    $env:CERBERUS = 1

    do_info "SETTING UP MSVC BUILD ENVIRONMENT"

    # Save the current directory and swicth to where the MSVC tools set batch file is located.
    Push-Location
    $dir = $global:MSVC_INSTALLS[$global:MSVC_SELECTED_IDX] + "\VC\Auxiliary\Build"
    Set-Location $dir

    # Parse the vcvarsall.bat to get the values and set the appropriate evnvironment variables.
    cmd /c "vcvarsall.bat x86_amd64&set" |
    ForEach-Object {
        if ($_ -match "=") {
            $v = $_.Split("=",2)
            Set-Item -Force -Path "ENV:\$($v[0])" -Value "$($v[1])"
        }
    }
    Pop-Location | Out-Null
}

# To confirm the installation of a MS Visual Studio install. The script relies on the presence of the MS Visual Studio
# installer, specifically vswhere.
function do_check_vswhere([string]$_vsi) {
    $global:EXITCODE = -1

    # If the path passed parameter is valid to an application called vswhere; then run it to checck that it is vswhere.
    # The parameter passed should be tested first before testing what would be the default location on a 32 bit MS Windows
    # installation. The main script defaults to what would be the 64 bit location.
    if ((Test-Path($_vsi + "\vswhere.exe"))) {
        execute "$_vsi\vswhere.exe" "-h"
    }
    else {
        $_vsi = "$([System.Environment]::GetEnvironmentVariable('ProgramFiles'))\Microsoft Visual Studio\Installer"
        execute "$_vsi\vswhere.exe" "-h"
    }

    # If vswhere was unable to run, then issue a warning. An error isn't thrown, as MinGW may be the prefered compiler.
    if ($global:EXITCODE -ne 0) {
        $global:MESSAGE = "Failed to execute vswhere in directory`n$_vsi"
        return
    }

    # If the execution of vswhere was successful; then use the returned output stream
    # to check for specific information to identify that it is vswhere.
    if (-not($global:MESSAGE.Contains("Visual Studio Locator version"))) {
        $global:MESSAGE = "Not a valid vswhere in directory`n$_vsi"
        return
    }

    # If this point is reached, then the path is valid and vswhere will be used to get the  information about the MS
    # Visual Studio locations.
    $global:VSINSTALLER_PATH = $_vsi
}

# Check the passed parameters for valid MS Visual Studio installations.
# All detected MS Visaul Studio installations will be placed the MSVC_INSTALLS array and the one
# selected will depend on either the version passed on as a parameter, or the highest version found.
function do_msvc([string]$_vsver, [string]$_vsi) {
    $global:EXITCODE = 0

    do_info "`nChecking for Visual Studio installs..."

    # Before executing any applications. The paths need to be checked.
    # If there is no MS Visual Studio installer installation; then ignore MS Visual Studio detections.
    if (-not((Test-Path($_vsi)) -or (Test-Path("$([System.Environment]::GetEnvironmentVariable('ProgramFiles'))\Microsoft Visual Studio\Installer")))) {
        $global:EXITCODE = -2
        $global:MESSAGE = "NO VISUAL STUDIO INSTALLER PATH SET FOR VISUAL STUDIO CHECK"
        return
    }

    # The Visual Studio installer will have a tool where the selected Visual Studio's path can be retrieved;. This tools is vswhere.
    # NOTE: The do_check_vswhere will set the error and messages.
    do_check_vswhere "$_vsi"
    if ($global:EXITCODE -ne 0) { return }

    # If the vswhere check has passed; then it's time to get all Visual Studio installs and paths.
    # vswhere has a handy option to sort all retrieved information from latest to oldest.
    # IF the vsversions array is empty; then there are no Visual Studio installs.
    execute "$global:VSINSTALLER_PATH\vswhere.exe" "-sort -property catalog_productLineVersion"
    if ($global:EXITCODE -ne 0) { return }

    [string[]]$vsversions = $global:MESSAGE.Split("`n") | ForEach-Object { "$_".Trim() }
    $vsversions = $vsversions.Where({ $_ -ne "" })

    execute "$global:VSINSTALLER_PATH\vswhere.exe" "-sort -property installationPath"
    if ($global:EXITCODE -ne 0) { return }

    # Store the paths for detected installs and remove empty items.
    $global:MSVC_INSTALLS = $global:MESSAGE.Split("`n") | ForEach-Object { "$_".Trim() }
    $global:MSVC_INSTALLS = $global:MSVC_INSTALLS.Where({ $_ -ne "" })

    # Now check if the the MS Visual Studio matches the passed parameter. If it does, then set the variable MSVC_SELECTED_IDX to it's index.
    $global:MSVC_SELECTED_IDX = [int]$vsversions.IndexOf($_vsver)
    if ($global:MSVC_SELECTED_IDX -lt 0) { $global:MSVC_SELECTED_IDX = 0 }

    # The final check is to see if the selected install contains MSVC by checking for vcvarsall.bat
    # If vcvarsall.bat is not present, then set MSVC_SELECTED_IDX to minus one to indicate that there is no Visual Studio that can be used.
    if (-not(Test-Path($global:MSVC_INSTALLS[$global:MSVC_SELECTED_IDX] + "\VC\Auxiliary\Build\vcvarsall.bat"))) {
        $global:MSVC_SELECTED_IDX = -1
        return
    }

    # If this point is reached, then it's safe to say that there is a compiler present, so set the compiler installed flag.
    $global:COMPILER_INSTALLED = $true

    # Set up the MSVC environment.
    do_msvc_env
}

############################
# MINGW DETECTION
############################
# Check if the passed parameter is a valid MinGW installation.
# This function will set the compiler installed flag and return the path of the MinGW installation.
# NOTE: GCC will output version info either in stderr or stdout depending on the version option passed.
# As this is a test and not compiling, then the --version option must be used, else an error could be thrown.
function do_mingw([string]$_mingw) {
    $global:EXITCODE = 0

    do_info "`nChecking for MinGW"

    # The first thing is to check to see if the parameter passed has a valid MinGW install. If so, then add it to the
    # current sessions PATH variable, else issue a message and try to look for a system wide install.
    execute "$_mingw/bin/g++" "--version" "2>$null"
    if ($global:EXITCODE -eq 0) {

        # Now get the current PATH variable for checking to see if the MinGW path passed is already in the systems PATH variable.
        [string]$path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

        # Only pre-end the passed parameter to the systems environment variable if there isn't one already there.
        if (-not($path.Contains("$_mingw\bin"))) {
            [Environment]::SetEnvironmentVariable('PATH', $_mingw + "\bin;" + $path)
        }

        $global:COMPILER_INSTALLED = $true
        return $_mingw
    } else {
        do_error("MinGW executable check failed.`nCheck that $_mingw is a valid installation.`nChecking for a system wide install.")
    }

    # If the above failed, try to see if there is already one installed on the systems PATH.
    # Note: Only checks the first one found in the systems environment PATH variable.
    [string]$local:GCC = $(get-command g++.exe 2>$null).Path
    
    if(-not([string]::IsNullOrEmpty($GCC))) {

        [string]$local:SYSMINGW = Split-Path $(Split-Path $GCC -Parent) -Parent        
        execute "g++" "--version"
        if ($global:EXITCODE -ne 0) {
            $global:COMPILER_INSTALLED = $false
            do_error("System MinGW executable check failed.`nCheck that $SYSMINGW is valid.")
            return "NOT INSTALLED"
        }
        do_success("Found MinGW: $SYSMINGW")
        return $SYSMINGW
    }

    return "NOT INSTALLED"
}

############################
# QT FRAME WORK DETECTION
############################
# Scan the passed parameters for valid Qt SDK installations.
function do_qtsdk_check([string]$_qtver, [string]$_qtsdk) {
    $global:EXITCODE = 0

    do_info "`nChecking for Qt installations..."

    # Check that there is a Qt path before trying to execute any applications.
    # If it does't exist; then ignore Qt detection.
    if (-not(Test-Path($_qtsdk))) {
        $global:EXITCODE = -2
        $global:MESSAGE = "NO DIRECTORY NOT SET FOR QT CHECK"
        return
    }

    # Check that the Qt SDK's main path is valid by querying the maintenance tool.
    if (Test-Path($_qtsdk + "\MaintenanceTool.exe")) {
        execute "$_qtsdk\MaintenanceTool.exe" "-v"
    }

    # If the maintenance tool was not detected; then issue a warning and ignore Qt detections.
    if ($global:EXITCODE -ne 0) {
        $global:MESSAGE = "Failed to execute Qt Maintenance tool in directory`n$_qtsdk`nIs Qt installed?"
        return
    }

    # If the maintenance tool issue the incorrect output; then issue a warning and ignore Qt detections.
    if (-not($global:MESSAGE.Contains("IFW Version:"))) {
        $global:MESSAGE = "Not a valid Qt maintenance tool in directory`n$_qtsdk`nIs this a valid Qt SDK?"
        $global:EXITCODE = 1
        return
    }
    else { do_success "Found MaintenanceTool" }

    # Okay if the script has made it this far, then the Qt SDK directory needs to be scanned for valid MSVC qmake installations.
    # This will store all MSVC paths into the QT_INSTALLS array and sort them in deceneding order.
    $global:QT_INSTALLS = Get-ChildItem $_qtsdk |
    Where-Object { $_.PSIsContainer } |
    Foreach-Object {
        if ($_.Name.Contains(".")) {
            Get-ChildItem $_qtsdk\$_ |
            Where-Object { $_.PSIsContainer } |
            ForEach-Object {
                if ($_.Name.Contains("msvc")) {
                    $global:EXITCODE = -1
                    [string]$p = $_.FullName.Trim()
                    execute $p"\bin\qmake.exe" "-v"
                    if ($global:EXITCODE -eq 0) { $_.FullName }
                }
            }
        }
    } |
    Sort-Object -Descending { 
        [Version] $(if ($_ -match "[\d\.]+") { 
                $matches[0] -replace "_", "."
            }
        )
    } 

    # Now check if the the MS Visual Studio matches the passed parameter. If it does, then set the variable MSVC_SELECTED_IDX to it's index.
    $global:QT_SELECTED_IDX = -1
    $global:QT_INSTALLS | ForEach-Object {
        if ("$_".Contains("`\$_qtver`\")) {
            $global:QT_SELECTED_IDX = [int]$global:QT_INSTALLS.IndexOf($_)
            do_success "Found Qt $_qtver"
        }
    }
    
    # If the variable QT_SELECTED_IDX is less than zero, and the QT_INSTALLS array count is greater than zero. Then select the first element in the QT_INSTALL array.
    # If not; then there are no kits installed.
    if (($global:QT_SELECTED_IDX -lt 0) -and ($global:QT_INSTALLS.Count -gt 0)) {
        $global:QT_SELECTED_IDX = 0
        do_unknown "Unknown Qt $_qtver`nSelecting highest version detected."
    }
    else {
        $global:MESSAGE = "No Qt SDK kits detected."
    }
}

############################
# GIT DETECTION
############################
# Check if git is installed system wide and valid. If it is, then set the git installed flag.
function do_git_check() {

    [string]$local:GIT_PATH = $(get-command git.exe).Path

    if(-not([String]::IsNullOrEmpty($GIT_PATH))) {
        execute "git" "--version" | Out-Null
        if ($global:EXITCODE -ne 0) { return }
    }
    $global:GIT_INSTALLED = $true
}
