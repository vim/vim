" Vim :command, :delcommand and :comclear commands
" VIM_TEST_SETUP hi link vimUserCmdName Todo
" VIM_TEST_SETUP hi link vimDelcommandName Todo


" List

command
command F


" Define

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


" Multiline define

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


" Errors

command! -badattr=arguments -bang -badattr -nargs=* Foo echo "Foo"


" Delete

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


" Issue #17326 (syntax highlighting breaks with complex :s comamnd)

command -range=% -nargs=? -bang Tb {
    if "<bang>" == "!"
        :<line1>,<line2>s/\v"[^"]*"/\=substitute(submatch(0), " ",         "•", "g")/ge
    endif
    if "<args>" == ""
        :<line1>,<line2>!column -t
    else
        :<line1>,<line2>!column -t -s'<args>'
    endif
    if "<bang>" == "!"
        :<line1>,<line2>s/•/ /ge
    endif
}

command -range=% -nargs=? -bang Tb :<line1>,<line2>s/\v"[^"]*"/\=substitute(submatch(0), " ", "•", "g")/ge


" Unreported issue (:map with trailing bar in replacement text)

command! Foo
      \ map lhs rhs |
      \ abbreviate foo bar |
      \ echo "Foo"


" Issue #18414 (Syntax group vimUserCmdReplacement lacking a keepend?)

def Vim9Context()
  command! MyFunction MyFunc()
  # I am a comment

  command! ToggleWrap setlocal wrap!
  # I am a comment but I didn't get highlighted
enddef

command! MyFunction call MyFunc()
" I am a comment

command! ToggleWrap setlocal wrap!
" I am a comment but I didn't get highlighted


" Issue #18448 (comment for subsequent command is not highlighted)

def Vim9Context()
  command! -nargs=1 -complete=file Rg :term rg <args>
  # command! -nargs=1 -complete=file Rg :term ++shell rg <args>
enddef

