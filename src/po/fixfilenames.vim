" Invoked with the name "vim.pot" and a list of Vim script names.
" Converts them to a .js file, stripping comments, so that xgettext works.

set shortmess+=A

let s:namenum = 0
for s:name in argv()[1:]
  let s:jsname = fnamemodify(s:name, ":t:r") .. s:namenum .. ".js"
  exe "%s+" .. s:jsname .. "+" .. substitute(s:name, '\\', '/', 'g') .. "+ge"
  let s:namenum +=1
endfor

write
last
quit
