#!/bin/bash
set -e

# Using a systemd user service doesn't work because it seems like github actions
# doesn't support user sessions? Just run sway in the background and disown it.
WLR_BACKENDS=headless sway &
disown
