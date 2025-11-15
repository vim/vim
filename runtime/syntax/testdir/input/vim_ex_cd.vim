" Vim :cd commands
" VIM_TEST_SETUP hi link vimCdArg Todo


cd
cd!
cd  foo
cd! foo
cd  %:h/foo
cd! %:h/foo
cd  foo bar/baz
cd! foo bar/baz
cd  -
cd! -

cd \"foo\"bar\"
cd \|foo\|bar\|
cd \#foo\#bar\#
cd \ foo\ bar\ 
cd \\foo\\bar\\

cd  " comment
cd  | echo "..."
cd! " comment
cd! | echo "..."


function Foo()
  cd
  cd!
  cd  foo
  cd! foo
  cd  -
  cd! -

  cd  " comment
  cd  | echo "..."
  cd! " comment
  cd! | echo "..."
endfunction

def Foo()
  cd
  cd!
  cd  foo
  cd! foo
  cd  -
  cd! -

  cd  # comment
  cd  | echo "..."
  cd! # comment
  cd! | echo "..."
enddef


cd      | lcd      | tcd      | chdir      | lchdir      | tchdir      | echo "..."
cd  -   | lcd  -   | tcd  -   | chdir  -   | lchdir  -   | tchdir  -   | echo "..."
cd  foo | lcd  foo | tcd  foo | chdir  foo | lchdir  foo | tchdir  foo | echo "..."
cd!     | lcd!     | tcd!     | chdir!     | lchdir!     | tchdir!     | echo "..."
cd! -   | lcd! -   | tcd! -   | chdir! -   | lchdir! -   | tchdir! -   | echo "..."
cd! foo | lcd! foo | tcd! foo | chdir! foo | lchdir! foo | tchdir! foo | echo "..."

cd          " comment
cd!         " comment
cd      -   " comment
cd!     -   " comment
cd      foo " comment
cd!     foo " comment
lcd         " comment
lcd!        " comment
lcd     -   " comment
lcd!    -   " comment
lcd     foo " comment
lcd!    foo " comment
tcd         " comment
tcd!        " comment
tcd     -   " comment
tcd!    -   " comment
tcd     foo " comment
tcd!    foo " comment
chdir       " comment
chdir!      " comment
chdir   -   " comment
chdir!  -   " comment
chdir   foo " comment
chdir!  foo " comment
lchdir      " comment
lchdir!     " comment
lchdir  -   " comment
lchdir! -   " comment
lchdir  foo " comment
lchdir! foo " comment
tchdir      " comment
tchdir!     " comment
tchdir  -   " comment
tchdir! -   " comment
tchdir  foo " comment
tchdir! foo " comment


" Issue #17964 (Vim script highlight: endif is not highlighted after lcd)

def LcdBack()
    if get(g:, "lcd", 0)
        g:lcd = 0
        lcd -
    endif
enddef

