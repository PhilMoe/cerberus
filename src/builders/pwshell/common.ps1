# COMMON FUNCTIONS
# THE SCRIPT IS PART OF THE CERBERUS X BUILER TOOL.

[int]$global:EXITCODE = -1
[string]$global:MESSAGE = ""
[string]$global:VSINSTALLER_PATH = ""
[string[]]$global:MSVC_INSTALLS = @()
[string[]]$global:QT_INSTALLS = @()
[int]$global:QT_SELECTED_IDX = -1
[int]$global:MSVC_SELECTED_IDX = -1
[bool]$global:COMPILER_INSTALLED = $false

# Display colourised information
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

# General external application execution function.
function execute([String]$_cmd, [string[]]$_argList) {
    $global:EXITCODE = 0
    
    do_build "EXECUTING: $_cmd $_argList"
    $expr = "& `"$_cmd`" $_argList"
    try {
        $global:MESSAGE = Invoke-Expression "$expr 2>&1"  | Out-String
        if ($stdstream -eq $true) { Write-Host $global:MESSAGE }
        if (-not($LASTEXITCODE -eq 0)) { throw $global:MESSAGE }
    }
    catch {
        $global:EXITCODE = 1
        $global:MESSAGE = $_ | Out-String
    }
}

# Function to clean up after transcc builds
function clean_build([string]$_srcfile, [bool]$_dotbuild = $true) {
    [string]$dir = ""
    if ($_dotbuild -eq $true) {
        $dir = "$SRC\$_srcfile\$_srcfile.build"
    } else {
        $dir = "$_srcfile"
    }
    if (Test-Path("$dir")) { Remove-Item -Force -Recurse "$dir" }
}

# Function to execute a transcc build.
function transcc([string]$_name, [string]$_target, [string]$_srcfile, [string]$_mode = "1") {
    $global:EXITCODE = -1

    [string]$srcpath = "$SRC\$_srcfile"
    # Only proceed if transcc has been built.
    if (-not(Test-Path("$BIN\transcc_winnt.exe"))) {
        do_error "NO TRANSCC PRESENT"
        $global:EXITCODE = 1
        return
    }

    do_info "BUILDING $_name"

    # Set the toolchain based upon the target and msbuild
    [string]$toolchain = ""
    if ($msbuild -eq $true) {
        if ($_target -eq "C++_Tool") {
            $toolchain = "+CC_USE_MINGW=0"
        } else {
            $toolchain = "+GLFW_USE_MINGW=0"
        }
    }
 
    execute "$BIN\transcc_winnt.exe" "-target=$_target -builddir=`"$_srcfile.build`" -clean -config=release +CPP_GC_MODE=$_mode $toolchain `"$srcpath\$_srcfile.cxs`""
    if ($global:EXITCODE -ne 0) {
        $global:EXITCODE = 1
        return
    }
}
