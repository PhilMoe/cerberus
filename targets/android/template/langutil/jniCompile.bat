@echo off

set NDKBUILDCMD="C:\Users\mike\AppData\Local\Android\Sdk\ndk-bundle\ndk-build"
call %NDKBUILDCMD% NDK_OUT=../../build/jniObjs NDK_LIBS_OUT=./jniLibs -j 16 2> log.txt

rem copy /y "..\..\..\..\..\platform\android\ARCore\libs\arm64-v8a\libarcore_sdk.so" "jniLibs\arm64-v8a\libarcore_sdk.so"
rem copy /y "..\..\..\..\..\platform\android\ARCore\libs\armeabi-v7a\libarcore_sdk.so" "jniLibs\armeabi-v7a\libarcore_sdk.so"
rem copy /y "..\..\..\..\..\platform\android\ARCore\libs\x86\libarcore_sdk.so" "jniLibs\x86\libarcore_sdk.so"

rem everything below here is for automated systems, it can be ignored or removed
if "%1"=="nopause" goto end
pause
:end
if not %ERRORLEVEL% equ 0 ( EXIT /B 1 )