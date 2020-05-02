#!/bin/bash
set -e

if ! modprobe snd-dummy; then
    # snd-dummy is contained in linux-modules-extra (if exists)
    apt install -y "linux-modules-extra-$(uname -r)"
    modprobe snd-dummy
fi
