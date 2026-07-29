#!/bin/bash
# VIM_TEST_SETUP setlocal fen fdc=2 fdl=8 fdm=syntax
# VIM_TEST_SETUP let g:sh_fold_enabled = 1 + 2 + 4
# VIM_TEST_SETUP highlight link shFunctionExprRegion Todo
# VIM_TEST_SETUP highlight link shFunctionSubShRegion Todo
typeset -i n=0
doosie() ((n+=1)); doosie
donee() [[ -n $# ]]; donee

thence()
until :
do
    :
done
thence

whiles() while false; do :; done; whiles

elsewhere() if :
then :; fi; elsewhere

selector() select x in 1 2; do
    break
done
selector 0</dev/null 2>/dev/null || :

cased() case "$#" in *) :;; esac; cased

fore()
for x in 1 2
do
    :
done
fore

iffy() for ((;;))
do
    break
done
iffy

if :; then
    function !?#()
    (
        function @α! {
            echo "$1"
        }
        @α! "$1"
    )
    eval !?\# "\"$1\""
fi

namespace ()
{ echo $#;
}; namespace $@

# Whether "=" belongs to a name or delimits a name depends on whether
# the reserved word "function" is present, if so, then "=" is part of
# the function name; else, "=" delimits the name of a variable when this
# name is given in alphanumeric characters and "_"s before the leftmost
# "="; otherwise, "=" is part of the function name when this name has
# one or more supported NON-alphanumeric (or "_") characters before "=".
xs=()
(
    echo $(( 1 + ${#xs[*]} ))
    xs=()
    {
        echo $(( 2 + ${#xs[*]} ))
        xs=()
        if :; then echo $(( 3 + ${#xs[*]} )); fi
    }
)

iδ=() (
    =id=() {
        ===()
        if :; then echo $*; fi; === $*
    }; =id= $*
); id= iδ= iδ= iδ=

function f=() (
    function f=f {
        function f=f=
        if :; then echo $*; fi; f\=f\= $*
    }; f\=f $*
); f= f\= f= f=

#function() {
#   echo "$1"
#}

function function {
    set -- ${*/\
#./}
    set -- `printf %\
#.f\  $* 2>/dev/null`;: # FIXME: Not a comment (%\)
    set -- ${*%.}
    local IFS=+
    echo $((("${*\
#function()}") / ${\
#} + 16\
#0\
));: # FIXME: Not a comment (16\)
}; eval "\function" $@

function function#function () {
    echo "$1"
}; eval "function#function" "$1"

function# () {
    echo "$1"
}; function# "$1"

# Parens are not escaped, hence this is invalid variable assignment.
f=f()
{
    f=f=()
    (
        f=f=f()
        if :; then :; fi
    )
}

# Identifiers cannot have a leading "#" in their names unless the shell
# option "interactive_comments" is unset and is in effect.
function #function() {
    echo "$1"
}
