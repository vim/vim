" Vim filetype plugin file
" Language:	Go
" Maintainer:	David Barnett (https://github.com/google/vim-ft-go)
" Last Change:	2014 Aug 16
" 2024 Jul 16 by Vim Project (add recommended indent style)

if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

setlocal formatoptions-=t
setlocal formatprg=gofmt

setlocal comments=s1:/*,mb:*,ex:*/,://
setlocal commentstring=//\ %s
setlocal keywordprg=:GoKeywordPrg

command! -buffer -nargs=* GoKeywordPrg call s:GoKeywordPrg()

let b:undo_ftplugin = 'setl fo< com< cms< fp< kp<'
	\ . '\n delcommand -buffer GoKeywordPrg'

if get(g:, 'go_recommended_style', 1)
  setlocal noexpandtab softtabstop=0 shiftwidth=0
  let b:undo_ftplugin ..= ' | setl et< sts< sw<'
endif

if !exists('*' .. expand('<SID>') .. 'GoKeywordPrg')
  func! s:GoKeywordPrg()
    let temp_isk = &l:iskeyword
    setl iskeyword+=.
    try
      let cmd = 'go doc -C ' . shellescape(expand('%:h')) . ' ' . shellescape(expand('<cword>'))
      if has('nvim')
        enew
        call jobstart(cmd, #{term: v:true})
      else
        exe '!' . cmd
      endif
    finally
      let &l:iskeyword = temp_isk
    endtry
  endfunc
endif

" vim: sw=2 sts=2 et
