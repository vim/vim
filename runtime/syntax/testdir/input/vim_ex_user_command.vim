" Vim user commands
" VIM_TEST_SETUP hi link vimUsrCmd vimCommand
" VIM_TEST_SETUP hi link vimUsrCmd_ vimCommand
" VIM_TEST_SETUP hi link vimUsrCmdArgs Todo

Foo
:Foo
42Foo
:42Foo

echo "..." | Foo
echo "..." | :Foo
echo "..." | 42Foo
echo "..." | :42Foo

Foo!
Foo! arg1
     "\ comment
      \ arg2
echo "..."

Foo arg1
    "\ comment
    \ arg2
echo "..."

Foo | trailing bar not supported
Foo " tail comments not supported

def Vim9Context()
  Foo
  :Foo
  :42Foo

  echo "..." | Foo
  echo "..." | :Foo
  echo "..." | :42Foo

  Foo!
  Foo! arg1
       #\ comment
	\ arg2
  echo "..."

  Foo arg1
      #\ comment
      \ arg2
  echo "..."

  Foo | trailing bar not supported
  Foo # tail comments not supported
enddef


" Issue #20854 (Custom command call with the dot in the end breaks next
"   highlighting)

def RunOdin()
    update
    if !exists("$WSL_DISTRO_NAME")
        Term! odin run .
    else
        Term! odin run . -thread-count:1
    endif
enddef

