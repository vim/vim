" Vim syntax file for the D programming language (version 1.053 and 2.047).
"
" Language:     D
" Maintainer:   Jesse Phillips <Jesse.K.Phillips+D@gmail.com>
" Last Change:  2010 Sep 21
" Version:      0.22
"
" Contributors:
"   - Jason Mills <jasonmills@nf.sympatico.ca>: original Maintainer
"   - Kirk McDonald: version 0.17 updates, with minor modifications
"     (http://paste.dprogramming.com/dplmb7qx?view=hidelines)
"   - Tim Keating: patch to fix a bug in highlighting the `\` literal
"   - Frank Benoit: Fixed a bug that caused some identifiers and numbers to highlight as octal number errors.
"   - Shougo Matsushita <Shougo.Matsu@gmail.com>: updates for latest 2.047 highlighting
"   - Ellery Newcomer: Fixed some highlighting bugs.
"   - Steven N. Oliver: #! highlighting
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
syn keyword dExternal              import package module extern
syn keyword dConditional           if else switch
syn keyword dBranch                goto break continue
syn keyword dRepeat                while for do foreach foreach_reverse
syn keyword dBoolean               true false
syn keyword dConstant              null
syn keyword dConstant              __FILE__ __LINE__ __EOF__ __VERSION__
syn keyword dConstant              __DATE__ __TIME__ __TIMESTAMP__ __VENDOR__
syn keyword dTypedef               alias typedef
syn keyword dStructure             template interface class struct union
syn keyword dEnum                  enum
syn keyword dOperator              new delete typeof typeid cast align is
syn keyword dOperator              this super
if exists("d_hl_operator_overload")
  syn keyword dOpOverload          opNeg opCom opPostInc opPostDec opCast opAdd
  syn keyword dOpOverload          opSub opSub_r opMul opDiv opDiv_r opMod 
  syn keyword dOpOverload          opMod_r opAnd opOr opXor opShl opShl_r opShr
  syn keyword dOpOverload          opShr_r opUShr opUShr_r opCat
  syn keyword dOpOverload          opCat_r opEquals opEquals opCmp
  syn keyword dOpOverload          opAssign opAddAssign opSubAssign opMulAssign
  syn keyword dOpOverload          opDivAssign opModAssign opAndAssign 
  syn keyword dOpOverload          opOrAssign opXorAssign opShlAssign 
  syn keyword dOpOverload          opShrAssign opUShrAssign opCatAssign
  syn keyword dOpOverload          opIndex opIndexAssign opIndexOpAssign
  syn keyword dOpOverload          opCall opSlice opSliceAssign opSliceOpAssign 
  syn keyword dOpOverload          opPos opAdd_r opMul_r opAnd_r opOr_r opXor_r
  syn keyword dOpOverload          opIn opIn_r opPow opDispatch opStar opDot 
  syn keyword dOpOverload          opApply opApplyReverse
  syn keyword dOpOverload          opUnary opIndexUnary opSliceUnary
  syn keyword dOpOverload          opBinary opBinaryRight
endif

syn keyword dType                  void ushort int uint long ulong float
syn keyword dType                  byte ubyte double bit char wchar ucent cent
syn keyword dType                  short bool dchar wstring dstring
syn keyword dType                  real ireal ifloat idouble
syn keyword dType                  creal cfloat cdouble
syn keyword dDebug                 deprecated unittest invariant
syn keyword dExceptions            throw try catch finally
syn keyword dScopeDecl             public protected private export
syn keyword dStatement             debug return with
syn keyword dStatement             function delegate __traits mixin macro
syn keyword dStorageClass          in out inout ref lazy body
syn keyword dStorageClass          pure nothrow
syn keyword dStorageClass          auto static override final abstract volatile
syn keyword dStorageClass          __gshared __thread
syn keyword dStorageClass          synchronized shared immutable const lazy
syn keyword dPragma                pragma
syn keyword dIdentifier            _arguments _argptr __vptr __monitor _ctor _dtor
syn keyword dScopeIdentifier       contained exit success failure
syn keyword dAttribute             contained safe trusted system
syn keyword dAttribute             contained property disable
syn keyword dVersionIdentifier     contained DigitalMars GNU LDC LLVM
syn keyword dVersionIdentifier     contained X86 X86_64 Windows Win32 Win64 
syn keyword dVersionIdentifier     contained linux Posix OSX FreeBSD
syn keyword dVersionIdentifier     contained LittleEndian BigEndian D_Coverage
syn keyword dVersionIdentifier     contained D_Ddoc D_InlineAsm_X86
syn keyword dVersionIdentifier     contained D_InlineAsm_X86_64 D_LP64 D_PIC
syn keyword dVersionIdentifier     contained unittest D_Version2 none all

" Highlight the sharpbang
syn match dSharpBang "\%^#!.*"     display

" Attributes/annotations
syn match dAnnotation	"@[_$a-zA-Z][_$a-zA-Z0-9_]*\>" contains=dAttribute

" Version Identifiers
syn match dVersion   "[^.]version" nextgroup=dVersionInside
syn match dVersion   "^version" nextgroup=dVersionInside
syn match dVersionInside  "([_a-zA-Z][_a-zA-Z0-9]*\>" transparent contained contains=dVersionIdentifier

" Scope StorageClass
syn match dStorageClass   "scope"

" Scope Identifiers
syn match dScope	"scope\s*([_a-zA-Z][_a-zA-Z0-9]*\>"he=s+5 contains=dScopeIdentifier

" String is a statement and a module name.
syn match dType "^string"
syn match dType "[^.]\s*\<string\>"ms=s+1

" Assert is a statement and a module name.
syn match dAssert "^assert"
syn match dAssert "[^.]\s*\<assert\>"ms=s+1

" dTokens is used by the token string highlighting
syn cluster dTokens contains=dExternal,dConditional,dBranch,dRepeat,dBoolean
syn cluster dTokens add=dConstant,dTypedef,dStructure,dOperator,dOpOverload
syn cluster dTokens add=dType,dDebug,dExceptions,dScopeDecl,dStatement
syn cluster dTokens add=dStorageClass,dPragma,dAssert,dAnnotation


" Labels
"
" We contain dScopeDecl so public: private: etc. are not highlighted like labels
syn match dUserLabel    "^\s*[_$a-zA-Z][_$a-zA-Z0-9_]*\s*:"he=e-1 contains=dLabel,dScopeDecl,dEnum
syn keyword dLabel      case default

syn cluster dTokens add=dUserLabel,dLabel

" Comments
"
syn keyword dTodo                                                                contained TODO FIXME TEMP REFACTOR REVIEW HACK BUG XXX
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
hi def link dBinary              Number
hi def link dDec                 Number
hi def link dHex                 Number
hi def link dOctal               Number
hi def link dFloat               Float
hi def link dHexFloat            Float
hi def link dDebug               Debug
hi def link dBranch              Conditional
hi def link dConditional         Conditional
hi def link dLabel               Label
hi def link dUserLabel           Label
hi def link dRepeat              Repeat
hi def link dExceptions          Exception
hi def link dAssert              Statement
hi def link dStatement           Statement
hi def link dScopeDecl           dStorageClass
hi def link dStorageClass        StorageClass
hi def link dBoolean             Boolean
hi def link dUnicode             Special
hi def link dTokenStringBrack    String
hi def link dHereString          String
hi def link dNestString          String
hi def link dDelimString         String
hi def link dRawString           String
hi def link dString              String
hi def link dHexString           String
hi def link dCharacter           Character
hi def link dEscSequence         SpecialChar
hi def link dSpecialCharError    Error
hi def link dOctalError          Error
hi def link dOperator            Operator
hi def link dOpOverload          Identifier
hi def link dConstant            Constant
hi def link dTypedef             Typedef
hi def link dEnum                Structure
hi def link dStructure           Structure
hi def link dTodo                Todo
hi def link dType                Type
hi def link dLineComment         Comment
hi def link dBlockComment        Comment
hi def link dNestedComment       Comment
hi def link dExternal            Include
hi def link dPragma              PreProc
hi def link dAnnotation          PreProc
hi def link dSharpBang           PreProc
hi def link dAttribute           StorageClass
hi def link dIdentifier          Identifier
hi def link dVersionIdentifier   Identifier
hi def link dVersion             dStatement
hi def link dScopeIdentifier     dStatement
hi def link dScope               dStorageClass

let b:current_syntax = "d"

" Marks contents of the asm statment body as special

syn match dAsmStatement "\<asm\>"
syn region dAsmBody start="asm[\n]*\s*{"hs=e+1 end="}"he=e-1 contains=dAsmStatement,dAsmOpCode

hi def link dAsmBody dUnicode
hi def link dAsmStatement dStatement
hi def link dAsmOpCode Identifier

syn keyword dAsmOpCode contained	aaa  	aad  	aam  	aas  	adc
syn keyword dAsmOpCode contained	add 	addpd 	addps 	addsd 	addss
syn keyword dAsmOpCode contained	and 	andnpd 	andnps 	andpd 	andps
syn keyword dAsmOpCode contained	arpl 	bound 	bsf 	bsr 	bswap
syn keyword dAsmOpCode contained	bt 	btc 	btr 	bts 	call
syn keyword dAsmOpCode contained	cbw 	cdq 	clc 	cld 	clflush
syn keyword dAsmOpCode contained	cli 	clts 	cmc 	cmova 	cmovae
syn keyword dAsmOpCode contained	cmovb 	cmovbe 	cmovc 	cmove 	cmovg
syn keyword dAsmOpCode contained	cmovge 	cmovl 	cmovle 	cmovna 	cmovnae
syn keyword dAsmOpCode contained	cmovnb 	cmovnbe 	cmovnc 	cmovne 	cmovng
syn keyword dAsmOpCode contained	cmovnge 	cmovnl 	cmovnle 	cmovno 	cmovnp
syn keyword dAsmOpCode contained	cmovns 	cmovnz 	cmovo 	cmovp 	cmovpe
syn keyword dAsmOpCode contained	cmovpo 	cmovs 	cmovz 	cmp 	cmppd
syn keyword dAsmOpCode contained	cmpps 	cmps 	cmpsb 	cmpsd 	cmpss
syn keyword dAsmOpCode contained	cmpsw 	cmpxch8b 	cmpxchg 	comisd 	comiss
syn keyword dAsmOpCode contained	cpuid 	cvtdq2pd 	cvtdq2ps 	cvtpd2dq 	cvtpd2pi
syn keyword dAsmOpCode contained	cvtpd2ps 	cvtpi2pd 	cvtpi2ps 	cvtps2dq 	cvtps2pd
syn keyword dAsmOpCode contained	cvtps2pi 	cvtsd2si 	cvtsd2ss 	cvtsi2sd 	cvtsi2ss
syn keyword dAsmOpCode contained	cvtss2sd 	cvtss2si 	cvttpd2dq 	cvttpd2pi 	cvttps2dq
syn keyword dAsmOpCode contained	cvttps2pi 	cvttsd2si 	cvttss2si 	cwd 	cwde
syn keyword dAsmOpCode contained	da 	daa 	das 	db 	dd
syn keyword dAsmOpCode contained	de 	dec 	df 	di 	div
syn keyword dAsmOpCode contained	divpd 	divps 	divsd 	divss 	dl
syn keyword dAsmOpCode contained	dq 	ds 	dt 	dw 	emms
syn keyword dAsmOpCode contained	enter 	f2xm1 	fabs 	fadd 	faddp
syn keyword dAsmOpCode contained	fbld 	fbstp 	fchs 	fclex 	fcmovb
syn keyword dAsmOpCode contained	fcmovbe 	fcmove 	fcmovnb 	fcmovnbe 	fcmovne
syn keyword dAsmOpCode contained	fcmovnu 	fcmovu 	fcom 	fcomi 	fcomip
syn keyword dAsmOpCode contained	fcomp 	fcompp 	fcos 	fdecstp 	fdisi
syn keyword dAsmOpCode contained	fdiv 	fdivp 	fdivr 	fdivrp 	feni
syn keyword dAsmOpCode contained	ffree 	fiadd 	ficom 	ficomp 	fidiv
syn keyword dAsmOpCode contained	fidivr 	fild 	fimul 	fincstp 	finit
syn keyword dAsmOpCode contained	fist 	fistp 	fisub 	fisubr 	fld
syn keyword dAsmOpCode contained	fld1 	fldcw 	fldenv 	fldl2e 	fldl2t
syn keyword dAsmOpCode contained	fldlg2 	fldln2 	fldpi 	fldz 	fmul
syn keyword dAsmOpCode contained	fmulp 	fnclex 	fndisi 	fneni 	fninit
syn keyword dAsmOpCode contained	fnop 	fnsave 	fnstcw 	fnstenv 	fnstsw
syn keyword dAsmOpCode contained	fpatan 	fprem 	fprem1 	fptan 	frndint
syn keyword dAsmOpCode contained	frstor 	fsave 	fscale 	fsetpm 	fsin
syn keyword dAsmOpCode contained	fsincos 	fsqrt 	fst 	fstcw 	fstenv
syn keyword dAsmOpCode contained	fstp 	fstsw 	fsub 	fsubp 	fsubr
syn keyword dAsmOpCode contained	fsubrp 	ftst 	fucom 	fucomi 	fucomip
syn keyword dAsmOpCode contained	fucomp 	fucompp 	fwait 	fxam 	fxch
syn keyword dAsmOpCode contained	fxrstor 	fxsave 	fxtract 	fyl2x 	fyl2xp1
syn keyword dAsmOpCode contained	hlt 	idiv 	imul 	in 	inc
syn keyword dAsmOpCode contained	ins 	insb 	insd 	insw 	int
syn keyword dAsmOpCode contained	into 	invd 	invlpg 	iret 	iretd
syn keyword dAsmOpCode contained	ja 	jae 	jb 	jbe 	jc
syn keyword dAsmOpCode contained	jcxz 	je 	jecxz 	jg 	jge
syn keyword dAsmOpCode contained	jl 	jle 	jmp 	jna 	jnae
syn keyword dAsmOpCode contained	jnb 	jnbe 	jnc 	jne 	jng
syn keyword dAsmOpCode contained	jnge 	jnl 	jnle 	jno 	jnp
syn keyword dAsmOpCode contained	jns 	jnz 	jo 	jp 	jpe
syn keyword dAsmOpCode contained	jpo 	js 	jz 	lahf 	lar
syn keyword dAsmOpCode contained	ldmxcsr 	lds 	lea 	leave 	les
syn keyword dAsmOpCode contained	lfence 	lfs 	lgdt 	lgs 	lidt
syn keyword dAsmOpCode contained	lldt 	lmsw 	lock 	lods 	lodsb
syn keyword dAsmOpCode contained	lodsd 	lodsw 	loop 	loope 	loopne
syn keyword dAsmOpCode contained	loopnz 	loopz 	lsl 	lss 	ltr
syn keyword dAsmOpCode contained	maskmovdqu 	maskmovq 	maxpd 	maxps 	maxsd
syn keyword dAsmOpCode contained	maxss 	mfence 	minpd 	minps 	minsd
syn keyword dAsmOpCode contained	minss 	mov 	movapd 	movaps 	movd
syn keyword dAsmOpCode contained	movdq2q 	movdqa 	movdqu 	movhlps 	movhpd
syn keyword dAsmOpCode contained	movhps 	movlhps 	movlpd 	movlps 	movmskpd
syn keyword dAsmOpCode contained	movmskps 	movntdq 	movnti 	movntpd 	movntps
syn keyword dAsmOpCode contained	movntq 	movq 	movq2dq 	movs 	movsb
syn keyword dAsmOpCode contained	movsd 	movss 	movsw 	movsx 	movupd
syn keyword dAsmOpCode contained	movups 	movzx 	mul 	mulpd 	mulps
syn keyword dAsmOpCode contained	mulsd 	mulss 	neg 	nop 	not
syn keyword dAsmOpCode contained	or 	orpd 	orps 	out 	outs
syn keyword dAsmOpCode contained	outsb 	outsd 	outsw 	packssdw 	packsswb
syn keyword dAsmOpCode contained	packuswb 	paddb 	paddd 	paddq 	paddsb
syn keyword dAsmOpCode contained	paddsw 	paddusb 	paddusw 	paddw 	pand
syn keyword dAsmOpCode contained	pandn 	pavgb 	pavgw 	pcmpeqb 	pcmpeqd
syn keyword dAsmOpCode contained	pcmpeqw 	pcmpgtb 	pcmpgtd 	pcmpgtw 	pextrw
syn keyword dAsmOpCode contained	pinsrw 	pmaddwd 	pmaxsw 	pmaxub 	pminsw
syn keyword dAsmOpCode contained	pminub 	pmovmskb 	pmulhuw 	pmulhw 	pmullw
syn keyword dAsmOpCode contained	pmuludq 	pop 	popa 	popad 	popf
syn keyword dAsmOpCode contained	popfd 	por 	prefetchnta 	prefetcht0 	prefetcht1
syn keyword dAsmOpCode contained	prefetcht2 	psadbw 	pshufd 	pshufhw 	pshuflw
syn keyword dAsmOpCode contained	pshufw 	pslld 	pslldq 	psllq 	psllw
syn keyword dAsmOpCode contained	psrad 	psraw 	psrld 	psrldq 	psrlq
syn keyword dAsmOpCode contained	psrlw 	psubb 	psubd 	psubq 	psubsb
syn keyword dAsmOpCode contained	psubsw 	psubusb 	psubusw 	psubw 	punpckhbw
syn keyword dAsmOpCode contained	punpckhdq 	punpckhqdq 	punpckhwd 	punpcklbw 	punpckldq
syn keyword dAsmOpCode contained	punpcklqdq 	punpcklwd 	push 	pusha 	pushad
syn keyword dAsmOpCode contained	pushf 	pushfd 	pxor 	rcl 	rcpps
syn keyword dAsmOpCode contained	rcpss 	rcr 	rdmsr 	rdpmc 	rdtsc
syn keyword dAsmOpCode contained	rep 	repe 	repne 	repnz 	repz
syn keyword dAsmOpCode contained	ret 	retf 	rol 	ror 	rsm
syn keyword dAsmOpCode contained	rsqrtps 	rsqrtss 	sahf 	sal 	sar
syn keyword dAsmOpCode contained	sbb 	scas 	scasb 	scasd 	scasw
syn keyword dAsmOpCode contained	seta 	setae 	setb 	setbe 	setc
syn keyword dAsmOpCode contained	sete 	setg 	setge 	setl 	setle
syn keyword dAsmOpCode contained	setna 	setnae 	setnb 	setnbe 	setnc
syn keyword dAsmOpCode contained	setne 	setng 	setnge 	setnl 	setnle
syn keyword dAsmOpCode contained	setno 	setnp 	setns 	setnz 	seto
syn keyword dAsmOpCode contained	setp 	setpe 	setpo 	sets 	setz
syn keyword dAsmOpCode contained	sfence 	sgdt 	shl 	shld 	shr
syn keyword dAsmOpCode contained	shrd 	shufpd 	shufps 	sidt 	sldt
syn keyword dAsmOpCode contained	smsw 	sqrtpd 	sqrtps 	sqrtsd 	sqrtss
syn keyword dAsmOpCode contained	stc 	std 	sti 	stmxcsr 	stos
syn keyword dAsmOpCode contained	stosb 	stosd 	stosw 	str 	sub
syn keyword dAsmOpCode contained	subpd 	subps 	subsd 	subss 	sysenter
syn keyword dAsmOpCode contained	sysexit 	test 	ucomisd 	ucomiss 	ud2
syn keyword dAsmOpCode contained	unpckhpd 	unpckhps 	unpcklpd 	unpcklps 	verr
syn keyword dAsmOpCode contained	verw 	wait 	wbinvd 	wrmsr 	xadd
syn keyword dAsmOpCode contained	xchg 	xlat 	xlatb 	xor 	xorpd
syn keyword dAsmOpCode contained	xorps 				
syn keyword dAsmOpCode contained	addsubpd 	addsubps 	fisttp 	haddpd 	haddps
syn keyword dAsmOpCode contained	hsubpd 	hsubps 	lddqu 	monitor 	movddup
syn keyword dAsmOpCode contained	movshdup 	movsldup 	mwait 		
syn keyword dAsmOpCode contained	pavgusb 	pf2id 	pfacc 	pfadd 	pfcmpeq
syn keyword dAsmOpCode contained	pfcmpge 	pfcmpgt 	pfmax 	pfmin 	pfmul
syn keyword dAsmOpCode contained	pfnacc 	pfpnacc 	pfrcp 	pfrcpit1 	pfrcpit2
syn keyword dAsmOpCode contained	pfrsqit1 	pfrsqrt 	pfsub 	pfsubr 	pi2fd
syn keyword dAsmOpCode contained	pmulhrw 	pswapd

