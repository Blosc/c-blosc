IF /i "%APPVEYOR_REPO_TAG_NAME%"=="" goto CMake
IF /i NOT "%CONAN_VISUAL_VERSIONS%"=="" goto Conan
goto commonexit

:CMake
cmake "-G%GENERATOR%" -H. -B_builds
cmake --build _builds --config "%CONFIG%"
cd _builds
ctest -VV -C "%CONFIG%"
goto commonexit

:Conan
SET CONAN_REFERENCE=c-blosc/%APPVEYOR_REPO_TAG_NAME%
python "conanbuild.py"
goto commonexit

:commonexit
exit
