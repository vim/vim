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

  " resize popup
  call term_sendkeys(buf, ":call popup_move(popupwin, {'minwidth': 15, 'maxwidth': 25, 'minheight': 3, 'maxheight': 5})\<CR>")
  call term_sendkeys(buf, ":redraw\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_05', {})

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

  call popup_hide(winid)
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('hello', line)

  call popup_show(winid)
  redraw
  let line = join(map(range(1, 5), 'screenstring(1, v:val)'), '')
  call assert_equal('world', line)


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
