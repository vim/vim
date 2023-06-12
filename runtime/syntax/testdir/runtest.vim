" Runs all the syntax tests for which there is no "done/name" file.
"
" Current directory must be runtime/syntax.

" Only do this with the +eval feature
if 1

let cwd = getcwd()
if cwd !~ '[/\\]runtime[/\\]syntax\>'
  echoerr 'Current directory must be "runtime/syntax"'
  qall
endif
if !isdirectory('testdir')
  echoerr '"testdir" directory not found'
  qall
endif

" Use the script for source code screendump testing.  It sources other scripts,
" therefore we must "cd" there.
cd ../../src/testdir
source screendump.vim
exe 'cd ' .. fnameescape(cwd)

" For these tests we need to be able to run terminal Vim with 256 colors.  On
" MS-Windows the console only has 16 colors and the GUI can't run in a
" terminal.
if !CanRunVimInTerminal()
  echomsg 'Cannot make screendumps, aborting'
  qall
endif

cd testdir
if !isdirectory('done')
  call mkdir('done')
endif

set nocp
set nowrapscan
set report=9999
set modeline
set debug=throw
set nomore

au! SwapExists * call HandleSwapExists()
func HandleSwapExists()
  " Ignore finding a swap file for the test input, the user might be editing
  " it and that's OK.
  if expand('<afile>') =~ 'input[/\\].*\..*'
    let v:swapchoice = 'e'
  endif
endfunc


let failed_count = 0
for fname in glob('input/*.*', 1, 1)
  if fname =~ '\~$'
    " backup file, skip
    continue
  endif

  let linecount = readfile(fname)->len()
  let root = substitute(fname, 'input[/\\]\(.*\)\..*', '\1', '')

  " Execute the test if the "done" file does not exist of when the input file
  " is newer.
  let in_time = getftime(fname)
  let out_time = getftime('done/' .. root)
  if out_time < 0 || in_time > out_time
    for dumpname in glob('failed/' .. root .. '_\d*\.dump', 1, 1)
      call delete(dumpname)
    endfor
    call delete('done/' .. root)

    let lines =<< trim END
      syntax on
    END
    call writefile(lines, 'Xtestscript')
    let buf = RunVimInTerminal('-S Xtestscript ' .. fname, {})

    " Screendump at the start of the file: root_00.dump
    let fail = VerifyScreenDump(buf, root .. '_00', {})

    " Make a Screendump every 18 lines of the file: root_NN.dump
    let topline = 1
    let nr = 1
    while linecount - topline > 20
      let topline += 18
      call term_sendkeys(buf, printf("%dGzt", topline))
      let fail += VerifyScreenDump(buf, root .. printf('_%02d', nr), {})
      let nr += 1
    endwhile

    " Screendump at the end of the file: root_99.dump
    call term_sendkeys(buf, 'Gzb')
    let fail += VerifyScreenDump(buf, root .. '_99', {})

    call StopVimInTerminal(buf)
    call delete('Xtestscript')

    if fail == 0
      call writefile(['OK'], 'done/' . root)
      echo "Test " . root . " OK\n"
    else
      let failed_count += 1
    endif
  endif
endfor

" Matching "if 1" at the start.
endif

if failed_count > 0
  " have make report an error
  cquit
endif
qall!
