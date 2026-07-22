" Print py3 interface versions. Part 2.
" This is done separately from part 1 because Vim cannot concurrently load
" Python 2 and 3 together.

if 1 " This prevents it from being run in tiny versions
  execute 'source' expand('<sfile>:h') .. '/if_ver-cmd.vim'

  echo 'Python 3:'
  PrintVer python3 print(sys.version)
endif
