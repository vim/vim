" Tests for tabpanel

source check.vim
source screendump.vim
CheckFeature tabpanel

function! s:reset()
  set tabpanel&
  set tabpanelopt&
  set showtabpanel&
endfunc

function! Test_tabpanel_mouse()
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

  tabonly!
  call s:reset()
  let &mouse = save_mouse
  let &showtabline = save_showtabline
endfunc

function! Test_tabpanel_drawing()
  CheckScreendump

  let lines =<< trim END
    function! MyTabPanel()
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
    nnoremap \07 <Cmd>set tabpanelopt+=columns:10<CR>
    nnoremap \08 <Cmd>set tabpanelopt+=wrap<CR>
    nnoremap \09 gt
    nnoremap \10 <Cmd>set tabpanelopt-=align:right<CR>
    nnoremap \11 <Cmd>set showtabpanel=1 tabpanelopt+=vert:<Bslash><Bar><CR>
    nnoremap \12 <Cmd>tab terminal NONE<CR><C-w>N
    nnoremap \13 <Cmd>tabclose!<CR><Cmd>tabclose!<CR>
  END
  call writefile(lines, 'XTest_tabpanel', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel', {'rows': 6, 'cols': 45})

  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_00', {})

  for i in range(1, 13)
    let n = printf('%02d', i)
    call term_sendkeys(buf, '\' .. n)
    if i == 10
      call term_sendkeys(buf, ":redraw!\<cr>")
    endif
    call VerifyScreenDump(buf, 'Test_tabpanel_drawing_' .. n, {})
  endfor

  call StopVimInTerminal(buf)
endfunc

function! Test_tabpanel_drawing_with_popupwin()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:20
    set showtabline=0
    tabnew
    setlocal buftype=nofile
    call setbufline(bufnr(), 1, repeat([repeat('.', &columns - 20)], &lines))
    highlight TestingForTabPanelPopupwin guibg=#7777ff guifg=#000000
    for line in [1, &lines]
      for col in [1, &columns - 20 - 2]
        call popup_create([
          \   '@',
          \ ], {
          \   'line': line,
          \   'col': col,
          \   'border': [],
          \   'highlight': 'TestingForTabPanelPopupwin',
          \ })
      endfor
    endfor
    call cursor(4, 10)
    call popup_atcursor('atcursor', {
      \   'highlight': 'TestingForTabPanelPopupwin',
      \ })
  END
  call writefile(lines, 'XTest_tabpanel_with_popupwin', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_with_popupwin', {'rows': 10, 'cols': 45})

  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_with_popupwin_0', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabpanel_drawing_fill_tailing()
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

function! Test_tabpanel_drawing_pum()
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

  call term_sendkeys(buf, "i\<C-x>\<C-v>")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_pum_0', {})

  call term_sendkeys(buf, "\<cr>  ab\<C-x>\<C-v>")
  call VerifyScreenDump(buf, 'Test_tabpanel_drawing_pum_1', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabpanel_scrolling()
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
    call term_sendkeys(buf, ":wincmd " .. c ..  "\<cr>")
    call term_sendkeys(buf, "\<C-d>\<C-d>")
    call term_sendkeys(buf, "r@")
    call VerifyScreenDump(buf, 'Test_tabpanel_drawing_scrolling_' .. n, {})
    let n += 1
  endfor

  call StopVimInTerminal(buf)
endfunc

function! Test_tabpanel_many_tabpages()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10,wrap
    set showtabline=0
    set tabpanel=%{g:actual_curtabpage}:%f
    execute join(repeat(['tabnew'], 20), ' | ')
  END
  call writefile(lines, 'XTest_tabpanel_many_tabpages', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_many_tabpages', {'rows': 10, 'cols': 45})
  for n in range(0, 3)
    call term_sendkeys(buf, "gt")
    call VerifyScreenDump(buf, 'Test_tabpanel_many_tabpages_' .. n, {})
  endfor
  call term_sendkeys(buf, ":tabnext +10\<cr>")
  call term_sendkeys(buf, ":tabnext -3\<cr>")
  call VerifyScreenDump(buf, 'Test_tabpanel_many_tabpages_4', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabpanel_visual()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10
    set showtabline=0
    tabnew
    call setbufline(bufnr(), 1, ['aaa1 bbb1 ccc1 ddd1', 'aaa2 bbb2 ccc2 ddd2', 'aaa3 bbb3 ccc3 ddd3', 'aaa4 bbb4 ccc4 ddd4'])
  END
  call writefile(lines, 'XTest_tabpanel_visual', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_visual', {'rows': 10, 'cols': 45})
  call term_sendkeys(buf, "v2w")
  call VerifyScreenDump(buf, 'Test_tabpanel_visual_0', {})
  call term_sendkeys(buf, "\<esc>0jw")
  call term_sendkeys(buf, "v2wge")
  call VerifyScreenDump(buf, 'Test_tabpanel_visual_1', {})
  call term_sendkeys(buf, "y:echo @\"\<cr>")
  call VerifyScreenDump(buf, 'Test_tabpanel_visual_2', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabpanel_commandline()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10
    set showtabline=0
    tabnew
  END
  call writefile(lines, 'XTest_tabpanel_commandline', 'D')

  let buf = RunVimInTerminal('-S XTest_tabpanel_commandline', {'rows': 10, 'cols': 45})
  call term_sendkeys(buf, ":ab\<tab>")
  call VerifyScreenDump(buf, 'Test_tabpanel_commandline_0', {})

  call term_sendkeys(buf, "\<esc>")
  call term_sendkeys(buf, ":set wildoptions=pum\<cr>")
  call term_sendkeys(buf, ":ab\<tab>")
  call VerifyScreenDump(buf, 'Test_tabpanel_commandline_1', {})

  call StopVimInTerminal(buf)
endfunc

function! Test_tabpanel_tabline_and_tabpanel()
  CheckScreendump

  let lines =<< trim END
    set showtabpanel=2
    set tabpanelopt=columns:10,vert:@
    set showtabline=2
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

" vim: shiftwidth=2 sts=2 expandtab
