setlocal EnableDelayedExpansion

@ECHO off

::set up dirs
set "OUT_DIR=./artifacts"
set "CURR_DIR=."

::remove original artifacts (if any)
if exist "%OUT_DIR%" rmdir "%OUT_DIR%"  /s /q 
if errorlevel 1 exit /b %errorlevel%

::create artifacts folder
mkdir "%OUT_DIR%"
if errorlevel 1 exit /b %errorlevel%


::move required dll,.lib and .exp into build folder
xcopy  "%CURR_DIR%\blosc\Release\blosc.*" "%OUT_DIR%" /y/s/q/i
if errorlevel 1 exit /b %errorlevel%


exit /b 0

@ECHO ON

:FAIL
popd 
exit /b %errorlevel%
@ECHO ON

