" Vim :helpgrep command
" VIM_TEST_SETUP hi link vimHelpgrepPattern Todo


helpgrep :help
helpgrep :help@en
helpgrep :h\%(elp\)\=

lhelpgrep :help
lhelpgrep :help@en
lhelpgrep :h\%(elp\)\=


" no tail comment or trailing bar

helpgrep :help " not a comment
helpgrep :help | not a command

