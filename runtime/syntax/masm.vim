" Vim syntax file
" Language:	Microsoft Assembler (80x86)
" Maintainer:	Rob Brady <robb@datatone.com>
" Last Change:	$Date$
" URL: http://www.datatone.com/~robb/vim/syntax/masm.vim
" $Revision$

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case ignore


" syn match masmType "\.word"

syn match masmIdentifier	"[a-z_$][a-z0-9_$]*"
syn match masmLabel		"^[A-Z_$][A-Z0-9_$]*:"he=e-1

syn match masmDecimal		"\d*"
syn match masmBinary		"[0-1]\+b"  "put this before hex or 0bfh dies!
syn match masmHexadecimal	"[0-9]\x*h"
syn match masmFloat		"[0-9]\x*r"

syn match masmComment		";.*"
syn region masmString		start=+'+ end=+'+

syn keyword masmOperator	AND BYTE PTR CODEPTR DATAPTR DUP DWORD EQ FAR
syn keyword masmOperator	FWORD GE GT HIGH LARGE LE LOW LT MOD NE NEAR
syn keyword masmOperator	NOT OFFSET OR PROC PWORD QWORD SEG SHORT TBYTE
syn keyword masmOperator	TYPE WORD PARA
syn keyword masmDirective	ALIGN ARG ASSUME CODESEG COMM
syn keyword masmDirective	CONST DATASEG DB DD DF DISPLAY DOSSEG DP
syn keyword masmDirective	DQ DT DW ELSE ELSEIF EMUL END ENDIF ENDM ENDP
syn keyword masmDirective	ENDS ENUM EQU PROC PUBLIC PUBLICDLL RADIX
syn keyword masmDirective	EXTRN FARDATA GLOBAL RECORD SEGMENT SMALLSTACK
syn keyword masmDirective	GROUP IF IF1 IF2 IFB IFDEF IFDIF IFDIFI
syn keyword masmDirective	IFE IFIDN IFIDNI IFNB IFNDEF INCLUDE INCLUDLIB
syn keyword masmDirective	LABEL LARGESTACK STACK STRUC SUBTTL TITLE
syn keyword masmDirective	MODEL NAME NOEMUL UNION USES VERSION
syn keyword masmDirective	ORG FLAT
syn match   masmDirective	"\.model"
syn match   masmDirective	"\.186"
syn match   masmDirective	"\.286"
syn match   masmDirective	"\.286c"
syn match   masmDirective	"\.286p"
syn match   masmDirective	"\.287"
syn match   masmDirective	"\.386"
syn match   masmDirective	"\.386c"
syn match   masmDirective	"\.386p"
syn match   masmDirective	"\.387"
syn match   masmDirective	"\.486"
syn match   masmDirective	"\.486c"
syn match   masmDirective	"\.486p"
syn match   masmDirective	"\.8086"
syn match   masmDirective	"\.8087"
syn match   masmDirective	"\.ALPHA"
syn match   masmDirective	"\.CODE"
syn match   masmDirective	"\.DATA"

syn keyword masmRegister	AX BX CX DX SI DI BP SP
syn keyword masmRegister	ES DS SS CS
syn keyword masmRegister	AH BH CH DH AL BL CL DL
syn keyword masmRegister	EAX EBX ECX EDX ESI EDI EBP ESP


" these are current as of the 486 - don't have any pentium manuals handy
syn keyword masmOpcode		AAA AAD AAM AAS ADC ADD AND ARPL BOUND BSF
syn keyword masmOpcode		BSR BSWAP BT BTC BTR BTS BSWAP BT BTC BTR
syn keyword masmOpcode		BTS CALL CBW CDQ CLC CLD CLI CLTS CMC CMP
syn keyword masmOpcode		CMPS CMPSB CMPSW CMPSD CMPXCHG CWD CWDE DAA
syn keyword masmOpcode		DAS DEC DIV ENTER HLT IDIV IMUL IN INC INS
syn keyword masmOpcode		INSB INSW INSD INT INTO INVD INVLPG IRET
syn keyword masmOpcode		IRETD JA JAE JB JBE JC JCXZ JECXZ JE JZ JG
syn keyword masmOpcode		JGE JL JLE JNA JNAE JNB JNBE JNC JNE JNG JNGE
syn keyword masmOpcode		JNL JNLE JNO JNP JNS JNZ JO JP JPE JPO JS JZ
syn keyword masmOpcode		JMP LAHF LAR LEA LEAVE LGDT LIDT LGS LSS LFS
syn keyword masmOpcode		LODS LODSB LODSW LODSD LOOP LOOPE LOOPZ LOONE
syn keyword masmOpcode		LOOPNE RETF RETN
syn keyword masmOpcode		LDS LES LLDT LMSW LOCK LSL LTR MOV MOVS MOVSB
syn keyword masmOpcode		MOVSW MOVSD MOVSX MOVZX MUL NEG NOP NOT OR
syn keyword masmOpcode		OUT OUTS OUTSB OUTSW OUTSD POP POPA POPD
syn keyword masmOpcode		POPF POPFD PUSH PUSHA PUSHAD PUSHF PUSHFD
syn keyword masmOpcode		RCL RCR ROL ROR REP REPE REPZ REPNE REPNZ
syn keyword masmOpcode		RET SAHF SAL SAR SHL SHR SBB SCAS SCASB
syn keyword masmOpcode		SCASW SCASD SETA SETAE SETB SETBE SETC SETE
syn keyword masmOpcode		SETG SETGE SETL SETLE SETNA SETNAE SETNB
syn keyword masmOpcode		SETNBE SETNC SETNE SETNG SETNGE SETNL SETNLE
syn keyword masmOpcode		SETNO SETNP SETNS SETNZ SETO SETP SETPE SETPO
syn keyword masmOpcode		SETS SETZ SGDT SIDT SHLD SHRD SLDT SMSW STC
syn keyword masmOpcode		STD STI STOS STOSB STOSW STOSD STR SUB TEST
syn keyword masmOpcode		VERR VERW WAIT WBINVD XADD XCHG XLAT XLATB XOR

" floating point coprocessor as of 487
syn keyword masmOpFloat		F2XM1 FABS FADD FADDP FBLD FBSTP FCHS FCLEX
syn keyword masmOpFloat		FNCLEX FCOM FCOMP FCOMPP FCOS FDECSTP FDISI
syn keyword masmOpFloat		FNDISI FDIV FDIVP FDIVR FDIVRP FENI FNENI
syn keyword masmOpFloat		FFREE FIADD FICOM FICOMP FIDIV FIDIVR FILD
syn keyword masmOpFloat		FIMUL FINCSTP FINIT FNINIT FIST FISTP FISUB
syn keyword masmOpFloat		FISUBR FLD FLDCW FLDENV FLDLG2 FLDLN2 FLDL2E
syn keyword masmOpFloat		FLDL2T FLDPI FLDZ FLD1 FMUL FMULP FNOP FPATAN
syn keyword masmOpFloat		FPREM FPREM1 FPTAN FRNDINT FRSTOR FSAVE
syn keyword masmOpFloat		FNSAVE FSCALE FSETPM FSIN FSINCOS FSQRT FST
syn keyword masmOpFloat		FSTCW FNSTCW FSTENV FNSTENV FSTP FSTSW FNSTSW
syn keyword masmOpFloat		FSUB FSUBP FSUBR FSUBRP FTST FUCOM FUCOMP
syn keyword masmOpFloat		FUCOMPP FWAIT FXAM FXCH FXTRACT FYL2X FYL2XP1
syn match   masmOpFloat		"FSTSW[ \t]\+AX"
syn match   masmOpFloat		"FNSTSW[ \t]\+AX"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_masm_syntax_inits")
  if version < 508
    let did_masm_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default methods for highlighting.  Can be overridden later
  HiLink masmLabel	Label
  HiLink masmComment	Comment
  HiLink masmDirective	Statement
  HiLink masmOperator	Statement
  HiLink masmString	String

  HiLink masmHexadecimal Number
  HiLink masmDecimal	Number
  HiLink masmBinary	Number
  HiLink masmFloat	Number

  HiLink masmIdentifier Identifier

  delcommand HiLink
endif

let b:current_syntax = "masm"

" vim: ts=8
