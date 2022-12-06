" Test MS-Windows console event handling.

source check.vim
CheckMSWindows
CheckNotGui  
".. The key events should also work in gui, but not yet.

source mouse.vim

" Test for sending low level key presses
func SendKeys(keylist)
  for k in a:keylist
    call test_mswin_event("keyboard", #{event: "keydown", keycode: k})
  endfor
  for k in reverse(a:keylist)
    call test_mswin_event("keyboard", #{event: "keyup", keycode: k})
  endfor
endfunc

" Test MS-Windows console key events
func Test_windows_console_key_event()
  CheckMSWindows
  CheckNotGui
  new

  let VK = {
	\ 'SPACE'      : 0x20,
        \ 'SHIFT'      : 0x10,
        \ 'LSHIFT'     : 0xA0,
        \ 'RSHIFT'     : 0xA1,
        \ 'CONTROL'    : 0x11,
        \ 'LCONTROL'   : 0xA2,
        \ 'RCONTROL'   : 0xA3,
        \ 'MENU'       : 0x12,
        \ 'LMENU'      : 0xA4,
        \ 'RMENU'      : 0xA5,
        \ 'OEM_1'      : 0xBA,
        \ 'OEM_2'      : 0xBF,
        \ 'OEM_3'      : 0xC0,
        \ 'OEM_4'      : 0xDB,
        \ 'OEM_5'      : 0xDC,
        \ 'OEM_6'      : 0xDD,
        \ 'OEM_7'      : 0xDE,
        \ 'OEM_PLUS'   : 0xBB,
        \ 'OEM_COMMA'  : 0xBC,
        \ 'OEM_MINUS'  : 0xBD,
        \ 'OEM_PERIOD' : 0xBE,
        \ 'PRIOR'      : 0x21,
        \ 'NEXT'       : 0x22,
        \ 'END'        : 0x23,
        \ 'HOME'       : 0x24,
        \ 'LEFT'       : 0x25,
        \ 'UP'         : 0x26,
        \ 'RIGHT'      : 0x27,
        \ 'DOWN'       : 0x28,
        \ 'KEY0'       : 0x30,
        \ 'KEY1'       : 0x31,
        \ 'KEY2'       : 0x32,
        \ 'KEY3'       : 0x33,
        \ 'KEY4'       : 0x34,
        \ 'KEY5'       : 0x35,
        \ 'KEY6'       : 0x36,
        \ 'KEY7'       : 0x37,
        \ 'KEY8'       : 0x38,
        \ 'KEY9'       : 0x39,
	\ 'NUMPAD0'    : 0x60,
        \ 'NUMPAD1'    : 0x61,
        \ 'NUMPAD2'    : 0x62,
        \ 'NUMPAD3'    : 0x63,
        \ 'NUMPAD4'    : 0x64,
        \ 'NUMPAD5'    : 0x65,
        \ 'NUMPAD6'    : 0x66,
        \ 'NUMPAD7'    : 0x67,
        \ 'NUMPAD8'    : 0x68,
        \ 'NUMPAD9'    : 0x69,
        \ 'MULTIPLY'   : 0x6A,
        \ 'ADD'        : 0x6B,
        \ 'SUBTRACT'   : 0x6D,
        \ 'F1'         : 0x70,
        \ 'F2'         : 0x71,
        \ 'F3'         : 0x72,
        \ 'F4'         : 0x73,
        \ 'F5'         : 0x74,
        \ 'F6'         : 0x75,
        \ 'F7'         : 0x76,
        \ 'F8'         : 0x77,
        \ 'F9'         : 0x78,
        \ 'F10'        : 0x79,
        \ 'F11'        : 0x7A,
        \ 'F12'        : 0x7B,
	\ }

  let vim_MOD_MASK_SHIFT = 0x02
  let vim_MOD_MASK_CTRL  = 0x04
  let vim_MOD_MASK_ALT   = 0x08
  
  let modifiers = [
    \ ["",       0,   []],
    \ ["S-",     2,   [VK.SHIFT]],
    \ ["C-",     4,   [VK.CONTROL]],
    \ ["C-S-",   6,   [VK.CONTROL, VK.SHIFT]],
    \ ["A-",     8,   [VK.MENU]],
    \ ["A-S-",   10,  [VK.MENU, VK.SHIFT]],
    \ ["A-C-",   12,  [VK.MENU, VK.CONTROL]],
    \ ["A-C-S-", 14,  [VK.MENU, VK.CONTROL, VK.SHIFT]],
    \]

  " Some punctuation characters
  " Assuming Standard US PC Keyboard layout
  let test_oem_keys = [
	\ [[VK.SPACE], ' '],
        \ [[VK.OEM_1], ';'],
        \ [[VK.OEM_2], '/'],
        \ [[VK.OEM_3], '`'],
        \ [[VK.OEM_4], '['],
        \ [[VK.OEM_5], '\'],
        \ [[VK.OEM_6], ']'],
        \ [[VK.OEM_7], ''''],
        \ [[VK.OEM_PLUS], '='],
        \ [[VK.OEM_COMMA], ','],
        \ [[VK.OEM_MINUS], '-'],
        \ [[VK.OEM_PERIOD], '.'],
	\ [[VK.SHIFT, VK.OEM_1], ':'],
        \ [[VK.SHIFT, VK.OEM_2], '?'],
        \ [[VK.SHIFT, VK.OEM_3], '~'],
        \ [[VK.SHIFT, VK.OEM_4], '{'],
        \ [[VK.SHIFT, VK.OEM_5], '|'],
        \ [[VK.SHIFT, VK.OEM_6], '}'],
        \ [[VK.SHIFT, VK.OEM_7], '"'],
	\ [[VK.SHIFT, VK.OEM_PLUS], '+'],
        \ [[VK.SHIFT, VK.OEM_COMMA], '<'],
        \ [[VK.SHIFT, VK.OEM_MINUS], '_'],
        \ [[VK.SHIFT, VK.OEM_PERIOD], '>'],
        \ ]

  for [kcodes, kstr] in test_oem_keys
    call SendKeys(kcodes)
    let ch = getcharstr(0)
    if ch == ''
      throw 'Skipped: The MS-Windows console input buffer was empty.'
    endif
    call assert_equal($"{kstr}", $"{ch}")
    let mod_mask = getcharmod()
    if kcodes[0] == VK.SHIFT
	call assert_equal(vim_MOD_MASK_SHIFT, mod_mask, $"key = {kstr}")
    else
	call assert_equal(0, mod_mask, $"key = {kstr}")
    endif
  endfor


" Test keyboard codes for digits
" (0x30 - 0x39) : VK_0 - VK_9 are the same as ASCII '0' - '9'
  for kc in range(48, 57)
    call SendKeys([kc])
    let ch = getcharstr(0)
    if ch == ''
      throw 'Skipped: The MS-Windows console input buffer was empty.'
    endif
    call assert_equal(nr2char(kc), ch)
  endfor

" Test for lowercase 'a' to 'z', VK codes 65(0x41) - 90(0x5A)
" VK_A - VK_Z virtual key codes coincide with uppercase ASCII codes 'A'-'Z'.
" eg VK_A is 65 and the ASCII character code for uppercase 'A' is also 65.
" Caution: these are interpreted as lowercase when Shift is NOT pressed. 
" Sending VK_A (65) 'A' Key code without shift modifier, will produce ASCII
" char 'a' (91) as the output.
" The ASCII codes for the lowercase letters are 32 higher than their uppercase
" counterparts.
  for kc in range(65, 90)
    call SendKeys([kc])
    let ch = getcharstr(0)
    if ch == ''
      throw 'Skipped: The MS-Windows console input buffer was empty.'
    endif
    call assert_equal(nr2char(kc + 32), ch)
  endfor

"  Test for Uppercase 'A' - 'Z' keys
"  With VK_SHIFT, expect the keycode = character code.
  for kc in range(65, 90)
    call SendKeys([VK.SHIFT, kc])
    let ch = getcharstr(0)
    if ch == ''
      throw 'Skipped: The MS-Windows console input buffer was empty.'
    endif
    call assert_equal(nr2char(kc), ch)
  endfor

  " Test for <Ctrl-A> to <Ctrl-Z> keys
 "  Same as for lowercase, except with Ctrl Key
 "  Expect the unicode characters 0x01 to 0x1A
  for kc in range(65, 90)
    call SendKeys([VK.CONTROL, kc])
    let ch = getcharstr(0)
    if ch == ''
      throw 'Skipped: The MS-Windows console input buffer was empty.'
    endif
    call assert_equal(nr2char(kc - 64), ch)
  endfor

  " Test for Function Keys 'F1' to 'F12'
  " VK codes 112(0x70) - 123(0x7B)
  " With ALL permutatios of modifiers; Shift, Ctrl & Alt
  for n in range(1, 12)
    for [mod_str, vim_mod_mask, mod_keycodes] in modifiers
      let kstr = $"{mod_str}F{n}"
      let keycode = eval('"\<' .. kstr .. '>"')
      call SendKeys(mod_keycodes + [111+n])
      let ch = getcharstr(0)
      if ch == ''
	throw 'Skipped: The MS-Windows console input buffer was empty.'
      endif
      let mod_mask = getcharmod()
      call assert_equal(keycode, $"{ch}", $"key = {kstr}")
      " workaround for termcap changing the character instead of sending Shift
      if index(mod_keycodes, 0x10) >= 0
	let vim_mod_mask = vim_mod_mask - 2
      endif
      call assert_equal(vim_mod_mask, mod_mask, $"key = {kstr}")
    endfor
  endfor


  " Test for the various Ctrl and Shift key combinations.
  " Refer to the following page for the virtual key codes:
  " https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
  let keytests = [
    \ [[VK.SHIFT,    VK.PRIOR], "S-Pageup", 2],
    \ [[VK.LSHIFT,   VK.PRIOR], "S-Pageup", 2],
    \ [[VK.RSHIFT,   VK.PRIOR], "S-Pageup", 2],
    \ [[VK.CONTROL,  VK.PRIOR], "C-Pageup", 4],
    \ [[VK.LCONTROL, VK.PRIOR], "C-Pageup", 4],
    \ [[VK.RCONTROL, VK.PRIOR], "C-Pageup", 4],
    \ [[VK.CONTROL,  VK.SHIFT, VK.PRIOR], "C-S-Pageup", 6],
    \ [[VK.SHIFT,    VK.NEXT], "S-PageDown", 2],
    \ [[VK.LSHIFT,   VK.NEXT], "S-PageDown", 2],
    \ [[VK.RSHIFT,   VK.NEXT], "S-PageDown", 2],
    \ [[VK.CONTROL,  VK.NEXT], "C-PageDown", 4],
    \ [[VK.LCONTROL, VK.NEXT], "C-PageDown", 4],
    \ [[VK.RCONTROL, VK.NEXT], "C-PageDown", 4],
    \ [[VK.CONTROL,  VK.SHIFT, VK.NEXT], "C-S-PageDown", 6],
    \ [[VK.SHIFT,    VK.END], "S-End", 0],
    \ [[VK.CONTROL,  VK.END], "C-End", 0],
    \ [[VK.CONTROL,  VK.SHIFT, VK.END], "C-S-End", 4],
    \ [[VK.SHIFT,    VK.HOME], "S-Home", 0],
    \ [[VK.CONTROL,  VK.HOME], "C-Home", 0],
    \ [[VK.CONTROL,  VK.SHIFT, VK.HOME], "C-S-Home", 4],
    \ [[VK.SHIFT,    VK.LEFT], "S-Left", 0],
    \ [[VK.CONTROL,  VK.LEFT], "C-Left", 0],
    \ [[VK.CONTROL,  VK.SHIFT, VK.LEFT], "C-S-Left", 4],
    \ [[VK.SHIFT,    VK.UP], "S-Up", 0],
    \ [[VK.CONTROL,  VK.UP], "C-Up", 4],
    \ [[VK.CONTROL,  VK.SHIFT, VK.UP], "C-S-Up", 4],
    \ [[VK.SHIFT,    VK.RIGHT], "S-Right", 0],
    \ [[VK.CONTROL,  VK.RIGHT], "C-Right", 0],
    \ [[VK.CONTROL,  VK.SHIFT, VK.RIGHT], "C-S-Right", 4],
    \ [[VK.SHIFT,    VK.DOWN], "S-Down", 0],
    \ [[VK.CONTROL,  VK.DOWN], "C-Down", 4],
    \ [[VK.CONTROL,  VK.SHIFT, VK.DOWN], "C-S-Down", 4],
    \ [[VK.CONTROL,  VK.KEY0], "C-0", 4],
    \ [[VK.CONTROL,  VK.KEY1], "C-1", 4],
    \ [[VK.CONTROL,  VK.KEY2], "C-2", 4],
    \ [[VK.CONTROL,  VK.KEY3], "C-3", 4],
    \ [[VK.CONTROL,  VK.KEY4], "C-4", 4],
    \ [[VK.CONTROL,  VK.KEY5], "C-5", 4],
    \ [[VK.CONTROL,  VK.KEY6], "C-^", 0],
    \ [[VK.CONTROL,  VK.KEY7], "C-7", 4],
    \ [[VK.CONTROL,  VK.KEY8], "C-8", 4],
    \ [[VK.CONTROL,  VK.KEY9], "C-9", 4],
    \ [[VK.CONTROL,  VK.NUMPAD0], "C-0", 4],
    \ [[VK.CONTROL,  VK.NUMPAD1], "C-1", 4],
    \ [[VK.CONTROL,  VK.NUMPAD2], "C-2", 4],
    \ [[VK.CONTROL,  VK.NUMPAD3], "C-3", 4],
    \ [[VK.CONTROL,  VK.NUMPAD4], "C-4", 4],
    \ [[VK.CONTROL,  VK.NUMPAD5], "C-5", 4],
    \ [[VK.CONTROL,  VK.NUMPAD6], "C-6", 4],
    \ [[VK.CONTROL,  VK.NUMPAD7], "C-7", 4],
    \ [[VK.CONTROL,  VK.NUMPAD8], "C-8", 4],
    \ [[VK.CONTROL,  VK.NUMPAD9], "C-9", 4],
    \ [[VK.CONTROL,  VK.MULTIPLY], "C-*", 4],
    \ [[VK.CONTROL,  VK.ADD], "C-+", 4],
    \ [[VK.CONTROL,  VK.SUBTRACT], "C--", 4]
    \ ]

  for [kcodes, kstr, kmod] in keytests
    call SendKeys(kcodes)
    sleep 10ms
    let ch = getcharstr(0)
    if ch == ''
      throw 'Skipped: The MS-Windows console input buffer was empty.'
    endif
    let mod = getcharmod()
    "if ch != ''
      let keycode = eval('"\<' .. kstr .. '>"')
      call assert_equal(keycode, ch, $"key = {kstr}")
    call assert_equal(kmod, mod, $"key = {kstr}")
    "endif
  endfor

  bw!
endfunc

func Test_windows_console_mouse_event()
  CheckMSWindows
  CheckNotGui
  let msg = ''
  go
  " call assert_equal([0, 1, 1, 0], getpos('.'), msg)
  let row = 2
  let col = 6
  call MouseLeftClick(row, col)
  call MouseLeftRelease(row, col)
  " call assert_equal([0, 2, 6, 0], getpos('.'), msg)

"  func MouseLeftClick(row, col)
"  func MouseMiddleClick(row, col)
"  func MouseRightClick(row, col)
"  func MouseCtrlLeftClick(row, col)
"  func MouseCtrlRightClick(row, col)
"  func MouseAltLeftClick(row, col)
"  func MouseAltRightClick(row, col)
"  func MouseLeftRelease(row, col)
"  func MouseMiddleRelease(row, col)
"  func MouseRightRelease(row, col)
"  func MouseLeftDrag(row, col)
"  func MouseWheelUp(row, col)
"  func MouseWheelDown(row, col)
"  func MouseWheelLeft(row, col)
"  func MouseWheelRight(row, col)
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
"          \ ]
"    let mouseEventCodes = map(copy(mouseEventNames), "'<' .. v:val .. '>'")
"    let g:events = []
"    for e in mouseEventCodes
"      exe 'nnoremap ' .. e .. ' <Cmd>call add(g:events, "' ..
"            \ substitute(e, '[<>]', '', 'g') .. '")<CR>'
"    endfor

"    " Test various mouse buttons (0 - Left, 1 - Middle, 2 - Right)
"    for button in [0, 1, 2]
"      " Single click
"      let args = #{button: button, row: 2, col: 5, multiclick: 0, modifiers: 0}
"      call test_mswin_event('mouse', args)
"      let args.button = 3
"      call test_mswin_event('mouse', args)

"      " Double Click
"      let args.button = button
"      call test_mswin_event('mouse', args)
"      let args.multiclick = 1
"      call test_mswin_event('mouse', args)
"      let args.button = 3
"      let args.multiclick = 0
"      call test_mswin_event('mouse', args)

"      " Triple Click
"      let args.button = button
"      call test_mswin_event('mouse', args)
"      let args.multiclick = 1
"      call test_mswin_event('mouse', args)
"      call test_mswin_event('mouse', args)
"      let args.button = 3
"      let args.multiclick = 0
"      call test_mswin_event('mouse', args)

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

"    call assert_equal(['LeftMouse', '2-LeftMouse', '3-LeftMouse', 
"          \ '2-LeftMouse', 'C-LeftMouse', '3-LeftMouse'],
"          \ g:events)

"    for e in mouseEventCodes
"      exe 'nunmap ' .. e
"    endfor

"    " modeless selection
"    set mouse=
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
