" Test for the termdebug plugin

source shared.vim
source screendump.vim
source check.vim

CheckUnix
CheckFeature terminal
CheckExecutable gdb
CheckExecutable gcc

let g:GDB = exepath('gdb')
if g:GDB->empty()
  throw 'Skipped: gdb is not found in $PATH'
endif

let g:GCC = exepath('gcc')
if g:GCC->empty()
  throw 'Skipped: gcc is not found in $PATH'
endif

function s:generate_files(bin_name)
  let src_name = a:bin_name .. '.c'
  let lines =<< trim END
    #include <stdio.h>
    #include <stdlib.h>

    int isprime(int n)
    {
      if (n <= 1)
        return 0;

      for (int i = 2; i <= n / 2; i++)
        if (n % i == 0)
          return 0;

      return 1;
    }

    int main(int argc, char *argv[])
    {
      int n = 7;

      printf("%d is %s prime\n", n, isprime(n) ? "a" : "not a");

      return 0;
    }
  END
  call writefile(lines, src_name)
  call system($'{g:GCC} -g -o {a:bin_name} {src_name}')
endfunction

function s:cleanup_files(bin_name)
  call delete(a:bin_name)
  call delete(a:bin_name .. '.c')
endfunction

packadd termdebug

func Test_termdebug_basic()
  let bin_name = 'XTD_basic'
  let src_name = bin_name .. '.c'
  call s:generate_files(bin_name)

  edit XTD_basic.c
  Termdebug ./XTD_basic
  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  let gdb_buf = winbufnr(1)
  wincmd b
  Break 9
  call term_wait(gdb_buf)
  redraw!
  call assert_equal([
        \ {'lnum': 9, 'id': 1014, 'name': 'debugBreakpoint1.0',
        \  'priority': 110, 'group': 'TermDebug'}],
        \ sign_getplaced('', #{group: 'TermDebug'})[0].signs)
  Run
  call term_wait(gdb_buf, 400)
  redraw!
  call WaitForAssert({-> assert_equal([
        \ {'lnum': 9, 'id': 12, 'name': 'debugPC', 'priority': 110,
        \  'group': 'TermDebug'},
        \ {'lnum': 9, 'id': 1014, 'name': 'debugBreakpoint1.0',
        \  'priority': 110, 'group': 'TermDebug'}],
        \ sign_getplaced('', #{group: 'TermDebug'})[0].signs)})
  Finish
  call term_wait(gdb_buf)
  redraw!
  call WaitForAssert({-> assert_equal([
        \ {'lnum': 9, 'id': 1014, 'name': 'debugBreakpoint1.0',
        \  'priority': 110, 'group': 'TermDebug'},
        \ {'lnum': 20, 'id': 12, 'name': 'debugPC',
        \  'priority': 110, 'group': 'TermDebug'}],
        \ sign_getplaced('', #{group: 'TermDebug'})[0].signs)})
  Continue
  call term_wait(gdb_buf)

  let i = 2
  while i <= 258
    Break
    call term_wait(gdb_buf)
    if i == 2
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint2.0')[0].text, '02')})
    endif
    if i == 10
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint10.0')[0].text, '0A')})
    endif
    if i == 168
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint168.0')[0].text, 'A8')})
    endif
    if i == 255
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint255.0')[0].text, 'FF')})
    endif
    if i == 256
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint256.0')[0].text, 'F+')})
    endif
    if i == 258
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint258.0')[0].text, 'F+')})
    endif
    let i += 1
  endwhile

  let cn = 0
  " 60 is approx spaceBuffer * 3
  if winwidth(0) <= 78 + 60
    Var
    call assert_equal(winnr('$'), winnr())
    call assert_equal(['col', [['leaf', 1002], ['leaf', 1001], ['leaf', 1000], ['leaf', 1003 + cn]]], winlayout())
    let cn += 1
    bw!
    Asm
    call assert_equal(winnr('$'), winnr())
    call assert_equal(['col', [['leaf', 1002], ['leaf', 1001], ['leaf', 1000], ['leaf', 1003 + cn]]], winlayout())
    let cn += 1
    bw!
  endif
  set columns=160
  call term_wait(gdb_buf)
  let winw = winwidth(0)
  Var
  if winwidth(0) < winw
    call assert_equal(winnr('$') - 1, winnr())
    call assert_equal(['col', [['leaf', 1002], ['leaf', 1001], ['row', [['leaf', 1003 + cn], ['leaf', 1000]]]]], winlayout())
    let cn += 1
    bw!
  endif
  let winw = winwidth(0)
  Asm
  if winwidth(0) < winw
    call assert_equal(winnr('$') - 1, winnr())
    call assert_equal(['col', [['leaf', 1002], ['leaf', 1001], ['row', [['leaf', 1003 + cn], ['leaf', 1000]]]]], winlayout())
    let cn += 1
    bw!
  endif
  set columns&
  call term_wait(gdb_buf)

  wincmd t
  quit!
  redraw!
  call WaitForAssert({-> assert_equal(1, winnr('$'))})
  call assert_equal([], sign_getplaced('', #{group: 'TermDebug'})[0].signs)

  for use_prompt in [v:true, v:false]
    let g:termdebug_config = {}
    let g:termdebug_config['use_prompt'] = use_prompt
    TermdebugCommand ./XTD_basic arg args
    call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
    call WaitForAssert({-> assert_equal(3, winnr('$'))})
    wincmd t
    quit!
    redraw!
    call WaitForAssert({-> assert_equal(1, winnr('$'))})
    unlet g:termdebug_config
  endfor

  call s:cleanup_files(bin_name)
  %bw!
endfunc

func Test_termdebug_decimal_breakpoints()
  let bin_name = 'example_file'
  let src_name = bin_name .. '.c'
  call s:generate_files(bin_name)

  exe "edit " .. src_name

  let g:termdebug_config = {}
  let g:termdebug_config['sign_decimal'] = 1

  exe "Termdebug " .. bin_name
  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  let gdb_buf = winbufnr(1)
  wincmd b
  Break 9
  call term_wait(gdb_buf)
  redraw!

  let i = 2
  while i <= 258
    Break
    call term_wait(gdb_buf)
    if i == 2
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint2.0')[0].text, '2')})
    endif
    if i == 10
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint10.0')[0].text, '10')})
    endif
    if i == 168
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint168.0')[0].text, '9+')})
    endif
    if i == 255
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint255.0')[0].text, '9+')})
    endif
    if i == 256
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint256.0')[0].text, '9+')})
    endif
    if i == 258
      call WaitForAssert({-> assert_equal(sign_getdefined('debugBreakpoint258.0')[0].text, '9+')})
    endif
    let i += 1
  endwhile

  wincmd t
  quit!
  redraw!
  call WaitForAssert({-> assert_equal(1, winnr('$'))})
  call assert_equal([], sign_getplaced('', #{group: 'TermDebug'})[0].signs)

  call s:cleanup_files(bin_name)
  %bw!
endfunc

func Test_termdebug_tbreak()
  let g:test_is_flaky = 1
  let bin_name = 'XTD_tbreak'
  let src_name = bin_name .. '.c'

  eval s:generate_files(bin_name)

  execute 'edit ' .. src_name
  execute 'Termdebug ./' .. bin_name

  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  let gdb_buf = winbufnr(1)
  wincmd b

  let bp_line = 22        " 'return' statement in main
  let temp_bp_line = 10   " 'if' statement in 'for' loop body
  execute "Tbreak " .. temp_bp_line
  execute "Break " .. bp_line

  call term_wait(gdb_buf)
  redraw!
  " both temporary and normal breakpoint signs were displayed...
  call assert_equal([
        \ {'lnum': temp_bp_line, 'id': 1014, 'name': 'debugBreakpoint1.0',
        \  'priority': 110, 'group': 'TermDebug'},
        \ {'lnum': bp_line, 'id': 2014, 'name': 'debugBreakpoint2.0',
        \  'priority': 110, 'group': 'TermDebug'}],
        \ sign_getplaced('', #{group: 'TermDebug'})[0].signs)

  Run
  call term_wait(gdb_buf, 400)
  redraw!
  " debugPC sign is on the line where the temp. bp was set;
  " temp. bp sign was removed after hit;
  " normal bp sign is still present
  call WaitForAssert({-> assert_equal([
        \ {'lnum': temp_bp_line, 'id': 12, 'name': 'debugPC', 'priority': 110,
        \  'group': 'TermDebug'},
        \ {'lnum': bp_line, 'id': 2014, 'name': 'debugBreakpoint2.0',
        \  'priority': 110, 'group': 'TermDebug'}],
        \ sign_getplaced('', #{group: 'TermDebug'})[0].signs)})

  Continue
  call term_wait(gdb_buf)
  redraw!
  " debugPC is on the normal breakpoint,
  " temp. bp on line 10 was only hit once
  call WaitForAssert({-> assert_equal([
        \ {'lnum': bp_line, 'id': 12, 'name': 'debugPC', 'priority': 110,
        \  'group': 'TermDebug'},
        \ {'lnum': bp_line, 'id': 2014, 'name': 'debugBreakpoint2.0',
        \  'priority': 110, 'group': 'TermDebug'}],
        \ sign_getplaced('', #{group: 'TermDebug'})[0].signs)})

  wincmd t
  quit!
  redraw!
  call WaitForAssert({-> assert_equal(1, winnr('$'))})
  call assert_equal([], sign_getplaced('', #{group: 'TermDebug'})[0].signs)

  eval s:cleanup_files(bin_name)
  %bw!
endfunc

func Test_termdebug_evaluate()
  let bin_name = 'XTD_evaluate'
  let src_name = bin_name .. '.c'
  call s:generate_files(bin_name)

  edit XTD_evaluate.c
  Termdebug ./XTD_evaluate
  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  let gdb_buf = winbufnr(1)
  wincmd b

  " return stmt in main
  Break 22
  call term_wait(gdb_buf)
  Run
  call term_wait(gdb_buf, 400)
  redraw!

  " Evaluate an expression
  Evaluate n
  call term_wait(gdb_buf)
  call assert_equal(execute('1messages')->trim(), '"n": 7')
  Evaluate argc
  call term_wait(gdb_buf)
  call assert_equal(execute('1messages')->trim(), '"argc": 1')
  Evaluate isprime(n)
  call term_wait(gdb_buf)
  call assert_equal(execute('1messages')->trim(), '"isprime(n)": 1')

  wincmd t
  quit!
  redraw!
  call s:cleanup_files(bin_name)
  %bw!
endfunc

func Test_termdebug_evaluate_in_popup()
  CheckScreendump
  let bin_name = 'XTD_evaluate_in_popup'
  let src_name = bin_name .. '.c'
  let code =<< trim END
    struct Point {
      int x;
      int y;
    };

    int main(int argc, char* argv[]) {
      struct Point p = {argc, 2};
      struct Point* p_ptr = &p;
      return 0;
    }
  END
  call writefile(code, src_name, 'D')
  call system($'{g:GCC} -g -o {bin_name} {src_name}')

  let lines =<< trim END
    edit XTD_evaluate_in_popup.c
    packadd termdebug
    let g:termdebug_config = {}
    let g:termdebug_config['evaluate_in_popup'] = v:true
    Termdebug ./XTD_evaluate_in_popup
    wincmd b
    Break 9
    Run
  END

  call writefile(lines, 'Xscript', 'D')
  let buf = RunVimInTerminal('-S Xscript', {})
  call TermWait(buf, 400)

  call term_sendkeys(buf, ":Evaluate p\<CR>")
  call TermWait(buf, 400)
  call VerifyScreenDump(buf, 'Test_termdebug_evaluate_in_popup_01', {})

  call term_sendkeys(buf, ":Evaluate p_ptr\<CR>")
  call TermWait(buf, 400)
  call VerifyScreenDump(buf, 'Test_termdebug_evaluate_in_popup_02', {})

  " Cleanup
  call term_sendkeys(buf, ":Gdb")
  call term_sendkeys(buf, ":quit!\<CR>")
  call term_sendkeys(buf, ":qa!\<CR>")
  call StopVimInTerminal(buf)
  call delete(bin_name)
  %bw!
endfunc

func Test_termdebug_mapping()
  %bw!
  call assert_true(maparg('K', 'n', 0, 1)->empty())
  call assert_true(maparg('-', 'n', 0, 1)->empty())
  call assert_true(maparg('+', 'n', 0, 1)->empty())
  Termdebug
  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  wincmd b
  call assert_false(maparg('K', 'n', 0, 1)->empty())
  call assert_false(maparg('-', 'n', 0, 1)->empty())
  call assert_false(maparg('+', 'n', 0, 1)->empty())
  call assert_false(maparg('K', 'n', 0, 1).buffer)
  call assert_false(maparg('-', 'n', 0, 1).buffer)
  call assert_false(maparg('+', 'n', 0, 1).buffer)
  call assert_equal(':Evaluate<CR>', maparg('K', 'n', 0, 1).rhs)
  wincmd t
  quit!
  redraw!
  call WaitForAssert({-> assert_equal(1, winnr('$'))})
  call assert_true(maparg('K', 'n', 0, 1)->empty())
  call assert_true(maparg('-', 'n', 0, 1)->empty())
  call assert_true(maparg('+', 'n', 0, 1)->empty())

  %bw!
  nnoremap K :echom "K"<cr>
  nnoremap - :echom "-"<cr>
  nnoremap + :echom "+"<cr>
  Termdebug
  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  wincmd b
  call assert_false(maparg('K', 'n', 0, 1)->empty())
  call assert_false(maparg('-', 'n', 0, 1)->empty())
  call assert_false(maparg('+', 'n', 0, 1)->empty())
  call assert_false(maparg('K', 'n', 0, 1).buffer)
  call assert_false(maparg('-', 'n', 0, 1).buffer)
  call assert_false(maparg('+', 'n', 0, 1).buffer)
  call assert_equal(':Evaluate<CR>', maparg('K', 'n', 0, 1).rhs)
  wincmd t
  quit!
  redraw!
  call WaitForAssert({-> assert_equal(1, winnr('$'))})
  call assert_false(maparg('K', 'n', 0, 1)->empty())
  call assert_false(maparg('-', 'n', 0, 1)->empty())
  call assert_false(maparg('+', 'n', 0, 1)->empty())
  call assert_false(maparg('K', 'n', 0, 1).buffer)
  call assert_false(maparg('-', 'n', 0, 1).buffer)
  call assert_false(maparg('+', 'n', 0, 1).buffer)
  call assert_equal(':echom "K"<cr>', maparg('K', 'n', 0, 1).rhs)

  %bw!

  " -- Test that local-buffer mappings are restored in the correct buffers --
  " local mappings for foo
  file foo
  nnoremap <buffer> K :echom "bK"<cr>
  nnoremap <buffer> - :echom "b-"<cr>
  nnoremap <buffer> + :echom "b+"<cr>

  " no mappings for 'bar'
  enew
  file bar

  " Start termdebug from foo
  buffer foo
  Termdebug
  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  wincmd b
  call assert_true(maparg('K', 'n', 0, 1).buffer)
  call assert_true(maparg('-', 'n', 0, 1).buffer)
  call assert_true(maparg('+', 'n', 0, 1).buffer)
  call assert_equal(maparg('K', 'n', 0, 1).rhs, ':echom "bK"<cr>')

  Source
  buffer bar
  call assert_false(maparg('K', 'n', 0, 1)->empty())
  call assert_false(maparg('-', 'n', 0, 1)->empty())
  call assert_false(maparg('+', 'n', 0, 1)->empty())
  call assert_true(maparg('K', 'n', 0, 1).buffer->empty())
  call assert_true(maparg('-', 'n', 0, 1).buffer->empty())
  call assert_true(maparg('+', 'n', 0, 1).buffer->empty())
  wincmd t
  quit!
  redraw!
  call WaitForAssert({-> assert_equal(1, winnr('$'))})

  " Termdebug session ended. Buffer 'bar' shall have no mappings
  call assert_true(bufname() ==# 'bar')
  call assert_false(maparg('K', 'n', 0, 1)->empty())
  call assert_false(maparg('-', 'n', 0, 1)->empty())
  call assert_false(maparg('+', 'n', 0, 1)->empty())
  call assert_true(maparg('K', 'n', 0, 1).buffer->empty())
  call assert_true(maparg('-', 'n', 0, 1).buffer->empty())
  call assert_true(maparg('+', 'n', 0, 1).buffer->empty())

  " Buffer 'foo' shall have the same mapping as before running the termdebug
  " session
  buffer foo
  call assert_true(bufname() ==# 'foo')
  call assert_true(maparg('K', 'n', 0, 1).buffer)
  call assert_true(maparg('-', 'n', 0, 1).buffer)
  call assert_true(maparg('+', 'n', 0, 1).buffer)
  call assert_equal(':echom "bK"<cr>', maparg('K', 'n', 0, 1).rhs)

  nunmap K
  nunmap +
  nunmap -
  %bw!
endfunc

function Test_termdebug_save_restore_variables()
  " saved mousemodel
  let &mousemodel=''

  " saved keys
  nnoremap K :echo "hello world!"<cr>
  let expected_map_K = maparg('K', 'n', 0 , 1)
  nnoremap + :echo "hello plus!"<cr>
  let expected_map_plus = maparg('+', 'n', 0 , 1)
  let expected_map_minus = {}

  " saved &columns
  let expected_columns = &columns

  " We want termdebug to overwrite 'K' map but not '+' map.
  let g:termdebug_config = {}
  let g:termdebug_config['map_K'] = v:true

  Termdebug
  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  call WaitForAssert({-> assert_match(&mousemodel, 'popup_setpos')})
  wincmd t
  quit!
  call WaitForAssert({-> assert_equal(1, winnr('$'))})

  call assert_true(empty(&mousemodel))

  call assert_true(empty(expected_map_minus))
  call assert_equal(expected_map_K.rhs, maparg('K', 'n', 0, 1).rhs)
  call assert_equal(expected_map_plus.rhs, maparg('+', 'n', 0, 1).rhs)

  call assert_equal(expected_columns, &columns)

  nunmap K
  nunmap +
  unlet g:termdebug_config
endfunction

function Test_termdebug_sanity_check()
  " Test if user has filename/folders with wrong names
  let g:termdebug_config = {}
  let s:dict = {'disasm_window': 'Termdebug-asm-listing', 'use_prompt': 'gdb', 'variables_window': 'Termdebug-variables-listing'}

  for key in keys(s:dict)
    let s:filename = s:dict[key]
    let g:termdebug_config[key] = v:true
    let s:error_message = "You have a file/folder named '" .. s:filename .. "'"

    " Write dummy file with bad name
    call writefile(['This', 'is', 'a', 'test'], s:filename, 'D')
    Termdebug
    call WaitForAssert({-> assert_true(execute('messages') =~ s:error_message)})
    call WaitForAssert({-> assert_equal(1, winnr('$'))})

    call delete(s:filename)
    call remove(g:termdebug_config, key)
  endfor

  unlet g:termdebug_config
endfunction

function Test_termdebug_double_termdebug_instances()
  let s:error_message = 'Terminal debugger already running, cannot run two'
  Termdebug
  call WaitForAssert({-> assert_true(get(g:, "termdebug_is_running", v:false))})
  call WaitForAssert({-> assert_equal(3, winnr('$'))})
  Termdebug
  call WaitForAssert({-> assert_true(execute('messages') =~ s:error_message)})
  wincmd t
  quit!
  call WaitForAssert({-> assert_equal(1, winnr('$'))})
  :%bw!
endfunction

function Test_termdebug_config_types()
  " TODO Remove the deprecated features after 1 Jan 2025.
  let g:termdebug_config = {}
  let s:error_message = 'Deprecation Warning:'
  call assert_true(maparg('K', 'n', 0, 1)->empty())

  for key in ['disasm_window', 'variables_window', 'map_K']
    for val in [0, 1, v:true, v:false]
      let g:termdebug_config[key] = val
      Termdebug

      " Type check: warning is displayed
      if typename(val) == 'number'
        call WaitForAssert({-> assert_true(execute('messages') =~ s:error_message)})
      endif

      " Test on g:termdebug_config keys
      if val && key != 'map_K'
        call WaitForAssert({-> assert_equal(4, winnr('$'))})
        call remove(g:termdebug_config, key)
      else
        call WaitForAssert({-> assert_equal(3, winnr('$'))})
      endif

      " Test on mapping
      if key == 'map_K'
        if val
          call assert_equal(':Evaluate<CR>', maparg('K', 'n', 0, 1).rhs)
        else
          call assert_true(maparg('K', 'n', 0, 1)->empty())
        endif
      endif

      " Shutoff termdebug
      wincmd t
      quit!
      call WaitForAssert({-> assert_equal(1, winnr('$'))})
      :%bw!

    endfor
  endfor

  unlet g:termdebug_config
endfunction

" vim: shiftwidth=2 sts=2 expandtab
