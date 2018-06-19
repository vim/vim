if exists("b:did_indent")
    finish
endif
let b:did_indent = 1

setlocal comments=:;
setlocal commentstring=;%s
setlocal formatoptions-=t
setlocal lisp

let b:undo_indent = "setl comments< commentstring< formatoptions< lisp<"
