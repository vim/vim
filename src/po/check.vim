" Vim script for checking .po files.
"
" Go through the file and verify that all %...s items in "msgid" are identical
" to the ones in "msgstr".

if 1	" Only execute this if the eval feature is available.

" Function to get a split line at the cursor.
" Used for both msgid and msgstr lines.
" Removes all text except % items and returns the result.
func! GetMline()
  let idline = substitute(getline('.'), '"\(.*\)"$', '\1', '')
  while line('.') < line('$')
    +
    let line = getline('.')
    if line[0] != '"'
      break
    endif
    let idline .= substitute(line, '"\(.*\)"$', '\1', '')
  endwhile

  " remove everything but % items.
  return substitute(idline, '[^%]*\(%[-+ #''.0-9*]*l\=[dsuxXpoc%]\)\=', '\1', 'g')
endfunc

" Start at the first "msgid" line.
1
/^msgid
let startline = line('.')
let error = 0

while 1
  if getline(line('.') - 1) !~ "no-c-format"
    let fromline = GetMline()
    if getline('.') !~ '^msgstr'
      echo 'Missing "msgstr" in line ' . line('.')
      let error = 1
    endif
    let toline = GetMline()
    if fromline != toline
      echo 'Mismatching % in line ' . (line('.') - 1)
      let error = 1
    endif
  endif

  " Find next msgid.
  " Wrap around at the end of the file, quit when back at the first one.
  /^msgid
  if line('.') == startline
    break
  endif
endwhile

if error == 0
  echo "OK"
endif

endif
