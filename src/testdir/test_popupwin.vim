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
  call assert_equal(1, popup_getposition(winid).visible)

  call popup_hide(winid)
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('hello', line)
  call assert_equal(0, popup_getposition(winid).visible)

  call popup_show(winid)
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('world', line)
  call assert_equal(1, popup_getposition(winid).visible)


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

func Test_popup_getposition()
  let winid = popup_create('hello', {
    \ 'line': 2,
    \ 'col': 3,
    \ 'minwidth': 10,
    \ 'minheight': 11,
    \})
  redraw
  let res = popup_getposition(winid)
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
    let position = popup_getposition(winid)
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
    let position = popup_getposition(winid)
    call assert_equal(test[1], position.width)
    call assert_equal(test[2], position.height)

    call popup_close(winid)
    call assert_equal({}, popup_getposition(winid))
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
