" Vim :normal command


normal j
normal! j
normal!j


" No trailing bar

normal j 42|echo "not echo command"


" No trailing comment

normal j "0p


" Multiline arg

normal j
      \k
      "\ comment
      \j


" Issue: word-boundary required after name (old bug)

normalj

