" Test 'statuslineopt' with 'statusline'
"

source util/screendump.vim

def SetUp()
  set laststatus=2
  set ch=1
enddef

def TearDown()
  set laststatus&
  set statusline&
  set statuslineopt&
  set ch&
  :only
enddef

def s:Assert_match_statusline(winid: number, stlh: number, expect: list<string>): void
  redraw!
  if has('gui_running')
    sleep 1m
  endif
  var wi = getwininfo(winid)[0]
  var winh = wi.winrow + wi.height
  # Read screen content directly after redraw! to avoid a second redraw!
  # inside g:ScreenLines() that may process GUI events and change the window
  # layout between the getwininfo() call and the screenstring() calls.
  var actual = mapnew(range(winh, winh + wi.status_height - 1),
      (_, l) => join(mapnew(range(1, &columns),
          (_, c) => screenstring(l, c)), '')[wi.wincol - 1 : wi.wincol - 1 + wi.width - 1])
  assert_equal(stlh, wi.status_height)
  for i in range(len(expect))
    assert_match(expect[i], actual[i], $'[{i}]')
  endfor
enddef

def Test_statuslineopt()
  set statuslineopt=maxheight:2
  &statusline = "AAA"
  var wid = win_getid()
  s:Assert_match_statusline(wid, 1, ['^AAA *'])
  &statusline = "AAA%@BBB"
  s:Assert_match_statusline(wid, 2, ['^AAA *', '^BBB *'])
  &statusline = "AAA%@BBB%@CCC"
  s:Assert_match_statusline(wid, 2, ['^AAA *', '^BBB *'])

  set statuslineopt=maxheight:3
  s:Assert_match_statusline(wid, 3, ['^AAA *', '^BBB *', '^CCC *'])
  &statusline = "AAA%@BBB"
  s:Assert_match_statusline(wid, 2, ['^AAA *', '^BBB *'])

  # Best effort
  &statusline = "AAA%@BBB%@CCC"
  set statuslineopt=maxheight:999
  s:Assert_match_statusline(wid, 3, ['^AAA *', '^BBB *', '^CCC *'])

  # Single line
  set statuslineopt=maxheight:1
  s:Assert_match_statusline(wid, 1, ['^AAA *'])
enddef

def Test_statuslineopt_fixedheight()
  set statuslineopt=maxheight:2,fixedheight
  &statusline = "AAA"
  var wid = win_getid()
  var stlh = 2
  s:Assert_match_statusline(wid, stlh, ['^AAA *', '^ *'])
  &statusline = "AAA%@BBB"
  s:Assert_match_statusline(wid, stlh, ['^AAA *', '^BBB *'])
  &statusline = "AAA%@BBB%@CCC"
  s:Assert_match_statusline(wid, stlh, ['^AAA *', '^BBB *'])

  set statuslineopt=maxheight:3,fixedheight
  stlh = 3
  s:Assert_match_statusline(wid, stlh, ['^AAA *', '^BBB *', '^CCC *'])
  &statusline = "AAA%@BBB"
  s:Assert_match_statusline(wid, stlh, ['^AAA *', '^BBB *', '^ *'])

  # Best effort
  &statusline = "AAA%@BBB%@CCC"
  set statuslineopt=maxheight:999,fixedheight
  stlh = &lines - &ch - 1
  s:Assert_match_statusline(wid, stlh, ['^AAA *', '^BBB *', '^CCC *', '^ *'])

  # Single line
  set statuslineopt=maxheight:1
  stlh = 1
  s:Assert_match_statusline(wid, stlh, ['^AAA *'])
enddef

def Test_statuslineopt_multi_win()
  &statusline = "AAA%@BBB%@BB3"
  var wid1 = win_getid()
  new ccc
  &l:statusline = "CCC%@DDD%@DD3"
  var wid2 = win_getid()
  set statuslineopt=maxheight:2
  var stlh = 2
  s:Assert_match_statusline(wid1, stlh, ['^AAA *', '^BBB *'])
  s:Assert_match_statusline(wid2, stlh, ['^CCC *', '^DDD *'])

  vnew eee
  &l:statusline = "EEE%@FFF%@FF3"
  var wid3 = win_getid()
  s:Assert_match_statusline(wid1, stlh, ['^AAA *', '^BBB *'])
  s:Assert_match_statusline(wid2, stlh, ['^CCC *', '^DDD *'])
  s:Assert_match_statusline(wid3, stlh, ['^EEE *', '^FFF *'])

  quit
  new eee
  wid3 = win_getid()
  s:Assert_match_statusline(wid1, stlh, ['^AAA *', '^BBB *'])
  s:Assert_match_statusline(wid2, stlh, ['^CCC *', '^DDD *'])
  s:Assert_match_statusline(wid3, stlh, ['^EEE *', '^FFF *'])

  # Best effort (fixedheight fills all available space)
  # Window equalization may not give exactly equal frame heights depending on
  # the terminal size, so allow a difference of 1 relative to wid1.
  set statuslineopt=maxheight:999,fixedheight
  var h1 = getwininfo(wid1)[0].height
  assert_inrange(h1 - 1, h1 + 1, getwininfo(wid2)[0].height)
  assert_inrange(h1 - 1, h1 + 1, getwininfo(wid3)[0].height)
  s:Assert_match_statusline(wid1, getwininfo(wid1)[0].status_height,
      ['^AAA *', '^BBB *', '^BB3 *', '^ *'])
  s:Assert_match_statusline(wid2, getwininfo(wid2)[0].status_height,
      ['^CCC *', '^DDD *', '^DD3 *', '^ *'])
  s:Assert_match_statusline(wid3, getwininfo(wid3)[0].status_height,
      ['^EEE *', '^FFF *', '^FF3 *', '^ *'])

  # Single line
  set statuslineopt=maxheight:1
  stlh = 1
  s:Assert_match_statusline(wid1, stlh, ['^AAA *'])
  s:Assert_match_statusline(wid2, stlh, ['^CCC *'])
  s:Assert_match_statusline(wid3, stlh, ['^EEE *'])
enddef

def Test_statuslineopt_winlocal()
  # Test window-local 'statuslineopt':
  # wid1 uses global statuslineopt (maxheight:1)
  # wid2 uses local statuslineopt (maxheight:3), both using the global statusline
  &statusline = "AAA%@BBB%@CCC"
  set statuslineopt=maxheight:1
  var wid1 = win_getid()
  new ddd
  var wid2 = win_getid()
  setlocal statuslineopt=maxheight:3

  s:Assert_match_statusline(wid1, 1, ['^AAA *'])
  s:Assert_match_statusline(wid2, 3, ['^AAA *', '^BBB *', '^CCC *'])

  # After clearing the local statuslineopt, wid2 reverts to global (maxheight:1)
  setlocal statuslineopt<
  s:Assert_match_statusline(wid1, 1, ['^AAA *'])
  s:Assert_match_statusline(wid2, 1, ['^AAA *'])

  # Window with local statusline and local statuslineopt (fixedheight)
  &l:statusline = "DDD%@EEE"
  setlocal statuslineopt=maxheight:3,fixedheight
  s:Assert_match_statusline(wid2, 3, ['^DDD *', '^EEE *', '^ *'])

  # wid1 (global settings) is unaffected by wid2's local changes
  s:Assert_match_statusline(wid1, 1, ['^AAA *'])
enddef

def Test_statuslineopt_split_inherit()
  # setl stlo follows the same inheritance rules as setl stl:
  #   :sp  - local stlo IS inherited (new window shows same buffer)
  #   :new - local stlo is NOT inherited (cleared when entering new buffer)
  set statuslineopt=maxheight:1
  &statusline = "SEG1%@SEG2%@SEG3"
  setlocal statuslineopt=maxheight:3
  var wid1 = win_getid()
  s:Assert_match_statusline(wid1, 3, ['^SEG1 *', '^SEG2 *', '^SEG3 *'])

  # :sp splits the same buffer - local stlo is inherited
  split
  var wid2 = win_getid()
  s:Assert_match_statusline(wid2, 3, ['^SEG1 *', '^SEG2 *', '^SEG3 *'])
  quit

  # :new opens a new empty buffer - local stlo is NOT inherited, falls back to
  # global (maxheight:1)
  new
  var wid3 = win_getid()
  s:Assert_match_statusline(wid3, 1, ['^SEG1 *'])
  quit
enddef

let g:StloStatusVar = ''
def g:StloStatusLine(): string
  return g:StloStatusVar
enddef

def Test_statuslineopt_expr()
  new bbb.txt
  g:StloStatusVar = 'A001%@A002%@%t'
  set statuslineopt=maxheight:3
  &statusline = "%!StloStatusLine()"
  var wid = win_getid()
  var stlh = 3
  s:Assert_match_statusline(wid, stlh, ['^A001 *', '^A002 *', '^bbb\.txt *'])

  &statusline = "%{%StloStatusLine()%}"
  s:Assert_match_statusline(wid, stlh, ['^A001 *', '^A002 *', '^bbb\.txt *'])
  g:StloStatusVar = 'B00001%@B002'
  s:Assert_match_statusline(wid, stlh, ['^B00001 *', '^B002 *', '^ *'])
enddef

func Test_multistatusline_highlight()
  CheckScreendump

  let lines =<< trim END
    func MyStatusLine()
      return 'L1A01%=%#Search#L1A02%*%=%2*L1A03%*%@'
        \ .. '%2*L2B01%*%=L2B02%=%#Search#L2B03%*%@'
        \ .. '%#Search#L3C01%*%=%2*L3C02%*%=L3C03%@'
    endfunc

    set laststatus=2
    set statuslineopt=maxheight:3
    set statusline=%!MyStatusLine()
  END
  call writefile(lines, 'XTest_multistatusline_highlight', 'D')

  let buf = g:RunVimInTerminal('-S XTest_multistatusline_highlight', {'rows': 6})
  call term_sendkeys(buf, "\<C-L>")
  call VerifyScreenDump(buf, 'Test_multistatusline_highlight_01', {})
  call term_sendkeys(buf, ":hi link User2 Error\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_multistatusline_highlight_02', {})
  call term_sendkeys(buf, ":hi link User2 NONE\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_multistatusline_highlight_01', {})

  call StopVimInTerminal(buf)
endfunc

func Test_multistatusline_carry_hl()
  CheckScreendump

  " %#XX# / %N* set on one row should persist on subsequent rows until %*
  " (or another %# / %*) changes it.
  let lines =<< trim END
    func MyStatusLine()
      return 'L1A%=%#Search#L1B%@'
        \ .. 'L2 carried Search%@'
        \ .. '%*L3 reset%@'
        \ .. '%2*L4 user2%@'
        \ .. 'L5 carried user2%@'
        \ .. '%*L6 reset'
    endfunc

    hi User2 ctermfg=Yellow ctermbg=Blue
    set laststatus=2
    set statuslineopt=maxheight:6
    set statusline=%!MyStatusLine()
  END
  call writefile(lines, 'XTest_multistatusline_carry_hl', 'D')

  let buf = g:RunVimInTerminal('-S XTest_multistatusline_carry_hl', {'rows': 9})
  call term_sendkeys(buf, "\<C-L>")
  call VerifyScreenDump(buf, 'Test_multistatusline_carry_hl_01', {})

  call StopVimInTerminal(buf)
endfunc

func Test_statuslineopt_default_stl()
  CheckScreendump

  " fixedheight with no custom stl: status area is fixed at 4 rows, filled
  " with fillchar and must not bleed through buffer content.
  let lines =<< trim END
    set laststatus=2
    set statuslineopt=maxheight:4,fixedheight
  END
  call writefile(lines, 'XTest_statuslineopt_default_stl', 'D')

  let buf = g:RunVimInTerminal('-S XTest_statuslineopt_default_stl', {'rows': 8})
  call term_sendkeys(buf, "\<C-L>")
  call VerifyScreenDump(buf, 'Test_statuslineopt_default_stl_01', {})

  call StopVimInTerminal(buf)
endfunc

def Test_statuslineopt_default_stl_maxheight()
  # maxheight without fixedheight: status area adapts to rendered content.
  # Default statusline is always single-line, so status area must be 1 row
  # regardless of maxheight value.
  set statuslineopt=maxheight:9
  var wid = win_getid()
  assert_equal(1, getwininfo(wid)[0].status_height)

  # After :new the new window also has stlo=maxheight:9 but w_height should
  # be equalized (both windows get roughly half the available content rows).
  new
  var wid2 = win_getid()
  var h1 = getwininfo(wid)[0].height
  var h2 = getwininfo(wid2)[0].height
  # Both status areas must be 1 row (not 9)
  assert_equal(1, getwininfo(wid)[0].status_height)
  assert_equal(1, getwininfo(wid2)[0].status_height)
  # Content heights must be nearly equal (differ by at most 1 due to p_wh)
  assert_true(abs(h1 - h2) <= 1, $'h1={h1} h2={h2} should be equal')
  only
enddef

func Test_statuslineopt_new_split()
  CheckScreendump

  " Test :new — local stlo is NOT inherited (new buffer), so the new window
  " uses global stlo (maxheight:1, single-line) while the original keeps
  " fixedheight,maxheight:5.
  let lines =<< trim END
    set laststatus=2
    set statuslineopt=maxheight:1
    set statusline=SEG1%@SEG2%@SEG3%@SEG4%@SEG5
    setlocal statuslineopt=fixedheight,maxheight:5
  END
  call writefile(lines, 'XTest_statuslineopt_new_split', 'D')

  let buf = g:RunVimInTerminal('-S XTest_statuslineopt_new_split', {'rows': 20})
  call term_sendkeys(buf, ":new\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_statuslineopt_new_split_01', {})

  call StopVimInTerminal(buf)
endfunc

func Test_statuslineopt_sp_split()
  CheckScreendump

  " Test :sp — local stlo IS inherited (same buffer), so both windows use
  " fixedheight,maxheight:5.
  let lines =<< trim END
    set laststatus=2
    set statusline=SEG1%@SEG2%@SEG3%@SEG4%@SEG5
    setlocal statuslineopt=fixedheight,maxheight:5
  END
  call writefile(lines, 'XTest_statuslineopt_sp_split', 'D')

  let buf = g:RunVimInTerminal('-S XTest_statuslineopt_sp_split', {'rows': 15})
  call term_sendkeys(buf, ":sp\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_statuslineopt_sp_split_01', {})

  call StopVimInTerminal(buf)
endfunc

func Test_statuslineopt_sp_foo()
  CheckScreendump

  " Test :sp foo — local stlo is NOT inherited (different file), so the new
  " window uses global stlo (maxheight:1, single-line).
  let lines =<< trim END
    set laststatus=2
    set statuslineopt=maxheight:1
    set statusline=SEG1%@SEG2%@SEG3%@SEG4%@SEG5
    setlocal statuslineopt=fixedheight,maxheight:5
  END
  call writefile(lines, 'XTest_statuslineopt_sp_foo', 'D')

  let buf = g:RunVimInTerminal('-S XTest_statuslineopt_sp_foo', {'rows': 20})
  call term_sendkeys(buf, ":sp Xfoo\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_statuslineopt_sp_foo_01', {})

  call StopVimInTerminal(buf)
endfunc

func Test_statuslineopt_wincmd_eq()
  CheckScreendump

  " Test CTRL-W_= with mixed global stlo (maxheight:3) and local stlo
  " (fixedheight,maxheight:5); equalization assigns equal content rows to
  " both windows despite their different status-line heights.
  let lines =<< trim END
    set laststatus=2
    set statuslineopt=maxheight:3
    set statusline=GA%@GB%@GC
    new
    setlocal statuslineopt=fixedheight,maxheight:5
    setlocal statusline=LA%@LB%@LC%@LD%@LE
  END
  call writefile(lines, 'XTest_statuslineopt_wincmd_eq', 'D')

  let buf = g:RunVimInTerminal('-S XTest_statuslineopt_wincmd_eq', {'rows': 12})
  call term_sendkeys(buf, "\<C-W>=\<C-L>")
  call VerifyScreenDump(buf, 'Test_statuslineopt_wincmd_eq_01', {})

  call StopVimInTerminal(buf)
endfunc

func Test_statuslineopt_wincmd_underscore()
  CheckScreendump

  " Test CTRL-W__ with global stlo maxheight:3 and multi-line statusline.
  " After maximizing, the large window should keep stlh=3, not collapse to 1
  " because the minimized other window constrained global_stlh.
  let lines =<< trim END
    set laststatus=2
    set statuslineopt=maxheight:3
    set statusline=GA%@GB%@GC
    new
  END
  call writefile(lines, 'XTest_statuslineopt_wincmd_underscore', 'D')

  let buf = g:RunVimInTerminal('-S XTest_statuslineopt_wincmd_underscore', {'rows': 14})
  call term_sendkeys(buf, "\<C-W>_\<C-L>")
  call VerifyScreenDump(buf, 'Test_statuslineopt_wincmd_underscore_01', {})

  call StopVimInTerminal(buf)
endfunc

def Test_statuslineopt_besteff_order()
  # Test the best-effort option update: keyword order is preserved and
  # duplicate maxheight: removed.

  # "fixedheight" before "maxheight:" - order must be preserved.
  :5split
  set statuslineopt=fixedheight,maxheight:9
  assert_equal('fixedheight,maxheight:5', &statuslineopt)
  set statuslineopt&
  only

  # "maxheight:" before "fixedheight" - order must be preserved.
  :5split
  set statuslineopt=maxheight:9,fixedheight
  assert_equal('maxheight:5,fixedheight', &statuslineopt)
  set statuslineopt&
  only

  # set stlo+=maxheight:N: last maxheight wins, earlier one is removed.
  :5split
  set statuslineopt=fixedheight,maxheight:2
  set statuslineopt+=maxheight:9
  assert_equal('fixedheight,maxheight:5', &statuslineopt)
  set statuslineopt&
  only

  # Duplicate maxheight: in one assignment: last wins, earlier removed.
  :5split
  set statuslineopt=maxheight:2,fixedheight,maxheight:9
  assert_equal('fixedheight,maxheight:5', &statuslineopt)
  only
enddef

" vim: shiftwidth=2 sts=2 expandtab
