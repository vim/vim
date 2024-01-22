" Test for the --remote functionality

source check.vim
CheckFeature clientserver
CheckFeature terminal

source shared.vim
source screendump.vim
source mouse.vim
source term_util.vim

let s:remote_works = 0
let s:skip = 'Skipped: --remote feature is not possible'

" nees to be run as first test to verify, that vim --servername works
func Verify_remote_feature_works()
  CheckRunVimInTerminal
  enew
  let buf = RunVimInTerminal('--servername XVIMTEST', {'rows': 8})
  call TermWait(buf)
  let cmd = GetVimCommandCleanTerm() .. '--serverlist'
  call term_sendkeys(buf, ":r! " .. cmd .. "\<CR>")
  call TermWait(buf)
  call term_sendkeys(buf, ":w! XVimRemoteTest.txt\<CR>")
  call TermWait(buf)
  call term_sendkeys(buf, ":q\<CR>")
  call StopVimInTerminal(buf)
  bw!
  let result = readfile('XVimRemoteTest.txt')
  call delete('XVimRemoteTest.txt')
  if empty(result)
    throw s:skip
  endif
  let s:remote = 1
endfunc

call Verify_remote_feature_works()

if !s:remote
  finish
endif

let g:remote = s:remote

func Test_remote_servername()
  CheckRunVimInTerminal

  call writefile(range(1, 20), 'XTEST.txt', 'D')

  " Run Vim in a terminal and open a terminal window to run Vim in.
  let lines =<< trim END
    set wildignore=*.txt
  END
  call writefile(lines, 'XRemoteEditing.vim', 'D')
  let buf = RunVimInTerminal('--servername XVIMTEST -S XRemoteEditing.vim', {'rows': 8})
  call TermWait(buf)
  botright new
  let buf2 = RunVimInTerminal('--servername XVIMTEST --remote-silent XTEST.txt', {'rows': 5, 'wait_for_ruler': 0})
  exe buf2 .. 'bw!'
  call term_sendkeys(buf, ":3,$d\<CR>")
  call term_sendkeys(buf, ":wq!\<CR>")
  call StopVimInTerminal(buf)
  let buf_contents = readfile('XTEST.txt')
  call assert_equal(2, len(buf_contents))
  bw!
  close
endfunc

" vim: shiftwidth=2 sts=2 expandtab
