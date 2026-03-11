#!/bin/mksh
# VIM_TEST_SETUP setlocal fen fdc=2 fdl=8 fdm=syntax
# VIM_TEST_SETUP let g:sh_fold_enabled = 1 + 2 + 4


typeset -i10 n=0
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

if :; then
    %%%()
    (
        function @a:() {
            echo "$1"
        }
        @a: "$1"
    )
    %%% "$1"
fi
