" Vim syntax file
" Language:	Fortran95 (and Fortran90, Fortran77, F and elf90)
" Version:	0.86
" URL:		http://www.unb.ca/chem/ajit/syntax/fortran.vim
" Last Change:	2003 Mar. 12
" Maintainer:	Ajit J. Thakkar (ajit AT unb.ca); <http://www.unb.ca/chem/ajit/>
" Usage:	Do :help fortran-syntax from Vim
" Credits:
"  Version 0.1 was based on the fortran 77 syntax file by Mario Eusebio and
"  Preben Guldberg. Useful suggestions were made by: Andrej Panjkov,
"  Bram Moolenaar, Thomas Olsen, Michael Sternberg, Christian Reile,
"  Walter Dieudonné, Alexander Wagner, Roman Bertle, Charles Rendleman,
"  and Andrew Griffiths. For instructions on use, do :help fortran from vim

" For version 5.x: Clear all syntax items
" For version 6.x: Quit if a syntax file is already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" let b:fortran_dialect = fortran_dialect if set correctly by user
if exists("fortran_dialect")
  if fortran_dialect =~ '\<\(f\(9[05]\|77\)\|elf\|F\)\>'
    let b:fortran_dialect = matchstr(fortran_dialect,'\<\(f\(9[05]\|77\)\|elf\|F\)\>')
  else
    echohl WarningMsg | echo "Unknown value of fortran_dialect" | echohl None
    let b:fortran_dialect = "unknown"
  endif
else
  let b:fortran_dialect = "unknown"
endif

" fortran_dialect not set or set incorrectly by user,
if b:fortran_dialect == "unknown"
  " set b:fortran_dialect from directive in first three lines of file
  let b:fortran_retype = getline(1)." ".getline(2)." ".getline(3)
  if b:fortran_retype =~ '\<fortran_dialect\s*=\s*F\>'
    let b:fortran_dialect = "F"
  elseif b:fortran_retype =~ '\<fortran_dialect\s*=\s*elf\>'
    let b:fortran_dialect = "elf"
  elseif b:fortran_retype =~ '\<fortran_dialect\s*=\s*f90\>'
    let b:fortran_dialect = "f90"
  elseif b:fortran_retype =~ '\<fortran_dialect\s*=\s*f95\>'
    let b:fortran_dialect = "f95"
  elseif b:fortran_retype =~ '\<fortran_dialect\s*=\s*f77\>'
    let b:fortran_dialect = "f77"
  else
    " no directive found, so assume f95
    let b:fortran_dialect = "f95"
  endif
  unlet b:fortran_retype
endif

" Choose between fixed and free source form if this hasn't been done yet
if !exists("b:fortran_fixed_source")
  if b:fortran_dialect == "elf" || b:fortran_dialect == "F"
    " elf and F require free source form
    let b:fortran_fixed_source = 0
  elseif b:fortran_dialect == "f77"
    " f77 requires fixed source form
    let b:fortran_fixed_source = 1
  elseif exists("fortran_free_source")
    " User guarantees free source form for all f90 and f95 files
    let b:fortran_fixed_source = 0
  elseif exists("fortran_fixed_source")
    " User guarantees fixed source form for all f90 and f95 files
    let b:fortran_fixed_source = 1
  else
    " f90 and f95 allow both fixed and free source form.
    " Assume fixed source form unless signs of free source form
    " are detected in the first five columns of the first b:lmax lines.
    " Detection becomes more accurate and time-consuming if more lines
    " are checked. Increase the limit below if you keep lots of comments at
    " the very top of each file and you have a fast computer.
    let b:lmax = 25
    if ( b:lmax > line("$") )
      let b:lmax = line("$")
    endif
    let b:fortran_fixed_source = 1
    let b:ln=1
    while b:ln <= b:lmax
      let b:test = strpart(getline(b:ln),0,5)
      if b:test[0] !~ '[Cc*!#]' && b:test !~ '^ \+[!#]' && b:test =~ '[^ 0-9\t]'
	let b:fortran_fixed_source = 0
	break
      endif
      let b:ln = b:ln + 1
    endwhile
    unlet b:lmax b:ln b:test
  endif
endif

syn case ignore

if b:fortran_dialect == "f77"
  syn match fortranIdentifier		"\<\a\(\a\|\d\)*\>" contains=fortranSerialNumber
else
  syn match fortran90Identifier		"\<\a\w*\>" contains=fortranSerialNumber
  if version >= 600
    if b:fortran_fixed_source == 1
      syn match fortranConstructName	"^\s\{6,}\zs\a\w*\ze\s*:"
    else
      syn match fortranConstructName	"^\s*\zs\a\w*\ze\s*:"
    endif
    if exists("fortran_more_precise")
      syn match fortranConstructName "\(\<end\s*do\s\+\)\@<=\a\w*"
      syn match fortranConstructName "\(\<end\s*if\s\+\)\@<=\a\w*"
      syn match fortranConstructName "\(\<end\s*select\s\+\)\@<=\a\w*"
    endif
  else
    if b:fortran_fixed_source == 1
      syn match fortranConstructName	"^\s\{6,}\a\w*\s*:"
    else
      syn match fortranConstructName	"^\s*\a\w*\s*:"
    endif
  endif
endif

syn match   fortranUnitHeader	"\<end\>"

syn keyword fortranType		character complex integer
syn keyword fortranType		intrinsic
syn match fortranType		"\<implicit\>"
syn keyword fortranStructure	dimension
syn keyword fortranStorageClass	parameter save
syn match fortranUnitHeader	"\<subroutine\>"
syn keyword fortranCall		call
syn match fortranUnitHeader	"\<function\>"
syn match fortranUnitHeader	"\<program\>"
syn keyword fortranStatement	return stop
syn keyword fortranConditional	else then
syn match fortranConditional	"\<if\>"
syn match fortranRepeat		"\<do\>"

syn keyword fortranTodo		contained todo fixme

"Catch errors caused by too many right parentheses
syn region fortranParen transparent start="(" end=")" contains=ALLBUT,fortranParenError,@fortranCommentGroup,cIncluded
syn match  fortranParenError   ")"

syn match fortranOperator	"\.\s*n\=eqv\s*\."
syn match fortranOperator	"\.\s*\(and\|or\|not\)\s*\."
syn match fortranOperator	"\(+\|-\|/\|\*\)"

syn match fortranBoolean	"\.\s*\(true\|false\)\s*\."

syn keyword fortranReadWrite	backspace close inquire open rewind endfile
syn keyword fortranReadWrite	read write print

"If tabs are allowed then the left margin checks do not work
if exists("fortran_have_tabs")
  syn match fortranTab		"\t"  transparent
else
  syn match fortranTab		"\t"
endif

syn keyword fortranIO		unit file iostat access blank fmt form
syn keyword fortranIO		recl status exist opened number named name
syn keyword fortranIO		sequential direct rec
syn keyword fortranIO		formatted unformatted nextrec

syn keyword fortran66Intrinsic		cabs ccos cexp clog csin csqrt
syn keyword fortran66Intrinsic		dacos dasin datan datan2 dcos dcosh
syn keyword fortran66Intrinsic		ddim dexp dint dlog dlog10 dmod dabs
syn keyword fortran66Intrinsic		dnint dsign dsin dsinh dsqrt dtan
syn keyword fortran66Intrinsic		dtanh iabs idim idnint isign idint ifix
syn keyword fortran66Intrinsic		amax0 amax1 dmax1 max0 max1
syn keyword fortran66Intrinsic		amin0 amin1 dmin1 min0 min1
syn keyword fortran66Intrinsic		amod float sngl alog alog10

" Intrinsics provided by some vendors
syn keyword fortranExtraIntrinsic	cdabs cdcos cdexp cdlog cdsin cdsqrt
syn keyword fortranExtraIntrinsic	cqabs cqcos cqexp cqlog cqsin cqsqrt
syn keyword fortranExtraIntrinsic	qacos qasin qatan qatan2 qcos qcosh
syn keyword fortranExtraIntrinsic	qdim qexp iqint qlog qlog10 qmod qabs
syn keyword fortranExtraIntrinsic	qnint qsign qsin qsinh qsqrt qtan
syn keyword fortranExtraIntrinsic	qtanh qmax1 qmin1
syn keyword fortranExtraIntrinsic	dimag qimag dcmplx qcmplx dconjg qconjg
syn keyword fortranExtraIntrinsic	gamma dgamma qgamma algama dlgama qlgama
syn keyword fortranExtraIntrinsic	erf derf qerf erfc derfc qerfc
syn keyword fortranExtraIntrinsic	dfloat

syn keyword fortran77Intrinsic	abs acos aimag aint anint asin atan atan2
syn keyword fortran77Intrinsic	cos sin tan sinh cosh tanh exp log log10
syn keyword fortran77Intrinsic	sign sqrt int cmplx nint min max conjg
syn keyword fortran77Intrinsic	char ichar index
syn match fortran77Intrinsic	"\<len\s*[(,]"me=s+3
syn match fortran77Intrinsic	"\<real\s*("me=s+4
syn match fortranType		"\<implicit\s\+real"
syn match fortranType		"^\s*real\>"
syn match fortran90Intrinsic	"\<logical\s*("me=s+7
syn match fortranType		"\<implicit\s\+logical"
syn match fortranType		"^\s*logical\>"

"Numbers of various sorts
" Integers
syn match fortranNumber	display "\<\d\+\(_\a\w*\)\=\>"
" floating point number, without a decimal point
syn match fortranFloatNoDec	display	"\<\d\+[deq][-+]\=\d\+\(_\a\w*\)\=\>"
" floating point number, starting with a decimal point
syn match fortranFloatIniDec	display	"\.\d\+\([deq][-+]\=\d\+\)\=\(_\a\w*\)\=\>"
" floating point number, no digits after decimal
syn match fortranFloatEndDec	display	"\<\d\+\.\([deq][-+]\=\d\+\)\=\(_\a\w*\)\=\>"
" floating point number, D or Q exponents
syn match fortranFloatDExp	display	"\<\d\+\.\d\+\([dq][-+]\=\d\+\)\=\(_\a\w*\)\=\>"
" floating point number
syn match fortranFloat	display	"\<\d\+\.\d\+\(e[-+]\=\d\+\)\=\(_\a\w*\)\=\>"
" Numbers in formats
syn match fortranFormatSpec	display	"\d*f\d\+\.\d\+"
syn match fortranFormatSpec	display	"\d*e[sn]\=\d\+\.\d\+\(e\d+\>\)\="
syn match fortranFormatSpec	display	"\d*\(d\|q\|g\)\d\+\.\d\+\(e\d+\)\="
syn match fortranFormatSpec	display	"\d\+x\>"
" The next match cannot be used because it would pick up identifiers as well
" syn match fortranFormatSpec	display	"\<\(a\|i\)\d\+"

" Numbers as labels
syn match fortranLabelNumber	display	"^\d\{1,5}\s"me=e-1
syn match fortranLabelNumber	display	"^ \d\{1,4}\s"ms=s+1,me=e-1
syn match fortranLabelNumber	display	"^  \d\{1,3}\s"ms=s+2,me=e-1
syn match fortranLabelNumber	display	"^   \d\d\=\s"ms=s+3,me=e-1
syn match fortranLabelNumber	display	"^    \d\s"ms=s+4,me=e-1

if version >= 600 && exists("fortran_more_precise")
  " Numbers as targets
  syn match fortranTarget	display	"\(\<if\s*(.\+)\s*\)\@<=\(\d\+\s*,\s*\)\{2}\d\+\>"
  syn match fortranTarget	display	"\(\<do\s\+\)\@<=\d\+\>"
  syn match fortranTarget	display	"\(\<go\s*to\s*(\=\)\@<=\(\d\+\s*,\s*\)*\d\+\>"
endif

syn keyword fortranTypeEx	external
syn keyword fortranIOEx		format
syn keyword fortranStatementEx	continue
syn match fortranStatementEx	"\<go\s*to\>"
syn region fortranStringEx	start=+'+ end=+'+ contains=fortranContinueMark,fortranLeftMargin,fortranSerialNumber
syn keyword fortran77IntrinsicEx	dim lge lgt lle llt mod
syn keyword fortranStatementOb	assign pause to

if b:fortran_dialect != "f77"

  syn keyword fortranType	type none

  syn keyword fortranStructure	private public intent optional
  syn keyword fortranStructure	pointer target allocatable
  syn keyword fortranStorageClass	in out
  syn match fortranStorageClass	"\<kind\s*="me=s+4
  syn match fortranStorageClass	"\<len\s*="me=s+3

  syn match fortranUnitHeader	"\<module\>"
  syn keyword fortranUnitHeader	use only contains
  syn keyword fortranUnitHeader	result operator assignment
  syn match fortranUnitHeader	"\<interface\>"
  syn match fortranUnitHeader	"\<recursive\>"
  syn keyword fortranStatement	allocate deallocate nullify cycle exit
  syn match fortranConditional	"\<select\>"
  syn keyword fortranConditional	case default where elsewhere

  syn match fortranOperator	"\(\(>\|<\)=\=\|==\|/=\|=\)"
  syn match fortranOperator	"=>"

  syn region fortranString	start=+"+ end=+"+	contains=fortranLeftMargin,fortranContinueMark,fortranSerialNumber
  syn keyword fortranIO		pad position action delim readwrite
  syn keyword fortranIO		eor advance nml

  syn keyword fortran90Intrinsic	adjustl adjustr all allocated any
  syn keyword fortran90Intrinsic	associated bit_size btest ceiling
  syn keyword fortran90Intrinsic	count cshift date_and_time
  syn keyword fortran90Intrinsic	digits dot_product eoshift epsilon exponent
  syn keyword fortran90Intrinsic	floor fraction huge iand ibclr ibits ibset ieor
  syn keyword fortran90Intrinsic	ior ishft ishftc lbound len_trim
  syn keyword fortran90Intrinsic	matmul maxexponent maxloc maxval merge
  syn keyword fortran90Intrinsic	minexponent minloc minval modulo mvbits nearest
  syn keyword fortran90Intrinsic	pack present product radix random_number
  syn match fortran90Intrinsic		"\<not\>\(\s*\.\)\@!"me=s+3
  syn keyword fortran90Intrinsic	random_seed range repeat reshape rrspacing scale
  syn keyword fortran90Intrinsic	selected_int_kind selected_real_kind scan
  syn keyword fortran90Intrinsic	shape size spacing spread set_exponent
  syn keyword fortran90Intrinsic	tiny transpose trim ubound unpack verify
  syn keyword fortran90Intrinsic	precision sum system_clock
  syn match fortran90Intrinsic	"\<kind\>\s*[(,]"me=s+4

  syn match  fortranUnitHeader	"\<end\s*function"
  syn match  fortranUnitHeader	"\<end\s*interface"
  syn match  fortranUnitHeader	"\<end\s*module"
  syn match  fortranUnitHeader	"\<end\s*program"
  syn match  fortranUnitHeader	"\<end\s*subroutine"
  syn match  fortranRepeat	"\<end\s*do"
  syn match  fortranConditional	"\<end\s*where"
  syn match  fortranConditional	"\<select\s*case"
  syn match  fortranConditional	"\<end\s*select"
  syn match  fortranType	"\<end\s*type"
  syn match  fortranType	"\<in\s*out"

  syn keyword fortranUnitHeaderEx	procedure
  syn keyword fortranIOEx		namelist
  syn keyword fortranConditionalEx	while
  syn keyword fortran90IntrinsicEx	achar iachar transfer

  syn keyword fortranInclude		include
  syn keyword fortran90StorageClassR	sequence
endif

syn match   fortranConditional	"\<end\s*if"
syn match   fortranIO		contains=fortranOperator "\<e\(nd\|rr\)\s*=\s*\d\+"
syn match   fortranConditional	"\<else\s*if"

syn keyword fortranUnitHeaderR	entry
syn match fortranTypeR		display "double\s\+precision"
syn match fortranTypeR		display "double\s\+complex"
syn match fortranUnitHeaderR	display "block\s\+data"
syn keyword fortranStorageClassR	common equivalence data
syn keyword fortran77IntrinsicR	dble dprod
syn match   fortran77OperatorR	"\.\s*[gl][et]\s*\."
syn match   fortran77OperatorR	"\.\s*\(eq\|ne\)\s*\."

if b:fortran_dialect == "f95"
  syn keyword fortranRepeat		forall
  syn match fortranRepeat		"\<end\s*forall"
  syn keyword fortran95Intrinsic	null cpu_time
  syn match fortranType			"\<elemental\>"
  syn match fortranType			"\<pure\>"
  if exists("fortran_more_precise")
    syn match fortranConstructName "\(\<end\s*forall\s\+\)\@<=\a\w*\>"
  endif
endif

syn cluster fortranCommentGroup contains=fortranTodo

if (b:fortran_fixed_source == 1)
  if !exists("fortran_have_tabs")
    "Flag items beyond column 72
    syn match fortranSerialNumber	excludenl "^.\{73,}$"lc=72
    "Flag left margin errors
    syn match fortranLabelError	"^.\{-,4}[^0-9 ]" contains=fortranTab
    syn match fortranLabelError	"^.\{4}\d\S"
  endif
  syn match fortranComment		excludenl "^[!c*].*$" contains=@fortranCommentGroup
  syn match fortranLeftMargin		transparent "^ \{5}"
  syn match fortranContinueMark		display "^.\{5}\S"lc=5
else
  syn match fortranContinueMark		display "&"
endif

if b:fortran_dialect != "f77"
  syn match fortranComment	excludenl "!.*$" contains=@fortranCommentGroup
endif

"cpp is often used with Fortran
syn match	cPreProc		"^\s*#\s*\(define\|ifdef\)\>.*"
syn match	cPreProc		"^\s*#\s*\(elif\|if\)\>.*"
syn match	cPreProc		"^\s*#\s*\(ifndef\|undef\)\>.*"
syn match	cPreCondit		"^\s*#\s*\(else\|endif\)\>.*"
syn region	cIncluded	contained start=+"[^(]+ skip=+\\\\\|\\"+ end=+"+ contains=fortranLeftMargin,fortranContinueMark,fortranSerialNumber
syn match	cIncluded		contained "<[^>]*>"
syn match	cInclude		"^\s*#\s*include\>\s*["<]" contains=cIncluded

"Synchronising limits assume that comment and continuation lines are not mixed
if (b:fortran_fixed_source == 0)
  syn sync linecont "&" maxlines=40
else
  syn sync minlines=20
endif

if version >= 600 && exists("fortran_fold")

  syn sync fromstart
  if (b:fortran_fixed_source == 1)
    syn region fortranProgram transparent fold keepend start="^\s*program\s\+\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\(program\(\s\+\z1\>\)\=\|$\)" contains=ALLBUT,fortranModule
    syn region fortranModule transparent fold keepend start="^\s*module\s\+\(procedure\)\@!\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\(module\(\s\+\z1\>\)\=\|$\)" contains=ALLBUT,fortranProgram
    syn region fortranFunction transparent fold keepend extend start="^\s*\(elemental \|pure \|recursive \)\=\s*function\s\+\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\($\|function\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule
    syn region fortranSubroutine transparent fold keepend extend start="^\s*\(elemental \|pure \|recursive \)\=\s*subroutine\s\+\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\($\|subroutine\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule
    syn region fortranBlockData transparent fold keepend start="\<block\s*data\s\+\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\($\|block\s*data\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortran77Loop,fortranCase,fortran90Loop,fortranIfBlock
    syn region fortranInterface transparent fold keepend extend start="^\s*interface\>" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*interface\>" contains=ALLBUT,fortranProgram,fortranModule,fortran77Loop,fortranCase,fortran90Loop,fortranIfBlock
  else
    syn region fortranProgram transparent fold keepend start="^\s*program\s\+\z(\a\w*\)" skip="^\s*[!#].*$" excludenl end="\<end\s*\(program\(\s\+\z1\>\)\=\|$\)" contains=ALLBUT,fortranModule
    syn region fortranModule transparent fold keepend start="^\s*module\s\+\(procedure\)\@!\z(\a\w*\)" skip="^\s*[!#].*$" excludenl end="\<end\s*\(module\(\s\+\z1\>\)\=\|$\)" contains=ALLBUT,fortranProgram
    syn region fortranFunction transparent fold keepend extend start="^\s*\(elemental \|pure \|recursive \)\=\s*function\s\+\z(\a\w*\)" skip="^\s*[!#].*$" excludenl end="\<end\s*\($\|function\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule
    syn region fortranSubroutine transparent fold keepend extend start="^\s*\(elemental \|pure \|recursive \)\=\s*subroutine\s\+\z(\a\w*\)" skip="^\s*[!#].*$" excludenl end="\<end\s*\($\|subroutine\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule
    syn region fortranBlockData transparent fold keepend start="\<block\s*data\s\+\z(\a\w*\)" skip="^\s*[!#].*$" excludenl end="\<end\s*\($\|block\s*data\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortran77Loop,fortranCase,fortran90Loop,fortranIfBlock
    syn region fortranInterface transparent fold keepend extend start="^\s*interface\>" skip="^\s*[!#].*$" excludenl end="\<end\s*interface\>" contains=ALLBUT,fortranProgram,fortranModule,fortran77Loop,fortranCase,fortran90Loop,fortranIfBlock
  endif

  if exists("fortran_fold_conditionals")
    if (b:fortran_fixed_source == 1)
      syn region fortran77Loop transparent fold keepend start="\<do\s\+\z(\d\+\)" end="^\s*\z1\>" contains=ALLBUT,fortranUnitHeader,fortranStructure,fortranStorageClass,fortranType,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortranBlockData
      syn region fortran90Loop transparent fold keepend extend start="\(\<end\s\+\)\@<!\<do\(\s\+\a\|\s*$\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*do\>" contains=ALLBUT,fortranUnitHeader,fortranStructure,fortranStorageClass,fortranType,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortranBlockData
      syn region fortranIfBlock transparent fold keepend extend start="\(\<e\(nd\|lse\)\s\+\)\@<!\<if\s*(.\+)\s*then\>" skip="^\([!c*]\|\s*#\).*$" end="\<end\s*if\>" contains=ALLBUT,fortranUnitHeader,fortranStructure,fortranStorageClass,fortranType,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortranBlockData
      syn region fortranCase transparent fold keepend extend start="\<select\s*case\>" skip="^\([!c*]\|\s*#\).*$" end="\<end\s*select\>" contains=ALLBUT,fortranUnitHeader,fortranStructure,fortranStorageClass,fortranType,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortranBlockData
    else
      syn region fortran77Loop transparent fold keepend start="\<do\s\+\z(\d\+\)" end="^\s*\z1\>" contains=ALLBUT,fortranUnitHeader,fortranStructure,fortranStorageClass,fortranType,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortranBlockData
      syn region fortran90Loop transparent fold keepend extend start="\(\<end\s\+\)\@<!\<do\(\s\+\a\|\s*$\)" skip="^\s*[!#].*$" excludenl end="\<end\s*do\>" contains=ALLBUT,fortranUnitHeader,fortranStructure,fortranStorageClass,fortranType,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortranBlockData
      syn region fortranIfBlock transparent fold keepend extend start="\(\<e\(nd\|lse\)\s\+\)\@<!\<if\s*(.\+)\s*then\>" skip="^\s*[!#].*$" end="\<end\s*if\>" contains=ALLBUT,fortranUnitHeader,fortranStructure,fortranStorageClass,fortranType,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortranBlockData
      syn region fortranCase transparent fold keepend extend start="\<select\s*case\>" skip="^\s*[!#].*$" end="\<end\s*select\>" contains=ALLBUT,fortranUnitHeader,fortranStructure,fortranStorageClass,fortranType,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortranBlockData
    endif
  endif

  if exists("fortran_fold_multilinecomments")
    if (b:fortran_fixed_source == 1)
      syn match fortranMultiLineComments transparent fold "\(^[!c*].*\(\n\|\%$\)\)\{4,}" contains=ALLBUT,fortranMultiCommentLines
    else
      syn match fortranMultiLineComments transparent fold "\(^\s*!.*\(\n\|\%$\)\)\{4,}" contains=ALLBUT,fortranMultiCommentLines
    endif
  endif
endif

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_fortran_syn_inits")
  if version < 508
    let did_fortran_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default highlighting differs for each dialect.
  " Transparent groups:
  " fortranParen, fortranLeftMargin
  " fortranProgram, fortranModule, fortranSubroutine, fortranFunction,
  " fortranBlockData
  " fortran77Loop, fortran90Loop, fortranIfBlock, fortranCase
  " fortranMultiCommentLines
  HiLink fortranStatement		Statement
  HiLink fortranConstructName	Special
  HiLink fortranConditional		Conditional
  HiLink fortranRepeat		Repeat
  HiLink fortranTodo			Todo
  HiLink fortranContinueMark		Todo
  HiLink fortranString		String
  HiLink fortranNumber		Number
  HiLink fortranOperator		Operator
  HiLink fortranBoolean		Boolean
  HiLink fortranLabelError		Error
  HiLink fortranObsolete		Todo
  HiLink fortranType			Type
  HiLink fortranStructure		Type
  HiLink fortranStorageClass		StorageClass
  HiLink fortranCall			fortranUnitHeader
  HiLink fortranUnitHeader		fortranPreCondit
  HiLink fortranReadWrite		fortran90Intrinsic
  HiLink fortranIO			fortran90Intrinsic
  HiLink fortran95Intrinsic		fortran90Intrinsic
  HiLink fortran77Intrinsic		fortran90Intrinsic
  HiLink fortran90Intrinsic		Special

  if ( b:fortran_dialect == "elf" || b:fortran_dialect == "F" )
    HiLink fortranStatementOb	fortranObsolete
    HiLink fortran66Intrinsic	fortranObsolete
    HiLink fortran77IntrinsicR	fortranObsolete
    HiLink fortranUnitHeaderR	fortranObsolete
    HiLink fortranTypeR		fortranObsolete
    HiLink fortranStorageClassR	fortranObsolete
    HiLink fortran90StorageClassR	fortranObsolete
    HiLink fortran77OperatorR	fortranObsolete
    HiLink fortranInclude		fortranObsolete
  else
    HiLink fortranStatementOb	Statement
    HiLink fortran66Intrinsic	fortran90Intrinsic
    HiLink fortran77IntrinsicR	fortran90Intrinsic
    HiLink fortranUnitHeaderR	fortranPreCondit
    HiLink fortranTypeR		fortranType
    HiLink fortranStorageClassR	fortranStorageClass
    HiLink fortran77OperatorR	fortranOperator
    HiLink fortranInclude		Include
    HiLink fortran90StorageClassR	fortranStorageClass
  endif

  if ( b:fortran_dialect == "F" )
    HiLink fortranLabelNumber	fortranObsolete
    HiLink fortranTarget		fortranObsolete
    HiLink fortranFormatSpec		fortranObsolete
    HiLink fortranFloatDExp		fortranObsolete
    HiLink fortranFloatNoDec		fortranObsolete
    HiLink fortranFloatIniDec	fortranObsolete
    HiLink fortranFloatEndDec	fortranObsolete
    HiLink fortranTypeEx		fortranObsolete
    HiLink fortranIOEx		fortranObsolete
    HiLink fortranStatementEx	fortranObsolete
    HiLink fortranStringEx		fortranObsolete
    HiLink fortran77IntrinsicEx	fortranObsolete
    HiLink fortranUnitHeaderEx	fortranObsolete
    HiLink fortranConditionalEx	fortranObsolete
    HiLink fortran90IntrinsicEx	fortranObsolete
  else
    HiLink fortranLabelNumber	Special
    HiLink fortranTarget		Special
    HiLink fortranFormatSpec		Identifier
    HiLink fortranFloatDExp		fortranFloat
    HiLink fortranFloatNoDec		fortranFloat
    HiLink fortranFloatIniDec	fortranFloat
    HiLink fortranFloatEndDec	fortranFloat
    HiLink fortranTypeEx		fortranType
    HiLink fortranIOEx		fortranIO
    HiLink fortranStatementEx	fortranStatement
    HiLink fortranStringEx		fortranString
    HiLink fortran77IntrinsicEx	fortran90Intrinsic
    HiLink fortranUnitHeaderEx	fortranUnitHeader
    HiLink fortranConditionalEx	fortranConditional
    HiLink fortran90IntrinsicEx	fortran90Intrinsic
  endif

  HiLink fortranFloat		Float
  HiLink fortran90Identifier		fortranIdentifier
  "Uncomment the next line if you want all fortran variables to be highlighted
  "HiLink fortranIdentifier		Identifier
  HiLink fortranPreCondit		PreCondit
  HiLink fortranInclude		Include
  HiLink cIncluded			fortranString
  HiLink cInclude			Include
  HiLink cPreProc			PreProc
  HiLink cPreCondit			PreCondit
  HiLink fortranParenError		Error
  HiLink fortranComment		Comment
  HiLink fortranSerialNumber		Todo
  HiLink fortranTab			Error
  " Vendor extensions
  HiLink fortranExtraIntrinsic	Special

  delcommand HiLink
endif

let b:current_syntax = "fortran"

" vim: ts=8 tw=132
