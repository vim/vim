" Script to generate a file that shows al 256 xterm colors

new
call setline(1, 'ANSI')

" ANSI colors
let s = ''
for nr in range(0, 7)
  let s .= "\033[4" . nr . "m    "
endfor
for nr in range(8, 15)
  let s .= "\033[10" . (nr - 8) . "m    "
endfor
let s .= "\033[107m|"
call setline(2, s)

" 6 x 6 x 6 color cube
call setline(3, 'color cube')
for high in range(0, 5)
  let s = ''
  for low in range(0, 35)
    let nr = low + high * 36
    let s .= "\033[48;5;" . (nr + 16) . "m  "
  endfor
  let s .= "\033[107m|"
  call setline(high + 4, s)
endfor

" 24 shades of grey
call setline(10, 'grey ramp')
let s = ''
for nr in range(0, 23)
    let s .= "\033[48;5;" . (nr + 232) . "m   "
endfor
let s .= "\033[107m|"
call setline(11, s)

set binary
write! <sfile>:h/xterm_ramp.txt
quit
