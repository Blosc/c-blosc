#!/bin/bash

set -e
set -x

if [[ ! -z $TRAVIS_TAG ]]; then
    export CONAN_REFERENCE="c-blosc/${TRAVIS_TAG}"
    if [[ "$(uname -s)" == 'Darwin' ]]; then
        if which pyenv > /dev/null; then
            eval "$(pyenv init -)"
        fi
        pyenv activate conan
    fi
    python build.py
else
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    ctest
fi

