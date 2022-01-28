" This script generates the table nv_cmd_idx[] which contains the index in
" nv_cmds[] table (normal.c) for each of the command character supported in
" normal/visual mode.
" This is used to speed up lookup in nv_cmds[].
"
" Script should be run every time a new normal/visual mode command is added to
" the nv_cmds[] table in src/normal.c.

language collate C
let nv_cmdtbl = []

" Generate the table of normal/visual mode command characters and their
" corresponding index.
let idx = 0
while v:true
  let ch = internal_get_nv_cmdchar(idx)
  if ch == -1
    break
  endif
  " [index, cmd_char]
  call add(nv_cmdtbl, #{idx: idx, cmdchar: ch})
  let idx += 1
endwhile

" sort the command table using the cmdchar (second item)
func s:sortby_cmdchar(i1, i2)
  return a:i1.cmdchar == a:i2.cmdchar ? 0 : a:i1.cmdchar > a:i2.cmdchar ? 1 : -1
endfunc
call sort(nv_cmdtbl, 's:sortby_cmdchar')

" Compute the highest index upto which the command character can be directly
" used as an index.
for idx in range(nv_cmdtbl->len())
  if idx != nv_cmdtbl[idx].cmdchar
    let nv_max_linear = idx - 1
    break
  endif
endfor

" Generate a header file with the table
let output =<< trim END
  /*
   * Automatically generated code by the create_nvcmdidxs.vim script.
   *
   * Table giving the index in nv_cmds[] to lookup based on
   * the command character.
   */

  // nv_cmd_idx[<normal mode command character>] => nv_cmds[] index
  static const unsigned short nv_cmd_idx[] =
  {
END

for item in nv_cmdtbl
  let line = '  /* ' .. printf('%5d', item.cmdchar) .. ' */ '
  let line ..= printf('%3d', item.idx) .. ','
  let output += [line]
endfor

let output += [ '};' ]
let output += [ '' ]

let output += ['// The highest index for which']
let output += ['// nv_cmds[idx].cmd_char == nv_cmd_idx[nv_cmds[idx].cmd_char]']
let output += ['static const int nv_max_linear = ' .. nv_max_linear .. ';']

call writefile(output, "nv_cmdidxs.h")
quit
