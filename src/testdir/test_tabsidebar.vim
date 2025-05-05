" Tests for tabsidebar

source check.vim
source screendump.vim
CheckFeature tabsidebar

function! s:reset()
  set showtabsidebar&
  set tabsidebarcolumns&
  set tabsidebar&
  set tabsidebaralign&
  set tabsidebarwrap&
endfunc

function! Test_tabsidebar_showtabsidebar()
  set showtabsidebar&
  call assert_equal(0, &showtabsidebar)
  set showtabsidebar=0
  call assert_equal(0, &showtabsidebar)
  set showtabsidebar=1
  call assert_equal(1, &showtabsidebar)
  set showtabsidebar=2
  call assert_equal(2, &showtabsidebar)
  let &showtabsidebar = 0
  call assert_equal(0, &showtabsidebar)
  let &showtabsidebar = 1
  call assert_equal(1, &showtabsidebar)
  let &showtabsidebar = 2
  call assert_equal(2, &showtabsidebar)
  call s:reset()
endfunc

function! Test_tabsidebar_tabsidebarcolumns()
  set tabsidebarcolumns&
  call assert_equal(0, &tabsidebarcolumns)
  set tabsidebarcolumns=0
  call assert_equal(0, &tabsidebarcolumns)
  set tabsidebarcolumns=5
  call assert_equal(5, &tabsidebarcolumns)
  set tabsidebarcolumns=10
  call assert_equal(10, &tabsidebarcolumns)
  let &tabsidebarcolumns = 0
  call assert_equal(0, &tabsidebarcolumns)
  let &tabsidebarcolumns = 5
  call assert_equal(5, &tabsidebarcolumns)
  let &tabsidebarcolumns = 10
  call assert_equal(10, &tabsidebarcolumns)
  call s:reset()
endfunc

function! Test_tabsidebar_tabsidebar()
  set tabsidebar&
  call assert_equal('', &tabsidebar)
  set tabsidebar=aaa
  call assert_equal('aaa', &tabsidebar)
  let &tabsidebar = 'bbb'
  call assert_equal('bbb', &tabsidebar)
  call s:reset()
endfunc

function! Test_tabsidebar_tabsidebaralign()
  set tabsidebaralign&
  call assert_equal(0, &tabsidebaralign)
  set tabsidebaralign
  call assert_equal(1, &tabsidebaralign)
  set notabsidebaralign
  call assert_equal(0, &tabsidebaralign)
  set tabsidebaralign!
  call assert_equal(1, &tabsidebaralign)
  call s:reset()
endfunc

function! Test_tabsidebar_tabsidebarwrap()
  set tabsidebarwrap&
  call assert_equal(0, &tabsidebarwrap)
  set tabsidebarwrap
  call assert_equal(1, &tabsidebarwrap)
  set notabsidebarwrap
  call assert_equal(0, &tabsidebarwrap)
  set tabsidebarwrap!
  call assert_equal(1, &tabsidebarwrap)
  call s:reset()
endfunc

function! Test_tabsidebar_mouse()
  let save_showtabline = &showtabline
  let save_mouse = &mouse
  set showtabline=0 mouse=a

  tabnew
  tabnew

  call test_setmouse(1, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(3, tabpagenr())

  set showtabsidebar=2 tabsidebarcolumns=10

  call test_setmouse(1, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(1, tabpagenr())
  call test_setmouse(2, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(2, tabpagenr())
  call test_setmouse(3, 1)
  call feedkeys("\<LeftMouse>", 'xt')
  call assert_equal(3, tabpagenr())

  tabonly!
  call s:reset()
  let &mouse = save_mouse
  let &showtabline = save_showtabline
endfunc

function! Test_tabsidebar_drawing()
  CheckScreendump

  let lines =<< trim END
    function! MyTabsidebar()
      let n = g:actual_curtabpage
      let hi = n == tabpagenr() ? 'TabLineSel' : 'TabLine'
      let label = printf("\n%%#%sTabNumber#%d:%%#%s#", hi, n, hi)
      let label ..= '%1*%f%*'
      return label
    endfunction
    hi User1 ctermfg=12

    set showtabline=0
    set showtabsidebar=0
    set tabsidebarcolumns=16
    set tabsidebar=
    silent edit Xtabsidebar1

    nnoremap \01 <Cmd>set showtabsidebar=2<CR>
    nnoremap \02 <C-w>v
    nnoremap \03 <Cmd>call setline(1, ['a', 'b', 'c'])<CR>
    nnoremap \04 <Cmd>silent tabnew Xtabsidebar2<CR><Cmd>call setline(1, ['d', 'e', 'f'])<CR>
    nnoremap \05 <Cmd>set tabsidebar=%!MyTabsidebar()<CR>
    nnoremap \06 <Cmd>set tabsidebaralign<CR>
    nnoremap \07 <Cmd>set tabsidebarcolumns=10<CR>
    nnoremap \08 <Cmd>set tabsidebarwrap<CR>
    nnoremap \09 gt
    nnoremap \10 <Cmd>set notabsidebaralign<CR>
    nnoremap \11 <Cmd>set showtabsidebar=1 fillchars+=tabsidebar:<Bslash><Bar><CR>
    nnoremap \12 <Cmd>tab terminal NONE<CR><C-w>N
    nnoremap \13 <Cmd>tabclose!<CR><Cmd>tabclose!<CR>
  END
  call writefile(lines, 'XTest_tabsidebar', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar', {'rows': 6, 'cols': 45})

  call VerifyScreenDump(buf, 'Test_tabsidebar_drawing_00', {})

  for i in range(1, 13)
    let n = printf('%02d', i)
    call term_sendkeys(buf, '\' .. n)
    call VerifyScreenDump(buf, 'Test_tabsidebar_drawing_' .. n, {})
  endfor

  call StopVimInTerminal(buf)
endfunc

function! Test_tabsidebar_drawing_with_popupwin()
  CheckScreendump

  let lines =<< trim END
    set showtabsidebar=2
    set tabsidebarcolumns=20
    set showtabline=0
    tabnew
    setlocal buftype=nofile
    call setbufline(bufnr(), 1, repeat([repeat('.', &columns - &tabsidebarcolumns)], &lines))
    highlight TestingForTabSideBarPopupwin guibg=#7777ff guifg=#000000
    for line in [1, &lines]
      for col in [1, &columns - &tabsidebarcolumns - 2]
        call popup_create([
          \   '@',
          \ ], {
          \   'line': line,
          \   'col': col,
          \   'border': [],
          \   'highlight': 'TestingForTabSideBarPopupwin',
          \ })
      endfor
    endfor
    call cursor(4, 10)
    call popup_atcursor('atcursor', {
      \   'highlight': 'TestingForTabSideBarPopupwin',
      \ })
  END
  call writefile(lines, 'XTest_tabsidebar_with_popupwin', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar_with_popupwin', {'rows': 10, 'cols': 45})

  call VerifyScreenDump(buf, 'Test_tabsidebar_drawing_with_popupwin_0', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabsidebar_drawing_fill_tailing()
  CheckScreendump

  let lines =<< trim END
    set showtabsidebar=2
    set tabsidebarcolumns=20
    set showtabline=0
    e aaa.txt
    tabnew
    e bbb.txt
    let &tabsidebar = "abc"
    redraw!
    " Check whether "abc" is cleared
    let &tabsidebar = "\nTOP\n%f\nBOTTOM"
  END
  call writefile(lines, 'XTest_tabsidebar_fill_tailing', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar_fill_tailing', {'rows': 10, 'cols': 45})

  call VerifyScreenDump(buf, 'Test_tabsidebar_drawing_fill_tailing_0', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabsidebar_drawing_pum()
  CheckScreendump

  let lines =<< trim END
    set showtabsidebar=2
    set tabsidebarcolumns=20
    set showtabline=0
    e aaa.txt
    tabnew
    e bbb.txt
  END
  call writefile(lines, 'XTest_tabsidebar_pum', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar_pum', {'rows': 10, 'cols': 45})

  call term_sendkeys(buf, "i\<C-x>\<C-v>")
  call VerifyScreenDump(buf, 'Test_tabsidebar_drawing_pum_0', {})

  call term_sendkeys(buf, "\<cr>  ab\<C-x>\<C-v>")
  call VerifyScreenDump(buf, 'Test_tabsidebar_drawing_pum_1', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabsidebar_scrolling()
  CheckScreendump

  let lines =<< trim END
    set showtabsidebar=2
    set tabsidebarcolumns=20
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
  call writefile(lines, 'XTest_tabsidebar_scrolling', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar_scrolling', {'rows': 10, 'cols': 45})
  let n = 0
  for c in ['H', 'J', 'K', 'L']
    call term_sendkeys(buf, ":wincmd " .. c ..  "\<cr>")
    call term_sendkeys(buf, "\<C-d>\<C-d>")
    call term_sendkeys(buf, "r@")
    call VerifyScreenDump(buf, 'Test_tabsidebar_drawing_scrolling_' .. n, {})
    let n += 1
  endfor

  call StopVimInTerminal(buf)
endfunc

function! Test_tabsidebar_many_tabpages()
  CheckScreendump

  let lines =<< trim END
    set showtabsidebar=2
    set tabsidebarcolumns=10
    set tabsidebarwrap
    set showtabline=0
    set tabsidebar=%{g:actual_curtabpage}:%f
    execute join(repeat(['tabnew'], 20), ' | ')
  END
  call writefile(lines, 'XTest_tabsidebar_many_tabpages', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar_many_tabpages', {'rows': 10, 'cols': 45})
  for n in range(0, 3)
    call term_sendkeys(buf, "gt")
    call VerifyScreenDump(buf, 'Test_tabsidebar_many_tabpages_' .. n, {})
  endfor
  call term_sendkeys(buf, ":tabnext +10\<cr>")
  call term_sendkeys(buf, ":tabnext -3\<cr>")
  call VerifyScreenDump(buf, 'Test_tabsidebar_many_tabpages_4', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabsidebar_visual()
  CheckScreendump

  let lines =<< trim END
    set showtabsidebar=2
    set tabsidebarcolumns=10
    set showtabline=0
    tabnew
    call setbufline(bufnr(), 1, ['aaa1 bbb1 ccc1 ddd1', 'aaa2 bbb2 ccc2 ddd2', 'aaa3 bbb3 ccc3 ddd3', 'aaa4 bbb4 ccc4 ddd4'])
  END
  call writefile(lines, 'XTest_tabsidebar_visual', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar_visual', {'rows': 10, 'cols': 45})
  call term_sendkeys(buf, "v2w")
  call VerifyScreenDump(buf, 'Test_tabsidebar_visual_0', {})
  call term_sendkeys(buf, "\<esc>0jw")
  call term_sendkeys(buf, "v2wge")
  call VerifyScreenDump(buf, 'Test_tabsidebar_visual_1', {})
  call term_sendkeys(buf, "y:echo @\"\<cr>")
  call VerifyScreenDump(buf, 'Test_tabsidebar_visual_2', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabsidebar_commandline()
  CheckScreendump

  let lines =<< trim END
    set showtabsidebar=2
    set tabsidebarcolumns=10
    set showtabline=0
    tabnew
  END
  call writefile(lines, 'XTest_tabsidebar_commandline', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar_commandline', {'rows': 10, 'cols': 45})
  call term_sendkeys(buf, ":ab\<tab>")
  call VerifyScreenDump(buf, 'Test_tabsidebar_commandline_0', {})

  call term_sendkeys(buf, "\<esc>")
  call term_sendkeys(buf, ":set wildoptions=pum\<cr>")
  call term_sendkeys(buf, ":ab\<tab>")
  call VerifyScreenDump(buf, 'Test_tabsidebar_commandline_1', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabsidebar_tabline_and_tabsidebar()
  CheckScreendump

  let lines =<< trim END
    set showtabsidebar=2
    set tabsidebarcolumns=10
    set showtabline=2
    set fillchars+=tabsidebar:@
    e aaa.txt
    tabnew
    e bbb.txt
    tabnew
    e ccc.txt
  END
  call writefile(lines, 'XTest_tabsidebar_tabline_and_tabsidebar', 'D')

  let buf = RunVimInTerminal('-S XTest_tabsidebar_tabline_and_tabsidebar', {'rows': 10, 'cols': 45})
  call VerifyScreenDump(buf, 'Test_tabsidebar_tabline_and_tabsidebar_0', {})

  call StopVimInTerminal(buf)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
