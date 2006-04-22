" Vim syntax file
" Language:	Fortran95 (and Fortran90, Fortran77, F and elf90)
" Version:	0.88
" URL:		http://www.unb.ca/chem/ajit/syntax/fortran.vim
" Last Change:	2006 Apr. 22
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
    let b:lmax = 250
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

if b:fortran_dialect !=? "f77"
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

syn match fortranType		"\<character\>"
syn match fortranType		"\<complex\>"
syn match fortranType		"\<integer\>"
syn keyword fortranType		intrinsic
syn match fortranType		"\<implicit\>"
syn keyword fortranStructure	dimension
syn keyword fortranStorageClass	parameter save
syn match fortranUnitHeader	"\<subroutine\>"
syn keyword fortranCall		call
syn match fortranUnitHeader	"\<function\>"
syn match fortranUnitHeader	"\<program\>"
syn keyword fortranKeyword	return stop
syn keyword fortranConditional	else then
syn match fortranConditional	"\<if\>"
syn match fortranRepeat		"\<do\>"

syn keyword fortranTodo		contained todo fixme

"Catch errors caused by too many right parentheses
syn region fortranParen transparent start="(" end=")" contains=ALLBUT,fortranParenError,@fortranCommentGroup,cIncluded,@spell
syn match  fortranParenError   ")"

syn match fortranOperator	"\.\s*n\=eqv\s*\."
syn match fortranOperator	"\.\s*\(and\|or\|not\)\s*\."
syn match fortranOperator	"\(+\|-\|/\|\*\)"

syn match fortranBoolean	"\.\s*\(true\|false\)\s*\."

syn keyword fortranReadWrite	backspace close endfile inquire open print read rewind write

"If tabs are allowed then the left margin checks do not work
if exists("fortran_have_tabs")
  syn match fortranTab		"\t"  transparent
else
  syn match fortranTab		"\t"
endif

syn keyword fortranIO		access blank direct exist file fmt form formatted iostat name named nextrec number opened rec recl sequential status unformatted unit

syn keyword fortran66Intrinsic		alog alog10 amax0 amax1 amin0 amin1 amod cabs ccos cexp clog csin csqrt dabs dacos dasin datan datan2 dcos dcosh ddim dexp dint dlog dlog10 dmax1 dmin1 dmod dnint dsign dsin dsinh dsqrt dtan dtanh float iabs idim idint idnint ifix isign max0 max1 min0 min1 sngl

" Intrinsics provided by some vendors
syn keyword fortranExtraIntrinsic	algama cdabs cdcos cdexp cdlog cdsin cdsqrt cqabs cqcos cqexp cqlog cqsin cqsqrt dcmplx dconjg derf derfc dfloat dgamma dimag dlgama erf erfc gamma iqint qabs qacos qasin qatan qatan2 qcmplx qconjg qcos qcosh qdim qerf qerfc qexp qgamma qimag qlgama qlog qlog10 qmax1 qmin1 qmod qnint qsign qsin qsinh qsqrt qtan qtanh

syn keyword fortran77Intrinsic	abs acos aimag aint anint asin atan atan2 char cmplx conjg cos cosh exp ichar index int log log10 max min nint sign sin sinh sqrt tan tanh
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
syn keyword fortranKeywordEx	continue
syn match fortranKeywordEx	"\<go\s*to\>"
syn region fortranStringEx	start=+'+ end=+'+ contains=fortranContinueMark,fortranLeftMargin,fortranSerialNumber
syn keyword fortran77IntrinsicEx	dim lge lgt lle llt mod
syn keyword fortranKeywordOb	assign pause to

if b:fortran_dialect != "f77"

  syn match fortranType         "\<type\>"
  syn keyword fortranType	none

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
  syn keyword fortranKeyword	allocate deallocate nullify cycle exit
  syn match fortranConditional	"\<select\>"
  syn keyword fortranConditional	case default where elsewhere

  syn match fortranOperator	"\(\(>\|<\)=\=\|==\|/=\|=\)"
  syn match fortranOperator	"=>"

  syn region fortranString	start=+"+ end=+"+	contains=fortranLeftMargin,fortranContinueMark,fortranSerialNumber
  syn keyword fortranIO		pad position action delim readwrite
  syn keyword fortranIO		eor advance nml

  syn keyword fortran90Intrinsic	adjustl adjustr all allocated any associated bit_size btest ceiling count cshift date_and_time digits dot_product eoshift epsilon exponent floor fraction huge iand ibclr ibits ibset ieor ior ishft ishftc lbound len_trim matmul maxexponent maxloc maxval merge minexponent minloc minval modulo mvbits nearest pack precision present product radix random_number random_seed range repeat reshape rrspacing
  syn keyword fortran90Intrinsic	scale scan selected_int_kind selected_real_kind set_exponent shape size spacing spread sum system_clock tiny transpose trim ubound unpack verify
  syn match fortran90Intrinsic		"\<not\>\(\s*\.\)\@!"me=s+3
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

if b:fortran_dialect == "f95" || b:fortran_dialect == "F"
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
  syn match fortranComment	excludenl "!.*$" contains=@fortranCommentGroup,@spell
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
if exists("fortran_fold") || exists("fortran_more_precise")
  syn sync fromstart
elseif (b:fortran_fixed_source == 0)
  syn sync linecont "&" minlines=20
else
  syn sync minlines=20
endif

if version >= 600 && exists("fortran_fold")

  if (b:fortran_fixed_source == 1)
    syn region fortranProgram transparent fold keepend start="^\s*program\s\+\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\(program\(\s\+\z1\>\)\=\|$\)" contains=ALLBUT,fortranModule
    syn region fortranModule transparent fold keepend start="^\s*module\s\+\(procedure\)\@!\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\(module\(\s\+\z1\>\)\=\|$\)" contains=ALLBUT,fortranProgram
    syn region fortranFunction transparent fold keepend extend start="^\s*\(elemental \|pure \|recursive \)\=\s*\(\(\(real \|integer \|logical \|complex \|double \s*precision \)\s*\((\(\s*kind\s*=\)\=\s*\w\+\s*)\)\=\)\|type\s\+(\s*\w\+\s*) \|character \((\(\s*len\s*=\)\=\s*\d\+\s*)\|(\(\s*kind\s*=\)\=\s*\w\+\s*)\)\=\)\=\s*function\s\+\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\($\|function\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule
    syn region fortranSubroutine transparent fold keepend extend start="^\s*\(elemental \|pure \|recursive \)\=\s*subroutine\s\+\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\($\|subroutine\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule
    syn region fortranBlockData transparent fold keepend start="\<block\s*data\s\+\z(\a\w*\)" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*\($\|block\s*data\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule,fortranSubroutine,fortranFunction,fortran77Loop,fortranCase,fortran90Loop,fortranIfBlock
    syn region fortranInterface transparent fold keepend extend start="^\s*interface\>" skip="^\([!c*]\|\s*#\).*$" excludenl end="\<end\s*interface\>" contains=ALLBUT,fortranProgram,fortranModule,fortran77Loop,fortranCase,fortran90Loop,fortranIfBlock
  else
    syn region fortranProgram transparent fold keepend start="^\s*program\s\+\z(\a\w*\)" skip="^\s*[!#].*$" excludenl end="\<end\s*\(program\(\s\+\z1\>\)\=\|$\)" contains=ALLBUT,fortranModule
    syn region fortranModule transparent fold keepend start="^\s*module\s\+\(procedure\)\@!\z(\a\w*\)" skip="^\s*[!#].*$" excludenl end="\<end\s*\(module\(\s\+\z1\>\)\=\|$\)" contains=ALLBUT,fortranProgram
    syn region fortranFunction transparent fold keepend extend start="^\s*\(elemental \|pure \|recursive \)\=\s*\(\(\(real \|integer \|logical \|complex \|double \s*precision \)\s*\((\(\s*kind\s*=\)\=\s*\w\+\s*)\)\=\)\|type\s\+(\s*\w\+\s*) \|character \((\(\s*len\s*=\)\=\s*\d\+\s*)\|(\(\s*kind\s*=\)\=\s*\w\+\s*)\)\=\)\=\s*function\s\+\z(\a\w*\)" skip="^\s*[!#].*$" excludenl end="\<end\s*\($\|function\(\s\+\z1\>\)\=\)" contains=ALLBUT,fortranProgram,fortranModule
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
  HiLink fortranKeyword 	Keyword
  HiLink fortranConstructName	Identifier
  HiLink fortranConditional	Conditional
  HiLink fortranRepeat		Repeat
  HiLink fortranTodo		Todo
  HiLink fortranContinueMark	Todo
  HiLink fortranString		String
  HiLink fortranNumber		Number
  HiLink fortranOperator	Operator
  HiLink fortranBoolean		Boolean
  HiLink fortranLabelError	Error
  HiLink fortranObsolete	Todo
  HiLink fortranType		Type
  HiLink fortranStructure	Type
  HiLink fortranStorageClass	StorageClass
  HiLink fortranCall		fortranUnitHeader
  HiLink fortranUnitHeader	fortranPreCondit
  HiLink fortranReadWrite	Keyword
  HiLink fortranIO		Keyword
  HiLink fortran95Intrinsic	fortran90Intrinsic
  HiLink fortran77Intrinsic	fortran90Intrinsic
  HiLink fortran90Intrinsic	Function

  if ( b:fortran_dialect == "elf" || b:fortran_dialect == "F" )
    HiLink fortranKeywordOb	fortranObsolete
    HiLink fortran66Intrinsic	fortranObsolete
    HiLink fortran77IntrinsicR	fortranObsolete
    HiLink fortranUnitHeaderR	fortranObsolete
    HiLink fortranTypeR		fortranObsolete
    HiLink fortranStorageClassR	fortranObsolete
    HiLink fortran90StorageClassR	fortranObsolete
    HiLink fortran77OperatorR	fortranObsolete
    HiLink fortranInclude	fortranObsolete
  else
    HiLink fortranKeywordOb	fortranKeyword
    HiLink fortran66Intrinsic	fortran90Intrinsic
    HiLink fortran77IntrinsicR	fortran90Intrinsic
    HiLink fortranUnitHeaderR	fortranPreCondit
    HiLink fortranTypeR		fortranType
    HiLink fortranStorageClassR	fortranStorageClass
    HiLink fortran77OperatorR	fortranOperator
    HiLink fortranInclude	Include
    HiLink fortran90StorageClassR	fortranStorageClass
  endif

  if ( b:fortran_dialect == "F" )
    HiLink fortranLabelNumber	fortranObsolete
    HiLink fortranTarget	fortranObsolete
    HiLink fortranFormatSpec	fortranObsolete
    HiLink fortranFloatDExp	fortranObsolete
    HiLink fortranFloatNoDec	fortranObsolete
    HiLink fortranFloatIniDec	fortranObsolete
    HiLink fortranFloatEndDec	fortranObsolete
    HiLink fortranTypeEx	fortranObsolete
    HiLink fortranIOEx		fortranObsolete
    HiLink fortranKeywordEx	fortranObsolete
    HiLink fortranStringEx	fortranObsolete
    HiLink fortran77IntrinsicEx	fortranObsolete
    HiLink fortranUnitHeaderEx	fortranObsolete
    HiLink fortranConditionalEx	fortranObsolete
    HiLink fortran90IntrinsicEx	fortranObsolete
  else
    HiLink fortranLabelNumber	Special
    HiLink fortranTarget	Special
    HiLink fortranFormatSpec	Identifier
    HiLink fortranFloatDExp	fortranFloat
    HiLink fortranFloatNoDec	fortranFloat
    HiLink fortranFloatIniDec	fortranFloat
    HiLink fortranFloatEndDec	fortranFloat
    HiLink fortranTypeEx	fortranType
    HiLink fortranIOEx		fortranIO
    HiLink fortranKeywordEx	fortranKeyword
    HiLink fortranStringEx	fortranString
    HiLink fortran77IntrinsicEx	fortran90Intrinsic
    HiLink fortranUnitHeaderEx	fortranUnitHeader
    HiLink fortranConditionalEx	fortranConditional
    HiLink fortran90IntrinsicEx	fortran90Intrinsic
  endif

  HiLink fortranFloat		Float
  HiLink fortranPreCondit	PreCondit
  HiLink fortranInclude		Include
  HiLink cIncluded		fortranString
  HiLink cInclude		Include
  HiLink cPreProc		PreProc
  HiLink cPreCondit		PreCondit
  HiLink fortranParenError	Error
  HiLink fortranComment		Comment
  HiLink fortranSerialNumber	Todo
  HiLink fortranTab		Error
  " Vendor extensions
  HiLink fortranExtraIntrinsic	Function

  delcommand HiLink
endif

let b:current_syntax = "fortran"

" vim: ts=8 tw=132
