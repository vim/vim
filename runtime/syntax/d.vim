" Vim syntax file for the D programming language (version 1.053 and 2.039).
"
" Language:	D
" Maintainer:	Jason Mills<jasonmills@nf.sympatico.ca>
" Last Change:	2010 Jan 07
" Version:	0.18
"
" Contributors:
"   - Kirk McDonald: version 0.17 updates, with minor modifications
"     (http://paste.dprogramming.com/dplmb7qx?view=hidelines)
"   - Jesse K. Phillips: patch for some keywords and attributes (annotations), with modifications
"   - Tim Keating: patch to fix a bug in highlighting the `\` literal
"   - Frank Benoit: Fixed a bug that caused some identifiers and numbers to highlight as octal number errors.
"
" Please email me with bugs, comments, and suggestions.
"
" Options:
"   d_comment_strings - Set to highlight strings and numbers in comments.
"
"   d_hl_operator_overload - Set to highlight D's specially named functions
"   that when overloaded implement unary and binary operators (e.g. opCmp).
"
" Todo:
"   - Determine a better method of sync'ing than simply setting minlines
"   to a large number.
"
"   - Several keywords (e.g., in, out, inout) are both storage class and
"   statements, depending on their context. Perhaps use pattern matching to
"   figure out which and highlight appropriately. For now I have made such
"   keywords storage classes so their highlighting is consistent with other
"   keywords that are commonly used with them, but are true storage classes,
"   such as lazy. Similarly, I made some statement keywords (e.g. body) storage
"   classes.
"
"   - Mark contents of the asm statement body as special
"
"   - Maybe highlight the 'exit', 'failure', and 'success' parts of the
"   scope() statement.
"
"   - Highlighting DDoc comments.
"

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Keyword definitions
"
syn keyword dExternal		import package module extern
syn keyword dConditional	if else switch
syn keyword dBranch		goto break continue
syn keyword dRepeat		while for do foreach foreach_reverse
syn keyword dBoolean		true false
syn keyword dConstant		null
syn keyword dConstant		__FILE__ __LINE__ __EOF__ __VERSION__
syn keyword dConstant		__DATE__ __TIME__ __TIMESTAMP__ __VENDOR__

syn keyword dTypedef		alias typedef
syn keyword dStructure		template interface class struct union
syn keyword dEnum		enum
syn keyword dOperator		new delete typeof typeid cast align is
syn keyword dOperator		this super
if exists("d_hl_operator_overload")
  syn keyword dOpOverload	opNeg opCom opPostInc opPostDec opCast opAdd opSub opSub_r
  syn keyword dOpOverload	opMul opDiv opDiv_r opMod opMod_r opAnd opOr opXor
  syn keyword dOpOverload	opShl opShl_r opShr opShr_r opUShr opUShr_r opCat
  syn keyword dOpOverload	opCat_r opEquals opEquals opCmp
  syn keyword dOpOverload	opAssign opAddAssign opSubAssign opMulAssign opDivAssign
  syn keyword dOpOverload	opModAssign opAndAssign opOrAssign opXorAssign
  syn keyword dOpOverload	opShlAssign opShrAssign opUShrAssign opCatAssign
  syn keyword dOpOverload	opIndex opIndexAssign opCall opSlice opSliceAssign opPos
  syn keyword dOpOverload	opAdd_r opMul_r opAnd_r opOr_r opXor_r opIn opIn_r
  syn keyword dOpOverload	opPow opDispatch opStar opDot opApply opApplyReverse
endif
syn keyword dType		ushort int uint long ulong float
syn keyword dType		void byte ubyte double bit char wchar ucent cent
syn keyword dType		short bool dchar string wstring dstring
syn keyword dType		real ireal ifloat idouble creal cfloat cdouble
syn keyword dDebug		deprecated unittest
syn keyword dExceptions		throw try catch finally
syn keyword dScopeDecl		public protected private export
syn keyword dStatement		version debug return with
syn keyword dStatement		function delegate __traits asm mixin macro
syn keyword dStorageClass	in out inout ref lazy scope body
syn keyword dStorageClass	pure nothrow
syn keyword dStorageClass	auto static override final abstract volatile __gshared __thread
syn keyword dStorageClass	synchronized immutable shared const invariant lazy
syn keyword dPragma		pragma

" Attributes/annotations
syn match dAnnotation	"@[_$a-zA-Z][_$a-zA-Z0-9_]*\>"

" Assert is a statement and a module name.
syn match dAssert "^assert\>"
syn match dAssert "[^.]\s*\<assert\>"ms=s+1

" dTokens is used by the token string highlighting
syn cluster dTokens contains=dExternal,dConditional,dBranch,dRepeat,dBoolean
syn cluster dTokens add=dConstant,dTypedef,dStructure,dOperator,dOpOverload
syn cluster dTokens add=dType,dDebug,dExceptions,dScopeDecl,dStatement
syn cluster dTokens add=dStorageClass,dPragma,dAssert,dAnnotation

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
syn match dUserLabel    "^\s*[_$a-zA-Z][_$a-zA-Z0-9_]*\s*:"he=e-1 contains=dLabel,dScopeDecl,dEnum
syn keyword dLabel	case default

syn cluster dTokens add=dUserLabel,dLabel

" Comments
"
syn keyword dTodo	contained TODO FIXME TEMP REFACTOR REVIEW HACK BUG XXX
syn match dCommentStar	contained "^\s*\*[^/]"me=e-1
syn match dCommentStar	contained "^\s*\*$"
syn match dCommentPlus	contained "^\s*+[^/]"me=e-1
syn match dCommentPlus	contained "^\s*+$"
if exists("d_comment_strings")
  syn region dBlockCommentString	contained start=+"+ end=+"+ end=+\*/+me=s-1,he=s-1 contains=dCommentStar,dUnicode,dEscSequence,@Spell
  syn region dNestedCommentString	contained start=+"+ end=+"+ end="+"me=s-1,he=s-1 contains=dCommentPlus,dUnicode,dEscSequence,@Spell
  syn region dLineCommentString		contained start=+"+ end=+$\|"+ contains=dUnicode,dEscSequence,@Spell
  syn region dBlockComment	start="/\*"  end="\*/" contains=dBlockCommentString,dTodo,@Spell
  syn region dNestedComment	start="/+"  end="+/" contains=dNestedComment,dNestedCommentString,dTodo,@Spell
  syn match  dLineComment	"//.*" contains=dLineCommentString,dTodo,@Spell
else
  syn region dBlockComment	start="/\*"  end="\*/" contains=dBlockCommentString,dTodo,@Spell
  syn region dNestedComment	start="/+"  end="+/" contains=dNestedComment,dNestedCommentString,dTodo,@Spell
  syn match  dLineComment	"//.*" contains=dLineCommentString,dTodo,@Spell
endif

hi link dLineCommentString	dBlockCommentString
hi link dBlockCommentString	dString
hi link dNestedCommentString	dString
hi link dCommentStar		dBlockComment
hi link dCommentPlus		dNestedComment

syn cluster dTokens add=dBlockComment,dNestedComment,dLineComment

" /+ +/ style comments and strings that span multiple lines can cause
" problems. To play it safe, set minlines to a large number.
syn sync minlines=200
" Use ccomment for /* */ style comments
syn sync ccomment dBlockComment

" Characters
"
syn match dSpecialCharError contained "[^']"

" Escape sequences (oct,specal char,hex,wchar, character entities \&xxx;)
" These are not contained because they are considered string literals.
syn match dEscSequence	"\\\(\o\{1,3}\|[\"\\'\\?ntbrfva]\|u\x\{4}\|U\x\{8}\|x\x\x\)"
syn match dEscSequence	"\\&[^;& \t]\+;"
syn match dCharacter	"'[^']*'" contains=dEscSequence,dSpecialCharError
syn match dCharacter	"'\\''" contains=dEscSequence
syn match dCharacter	"'[^\\]'"

syn cluster dTokens add=dEscSequence,dCharacter

" Unicode characters
"
syn match dUnicode "\\u\d\{4\}"

" String.
"
syn region dString	start=+"+ end=+"[cwd]\=+ skip=+\\\\\|\\"+ contains=dEscSequence,@Spell
syn region dRawString	start=+`+ end=+`[cwd]\=+ contains=@Spell
syn region dRawString	start=+r"+ end=+"[cwd]\=+ contains=@Spell
syn region dHexString	start=+x"+ end=+"[cwd]\=+ contains=@Spell
syn region dDelimString	start=+q"\z(.\)+ end=+\z1"+ contains=@Spell
syn region dHereString	start=+q"\z(\I\i*\)\n+ end=+\n\z1"+ contains=@Spell

" Nesting delimited string contents
"
syn region dNestParenString start=+(+ end=+)+ contained transparent contains=dNestParenString,@Spell
syn region dNestBrackString start=+\[+ end=+\]+ contained transparent contains=dNestBrackString,@Spell
syn region dNestAngleString start=+<+ end=+>+ contained transparent contains=dNestAngleString,@Spell
syn region dNestCurlyString start=+{+ end=+}+ contained transparent contains=dNestCurlyString,@Spell

" Nesting delimited strings
"
syn region dParenString	matchgroup=dParenString start=+q"(+ end=+)"+ contains=dNestParenString,@Spell
syn region dBrackString	matchgroup=dBrackString start=+q"\[+ end=+\]"+ contains=dNestBrackString,@Spell
syn region dAngleString	matchgroup=dAngleString start=+q"<+ end=+>"+ contains=dNestAngleString,@Spell
syn region dCurlyString	matchgroup=dCurlyString start=+q"{+ end=+}"+ contains=dNestCurlyString,@Spell

hi link dParenString dNestString
hi link dBrackString dNestString
hi link dAngleString dNestString
hi link dCurlyString dNestString

syn cluster dTokens add=dString,dRawString,dHexString,dDelimString,dNestString

" Token strings
"
syn region dNestTokenString start=+{+ end=+}+ contained contains=dNestTokenString,@dTokens
syn region dTokenString matchgroup=dTokenStringBrack transparent start=+q{+ end=+}+ contains=dNestTokenString,@dTokens

syn cluster dTokens add=dTokenString

" Numbers
"
syn case ignore

syn match dDec		display "\<\d[0-9_]*\(u\=l\=\|l\=u\=\)\>"

" Hex number
syn match dHex		display "\<0x[0-9a-f_]\+\(u\=l\=\|l\=u\=\)\>"

syn match dOctal	display "\<0[0-7_]\+\(u\=l\=\|l\=u\=\)\>"
" flag an octal number with wrong digits
syn match dOctalError	display "\<0[0-7_]*[89][0-9_]*"

" binary numbers
syn match dBinary	display "\<0b[01_]\+\(u\=l\=\|l\=u\=\)\>"

"floating point without the dot
syn match dFloat	display "\<\d[0-9_]*\(fi\=\|l\=i\)\>"
"floating point number, with dot, optional exponent
syn match dFloat	display "\<\d[0-9_]*\.[0-9_]*\(e[-+]\=[0-9_]\+\)\=[fl]\=i\="
"floating point number, starting with a dot, optional exponent
syn match dFloat	display "\(\.[0-9_]\+\)\(e[-+]\=[0-9_]\+\)\=[fl]\=i\=\>"
"floating point number, without dot, with exponent
"syn match dFloat	display "\<\d\+e[-+]\=\d\+[fl]\=\>"
syn match dFloat	display "\<\d[0-9_]*e[-+]\=[0-9_]\+[fl]\=\>"

"floating point without the dot
syn match dHexFloat	display "\<0x[0-9a-f_]\+\(fi\=\|l\=i\)\>"
"floating point number, with dot, optional exponent
syn match dHexFloat	display "\<0x[0-9a-f_]\+\.[0-9a-f_]*\(p[-+]\=[0-9_]\+\)\=[fl]\=i\="
"floating point number, without dot, with exponent
syn match dHexFloat	display "\<0x[0-9a-f_]\+p[-+]\=[0-9_]\+[fl]\=i\=\>"

syn cluster dTokens add=dDec,dHex,dOctal,dOctalError,dBinary,dFloat,dHexFloat

syn case match

" Pragma (preprocessor) support
" TODO: Highlight following Integer and optional Filespec.
syn region  dPragma start="#\s*\(line\>\)" skip="\\$" end="$"


" The default highlighting.
"
hi def link dBinary		Number
hi def link dDec		Number
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
hi def link dTokenStringBrack	String
hi def link dHereString		String
hi def link dNestString		String
hi def link dDelimString	String
hi def link dRawString		String
hi def link dString		String
hi def link dHexString		String
hi def link dCharacter		Character
hi def link dEscSequence	SpecialChar
hi def link dSpecialCharError	Error
hi def link dOctalError		Error
hi def link dOperator		Operator
hi def link dOpOverload		Identifier
hi def link dConstant		Constant
hi def link dTypedef		Typedef
hi def link dEnum		Structure
hi def link dStructure		Structure
hi def link dTodo		Todo
hi def link dType		Type
hi def link dLineComment	Comment
hi def link dBlockComment	Comment
hi def link dNestedComment	Comment
hi def link dExternal		Include
hi def link dPragma		PreProc
hi def link dAnnotation		PreProc

let b:current_syntax = "d"

" vim: ts=8 noet
