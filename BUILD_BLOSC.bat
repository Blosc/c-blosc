setlocal EnableDelayedExpansion

:: Libraries from local folder
@SETLOCAL
@ECHO off
call "%VS140COMNTOOLS%..\..\VC\bin\vcvars32.bat"

set OUT_DIR=.

::Assume this script lives in root of blosc repo
set "lastCompileAttempt=Blosc"
::create visual studio project with cmake
@ECHO ON
cmake -DCMAKE_INSTALL_PREFIX="%OUT_DIR%" --build .
@ECHO OFF
if !errorlevel! neq 0 exit /b !errorlevel!

::build visual studio
@ECHO ON
msbuild blosc.sln /p:Configuration=Release /p:Platform=Win32 
@ECHO OFF
if !errorlevel! neq 0 exit /b !errorlevel!
popd


exit /b 0

@ECHO ON

:FAIL
echo Failed to compile:  "%lastCompileAttempt%" error code: %errorlevel%
@ECHO OFF
popd 
pause 
exit /b %errorlevel%
@ECHO ON

