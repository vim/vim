" Vim syntax file for the D programming language (version 0.90).
"
" Language:     D
" Maintainer:   Jason Mills<jmills@cs.mun.ca>
" URL:
" Last Change:  2004 May 21
" Version:      0.8
"
" Options:
"   d_comment_strings - set to highlight strings and numbers in comments
"
"   d_hl_operator_overload - set to highlight D's specially named functions
"   that when overloaded implement unary and binary operators (e.g. cmp).
"
" Todo:
"   - Allow user to set sync minlines
"
"   - Several keywords (namely, in and out) are both storage class and
"   statements, depending on their context. Must use some matching to figure
"   out which and highlight appropriately. For now I have made such keywords
"   statements.
"
"   - Mark contents of the asm statement body as special
"

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Keyword definitions
"
syn keyword dExternal	     import module extern
syn keyword dConditional     if else switch
syn keyword dBranch	     goto break continue
syn keyword dRepeat	     while for do foreach
syn keyword dBoolean	     true false
syn keyword dConstant	     null
syn keyword dTypedef	     alias typedef
syn keyword dStructure	     template interface class enum struct union
syn keyword dOperator	     new delete typeof cast align is
syn keyword dOperator	     this super
if exists("d_hl_operator_overload")
  syn keyword dOpOverload  opNeg opCom opPostInc opPostDec opAdd opSub opSub_r
  syn keyword dOpOverload  opMul opDiv opDiv_r opMod opMod_r opAnd opOr opXor
  syn keyword dOpOverload  opShl opShl_r opShr opShr_r opUShr opUShr_r opCat
  syn keyword dOpOverload  opCat_r opEquals opEquals opCmp opCmp opCmp opCmp
  syn keyword dOpOverload  opAddAssign opSubAssign opMulAssign opDivAssign
  syn keyword dOpOverload  opModAssign opAndAssign opOrAssign opXorAssign
  syn keyword dOpOverload  opShlAssign opShrAssign opUShrAssign opCatAssign
  syn keyword dOpOverload  opIndex opCall opSlice
endif
syn keyword dType	     ushort int uint long ulong float
syn keyword dType	     void byte ubyte double bit char wchar ucent cent
syn keyword dType	     short bool dchar
syn keyword dType	     real ireal ifloat idouble creal cfloat cdouble
syn keyword dDebug	     deprecated unittest
syn keyword dExceptions      throw try catch finally
syn keyword dScopeDecl       public protected private export
syn keyword dStatement       version debug return with invariant body
syn keyword dStatement       in out inout asm mixin
syn keyword dStatement       function delegate
syn keyword dStorageClass    auto static override final const abstract volatile
syn keyword dStorageClass    synchronized
syn keyword dPragma	     pragma


" Assert is a statement and a module name.
syn match dAssert "^assert\>"
syn match dAssert "[^.]\s*\<assert\>"ms=s+1

" Marks contents of the asm statment body as special
"
" TODO
"syn match dAsmStatement "\<asm\>"
"syn region dAsmBody start="asm[\n]*\s*{"hs=e+1 end="}"he=e-1 contains=dAsmStatement
"
"hi def link dAsmBody dUnicode
"hi def link dAsmStatement dStatement

" Labels
"
" We contain dScopeDecl so public: private: etc. are not highlighted like labels
syn match   dUserLabel       "^\s*[_$a-zA-Z][_$a-zA-Z0-9_]*\s*:"he=e-1 contains=dLabel,dScopeDecl
syn keyword dLabel	     case default

" Comments
"
syn keyword dTodo	      contained TODO FIXME TEMP XXX
syn match   dCommentStar      contained "^\s*\*[^/]"me=e-1
syn match   dCommentStar      contained "^\s*\*$"
syn match   dCommentPlus      contained "^\s*+[^/]"me=e-1
syn match   dCommentPlus      contained "^\s*+$"
if exists("d_comment_strings")
  syn region  dBlockCommentString   contained  start=+"+ end=+"+ end=+\*/+me=s-1,he=s-1 contains=dCommentStar,dUnicode,dEscSequence,@Spell
  syn region  dNestedCommentString  contained  start=+"+ end=+"+ end="+"me=s-1,he=s-1 contains=dCommentPlus,dUnicode,dEscSequence,@Spell
  syn region  dLineCommentString    contained start=+"+  end=+$\|"+ contains=dUnicode,dEscSequence,@Spell
  syn region  dBlockComment     start="/\*"  end="\*/" contains=dBlockCommentString,dTodo,@Spell
  syn region  dNestedComment    start="/+"  end="+/" contains=dNestedComment,dNestedCommentString,dTodo,@Spell
  syn match   dLineComment      "//.*" contains=dLineCommentString,dTodo,@Spell
else
  syn region  dBlockComment     start="/\*"  end="\*/" contains=dBlockCommentString,dTodo,@Spell
  syn region  dNestedComment    start="/+"  end="+/" contains=dNestedComment,dNestedCommentString,dTodo,@Spell
  syn match   dLineComment      "//.*" contains=dLineCommentString,dTodo,@Spell
endif

hi link dLineCommentString dBlockCommentString
hi link dBlockCommentString dString
hi link dNestedCommentString dString
hi link dCommentStar  dBlockComment
hi link dCommentPlus  dNestedComment

syn sync minlines=25

" Characters
"
syn match dSpecialCharError contained "[^']"

" Escape sequences (oct,specal char,hex,wchar). These are not contained
" because they are considered string litterals
syn match dEscSequence "\\\(\o\{1,3}\|[\"\\'\\?ntbrfva]\|u\x\{4}\|U\x\{8}\|x\x\x\)"
syn match dCharacter  "'[^']*'" contains=dEscSequence,dSpecialCharError
syn match dCharacter  "'\\''" contains=dEscSequence
syn match dCharacter  "'[^\\]'"

" Unicode characters
"
syn match   dUnicode "\\u\d\{4\}"

" String.
"
syn region  dString start=+"+ end=+"+ contains=dEscSequence,@Spell
syn region  dRawString start=+`+ skip=+\\`+ end=+`+ contains=@Spell
syn region  dRawString start=+r"+ skip=+\\"+ end=+"+ contains=@Spell
syn region  dHexString start=+x"+ skip=+\\"+ end=+"+

" Numbers
"
syn case ignore
syn match dInt        display "\<\d[0-9_]*\(u\=l\=\|l\=u\=\)\>"
" Hex number
syn match dHex        display "\<0x[0-9a-f_]\+\(u\=l\=\|l\=u\=\)\>"
syn match dHex        display "\<\x[0-9a-f_]*h\(u\=l\=\|l\=u\=\)\>"
" Flag the first zero of an octal number as something special
syn match dOctal      display "\<0[0-7_]\+\(u\=l\=\|l\=u\=\)\>" contains=cOctalZero
syn match dOctalZero  display contained "\<0"

"floating point without the dot
syn match dFloat      display "\<\d[0-9_]*\(fi\=\|l\=i\)\>"
"floating point number, with dot, optional exponent
syn match dFloat      display "\<\d[0-9_]*\.[0-9_]*\(e[-+]\=[0-9_]\+\)\=[fl]\=i\="
"floating point number, starting with a dot, optional exponent
syn match dFloat      display "\(\.[0-9_]\+\)\(e[-+]\=[0-9_]\+\)\=[fl]\=i\=\>"
"floating point number, without dot, with exponent
"syn match dFloat      display "\<\d\+e[-+]\=\d\+[fl]\=\>"
syn match dFloat      display "\<\d[0-9_]*e[-+]\=[0-9_]\+[fl]\=\>"

"floating point without the dot
syn match dHexFloat      display "\<0x\x\+\(fi\=\|l\=i\)\>"
"floating point number, with dot, optional exponent
syn match dHexFloat      display "\<0x\x\+\.\x*\(p[-+]\=\x\+\)\=[fl]\=i\="
"floating point number, without dot, with exponent
syn match dHexFloat      display "\<0x\x\+p[-+]\=\x\+[fl]\=\>"

" binary numbers
syn match dBinary     display "\<0b[01_]\+\>"
" flag an octal number with wrong digits
syn match dOctalError display "0\o*[89]\d*"
syn case match

" Pragma (preprocessor) support
" TODO: Highlight following Integer and optional Filespec.
syn region  dPragma start="#\s*\(line\>\)" skip="\\$" end="$"


" The default highlighting.
"
hi def link dBinary		Number
hi def link dInt		Number
hi def link dHex		Number
hi def link dOctal		Number
hi def link dFloat		Float
hi def link dHexFloat		Float
hi def link dDebug		Debug
hi def link dBranch		Conditional
hi def link dConditional	Conditional
hi def link dLabel		Label
hi def link dUserLabel		Label
hi def link dRepeat		Repeat
hi def link dExceptions		Exception
hi def link dAssert		Statement
hi def link dStatement		Statement
hi def link dScopeDecl		dStorageClass
hi def link dStorageClass	StorageClass
hi def link dBoolean		Boolean
hi def link dUnicode		Special
hi def link dRawString		String
hi def link dString		String
hi def link dHexString		String
hi def link dCharacter		Character
hi def link dEscSequence	SpecialChar
hi def link dSpecialCharError	Error
hi def link dOctalError		Error
hi def link dOperator		Operator
hi def link dOpOverload		Operator
hi def link dConstant		Constant
hi def link dTypedef		Typedef
hi def link dStructure		Structure
hi def link dTodo		Todo
hi def link dType		Type
hi def link dLineComment	Comment
hi def link dBlockComment	Comment
hi def link dNestedComment	Comment
hi def link dExternal		Include
hi def link dPragma		PreProc

let b:current_syntax = "d"

" vim: ts=8
