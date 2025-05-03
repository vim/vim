" Vim :mzscheme and :mzfile commands
" VIM_TEST_SETUP let g:vimsyn_folding = "fm"
" VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


mzscheme << EOF
(display "MzScheme script")
EOF
 
  mzscheme << trim EOF
    (display "MzScheme script")
  EOF
 
mzscheme <<
(display "MzScheme script")
.

  mzscheme << trim
    (display "MzScheme script")
  .

function Foo()
  mzscheme << trim EOF
    (display "MzScheme script in :func")
  EOF
endfunction | call Foo()

def Bar()
  mzscheme << trim EOF
    (display "MzScheme script in :def")
  EOF
enddef | call Bar()

mzscheme (display "MzScheme statement");
      "\ comment
      \ (display "MzScheme statement again")

mzfile foo.rkt

