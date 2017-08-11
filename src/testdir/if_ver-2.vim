" Print py3 interface version and write the result into if_ver.txt.
" For Ubuntu. Part 2.

redir! >> if_ver.txt
if 1
  echo "\nPython 3:"
  python3 import sys; print(sys.version)
  echo "\n"
endif
redir END
