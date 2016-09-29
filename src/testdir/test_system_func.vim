" Test systemlist()

let s:dump_cmd = has('win32') ? 'type' : 'cat'

func! s:systemlist(content) abort
  let file = tempname()
  call writefile(a:content, file, 'b')
  let cmdline = printf('%s %s', s:dump_cmd, shellescape(file))
  call assert_equal(a:content, systemlist(cmdline))
  call delete(file)
endfunc

func Test_systemlist()
  let save_shellslash = &shellslash
  set noshellslash  " Need for shellescape()

  " empty file
  call s:systemlist([''])

  " noeol file
  call s:systemlist(['foo'])

  " eol file
  call s:systemlist(['foo', ''])

  let &shellslash = save_shellslash
endfunc
