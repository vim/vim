" Vim syntax file
" Language:	Vim help file
" Maintainer:	Bram Moolenaar (Bram@vim.org)
" Last Change:	2004 May 17

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn match helpHeadline		"^[A-Z ]\+[ ]\+\*"me=e-1
syn match helpSectionDelim	"^=\{3,}.*===$"
syn match helpSectionDelim	"^-\{3,}.*--$"
syn region helpExample		matchgroup=helpIgnore start=" >$" start="^>$" end="^[^ \t]"me=e-1 end="^<"
if has("ebcdic")
  syn match helpHyperTextJump	"\\\@<!|[^"*|]\+|"
  syn match helpHyperTextEntry	"\*[^"*|]\+\*\s"he=e-1
  syn match helpHyperTextEntry	"\*[^"*|]\+\*$"
else
  syn match helpHyperTextJump	"\\\@<!|[#-)!+-~]\+|"
  syn match helpHyperTextEntry	"\*[#-)!+-~]\+\*\s"he=e-1
  syn match helpHyperTextEntry	"\*[#-)!+-~]\+\*$"
endif
syn match helpNormal		"|.*====*|"
syn match helpNormal		":|vim:|"	" for :help modeline
syn match helpVim		"Vim version [0-9.a-z]\+"
syn match helpVim		"VIM REFERENCE.*"
syn match helpOption		"'[a-z]\{2,\}'"
syn match helpOption		"'t_..'"
syn match helpHeader		"\s*\zs.\{-}\ze\s\=\~$" nextgroup=helpIgnore
syn match helpIgnore		"." contained
syn keyword helpNote		note Note NOTE note: Note: NOTE: Notes Notes:
syn match helpSpecial		"\<N\>"
syn match helpSpecial		"\<N\.$"me=e-1
syn match helpSpecial		"\<N\.\s"me=e-2
syn match helpSpecial		"(N\>"ms=s+1
syn match helpSpecial		"\[N]"
" avoid highlighting N  N in help.txt
syn match helpSpecial		"N  N"he=s+1
syn match helpSpecial		"Nth"me=e-2
syn match helpSpecial		"N-1"me=e-2
syn match helpSpecial		"{[-a-zA-Z0-9'":%#=[\]<>.,]\+}"
syn match helpSpecial		"{[-a-zA-Z0-9'"*+/:%#=[\]<>.,]\+}"
syn match helpSpecial		"\s\[[-a-z^A-Z0-9_]\{2,}]"ms=s+1
syn match helpSpecial		"<[-a-zA-Z0-9_]\+>"
syn match helpSpecial		"<[SCM]-.>"
syn match helpNormal		"<---*>"
syn match helpSpecial		"\[range]"
syn match helpSpecial		"\[line]"
syn match helpSpecial		"\[count]"
syn match helpSpecial		"\[offset]"
syn match helpSpecial		"\[cmd]"
syn match helpSpecial		"\[num]"
syn match helpSpecial		"\[+num]"
syn match helpSpecial		"\[-num]"
syn match helpSpecial		"\[+cmd]"
syn match helpSpecial		"\[++opt]"
syn match helpSpecial		"\[arg]"
syn match helpSpecial		"\[arguments]"
syn match helpSpecial		"\[ident]"
syn match helpSpecial		"\[addr]"
syn match helpSpecial		"\[group]"
syn match helpSpecial		"CTRL-."
syn match helpSpecial		"CTRL-Break"
syn match helpSpecial		"CTRL-PageUp"
syn match helpSpecial		"CTRL-PageDown"
syn match helpSpecial		"CTRL-Insert"
syn match helpSpecial		"CTRL-Del"
syn match helpSpecial		"CTRL-{char}"
syn region helpNotVi		start="{Vi[: ]" start="{not" start="{only" end="}" contains=helpLeadBlank,helpHyperTextJump
syn match helpLeadBlank		"^\s\+" contained

" Highlight group items in their own color.
syn match helpComment		"\t[* ]Comment\t\+[a-z].*"
syn match helpConstant		"\t[* ]Constant\t\+[a-z].*"
syn match helpString		"\t[* ]String\t\+[a-z].*"
syn match helpCharacter		"\t[* ]Character\t\+[a-z].*"
syn match helpNumber		"\t[* ]Number\t\+[a-z].*"
syn match helpBoolean		"\t[* ]Boolean\t\+[a-z].*"
syn match helpFloat		"\t[* ]Float\t\+[a-z].*"
syn match helpIdentifier	"\t[* ]Identifier\t\+[a-z].*"
syn match helpFunction		"\t[* ]Function\t\+[a-z].*"
syn match helpStatement		"\t[* ]Statement\t\+[a-z].*"
syn match helpConditional	"\t[* ]Conditional\t\+[a-z].*"
syn match helpRepeat		"\t[* ]Repeat\t\+[a-z].*"
syn match helpLabel		"\t[* ]Label\t\+[a-z].*"
syn match helpOperator		"\t[* ]Operator\t\+["a-z].*"
syn match helpKeyword		"\t[* ]Keyword\t\+[a-z].*"
syn match helpException		"\t[* ]Exception\t\+[a-z].*"
syn match helpPreProc		"\t[* ]PreProc\t\+[a-z].*"
syn match helpInclude		"\t[* ]Include\t\+[a-z].*"
syn match helpDefine		"\t[* ]Define\t\+[a-z].*"
syn match helpMacro		"\t[* ]Macro\t\+[a-z].*"
syn match helpPreCondit		"\t[* ]PreCondit\t\+[a-z].*"
syn match helpType		"\t[* ]Type\t\+[a-z].*"
syn match helpStorageClass	"\t[* ]StorageClass\t\+[a-z].*"
syn match helpStructure		"\t[* ]Structure\t\+[a-z].*"
syn match helpTypedef		"\t[* ]Typedef\t\+[Aa-z].*"
syn match helpSpecial		"\t[* ]Special\t\+[a-z].*"
syn match helpSpecialChar	"\t[* ]SpecialChar\t\+[a-z].*"
syn match helpTag		"\t[* ]Tag\t\+[a-z].*"
syn match helpDelimiter		"\t[* ]Delimiter\t\+[a-z].*"
syn match helpSpecialComment	"\t[* ]SpecialComment\t\+[a-z].*"
syn match helpDebug		"\t[* ]Debug\t\+[a-z].*"
syn match helpUnderlined	"\t[* ]Underlined\t\+[a-z].*"
syn match helpError		"\t[* ]Error\t\+[a-z].*"
syn match helpTodo		"\t[* ]Todo\t\+[a-z].*"


" Additionally load a language-specific syntax file "help_ab.vim".
let i = match(expand("%"), '\.\a\ax$')
if i > 0
  exe "runtime syntax/help_" . strpart(expand("%"), i + 1, 2) . ".vim"
endif

syn sync minlines=40


" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_help_syntax_inits")
  if version < 508
    let did_help_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink helpExampleStart	helpIgnore
  HiLink helpIgnore		Ignore
  HiLink helpHyperTextJump	Subtitle
  HiLink helpHyperTextEntry	String
  HiLink helpHeadline		Statement
  HiLink helpHeader		PreProc
  HiLink helpSectionDelim	PreProc
  HiLink helpVim		Identifier
  HiLink helpExample		Comment
  HiLink helpOption		Type
  HiLink helpNotVi		Special
  HiLink helpSpecial		Special
  HiLink helpNote		Todo
  HiLink Subtitle		Identifier

  HiLink helpComment		Comment
  HiLink helpConstant		Constant
  HiLink helpString		String
  HiLink helpCharacter		Character
  HiLink helpNumber		Number
  HiLink helpBoolean		Boolean
  HiLink helpFloat		Float
  HiLink helpIdentifier		Identifier
  HiLink helpFunction		Function
  HiLink helpStatement		Statement
  HiLink helpConditional	Conditional
  HiLink helpRepeat		Repeat
  HiLink helpLabel		Label
  HiLink helpOperator		Operator
  HiLink helpKeyword		Keyword
  HiLink helpException		Exception
  HiLink helpPreProc		PreProc
  HiLink helpInclude		Include
  HiLink helpDefine		Define
  HiLink helpMacro		Macro
  HiLink helpPreCondit		PreCondit
  HiLink helpType		Type
  HiLink helpStorageClass	StorageClass
  HiLink helpStructure		Structure
  HiLink helpTypedef		Typedef
  HiLink helpSpecialChar	SpecialChar
  HiLink helpTag		Tag
  HiLink helpDelimiter		Delimiter
  HiLink helpSpecialComment	SpecialComment
  HiLink helpDebug		Debug
  HiLink helpUnderlined		Underlined
  HiLink helpError		Error
  HiLink helpTodo		Todo

  delcommand HiLink
endif

let b:current_syntax = "help"

" vim: ts=8 sw=2
