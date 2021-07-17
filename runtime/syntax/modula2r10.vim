" Vim syntax file
" Language:    Modula-2 (R10)
" Maintainer:  B.Kowarsch <trijezdci@moc.liamg>
" Last Change: 2020 June 18 (moved repository from bb to github)

" ----------------------------------------------------
" THIS FILE IS LICENSED UNDER THE VIM LICENSE
" see https://github.com/vim/vim/blob/master/LICENSE
" ----------------------------------------------------

" Remarks:
" Vim Syntax files are available for the following Modula-2 dialects:
" * for the PIM dialect : m2pim.vim
" * for the ISO dialect : m2iso.vim
" * for the R10 dialect : m2r10.vim (this file)

" -----------------------------------------------------------------------------
" This syntax description follows the Modula-2 Revision 2010 language report
" (Kowarsch and Sutcliffe, 2015) available at http://modula-2.info/m2r10.
" -----------------------------------------------------------------------------

" Parameters:
"
" Vim's filetype script recognises Modula-2 dialect tags within the first 100
" lines of Modula-2 .def and .mod input files.  The script sets filetype and
" dialect automatically when a valid dialect tag is found in the input file.
" The dialect tag for the R10 dialect is (*!m2r10*).  It is recommended to put
" the tag immediately after the module header in the Modula-2 input file.
"
" Example:
"  DEFINITION MODULE Foolib; (*!m2r10*)
"
" Variable g:modula2_default_dialect sets the default Modula-2 dialect when the
" dialect cannot be determined from the contents of the Modula-2 input file:
" if defined and set to 'm2r10', the default dialect is R10.
"
" Variable g:m2r10_allow_lowline controls support for lowline in identifiers:
" if defined and set to a non-zero value, they are recognised, otherwise not
"
" Variables may be defined in Vim startup file .vimrc
"
" Examples: 
"  let g:modula2_default_dialect = 'm2r10'
"  let g:m2r10_allow_lowline = 1


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
" Note: MODULE, PROCEDURE and END are defined separately further below
syn keyword m2Resword ALIAS AND ARGLIST ARRAY BEGIN CASE CONST COPY DEFINITION
syn keyword m2Resword DIV DO ELSE ELSIF EXIT FOR FROM GENLIB IF IMPLEMENTATION
syn keyword m2Resword IMPORT IN LOOP MOD NEW NOT OF OPAQUE OR POINTER READ
syn keyword m2Resword RECORD RELEASE REPEAT RETAIN RETURN SET THEN TO TYPE
syn keyword m2Resword UNTIL VAR WHILE WRITE YIELD


" -----------------------------------------------------------------------------
" Schroedinger's Tokens
" -----------------------------------------------------------------------------
syn keyword m2SchroedToken CAPACITY COROUTINE LITERAL


" -----------------------------------------------------------------------------
" Builtin Constant Identifiers
" -----------------------------------------------------------------------------
syn keyword m2ConstIdent NIL FALSE TRUE


" -----------------------------------------------------------------------------
" Builtin Type Identifiers
" -----------------------------------------------------------------------------
syn keyword m2TypeIdent BOOLEAN CHAR UNICHAR OCTET
syn keyword m2TypeIdent CARDINAL LONGCARD INTEGER LONGINT REAL LONGREAL


" -----------------------------------------------------------------------------
" Builtin Procedure and Function Identifiers
" -----------------------------------------------------------------------------
syn keyword m2ProcIdent APPEND INSERT REMOVE SORT SORTNEW
syn keyword m2FuncIdent CHR ORD ODD ABS SGN MIN MAX LOG2 POW2 ENTIER
syn keyword m2FuncIdent PRED SUCC PTR COUNT LENGTH


" -----------------------------------------------------------------------------
" Builtin Macro Identifiers
" -----------------------------------------------------------------------------
syn keyword m2MacroIdent NOP TMIN TMAX TSIZE TLIMIT


" -----------------------------------------------------------------------------
" Builtin Primitives
" -----------------------------------------------------------------------------
syn keyword m2PrimitiveIdent SXF VAL STORE VALUE SEEK SUBSET


" -----------------------------------------------------------------------------
" Unsafe Facilities via Pseudo-Module UNSAFE
" -----------------------------------------------------------------------------
syn keyword m2UnsafeIdent UNSAFE BYTE WORD LONGWORD OCTETSEQ
syn keyword m2UnsafeIdent ADD SUB INC DEC SETBIT HALT
syn keyword m2UnsafeIdent ADR CAST BIT SHL SHR BWNOT BWAND BWOR


" -----------------------------------------------------------------------------
" Non-Portable Language Extensions
" -----------------------------------------------------------------------------
syn keyword m2NonPortableIdent ASSEMBLER ASM REG


" -----------------------------------------------------------------------------
" User Defined Identifiers
" -----------------------------------------------------------------------------
syn match m2Ident "[a-zA-Z][a-zA-Z0-9]*\(_\)\@!"
syn match m2LowLineIdent "[a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)\+"

syn match m2ReswordDo "\(TO\)\@<!DO"
syn match m2ReswordTo "TO\(\sDO\)\@!"

" TODO: support for OpenVMS reswords and identifiers which may include $ and %


" -----------------------------------------------------------------------------
" String Literals
" -----------------------------------------------------------------------------
syn region m2String start=/"/ end=/"/ oneline
syn region m2String start="\(^\|\s\|[({=<>&#,]\|\[\)\@<='" end=/'/ oneline


" -----------------------------------------------------------------------------
" Numeric Literals
" -----------------------------------------------------------------------------
syn match m2Base2Num "0b[01]\+\('[01]\+\)*"
syn match m2Base16Num "0[ux][0-9A-F]\+\('[0-9A-F]\+\)*"

"| *** VMSCRIPT BUG ALERT ***
"| The regular expression below causes errors when split into separate strings
"|
"| syn match m2Base10Num
"|   \ "\(\(0[bux]\@!\|[1-9]\)[0-9]*\('[0-9]\+\)*\)" .
"|   \ "\(\.[0-9]\+\('[0-9]\+\)*\(e[+-]\?[0-9]\+\('[0-9]\+\)*\)\?\)\?"
"|
"| E475: Invalid argument: m2Base10Num "\(\(0[bux]\@!\|[1-9]\)[0-9]*\('[0-9]\+\)*\)"
"|  . "\(\.[0-9]\+\('[0-9]\+\)*\(e[+-]\?[0-9]\+\('[0-9]\+\)*\)\?\)\?"
"|
"| However, the same regular expression works just fine as a sole string.
"| 
"| As a consequence, we have no choice but to put it all into a single line
"| which greatly diminishes readability and thereby increases the opportunity
"| for error during maintenance. Ideally, regular expressions should be split
"| into small human readable pieces with interleaved comments that explain
"| precisely what each piece is doing.  Vimscript imposes poor design. :-( 

syn match m2Base10Num
  \ "\(\(0[bux]\@!\|[1-9]\)[0-9]*\('[0-9]\+\)*\)\(\.[0-9]\+\('[0-9]\+\)*\(e[+-]\?[0-9]\+\('[0-9]\+\)*\)\?\)\?"


" -----------------------------------------------------------------------------
" Punctuation
" -----------------------------------------------------------------------------
syn match m2Punctuation
  \ "\.\|[,:;]\|\*\|[/+-]\|\#\|[=<>&]\|\^\|\[\|\]\|(\(\*\)\@!\|[){}]"


" -----------------------------------------------------------------------------
" Pragmas
" -----------------------------------------------------------------------------
syn region m2Pragma start="<\*" end="\*>"
  \ contains = m2PragmaKey, m2TechDebtPragma
syn keyword m2PragmaKey contained MSG IF ELSIF ELSE END INLINE NOINLINE OUT
syn keyword m2PragmaKey contained GENERATED ENCODING ALIGN PADBITS NORETURN
syn keyword m2PragmaKey contained PURITY SINGLEASSIGN LOWLATENCY VOLATILE
syn keyword m2PragmaKey contained FORWARD ADDR FFI FFIDENT

syn match m2DialectTag "(\*!m2r10\(+[a-z0-9]\+\)\?\*)"


" -----------------------------------------------------------------------------
" Line Comments
" -----------------------------------------------------------------------------
syn region m2Comment start=/^!/ end=/$/ oneline


" -----------------------------------------------------------------------------
" Block Comments
" -----------------------------------------------------------------------------
syn region m2Comment
  \ start="\(END\s\)\@<!(\*\(!m2r10\(+[a-z0-9]\+\)\?\*)\)\@!" end="\*)"
  \ contains = m2Comment, m2CommentKey, m2TechDebtMarker

syn match m2CommentKey
  \ "[Aa]uthor[s]\?\|[Cc]opyright\|[Ll]icense\|[Ss]ynopsis" contained
syn match m2CommentKey
  \ "\([Pp]re\|[Pp]ost\|[Ee]rror\)\-condition[s]\?:" contained


" -----------------------------------------------------------------------------
" Block Statement Tails
" -----------------------------------------------------------------------------
syn match m2ReswordEnd
  \ "END" nextgroup = m2StmtTailComment skipwhite
syn match m2StmtTailComment
  \ "(\*\s\(IF\|CASE\|FOR\|LOOP\|WHILE\)\s\*)" contained


" -----------------------------------------------------------------------------
" Technical Debt Markers
" -----------------------------------------------------------------------------
syn match m2ToDoHeader "TO DO"

syn match m2ToDoTail
  \ "END\(\s(\*\sTO DO\s\*)\)\@=" nextgroup = m2ToDoTailComment skipwhite
syntax match m2ToDoTailComment "(\*\sTO DO\s\*)" contained

" contained within pragma
syn keyword m2TechDebtPragma contained DEPRECATED

" contained within comment
syn keyword m2TechDebtMarker contained FIXME


" -----------------------------------------------------------------------------
" Disabled Code Sections
" -----------------------------------------------------------------------------
syn region m2DisabledCode start="^?<" end="^>?"


" -----------------------------------------------------------------------------
" Headers
" -----------------------------------------------------------------------------
" !!! this section must be second last !!!

" module header
syn match m2ModuleHeader
  \ "\(MODULE\|BLUEPRINT\)\( [A-Z][a-zA-Z0-9]*\)\?"
  \ contains = m2ReswordModule, m2ReswordBlueprint, m2ModuleIdent

syn match m2ModuleIdent
  \ "[A-Z][a-zA-Z0-9]*" contained

syn match m2ModuleTail
  \ "END [A-Z][a-zA-Z0-9]*\.$"
  \ contains = m2ReswordEnd, m2ModuleIdent, m2Punctuation

" procedure, sole occurrence
syn match m2ProcedureHeader
  \ "PROCEDURE\(\s\[\|\s[a-zA-Z]\)\@!" contains = m2ReswordProcedure

" procedure header
syn match m2ProcedureHeader
  \ "PROCEDURE [a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)*"
  \ contains = m2ReswordProcedure,
  \ m2ProcedureIdent, m2ProcedureLowlineIdent, m2IllegalChar, m2IllegalIdent

" procedure binding to operator
syn match m2ProcedureHeader
  \ "PROCEDURE \[[+-\*/\\=<>]\] [a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)*"
  \ contains = m2ReswordProcedure, m2Punctuation,
  \ m2ProcedureIdent, m2ProcedureLowlineIdent, m2IllegalChar, m2IllegalIdent

" procedure binding to builtin
syn match m2ProcedureHeader
  \ "PROCEDURE \[[A-Z]\+\(:\([#\*,]\|++\|--\)\?\)\?\] [a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)*"
  \ contains = m2ReswordProcedure,
  \ m2Punctuation, m2Resword, m2SchroedToken,
  \ m2ProcIdent, m2FuncIdent, m2PrimitiveIdent,
  \ m2ProcedureIdent, m2ProcedureLowlineIdent, m2IllegalChar, m2IllegalIdent

syn match m2ProcedureIdent
  \ "\([a-zA-Z]\)\([a-zA-Z0-9]*\)" contained

syn match m2ProcedureLowlineIdent
  \ "[a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)\+" contained

syn match m2ProcedureTail
  \ "END [a-zA-Z][a-zA-Z0-9]*\(_[a-zA-Z0-9]\+\)*;$"
  \ contains = m2ReswordEnd,
  \ m2ProcedureIdent, m2ProcedureLowLineIdent,
  \ m2Punctuation, m2IllegalChar, m2IllegalIdent

syn keyword m2ReswordModule contained MODULE
syn keyword m2ReswordBlueprint contained BLUEPRINT
syn keyword m2ReswordProcedure contained PROCEDURE
syn keyword m2ReswordEnd contained END


" -----------------------------------------------------------------------------
" Illegal Symbols
" -----------------------------------------------------------------------------
" !!! this section must be last !!!

" any '`' '~' '@' '$' '%'
syn match m2IllegalChar "[`~@$%]"

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

highlight m2PragmaKeyStyle
  \ gui=bold guifg=#CC6600 guibg=NONE
  \ term=italic cterm=bold ctermfg=215 ctermbg=NONE

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
highlight link m2ModuleHeader m2HeaderIdentStyle
highlight link m2ModuleTail Normal
highlight link m2ProcedureHeader Normal
highlight link m2ProcedureTail Normal

" lowline identifiers are rendered as errors if g:m2r10_allow_lowline is unset
if exists("g:m2r10_allow_lowline")
  if g:m2r10_allow_lowline != 0
    highlight link m2ProcedureLowlineIdent m2HeaderIdentStyle
  else
    highlight link m2ProcedureLowlineIdent Error
  endif
else
  highlight link m2ProcedureLowlineIdent m2HeaderIdentStyle
endif

" reserved words
highlight link m2Resword m2ReswordStyle
highlight link m2ReswordModule m2ReswordStyle
highlight link m2ReswordProcedure m2ReswordStyle
highlight link m2ReswordEnd m2ReswordStyle
highlight link m2ReswordDo m2ReswordStyle
highlight link m2ReswordTo m2ReswordStyle
highlight link m2SchroedToken m2ReswordStyle

" predefined identifiers
highlight link m2ConstIdent m2PredefIdentStyle
highlight link m2TypeIdent m2PredefIdentStyle
highlight link m2ProcIdent m2PredefIdentStyle
highlight link m2FuncIdent m2PredefIdentStyle
highlight link m2MacroIdent m2PredefIdentStyle
highlight link m2PrimitiveIdent m2PredefIdentStyle

" unsafe and non-portable identifiers
highlight link m2UnsafeIdent m2UnsafeIdentStyle
highlight link m2NonPortableIdent m2NonPortableIdentStyle

" user defined identifiers
highlight link m2Ident m2UserDefIdentStyle

" lowline identifiers are rendered as errors if g:m2r10_allow_lowline is unset
if exists("g:m2r10_allow_lowline")
  if g:m2r10_allow_lowline != 0
    highlight link m2LowLineIdent m2UserDefIdentStyle
  else
    highlight link m2LowLineIdent Error
  endif
else
  highlight link m2LowLineIdent m2UserDefIdentStyle
endif

" literals
highlight link m2String m2StringLiteralStyle
highlight link m2Base2Num m2NumericLiteralStyle
highlight link m2Base10Num m2NumericLiteralStyle
highlight link m2Base16Num m2NumericLiteralStyle

" punctuation
highlight link m2Punctuation m2PunctuationStyle

" pragmas
highlight link m2Pragma m2PragmaStyle
highlight link m2PragmaKey m2PragmaKeyStyle
highlight link m2DialectTag m2DialectTagStyle

" comments
highlight link m2Comment m2CommentStyle
highlight link m2CommentKey m2CommentKeyStyle
highlight link m2ToDoTailComment m2CommentStyle
highlight link m2StmtTailComment m2CommentStyle

" technical debt markers
highlight link m2ToDoHeader m2TechDebtMarkerStyle
highlight link m2ToDoTail m2TechDebtMarkerStyle
highlight link m2TechDebtPragma m2TechDebtMarkerStyle

" disabled code
highlight link m2DisabledCode m2DisabledCodeStyle

" illegal symbols
highlight link m2IllegalChar Error
highlight link m2IllegalIdent Error


let b:current_syntax = "m2r10"

" vim: ts=4

" END OF FILE
