#!/bin/bash
# shTouch did not stop before a "{" or "}", so a curly block following
# a "touch" command in a list (e.g. after "||" or "&&") was swallowed
# into the shTouch match and its closing "}" was flagged as shCurlyError.
 
touch foo || { echo "touch failed" ; }
touch foo && { echo "touch ok" ; }
touch foo || { { echo nested ; } ; }
 
# unaffected: touch followed by a plain command substitution
touch "$(date +%s)"
