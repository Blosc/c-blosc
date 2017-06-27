#!/bin/bash

set -e
set -x

if [[ "$(uname -s)" == 'Darwin' ]]; then
    brew update || brew update
    brew install cmake || true
fi

pip install conan --upgrade
pip install conan_package_tools

conan user
