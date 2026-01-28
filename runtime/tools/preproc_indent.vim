vim9script
# Script to fix preprocessor indentation in Vim's C source code.
#
# Usage: Vim -S <this-file>
#
# Specifications:
# - If there is no indentation on the line containing the preprocessor
#   directive (`#`) following the first `#if~`, the indentation amount is
#   `nesting level - 1` spaces. Otherwise, the indentation amount is `nesting
#   level` spaces.
# - However, if a preprocessor directive line is detected after the
#   corresponding `#endif` of the above `#if~`, the indentation amount is
#   fixed at `nesting level` and the line is reprocessed from the first line.
# - If the preprocessor directive line ends with a line continuation (`\`) and
#   the next line is blank, the line continuation (`\`) and the next line are
#   deleted.
#
# Author: Hirohito Higashi (@h-east)
# Last Update: 2026 Jan 12

def Get_C_source_files(): list<string>
  var list_of_c_files: list<string> = []
  if empty(list_of_c_files)
    var fpath = '../../src'
    var list = glob(fpath .. '/*.[ch]', 0, 1) + [fpath .. '/xxd/xxd.c']
    # Some files are auto-generated, so skip those
    list_of_c_files = filter(list, (i, v) => v !~ 'dlldata.c\|if_ole.h\|iid_ole.c')
  endif
  return list_of_c_files
enddef

def FixPreprocessorIndent(fname: string)
  execute 'edit! ' .. fname

  var nest: number = 0
  var indent_offset: number = 0  # -1 if whole-file guard detected
  var first_if_seen: bool = false
  var offset_determined: bool = false
  var whole_file_guard_ended = false

  # First pass: remove trailing backslash + empty next line
  var lnum = 1
  while lnum <= line('$')
    var line: string = getline(lnum)
    if line =~# '^\s*#.*\\$'
      var next_line: string = getline(lnum + 1)
      if next_line =~# '^\s*$'
        # Remove backslash from current line and delete next line
        setline(lnum, substitute(line, '\s*\\$', '', ''))
        deletebufline('%', lnum + 1)
        continue  # Don't increment, check same line again
      endif
    endif
    lnum += 1
  endwhile

  # Second pass: fix preprocessor indent
  while true
    var is_reprocess: bool = false
    for l in range(1, line('$'))
      var line: string = getline(l)

      # Skip if not a preprocessor directive
      if line !~# '^\s*#'
        continue
      endif

      # Extract directive and current indent
      var match_li: list<string> = matchlist(line, '^\(\s*\)#\(\s*\)\(\w\+\)')
      if empty(match_li)
        continue
      endif
      var cur_spaces: string = !empty(match_li[1]) ? match_li[1] : match_li[2]
      var directive: string = match_li[3]

      # If indent_offset != 0 but we encounter indented #, it's not whole-file
      # guard. Reprocess from line 1 with indent_offset=0
      if whole_file_guard_ended && offset_determined && indent_offset != 0
        indent_offset = 0
        nest = 0
        is_reprocess = true
        break
      endif

      # After first #if, determine offset from first nested directive
      # Only check if # is at column 1 (no leading spaces)
      if first_if_seen && !offset_determined
        offset_determined = true
        if empty(cur_spaces)
          # No indent after first `#if` --> whole-file guard style
          indent_offset = -1
        endif
      endif

      # Determine expected indent based on directive type
      var expected_indent: number

      if directive ==# 'if' || directive ==# 'ifdef' || directive ==# 'ifndef'
        if !first_if_seen
          first_if_seen = true
        endif
        expected_indent = nest + indent_offset
        nest += 1
      elseif directive ==# 'elif' || directive ==# 'else'
        expected_indent = nest - 1 + indent_offset
      elseif directive ==# 'endif'
        nest -= 1
        if nest <= 0
          # Reset for next top-level block (but keep offset_determined)
          nest = 0
          whole_file_guard_ended = true
        endif
        expected_indent = nest + indent_offset
      else
        # Other directives (#define, #include, #error, #pragma, etc.)
        expected_indent = nest + indent_offset
      endif

      if expected_indent < 0
        expected_indent = 0
      endif

      # Build expected line
      var rest = substitute(line, '^\s*#\s*', '', '')
      var expected_line: string
      expected_line = '#' .. repeat(' ', expected_indent) .. rest

      # Update line if different
      if line !=# expected_line
        setline(l, expected_line)
      endif
    endfor

    if !is_reprocess
      break
    endif
  endwhile

  update
enddef

# Main
for fname in Get_C_source_files()
  FixPreprocessorIndent(fname)
endfor

qall!
# vim: et ts=2 sw=0
