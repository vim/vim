" Vim :filetype command


filetype
filetype on
filetype off
filetype plugin on
filetype plugin off
filetype indent on
filetype indent off
filetype plugin indent on
filetype plugin indent off

filetype | echo "Foo"
filetype " comment

function Foo()
  filetype
  filetype on
  filetype off
  filetype plugin on
  filetype plugin off
  filetype indent on
  filetype indent off
  filetype plugin indent on
  filetype plugin indent off
  filetype | echo "Foo"
  filetype " comment
endfunction

