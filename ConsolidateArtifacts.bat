setlocal EnableDelayedExpansion

:: Libraries from local folder
@SETLOCAL
@ECHO off
call "%VS140COMNTOOLS%..\..\VC\bin\vcvars32.bat"

set "OUT_DIR=./Artifacts"
set "CURR_DIR=."

::remove original artifacts (if any)
if exist "OUT_DIR" rmdir "%OUT_DIR%" /s /q
::create artifacts folder
mkdir "%OUT_DIR%"

::move required dll,.lib and .exp into build folder
xcopy  "%CURR_DIR%\blosc\Release\blosc.*" "%OUT_DIR%" /y/s/q/i
if !errorlevel! neq 0 exit /b !errorlevel!

exit /b 0

@ECHO ON

:FAIL
popd 
exit /b %errorlevel%
@ECHO ON

