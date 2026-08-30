" Simulated Vim command-line window
" VIM_TEST_SETUP let b:vimsyn_force_cmdline_window = v:true
" VIM_TEST_SETUP let g:vimsyn_folding = "acefhiHlmpPrtT"
" VIM_TEST_SETUP setl fdc=2 fdl=999 fdm=syntax

" simple commands

echo "..."
let x = 42
call execute(":")

" region openers

append
" comment
change
" comment
insert
" comment
let x =<< EOF
" comment
let x =<< trim EOF
" comment
let x =<< eval EOF
" comment
let x =<< trim eval EOF
" comment
augroup Foo
" comment
autocmd BufRead * {
" comment
command Foo {
" comment
function Foo()
" comment
def Foo()
let x = "...
" comment
let x = (
" comment

" Region closers

      \ )
" comment
      \ ..."
" comment
enddef
" comment
endfunction
" comment
}
" comment
}
" comment
augroup END
" comment
EOF
" comment
EOF
" comment
EOF
" comment
EOF
" comment
.
" comment
.
" comment
.

