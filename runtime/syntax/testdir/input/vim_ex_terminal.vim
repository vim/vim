" Vim :terminal command
" VIM_TEST_SETUP hi link vimTerminalCommand Todo


terminal
terminal ++kill=term tail -f /tmp/log
terminal ++hidden ++open make


" line continuations

terminal ++kill=term ++hidden tail
      \ -f
      \ /tmp/log
terminal ++kill=term ++hidden
      \ tail
      \ -f
      \ /tmp/log
terminal ++kill=term
      \ ++hidden
      \ tail
      \ -f
      \ /tmp/log
terminal
      \ ++kill=term
      \ ++hidden
      \ tail
      \ -f
      \ /tmp/log
terminal
      "\ comment
      \ ++kill=term
      "\ comment
      \ ++hidden
      "\ comment
      \ tail
      "\ comment
      \ -f
      "\ comment
      \ /tmp/log


" all options

terminal ++close ++noclose ++open ++curwin ++hidden ++norestore ++shell ++kill=term ++rows=42 ++cols=42 ++eof=exit ++type=conpty ++api=Tapi_ tail -f /tmp/log
terminal
      \ ++close
      \ ++noclose
      \ ++open
      \ ++curwin
      \ ++hidden
      \ ++norestore
      \ ++shell
      \ ++kill=term
      \ ++rows=42
      \ ++cols=42
      \ ++eof=exit
      \ ++type=conpty
      \ ++api=Tapi_
      \ tail
      \ -f
      \ /tmp/log


" escaped option prefix ++

terminal \++close
terminal ++close \++noclose

