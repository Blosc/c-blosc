setlocal EnableDelayedExpansion

git reset --hard || exit /b 1
git clean -ffdx || exit /b 1

::No submodules in this repo, no need to use these commands
::git submodule foreach --recursive git reset --hard || exit /b 1
::git submodule foreach --recursive git clean -ffdx || exit /b 1