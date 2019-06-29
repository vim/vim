" Tests for popup windows

source check.vim
CheckFeature textprop

source screendump.vim

func Test_simple_popup()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif
  let lines =<< trim END
	call setline(1, range(1, 100))
	hi PopupColor1 ctermbg=lightblue
	hi PopupColor2 ctermbg=lightcyan
	hi Comment ctermfg=red
	call prop_type_add('comment', {'highlight': 'Comment'})
	let winid = popup_create('hello there', {'line': 3, 'col': 11, 'minwidth': 20, 'highlight': 'PopupColor1'})
	let winid2 = popup_create(['another one', 'another two', 'another three'], {'line': 3, 'col': 25, 'minwidth': 20})
	call setwinvar(winid2, '&wincolor', 'PopupColor2')
  END
  call writefile(lines, 'XtestPopup')
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

  " set 'columns' to a small value, size must be recomputed
  call term_sendkeys(buf, ":let cols = &columns\<CR>")
  call term_sendkeys(buf, ":set columns=12\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_04a', {})
  call term_sendkeys(buf, ":let &columns = cols\<CR>")

  " resize popup, show empty line at bottom
  call term_sendkeys(buf, ":call popup_move(popupwin, {'minwidth': 15, 'maxwidth': 25, 'minheight': 3, 'maxheight': 5})\<CR>")
  call term_sendkeys(buf, ":redraw\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_05', {})

  " show not fitting line at bottom
  call term_sendkeys(buf, ":call setbufline(winbufnr(popupwin), 3, 'this line will not fit here')\<CR>")
  call term_sendkeys(buf, ":redraw\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_06', {})

  " move popup over ruler
  call term_sendkeys(buf, ":set cmdheight=2\<CR>")
  call term_sendkeys(buf, ":call popup_move(popupwin, {'line': 7, 'col': 55})\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_07', {})

  " clear all popups after moving the cursor a bit, so that ruler is updated
  call term_sendkeys(buf, "axxx\<Esc>")
  call term_wait(buf)
  call term_sendkeys(buf, "0")
  call term_wait(buf)
  call term_sendkeys(buf, ":call popup_clear()\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_08', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopup')
endfunc

func Test_popup_with_border_and_padding()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif

  for iter in range(0, 1)
    let lines =<< trim END
	  call setline(1, range(1, 100))
	  call popup_create('hello border', {'line': 2, 'col': 3, 'border': []})
	  call popup_create('hello padding', {'line': 2, 'col': 23, 'padding': []})
	  call popup_create('hello both', {'line': 2, 'col': 43, 'border': [], 'padding': []})
	  call popup_create('border TL', {'line': 6, 'col': 3, 'border': [1, 0, 0, 4]})
	  call popup_create('paddings', {'line': 6, 'col': 23, 'padding': [1, 3, 2, 4]})
	  call popup_create('wrapped longer text', {'line': 8, 'col': 55, 'padding': [0, 3, 0, 3], 'border': [0, 1, 0, 1]})
	  call popup_create('right aligned text', {'line': 11, 'col': 56, 'wrap': 0, 'padding': [0, 3, 0, 3], 'border': [0, 1, 0, 1]})
    END
    call insert(lines, iter == 1 ? '' : 'set enc=latin1')
    call writefile(lines, 'XtestPopupBorder')
    let buf = RunVimInTerminal('-S XtestPopupBorder', {'rows': 15})
    call VerifyScreenDump(buf, 'Test_popupwin_2' .. iter, {})

    call StopVimInTerminal(buf)
    call delete('XtestPopupBorder')
  endfor

  let lines =<< trim END
	call setline(1, range(1, 100))
	hi BlueColor ctermbg=lightblue
	hi TopColor ctermbg=253
	hi RightColor ctermbg=245
	hi BottomColor ctermbg=240
	hi LeftColor ctermbg=248
	call popup_create('hello border', {'line': 2, 'col': 3, 'border': [], 'borderhighlight': ['BlueColor']})
	call popup_create(['hello border', 'and more'], {'line': 2, 'col': 23, 'border': [], 'borderhighlight': ['TopColor', 'RightColor', 'BottomColor', 'LeftColor']})
	call popup_create(['hello border', 'lines only'], {'line': 2, 'col': 43, 'border': [], 'borderhighlight': ['BlueColor'], 'borderchars': ['x']})
	call popup_create(['hello border', 'with corners'], {'line': 2, 'col': 60, 'border': [], 'borderhighlight': ['BlueColor'], 'borderchars': ['x', '#']})
	let winid = popup_create(['hello border', 'with numbers'], {'line': 6, 'col': 3, 'border': [], 'borderhighlight': ['BlueColor'], 'borderchars': ['0', '1', '2', '3', '4', '5', '6', '7']})
	call popup_create(['hello border', 'just blanks'], {'line': 7, 'col': 23, 'border': [], 'borderhighlight': ['BlueColor'], 'borderchars': [' ']})
  END
  call writefile(lines, 'XtestPopupBorder')
  let buf = RunVimInTerminal('-S XtestPopupBorder', {'rows': 12})
  call VerifyScreenDump(buf, 'Test_popupwin_22', {})

  " check that changing borderchars triggers a redraw
  call term_sendkeys(buf, ":call popup_setoptions(winid, {'borderchars': ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h']})\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_23', {})

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
	\ 'firstline': 1,
	\ 'scrollbar': 0,
	\ 'visible': 1}
  let winid = popup_create('hello border', {'line': 2, 'col': 3, 'border': []})",
  call assert_equal(with_border_or_padding, popup_getpos(winid))
  let options = popup_getoptions(winid)
  call assert_equal([], options.border)
  call assert_false(has_key(options, "padding"))

  let winid = popup_create('hello padding', {'line': 2, 'col': 3, 'padding': []})
  let with_border_or_padding.width = 15
  let with_border_or_padding.core_width = 13
  call assert_equal(with_border_or_padding, popup_getpos(winid))
  let options = popup_getoptions(winid)
  call assert_false(has_key(options, "border"))
  call assert_equal([], options.padding)

  call popup_setoptions(winid, {
	\ 'padding': [1, 2, 3, 4],
	\ 'border': [4, 0, 7, 8],
	\ 'borderhighlight': ['Top', 'Right', 'Bottom', 'Left'],
	\ 'borderchars': ['1', '^', '2', '>', '3', 'v', '4', '<'],
	\ })
  let options = popup_getoptions(winid)
  call assert_equal([1, 0, 1, 1], options.border)
  call assert_equal([1, 2, 3, 4], options.padding)
  call assert_equal(['Top', 'Right', 'Bottom', 'Left'], options.borderhighlight)
  call assert_equal(['1', '^', '2', '>', '3', 'v', '4', '<'], options.borderchars)

  let winid = popup_create('hello both', {'line': 3, 'col': 8, 'border': [], 'padding': []})
  call assert_equal({
	\ 'line': 3,
	\ 'core_line': 5,
	\ 'col': 8,
	\ 'core_col': 10,
	\ 'width': 14,
	\ 'core_width': 10,
	\ 'height': 5,
	\ 'scrollbar': 0,
	\ 'core_height': 1,
	\ 'firstline': 1,
	\ 'visible': 1}, popup_getpos(winid))

  call popup_clear()
endfunc

func Test_popup_with_syntax_win_execute()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif
  let lines =<< trim END
	call setline(1, range(1, 100))
	hi PopupColor ctermbg=lightblue
	let winid = popup_create([
	    \ '#include <stdio.h>',
	    \ 'int main(void)',
	    \ '{',
	    \ '    printf(123);',
	    \ '}',
	    \], {'line': 3, 'col': 25, 'highlight': 'PopupColor'})
	call win_execute(winid, 'set syntax=cpp')
  END
  call writefile(lines, 'XtestPopup')
  let buf = RunVimInTerminal('-S XtestPopup', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_10', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopup')
endfunc

func Test_popup_with_syntax_setbufvar()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
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

func Test_popup_with_matches()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif
  let lines =<< trim END
	call setline(1, ['111 222 333', '444 555 666'])
	let winid = popup_create([
	    \ '111 222 333',
	    \ '444 555 666',
	    \], {'line': 3, 'col': 10, 'border': []})
	set hlsearch
	/666
	call matchadd('ErrorMsg', '111')
	call matchadd('ErrorMsg', '444')
	call win_execute(winid, "call matchadd('ErrorMsg', '111')")
	call win_execute(winid, "call matchadd('ErrorMsg', '555')")
  END
  call writefile(lines, 'XtestPopupMatches')
  let buf = RunVimInTerminal('-S XtestPopupMatches', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_matches', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupMatches')
endfunc

func Test_popup_all_corners()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
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

func Test_popup_firstline()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif
  let lines =<< trim END
	call setline(1, range(1, 20))
	call popup_create(['1111', '222222', '33333', '44', '5', '666666', '77777', '888', '9999999999999999'], {
	      \ 'maxheight': 4,
	      \ 'firstline': 3,
	      \ })
  END
  call writefile(lines, 'XtestPopupFirstline')
  let buf = RunVimInTerminal('-S XtestPopupFirstline', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_firstline', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupFirstline')

  let winid = popup_create(['1111', '222222', '33333', '44444'], {
	\ 'maxheight': 2,
	\ 'firstline': 3,
	\ })
  call assert_equal(3, popup_getoptions(winid).firstline)
  call popup_setoptions(winid, {'firstline': 1})
  call assert_equal(1, popup_getoptions(winid).firstline)

  call popup_close(winid)
endfunc

func Test_popup_drag()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif
  " create a popup that covers the command line
  let lines =<< trim END
	call setline(1, range(1, 20))
	let winid = popup_create(['1111', '222222', '33333'], {
	      \ 'drag': 1,
	      \ 'border': [],
	      \ 'line': &lines - 4,
	      \ })
	func Dragit()
	  call feedkeys("\<F3>\<LeftMouse>\<F4>\<LeftDrag>\<LeftRelease>", "xt")
	endfunc
	map <silent> <F3> :call test_setmouse(&lines - 4, &columns / 2)<CR>
	map <silent> <F4> :call test_setmouse(&lines - 8, &columns / 2)<CR>
  END
  call writefile(lines, 'XtestPopupDrag')
  let buf = RunVimInTerminal('-S XtestPopupDrag', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_drag_01', {})

  call term_sendkeys(buf, ":call Dragit()\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_drag_02', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupDrag')
endfunc

func Test_popup_with_mask()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif
  let lines =<< trim END
	call setline(1, repeat([join(range(1, 40), '')], 10))
	hi PopupColor ctermbg=lightgrey
	let winid = popup_create([
	    \ 'some text',
	    \ 'another line',
	    \], {
	    \ 'line': 2,
	    \ 'col': 10,
	    \ 'zindex': 90,
	    \ 'padding': [],
	    \ 'highlight': 'PopupColor',
	    \ 'mask': [[1,1,1,1], [-5,-1,4,4], [7,9,2,3], [2,4,3,3]]})
	call popup_create([
	    \ 'xxxxxxxxx',
	    \ 'yyyyyyyyy',
	    \], {
	    \ 'line': 3,
	    \ 'col': 18,
	    \ 'zindex': 20})
  END
  call writefile(lines, 'XtestPopupMask')
  let buf = RunVimInTerminal('-S XtestPopupMask', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_mask_1', {})

  call term_sendkeys(buf, ":call popup_move(winid, {'col': 11, 'line': 3})\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_mask_2', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupMask')
endfunc

func Test_popup_select()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif
  if !has('clipboard')
    throw 'Skipped: clipboard feature missing'
  endif
  " create a popup with some text to be selected
  let lines =<< trim END
    set clipboard=autoselect
    call setline(1, range(1, 20))
    let winid = popup_create(['the word', 'some more', 'several words here'], {
	  \ 'drag': 1,
	  \ 'border': [],
	  \ 'line': 3,
	  \ 'col': 10,
	  \ })
    func Select1()
      call feedkeys("\<F3>\<LeftMouse>\<F4>\<LeftDrag>\<LeftRelease>", "xt")
    endfunc
    map <silent> <F3> :call test_setmouse(4, 15)<CR>
    map <silent> <F4> :call test_setmouse(6, 23)<CR>
  END
  call writefile(lines, 'XtestPopupSelect')
  let buf = RunVimInTerminal('-S XtestPopupSelect', {'rows': 10})
  call term_sendkeys(buf, ":call Select1()\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_select_01', {})

  call term_sendkeys(buf, ":call popup_close(winid)\<CR>")
  call term_sendkeys(buf, "\"*p")
  call VerifyScreenDump(buf, 'Test_popupwin_select_02', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupSelect')
endfunc

func Test_popup_in_tab()
  " default popup is local to tab, not visible when in other tab
  let winid = popup_create("text", {})
  let bufnr = winbufnr(winid)
  call assert_equal(1, popup_getpos(winid).visible)
  call assert_equal(0, popup_getoptions(winid).tabpage)
  tabnew
  call assert_equal(0, popup_getpos(winid).visible)
  call assert_equal(1, popup_getoptions(winid).tabpage)
  quit
  call assert_equal(1, popup_getpos(winid).visible)

  call assert_equal(1, bufexists(bufnr))
  call popup_clear()
  " buffer is gone now
  call assert_equal(0, bufexists(bufnr))

  " global popup is visible in any tab
  let winid = popup_create("text", {'tabpage': -1})
  call assert_equal(1, popup_getpos(winid).visible)
  call assert_equal(-1, popup_getoptions(winid).tabpage)
  tabnew
  call assert_equal(1, popup_getpos(winid).visible)
  call assert_equal(-1, popup_getoptions(winid).tabpage)
  quit
  call assert_equal(1, popup_getpos(winid).visible)
  call popup_clear()

  " create popup in other tab
  tabnew
  let winid = popup_create("text", {'tabpage': 1})
  call assert_equal(0, popup_getpos(winid).visible)
  call assert_equal(1, popup_getoptions(winid).tabpage)
  quit
  call assert_equal(1, popup_getpos(winid).visible)
  call assert_equal(0, popup_getoptions(winid).tabpage)
  call popup_clear()
endfunc

func Test_popup_valid_arguments()
  " Zero value is like the property wasn't there
  let winid = popup_create("text", {"col": 0})
  let pos = popup_getpos(winid)
  call assert_inrange(&columns / 2 - 1, &columns / 2 + 1, pos.col)
  call popup_clear()

  " using cursor column has minimum value of 1
  let winid = popup_create("text", {"col": 'cursor-100'})
  let pos = popup_getpos(winid)
  call assert_equal(1, pos.col)
  call popup_clear()

  " center
  let winid = popup_create("text", {"pos": 'center'})
  let pos = popup_getpos(winid)
  let around = (&columns - pos.width) / 2
  call assert_inrange(around - 1, around + 1, pos.col)
  let around = (&lines - pos.height) / 2
  call assert_inrange(around - 1, around + 1, pos.line)
  call popup_clear()
endfunc

func Test_popup_invalid_arguments()
  call assert_fails('call popup_create(666, {})', 'E714:')
  call popup_clear()
  call assert_fails('call popup_create("text", "none")', 'E715:')
  call popup_clear()

  call assert_fails('call popup_create("text", {"col": "xxx"})', 'E475:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"col": "cursor8"})', 'E15:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"col": "cursor+x"})', 'E15:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"col": "cursor+8x"})', 'E15:')
  call popup_clear()

  call assert_fails('call popup_create("text", {"line": "xxx"})', 'E475:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"line": "cursor8"})', 'E15:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"line": "cursor+x"})', 'E15:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"line": "cursor+8x"})', 'E15:')
  call popup_clear()

  call assert_fails('call popup_create("text", {"pos": "there"})', 'E475:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"padding": "none"})', 'E714:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"border": "none"})', 'E714:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"borderhighlight": "none"})', 'E714:')
  call popup_clear()
  call assert_fails('call popup_create("text", {"borderchars": "none"})', 'E714:')
  call popup_clear()

  call assert_fails('call popup_create([{"text": "text"}, 666], {})', 'E715:')
  call popup_clear()
  call assert_fails('call popup_create([{"text": "text", "props": "none"}], {})', 'E714:')
  call popup_clear()
  call assert_fails('call popup_create([{"text": "text", "props": ["none"]}], {})', 'E715:')
  call popup_clear()
endfunc

func Test_win_execute_closing_curwin()
  split
  let winid = popup_create('some text', {})
  call assert_fails('call win_execute(winid, winnr() .. "close")', 'E994')
  call popup_clear()
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
  call popup_clear()
endfunc

func Test_popup_with_wrap()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
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
    throw 'Skipped: cannot make screendumps'
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
    throw 'Skipped: timer feature not supported'
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

  " cursor in first line, popup in line 2
  call cursor(1, 1)
  redraw
  let winid = popup_atcursor(['vim', 'is', 'great'], {})
  redraw
  let pos = popup_getpos(winid)
  call assert_equal(2, pos.line)
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
  call popup_clear()
endfunc

func ShowDialog(key, result)
  let s:cb_res = 999
  let winid = popup_dialog('do you want to quit (Yes/no)?', {
	  \ 'filter': 'popup_filter_yesno',
	  \ 'callback': 'QuitCallback',
	  \ })
  redraw
  call feedkeys(a:key, "xt")
  call assert_equal(winid, s:cb_winid)
  call assert_equal(a:result, s:cb_res)
endfunc

func Test_popup_dialog()
  func QuitCallback(id, res)
    let s:cb_winid = a:id
    let s:cb_res = a:res
  endfunc

  let winid = ShowDialog("y", 1)
  let winid = ShowDialog("Y", 1)
  let winid = ShowDialog("n", 0)
  let winid = ShowDialog("N", 0)
  let winid = ShowDialog("x", 0)
  let winid = ShowDialog("X", 0)
  let winid = ShowDialog("\<Esc>", 0)
  let winid = ShowDialog("\<C-C>", -1)

  delfunc QuitCallback
endfunc

func ShowMenu(key, result)
  let s:cb_res = 999
  let winid = popup_menu(['one', 'two', 'something else'], {
	  \ 'callback': 'QuitCallback',
	  \ })
  redraw
  call feedkeys(a:key, "xt")
  call assert_equal(winid, s:cb_winid)
  call assert_equal(a:result, s:cb_res)
endfunc

func Test_popup_menu()
  func QuitCallback(id, res)
    let s:cb_winid = a:id
    let s:cb_res = a:res
  endfunc

  let winid = ShowMenu(" ", 1)
  let winid = ShowMenu("j \<CR>", 2)
  let winid = ShowMenu("JjK \<CR>", 2)
  let winid = ShowMenu("jjjjjj ", 3)
  let winid = ShowMenu("kkk ", 1)
  let winid = ShowMenu("x", -1)
  let winid = ShowMenu("X", -1)
  let winid = ShowMenu("\<Esc>", -1)
  let winid = ShowMenu("\<C-C>", -1)

  delfunc QuitCallback
endfunc

func Test_popup_menu_screenshot()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif

  let lines =<< trim END
	call setline(1, range(1, 20))
	hi PopupSelected ctermbg=lightblue
	call popup_menu(['one', 'two', 'another'], {'callback': 'MenuDone', 'title': ' make a choice from the list '})
	func MenuDone(id, res)
	  echomsg "selected " .. a:res
	endfunc
  END
  call writefile(lines, 'XtestPopupMenu')
  let buf = RunVimInTerminal('-S XtestPopupMenu', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_menu_01', {})

  call term_sendkeys(buf, "jj")
  call VerifyScreenDump(buf, 'Test_popupwin_menu_02', {})

  call term_sendkeys(buf, " ")
  call VerifyScreenDump(buf, 'Test_popupwin_menu_03', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupMenu')
endfunc

func Test_popup_title()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif

  " Create a popup without title or border, a line of padding will be added to
  " put the title on.
  let lines =<< trim END
	call setline(1, range(1, 20))
	call popup_create(['one', 'two', 'another'], {'title': 'Title String'})
  END
  call writefile(lines, 'XtestPopupTitle')
  let buf = RunVimInTerminal('-S XtestPopupTitle', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_title', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupTitle')

  let winid = popup_create('something', {'title': 'Some Title'})
  call assert_equal('Some Title', popup_getoptions(winid).title)
  call popup_setoptions(winid, {'title': 'Another Title'})
  call assert_equal('Another Title', popup_getoptions(winid).title)

  call popup_clear()
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
  call assert_equal(5, pos.width)
  call assert_equal(5, pos.height)

  let winid = popup_create([], {'border': []})
  redraw
  let pos = popup_getpos(winid)
  call assert_equal(3, pos.width)
  call assert_equal(3, pos.height)
endfunc

func Test_popup_never_behind()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
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

  call popup_clear()
  %bwipe!
endfunc

func Test_adjust_left_past_screen_width()
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

  call popup_clear()
  %bwipe!
endfunc

func Test_popup_moved()
  new
  call test_override('char_avail', 1)
  call setline(1, ['one word to move around', 'a WORD.and->some thing'])

  exe "normal gg0/word\<CR>"
  let winid = popup_atcursor('text', {'moved': 'any'})
  redraw
  call assert_equal(1, popup_getpos(winid).visible)
  call assert_equal([4, 4], popup_getoptions(winid).moved)
  " trigger the check for last_cursormoved by going into insert mode
  call feedkeys("li\<Esc>", 'xt')
  call assert_equal({}, popup_getpos(winid))
  call popup_clear()

  exe "normal gg0/word\<CR>"
  let winid = popup_atcursor('text', {'moved': 'word'})
  redraw
  call assert_equal(1, popup_getpos(winid).visible)
  call assert_equal([4, 7], popup_getoptions(winid).moved)
  call feedkeys("hi\<Esc>", 'xt')
  call assert_equal({}, popup_getpos(winid))
  call popup_clear()

  exe "normal gg0/word\<CR>"
  let winid = popup_atcursor('text', {'moved': 'word'})
  redraw
  call assert_equal(1, popup_getpos(winid).visible)
  call assert_equal([4, 7], popup_getoptions(winid).moved)
  call feedkeys("li\<Esc>", 'xt')
  call assert_equal(1, popup_getpos(winid).visible)
  call feedkeys("ei\<Esc>", 'xt')
  call assert_equal(1, popup_getpos(winid).visible)
  call feedkeys("eli\<Esc>", 'xt')
  call assert_equal({}, popup_getpos(winid))
  call popup_clear()

  " WORD is the default
  exe "normal gg0/WORD\<CR>"
  let winid = popup_atcursor('text', {})
  redraw
  call assert_equal(1, popup_getpos(winid).visible)
  call assert_equal([2, 15], popup_getoptions(winid).moved)
  call feedkeys("eli\<Esc>", 'xt')
  call assert_equal(1, popup_getpos(winid).visible)
  call feedkeys("wi\<Esc>", 'xt')
  call assert_equal(1, popup_getpos(winid).visible)
  call feedkeys("Eli\<Esc>", 'xt')
  call assert_equal({}, popup_getpos(winid))
  call popup_clear()

  exe "normal gg0/word\<CR>"
  let winid = popup_atcursor('text', {'moved': [5, 10]})
  redraw
  call assert_equal(1, popup_getpos(winid).visible)
  call feedkeys("eli\<Esc>", 'xt')
  call feedkeys("ei\<Esc>", 'xt')
  call assert_equal(1, popup_getpos(winid).visible)
  call feedkeys("eli\<Esc>", 'xt')
  call assert_equal({}, popup_getpos(winid))
  call popup_clear()

  bwipe!
  call test_override('ALL', 0)
endfunc

func Test_notifications()
  if !has('timers')
    throw 'Skipped: timer feature not supported'
  endif
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif

  call writefile([
	\ "call setline(1, range(1, 20))",
	\ "hi Notification ctermbg=lightblue",
	\ "call popup_notification('first notification', {})",
	\], 'XtestNotifications')
  let buf = RunVimInTerminal('-S XtestNotifications', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_notify_01', {})

  " second one goes below the first one
  call term_sendkeys(buf, ":hi link PopupNotification Notification\<CR>")
  call term_sendkeys(buf, ":call popup_notification('another important notification', {})\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_notify_02', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestNotifications')
endfunc

func Test_popup_scrollbar()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif

  let lines =<< trim END
    call setline(1, range(1, 20))
    hi ScrollThumb ctermbg=blue
    hi ScrollBar ctermbg=red
    let winid = popup_create(['one', 'two', 'three', 'four', 'five',
	  \ 'six', 'seven', 'eight', 'nine'], {
	  \ 'minwidth': 8,
	  \ 'maxheight': 4,
	  \ })
    func ScrollUp()
      call feedkeys("\<F3>\<ScrollWheelUp>", "xt")
    endfunc
    func ScrollDown()
      call feedkeys("\<F3>\<ScrollWheelDown>", "xt")
    endfunc
    func ClickTop()
      call feedkeys("\<F4>\<LeftMouse>", "xt")
    endfunc
    func ClickBot()
      call feedkeys("\<F5>\<LeftMouse>", "xt")
    endfunc
    map <silent> <F3> :call test_setmouse(5, 36)<CR>
    map <silent> <F4> :call test_setmouse(4, 42)<CR>
    map <silent> <F5> :call test_setmouse(7, 42)<CR>
  END
  call writefile(lines, 'XtestPopupScroll')
  let buf = RunVimInTerminal('-S XtestPopupScroll', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_1', {})

  call term_sendkeys(buf, ":call popup_setoptions(winid, {'firstline': 2})\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_2', {})

  call term_sendkeys(buf, ":call popup_setoptions(winid, {'firstline': 6})\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_3', {})

  call term_sendkeys(buf, ":call popup_setoptions(winid, {'firstline': 9})\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_4', {})

  call term_sendkeys(buf, ":call popup_setoptions(winid, {'scrollbarhighlight': 'ScrollBar', 'thumbhighlight': 'ScrollThumb'})\<CR>")
  call term_sendkeys(buf, ":call ScrollUp()\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_5', {})

  call term_sendkeys(buf, ":call ScrollDown()\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_6', {})

  call term_sendkeys(buf, ":call ScrollDown()\<CR>")
  " wait a bit, otherwise it fails sometimes (double click recognized?)
  sleep 100m
  call term_sendkeys(buf, ":call ScrollDown()\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_7', {})

  call term_sendkeys(buf, ":call ClickTop()\<CR>")
  sleep 100m
  call term_sendkeys(buf, ":call ClickTop()\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_8', {})

  call term_sendkeys(buf, ":call ClickBot()\<CR>")
  call VerifyScreenDump(buf, 'Test_popupwin_scroll_9', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupScroll')
endfunc

func Test_popup_settext()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif

  let lines =<< trim END
    let opts = {'wrap': 0}
    let p = popup_create('test', opts)
    call popup_settext(p, 'this is a text')
  END

  call writefile( lines, 'XtestPopupSetText' )
  let buf = RunVimInTerminal('-S XtestPopupSetText', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popup_settext_01', {})

  " Setting to empty string clears it
  call term_sendkeys(buf, ":call popup_settext(p, '')\<CR>")
  call VerifyScreenDump(buf, 'Test_popup_settext_02', {})

  " Setting a list
  call term_sendkeys(buf, ":call popup_settext(p, ['a','b','c'])\<CR>")
  call VerifyScreenDump(buf, 'Test_popup_settext_03', {})

  " Shrinking with a list
  call term_sendkeys(buf, ":call popup_settext(p, ['a'])\<CR>")
  call VerifyScreenDump(buf, 'Test_popup_settext_04', {})

  " Growing with a list
  call term_sendkeys(buf, ":call popup_settext(p, ['a','b','c'])\<CR>")
  call VerifyScreenDump(buf, 'Test_popup_settext_03', {})

  " Empty list clears
  call term_sendkeys(buf, ":call popup_settext(p, [])\<CR>")
  call VerifyScreenDump(buf, 'Test_popup_settext_05', {})

  " Dicts
  call term_sendkeys(buf, ":call popup_settext(p, [{'text': 'aaaa'}, {'text': 'bbbb'}, {'text': 'cccc'}])\<CR>")
  call VerifyScreenDump(buf, 'Test_popup_settext_06', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopupSetText')
endfunc

func Test_popup_hidden()
  new

  let winid = popup_atcursor('text', {'hidden': 1})
  redraw
  call assert_equal(0, popup_getpos(winid).visible)
  call popup_close(winid)

  let winid = popup_create('text', {'hidden': 1})
  redraw
  call assert_equal(0, popup_getpos(winid).visible)
  call popup_close(winid)

  func QuitCallback(id, res)
    let s:cb_winid = a:id
    let s:cb_res = a:res
  endfunc
  let winid = popup_dialog('make a choice', {'hidden': 1,
	  \ 'filter': 'popup_filter_yesno',
	  \ 'callback': 'QuitCallback',
	  \ })
  redraw
  call assert_equal(0, popup_getpos(winid).visible)
  call assert_equal(function('popup_filter_yesno'), popup_getoptions(winid).filter)
  call assert_equal(function('QuitCallback'), popup_getoptions(winid).callback)
  exe "normal anot used by filter\<Esc>"
  call assert_equal('not used by filter', getline(1))

  call popup_show(winid)
  call feedkeys('y', "xt")
  call assert_equal(1, s:cb_res)

  bwipe!
  delfunc QuitCallback
endfunc

" Test options not checked elsewhere
func Test_set_get_options()
  let winid = popup_create('some text', {'highlight': 'Beautiful'})
  let options = popup_getoptions(winid)
  call assert_equal(1, options.wrap)
  call assert_equal(0, options.drag)
  call assert_equal('Beautiful', options.highlight)

  call popup_setoptions(winid, {'wrap': 0, 'drag': 1, 'highlight': 'Another'})
  let options = popup_getoptions(winid)
  call assert_equal(0, options.wrap)
  call assert_equal(1, options.drag)
  call assert_equal('Another', options.highlight)

  call popup_close(winid)
endfunc

func Test_popupwin_garbage_collect()
  func MyPopupFilter(x, winid, c)
    " NOP
  endfunc

  let winid = popup_create('something', {'filter': function('MyPopupFilter', [{}])})
  call test_garbagecollect_now()
  redraw
  " Must not crach caused by invalid memory access
  call feedkeys('j', 'xt')
  call assert_true(v:true)

  call popup_close(winid)
  delfunc MyPopupFilter
endfunc
