# COMMON FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

########################################
# COMMON FUNCTION USE BY OTHER SCRIPTS
########################################

# Various flags and variables
[int]$global:EXITCODE = -1                      # Used to store the exit code after any call to functions and applications.
[string]$global:MESSAGE = ""                    # General messaging variable after any call to functions and applications.
[string]$global:VSINSTALLER_PATH = ""           # Holds the path to the Visual Studio installer.
[string[]]$global:MSVC_INSTALLS = @()           # Holds the total number of installed Visual Studio version found.
[string[]]$global:QT_INSTALLS = @()             # Holds the total number of Qt kits installed
[int]$global:QT_SELECTED_IDX = -1               # Used to select a specific Qt kit
[int]$global:MSVC_SELECTED_IDX = -1             # Used to select a specofic Visaul Studio version.
[bool]$global:COMPILER_INSTALLED = $false       # Flag to indicate that there is a compiler present.
[bool]$global:GIT_INSTALLED = $false            # Flag to indicate that there is a git installation present.
[string]$global:MINGW_STORE = ""                # Holds the original MINGW_PATH value in the config.winnt.txt
[string]$global:MSBUILD_STORE = ""              # Holds the original MSBUILD_PATH value in the config.winnt.txt

#########################################
# Display colourised information
#########################################
function do_info([string]$_msg) {
    Write-Host $_msg -ForegroundColor Cyan
}

function do_header([string]$_msg) {
    Write-Host $_msg -ForegroundColor Yellow
}

function do_build([string]$_msg) {
    Write-Host $_msg -ForegroundColor Blue
}

function do_error([string]$_msg) {
    Write-Host $_msg -ForegroundColor Red
}

function do_success([string]$_msg) {
    Write-Host $_msg -ForegroundColor Green
}

function do_unknown([string]$_msg) {
    Write-Host $_msg -ForegroundColor Magenta
}

###################################################
# General external application execution function.
###################################################
function execute([String]$_cmd, [string[]]$_argList) {
    $global:EXITCODE = 0
    
    do_build "EXECUTING: $_cmd $_argList"
    $expr = "& `"$_cmd`" $_argList"
    try {
        $global:MESSAGE = Invoke-Expression "$expr"  | Out-String
        if ($stdstream -eq $true) { Write-Host $global:MESSAGE }
        if (-not($LASTEXITCODE -eq 0)) { throw $global:MESSAGE }
    }
    catch {
        $global:EXITCODE = 1
        $global:MESSAGE = $_ | Out-String
    }
  
}

###############################################
# Function to clean up after transcc builds.
###############################################
# Passing $false as the second parameter will allow a non .build directory to be deleted.
function clean_build([string]$_srcfile, [bool]$_dotbuild = $true) {
    [string]$dir = ""
    if ($_dotbuild -eq $true) {
        $dir = "$SRC\$_srcfile\$_srcfile.build"
    } else {
        $dir = "$_srcfile"
    }
    if (Test-Path("$dir")) { Remove-Item -Force -Recurse "$dir" }
}

########################################
# Function to build with transcc
########################################
# The last parameter sets the garbage collection mode to use. The default is to use gc mode 1.
# See the Cerberus config documentation about garbage collection.
function transcc([string]$_name, [string]$_target, [string]$_srcfile, [string]$_gc_mode = "1") {
    $global:EXITCODE = -1

    [string]$srcpath = "$SRC\$_srcfile"
    # Only proceed if transcc has been built.
    if (-not(Test-Path("$BIN\transcc_winnt.exe"))) {
        do_error "NO TRANSCC PRESENT"
        $global:EXITCODE = 1
        return
    }

    do_info "BUILDING $_name"
 
    [string[]]$arguments=(
        "-target=$_target",
        "-builddir=`"$_srcfile.build`"",
        "-clean",
        "-config=release",
        "+CPP_GC_MODE=$_gc_mode"
    )

    # If the build tool is MSBuild, then pass the Windows SDK and Platform Toolset versions.
    if ($global:msbuild -eq $true) {
        if ($_target -eq "C++_Tool") {
            $arguments += (
                "+CC_USE_MINGW=0",
                "+CC_MSVC_MSIZE=`"$msize`"",
                "+CC_WINSDK_VERSION=`"$winsdk`"",
                "+CC_PLATFORM_TOOLSET=`"$platformtoolset`""
            )
        } else {
            $arguments += (
                "+GLFW_USE_MINGW=0",
                "+GLFW_MSVC_MSIZE_WINNT=`"$msize`"",
                "+GLFW_WINSDK_VERSION=`"$winsdk`"",
                "+GLFW_PLATFORM_TOOLSET=`"$platformtoolset`""
            )
        }
    } else {
        if ($_target -eq "C++_Tool") {
            $arguments += (
                "+CC_USE_MINGW=1",
                "+CC_MINGW_MSIZE=`"$msize`""
            )

            if ($mingwstatic -eq $true) {
                $arguments += ( "+CC_MINGW_STATIC_LINK=1" )
            }

        } else {
            $arguments += (
                "+GLFW_USE_MINGW=1",
                "+GLFW_MSIZE_WINNT=`"$msize`""
            )

            if ($mingwstatic -eq $true) {
                $arguments += ( "+GLFW_MINGW_STATIC_LINK=1" )
            }
        }
    }

    $arguments+=(
        "`"$srcpath\$_srcfile.cxs`""
    )

    execute "$BIN\transcc_winnt.exe" $arguments
    #execute "$BIN\transcc_winnt.exe" "-target=$_target -builddir=`"$_srcfile.build`" -clean -config=release +CPP_GC_MODE=$_gc_mode $toolchain `"$srcpath\$_srcfile.cxs`""
    if ($global:EXITCODE -ne 0) {
        $global:EXITCODE = 1
        return
    }
}

####################################################
#  Functions to work with the first key/value pairs.
####################################################
# This is for working with the config.winnt.txt file.
# NOTE: Only the first non commented key is detected. 
function do_set_config_var([string]$_path, [string]$_key, [string]$_value) {
    do_info "MODIFYING: $_path"
  
    # Update the config file with the
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

}

# Gets the first key=value pair from a config file that matches the key.
function do_get_config_var([string]$_path, [string]$_key) {
    $value = switch -Regex -File $_path { "^$_key=`"(.*)`"" { $Matches[1]; break } }
    return $value
}
