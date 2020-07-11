" Tests for the writefile() function and some :write commands.

source check.vim
source term_util.vim

func Test_writefile()
  let f = tempname()
  call writefile(["over","written"], f, "b")
  call writefile(["hello","world"], f, "b")
  call writefile(["!", "good"], f, "a")
  call writefile(["morning"], f, "ab")
  call writefile(["", "vimmers"], f, "ab")
  let l = readfile(f)
  call assert_equal("hello", l[0])
  call assert_equal("world!", l[1])
  call assert_equal("good", l[2])
  call assert_equal("morning", l[3])
  call assert_equal("vimmers", l[4])
  call delete(f)

  call assert_fails('call writefile("text", "Xfile")', 'E475: Invalid argument: writefile() first argument must be a List or a Blob')
endfunc

func Test_writefile_ignore_regexp_error()
  write Xt[z-a]est.txt
  call delete('Xt[z-a]est.txt')
endfunc

func Test_writefile_fails_gently()
  call assert_fails('call writefile(["test"], "Xfile", [])', 'E730:')
  call assert_false(filereadable("Xfile"))
  call delete("Xfile")

  call assert_fails('call writefile(["test", [], [], [], "tset"], "Xfile")', 'E730:')
  call assert_false(filereadable("Xfile"))
  call delete("Xfile")

  call assert_fails('call writefile([], "Xfile", [])', 'E730:')
  call assert_false(filereadable("Xfile"))
  call delete("Xfile")

  call assert_fails('call writefile([], [])', 'E730:')
endfunc

func Test_writefile_fails_conversion()
  if !has('iconv') || has('sun')
    return
  endif
  " Without a backup file the write won't happen if there is a conversion
  " error.
  set nobackup nowritebackup backupdir=. backupskip=
  new
  let contents = ["line one", "line two"]
  call writefile(contents, 'Xfile')
  edit Xfile
  call setline(1, ["first line", "cannot convert \u010b", "third line"])
  call assert_fails('write ++enc=cp932', 'E513:')
  call assert_equal(contents, readfile('Xfile'))

  call delete('Xfile')
  bwipe!
  set backup& writebackup& backupdir&vim backupskip&vim
endfunc

func Test_writefile_fails_conversion2()
  if !has('iconv') || has('sun')
    return
  endif
  " With a backup file the write happens even if there is a conversion error,
  " but then the backup file must remain
  set nobackup writebackup backupdir=. backupskip=
  let contents = ["line one", "line two"]
  call writefile(contents, 'Xfile_conversion_err')
  edit Xfile_conversion_err
  call setline(1, ["first line", "cannot convert \u010b", "third line"])
  set fileencoding=latin1
  let output = execute('write')
  call assert_match('CONVERSION ERROR', output)
  call assert_equal(contents, readfile('Xfile_conversion_err~'))

  call delete('Xfile_conversion_err')
  call delete('Xfile_conversion_err~')
  bwipe!
  set backup& writebackup& backupdir&vim backupskip&vim
endfunc

func SetFlag(timer)
  let g:flag = 1
endfunc

func Test_write_quit_split()
  " Prevent exiting by splitting window on file write.
  augroup testgroup
    autocmd BufWritePre * split
  augroup END
  e! Xfile
  call setline(1, 'nothing')
  wq

  if has('timers')
    " timer will not run if "exiting" is still set
    let g:flag = 0
    call timer_start(1, 'SetFlag')
    sleep 50m
    call assert_equal(1, g:flag)
    unlet g:flag
  endif
  au! testgroup
  bwipe Xfile
  call delete('Xfile')
endfunc

func Test_nowrite_quit_split()
  " Prevent exiting by opening a help window.
  e! Xfile
  help
  wincmd w
  exe winnr() . 'q'

  if has('timers')
    " timer will not run if "exiting" is still set
    let g:flag = 0
    call timer_start(1, 'SetFlag')
    sleep 50m
    call assert_equal(1, g:flag)
    unlet g:flag
  endif
  bwipe Xfile
endfunc

func Test_writefile_sync_arg()
  " This doesn't check if fsync() works, only that the argument is accepted.
  call writefile(['one'], 'Xtest', 's')
  call writefile(['two'], 'Xtest', 'S')
  call delete('Xtest')
endfunc

func Test_writefile_sync_dev_stdout()
  if !has('unix')
    return
  endif
  if filewritable('/dev/stdout')
    " Just check that this doesn't cause an error.
    call writefile(['one'], '/dev/stdout')
  else
    throw 'Skipped: /dev/stdout is not writable'
  endif
endfunc

func Test_writefile_autowrite()
  set autowrite
  new
  next Xa Xb Xc
  call setline(1, 'aaa')
  next
  call assert_equal(['aaa'], readfile('Xa'))
  call setline(1, 'bbb')
  call assert_fails('edit XX')
  call assert_false(filereadable('Xb'))

  set autowriteall
  edit XX
  call assert_equal(['bbb'], readfile('Xb'))

  bwipe!
  call delete('Xa')
  call delete('Xb')
  set noautowrite
endfunc

func Test_writefile_autowrite_nowrite()
  set autowrite
  new
  next Xa Xb Xc
  set buftype=nowrite
  call setline(1, 'aaa')
  let buf = bufnr('%')
  " buffer contents silently lost
  edit XX
  call assert_false(filereadable('Xa'))
  rewind
  call assert_equal('', getline(1))

  bwipe!
  set noautowrite
endfunc

" Test for ':w !<cmd>' to pipe lines from the current buffer to an external
" command.
func Test_write_pipe_to_cmd()
  CheckUnix
  new
  call setline(1, ['L1', 'L2', 'L3', 'L4'])
  2,3w !cat > Xfile
  call assert_equal(['L2', 'L3'], readfile('Xfile'))
  close!
  call delete('Xfile')
endfunc

" Test for :saveas
func Test_saveas()
  call assert_fails('saveas', 'E471:')
  call writefile(['L1'], 'Xfile')
  new Xfile
  new
  call setline(1, ['L1'])
  call assert_fails('saveas Xfile', 'E139:')
  close!
  enew | only
  call delete('Xfile')
endfunc

func Test_write_errors()
  " Test for writing partial buffer
  call writefile(['L1', 'L2', 'L3'], 'Xfile')
  new Xfile
  call assert_fails('1,2write', 'E140:')
  close!

  call assert_fails('w > Xtest', 'E494:')
 
  " Try to overwrite a directory
  if has('unix')
    call mkdir('Xdir1')
    call assert_fails('write Xdir1', 'E17:')
    call delete('Xdir1', 'd')
  endif

  " Test for :wall for a buffer with no name
  enew | only
  call setline(1, ['L1'])
  call assert_fails('wall', 'E141:')
  enew!

  " Test for writing a 'readonly' file
  new Xfile
  set readonly
  call assert_fails('write', 'E45:')
  close

  " Test for writing to a read-only file
  new Xfile
  call setfperm('Xfile', 'r--r--r--')
  call assert_fails('write', 'E505:')
  call setfperm('Xfile', 'rw-rw-rw-')
  close

  call delete('Xfile')

  call writefile(test_null_list(), 'Xfile')
  call assert_false(filereadable('Xfile'))
  call writefile(test_null_blob(), 'Xfile')
  call assert_false(filereadable('Xfile'))
  call assert_fails('call writefile([], "")', 'E482:')

  " very long file name
  let long_fname = repeat('n', 5000)
  call assert_fails('exe "w " .. long_fname', 'E75:')
  call assert_fails('call writefile([], long_fname)', 'E482:')
endfunc

" Test for writing to a file which is modified after Vim read it
func Test_write_file_mtime()
  CheckEnglish
  CheckRunVimInTerminal

  " First read the file into a buffer
  call writefile(["Line1", "Line2"], 'Xfile')
  let old_ftime = getftime('Xfile')
  let buf = RunVimInTerminal('Xfile', #{rows : 10})
  call term_wait(buf)
  call term_sendkeys(buf, ":set noswapfile\<CR>")
  call term_wait(buf)

  " Modify the file directly.  Make sure the file modification time is
  " different. Note that on Linux/Unix, the file is considered modified
  " outside, only if the difference is 2 seconds or more
  sleep 1
  call writefile(["Line3", "Line4"], 'Xfile')
  let new_ftime = getftime('Xfile')
  while new_ftime - old_ftime < 2
    sleep 100m
    call writefile(["Line3", "Line4"], 'Xfile')
    let new_ftime = getftime('Xfile')
  endwhile

  " Try to overwrite the file and check for the prompt
  call term_sendkeys(buf, ":w\<CR>")
  call term_wait(buf)
  call WaitForAssert({-> assert_equal("WARNING: The file has been changed since reading it!!!", term_getline(buf, 9))})
  call assert_equal("Do you really want to write to it (y/n)?",
        \ term_getline(buf, 10))
  call term_sendkeys(buf, "n\<CR>")
  call term_wait(buf)
  call assert_equal(new_ftime, getftime('Xfile'))
  call term_sendkeys(buf, ":w\<CR>")
  call term_wait(buf)
  call term_sendkeys(buf, "y\<CR>")
  call term_wait(buf)
  call WaitForAssert({-> assert_equal('Line2', readfile('Xfile')[1])})

  " clean up
  call StopVimInTerminal(buf)
  call delete('Xfile')
endfunc

" Test for an autocmd unloading a buffer during a write command
func Test_write_autocmd_unloadbuf_lockmark()
  augroup WriteTest
    autocmd BufWritePre Xfile enew | write
  augroup END
  e Xfile
  call assert_fails('lockmarks write', ['E32', 'E203:'])
  augroup WriteTest
    au!
  augroup END
  augroup! WriteTest
endfunc

" Test for writing a buffer with 'acwrite' but without autocmds
func Test_write_acwrite_error()
  new Xfile
  call setline(1, ['line1', 'line2', 'line3'])
  set buftype=acwrite
  call assert_fails('write', 'E676:')
  call assert_fails('1,2write!', 'E676:')
  call assert_fails('w >>', 'E676:')
  close!
endfunc

" Test for adding and removing lines from an autocmd when writing a buffer
func Test_write_autocmd_add_remove_lines()
  new Xfile
  call setline(1, ['aaa', 'bbb', 'ccc', 'ddd'])

  " Autocmd deleting lines from the file when writing a partial file
  augroup WriteTest2
    au!
    autocmd FileWritePre Xfile 1,2d
  augroup END
  call assert_fails('2,3w!', 'E204:')

  " Autocmd adding lines to a file when writing a partial file
  augroup WriteTest2
    au!
    autocmd FileWritePre Xfile call append(0, ['xxx', 'yyy'])
  augroup END
  %d
  call setline(1, ['aaa', 'bbb', 'ccc', 'ddd'])
  1,2w!
  call assert_equal(['xxx', 'yyy', 'aaa', 'bbb'], readfile('Xfile'))

  " Autocmd deleting lines from the file when writing the whole file
  augroup WriteTest2
    au!
    autocmd BufWritePre Xfile 1,2d
  augroup END
  %d
  call setline(1, ['aaa', 'bbb', 'ccc', 'ddd'])
  w
  call assert_equal(['ccc', 'ddd'], readfile('Xfile'))

  augroup WriteTest2
    au!
  augroup END
  augroup! WriteTest2

  close!
  call delete('Xfile')
endfunc

" Test for writing to a readonly file
func Test_write_readonly()
  " In Cirrus-CI, the freebsd tests are run under a root account. So this test
  " doesn't fail.
  CheckNotBSD
  call writefile([], 'Xfile')
  call setfperm('Xfile', "r--------")
  edit Xfile
  set noreadonly
  call assert_fails('write', 'E505:')
  let save_cpo = &cpo
  set cpo+=W
  call assert_fails('write!', 'E504:')
  let &cpo = save_cpo
  call setline(1, ['line1'])
  write!
  call assert_equal(['line1'], readfile('Xfile'))
  call delete('Xfile')
endfunc

" Test for 'patchmode'
func Test_patchmode()
  CheckNotBSD
  call writefile(['one'], 'Xfile')
  set patchmode=.orig nobackup writebackup
  new Xfile
  call setline(1, 'two')
  " first write should create the .orig file
  write
  " TODO: Xfile.orig is not created in Cirrus FreeBSD CI test
  call assert_equal(['one'], readfile('Xfile.orig'))
  call setline(1, 'three')
  " subsequent writes should not create/modify the .orig file
  write
  call assert_equal(['one'], readfile('Xfile.orig'))
  set patchmode& backup& writebackup&
  call delete('Xfile')
  call delete('Xfile.orig')
endfunc

" Test for writing to a file in a readonly directory
func Test_write_readonly_dir()
  if !has('unix') || has('bsd')
    " On MS-Windows, modifying files in a read-only directory is allowed.
    " In Cirrus-CI for Freebsd, tests are run under a root account where
    " modifying files in a read-only directory are allowed.
    return
  endif
  call mkdir('Xdir')
  call writefile(['one'], 'Xdir/Xfile1')
  call setfperm('Xdir', 'r-xr--r--')
  " try to create a new file in the directory
  new Xdir/Xfile2
  call setline(1, 'two')
  call assert_fails('write', 'E212:')
  " try to create a backup file in the directory
  edit! Xdir/Xfile1
  set backupdir=./Xdir
  set patchmode=.orig
  call assert_fails('write', 'E509:')
  call setfperm('Xdir', 'rwxr--r--')
  call delete('Xdir', 'rf')
  set backupdir& patchmode&
endfunc

" Test for writing a file using invalid file encoding
func Test_write_invalid_encoding()
  new
  call setline(1, 'abc')
  call assert_fails('write ++enc=axbyc Xfile', 'E213:')
  close!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
