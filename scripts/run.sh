#!/bin/bash

set -e
set -x

if [[ ! -z $TRAVIS_TAG ]]; then
    export CONAN_REFERENCE="c-blosc/${TRAVIS_TAG}"
    python conanbuild.py
else
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    ctest
fi

