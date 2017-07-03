setlocal EnableDelayedExpansion

:: Libraries from local folder
@SETLOCAL
@ECHO off

set OUT_DIR=.
set BUILD_TYPE="Release"

:: Remove existing Dll's
if exist "%OUT_DIR%\blosc\%BUILD_TYPE%" rmdir /s /q "%OUT_DIR%\blosc\%BUILD_TYPE%"
if errorlevel 1 exit /b %errorlevel%

::Assume this script lives in root of blosc repo
set "lastCompileAttempt=Blosc"
::create visual studio project with cmake
@ECHO ON
cmake -DCMAKE_INSTALL_PREFIX="%OUT_DIR%" -G "Visual Studio 14 2015 Win64" .
cmake --build . --target blosc_shared --config %BUILD_TYPE%
@ECHO OFF
if errorlevel 1 exit /b %errorlevel%

::@ECHO ON

::remove all files used for building
del /s /q /f "%OUT_DIR%\*.vcxproj"
del /s /q /f "%OUT_DIR%\*.vcxproj.*"
del /s /q /f "%OUT_DIR%\cmake_install.cmake"
del /s /q /f "%OUT_DIR%\cmake_uninstall.cmake"
del /s /q /f "%OUT_DIR%\CMakeCache.txt"
del /s /q /f "%OUT_DIR%\CPackConfig.cmake"
del /s /q /f "%OUT_DIR%\CPacksourceConfig.cmake"
del /s /q /f "%OUT_DIR%\CTestTestfile.cmake"
del /s /q /f "%OUT_DIR%\blosc.sln"
del /s /q /f "%OUT_DIR%\blosc\config.h"

::remove all folders used for building
rmdir /s /q "%OUT_DIR%\blosc\blosc_shared.dir"
rmdir /s /q "%OUT_DIR%\x64"
rmdir /s /q "%OUT_DIR%\bench\CMakeFiles"
rmdir /s /q "%OUT_DIR%\blosc\CMakeFiles"
rmdir /s /q "%OUT_DIR%\tests\CMakeFiles"
rmdir /s /q "%OUT_DIR%\CMakeFiles"
if errorlevel 1 exit /b %errorlevel%

exit /b 0

@ECHO ON

:FAIL
echo Failed to compile:  "%lastCompileAttempt%" error code: %errorlevel%
@ECHO OFF
popd 
pause 
exit /b %errorlevel%
@ECHO ON

