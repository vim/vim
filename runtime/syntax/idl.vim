" Vim syntax file
" Language:	IDL (Interface Description Language)
" Maintainer:	Jody Goldberg <jgoldberg@home.com>
" Last Change:	2001 May 09

" This is an experiment.  IDL's structure is simple enough to permit a full
" grammar based approach to rather than using a few heuristics.  The result
" is large and somewhat repetative but seems to work.

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Misc basic
syn match	idlId		contained "[a-zA-Z][a-zA-Z0-9_]*"
syn match	idlSemiColon	contained ";"
syn match	idlCommaArg	contained ","			skipempty skipwhite nextgroup=idlSimpDecl
syn region	idlArraySize1	contained start=:\[: end=:\]:	skipempty skipwhite nextgroup=idlArraySize1,idlSemiColon,idlCommaArg contains=idlArraySize1,idlLiteral
syn match   idlSimpDecl	 contained "[a-zA-Z][a-zA-Z0-9_]*"	skipempty skipwhite nextgroup=idlSemiColon,idlCommaArg,idlArraySize1
syn region  idlSting	 contained start=+"+  skip=+\\\(\\\\\)*"+  end=+"+
syn match   idlLiteral	 contained "[1-9]\d*\(\.\d*\)\="
syn match   idlLiteral	 contained "\.\d\+"
syn keyword idlLiteral	 contained TRUE FALSE

" Comments
syn keyword idlTodo contained	TODO FIXME XXX
syn region idlComment		start="/\*"  end="\*/" contains=idlTodo
syn match  idlComment		"//.*" contains=idlTodo
syn match  idlCommentError	"\*/"

" C style Preprocessor
syn region idlIncluded contained start=+"+  skip=+\\\(\\\\\)*"+  end=+"+
syn match  idlIncluded contained "<[^>]*>"
syn match  idlInclude		"^[ \t]*#[ \t]*include\>[ \t]*["<]" contains=idlIncluded,idlString
syn region idlPreCondit	start="^[ \t]*#[ \t]*\(if\>\|ifdef\>\|ifndef\>\|elif\>\|else\>\|endif\>\)"  skip="\\$"  end="$" contains=idlComment,idlCommentError
syn region idlDefine	start="^[ \t]*#[ \t]*\(define\>\|undef\>\)" skip="\\$" end="$" contains=idlLiteral, idlString

" Constants
syn keyword idlConst	const	skipempty skipwhite nextgroup=idlBaseType,idlBaseTypeInt

" Attribute
syn keyword idlROAttr	readonly	skipempty skipwhite nextgroup=idlAttr
syn keyword idlAttr	attribute	skipempty skipwhite nextgroup=idlBaseTypeInt,idlBaseType

" Types
syn region  idlD4	contained start="<" end=">"	skipempty skipwhite nextgroup=idlSimpDecl	contains=idlSeqType,idlBaseTypeInt,idlBaseType,idlLiteral
syn keyword idlSeqType	contained sequence		skipempty skipwhite nextgroup=idlD4
syn keyword idlBaseType		contained	float double char boolean octet any	skipempty skipwhite nextgroup=idlSimpDecl
syn keyword idlBaseTypeInt	contained	short long		skipempty skipwhite nextgroup=idlSimpDecl
syn keyword idlBaseType		contained	unsigned		skipempty skipwhite nextgroup=idlBaseTypeInt
syn region  idlD1		contained	start="<" end=">"	skipempty skipwhite nextgroup=idlSimpDecl	contains=idlString,idlLiteral
syn keyword idlBaseType		contained	string	skipempty skipwhite nextgroup=idlD1,idlSimpDecl
syn match   idlBaseType		contained	"[a-zA-Z0-9_]\+[ \t]*\(::[ \t]*[a-zA-Z0-9_]\+\)*"	skipempty skipwhite nextgroup=idlSimpDecl

" Modules
syn region  idlModuleContent contained start="{" end="}"	skipempty skipwhite nextgroup=idlSemiColon contains=idlUnion,idlStruct,idlEnum,idlInterface,idlComment,idlTypedef,idlConst,idlException,idlModule
syn match   idlModuleName contained	"[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlModuleContent,idlSemiColon
syn keyword idlModule			module			skipempty skipwhite nextgroup=idlModuleName

" Interfaces
syn region  idlInterfaceContent contained start="{" end="}"	skipempty skipwhite nextgroup=idlSemiColon contains=idlUnion,idlStruct,idlEnum,idlComment,idlROAttr,idlAttr,idlOp,idlOneWayOp,idlException,idlConst,idlTypedef
syn match   idlInheritFrom2 contained "," skipempty skipwhite nextgroup=idlInheritFrom
syn match idlInheritFrom contained "[a-zA-Z0-9_]\+[ \t]*\(::[ \t]*[a-zA-Z0-9_]\+\)*" skipempty skipwhite nextgroup=idlInheritFrom2,idlInterfaceContent
syn match idlInherit contained	":"		skipempty skipwhite nextgroup=idlInheritFrom
syn match   idlInterfaceName contained	"[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlInterfaceContent,idlInherit,idlSemiColon
syn keyword idlInterface		interface		skipempty skipwhite nextgroup=idlInterfaceName


" Raises
syn keyword idlRaises	contained raises	skipempty skipwhite nextgroup=idlRaises,idlContext,idlSemiColon

" Context
syn keyword idlContext	contained context	skipempty skipwhite nextgroup=idlRaises,idlContext,idlSemiColon

" Operation
syn match   idlParmList	contained "," skipempty skipwhite nextgroup=idlOpParms
syn region  idlArraySize contained start="\[" end="\]"	skipempty skipwhite nextgroup=idlArraySize,idlParmList contains=idlArraySize,idlLiteral
syn match   idlParmName contained "[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlParmList,idlArraySize
syn keyword idlParmInt	contained short long		skipempty skipwhite nextgroup=idlParmName
syn keyword idlParmType	contained unsigned		skipempty skipwhite nextgroup=idlParmInt
syn region  idlD3	contained start="<" end=">"	skipempty skipwhite nextgroup=idlParmName	contains=idlString,idlLiteral
syn keyword idlParmType	contained string		skipempty skipwhite nextgroup=idlD3,idlParmName
syn keyword idlParmType	contained void float double char boolean octet any	  skipempty skipwhite nextgroup=idlParmName
syn match   idlParmType	contained "[a-zA-Z0-9_]\+[ \t]*\(::[ \t]*[a-zA-Z0-9_]\+\)*" skipempty skipwhite nextgroup=idlParmName
syn keyword idlOpParms	contained in out inout		skipempty skipwhite nextgroup=idlParmType

syn region idlOpContents contained start="(" end=")"	skipempty skipwhite nextgroup=idlRaises,idlContext,idlSemiColon contains=idlOpParms
syn match   idlOpName   contained "[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlOpContents
syn keyword idlOpInt	contained short long		skipempty skipwhite nextgroup=idlOpName
syn region  idlD2	contained start="<" end=">"	skipempty skipwhite nextgroup=idlOpName	contains=idlString,idlLiteral
syn keyword idlOp	contained unsigned		skipempty skipwhite nextgroup=idlOpInt
syn keyword idlOp	contained string		skipempty skipwhite nextgroup=idlD2,idlOpName
syn keyword idlOp	contained void float double char boolean octet any		skipempty skipwhite nextgroup=idlOpName
syn match   idlOp	contained "[a-zA-Z0-9_]\+[ \t]*\(::[ \t]*[a-zA-Z0-9_]\+\)*"	skipempty skipwhite nextgroup=idlOpName
syn keyword idlOp	contained void			skipempty skipwhite nextgroup=idlOpName
syn keyword idlOneWayOp	contained oneway		skipempty skipwhite nextgroup=idOp

" Enum
syn region  idlEnumContents contained start="{" end="}"		skipempty skipwhite nextgroup=idlSemiColon, idlSimpDecl contains=idlId,idlComment
syn match   idlEnumName contained	"[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlEnumContents
syn keyword idlEnum			enum			skipempty skipwhite nextgroup=idlEnumName

" Typedef
syn keyword idlTypedef			typedef			skipempty skipwhite nextgroup=idlBaseType, idlBaseTypeInt, idlSeqType

" Struct
syn region  idlStructContent contained start="{" end="}" skipempty skipwhite nextgroup=idlSemiColon, idlSimpDecl	contains=idlBaseType, idlBaseTypeInt, idlSeqType,idlComment, idlEnum, idlUnion
syn match   idlStructName contained	"[a-zA-Z0-9_]\+" skipempty skipwhite nextgroup=idlStructContent
syn keyword idlStruct			struct		 skipempty skipwhite nextgroup=idlStructName

" Exception
syn keyword idlException exception skipempty skipwhite nextgroup=idlStructName

" Union
syn match   idlColon contained ":"	skipempty skipwhite nextgroup=idlCase,idlSeqType,idlBaseType,idlBaseTypeInt
syn region  idlCaseLabel contained start="" skip="::" end=":"me=e-1	skipempty skipwhite nextgroup=idlColon contains=idlLiteral,idlString
syn keyword idlCase		contained case				skipempty skipwhite nextgroup=idlCaseLabel
syn keyword idlCase		contained default			skipempty skipwhite nextgroup=idlColon
syn region  idlUnionContent	contained start="{" end="}"		skipempty skipwhite nextgroup=idlSemiColon,idlSimpDecl	contains=idlCase
syn region  idlSwitchType	contained start="(" end=")"		skipempty skipwhite nextgroup=idlUnionContent
syn keyword idlUnionSwitch	contained switch			skipempty skipwhite nextgroup=idlSwitchType
syn match   idlUnionName	contained "[a-zA-Z0-9_]\+"		skipempty skipwhite nextgroup=idlUnionSwitch
syn keyword idlUnion		union				skipempty skipwhite nextgroup=idlUnionName

syn sync lines=200

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_idl_syntax_inits")
  if version < 508
    let did_idl_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink idlInclude		Include
  HiLink idlPreProc		PreProc
  HiLink idlPreCondit		PreCondit
  HiLink idlDefine		Macro
  HiLink idlIncluded		String
  HiLink idlString		String
  HiLink idlComment		Comment
  HiLink idlTodo		Todo
  HiLink idlLiteral		Number

  HiLink idlModule		Keyword
  HiLink idlInterface		Keyword
  HiLink idlEnum		Keyword
  HiLink idlStruct		Keyword
  HiLink idlUnion		Keyword
  HiLink idlTypedef		Keyword
  HiLink idlException		Keyword

  HiLink idlModuleName		Typedef
  HiLink idlInterfaceName	Typedef
  HiLink idlEnumName		Typedef
  HiLink idlStructName		Typedef
  HiLink idlUnionName		Typedef

  HiLink idlBaseTypeInt		idlType
  HiLink idlBaseType		idlType
  HiLink idlSeqType		idlType
  HiLink idlD1			Paren
  HiLink idlD2			Paren
  HiLink idlD3			Paren
  HiLink idlD4			Paren
  "HiLink idlArraySize		Paren
  "HiLink idlArraySize1		Paren
  HiLink idlModuleContent	Paren
  HiLink idlUnionContent	Paren
  HiLink idlStructContent	Paren
  HiLink idlEnumContents	Paren
  HiLink idlInterfaceContent	Paren

  HiLink idlSimpDecl		Identifier
  HiLink idlROAttr		StorageClass
  HiLink idlAttr		Keyword
  HiLink idlConst		StorageClass

  HiLink idlOneWayOp		StorageClass
  HiLink idlOp			idlType
  HiLink idlParmType		idlType
  HiLink idlOpName		Function
  HiLink idlOpParms		StorageClass
  HiLink idlParmName		Identifier
  HiLink idlInheritFrom		Identifier

  HiLink idlId			Constant
  "HiLink idlCase		Keyword
  HiLink idlCaseLabel		Constant

  delcommand HiLink
endif

let b:current_syntax = "idl"

" vim: ts=8
