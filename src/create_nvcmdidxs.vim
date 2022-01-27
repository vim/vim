vim9script

# This script generates the table nv_cmd_idx[] which contains the index in
# nv_cmds[] table (normal.c) for each of the command character supported in
# normal/visual mode.
# This is used to speed up the command lookup in nv_cmds[].
#
# Script should be run using "make nvcmdidxs", every time a new normal/visual
# mode command is added to the nv_cmds[] table in src/normal.c.

def Create_nvcmdidxs_table()
  var nv_cmdtbl: list<dict<number>> = []

  # Generate the table of normal/visual mode command characters and their
  # corresponding index.
  var idx: number = 0
  var ch: number
  while true
    ch = internal_get_nv_cmdchar(idx)
    if ch == -1
      break
    endif
    add(nv_cmdtbl, {idx: idx, cmdchar: ch})
    idx += 1
  endwhile

  # sort the table by the command character
  sort(nv_cmdtbl, (a, b) => a.cmdchar - b.cmdchar)

  # Compute the highest index upto which the command character can be directly
  # used as an index.
  var nv_max_linear: number = 0
  for i in range(nv_cmdtbl->len())
    if i != nv_cmdtbl[i].cmdchar
      nv_max_linear = i - 1
      break
    endif
  endfor

  # Generate a header file with the table
  var output: list<string> =<< trim END
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

  # Add each command character in comment and the corresponding index
  var tbl: list<string> = mapnew(nv_cmdtbl, (k, v) =>
        '  /* ' .. printf('%5d', v.cmdchar) .. ' */ ' ..
        printf('%3d', v.idx) .. ','
    )
  output += tbl

  output += [ '};', '',
              '// The highest index for which',
              '// nv_cmds[idx].cmd_char == nv_cmd_idx[nv_cmds[idx].cmd_char]']
  output += ['static const int nv_max_linear = ' .. nv_max_linear .. ';']

  writefile(output, "nv_cmdidxs.h")
enddef

Create_nvcmdidxs_table()
quit

# vim: shiftwidth=2 sts=2 expandtab
