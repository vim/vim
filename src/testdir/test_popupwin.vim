" Tests for popup windows

if !has('textprop')
  finish
endif

source screendump.vim

func Test_simple_popup()
  if !CanRunVimInTerminal()
    return
  endif
  call writefile([
	\ "call setline(1, range(1, 100))",
	\ "hi PopupColor1 ctermbg=lightblue",
	\ "hi PopupColor2 ctermbg=lightcyan",
	\ "hi Comment ctermfg=red",
	\ "call prop_type_add('comment', {'highlight': 'Comment'})",
	\ "let winid = popup_create('hello there', {'line': 3, 'col': 11, 'minwidth': 20, 'highlight': 'PopupColor1'})",
	\ "let winid2 = popup_create(['another one', 'another two', 'another three'], {'line': 3, 'col': 25, 'minwidth': 20})",
	\ "call setwinvar(winid2, '&wincolor', 'PopupColor2')",
	\], 'XtestPopup')
  let buf = RunVimInTerminal('-S XtestPopup', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_01', {})

  " Add a tabpage
  call term_sendkeys(buf, ":tabnew\<CR>")
  call term_sendkeys(buf, ":let popupwin = popup_create(["
	\ .. "{'text': 'other tab'},"
	\ .. "{'text': 'a comment line', 'props': [{"
	\ .. "'col': 3, 'length': 7, 'minwidth': 20, 'type': 'comment'"
	\ .. "}]},"
	\ .. "], {'line': 4, 'col': 9, 'minwidth': 20})\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_02', {})

  " switch back to first tabpage
  call term_sendkeys(buf, "gt")
  call VerifyScreenDump(buf, 'Test_popupwin_03', {})

  " close that tabpage
  call term_sendkeys(buf, ":quit!\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_04', {})

  " resize popup, show empty line at bottom
  call term_sendkeys(buf, ":call popup_move(popupwin, {'minwidth': 15, 'maxwidth': 25, 'minheight': 3, 'maxheight': 5})\<CR>")
  call term_sendkeys(buf, ":redraw\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_05', {})

  " show not fitting line at bottom
  call term_sendkeys(buf, ":call setbufline(winbufnr(popupwin), 3, 'this line will not fit here')\<CR>")
  call term_sendkeys(buf, ":redraw\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_06', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopup')
endfunc

func Test_popup_with_border_and_padding()
  if !CanRunVimInTerminal()
    return
  endif

  for iter in range(0, 1)
    call writefile([iter == 1 ? '' : 'set enc=latin1',
	  \ "call setline(1, range(1, 100))",
	  \ "call popup_create('hello border', {'line': 2, 'col': 3, 'border': []})",
	  \ "call popup_create('hello padding', {'line': 2, 'col': 23, 'padding': []})",
	  \ "call popup_create('hello both', {'line': 2, 'col': 43, 'border': [], 'padding': []})",
	  \ "call popup_create('border TL', {'line': 6, 'col': 3, 'border': [1, 0, 0, 4]})",
	  \ "call popup_create('paddings', {'line': 6, 'col': 23, 'padding': [1, 3, 2, 4]})",
	  \], 'XtestPopupBorder')
    let buf = RunVimInTerminal('-S XtestPopupBorder', {'rows': 15})
    call VerifyScreenDump(buf, 'Test_popupwin_2' .. iter, {})

    call StopVimInTerminal(buf)
    call delete('XtestPopupBorder')
  endfor

  call writefile([
	\ "call setline(1, range(1, 100))",
	\ "hi BlueColor ctermbg=lightblue",
	\ "hi TopColor ctermbg=253",
	\ "hi RightColor ctermbg=245",
	\ "hi BottomColor ctermbg=240",
	\ "hi LeftColor ctermbg=248",
	\ "call popup_create('hello border', {'line': 2, 'col': 3, 'border': [], 'borderhighlight': ['BlueColor']})",
	\ "call popup_create(['hello border', 'and more'], {'line': 2, 'col': 23, 'border': [], 'borderhighlight': ['TopColor', 'RightColor', 'BottomColor', 'LeftColor']})",
	\ "call popup_create(['hello border', 'lines only'], {'line': 2, 'col': 43, 'border': [], 'borderhighlight': ['BlueColor'], 'borderchars': ['x']})",
	\ "call popup_create(['hello border', 'with corners'], {'line': 2, 'col': 60, 'border': [], 'borderhighlight': ['BlueColor'], 'borderchars': ['x', '#']})",
	\ "call popup_create(['hello border', 'with numbers'], {'line': 6, 'col': 3, 'border': [], 'borderhighlight': ['BlueColor'], 'borderchars': ['0', '1', '2', '3', '4', '5', '6', '7']})",
	\ "call popup_create(['hello border', 'just blanks'], {'line': 7, 'col': 23, 'border': [], 'borderhighlight': ['BlueColor'], 'borderchars': [' ']})",
	\], 'XtestPopupBorder')
  let buf = RunVimInTerminal('-S XtestPopupBorder', {'rows': 12})
  call VerifyScreenDump(buf, 'Test_popupwin_22', {})

  call StopVimInTerminal(buf)
  call delete('XtestPopupBorder')

  let with_border_or_padding = {
	\ 'line': 2,
	\ 'core_line': 3,
	\ 'col': 3,
	\ 'core_col': 4,
	\ 'width': 14,
	\ 'core_width': 12,
	\ 'height': 3,
	\ 'core_height': 1,
	\ 'visible': 1}
  let winid = popup_create('hello border', {'line': 2, 'col': 3, 'border': []})",
  call assert_equal(with_border_or_padding, popup_getpos(winid))

  let winid = popup_create('hello paddng', {'line': 2, 'col': 3, 'padding': []})
  call assert_equal(with_border_or_padding, popup_getpos(winid))

  let winid = popup_create('hello both', {'line': 3, 'col': 8, 'border': [], 'padding': []})
  call assert_equal({
	\ 'line': 3,
	\ 'core_line': 5,
	\ 'col': 8,
	\ 'core_col': 10,
	\ 'width': 14,
	\ 'core_width': 10,
	\ 'height': 5,
	\ 'core_height': 1,
	\ 'visible': 1}, popup_getpos(winid))
endfunc

func Test_popup_with_syntax_win_execute()
  if !CanRunVimInTerminal()
    return
  endif
  call writefile([
	\ "call setline(1, range(1, 100))",
	\ "hi PopupColor ctermbg=lightblue",
	\ "let winid = popup_create([",
	\ "\\ '#include <stdio.h>',",
	\ "\\ 'int main(void)',",
	\ "\\ '{',",
	\ "\\ '    printf(123);',",
	\ "\\ '}',",
	\ "\\], {'line': 3, 'col': 25, 'highlight': 'PopupColor'})",
	\ "call win_execute(winid, 'set syntax=cpp')",
	\], 'XtestPopup')
  let buf = RunVimInTerminal('-S XtestPopup', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_10', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopup')
endfunc

func Test_popup_with_syntax_setbufvar()
  if !CanRunVimInTerminal()
    return
  endif
  let lines =<< trim END
	call setline(1, range(1, 100))
	hi PopupColor ctermbg=lightgrey
	let winid = popup_create([
	    \ '#include <stdio.h>',
	    \ 'int main(void)',
	    \ '{',
	    \ '    printf(567);',
	    \ '}',
	    \], {'line': 3, 'col': 21, 'highlight': 'PopupColor'})
	call setbufvar(winbufnr(winid), '&syntax', 'cpp')
  END
  call writefile(lines, 'XtestPopup')
  let buf = RunVimInTerminal('-S XtestPopup', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_11', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopup')
endfunc

func Test_popup_all_corners()
  if !CanRunVimInTerminal()
    return
  endif
  let lines =<< trim END
	call setline(1, repeat([repeat('-', 60)], 15))
	set so=0
	normal 2G3|r#
	let winid1 = popup_create(['first', 'second'], {
	      \ 'line': 'cursor+1',
	      \ 'col': 'cursor',
	      \ 'pos': 'topleft',
	      \ 'border': [],
	      \ 'padding': [],
	      \ })
	normal 25|r@
	let winid1 = popup_create(['First', 'SeconD'], {
	      \ 'line': 'cursor+1',
	      \ 'col': 'cursor',
	      \ 'pos': 'topright',
	      \ 'border': [],
	      \ 'padding': [],
	      \ })
	normal 9G29|r%
	let winid1 = popup_create(['fiRSt', 'seCOnd'], {
	      \ 'line': 'cursor-1',
	      \ 'col': 'cursor',
	      \ 'pos': 'botleft',
	      \ 'border': [],
	      \ 'padding': [],
	      \ })
	normal 51|r&
	let winid1 = popup_create(['FIrsT', 'SEcoND'], {
	      \ 'line': 'cursor-1',
	      \ 'col': 'cursor',
	      \ 'pos': 'botright',
	      \ 'border': [],
	      \ 'padding': [],
	      \ })
  END
  call writefile(lines, 'XtestPopupCorners')
  let buf = RunVimInTerminal('-S XtestPopupCorners', {'rows': 12})
  call VerifyScreenDump(buf, 'Test_popupwin_corners', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupCorners')
endfunc

func Test_win_execute_closing_curwin()
  split
  let winid = popup_create('some text', {})
  call assert_fails('call win_execute(winid, winnr() .. "close")', 'E994')
  popupclear
endfunc

func Test_win_execute_not_allowed()
  let winid = popup_create('some text', {})
  call assert_fails('call win_execute(winid, "split")', 'E994:')
  call assert_fails('call win_execute(winid, "vsplit")', 'E994:')
  call assert_fails('call win_execute(winid, "close")', 'E994:')
  call assert_fails('call win_execute(winid, "bdelete")', 'E994:')
  call assert_fails('call win_execute(winid, "bwipe!")', 'E994:')
  call assert_fails('call win_execute(winid, "tabnew")', 'E994:')
  call assert_fails('call win_execute(winid, "tabnext")', 'E994:')
  call assert_fails('call win_execute(winid, "next")', 'E994:')
  call assert_fails('call win_execute(winid, "rewind")', 'E994:')
  call assert_fails('call win_execute(winid, "buf")', 'E994:')
  call assert_fails('call win_execute(winid, "edit")', 'E994:')
  call assert_fails('call win_execute(winid, "enew")', 'E994:')
  call assert_fails('call win_execute(winid, "wincmd x")', 'E994:')
  call assert_fails('call win_execute(winid, "wincmd w")', 'E994:')
  call assert_fails('call win_execute(winid, "wincmd t")', 'E994:')
  call assert_fails('call win_execute(winid, "wincmd b")', 'E994:')
  popupclear
endfunc

func Test_popup_with_wrap()
  if !CanRunVimInTerminal()
    return
  endif
  let lines =<< trim END
	 call setline(1, range(1, 100))
	 let winid = popup_create(
	   \ 'a long line that wont fit',
	   \ {'line': 3, 'col': 20, 'maxwidth': 10, 'wrap': 1})
  END
  call writefile(lines, 'XtestPopup')
  let buf = RunVimInTerminal('-S XtestPopup', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_wrap', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopup')
endfunc

func Test_popup_without_wrap()
  if !CanRunVimInTerminal()
    return
  endif
  let lines =<< trim END
	 call setline(1, range(1, 100))
	 let winid = popup_create(
	   \ 'a long line that wont fit',
	   \ {'line': 3, 'col': 20, 'maxwidth': 10, 'wrap': 0})
  END
  call writefile(lines, 'XtestPopup')
  let buf = RunVimInTerminal('-S XtestPopup', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_nowrap', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopup')
endfunc

func Test_popup_time()
  if !has('timers')
    return
  endif
  topleft vnew
  call setline(1, 'hello')

  call popup_create('world', {
	\ 'line': 1,
	\ 'col': 1,
	\ 'minwidth': 20,
	\ 'time': 500,
	\})
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('world', line)

  sleep 700m
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('hello', line)

  call popup_create('on the command line', {
	\ 'line': &lines,
	\ 'col': 10,
	\ 'minwidth': 20,
	\ 'time': 500,
	\})
  redraw
  let line = join(map(range(1, 30), 'screenstring(&lines, v:val)'), '')
  call assert_match('.*on the command line.*', line)

  sleep 700m
  redraw
  let line = join(map(range(1, 30), 'screenstring(&lines, v:val)'), '')
  call assert_notmatch('.*on the command line.*', line)

  bwipe!
endfunc

func Test_popup_hide()
  topleft vnew
  call setline(1, 'hello')

  let winid = popup_create('world', {
	\ 'line': 1,
	\ 'col': 1,
	\ 'minwidth': 20,
	\})
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('world', line)
  call assert_equal(1, popup_getpos(winid).visible)
  " buffer is still listed and active
  call assert_match(winbufnr(winid) .. 'u a.*\[Popup\]', execute('ls u'))

  call popup_hide(winid)
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('hello', line)
  call assert_equal(0, popup_getpos(winid).visible)
  " buffer is still listed but hidden
  call assert_match(winbufnr(winid) .. 'u h.*\[Popup\]', execute('ls u'))

  call popup_show(winid)
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('world', line)
  call assert_equal(1, popup_getpos(winid).visible)


  call popup_close(winid)
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('hello', line)

  " error is given for existing non-popup window
  call assert_fails('call popup_hide(win_getid())', 'E993:')

  " no error non-existing window
  call popup_hide(1234234)
  call popup_show(41234234)

  bwipe!
endfunc

func Test_popup_move()
  topleft vnew
  call setline(1, 'hello')

  let winid = popup_create('world', {
	\ 'line': 1,
	\ 'col': 1,
	\ 'minwidth': 20,
	\})
  redraw
  let line = join(map(range(1, 6), 'screenstring(1, v:val)'), '')
  call assert_equal('world ', line)

  call popup_move(winid, {'line': 2, 'col': 2})
  redraw
  let line = join(map(range(1, 6), 'screenstring(1, v:val)'), '')
  call assert_equal('hello ', line)
  let line = join(map(range(1, 6), 'screenstring(2, v:val)'), '')
  call assert_equal('~world', line)

  call popup_move(winid, {'line': 1})
  redraw
  let line = join(map(range(1, 6), 'screenstring(1, v:val)'), '')
  call assert_equal('hworld', line)

  call popup_close(winid)

  bwipe!
endfunc

func Test_popup_getpos()
  let winid = popup_create('hello', {
    \ 'line': 2,
    \ 'col': 3,
    \ 'minwidth': 10,
    \ 'minheight': 11,
    \})
  redraw
  let res = popup_getpos(winid)
  call assert_equal(2, res.line)
  call assert_equal(3, res.col)
  call assert_equal(10, res.width)
  call assert_equal(11, res.height)
  call assert_equal(1, res.visible)

  call popup_close(winid)
endfunc

func Test_popup_width_longest()
  let tests = [
	\ [['hello', 'this', 'window', 'displays', 'all of its text'], 15],
	\ [['hello', 'this', 'window', 'all of its text', 'displays'], 15],
	\ [['hello', 'this', 'all of its text', 'window', 'displays'], 15],
	\ [['hello', 'all of its text', 'this', 'window', 'displays'], 15],
	\ [['all of its text', 'hello', 'this', 'window', 'displays'], 15],
	\ ]

  for test in tests
    let winid = popup_create(test[0], {'line': 2, 'col': 3})
    redraw
    let position = popup_getpos(winid)
    call assert_equal(test[1], position.width)
    call popup_close(winid)
  endfor
endfunc

func Test_popup_wraps()
  let tests = [
	\ ['nowrap', 6, 1],
	\ ['a line that wraps once', 12, 2],
	\ ['a line that wraps two times', 12, 3],
	\ ]
  for test in tests
    let winid = popup_create(test[0],
	  \ {'line': 2, 'col': 3, 'maxwidth': 12})
    redraw
    let position = popup_getpos(winid)
    call assert_equal(test[1], position.width)
    call assert_equal(test[2], position.height)

    call popup_close(winid)
    call assert_equal({}, popup_getpos(winid))
  endfor
endfunc

func Test_popup_getoptions()
  let winid = popup_create('hello', {
    \ 'line': 2,
    \ 'col': 3,
    \ 'minwidth': 10,
    \ 'minheight': 11,
    \ 'maxwidth': 20,
    \ 'maxheight': 21,
    \ 'zindex': 100,
    \ 'time': 5000,
    \ 'fixed': 1
    \})
  redraw
  let res = popup_getoptions(winid)
  call assert_equal(2, res.line)
  call assert_equal(3, res.col)
  call assert_equal(10, res.minwidth)
  call assert_equal(11, res.minheight)
  call assert_equal(20, res.maxwidth)
  call assert_equal(21, res.maxheight)
  call assert_equal(100, res.zindex)
  call assert_equal(1, res.fixed)
  if has('timers')
    call assert_equal(5000, res.time)
  endif
  call popup_close(winid)

  let winid = popup_create('hello', {})
  redraw
  let res = popup_getoptions(winid)
  call assert_equal(0, res.line)
  call assert_equal(0, res.col)
  call assert_equal(0, res.minwidth)
  call assert_equal(0, res.minheight)
  call assert_equal(0, res.maxwidth)
  call assert_equal(0, res.maxheight)
  call assert_equal(50, res.zindex)
  call assert_equal(0, res.fixed)
  if has('timers')
    call assert_equal(0, res.time)
  endif
  call popup_close(winid)
  call assert_equal({}, popup_getoptions(winid))
endfunc

func Test_popup_option_values()
  new
  " window-local
  setlocal number
  setlocal nowrap
  " buffer-local
  setlocal omnifunc=Something
  " global/buffer-local
  setlocal path=/there
  " global/window-local
  setlocal scrolloff=9

  let winid = popup_create('hello', {})
  call assert_equal(0, getwinvar(winid, '&number'))
  call assert_equal(1, getwinvar(winid, '&wrap'))
  call assert_equal('', getwinvar(winid, '&omnifunc'))
  call assert_equal(&g:path, getwinvar(winid, '&path'))
  call assert_equal(&g:scrolloff, getwinvar(winid, '&scrolloff'))

  call popup_close(winid)
  bwipe
endfunc

func Test_popup_atcursor()
  topleft vnew
  call setline(1, [
  \  'xxxxxxxxxxxxxxxxx',
  \  'xxxxxxxxxxxxxxxxx',
  \  'xxxxxxxxxxxxxxxxx',
  \])

  call cursor(2, 2)
  redraw
  let winid = popup_atcursor('vim', {})
  redraw
  let line = join(map(range(1, 17), 'screenstring(1, v:val)'), '')
  call assert_equal('xvimxxxxxxxxxxxxx', line)
  call popup_close(winid)

  call cursor(3, 4)
  redraw
  let winid = popup_atcursor('vim', {})
  redraw
  let line = join(map(range(1, 17), 'screenstring(2, v:val)'), '')
  call assert_equal('xxxvimxxxxxxxxxxx', line)
  call popup_close(winid)

  call cursor(1, 1)
  redraw
  let winid = popup_create('vim', {
  \ 'line': 'cursor+2',
  \ 'col': 'cursor+1',
  \})
  redraw
  let line = join(map(range(1, 17), 'screenstring(3, v:val)'), '')
  call assert_equal('xvimxxxxxxxxxxxxx', line)
  call popup_close(winid)

  call cursor(3, 3)
  redraw
  let winid = popup_create('vim', {
  \ 'line': 'cursor-2',
  \ 'col': 'cursor-1',
  \})
  redraw
  let line = join(map(range(1, 17), 'screenstring(1, v:val)'), '')
  call assert_equal('xvimxxxxxxxxxxxxx', line)
  call popup_close(winid)

  " just enough room above
  call cursor(3, 3)
  redraw
  let winid = popup_atcursor(['vim', 'is great'], {})
  redraw
  let pos = popup_getpos(winid)
  call assert_equal(1, pos.line)
  call popup_close(winid)

  " not enough room above, popup goes below the cursor
  call cursor(3, 3)
  redraw
  let winid = popup_atcursor(['vim', 'is', 'great'], {})
  redraw
  let pos = popup_getpos(winid)
  call assert_equal(4, pos.line)
  call popup_close(winid)

  bwipe!
endfunc

func Test_popup_filter()
  new
  call setline(1, 'some text')

  func MyPopupFilter(winid, c)
    if a:c == 'e'
      let g:eaten = 'e'
      return 1
    endif
    if a:c == '0'
      let g:ignored = '0'
      return 0
    endif
    if a:c == 'x'
      call popup_close(a:winid)
      return 1
    endif
    return 0
  endfunc

  let winid = popup_create('something', {'filter': 'MyPopupFilter'})
  redraw

  " e is consumed by the filter
  call feedkeys('e', 'xt')
  call assert_equal('e', g:eaten)

  " 0 is ignored by the filter
  normal $
  call assert_equal(9, getcurpos()[2])
  call feedkeys('0', 'xt')
  call assert_equal('0', g:ignored)
  call assert_equal(1, getcurpos()[2])

  " x closes the popup
  call feedkeys('x', 'xt')
  call assert_equal('e', g:eaten)
  call assert_equal(-1, winbufnr(winid))

  delfunc MyPopupFilter
  popupclear
endfunc

func Test_popup_close_callback()
  func PopupDone(id, result)
    let g:result = a:result
  endfunc
  let winid = popup_create('something', {'callback': 'PopupDone'})
  redraw
  call popup_close(winid, 'done')
  call assert_equal('done', g:result)
endfunc

func Test_popup_empty()
  let winid = popup_create('', {'padding': [2,2,2,2]})
  redraw
  let pos = popup_getpos(winid)
  call assert_equal(4, pos.width)
  call assert_equal(5, pos.height)

  let winid = popup_create([], {'border': []})
  redraw
  let pos = popup_getpos(winid)
  call assert_equal(2, pos.width)
  call assert_equal(3, pos.height)
endfunc

func Test_popup_never_behind()
  if !CanRunVimInTerminal()
    return
  endif
  " +-----------------------------+
  " |             |               |
  " |             |               |
  " |             |               |
  " |            line1            |
  " |------------line2------------|
  " |            line3            |
  " |            line4            |
  " |                             |
  " |                             |
  " +-----------------------------+
  let lines =<< trim END
    only 
    split
    vsplit
    let info_window1 = getwininfo()[0]
    let line = info_window1['height']
    let col = info_window1['width']
    call popup_create(['line1', 'line2', 'line3', 'line4'], {
	      \   'line' : line,
	      \   'col' : col,
	      \ })
  END
  call writefile(lines, 'XtestPopupBehind')
  let buf = RunVimInTerminal('-S XtestPopupBehind', {'rows': 10})
  call term_sendkeys(buf, "\<C-W>w")
  call VerifyScreenDump(buf, 'Test_popupwin_behind', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupBehind')
endfunc

func s:VerifyPosition( p, msg, line, col, width, height )
  call assert_equal( a:line,   popup_getpos( a:p ).line,   a:msg . ' (l)' )
  call assert_equal( a:col,    popup_getpos( a:p ).col,    a:msg . ' (c)' )
  call assert_equal( a:width,  popup_getpos( a:p ).width,  a:msg . ' (w)' )
  call assert_equal( a:height, popup_getpos( a:p ).height, a:msg . ' (h)' )
endfunc

func Test_popup_position_adjust()
  " Anything placed past 2 cells from of the right of the screen is moved to the
  " left.
  "
  " When wrapping is disabled, we also shift to the left to display on the
  " screen, unless fixed is set.

  " Entries for cases which don't vary based on wrapping.
  " Format is per tests described below
  let both_wrap_tests = [
        \       [ 'a', 5, &columns,        5, &columns - 2, 1, 1 ],
        \       [ 'b', 5, &columns + 1,    5, &columns - 2, 1, 1 ],
        \       [ 'c', 5, &columns - 1,    5, &columns - 2, 1, 1 ],
        \       [ 'd', 5, &columns - 2,    5, &columns - 2, 1, 1 ],
        \       [ 'e', 5, &columns - 3,    5, &columns - 3, 1, 1 ],
        \
        \       [ 'aa', 5, &columns,        5, &columns - 2, 2, 1 ],
        \       [ 'bb', 5, &columns + 1,    5, &columns - 2, 2, 1 ],
        \       [ 'cc', 5, &columns - 1,    5, &columns - 2, 2, 1 ],
        \       [ 'dd', 5, &columns - 2,    5, &columns - 2, 2, 1 ],
        \       [ 'ee', 5, &columns - 3,    5, &columns - 3, 2, 1 ],
        \
        \       [ 'aaa', 5, &columns,        5, &columns - 2, 3, 1 ],
        \       [ 'bbb', 5, &columns + 1,    5, &columns - 2, 3, 1 ],
        \       [ 'ccc', 5, &columns - 1,    5, &columns - 2, 3, 1 ],
        \       [ 'ddd', 5, &columns - 2,    5, &columns - 2, 3, 1 ],
        \       [ 'eee', 5, &columns - 3,    5, &columns - 3, 3, 1 ],
        \ ]

  " these test groups are dicts with:
  "  - comment: something to identify the group of tests by
  "  - options: dict of options to merge with the row/col in tests
  "  - tests: list of cases. Each one is a list with elements:
  "     - text
  "     - row
  "     - col
  "     - expected row
  "     - expected col
  "     - expected width
  "     - expected height
  let tests = [
        \ {
        \   'comment': 'left-aligned with wrapping',
        \   'options': {
        \     'wrap': 1,
        \     'pos': 'botleft',
        \   },
        \   'tests': both_wrap_tests + [
        \       [ 'aaaa', 5, &columns,        4, &columns - 2, 3, 2 ],
        \       [ 'bbbb', 5, &columns + 1,    4, &columns - 2, 3, 2 ],
        \       [ 'cccc', 5, &columns - 1,    4, &columns - 2, 3, 2 ],
        \       [ 'dddd', 5, &columns - 2,    4, &columns - 2, 3, 2 ],
        \       [ 'eeee', 5, &columns - 3,    5, &columns - 3, 4, 1 ],
        \   ],
        \ },
        \ {
        \   'comment': 'left aligned without wrapping',
        \   'options': {
        \     'wrap': 0,
        \     'pos': 'botleft',
        \   },
        \   'tests': both_wrap_tests + [
        \       [ 'aaaa', 5, &columns,        5, &columns - 3, 4, 1 ],
        \       [ 'bbbb', 5, &columns + 1,    5, &columns - 3, 4, 1 ],
        \       [ 'cccc', 5, &columns - 1,    5, &columns - 3, 4, 1 ],
        \       [ 'dddd', 5, &columns - 2,    5, &columns - 3, 4, 1 ],
        \       [ 'eeee', 5, &columns - 3,    5, &columns - 3, 4, 1 ],
        \   ],
        \ },
        \ {
        \   'comment': 'left aligned with fixed position',
        \   'options': {
        \     'wrap': 0,
        \     'fixed': 1,
        \     'pos': 'botleft',
        \   },
        \   'tests': both_wrap_tests + [
        \       [ 'aaaa', 5, &columns,        5, &columns - 2, 3, 1 ],
        \       [ 'bbbb', 5, &columns + 1,    5, &columns - 2, 3, 1 ],
        \       [ 'cccc', 5, &columns - 1,    5, &columns - 2, 3, 1 ],
        \       [ 'dddd', 5, &columns - 2,    5, &columns - 2, 3, 1 ],
        \       [ 'eeee', 5, &columns - 3,    5, &columns - 3, 4, 1 ],
        \   ],
        \ },
      \ ]

  for test_group in tests
    for test in test_group.tests
      let [ text, line, col, e_line, e_col, e_width, e_height ] = test
      let options = {
            \ 'line': line,
            \ 'col': col,
            \ }
      call extend( options, test_group.options )

      let p = popup_create( text, options )

      let msg = string( extend( options, { 'text': text } ) )
      call s:VerifyPosition( p, msg, e_line, e_col, e_width, e_height )
      call popup_close( p )
    endfor
  endfor

  popupclear
  %bwipe!
endfunc

function Test_adjust_left_past_screen_width()
  " width of screen
  let X = join(map(range(&columns), {->'X'}), '')

  let p = popup_create( X, { 'line': 1, 'col': 1, 'wrap': 0 } )
  call s:VerifyPosition( p, 'full width topleft', 1, 1, &columns, 1 )

  redraw
  let line = join(map(range(1, &columns + 1), 'screenstring(1, v:val)'), '')
  call assert_equal(X, line)

  call popup_close( p )
  redraw

  " Same if placed on the right hand side
  let p = popup_create( X, { 'line': 1, 'col': &columns, 'wrap': 0 } )
  call s:VerifyPosition( p, 'full width topright', 1, 1, &columns, 1 )

  redraw
  let line = join(map(range(1, &columns + 1), 'screenstring(1, v:val)'), '')
  call assert_equal(X, line)

  call popup_close( p )
  redraw

  " Extend so > window width
  let X .= 'x'

  let p = popup_create( X, { 'line': 1, 'col': 1, 'wrap': 0 } )
  call s:VerifyPosition( p, 'full width +  1 topleft', 1, 1, &columns, 1 )

  redraw
  let line = join(map(range(1, &columns + 1), 'screenstring(1, v:val)'), '')
  call assert_equal(X[ : -2 ], line)

  call popup_close( p )
  redraw

  " Shifted then truncated (the x is not visible)
  let p = popup_create( X, { 'line': 1, 'col': &columns - 3, 'wrap': 0 } )
  call s:VerifyPosition( p, 'full width + 1 topright', 1, 1, &columns, 1 )

  redraw
  let line = join(map(range(1, &columns + 1), 'screenstring(1, v:val)'), '')
  call assert_equal(X[ : -2 ], line)

  call popup_close( p )
  redraw

  " Not shifted, just truncated
  let p = popup_create( X,
        \ { 'line': 1, 'col': 2, 'wrap': 0, 'fixed': 1 } )
  call s:VerifyPosition( p, 'full width + 1 fixed', 1, 2, &columns - 1, 1)

  redraw
  let line = join(map(range(1, &columns + 1), 'screenstring(1, v:val)'), '')
  let e_line = ' ' . X[ 1 : -2 ]
  call assert_equal(e_line, line)

  call popup_close( p )
  redraw

  popupclear
  %bwipe!
endfunction
