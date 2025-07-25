" Vim :profile and :profdel commands


profile  start /tmp/myprofile
profile  stop
profile  pause
profile  continue
profile  func MyFunc
profile  file MyScript.vim 
profile! file MyScript.vim
profile  dump

profdel  func MyFunc
profdel  file MyScript.vim
profdel  here


" Tail comment

profile  start /tmp/myprofile " comment
profile  stop                 " comment
profile  pause                " comment
profile  continue             " comment
profile  func MyFunc          " comment
profile  file MyScript.vim    " comment
profile! file MyScript.vim    " comment
profile  dump                 " comment

profdel  func MyFunc          " comment
profdel  file MyScript.vim    " comment
profdel  here                 " comment


" Trailing command

profile  start /tmp/myprofile | echo "Foo"
profile  stop                 | echo "Foo"
profile  pause                | echo "Foo"
profile  continue             | echo "Foo"
profile  func MyFunc          | echo "Foo"
profile  file MyScript.vim    | echo "Foo"
profile! file MyScript.vim    | echo "Foo"
profile  dump                 | echo "Foo"

profdel  func MyFunc          | echo "Foo"
profdel  file MyScript.vim    | echo "Foo"
profdel  here                 | echo "Foo"

