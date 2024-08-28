" Vim :command, :delcommand and :comclear commands


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

command! -addr=arguments -bang -bar -buffer -complete=arglist -count=1 -keepscript -nargs=* -range=% -register Foo
      \ echo "Foo"

command! -complete=custom,Completer1 Foo echo "Foo"
command! -complete=customlist,Completer2 Foo echo "Foo"

function Foo()
  command! Foo echo "Foo (defined in :function)"
endfunction

def Foo2()
  command! Foo echo "Foo (defined in :def)"
enddef

" multiline define

" command!
"       \ -addr=lines
"       \ -bang
"       \ -bar
"       \ -buffer
"       \ -complete=buffer
"       \ -count
"       \ -nargs=*
"       \ -range
"       \ -register
"       \ -keepscript
"       \ Foo 
"       \ echo "FOO"

" errors

command! -badattr=arguments -bang -badattr -nargs=* Foo echo "Foo"

" delete

delcommand Foo
delcommand -buffer Foo

delcommand Foo | echo "..."
delcommand -buffer Foo | echo "..."

delcommand Foo " comment
delcommand -buffer Foo " comment

comclear
comclear " comment
comclear | echo "..."


" Issue #14135

com Foo call system('ls')

