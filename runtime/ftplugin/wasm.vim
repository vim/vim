if exists("b:did_ftplugin")
    finish
endif

setl comments=:;
setl define=^\\s*(\\%(func\\|module\\)\\s\\+
setl formatoptions-=t
setl lisp
setl commentstring=;%s
setl comments^=:;;;,:;;
setl iskeyword+=$,.

let b:undo_ftplugin = "setlocal comments< define< formatoptions< lisp< commentstring< iskeyword<"
