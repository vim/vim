" Vim :defer command
" VIM_TEST_SETUP hi link vimUserFunc Todo


function Foo()
  defer delete("tmpfile")
  defer Delete("tmpfile")
endfunction

def Bar()
  defer delete("tmpfile")
  defer Delete("tmpfile")
  defer () => {
    echo "..."
  }()
enddef

