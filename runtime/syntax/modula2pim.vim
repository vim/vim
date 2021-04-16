" Vim syntax file
" Language:    Modula-2 (PIM)
" Maintainer:  B.Kowarsch <trijezdci@moc.liamg>
" Last Change: 2016 August 22

" ----------------------------------------------------
" THIS FILE IS LICENSED UNDER THE VIM LICENSE
" see https://github.com/vim/vim/blob/master/LICENSE
" ----------------------------------------------------

" Remarks:
" Vim Syntax files are available for the following Modula-2 dialects:
" * for the PIM dialect : m2pim.vim (this file)
" * for the ISO dialect : m2iso.vim
" * for the R10 dialect : m2r10.vim

" -----------------------------------------------------------------------------
" This syntax description follows the 3rd and 4th editions of N.Wirth's Book
" Programming in Modula-2 (aka PIM) plus the following language extensions:
" * non-leading, non-trailing, non-consecutive lowlines _ in identifiers
" * widely supported non-standard types BYTE, LONGCARD and LONGBITSET
" * non-nesting code disabling tags ?< and >? at the start of a line
" -----------------------------------------------------------------------------

" Parameters:
"
" Vim's filetype script recognises Modula-2 dialect tags within the first 100
" lines of Modula-2 .def and .mod input files.  The script sets filetype and
" dialect automatically when a valid dialect tag is found in the input file.
" The dialect tag for the PIM dialect is (*!m2pim*).  It is recommended to put
" the tag immediately after the module header in the Modula-2 input file.
"
" Example:
"  DEFINITION MODULE Foolib; (*!m2pim*)
"
" Variable g:modula2_default_dialect sets the default Modula-2 dialect when the
" dialect cannot be determined from the contents of the Modula-2 input file:
" if defined and set to 'm2pim', the default dialect is PIM.
"
" Variable g:m2pim_allow_lowline controls support for lowline in identifiers:
" if defined and set to a non-zero value, they are recognised, otherwise not
"
" Variable g:m2pim_disallow_octals controls the rendering of octal literals:
" if defined and set to a non-zero value, they are rendered as errors.
"
" Variable g:m2pim_disallow_synonyms controls the rendering of & and ~:
" if defined and set to a non-zero value, they are rendered as errors.
"
" Variables may be defined in Vim startup file .vimrc
"
" Examples: 
"  let g:modula2_default_dialect = 'm2pim'
"  let g:m2pim_allow_lowline = 1
"  let g:m2pim_disallow_octals = 1
"  let g:m2pim_disallow_synonyms = 1


" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Modula-2 is case sensitive
syn case match


" -----------------------------------------------------------------------------
" Reserved Words
" -----------------------------------------------------------------------------
syn keyword m2Resword AND ARRAY BEGIN BY CASE CONST DEFINITION DIV DO ELSE
syn keyword m2Resword ELSIF EXIT EXPORT FOR FROM IF IMPLEMENTATION IMPORT
syn keyword m2Resword IN LOOP MOD NOT OF OR POINTER QUALIFIED RECORD REPEAT
syn keyword m2Resword RETURN SET THEN TO TYPE UNTIL VAR WHILE WITH


" -----------------------------------------------------------------------------
" Builtin Constant Identifiers
" -----------------------------------------------------------------------------
syn keyword m2ConstIdent FALSE NIL TRUE


" -----------------------------------------------------------------------------
" Builtin Type Identifiers
" -----------------------------------------------------------------------------
syn keyword m2TypeIdent BITSET BOOLEAN CHAR PROC
syn keyword m2TypeIdent CARDINAL INTEGER LONGINT REAL LONGREAL


" -----------------------------------------------------------------------------
" Builtin Procedure and Function Identifiers
" -----------------------------------------------------------------------------
syn keyword m2ProcIdent CAP DEC EXCL HALT INC INCL
syn keyword m2FuncIdent ABS CHR FLOAT HIGH MAX MIN ODD ORD SIZE TRUNC VAL


" -----------------------------------------------------------------------------
" Wirthian Macro Identifiers
" -----------------------------------------------------------------------------
syn keyword m2MacroIdent NEW DISPOSE


" -----------------------------------------------------------------------------
" Unsafe Facilities via Pseudo-Module SYSTEM
" -----------------------------------------------------------------------------
syn keyword m2UnsafeIdent ADDRESS PROCESS WORD
syn keyword m2UnsafeIdent ADR TSIZE NEWPROCESS TRANSFER SYSTEM


" -----------------------------------------------------------------------------
" Non-Portable Language Extensions
" -----------------------------------------------------------------------------
syn keyword m2NonPortableIdent BYTE LONGCARD LONGBITSET


" -----------------------------------------------------------------------------
" User Defined Identifiers
" -----------------------------------------------------------------------------
syn match m2Ident "[a-zA-Z][a-zA-Z0-9]*\(_\)\@!"
syn match m2LowLineIdent "[a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)\+"


" -----------------------------------------------------------------------------
" String Literals
" -----------------------------------------------------------------------------
syn region m2String start=/"/ end=/"/ oneline
syn region m2String start=/'/ end=/'/ oneline


" -----------------------------------------------------------------------------
" Numeric Literals
" -----------------------------------------------------------------------------
syn match m2Num
  \ "\(\([0-7]\+\)[BC]\@!\|[89]\)[0-9]*\(\.[0-9]\+\([eE][+-]\?[0-9]\+\)\?\)\?"
syn match m2Num "[0-9A-F]\+H"
syn match m2Octal "[0-7]\+[BC]"


" -----------------------------------------------------------------------------
" Punctuation
" -----------------------------------------------------------------------------
syn match m2Punctuation
  \ "\.\|[,:;]\|\*\|[/+-]\|\#\|[=<>]\|\^\|\[\|\]\|(\(\*\)\@!\|[){}]"
syn match m2Synonym "[&~]"


" -----------------------------------------------------------------------------
" Pragmas
" -----------------------------------------------------------------------------
syn region m2Pragma start="(\*\$" end="\*)"
syn match m2DialectTag "(\*!m2pim\(+[a-z0-9]\+\)\?\*)"

" -----------------------------------------------------------------------------
" Block Comments
" -----------------------------------------------------------------------------
syn region m2Comment start="(\*\(\$\|!m2pim\(+[a-z0-9]\+\)\?\*)\)\@!" end="\*)"
  \ contains = m2Comment, m2CommentKey, m2TechDebtMarker
syn match m2CommentKey "[Aa]uthor[s]\?\|[Cc]opyright\|[Ll]icense\|[Ss]ynopsis"
syn match m2CommentKey "\([Pp]re\|[Pp]ost\|[Ee]rror\)\-condition[s]\?:"


" -----------------------------------------------------------------------------
" Technical Debt Markers
" -----------------------------------------------------------------------------
syn keyword m2TechDebtMarker contained DEPRECATED FIXME
syn match m2TechDebtMarker "TODO[:]\?" contained

" -----------------------------------------------------------------------------
" Disabled Code Sections
" -----------------------------------------------------------------------------
syn region m2DisabledCode start="^?<" end="^>?"


" -----------------------------------------------------------------------------
" Headers
" -----------------------------------------------------------------------------
" !!! this section must be second last !!!

" new module header
syn match m2ModuleHeader
  \ "MODULE\( [A-Z][a-zA-Z0-9]*\)\?"
  \ contains = m2ReswordModule, m2ModuleIdent

syn match m2ModuleIdent
  \ "[A-Z][a-zA-Z0-9]*" contained

syn match m2ModuleTail
  \ "END [A-Z][a-zA-Z0-9]*.$"
  \ contains = m2ReswordEnd, m2ModuleIdent, m2Punctuation

" new procedure header
syn match m2ProcedureHeader
  \ "PROCEDURE\( [a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)*\)\?"
  \ contains = m2ReswordProcedure,
  \ m2ProcedureIdent, m2ProcedureLowlineIdent, m2IllegalChar, m2IllegalIdent

syn match m2ProcedureIdent
  \ "\([a-zA-Z]\)\([a-zA-Z0-9]*\)" contained

syn match m2ProcedureLowlineIdent
  \ "[a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)\+" contained

syn match m2ProcedureTail
  \ "END\( \([a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)*\)[.;]$\)\?"
  \ contains = m2ReswordEnd,
  \ m2ProcedureIdent, m2ProcedureLowLineIdent,
  \ m2Punctuation, m2IllegalChar, m2IllegalIdent

syn keyword m2ReswordModule contained MODULE
syn keyword m2ReswordProcedure contained PROCEDURE
syn keyword m2ReswordEnd contained END


" -----------------------------------------------------------------------------
" Illegal Symbols
" -----------------------------------------------------------------------------
" !!! this section must be last !!!

" any '`' '!' '@ ''$' '%' or '\'
syn match m2IllegalChar "[`!@$%\\]"

" any solitary sequence of '_'
syn match m2IllegalChar "\<_\+\>"

" any '?' at start of line if not followed by '<'
syn match m2IllegalChar "^?\(<\)\@!"

" any '?' not following '>' at start of line
syn match m2IllegalChar "\(\(^>\)\|\(^\)\)\@<!?"

" any identifiers with leading occurrences of '_'
syn match m2IllegalIdent "_\+[a-zA-Z][a-zA-Z0-9]*\(_\+[a-zA-Z0-9]*\)*"

" any identifiers containing consecutive occurences of '_'
syn match m2IllegalIdent
  \ "[a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)*\(__\+[a-zA-Z0-9]\+\(_[a-zA-Z0-9]\+\)*\)\+"

" any identifiers with trailing occurrences of '_'
syn match m2IllegalIdent "[a-zA-Z][a-zA-Z0-9]*\(_\+[a-zA-Z0-9]\+\)*_\+\>"


" -----------------------------------------------------------------------------
" Define Rendering Styles
" -----------------------------------------------------------------------------
highlight m2PredefIdentStyle
  \ gui=bold guifg=Green4 guibg=NONE
  \ term=bold,italic cterm=bold ctermfg=28 ctermbg=NONE

highlight m2UnsafeIdentStyle
  \ gui=bold guifg=DarkOrange guibg=NONE
  \ term=bold,italic,undercurl cterm=bold ctermfg=208 ctermbg=NONE

highlight m2NonPortableIdentStyle
  \ gui=bold guifg=DarkOrange guibg=NONE
  \ term=bold,italic,undercurl cterm=bold ctermfg=208 ctermbg=NONE

highlight m2StringLiteralStyle
  \ gui=NONE guifg=DeepSkyBlue3 guibg=NONE
  \ term=NONE cterm=NONE ctermfg=32 ctermbg=NONE

highlight m2CommentStyle
  \ gui=italic guifg=Gray60 guibg=NONE
  \ term=italic cterm=italic ctermfg=246 ctermbg=NONE

highlight m2PragmaStyle
  \ gui=NONE guifg=#CC6600 guibg=NONE
  \ term=italic cterm=NONE ctermfg=215 ctermbg=NONE

highlight m2DialectTagStyle
  \ gui=bold,italic guifg=Gray60 guibg=NONE
  \ term=bold,italic cterm=bold,italic ctermfg=246 ctermbg=NONE

highlight m2TechDebtMarkerStyle
  \ gui=bold,italic guifg=Red guibg=NONE
  \ term=standout,bold,italic cterm=bold,italic ctermfg=196 ctermbg=NONE

if &background == "dark"
  highlight m2ReswordStyle
    \ gui=bold guifg=#CC0066 guibg=NONE
    \ term=bold,underline cterm=bold ctermfg=124 ctermbg=NONE
    
  highlight m2HeaderIdentStyle
    \ gui=bold guifg=LightGray guibg=NONE
    \ term=bold cterm=bold ctermfg=244 ctermbg=NONE
    
  highlight m2UserDefIdentStyle
    \ gui=NONE guifg=LightGray guibg=NONE
    \ term=NONE cterm=NONE ctermfg=244 ctermbg=NONE
    
  highlight m2NumericLiteralStyle
    \ gui=NONE guifg=SlateGray guibg=NONE
    \ term=NONE cterm=NONE ctermfg=242 ctermbg=NONE
    
  highlight m2PunctuationStyle
    \ gui=NONE guifg=LightGray guibg=NONE
    \ term=NONE cterm=NONE ctermfg=244 ctermbg=NONE
    
  highlight m2CommentKeyStyle
    \ gui=bold,italic guifg=Gray65 guibg=NONE
    \ term=bold,italic cterm=bold,italic ctermfg=248 ctermbg=NONE
    
  highlight m2DisabledCodeStyle
    \ gui=NONE guifg=Gray30 guibg=NONE
    \ term=inverse,italic cterm=NONE ctermfg=239 ctermbg=NONE
    
else " &background == "light"
  highlight m2ReswordStyle
    \ gui=bold guifg=#990033 guibg=NONE
    \ term=bold,underline cterm=bold ctermfg=88 ctermbg=NONE
    
  highlight m2HeaderIdentStyle
    \ gui=bold guifg=DarkBlue guibg=NONE
    \ term=bold cterm=bold ctermfg=18 ctermbg=NONE
    
  highlight m2UserDefIdentStyle
    \ gui=NONE guifg=Black guibg=NONE
    \ term=NONE cterm=NONE ctermfg=Black ctermbg=NONE
    
  highlight m2NumericLiteralStyle
    \ gui=NONE guifg=DarkSlateGray guibg=NONE
    \ term=NONE cterm=NONE ctermfg=240 ctermbg=NONE
    
  highlight m2PunctuationStyle
    \ gui=NONE guifg=Black guibg=NONE
    \ term=NONE cterm=NONE ctermfg=Black ctermbg=NONE
    
  highlight m2CommentKeyStyle
    \ gui=bold,italic guifg=Gray55 guibg=NONE
    \ term=bold,italic cterm=bold,italic ctermfg=245 ctermbg=NONE
    
  highlight m2DisabledCodeStyle
    \ gui=NONE guifg=Gray85 guibg=NONE
    \ term=inverse,italic cterm=NONE ctermfg=253 ctermbg=NONE
endif


" -----------------------------------------------------------------------------
" Assign Rendering Styles
" -----------------------------------------------------------------------------

" headers
highlight link m2ModuleIdent m2HeaderIdentStyle
highlight link m2ProcedureIdent m2HeaderIdentStyle
highlight link m2ModuleHeader Normal
highlight link m2ModuleTail Normal
highlight link m2ProcedureHeader Normal
highlight link m2ProcedureTail Normal

" lowline identifiers are rendered as errors if g:m2pim_allow_lowline is unset
if exists("g:m2pim_allow_lowline")
  if g:m2pim_allow_lowline != 0
    highlight link m2ProcedureLowlineIdent m2HeaderIdentStyle
  else
    highlight link m2ProcedureLowlineIdent Error
  endif
else
  highlight link m2ProcedureLowlineIdent Error
endif

" reserved words
highlight link m2Resword m2ReswordStyle
highlight link m2ReswordModule m2ReswordStyle
highlight link m2ReswordProcedure m2ReswordStyle
highlight link m2ReswordEnd m2ReswordStyle

" predefined identifiers
highlight link m2ConstIdent m2PredefIdentStyle
highlight link m2TypeIdent m2PredefIdentStyle
highlight link m2ProcIdent m2PredefIdentStyle
highlight link m2FuncIdent m2PredefIdentStyle
highlight link m2MacroIdent m2PredefIdentStyle

" unsafe and non-portable identifiers
highlight link m2UnsafeIdent m2UnsafeIdentStyle
highlight link m2NonPortableIdent m2NonPortableIdentStyle

" user defined identifiers
highlight link m2Ident m2UserDefIdentStyle

" lowline identifiers are rendered as errors if g:m2pim_allow_lowline is unset
if exists("g:m2pim_allow_lowline")
  if g:m2pim_allow_lowline != 0
    highlight link m2LowLineIdent m2UserDefIdentStyle
  else
    highlight link m2LowLineIdent Error
  endif
else
  highlight link m2LowLineIdent Error
endif

" literals
highlight link m2String m2StringLiteralStyle
highlight link m2Num m2NumericLiteralStyle

" octal literals are rendered as errors if g:m2pim_disallow_octals is set
if exists("g:m2pim_disallow_octals")
  if g:m2pim_disallow_octals != 0
    highlight link m2Octal Error
  else
    highlight link m2Octal m2NumericLiteralStyle
  endif
else
  highlight link m2Octal m2NumericLiteralStyle
endif

" punctuation
highlight link m2Punctuation m2PunctuationStyle

" synonyms & and ~ are rendered as errors if g:m2pim_disallow_synonyms is set
if exists("g:m2pim_disallow_synonyms")
  if g:m2pim_disallow_synonyms != 0
    highlight link m2Synonym Error
  else
    highlight link m2Synonym m2PunctuationStyle
  endif
else
  highlight link m2Synonym m2PunctuationStyle
endif

" pragmas
highlight link m2Pragma m2PragmaStyle
highlight link m2DialectTag m2DialectTagStyle

" comments
highlight link m2Comment m2CommentStyle
highlight link m2CommentKey m2CommentKeyStyle

" technical debt markers
highlight link m2TechDebtMarker m2TechDebtMarkerStyle

" disabled code
highlight link m2DisabledCode m2DisabledCodeStyle

" illegal symbols
highlight link m2IllegalChar Error
highlight link m2IllegalIdent Error


let b:current_syntax = "m2pim"

" vim: ts=4

" END OF FILE
