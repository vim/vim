" Vim :command, :delcommand and :comclear commands
" VIM_TEST_SETUP highlight link vimUserCmdName Todo
" VIM_TEST_SETUP highlight link vimDelcommandName Todo


" list

command
command F


" define

command  Foo echo "Foo"
command! Foo echo "Foo"

command! Foo echo "Foo" | echo "Bar"

command! Foo {
  echo "Foo"
  echo "Bar"
  echo "Baz"
}

command! -addr=arguments -bang -bar -buffer -complete=arglist -count=1 -keepscript -nargs=* -range=% -register Foo echo "Foo"

command! -addr=arguments -bang -bar -buffer -complete=arglist -count=1 -keepscript -nargs=* -range=% -register Foo
      \ echo "Foo"

command! -addr=arguments -bang -bar -buffer -complete=arglist -count=1 -keepscript -nargs=* -range=% -register
      \ Foo
      \ echo "Foo"

command! -addr=arguments -bang -bar -buffer -complete=arglist -count=1 -keepscript -nargs=* -range=% -register Foo
      "\ comment
      \ echo "Foo"

command! -addr=arguments -bang -bar -buffer -complete=arglist -count=1 -keepscript -nargs=* -range=% -register
      "\ comment
      \ Foo
      "\ comment
      \ echo "Foo"

command! -complete=custom,s:Completer1 Foo echo "Foo"
command! -complete=customlist,s:Completer2 Foo echo "Foo"

function Foo()
  command! Foo echo "Foo (defined in :function)"
endfunction

def Foo2()
  command! Foo echo "Foo (defined in :def)"
enddef


" multiline define

command! -addr=lines
      \ -bang
      \ -bar
      \ -buffer
      \ -complete=buffer
      \ -count
      \ -nargs=*
      \ -range
      \ -register
      \ -keepscript
      \ Foo
      \ echo "Foo" |
      \ echo "Bar"

command!
      \ -addr=lines
      \ -bang
      \ -bar
      \ -buffer
      \ -complete=buffer
      \ -count
      \ -nargs=*
      \ -range
      \ -register
      \ -keepscript
      \ Foo
      \ echo "Foo" |
      \ echo "Bar"

command!
      "\ comment
      \ -addr=lines
      \ -bang
      "\ comment
      "\ comment
      \ -bar
      \ -buffer
      "\ comment
      \ -complete=buffer
      "\ comment
      \ -count
      "\ comment
      \ -nargs=*
      "\ comment
      \ -range
      "\ comment
      \ -register
      "\ comment
      \ -keepscript
      "\ comment
      \ Foo
      "\ comment
      \ echo "Foo" |
      "\ comment
      \ echo "Bar"


" errors

command! -badattr=arguments -bang -badattr -nargs=* Foo echo "Foo"


" delete

delcommand Foo
delcommand -buffer Foo

delcommand Foo | echo "Foo"
delcommand -buffer Foo | echo "Foo"

delcommand Foo " comment
delcommand -buffer Foo " comment

comclear
comclear " comment
comclear | echo "Foo"


" Issue #14135 (vim.vim syntax highlighting broken wrt system())

com Foo call system('ls')


" Issue #17001 (Wrong vimUserCmdAttrError highlighting in vim.vim)

command! -bang -nargs=* -complete=file Make AsyncRun -program=make @ <args>

