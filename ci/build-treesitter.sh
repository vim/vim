#!/bin/bash
set -e

git clone https://github.com/tree-sitter/tree-sitter.git

cd tree-sitter

git fetch --tags
git checkout $(git describe --tags "$(git rev-list --tags --max-count=1)")

CC=gcc PREFIX=/usr make install
