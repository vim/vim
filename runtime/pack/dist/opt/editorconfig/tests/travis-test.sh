#!/bin/bash
# travis-test.sh: Script for running editorconfig-vim tests under Travis CI.
# Copyright (c) 2019 Chris White.  All rights reserved.
# Licensed Apache, version 2.0 or any later version, at your option.

# Error exit; debug output
set -vxEeuo pipefail

# Permit `travis-test.sh plugin` if TEST_WHICH is unset
if [[ ( ! "${TEST_WHICH:-}" ) && "${1:-}" ]]; then
    export TEST_WHICH="$1"
fi

if [[ ! "${TEST_WHICH:-}" ]]; then
    cat <<EOT
Usage: $0 \$WHICH
  or:  TEST_WHICH=\$WHICH $0
Run automated tests of editorconfig-vim

\$WHICH can be "core" or "plugin".
EOT
    exit 2
fi

if [[ "$TEST_WHICH" = 'plugin' ]]; then       # test plugin

    # If not running from Travis, do what Travis would have
    # done for us.
    if [[ ! "${BUNDLE_GEMFILE:-}" ]]; then
        here="$(cd "$(dirname "$0")" &>/dev/null ; pwd)"
        export BUNDLE_GEMFILE="${here}/plugin/Gemfile"
        # Install into tests/plugin/vendor.  Don't clear it first,
        # since you can clear it yourself if you're running from a
        # dev environment.
        bundle install --jobs=3 --retry=3 --deployment
    fi

    # Use the standalone Vimscript EditorConfig core to test the plugin's
    # external_command mode
    export EDITORCONFIG_VIM_EXTERNAL_CORE=tests/core/editorconfig

    bundle exec rspec tests/plugin/spec/editorconfig_spec.rb

elif [[ "$TEST_WHICH" = 'core' ]]; then     # test core
    cd tests/core
    mkdir -p build  # May already exist if running from a dev env
    cd build
    cmake ..
    ctest . --output-on-failure -VV -C Debug
    # -C Debug: for Visual Studio builds, you have to specify
    # a configuration.

else
    echo 'Invalid TEST_WHICH value' 1>&2
    exit 1
fi
