" Tests for tabpanel

source util/screendump.vim
CheckFeature tabpanel

function s:reset()
  set tabpanel&
  set tabpanelopt&
  set showtabpanel&
endfunc

function Test_tabpanel_showtabpanel_eq_0()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set noruler
    call setbufline(bufnr(), 1, ['aaa','bbb','ccc','ddd'])
    tabnew 0000
  END
  call writefile(lines, 'XTest_tabpanel_stpl_eq_0', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_stpl_eq_0', {'rows': 10, 'cols': 78})
  call term_sendkeys(buf, ":set showtabpanel=0\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_0_0', {})
  call term_sendkeys(buf, ":tabnext\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_0_1', {})
  call term_sendkeys(buf, ":set showtabpanel=2\<CR>")
  call term_sendkeys(buf, ":vsp aaa\<CR>:vsp bbb\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_0_2', {})
  call term_sendkeys(buf, ":set showtabpanel=0\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_0_3', {})
  call term_sendkeys(buf, ":wincmd |\<CR>")
  call term_sendkeys(buf, ":set showtabpanel=2\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_0_2', {})
  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_showtabpanel_eq_1()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=1
    set noruler
  END
  call writefile(lines, 'XTest_tabpanel_stpl_eq_1', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_stpl_eq_1', {'rows': 10, 'cols': 78})
  call term_sendkeys(buf, "\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_1_0', {})
  call term_sendkeys(buf, ":tabnew\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_1_1', {})
  call term_sendkeys(buf, ":tabfirst\<CR>:vsplit\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_1_2', {})
  call term_sendkeys(buf, ":tabclose\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_1_0', {})

  call term_sendkeys(buf, ":set tabpanelopt=align:right\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_1_0', {})
  call term_sendkeys(buf, ":tabnew\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_1_3', {})
  call term_sendkeys(buf, ":tabfirst\<CR>:vsplit\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_1_4', {})
  call term_sendkeys(buf, ":tabclose\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_stpl_eq_1_0', {})
  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_with_vsplit()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:20
    set showtabline=0
    tabnew
  END
  call writefile(lines, 'XTest_tabpanel_with_vsplit', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_with_vsplit', {'rows': 10, 'cols': 78})
  call VerifyScreenDump(buf, 'Test_tabpanel_with_vsplit_0', {})
  call term_sendkeys(buf, ":vsplit\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_vsplit_1', {})
  call term_sendkeys(buf, ":vsplit\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_vsplit_2', {})

  call term_sendkeys(buf, ":only\<CR>")
  call term_sendkeys(buf, ":set tabpanelopt=align:right,vert\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_vsplit_3', {})
  call term_sendkeys(buf, ":vsplit\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_vsplit_4', {})
  call term_sendkeys(buf, ":vsplit\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_vsplit_5', {})
  call StopVimInTerminal(buf)
endfunc

func Call_cmd_funcs()
  let g:results = [getcmdpos(), getcmdscreenpos(), getcmdline()]
endfunc

function Test_tabpanel_cmdline()
  let save_showtabline = &showtabline
  let g:results = []
  cnoremap <expr> <F2> Call_cmd_funcs()

  set showtabline=0 showtabpanel=0
  call Call_cmd_funcs()
  call assert_equal([0, 0, ''], g:results)
  call feedkeys(":\<F2>\<Esc>", "xt")
  call assert_equal([1, 2, ''], g:results)
  call feedkeys(":pwd\<F2>\<Esc>", "xt")
  call assert_equal([4, 5, 'pwd'], g:results)

  set showtabline=2 showtabpanel=2 tabpanelopt=columns:20,align:left
  call Call_cmd_funcs()
  call assert_equal([0, 0, ''], g:results)
  call feedkeys(":\<F2>\<Esc>", "xt")
  call assert_equal([1, 22, ''], g:results)
  call feedkeys(":pwd\<F2>\<Esc>", "xt")
  call assert_equal([4, 25, 'pwd'], g:results)

  set showtabline=2 showtabpanel=2 tabpanelopt+=align:right
  call Call_cmd_funcs()
  call assert_equal([0, 0, ''], g:results)
  call feedkeys(":\<F2>\<Esc>", "xt")
  call assert_equal([1, 2, ''], g:results)
  call feedkeys(":pwd\<F2>\<Esc>", "xt")
  call assert_equal([4, 5, 'pwd'], g:results)

  unlet g:results
  cunmap <F2>
  call s:reset()
  let &showtabline = save_showtabline
endfunc

function Test_tabpanel_mouse()
  let save_showtabline = &showtabline
  let save_mouse = &mouse
  set showtabline=0 mouse=a

  tabnew
  tabnew

  call test_setmouse(1, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(3, tabpagenr())

  set showtabpanel=2 tabpanelopt=columns:10

  call test_setmouse(1, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(1, tabpagenr())
  call test_setmouse(2, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(2, tabpagenr())
  call test_setmouse(3, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(3, tabpagenr())
  call test_setmouse(&lines, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(1, tabpagenr())

  " Drag the active tab page
  tablast
  call test_setmouse(3, 1)
  call feedkeys("\<LeftMouse>\<LeftDrag>", 'xt')
  call test_setmouse(2, 1)
  call feedkeys("\<LeftDrag>", 'xt')
  call assert_equal(3, tabpagenr())
  call feedkeys("\<LeftRelease>", 'xt')
  tabmove $

  " Drag the inactive tab page
  tablast
  call test_setmouse(2, 1)
  call feedkeys("\<LeftMouse>\<LeftDrag>", 'xt')
  call test_setmouse(1, 1)
  call feedkeys("\<LeftDrag>", 'xt')
  call assert_equal(2, tabpagenr())
  call feedkeys("\<LeftRelease>", 'xt')
  tabmove 2

  " Confirm that tabpagenr() does not change when dragging outside the tabpanel
  tablast
  call test_setmouse(3, 30)
  call feedkeys("\<LeftMouse>\<LeftDrag>", 'xt')
  call test_setmouse(1, 30)
  call feedkeys("\<LeftDrag>", 'xt')
  call feedkeys("\<LeftRelease>", 'xt')
  call assert_equal(3, tabpagenr())

  " Test getmousepos()
  call test_setmouse(2, 3)
  call feedkeys("\<LeftMouse>", 'xt')
  let pos = getmousepos()
  call assert_equal(0, pos['winid'])
  call assert_equal(0, pos['winrow'])
  call assert_equal(0, pos['wincol'])
  call assert_equal(2, pos['screenrow'])
  call assert_equal(3, pos['screencol'])

  call test_setmouse(1, 11)
  call feedkeys("\<LeftMouse>", 'xt')
  let pos = getmousepos()
  call assert_notequal(0, pos['winid'])
  call assert_equal(1, pos['winrow'])
  call assert_equal(1, pos['wincol'])
  call assert_equal(1, pos['screenrow'])
  call assert_equal(11, pos['screencol'])

  new
  wincmd x

  call test_setmouse(10, 11)
  call feedkeys("\<LeftMouse>", 'xt')
  let pos = getmousepos()
  call assert_notequal(0, pos['winid'])
  call assert_equal(10, pos['winrow'])
  call assert_equal(1, pos['wincol'])
  call assert_equal(10, pos['screenrow'])
  call assert_equal(11, pos['screencol'])

  tabonly!
  call s:reset()
  let &mouse = save_mouse
  let &showtabline = save_showtabline
endfunc

function Test_tabpanel_drawing()
  CheckScreendump

  let lines =<< trim END
    function MyTabPanel()
      let n = g:actual_curtabpage
      let hi = n == tabpagenr() ? 'TabLineSel' : 'TabLine'
      let label = printf("\n%%#%sTabNumber#%d:%%#%s#", hi, n, hi)
      let label ..= '%1*%f%*'
      return label
    endfunction
    hi User1 ctermfg=12

    set showtabline=0
    set showtabpanel=0
    set tabpanelopt=columns:16
    set tabpanel=
    silent edit Xtabpanel1

    nnoremap \01 <Cmd>set showtabpanel=2<CR>
    nnoremap \02 <C-w>v
    nnoremap \03 <Cmd>call setline(1, ['a', 'b', 'c'])<CR>
    nnoremap \04 <Cmd>silent tabnew Xtabpanel2<CR><Cmd>call setline(1, ['d', 'e', 'f'])<CR>
    nnoremap \05 <Cmd>set tabpanel=%!MyTabPanel()<CR>
    nnoremap \06 <Cmd>set tabpanelopt+=align:right<CR>
    nnoremap \07 <Cmd>tab terminal NONE<CR><C-w>N
    nnoremap \08 <Cmd>tabclose!<CR><Cmd>tabclose!<CR>
  END
  call writefile(lines, 'XTest_tabpanel', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel', {'rows': 6, 'cols': 45})

  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_00', {})

  for i in range(1, 8)
    let n = printf('%02d', i)
    call term_sendkeys(buf, '\' .. n)
    call VerifyScreenDump(buf, 'Test_tabpanel_drawing_' .. n, {})
  endfor

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_drawing_2()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=align:right,vert
    call setbufline(bufnr(), 1, ['', 'aaa'])
  END
  call writefile(lines, 'XTest_tabpanel_drawing_2', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_drawing_2', {'rows': 10, 'cols': 78})
  call term_sendkeys(buf, "ggo")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_2_0', {})

  call term_sendkeys(buf, "\<Esc>u:set tabpanelopt+=align:left\<CR>")
  call term_sendkeys(buf, "ggo")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_2_1', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_drawing_with_popupwin()
  CheckScreendump

  let tcols = 45
  let lines =<< trim END
    set showtabpanel=0
    set tabpanelopt=columns:20
    set showtabline=0
    set nowrap
    set noruler
    tabnew
    setlocal buftype=nofile
    call setbufline(bufnr(), 1, repeat([repeat('.', &columns)], &lines - &ch))
    for col in [1, &columns - 2]
      call popup_create(['@'],
            \ {
            \   'line': 1,
            \   'col': col,
            \   'border': [],
            \   'highlight': 'ErrorMsg',
            \ })
    endfor
    call cursor(5, 10)
    call popup_atcursor('atcursor', {
      \   'highlight': 'Question',
      \ })
  END
  call writefile(lines, 'XTest_tabpanel_with_popupwin', 'D')
  let buf = RunVimInTerminal('-S XTest_tabpanel_with_popupwin', {'rows': 10, 'cols': tcols})
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_with_popupwin_0', {})
  call term_sendkeys(buf, ":set showtabpanel=2\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_with_popupwin_1', {})
  call term_sendkeys(buf, ":set tabpanelopt+=align:right\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_with_popupwin_2', {})
  call term_sendkeys(buf, ":set showtabpanel=0\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_with_popupwin_0', {})
  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_drawing_fill_tailing()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:20
    set showtabline=0
    e aaa.txt
    tabnew
    e bbb.txt
    let &tabpanel = "abc"
    redraw!
    " Check whether "abc" is cleared
    let &tabpanel = "\nTOP\n%f\nBOTTOM"
  END
  call writefile(lines, 'XTest_tabpanel_fill_tailing', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_fill_tailing', {'rows': 10, 'cols': 45})

  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_fill_tailing_0', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_drawing_pum()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:20
    set showtabline=0
    e aaa.txt
    tabnew
    e bbb.txt
  END
  call writefile(lines, 'XTest_tabpanel_pum', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_pum', {'rows': 10, 'cols': 45})

  call term_sendkeys(buf, "i\<CR>aa\<CR>aaaa\<CR>aaac\<CR>aaab\<CR>\<Esc>")
  call term_sendkeys(buf, "ggi\<C-X>\<C-N>")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_pum_0', {})

  call term_sendkeys(buf, "\<Esc>Go  a\<C-X>\<C-P>")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_pum_1', {})

  call term_sendkeys(buf, "\<C-U>\<CR>\<Esc>")
  call term_sendkeys(buf, ":set tabpanelopt+=align:right\<CR>")
  let num = 45 - 20 - 2  " term-win-width - tabpanel-columns - 2
  call term_sendkeys(buf, num .. "a \<Esc>")
  call term_sendkeys(buf, "a\<C-X>\<C-N>")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_pum_2', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_scrolling()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:20
    set showtabline=0
    set nowrap
    set number
    e aaa.txt
    tabnew
    e bbb.txt
    vsplit
    call setbufline(bufnr(), 1, repeat(['text text text text'], 100))
    wincmd =
  END
  call writefile(lines, 'XTest_tabpanel_scrolling', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_scrolling', {'rows': 10, 'cols': 45})
  let n = 0
  for c in ['H', 'J', 'K', 'L']
    call term_sendkeys(buf, ":wincmd " .. c ..  "\<CR>")
    call term_sendkeys(buf, "\<C-d>\<C-d>")
    call term_sendkeys(buf, "r@")
    call VerifyScreenDump(buf, 'Test_tabpanel_drawing_scrolling_' .. n, {})
    let n += 1
  endfor

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_many_tabpages()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10
    set showtabline=0
    set tabpanel=%{g:actual_curtabpage}:tab
    execute join(repeat(['tabnew'], 20), ' | ')
  END
  call writefile(lines, 'XTest_tabpanel_many_tabpages', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_many_tabpages', {'rows': 10, 'cols': 45})
  for n in range(0, 3)
    call term_sendkeys(buf, "gt")
    call VerifyScreenDump(buf, 'Test_tabpanel_many_tabpages_' .. n, {})
  endfor
  call term_sendkeys(buf, ":tabnext +10\<CR>")
  call term_sendkeys(buf, ":tabnext -3\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_many_tabpages_4', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_visual()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10
    set showtabline=0 laststatus=2
    tabnew
    call setbufline(bufnr(), 1, ['aaa1 bbb1 ccc1 ddd1', 'aaa2 bbb2 ccc2 ddd2', 'aaa3 bbb3 ccc3 ddd3', 'aaa4 bbb4 ccc4 ddd4'])
  END
  call writefile(lines, 'XTest_tabpanel_visual', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_visual', {'rows': 10, 'cols': 45})
  call term_sendkeys(buf, "v2w")
  call VerifyScreenDump(buf, 'Test_tabpanel_visual_0', {})
  call term_sendkeys(buf, "\<Esc>0jw")
  call term_sendkeys(buf, "v2wge")
  call VerifyScreenDump(buf, 'Test_tabpanel_visual_1', {})
  call term_sendkeys(buf, "y:echo @\"\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_visual_2', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_commandline()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10
    set showtabline=0
    tabnew
  END
  call writefile(lines, 'XTest_tabpanel_commandline', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_commandline', {'rows': 10, 'cols': 45})
  call term_sendkeys(buf, ":ab\<Tab>")
  call VerifyScreenDump(buf, 'Test_tabpanel_commandline_0', {})

  call term_sendkeys(buf, "\<Esc>")
  call term_sendkeys(buf, ":set wildoptions=pum\<CR>")
  call term_sendkeys(buf, ":ab\<Tab>")
  call VerifyScreenDump(buf, 'Test_tabpanel_commandline_1', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_tabline_and_tabpanel()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10,vert
    set fillchars=tpl_vert:│
    set showtabline=2 laststatus=2
    e aaa.txt
    tabnew
    e bbb.txt
    tabnew
    e ccc.txt
  END
  call writefile(lines, 'XTest_tabpanel_tabline_and_tabpanel', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_tabline_and_tabpanel', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabpanel_tabline_and_tabpanel_0', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_dont_overflow_into_tabpanel()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10
    set showtabline=2
    tabnew
    call setline(1, repeat('x', 100))
    set wrap
  END
  call writefile(lines, 'XTest_tabpanel_dont_overflow_into_tabpanel', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_dont_overflow_into_tabpanel', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabpanel_dont_overflow_into_tabpanel_0', {})

  call StopVimInTerminal(buf)
endfunc

"""function Test_tabpanel_dont_vert_is_multibytes_left()
"""  CheckScreendump
"""
"""  let lines =<< trim END
"""    set showtabpanel=2
"""    set tabpanelopt=columns:10,vert
"""    set fillchars=tpl_vert:│
"""    set showtabline=2
"""    tabnew
"""  END
"""  call writefile(lines, 'XTest_tabpanel_vert_is_multibyte_lefts', 'D')
"""
"""  let buf = RunVimInTerminal('-S XTest_tabpanel_vert_is_multibyte_lefts', {'rows': 10, 'cols': 45})
"""  call VerifyScreenDump(buf, 'Test_tabpanel_vert_is_multibytes_left_0', {})
"""
"""  call term_sendkeys(buf, ":set tabpanelopt=columns:1,vert\<CR>")
"""  call VerifyScreenDump(buf, 'Test_tabpanel_vert_is_multibytes_left_1', {})
"""
"""  call term_sendkeys(buf, ":set tabpanelopt=columns:10,vert\<CR>")
"""  call VerifyScreenDump(buf, 'Test_tabpanel_vert_is_multibytes_left_2', {})
"""
"""  call term_sendkeys(buf, ":set tabpanelopt=columns:2,vert\<CR>")
"""  call VerifyScreenDump(buf, 'Test_tabpanel_vert_is_multibytes_left_3', {})
"""
"""  call StopVimInTerminal(buf)
"""endfunc

"""function Test_tabpanel_dont_vert_is_multibytes_right()
"""  CheckScreendump
"""
"""  let lines =<< trim END
"""    set showtabpanel=2
"""    set tabpanelopt=align:right,columns:10,vert
"""    set fillchars=tpl_vert:│
"""    set showtabline=2
"""    tabnew
"""  END
"""  call writefile(lines, 'XTest_tabpanel_vert_is_multibytes_right', 'D')
"""
"""  let buf = RunVimInTerminal('-S XTest_tabpanel_vert_is_multibytes_right', {'rows': 10, 'cols': 45})
"""  call VerifyScreenDump(buf, 'Test_tabpanel_vert_is_multibytes_right_0', {})
"""
"""  call term_sendkeys(buf, ":set tabpanelopt=align:right,columns:1,vert\<CR>")
"""  call VerifyScreenDump(buf, 'Test_tabpanel_vert_is_multibytes_right_1', {})
"""
"""  call term_sendkeys(buf, ":set tabpanelopt=align:right,columns:10,vert\<CR>")
"""  call VerifyScreenDump(buf, 'Test_tabpanel_vert_is_multibytes_right_2', {})
"""
"""  call term_sendkeys(buf, ":set tabpanelopt=align:right,columns:2,vert\<CR>")
"""  call VerifyScreenDump(buf, 'Test_tabpanel_vert_is_multibytes_right_3', {})
"""
"""  call StopVimInTerminal(buf)
"""endfunc

function Test_tabpanel_eval_tabpanel_statusline_tabline()
  CheckScreendump

  let lines =<< trim END
    function Expr()
      return "$%=[%f]%=$"
    endfunction
    set laststatus=2
    set showtabline=2
    set showtabpanel=2
    set statusline=%!Expr()
    set tabline=%!Expr()
    set tabpanel=%!Expr()
    set tabpanelopt=columns:10,vert
    e aaa
    tabnew
    e bbb
    tabnew
    e ccc
  END
  call writefile(lines, 'XTest_tabpanel_eval_tabpanel_statusline_tabline', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_eval_tabpanel_statusline_tabline', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabpanel_eval_tabpanel_statusline_tabline_0', {})
  call term_sendkeys(buf, ":set tabpanelopt+=align:right\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_eval_tabpanel_statusline_tabline_1', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_noeval_tabpanel_statusline_tabline()
  CheckScreendump

  let lines =<< trim END
    set laststatus=2
    set showtabline=2
    set showtabpanel=2
    set statusline=$%=[%f]%=$
    set tabline=$%=[%f]%=$
    set tabpanel=$%=[%f]%=$
    set tabpanelopt=columns:10,vert
    e aaa
    tabnew
    e bbb
    tabnew
    e ccc
  END
  call writefile(lines, 'XTest_tabpanel_noeval_tabpanel_statusline_tabline', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_noeval_tabpanel_statusline_tabline', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabpanel_noeval_tabpanel_statusline_tabline_0', {})
  call term_sendkeys(buf, ":set tabpanelopt+=align:right\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_noeval_tabpanel_statusline_tabline_1', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_eval_tabpanel_with_linebreaks()
  CheckScreendump

  let lines =<< trim END
    function Expr()
      return "top\n$%=[%f]%=$\nbottom"
    endfunction
    set showtabpanel=2
    set tabpanel=%!Expr()
    set tabpanelopt=columns:10
    set noruler
    e aaa
    tabnew
    e bbb
    tabnew
    e ccc
  END
  call writefile(lines, 'XTest_tabpanel_eval_tabpanel_with_linebreaks', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_eval_tabpanel_with_linebreaks', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabpanel_eval_tabpanel_with_linebreaks_0', {})
  call term_sendkeys(buf, ":set tabpanelopt+=align:right\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_eval_tabpanel_with_linebreaks_1', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_tabonly()
  CheckScreendump

  let lines =<< trim END
    tabnew
    set showtabpanel=1
    norm 100oasdf
    vsplit
  END
  call writefile(lines, 'XTest_tabpanel_tabonly', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_tabonly', {'rows': 10, 'cols': 78})
  call VerifyScreenDump(buf, 'Test_tabpanel_only_0', {})
  call term_sendkeys(buf, ":tabonly\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_only_1', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_equalalways()
  CheckScreendump

  let lines =<< trim END
    tabnew
    set showtabpanel=1
    set tabpanelopt=columns:20
    set equalalways
    split
    vsplit
  END
  call writefile(lines, 'XTest_tabpanel_equalalways', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_equalalways', {'rows': 10, 'cols': 78})
  call VerifyScreenDump(buf, 'Test_tabpanel_equalalways_0', {})
  call term_sendkeys(buf, ":set tabpanelopt=columns:10\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_equalalways_1', {})
  call term_sendkeys(buf, ":set tabpanelopt=columns:30\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_equalalways_2', {})
  call term_sendkeys(buf, ":set tabpanelopt=columns:5\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_equalalways_3', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_quitall()
  CheckScreendump

  let lines =<< trim END
    tabnew
    set showtabpanel=1
    set laststatus=2
    call setline(1, 'aaa')
    normal gt
    silent! quitall
  END
  call writefile(lines, 'XTest_tabpanel_quitall', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_quitall', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabpanel_quitall_0', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_ruler()
  CheckScreendump

  let lines =<< trim END
    tabnew
    set statusline& laststatus=0
    set rulerformat& ruler
    set showtabpanel=1
  END
  call writefile(lines, 'XTest_tabpanel_ruler', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_ruler', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabpanel_ruler_0', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_error()
  set tabpanel=%!NonExistingFunc()
  try
    set showtabpanel=2
    redraw!
  catch /^Vim\%((\a\+)\)\=:E117:/
  endtry
  call assert_true(empty(&tabpanel))

  try
    set tabpanel=%{my#util#TabPanelHighlight}%t
    redraw!
  catch /^Vim\%((\a\+)\)\=:E121:/
  endtry
  call assert_true(empty(&tabpanel))

  set tabpanel&vim
  set showtabpanel&vim
endfunc

function Test_tabpanel_with_msg_scrolled()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set noruler
    tabnew
    set modified
    tabfirst
  END
  call writefile(lines, 'XTest_tabpanel_with_msg_scrolled', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_with_msg_scrolled', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabpanel_with_msg_scrolled_0', {})
  call term_sendkeys(buf, ":qa\<CR>")
  call term_sendkeys(buf, "\<CR>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_msg_scrolled_1', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_with_cmdline_pum()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set noruler
    tabnew aaa
    set wildoptions+=pum
    func TimerCb(timer)
      tabnew bbb
    endfunc
    call timer_start(100, 'TimerCb')
  END
  call writefile(lines, 'XTest_tabpanel_with_cmdline_pum', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_with_cmdline_pum', {'rows': 10, 'cols': 45})
  call term_sendkeys(buf, "\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_cmdline_pum_0', {})
  call term_sendkeys(buf, ":set\<Tab>")
  call term_wait(buf, 120)
  call VerifyScreenDump(buf, 'Test_tabpanel_with_cmdline_pum_1', {})
  call term_sendkeys(buf, "\<Esc>:tabclose\<CR>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_cmdline_pum_0', {})

  call StopVimInTerminal(buf)
endfunc

function Test_tabpanel_with_cmdline_no_pum()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set noruler
    tabnew aaa
    set wildoptions-=pum
  END
  call writefile(lines, 'XTest_tabpanel_with_cmdline_pum', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_with_cmdline_pum', {'rows': 10, 'cols': 45})
  call term_sendkeys(buf, "\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_cmdline_no_pum_0', {})
  call term_sendkeys(buf, ":tabne\<Tab>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_cmdline_no_pum_1', {})
  call term_sendkeys(buf, "\<Esc>\<C-L>")
  call VerifyScreenDump(buf, 'Test_tabpanel_with_cmdline_no_pum_0', {})

  call StopVimInTerminal(buf)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
