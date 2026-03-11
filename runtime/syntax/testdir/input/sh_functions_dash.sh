#!/bin/dash
# VIM_TEST_SETUP setlocal fen fdc=2 fdl=8 fdm=syntax
# VIM_TEST_SETUP let g:sh_fold_enabled = 1 + 2 + 4


thence()
until :
do
    :
done
thence

whiles() while false; do :; done; whiles

elsewhere() if :
then :; fi; elsewhere

cased() case "$#" in *) :;; esac; cased

fore()
for x in 1 2
do
    :
done
fore

if :; then
    id2()
    (
        id1() {
            echo "$1"
        }
        id1 "$1"
    )
    id2 "$1"
fi

function ()
{ echo $#;
}; function $@
