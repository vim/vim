" Vim syntax file
" Language:		ESTEREL
" Maintainer:	Maurizio Tranchero <mtranchero@yahoo.it>
" Credits:		Luca Necchi	<luca.necchi@polito.it>
" Last Change:	Tue May 17 23:49:39 CEST 2005
" Version:		0.2

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" case is significant
syn case ignore
" Esterel Regions
syn region esterelModule		start=/module/	end=/end module/	contains=ALLBUT,esterelModule
syn region esterelLoop			start=/loop/	end=/end loop/		contains=ALLBUT,esterelModule
syn region esterelAbort			start=/abort/	end=/end abort/		contains=ALLBUT,esterelModule
syn region esterelEvery			start=/every/	end=/end every/		contains=ALLBUT,esterelModule
syn region esterelIf			start=/if/		end=/end if/		contains=ALLBUT,esterelModule
"syn region esterelConcurrent	start=/\[/		end=/\]/			contains=ALLBUT,esterelModule
syn region esterelConcurrent	transparent start=/\[/		end=/\]/		contains=ALLBUT,esterelModule
syn region esterelIfThen		start=/if/		end=/then/			oneline
" and weak abort? how to make vim know that start='weak abort'?
" Esterel Keywords
syn keyword esterelStatement	module signal end
syn keyword esterelIO			input output 
syn keyword esterelStatement	every do loop abort weak
syn keyword esterelStatement	emit present await
syn keyword esterelStatement	if then else
syn keyword esterelBoolean		and or not xor xnor nor nand
syn keyword esterelOperator		\[ \] 
syn keyword esterelPippo		pippo
" Esterel Types
syn keyword esterelType integer float bolean
" Esterel Comment
syn match esterelComment	"%.*$"
" Operators and special characters
syn match esterelSpecial	":"
syn match esterelSpecial	";"
syn match esterelOperator	"\["
syn match esterelOperator	"\]"
syn match esterelOperator	":="
syn match esterelStatement	"\<\(if\|else\)\>"
syn match esterelNone		"\<else\s\+if\>$"
syn match esterelNone		"\<else\s\+if\>\s"

" Class Linking
if version >= 508 || !exists("did_esterel_syntax_inits")
  if version < 508
    let did_esterel_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

	HiLink esterelStatement	Statement
	HiLink esterelType		Type
	HiLink esterelComment	Comment
	HiLink esterelBoolean	Number
	HiLink esterelIO		String
	HiLink esterelOperator	Type
	HiLink esterelSpecial	Special

  delcommand HiLink
endif

let b:current_syntax = "esterel"
