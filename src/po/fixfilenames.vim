" Invoked with the name "vim.pot" and a list of Vim script names.
" Converts them to a .js file, stripping comments, so that xgettext works.

set shortmess+=A

let s:namenum = 0
for name in argv()[1:]
  let jsname = fnamemodify(name, ":t:r") .. s:namenum .. ".js"
  exe "%s+" .. jsname .. "+" .. substitute(name, '\\', '/', 'g') .. "+"
  let s:namenum +=1
endfor

write
last
quit
