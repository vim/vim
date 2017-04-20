" This script generates the tables cmdidxs1[] and cmdidxs2[][] which,
" given a Ex command, determine the first value to probe to find
" a matching command in cmdnames[] based on the first character
" and the first 2 characters of the command.
" This is used to speed up lookup in cmdnames[].
"
" Script should be run every time new Ex commands are added in Vim,
" from the src/vim directory, since it reads commands from "ex_cmds.h".

let cmds = []
let skipped_cmds = 0

for line in readfile('ex_cmds.h')
  if line =~ '^EX(CMD_'
    let m = matchlist(line, '^EX(CMD_\S*,\s*"\([a-z][^"]*\)"')
    if len(m) >= 2
      let cmds += [ m[1] ]
    else
      let skipped_cmds += 1
    endif
  endif
endfor

let cmdidxs1 = {}
let cmdidxs2 = {}

for i in range(len(cmds) - 1, 0, -1)
  let cmd = cmds[i]
  let c1 = cmd[0] " First character of command
  let c2 = cmd[1] " Second character of command (if any)

  let cmdidxs1{c1} = i
  if c2 >= 'a' && c2 <= 'z'
    let cmdidxs2{c1}{c2} = i
  endif
endfor

let output =  [ '/* Automatically generated code by create_cmdidxs.vim' ]
let output += [ ' *' ]
let output += [ ' * Table giving the index of the first command in cmdnames[] to lookup' ]
let output += [ ' * based on the first letter of a command.' ]
let output += [ ' */' ]
let output += [ 'static const unsigned short cmdidxs1[26] =' ]
let output += [ '{' ]

let a_to_z = map(range(char2nr('a'), char2nr('z')), 'nr2char(v:val)')
for c1 in a_to_z
  let line = '  /* ' . c1 . ' */ ' . cmdidxs1{c1} . ((c1 == 'z') ? '' : ',')
  let output += [ line ]
endfor
let output += [ '};' ]
let output += [ '' ]
let output += [ '/*' ]
let output += [ ' * Table giving the index of the first command in cmdnames[] to lookup' ]
let output += [ ' * based on the first 2 letters of a command.' ]
let output += [ ' * Values in cmdidxs2[c1][c2] are relative to cmdidxs1[c1] so that they' ]
let output += [ ' * fit in a byte.' ]
let output += [ ' */' ]
let output += [ 'static const unsigned char cmdidxs2[26][26] =' ]
let output += [ '{ /*         a   b   c   d   e   f   g   h   i   j   k   l   m   n   o   p   q   r   s   t   u   v   w   x   y   z */' ]

for c1 in a_to_z
  let line = '  /* ' . c1 . ' */ {'
  for c2 in a_to_z
    if exists('cmdidxs2{c1}{c2}')
      let line .= printf('%3d', cmdidxs2{c1}{c2} - cmdidxs1{c1})
    else
      let line .= '  0'
    endif
    let line .= (c2 == 'z') ? '' : ','
  endfor
  let line .= ' }' . ((c1 == 'z') ? '' : ',')
  let output += [ line ]
endfor

let output += [ '};' ]
let output += [ '' ]
let output += [ 'static const int command_count = ' . (len(cmds) + skipped_cmds) . ';' ]

call writefile(output, "ex_cmdidxs.h")
quit
