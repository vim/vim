" Vim support file to switch on loading indent files for file types
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2005 Mar 28

if exists("did_indent_on")
  finish
endif
let did_indent_on = 1

augroup filetypeindent
  au FileType * call s:LoadIndent()
  func! s:LoadIndent()
    if exists("b:undo_indent")
      exe b:undo_indent
      unlet! b:undo_indent b:did_indent
    endif
    if expand("<amatch>") != ""
      if exists("b:did_indent")
	unlet b:did_indent
      endif
      runtime! indent/<amatch>.vim
    endif
  endfunc
augroup END
