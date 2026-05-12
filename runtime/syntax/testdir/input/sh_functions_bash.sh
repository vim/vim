#!/bin/bash
# VIM_TEST_SETUP setlocal fen fdc=2 fdl=8 fdm=syntax
# VIM_TEST_SETUP let g:sh_fold_enabled = 1 + 2 + 4


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

# Parens are not escaped, hence this is invalid variable assignment.
f=f()
{
    f=f=()
    (
        f=f=f()
        if :; then :; fi
    )
}
