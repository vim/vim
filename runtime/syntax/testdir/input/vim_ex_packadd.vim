" Vim :packadd command


packadd  foo/bar
packadd  foo/bar " comment
packadd  foo/bar | echo "..."
packadd! foo/bar
packadd! foo/bar " comment
packadd! foo/bar | echo "..."


def Vim9Context()
  packadd  foo/bar
  packadd  foo/bar # comment
  packadd  foo/bar | echo "..."
  packadd! foo/bar
  packadd! foo/bar # comment
  packadd! foo/bar | echo "..."
enddef

