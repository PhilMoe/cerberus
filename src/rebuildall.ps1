# Windows Powershell build script
# TO DO: The cserver needs to be rewritten so it doesn't need BlitzMax
param(
    [string]$mingw = "C:\Mingw",
    [string]$qtsdk = "C:\Qt\5.5\msvc2013_64",
    [string]$visualstudio = "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC",
    [string]$qtspec = "win32-msvc2013",
    [bool]$qtdbg = 0
)

# NOTE: This script requires that the execution policy for the current user be set to unrestricted.
# Open power shell as administrator and use:
#    get-executionpolicy -list
#    set-executionpolicy -scope currentuser unrestricted
# If the file is still blocked use:
#    unblock-file -path "full_path_to_this_script"
# You should reset the execution policy back to it's original state e.g.:
#    set-executionpolicy -scope currentuser undefined

clear

$arch = $env:PROCESSOR_ARCHITECTURE | %{$_ -replace "AMD", ""} | %{$_ -replace "x86", "32"}

function ErrorMsg($msg){
    Write-Host $msg -BackgroundColor Red -ForegroundColor Yellow
    exit 1
}

# Set compiler and sdk paths
if(-not (Test-Path "$mingw")){
    ErrorMsg "ERROR: No valid Windows GCC compiler tools installed!"
} else {
    $mingwPath = $mingw+";" + $mingw + "\bin;" + $mingw + "\include;" + $mingw + "\lib;"
}

if(-not (Test-Path "$qtsdk")){
    ErrorMsg "ERROR: No valid Qt SDK installed!"
} else {
    $qtsdkPath = $qtsdk + ";" + $qtsdk + "\bin;" + $qtsdk + "\include;" + $qtsdk + "\lib;" + $qtsdk + "\plugins;"
}

$env:Path = $qtsdkPath + $mingwPath + [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

# Add Visual Studio stuff. Note that this is cumulative and you will end up with an error that cmd's input line is too long, so just open and start another power shell session.
if(-not (Test-Path "$visualstudio")){
    ErrorMsg "ERROR: No valid Visual Studio tools installed!"
} else {
    pushd "$visualstudio"
    #if(Test-Path "$visualstudio\vcvarsall.bat"){
        cmd /c "vcvarsall.bat x86_amd64&set" |
        foreach {
            if ($_ -match "=") {
                $v = $_.split("="); set-item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
            }
       # }
    }
    popd
}

# Build transcc first
if(Test-Path $mingw\bin\g++.exe)
{
    write-host "Building transcc"
    write-host "Please wait"
    if(Test-Path ..\bin\transcc_winnt.exe){
        remove-item ..\bin\transcc_winnt.exe -force
    }

    g++ -O3 -DNDEBUG -o ..\bin\transcc_winnt transcc\transcc.build\cpptool\main.cpp -lpthread -s

    write-host "Building launcher"
    if(Test-Path ..\Cerberus.exe){
        remove-item ..\Cerberus.exe -force
    }

    windres launcher\resource.rc -O coff -o launcher\res.o
    g++ -Os -DNDEBUG -o ..\Cerberus.exe launcher\codeblocks\launcher.cpp launcher\res.o -ladvapi32 -s

    if(-Not (Test-Path ..\Cerberus.exe)){
        ErrorMgs "ERROR: Failed to build Cerberus launcher."
    }

} else {
    ErrorMsg "ERROR: No valid Windows GCC C++ compiler installed!"
    }

Write-Host ""
# Only continue if transcc was successfully built
if(Test-Path ..\bin\transcc_winnt.exe){
    
    # Before continuing, fix the location of MinGw in \bin\config.winnt.txt so that transcc knows where it is.
    $file = '..\bin\config.winnt.txt'
    $regex = '(?<=MINGW_PATH=")[^"]*'
    (Get-Content $file) -replace $regex, $mingw | Set-Content $file

    #Make makedocs
    write-host "makedocs"
    if(Test-Path ..\bin\makedocs_winnt.exe) {
        remove-item ..\bin\makedocs_winnt.exe -force
    }

    ../bin/transcc_winnt -target="C++_Tool" -builddir="makedocs.build" -clean -config="release" +CPP_GC_MODE=0 "makedocs\makedocs.cxs"
    move-item makedocs\makedocs.build\cpptool\main_winnt.exe ..\bin\makedocs_winnt.exe
    remove-item makedocs\makedocs.build -force -recurse

    if(-Not (Test-Path ..\bin\makedocs_winnt.exe)){
        ErrorMsg "ERROR: Failed to build makedocs!"
    }

    Write-Host ""
    #Make cserver
    if(Test-Path ..\bin\cserver_winnt.exe) {
        remove-item ..\bin\cserver_winnt.exe -force
    }

    write-host "building cserver"
    ../bin/transcc_winnt -target="Desktop_Game_(Glfw3)" -builddir="cserver.build" -clean -config="release" +CPP_GC_MODE=1 "cserver\cserver.cxs"
    Move-Item cserver\cserver.build\glfw3\gcc_winnt\Release"$arch"\CerberusGame.exe ..\bin\cserver_winnt.exe

    # Move the openal dll and licence text
    if(-Not (Test-Path ..\bin\openal.dll)) {
        Move-Item cserver\cserver.build\glfw3\gcc_winnt\Release"$arch"\OpenAL32.dll ..\bin\OpenAL32.DLL
        if(-Not (Test-Path ..\bin\OpenAL_COPYING)) {
            Move-Item cserver\cserver.build\glfw3\gcc_winnt\Release"$arch"\Openal32_COPYING ..\bin\OPENAL32_COPYING
        }
    }

    Remove-Item cserver\cserver.build -force -recurse

    # Clean out openal if failed to build cserver
    if(-Not (Test-Path ..\bin\cserver_winnt.exe)){
        if(Test-Path ..\bin\openal.dll) {
            Remove-Item cserver\cserver.build\glfw3\gcc_winnt\Release"$arch"\OpenAL32.dll ..\bin\OpenAL32.DLL
            if(Test-Path ..\bin\OpenAL_COPYING) {
                Remove-Item cserver\cserver.build\glfw3\gcc_winnt\Release"$arch"\Openal32_COPYING ..\bin\OPENAL32_COPYING
            }
        }
        ErrorMsg "ERROR: Failed to build cserver!"
    }
    
} else {
    ErrorMsg "Error: Failed to start transcc. Was it built successfully?"
}

Write-Host ""
#Make ted
write-host "building ted"
if(Test-Path ..\bin\Tedt.exe){
    Remove-Item ..\bin\Ted.exe -Force
    Remove-Item ..\bin\*.dll -Force
    Remove-Item ..\bin\plugins -Force -Recurse
}

if(Test-Path build-ted-Desktop-Release) {
    Remove-Item build-ted-Desktop-Release -force -recurse
}

New-Item build-ted-Desktop-Release -type directory
cd build-ted-Desktop-Release

qmake -spec $qtspec ../ted/ted.pro

if($qtdbg) {
    cmd /c nmake -f Makefile.Debug
    $deploy = "--debug"
} else {
    cmd /c nmake -f Makefile.Release
    $deploy = "--release"
}

cd ..
Remove-Item build-ted-Desktop-Release -force -recurse

# Deploy and clean
windeployqt $deplpoy --no-svg --no-angle --no-compiler-runtime --no-system-d3d-compiler --no-quick-import --no-translations --core ..\bin\Ted.exe
$folders = "audio","bearer","imageformats","mediaservice","playlistformats","position","printsupport","sensors","sensorgestures","sqldrivers","opengl32sw.dll"
foreach($folder in $folders){
    Remove-Item ..\bin\$folder -Force -Recurse
}

exit 0