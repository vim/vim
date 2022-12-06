" Helper functions for generating mouse events

" xterm2 and sgr always work, urxvt is optional.
let g:Ttymouse_values = ['xterm2', 'sgr']
if has('mouse_urxvt')
  call add(g:Ttymouse_values, 'urxvt')
endif

" dec doesn't support all the functionality
if has('mouse_dec')
  let g:Ttymouse_dec = ['dec']
else
  let g:Ttymouse_dec = []
endif

" netterm only supports left click
if has('mouse_netterm')
  let g:Ttymouse_netterm = ['netterm']
else
  let g:Ttymouse_netterm = []
endif

" Helper function to emit a terminal escape code.
func TerminalEscapeCode(code, row, col, m)
  if &ttymouse ==# 'xterm2'
    " need to use byte encoding here.
    let str = list2str([a:code + 0x20, a:col + 0x20, a:row + 0x20])
    if has('iconv')
      let bytes = str->iconv('utf-8', 'latin1')
    else
      " Hopefully the numbers are not too big.
      let bytes = str
    endif
    return "\<Esc>[M" .. bytes
  elseif &ttymouse ==# 'sgr'
    return printf("\<Esc>[<%d;%d;%d%s", a:code, a:col, a:row, a:m)
  elseif &ttymouse ==# 'urxvt'
    return printf("\<Esc>[%d;%d;%dM", a:code + 0x20, a:col, a:row)
  elseif &term ==# 'win32'
    return printf("\<Esc>[<%d;%d;%d%s", a:code, a:col, a:row, a:m)
  endif
endfunc

func DecEscapeCode(code, down, row, col)
    return printf("\<Esc>[%d;%d;%d;%d&w", a:code, a:down, a:row, a:col)
endfunc

func NettermEscapeCode(row, col)
    return printf("\<Esc>}%d,%d\r", a:row, a:col)
endfunc

func MouseLeftClickCode(row, col)
  if &ttymouse ==# 'dec'
    return DecEscapeCode(2, 4, a:row, a:col)
  elseif &ttymouse ==# 'netterm'
    return NettermEscapeCode(a:row, a:col)
  else
    return TerminalEscapeCode(0, a:row, a:col, 'M')
  endif
endfunc

func MouseLeftClick(row, col)
  call feedkeys(MouseLeftClickCode(a:row, a:col), 'Lx!')
endfunc

func MouseMiddleClickCode(row, col)
  if &ttymouse ==# 'dec'
    return DecEscapeCode(4, 2, a:row, a:col)
  else
    return TerminalEscapeCode(1, a:row, a:col, 'M')
  endif
endfunc

func MouseMiddleClick(row, col)
  if &term ==# 'win32'
    call feedkeys("\<MiddleMouse>", 'L')
  else
    call feedkeys(MouseMiddleClickCode(a:row, a:col), 'Lx!')
  endif
endfunc

func MouseRightClickCode(row, col)
  if &ttymouse ==# 'dec'
    return DecEscapeCode(6, 1, a:row, a:col)
  else
    return TerminalEscapeCode(2, a:row, a:col, 'M')
  endif
endfunc

func MouseRightClick(row, col)
  call feedkeys(MouseRightClickCode(a:row, a:col), 'Lx!')
endfunc

func MouseCtrlLeftClickCode(row, col)
  let ctrl = 0x10
  return TerminalEscapeCode(0 + ctrl, a:row, a:col, 'M')
endfunc

func MouseCtrlLeftClick(row, col)
  call feedkeys(MouseCtrlLeftClickCode(a:row, a:col), 'Lx!')
endfunc

func MouseCtrlRightClickCode(row, col)
  let ctrl = 0x10
  return TerminalEscapeCode(2 + ctrl, a:row, a:col, 'M')
endfunc

func MouseCtrlRightClick(row, col)
  call feedkeys(MouseCtrlRightClickCode(a:row, a:col), 'Lx!')
endfunc

func MouseAltLeftClickCode(row, col)
  let alt = 0x8
  return TerminalEscapeCode(0 + alt, a:row, a:col, 'M')
endfunc

func MouseAltLeftClick(row, col)
  call feedkeys(MouseAltLeftClickCode(a:row, a:col), 'Lx!')
endfunc

func MouseAltRightClickCode(row, col)
  let alt = 0x8
  return TerminalEscapeCode(2 + alt, a:row, a:col, 'M')
endfunc

func MouseAltRightClick(row, col)
  call feedkeys(MouseAltRightClickCode(a:row, a:col), 'Lx!')
endfunc

func MouseLeftReleaseCode(row, col)
  if &ttymouse ==# 'dec'
    return DecEscapeCode(3, 0, a:row, a:col)
  elseif &ttymouse ==# 'netterm'
    return ''
  else
    return TerminalEscapeCode(3, a:row, a:col, 'm')
  endif
endfunc

func MouseLeftRelease(row, col)
  call feedkeys(MouseLeftReleaseCode(a:row, a:col), 'Lx!')
endfunc

func MouseMiddleReleaseCode(row, col)
  if &ttymouse ==# 'dec'
    return DecEscapeCode(5, 0, a:row, a:col)
  else
    return TerminalEscapeCode(3, a:row, a:col, 'm')
  endif
endfunc

func MouseMiddleRelease(row, col)
  call feedkeys(MouseMiddleReleaseCode(a:row, a:col), 'Lx!')
endfunc

func MouseRightReleaseCode(row, col)
  if &ttymouse ==# 'dec'
    return DecEscapeCode(7, 0, a:row, a:col)
  else
    return TerminalEscapeCode(3, a:row, a:col, 'm')
  endif
endfunc

func MouseRightRelease(row, col)
  call feedkeys(MouseRightReleaseCode(a:row, a:col), 'Lx!')
endfunc

func MouseLeftDragCode(row, col)
  if &ttymouse ==# 'dec'
    return DecEscapeCode(1, 4, a:row, a:col)
  else
    return TerminalEscapeCode(0x20, a:row, a:col, 'M')
  endif
endfunc

func MouseLeftDrag(row, col)
  call feedkeys(MouseLeftDragCode(a:row, a:col), 'Lx!')
endfunc

func MouseWheelUpCode(row, col)
  return TerminalEscapeCode(0x40, a:row, a:col, 'M')
endfunc

func MouseWheelUp(row, col)
  call feedkeys(MouseWheelUpCode(a:row, a:col), 'Lx!')
endfunc

func MouseWheelDownCode(row, col)
  return TerminalEscapeCode(0x41, a:row, a:col, 'M')
endfunc

func MouseWheelDown(row, col)
  call feedkeys(MouseWheelDownCode(a:row, a:col), 'Lx!')
endfunc

func MouseWheelLeftCode(row, col)
  return TerminalEscapeCode(0x42, a:row, a:col, 'M')
endfunc

func MouseWheelLeft(row, col)
  call feedkeys(MouseWheelLeftCode(a:row, a:col), 'Lx!')
endfunc

func MouseWheelRightCode(row, col)
  return TerminalEscapeCode(0x43, a:row, a:col, 'M')
endfunc

func MouseWheelRight(row, col)
  call feedkeys(MouseWheelRightCode(a:row, a:col), 'Lx!')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
