" Test for getstacktrace() and v:stacktrace

source util/screendump.vim
import './util/vim9.vim' as v9

let s:thisfile = expand('%:p')
let s:testdir = s:thisfile->fnamemodify(':h')

func Filepath(name)
  return s:testdir .. '/' .. a:name
endfunc

func AssertStacktrace(expect, actual)
  call assert_equal(Filepath('runtest.vim'), a:actual[0]['filepath'])
  call assert_equal(a:expect, a:actual[-len(a:expect):])
endfunc

func Test_getstacktrace()
  let g:stacktrace = []
  let lines1 =<< trim [SCRIPT]
  " Xscript1
  source Xscript2
  func Xfunc1()
    " Xfunc1
    call Xfunc2()
  endfunc
  [SCRIPT]
  let lines2 =<< trim [SCRIPT]
  " Xscript2
  func Xfunc2()
    " Xfunc2
    let g:stacktrace = getstacktrace()
  endfunc
  [SCRIPT]
  call writefile(lines1, 'Xscript1', 'D')
  call writefile(lines2, 'Xscript2', 'D')
  source Xscript1
  call Xfunc1()
  call AssertStacktrace([
        \ #{funcref: funcref('Test_getstacktrace'), lnum: 38, filepath: s:thisfile},
        \ #{funcref: funcref('Xfunc1'), lnum: 5, filepath: Filepath('Xscript1')},
        \ #{funcref: funcref('Xfunc2'), lnum: 4, filepath: Filepath('Xscript2')},
        \ ], g:stacktrace)
  unlet g:stacktrace
endfunc

func Test_getstacktrace_event()
  let g:stacktrace = []
  let lines1 =<< trim [SCRIPT]
  " Xscript1
  func Xfunc()
    " Xfunc
    let g:stacktrace = getstacktrace()
  endfunc
  augroup test_stacktrace
    autocmd SourcePre * call Xfunc()
  augroup END
  [SCRIPT]
  let lines2 =<< trim [SCRIPT]
  " Xscript2
  [SCRIPT]
  call writefile(lines1, 'Xscript1', 'D')
  call writefile(lines2, 'Xscript2', 'D')
  source Xscript1
  source Xscript2
  call AssertStacktrace([
       \ #{funcref: funcref('Test_getstacktrace_event'), lnum: 65, filepath: s:thisfile},
       \ #{event: 'SourcePre Autocommands for "*"', lnum: 7, filepath: Filepath('Xscript1')},
       \ #{funcref: funcref('Xfunc'), lnum: 4, filepath: Filepath('Xscript1')},
       \ ], g:stacktrace)
  augroup test_stacktrace
    autocmd!
  augroup END
  unlet g:stacktrace
endfunc

func Test_vstacktrace()
  let lines1 =<< trim [SCRIPT]
  " Xscript1
  source Xscript2
  func Xfunc1()
    " Xfunc1
    call Xfunc2()
  endfunc
  [SCRIPT]
  let lines2 =<< trim [SCRIPT]
  " Xscript2
  func Xfunc2()
    " Xfunc2
    throw 'Exception from Xfunc2'
  endfunc
  [SCRIPT]
  call writefile(lines1, 'Xscript1', 'D')
  call writefile(lines2, 'Xscript2', 'D')
  source Xscript1
  call assert_equal([], v:stacktrace)
  try
    call Xfunc1()
  catch
    let stacktrace = v:stacktrace
    try
      call Xfunc1()
    catch
      let stacktrace_inner = v:stacktrace
    endtry
    let stacktrace_after = v:stacktrace " should be restored by the exception stack to the previous one
  endtry
  call assert_equal([], v:stacktrace)
  call AssertStacktrace([
       \ #{funcref: funcref('Test_vstacktrace'), lnum: 98, filepath: s:thisfile},
       \ #{funcref: funcref('Xfunc1'), lnum: 5, filepath: Filepath('Xscript1')},
       \ #{funcref: funcref('Xfunc2'), lnum: 4, filepath: Filepath('Xscript2')},
       \ ], stacktrace)
  call AssertStacktrace([
       \ #{funcref: funcref('Test_vstacktrace'), lnum: 102, filepath: s:thisfile},
       \ #{funcref: funcref('Xfunc1'), lnum: 5, filepath: Filepath('Xscript1')},
       \ #{funcref: funcref('Xfunc2'), lnum: 4, filepath: Filepath('Xscript2')},
       \ ], stacktrace_inner)
  call assert_equal(stacktrace, stacktrace_after)
endfunc

func Test_stacktrace_vim9()
  let lines =<< trim [SCRIPT]
  var stacktrace = getstacktrace()
  assert_notequal([], stacktrace)
  for d in stacktrace
    assert_true(has_key(d, 'lnum'))
  endfor
  try
    throw 'Exception from s:Func'
  catch
    assert_notequal([], v:stacktrace)
    assert_equal(len(stacktrace), len(v:stacktrace))
    for d in v:stacktrace
      assert_true(has_key(d, 'lnum'))
    endfor
  endtry
  call assert_equal([], v:stacktrace)
  [SCRIPT]
  call v9.CheckDefSuccess(lines)
endfunc

" Building a stacktrace at the "Executing autocommands" more prompt, before the
" autocommand has matched a pattern, must not crash.
func Test_getstacktrace_during_autocmd_prompt()
  CheckRunVimInTerminal

  let lines =<< trim [SCRIPT]
    func Cb(timer)
      " The not-yet-matched autocmd entry has no funcref, no event and an empty
      " filepath; only then does this hit the crash.
      for d in getstacktrace()
        if !has_key(d, 'funcref') && !has_key(d, 'event')
              \ && get(d, 'filepath', '') == ''
          call writefile(['ok'], 'Xstacktracedone')
        endif
      endfor
    endfunc
    augroup Test
      autocmd!
      autocmd User Foo echo 'autocmd body'
    augroup END
    func Go()
      " verbose=8 prints the "Executing autocommands" message; redraw! fixes the
      " screen origin so the fill lines make the more prompt land on it.
      set more verbose=8
      redraw!
      call timer_start(20, function('Cb'), #{repeat: -1})
      for i in range(&lines - 1)
        echom 'fill' .. i
      endfor
      doautocmd User Foo
    endfunc
  [SCRIPT]
  call writefile(lines, 'Xstacktracescript', 'D')

  let buf = RunVimInTerminal('-S Xstacktracescript', #{rows: 6})
  " Not from a timer: a nested timer callback would not fire at the more prompt.
  call term_sendkeys(buf, ":call Go()\<CR>")
  call WaitForAssert({-> assert_true(filereadable('Xstacktracedone'))})
  call assert_equal('run', job_status(term_getjob(buf)))

  " The more prompt makes a clean :qall unreliable, so stop the job.
  call job_stop(term_getjob(buf))
  call WaitForAssert({-> assert_equal('dead', job_status(term_getjob(buf)))})
  call delete('Xstacktracedone')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
