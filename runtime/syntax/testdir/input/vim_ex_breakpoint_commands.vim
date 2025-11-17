" Vim :break* commands
" VIM_TEST_SETUP hi link vimBreakpointNumber Number
" VIM_TEST_SETUP hi link vimBreakpointFunctionLine Number
" VIM_TEST_SETUP hi link vimBreakpointFileLine Number
" VIM_TEST_SETUP hi link vimBreakpointFilename Identifier
" VIM_TEST_SETUP hi link vimBreakpointFunction Type


breakadd func Foo
breakadd func 42 Foo
breakadd file foo.txt
breakadd file 42 foo.txt
breakadd here
breakadd expr g:foo

breaklist

breakdel 42
breakdel *
breakdel func Foo
breakdel func 42 Foo
breakdel file foo.txt
breakdel file 42 foo.txt

function Foo()
  breakadd func Foo
  breakadd func 42 Foo
  breakadd file foo.txt
  breakadd file 42 foo.txt
  breakadd here
  breakadd expr g:foo

  breaklist

  breakdel 42
  breakdel *
  breakdel func Foo
  breakdel func 42 Foo
  breakdel file foo.txt
  breakdel file 42 foo.txt
endfunction

def Vim9Context()
  breakadd func Foo
  breakadd func 42 Foo
  breakadd file foo.txt
  breakadd file 42 foo.txt
  breakadd here
  breakadd expr g:foo

  breaklist

  breakdel 42
  breakdel *
  breakdel func Foo
  breakdel func 42 Foo
  breakdel file foo.txt
  breakdel file 42 foo.txt
enddef


" tail comment and trailing bar

breakadd func Foo	 " comment
breakadd func 42 Foo	 " comment
breakadd file foo.txt	 " comment
breakadd file 42 foo.txt " comment
breakadd here		 " comment
breakadd expr g:foo	 " comment

breaklist		 " comment

breakdel 42		 " comment
breakdel *		 " comment
breakdel func Foo	 " comment
breakdel func 42 Foo	 " comment
breakdel file foo.txt	 " comment
breakdel file 42 foo.txt " comment

breakadd func Foo	 | echo "..."
breakadd func 42 Foo	 | echo "..."
breakadd file foo.txt	 | echo "..."
breakadd file 42 foo.txt | echo "..."
breakadd here		 | echo "..."
breakadd expr g:foo	 | echo "..."

breaklist		 | echo "..."

breakdel 42		 | echo "..."
breakdel *		 | echo "..."
breakdel func Foo	 | echo "..."
breakdel func 42 Foo	 | echo "..."
breakdel file foo.txt	 | echo "..."
breakdel file 42 foo.txt | echo "..."

