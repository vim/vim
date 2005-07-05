" Vim filetype plugin file
" Language:    MuPAD source files
" Maintainer:  Dave Silvia <dsilvia@mchsi.com>
" Filenames:   *.mu
" Date:        6/30/2004

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

" Change the :browse e filter to primarily show MuPAD source files.
if has("gui_win32")
  let  b:browsefilter=
		\ "MuPAD source (*.mu)\t*.mu\n" .
		\	"All Files (*.*)\t*.*\n"
endif

" matchit.vim not loaded -- don't do anyting below
if !exists("loaded_matchit")
	" echomsg "matchit.vim not loaded -- finishing"
	finish
endif

" source the AppendMatchGroup function file
runtime ftplugin/AppendMatchGroup.vim

" fill b:match_words for MuPAD
call AppendMatchGroup('domain,end_domain')
call AppendMatchGroup('proc,begin,end_proc')
call AppendMatchGroup('if,then,elif,else,end_if')
call AppendMatchGroup('\%(for\|while\|repeat\|case\),of,do,break,next,until,\%(end_for\|end_while\|end_repeat\|end_case\)')
