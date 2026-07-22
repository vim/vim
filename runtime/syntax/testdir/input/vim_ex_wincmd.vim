" Vim :wincmd command
" VIM_TEST_SETUP hi link vimWincmdArg Todo
" VIM_TEST_SETUP hi link vimCmdSep Special


wincmd s
wincmd S
wincmd v
wincmd n
wincmd ^
wincmd :
wincmd q
wincmd o
wincmd j
wincmd k
wincmd h
wincmd l
wincmd w
wincmd W
wincmd t
wincmd b
wincmd p
wincmd P
wincmd r
wincmd R
wincmd x
wincmd K
wincmd J
wincmd H
wincmd L
wincmd T
wincmd =
wincmd -
wincmd +
wincmd _
wincmd <
wincmd >
wincmd |
wincmd ]
wincmd g ]
wincmd f
wincmd F
wincmd gf
wincmd gF
wincmd gt
wincmd gT
wincmd z
wincmd }
wincmd g }


wincmd | | echo "Foo"
wincmd | " comment
wincmd s | echo "Foo"
wincmd s " comment


def Vim9Context()
  var wincmd = 42
  wincmd = 42
  :wincmd =
  wincmd = # comment
  wincmd = | echo "Foo"
  # KNOWN: incorrectly matches as the Ex command rather than a variable
  wincmd =
enddef

