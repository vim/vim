" Invoked with the name "vim.pot" and a list of Vim script names.
" Converts them to a .js file, stripping comments, so that xgettext works.

set shortmess+=A

for name in argv()[1:]
  let jsname = fnamemodify(name, ":r:gs?\\~?_?:gs?\\.?_?:gs?/?__?:gs?\\?__?") .. ".js"
  exe "%s+" .. jsname .. "+" .. substitute(name, '\\', '/', 'g') .. "+"
endfor

write
last
quit
