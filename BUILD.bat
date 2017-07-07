setlocal EnableDelayedExpansion

:: Libraries from local folder
@ECHO off

set OUT_DIR=.
set BUILD_TYPE="Release"

:: Remove existing Dll's
if exist "%OUT_DIR%\blosc\%BUILD_TYPE%" rmdir /s /q "%OUT_DIR%\blosc\%BUILD_TYPE%"
if errorlevel 1 exit /b %errorlevel%

::Assume this script lives in root of blosc repo
::create visual studio 64 bit project with cmake
@ECHO ON
cmake -DCMAKE_INSTALL_PREFIX="%OUT_DIR%" -G "Visual Studio 14 2015 Win64" .
@ECHO OFF
if errorlevel 1 exit /b %errorlevel%
@ECHO ON
cmake --build . --target blosc_shared --config %BUILD_TYPE%
@ECHO OFF
if errorlevel 1 exit /b %errorlevel%


exit /b 0

@ECHO ON

:FAIL
echo Failed to compile:  "Blosc" error code: %errorlevel%
@ECHO OFF
popd 
exit /b %errorlevel%
@ECHO ON

