IF "%APPVEYOR_REPO_TAG_NAME%"=="" (
    cmake "-G%GENERATOR%" -H. -B_builds
    cmake --build _builds --config "%CONFIG%"
    cd _builds
    ctest -VV -C "%CONFIG%"
) ELSE (
    SET CONAN_REFERENCE=c-blosc/%APPVEYOR_REPO_TAG_NAME%
    python build.py
)
