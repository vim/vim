" Vim syntax file
" Language:	    dircolors(1) input file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/dircolors/
" Latest Revision:  2004-05-22
" arch-tag:	    995e2983-2a7a-4f1e-b00d-3fdf8e076b40
" Color definition coloring implemented my Mikolaj Machowski <mikmach@wp.pl>

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" todo
syn keyword dircolorsTodo	contained FIXME TODO XXX NOTE

" comments
syn region  dircolorsComment	start="#" end="$" contains=dircolorsTodo

" numbers
syn match   dircolorsNumber	"\<\d\+\>"

" keywords
syn keyword dircolorsKeyword	TERM NORMAL NORM FILE DIR LNK LINK SYMLINK
syn keyword dircolorsKeyword	ORPHAN MISSING FIFO PIPE SOCK BLK BLOCK CHR
syn keyword dircolorsKeyword	CHAR DOOR EXEC LEFT LEFTCODE RIGHT RIGHTCODE
syn keyword dircolorsKeyword	END ENDCODE
if exists("dircolors_is_slackware")
  syn keyword	dircolorsKeyword    COLOR OPTIONS EIGHTBIT
endif

" extensions
syn match   dircolorsExtension	"^\s*\zs[.*]\S\+"

" colors
syn match dircolors01 "\<01\>"
syn match dircolors04 "\<04\>"
syn match dircolors05 "\<05\>"
syn match dircolors07 "\<07\>"
syn match dircolors08 "\<08\>"
syn match dircolors30 "\<30\>"
syn match dircolors31 "\<31\>"
syn match dircolors32 "\<32\>"
syn match dircolors33 "\<33\>"
syn match dircolors34 "\<34\>"
syn match dircolors35 "\<35\>"
syn match dircolors36 "\<36\>"
syn match dircolors37 "\<37\>"
syn match dircolors40 "\<40\>"
syn match dircolors41 "\<41\>"
syn match dircolors42 "\<42\>"
syn match dircolors43 "\<43\>"
syn match dircolors44 "\<44\>"
syn match dircolors45 "\<45\>"
syn match dircolors46 "\<46\>"
syn match dircolors47 "\<47\>"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_dircolors_syn_inits")
  if version < 508
    let did_dircolors_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
    command -nargs=+ HiDef hi <args>
  else
    command -nargs=+ HiLink hi def link <args>
    command -nargs=+ HiDef hi def <args>
  endif

  HiLink dircolorsTodo	Todo
  HiLink dircolorsComment	Comment
  HiLink dircolorsNumber	Number
  HiLink dircolorsKeyword	Keyword
  HiLink dircolorsExtension	Keyword

  HiDef dircolors01		term=bold cterm=bold gui=bold
  HiDef dircolors04		term=underline cterm=underline gui=underline
  "    HiDef dircolors05
  HiDef dircolors07		term=reverse cterm=reverse gui=reverse
  HiLink dircolors08		Ignore
  HiDef dircolors30		ctermfg=Black guifg=Black
  HiDef dircolors31		ctermfg=Red guifg=Red
  HiDef dircolors32		ctermfg=Green guifg=Green
  HiDef dircolors33		ctermfg=Yellow guifg=Yellow
  HiDef dircolors34		ctermfg=Blue guifg=Blue
  HiDef dircolors35		ctermfg=Magenta guifg=Magenta
  HiDef dircolors36		ctermfg=Cyan guifg=Cyan
  HiDef dircolors37		ctermfg=White guifg=White
  HiDef dircolors40		ctermbg=Black ctermfg=White guibg=Black guifg=White
  HiDef dircolors41		ctermbg=DarkRed guibg=DarkRed
  HiDef dircolors42		ctermbg=DarkGreen guibg=DarkGreen
  HiDef dircolors43		ctermbg=DarkYellow guibg=DarkYellow
  HiDef dircolors44		ctermbg=DarkBlue guibg=DarkBlue
  HiDef dircolors45		ctermbg=DarkMagenta guibg=DarkMagenta
  HiDef dircolors46		ctermbg=DarkCyan guibg=DarkCyan
  HiDef dircolors47		ctermbg=White ctermfg=Black guibg=White guifg=Black

  delcommand HiLink
  delcommand HiDef
endif

let b:current_syntax = "dircolors"

" vim: set sts=2 sw=2:
