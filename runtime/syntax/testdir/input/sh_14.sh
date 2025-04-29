#!/bin/bash
# Issue #17221 (sh syntax: escaped square brackets don't work in [[ ]])

[[ foo == [bar] ]]
[[ foo == \[bar\] ]]

echo [foo]
echo \[foo\]
