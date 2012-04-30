" Vim syntax file
" Language:	    Objective C
" Maintainer:	    Kazunobu Kuriyama <kazunobu.kuriyama@nifty.com>
" Ex-maintainer:    Anthony Hodsdon <ahodsdon@fastmail.fm>
" First Author:	    Valentino Kyriakides <1kyriaki@informatik.uni-hamburg.de>
" Last Change:	    2012 Apr 30

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif
let s:keepcpo= &cpo
set cpo&vim

if &filetype != 'objcpp'
  " Read the C syntax to start with
  if version < 600
    source <sfile>:p:h/c.vim
  else
    runtime! syntax/c.vim
  endif
endif

" Objective C extentions follow below
"
" NOTE: Objective C is abbreviated to ObjC/objc
" and uses *.h, *.m as file extensions!


" ObjC keywords, types, type qualifiers etc.
syn keyword objcStatement	self super _cmd
syn keyword objcType		id Class SEL IMP BOOL
syn keyword objcTypeModifier	bycopy in out inout oneway
syn keyword objcConstant	nil Nil

" Match the ObjC #import directive (like C's #include)
syn region objcImported display contained start=+"+  skip=+\\\\\|\\"+  end=+"+
syn match  objcImported display contained "<[-_0-9a-zA-Z.\/]*>"
syn match  objcImport display "^\s*\(%:\|#\)\s*import\>\s*["<]" contains=objcImported

" Match the important ObjC directives
syn match  objcScopeDecl    "@public\|@private\|@protected"
syn match  objcDirective    "@interface\|@implementation"
syn match  objcDirective    "@class\|@end\|@defs"
syn match  objcDirective    "@encode\|@protocol\|@selector"
syn match  objcDirective    "@try\|@catch\|@finally\|@throw\|@synchronized"

" Match the ObjC method types
"
" NOTE: here I match only the indicators, this looks
" much nicer and reduces cluttering color highlightings.
" However, if you prefer full method declaration matching
" append .* at the end of the next two patterns!
"
syn match objcInstMethod    "^\s*-\s*"
syn match objcFactMethod    "^\s*+\s*"

" To distinguish from a header inclusion from a protocol list.
syn match objcProtocol display "<[_a-zA-Z][_a-zA-Z0-9]*>" contains=objcType,cType,Type


" To distinguish labels from the keyword for a method's parameter.
syn region objcKeyForMethodParam display
    \ start="^\s*[_a-zA-Z][_a-zA-Z0-9]*\s*:\s*("
    \ end=")\s*[_a-zA-Z][_a-zA-Z0-9]*"
    \ contains=objcType,objcTypeModifier,cType,cStructure,cStorageClass,Type

" Objective-C Constant Strings
syn match objcSpecial display "%@" contained
syn region objcString start=+\(@"\|"\)+ skip=+\\\\\|\\"+ end=+"+ contains=cFormat,cSpecial,objcSpecial

" Objective-C Message Expressions
syn region objcMessage display start="\[" end="\]" contains=objcMessage,objcStatement,objcType,objcTypeModifier,objcString,objcConstant,objcDirective,cType,cStructure,cStorageClass,cString,cCharacter,cSpecialCharacter,cNumbers,cConstant,cOperator,cComment,cCommentL,Type

syn cluster cParenGroup add=objcMessage
syn cluster cPreProcGroup add=objcMessage

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_objc_syntax_inits")
  if version < 508
    let did_objc_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink objcImport		Include
  HiLink objcImported		cString
  HiLink objcTypeModifier	objcType
  HiLink objcType		Type
  HiLink objcScopeDecl		Statement
  HiLink objcInstMethod		Function
  HiLink objcFactMethod		Function
  HiLink objcStatement		Statement
  HiLink objcDirective		Statement
  HiLink objcKeyForMethodParam	None
  HiLink objcString		cString
  HiLink objcSpecial		Special
  HiLink objcProtocol		None
  HiLink objcConstant		cConstant

  delcommand HiLink
endif

let b:current_syntax = "objc"

let &cpo = s:keepcpo
unlet s:keepcpo

" vim: ts=8
