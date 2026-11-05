#!/bin/sh

# Retry a complete index refresh when a mirror is being synchronized.
set -eu

for apt_attempt in 1 2 3; do
    if apt-get -o Acquire::http::No-Cache=true \
               -o Acquire::https::No-Cache=true update; then
        exit 0
    else
        apt_status=$?
    fi
    if [ "$apt_attempt" -eq 3 ]; then
        exit "$apt_status"
    fi
    # A cached Release file can disagree with a newer debug package index.
    rm -f /var/lib/apt/lists/ddebs.ubuntu.com_* \
          /var/lib/apt/lists/partial/ddebs.ubuntu.com_*
    echo "Retrying APT index refresh after mirror failure ($apt_attempt/3)" >&2
    sleep 10
done
