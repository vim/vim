" Invoked with the name "vim.pot" and a list of Vim script names.
" Converts them to a .js file, stripping comments, so that xgettext works.
" Javascript is used because, like Vim, it accepts both single and double
" quoted strings.

set shortmess+=A

let s:namenum = 0
let s:fls = []
for s:name in argv()[1:]
  exe 'edit ' .. fnameescape(s:name)

  " Strip comments, also after :set commands.
  g/^\s*"/s/.*//
  g/^\s*set .*"/s/.*//

  " Write as .js file, xgettext recognizes them
  let s:fl = fnamemodify(s:name, ":t:r") .. s:namenum .. ".js"
  exe 'w! ' .. s:fl
  call add(s:fls, s:fl)
  let s:namenum += 1
endfor
call writefile(s:fls, "vim_to_js")
quit
