" Vim syntax file
" Language:	    BDF Font definition
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/bdf/
" Latest Revision:  2004-05-06
" arch-tag:	    b696b6ba-af24-41ba-b4eb-d248495eca68

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" numbers
syn match   bdfNumber	    display "\<\(\x\+\|\d\+\.\d\+\)\>"

" comments
syn region  bdfComment	    start="^COMMENT\>" end="$" contains=bdfTodo

" todo
syn keyword bdfTodo	    contained TODO FIXME XXX NOTE

" strings
syn region  bdfString	    start=+"+ skip=+""+ end=+"+

" properties
syn keyword bdfProperties   contained FONT SIZE FONTBOUNDINGBOX CHARS

" X11 properties
syn keyword bdfXProperties  contained FONT_ASCENT FONT_DESCENT DEFAULT_CHAR
syn keyword bdfXProperties  contained FONTNAME_REGISTRY FOUNDRY FAMILY_NAME
syn keyword bdfXProperties  contained WEIGHT_NAME SLANT SETWIDTH_NAME PIXEL_SIZE
syn keyword bdfXProperties  contained POINT_SIZE RESOLUTION_X RESOLUTION_Y SPACING
syn keyword bdfXProperties  contained CHARSET_REGISTRY CHARSET_ENCODING COPYRIGHT
syn keyword bdfXProperties  contained ADD_STYLE_NAME WEIGHT RESOLUTION X_HEIGHT
syn keyword bdfXProperties  contained QUAD_WIDTH FONT AVERAGE_WIDTH

syn region  bdfDefinition   transparent matchgroup=bdfDelim start="^STARTPROPERTIES\>" end="^ENDPROPERTIES\>" contains=bdfXProperties,bdfNumber,bdfString

" characters
syn keyword bdfCharProperties contained ENCODING SWIDTH DWIDTH BBX ATTRIBUTES BITMAP

syn match   bdfCharName	    contained display "\<[0-9a-zA-Z]\{1,14}\>"
syn match   bdfCharNameError contained display "\<[0-9a-zA-Z]\{15,}\>"

syn region  bdfStartChar    transparent matchgroup=bdfDelim start="\<STARTCHAR\>" end="$" contains=bdfCharName,bdfCharNameError

syn region  bdfCharDefinition transparent start="^STARTCHAR\>" matchgroup=bdfDelim end="^ENDCHAR\>" contains=bdfCharProperties,bdfNumber,bdfStartChar

" font
syn region  bdfFontDefinition transparent matchgroup=bdfDelim start="^STARTFONT\>" end="^ENDFONT\>" contains=bdfProperties,bdfDefinition,bdfCharDefinition,bdfNumber,bdfComment

if exists("bdf_minlines")
  let b:bdf_minlines = bdf_minlines
else
  let b:bdf_minlines = 50
endif
exec "syn sync minlines=" . b:bdf_minlines

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_bdf_syn_inits")
  if version < 508
    let did_bdf_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink bdfComment		Comment
  HiLink bdfTodo		Todo
  HiLink bdfNumber		Number
  HiLink bdfString		String
  HiLink bdfProperties	Keyword
  HiLink bdfXProperties	Keyword
  HiLink bdfCharProperties	Structure
  HiLink bdfDelim		Delimiter
  HiLink bdfCharName		String
  HiLink bdfCharNameError	Error
  delcommand HiLink
endif

let b:current_syntax = "bdf"

" vim: set sts=2 sw=2:
