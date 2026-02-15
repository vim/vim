#!/bin/bash
# Issue #19053 (sh syntax: escaped characters followed by # in double quotes)

CLEANURL='http://localhost:8077/test'

FILENAME=$(sed -r -e "s#[:/]+#\.#g" -e "s#[^a-zA-Z0-9\._]*##g" <<<${CLEANURL})
FILENAME=$(sed -r -e 's#[:/]+#\.#g' -e 's#[^a-zA-Z0-9\._]*##g' <<<${CLEANURL})
HDRFILE="${FILENAME}.hdr"

: "\\# not a comment"
# \\" a comment
echo "\\#"
:
