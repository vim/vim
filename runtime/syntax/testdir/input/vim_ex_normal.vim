" Vim :normal command

normal j
normal! j
normal!j

" no trailing bar
normal j 42|echo "not echo command"

" no trailing comment
normal j "0p

" multiline arg
normal j
      \k
      "\ comment
      \j

" word-boundary required after name (old bug)
normalj
