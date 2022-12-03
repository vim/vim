" Test MS-Windows console event handling.

source check.vim
CheckMSWindows
CheckNotGui
"  Note, I think it should work in gui also - so need to debug


" Test for sending low level key presses
func SendKeys(keylist)
  for k in a:keylist
    call test_mswin_event("keyboard", #{event: "keydown", keycode: k})
  endfor
  for k in reverse(a:keylist)
    call test_mswin_event("keyboard", #{event: "keyup", keycode: k})
  endfor
  sleep 50m
endfunc

" Test MS-Windows console key events
func Test_windows_console_key_event()
  let g:test_is_flaky = 1
  CheckMSWindows
  CheckNotGui
  new

"  " Test keyboard codes for digits
"  " (0x30 - 0x39) : VK_0 - VK_9 are the same as ASCII '0' - '9'
"    for kc in range(48, 57)
"      call SendKeys([kc])
"      let ch = getcharstr()
"      call assert_equal(nr2char(kc), ch)
"    endfor

" Test keyboard code for Spacebar 
  let kc = 0x20
  call SendKeys([kc])
  let ch = getcharstr()
  call assert_equal(nr2char(kc), ch)

"  " Test for lowercase 'a' to 'z', VK codes 65(0x41) - 90(0x5A)
"  " VK_A - VK_Z virtual key codes coincide with uppercase ASCII codes 'A'-'Z'.
"  " eg VK_A is 65 and the ASCII character code for uppercase 'A' is also 65.
"  " Note: these are interpreted as lowercase when Shift is NOT pressed. 
"  " Sending VK_A (65) 'A' Key code without shift modifier, will produce ASCII
"  " char 'a' (91) as the output.  The ASCII codes for the lowercase letters
"  " are 32 higher than their uppercase counterparts.
"    for kc in range(65, 90)
"      call SendKeys([kc])
"      let ch = getcharstr()
"      call assert_equal(nr2char(kc + 32), ch)
"    endfor

"  "  Test for Uppercase 'A' - 'Z' keys
"  "  With VK_SHIFT, expect the keycode = character code.
"    for kc in range(65, 90)
"      call SendKeys([0x10, kc])
"      let ch = getcharstr()
"      call assert_equal(nr2char(kc), ch)
"    endfor

"    " Test for <Ctrl-A> to <Ctrl-Z> keys
"   "  Same as for lowercase, except with Ctrl Key
"   "  Expect the unicode characters 0x01 to 0x1A
"    for kc in range(65, 90)
"      call SendKeys([0x11, kc])
"      let chstr = getcharstr()
"      call assert_equal(nr2char(kc - 64), chstr)
"    endfor

"  " Test keyboard code for <S-Pageup> 
"    "call SendKeys([0x10, 0x21])
"    call SendKeys([0x10,0x21])
"    let ch = getcharstr()
"    "let mod = getcharmod()
"    let keycode = eval('"\<S-Pageup>"')
"    call assert_equal(keycode, ch, "key = S-Pageup")
"    "call assert_equal(2, mod, "key = S-Pageup")

  " Test for the various Ctrl and Shift key combinations.
  " Refer to the following page for the virtual key codes:
  " https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
  let keytests = [
    \ [[0x10, 0x21], "S-Pageup", 2],
    \ [[0xA0, 0x21], "S-Pageup", 2],
    \ [[0xA1, 0x21], "S-Pageup", 2],
    \ [[0x11, 0x21], "C-Pageup", 4],
    \ [[0xA2, 0x21], "C-Pageup", 4],
    \ [[0xA3, 0x21], "C-Pageup", 4],
    \ [[0x11, 0x10, 0x21], "C-S-Pageup", 6],
    \ [[0x10, 0x22], "S-PageDown", 2],
    \ [[0xA0, 0x22], "S-PageDown", 2],
    \ [[0xA1, 0x22], "S-PageDown", 2],
    \ [[0x11, 0x22], "C-PageDown", 4],
    \ [[0xA2, 0x22], "C-PageDown", 4],
    \ [[0xA3, 0x22], "C-PageDown", 4],
    \ [[0x11, 0x10, 0x22], "C-S-PageDown", 6],
    \ [[0x10, 0x23], "S-End", 0],
    \ [[0x11, 0x23], "C-End", 0],
    \ [[0x11, 0x10, 0x23], "C-S-End", 4],
    \ [[0x10, 0x24], "S-Home", 0],
    \ [[0x11, 0x24], "C-Home", 0],
    \ [[0x11, 0x10, 0x24], "C-S-Home", 4],
    \ [[0x10, 0x25], "S-Left", 0],
    \ [[0x11, 0x25], "C-Left", 0],
    \ [[0x11, 0x10, 0x25], "C-S-Left", 4],
    \ [[0x10, 0x26], "S-Up", 0],
    \ [[0x11, 0x26], "C-Up", 4],
    \ [[0x11, 0x10, 0x26], "C-S-Up", 4],
    \ [[0x10, 0x27], "S-Right", 0],
    \ [[0x11, 0x27], "C-Right", 0],
    \ [[0x11, 0x10, 0x27], "C-S-Right", 4],
    \ [[0x10, 0x28], "S-Down", 0],
    \ [[0x11, 0x28], "C-Down", 4],
    \ [[0x11, 0x10, 0x28], "C-S-Down", 4],
    \ [[0x11, 0x30], "C-0", 4],
    \ [[0x11, 0x31], "C-1", 4],
    \ [[0x11, 0x32], "C-2", 4],
    \ [[0x11, 0x33], "C-3", 4],
    \ [[0x11, 0x34], "C-4", 4],
    \ [[0x11, 0x35], "C-5", 4],
    \ [[0x11, 0x36], "C-^", 0],
    \ [[0x11, 0x37], "C-7", 4],
    \ [[0x11, 0x38], "C-8", 4],
    \ [[0x11, 0x39], "C-9", 4],
    \ [[0x11, 0x60], "C-0", 4],
    \ [[0x11, 0x61], "C-1", 4],
    \ [[0x11, 0x62], "C-2", 4],
    \ [[0x11, 0x63], "C-3", 4],
    \ [[0x11, 0x64], "C-4", 4],
    \ [[0x11, 0x65], "C-5", 4],
    \ [[0x11, 0x66], "C-6", 4],
    \ [[0x11, 0x67], "C-7", 4],
    \ [[0x11, 0x68], "C-8", 4],
    \ [[0x11, 0x69], "C-9", 4],
    \ [[0x11, 0x6A], "C-*", 4],
    \ [[0x11, 0x6B], "C-+", 4],
    \ [[0x11, 0x6D], "C--", 4],
    \ [[0x11, 0x70], "C-F1", 4],
    \ [[0x11, 0x10, 0x70], "C-S-F1", 4],
    \ [[0x11, 0x71], "C-F2", 4],
    \ [[0x11, 0x10, 0x71], "C-S-F2", 4],
    \ [[0x11, 0x72], "C-F3", 4],
    \ [[0x11, 0x10, 0x72], "C-S-F3", 4],
    \ [[0x11, 0x73], "C-F4", 4],
    \ [[0x11, 0x10, 0x73], "C-S-F4", 4],
    \ [[0x11, 0x74], "C-F5", 4],
    \ [[0x11, 0x10, 0x74], "C-S-F5", 4],
    \ [[0x11, 0x75], "C-F6", 4],
    \ [[0x11, 0x10, 0x75], "C-S-F6", 4],
    \ [[0x11, 0x76], "C-F7", 4],
    \ [[0x11, 0x10, 0x76], "C-S-F7", 4],
    \ [[0x11, 0x77], "C-F8", 4],
    \ [[0x11, 0x10, 0x77], "C-S-F8", 4],
    \ [[0x11, 0x78], "C-F9", 4],
    \ [[0x11, 0x10, 0x78], "C-S-F9", 4],
    \ ]

"    for [kcodes, kstr, kmod] in keytests
"      call SendKeys(kcodes)
"      let ch = getcharstr(0)
"      let mod = getcharmod()
"      let keycode = eval('"\<' .. kstr .. '>"')
"      call assert_equal(keycode, ch, $"key = {kstr}")
"      call assert_equal(kmod, mod, $"key = {kstr}")
"    endfor

  bw!
endfunc

"  Not ready for this test just yet...
"  " Test MS-Windows console mouse events
"  func Test_windows_console_mouse_event()
"    CheckMSWindows
"    CheckNotGui
"    set mousemodel=extend
"    call test_override('no_query_mouse', 1)
"    new
"    call setline(1, ['one two three', 'four five six'])

"    " place the cursor using left click in normal mode
"    call cursor(1, 1)
"    let args = #{button: 0, row: 2, col: 4, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    eval 'mouse'->test_mswin_event(args)
"    call feedkeys("\<Esc>", 'Lx!')
"    call assert_equal([0, 2, 4, 0], getpos('.'))

"    " select and yank a word
"    let @" = ''
"    let args = #{button: 0, row: 1, col: 9, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.multiclick = 1
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    let args.multiclick = 0
"    call test_mswin_event('mouse', args)
"    call feedkeys("y", 'Lx!')
"    call assert_equal('three', @")

"    " create visual selection using right click
"    let @" = ''
"    let args = #{button: 0, row: 2, col: 6, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    call test_mswin_event('mouse', args)
"    let args = #{button: 2, row: 2, col: 13, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    call test_mswin_event('mouse', args)
"    call feedkeys("y", 'Lx!')
"    call assert_equal('five six', @")

"    " paste using middle mouse button
"    let @* = 'abc '
"    call feedkeys('""', 'Lx!')
"    let args = #{button: 1, row: 1, col: 9, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    call test_mswin_event('mouse', args)
"    call feedkeys("\<Esc>", 'Lx!')
"    call assert_equal(['one two abc three', 'four five six'], getline(1, '$'))

"    " extend visual selection using right click in visual mode
"    let @" = ''
"    call cursor(1, 1)
"    call feedkeys('v', 'Lx!')
"    let args = #{button: 2, row: 1, col: 17, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    call test_mswin_event('mouse', args)
"    call feedkeys("y", 'Lx!')
"    call assert_equal('one two abc three', @")

"    " extend visual selection using mouse drag
"    let @" = ''
"    call cursor(1, 1)
"    let args = #{button: 0, row: 2, col: 1, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args = #{button: 0x43, row: 2, col: 9, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 0x3
"    call test_mswin_event('mouse', args)
"    call feedkeys("y", 'Lx!')
"    call assert_equal('four five', @")

"    " select text by moving the mouse
"    let @" = ''
"    call cursor(1, 1)
"    redraw!
"    let args = #{button: 0, row: 1, col: 4, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 0x700
"    let args.col = 9
"    call test_mswin_event('mouse', args)
"    let args.col = 13
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    call test_mswin_event('mouse', args)
"    call feedkeys("y", 'Lx!')
"    call assert_equal(' two abc t', @")

"    " Using mouse in insert mode
"    call cursor(1, 1)
"    call feedkeys('i', 't')
"    let args = #{button: 0, row: 2, col: 11, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    call test_mswin_event('mouse', args)
"    call feedkeys("po\<Esc>", 'Lx!')
"    call assert_equal(['one two abc three', 'four five posix'], getline(1, '$'))

"    %d _
"    set scrolloff=0
"    call setline(1, range(1, 100))
"    " scroll up
"    let args = #{button: 0x200, row: 2, col: 1, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    call test_mswin_event('mouse', args)
"    call test_mswin_event('mouse', args)
"    call feedkeys("H", 'Lx!')
"    call assert_equal(10, line('.'))

"    " scroll down
"    let args = #{button: 0x100, row: 2, col: 1, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    call test_mswin_event('mouse', args)
"    call feedkeys("H", 'Lx!')
"    call assert_equal(4, line('.'))
"    set scrolloff&

"    %d _
"    set nowrap
"    call setline(1, range(10)->join('')->repeat(10))
"    " scroll left
"    let args = #{button: 0x500, row: 1, col: 5, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.col = 10
"    call test_mswin_event('mouse', args)
"    let args.col = 15
"    call test_mswin_event('mouse', args)
"    call feedkeys('g0', 'Lx!')
"    call assert_equal(19, col('.'))

"    " scroll right
"    let args = #{button: 0x600, row: 1, col: 15, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.col = 10
"    call test_mswin_event('mouse', args)
"    call feedkeys('g0', 'Lx!')
"    call assert_equal(7, col('.'))
"    set wrap&

"    %d _
"    call setline(1, repeat([repeat('a', 60)], 10))

"    " record various mouse events
"    let mouseEventNames = [
"          \ 'LeftMouse', 'LeftRelease', '2-LeftMouse', '3-LeftMouse',
"          \ 'S-LeftMouse', 'A-LeftMouse', 'C-LeftMouse', 'MiddleMouse',
"          \ 'MiddleRelease', '2-MiddleMouse', '3-MiddleMouse',
"          \ 'S-MiddleMouse', 'A-MiddleMouse', 'C-MiddleMouse',
"          \ 'RightMouse', 'RightRelease', '2-RightMouse',
"          \ '3-RightMouse', 'S-RightMouse', 'A-RightMouse', 'C-RightMouse',
"          \ 'X1Mouse', 'S-X1Mouse', 'A-X1Mouse', 'C-X1Mouse', 'X2Mouse',
"          \ 'S-X2Mouse', 'A-X2Mouse', 'C-X2Mouse'
"          \ ]
"    let mouseEventCodes = map(copy(mouseEventNames), "'<' .. v:val .. '>'")
"    let g:events = []
"    for e in mouseEventCodes
"      exe 'nnoremap ' .. e .. ' <Cmd>call add(g:events, "' ..
"            \ substitute(e, '[<>]', '', 'g') .. '")<CR>'
"    endfor

"    " Test various mouse buttons (0 - Left, 1 - Middle, 2 - Right, 0x300 - X1,
"    " 0x300- X2)
"    for button in [0, 1, 2, 0x300, 0x400]
"      " Single click
"      let args = #{button: button, row: 2, col: 5, multiclick: 0, modifiers: 0}
"      call test_mswin_event('mouse', args)
"      let args.button = 3
"      call test_mswin_event('mouse', args)

"      " Double/Triple click is supported by only the Left/Middle/Right mouse
"      " buttons
"      if button <= 2
"        " Double Click
"        let args.button = button
"        call test_mswin_event('mouse', args)
"        let args.multiclick = 1
"        call test_mswin_event('mouse', args)
"        let args.button = 3
"        let args.multiclick = 0
"        call test_mswin_event('mouse', args)

"        " Triple Click
"        let args.button = button
"        call test_mswin_event('mouse', args)
"        let args.multiclick = 1
"        call test_mswin_event('mouse', args)
"        call test_mswin_event('mouse', args)
"        let args.button = 3
"        let args.multiclick = 0
"        call test_mswin_event('mouse', args)
"      endif

"      " Shift click
"      let args = #{button: button, row: 3, col: 7, multiclick: 0, modifiers: 4}
"      call test_mswin_event('mouse', args)
"      let args.button = 3
"      call test_mswin_event('mouse', args)

"      " Alt click
"      let args.button = button
"      let args.modifiers = 8
"      call test_mswin_event('mouse', args)
"      let args.button = 3
"      call test_mswin_event('mouse', args)

"      " Ctrl click
"      let args.button = button
"      let args.modifiers = 16
"      call test_mswin_event('mouse', args)
"      let args.button = 3
"      call test_mswin_event('mouse', args)

"      call feedkeys("\<Esc>", 'Lx!')
"    endfor

"    call assert_equal(['LeftMouse', 'LeftRelease', 'LeftMouse', '2-LeftMouse',
"          \ 'LeftMouse', '2-LeftMouse', '3-LeftMouse', 'S-LeftMouse',
"          \ 'A-LeftMouse', 'C-LeftMouse', 'MiddleMouse', 'MiddleRelease',
"          \ 'MiddleMouse', '2-MiddleMouse', 'MiddleMouse', '2-MiddleMouse',
"          \ '3-MiddleMouse', 'S-MiddleMouse', 'A-MiddleMouse', 'C-MiddleMouse',
"          \ 'RightMouse', 'RightRelease', 'RightMouse', '2-RightMouse',
"          \ 'RightMouse', '2-RightMouse', '3-RightMouse', 'S-RightMouse',
"          \ 'A-RightMouse', 'C-RightMouse', 'X1Mouse', 'S-X1Mouse', 'A-X1Mouse',
"          \ 'C-X1Mouse', 'X2Mouse', 'S-X2Mouse', 'A-X2Mouse', 'C-X2Mouse'],
"          \ g:events)

"    for e in mouseEventCodes
"      exe 'nunmap ' .. e
"    endfor

"    " modeless selection
"    set mouse=
"    let save_guioptions = &guioptions
"    set guioptions+=A
"    %d _
"    call setline(1, ['one two three', 'four five sixteen'])
"    call cursor(1, 1)
"    redraw!
"    " Double click should select the word and copy it to clipboard
"    let @* = ''
"    let args = #{button: 0, row: 2, col: 11, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.multiclick = 1
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    let args.multiclick = 0
"    call test_mswin_event('mouse', args)
"    call feedkeys("\<Esc>", 'Lx!')
"    call assert_equal([0, 1, 1, 0], getpos('.'))
"    call assert_equal('sixteen', @*)
"    " Right click should extend the selection from cursor
"    call cursor(1, 6)
"    redraw!
"    let @* = ''
"    let args = #{button: 2, row: 1, col: 11, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    call test_mswin_event('mouse', args)
"    call feedkeys("\<Esc>", 'Lx!')
"    call assert_equal([0, 1, 6, 0], getpos('.'))
"    call assert_equal('wo thr', @*)
"    " Middle click should paste the clipboard contents
"    call cursor(2, 1)
"    redraw!
"    let args = #{button: 1, row: 1, col: 11, multiclick: 0, modifiers: 0}
"    call test_mswin_event('mouse', args)
"    let args.button = 3
"    call test_mswin_event('mouse', args)
"    call feedkeys("\<Esc>", 'Lx!')
"    call assert_equal([0, 2, 7, 0], getpos('.'))
"    call assert_equal('wo thrfour five sixteen', getline(2))
"    set mouse&
"    let &guioptions = save_guioptions

"    " Test invalid parameters for test_mswin_event()
"    let args = #{row: 2, col: 4, multiclick: 0, modifiers: 0}
"    call assert_false(test_mswin_event('mouse', args))
"    let args = #{button: 0, col: 4, multiclick: 0, modifiers: 0}
"    call assert_false(test_mswin_event('mouse', args))
"    let args = #{button: 0, row: 2, multiclick: 0, modifiers: 0}
"    call assert_false(test_mswin_event('mouse', args))
"    let args = #{button: 0, row: 2, col: 4, modifiers: 0}
"    call assert_false(test_mswin_event('mouse', args))
"    let args = #{button: 0, row: 2, col: 4, multiclick: 0}
"    call assert_false(test_mswin_event('mouse', args))

"    " Error cases for test_mswin_event()
"    call assert_fails("call test_mswin_event('a1b2c3', args)", 'E475:')
"    call assert_fails("call test_mswin_event([], args)", 'E1174:')
"    call assert_fails("call test_mswin_event('abc', [])", 'E1206:')
"    call assert_fails("call test_mswin_event(test_null_string(), {})", 'E475:')
"    call assert_false(test_mswin_event('mouse', test_null_dict()))

"    bw!
"    call test_override('no_query_mouse', 0)
"    set mousemodel&
"  endfunc


" vim: shiftwidth=2 sts=2 expandtab
