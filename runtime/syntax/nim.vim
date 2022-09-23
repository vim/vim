" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if v:version < 600
  syntax clear
elseif exists('b:current_syntax')
  finish
endif

" Keep user-supplied options
if !exists('nim_highlight_numbers')
  let nim_highlight_numbers = 1
endif
if !exists('nim_highlight_builtins')
  let nim_highlight_builtins = 1
endif
if !exists('nim_highlight_exceptions')
  let nim_highlight_exceptions = 1
endif
if !exists('nim_highlight_space_errors')
  let nim_highlight_space_errors = 1
endif
if !exists('nim_highlight_special_vars')
  let nim_highlight_special_vars = 1
endif

if exists('nim_highlight_all')
  let nim_highlight_numbers      = 1
  let nim_highlight_builtins     = 1
  let nim_highlight_exceptions   = 1
  let nim_highlight_space_errors = 1
  let nim_highlight_special_vars = 1
endif

syn region nimBrackets       contained extend keepend matchgroup=Bold start=+\(\\\)\@<!\[+ end=+]\|$+ skip=+\\\s*$\|\(\\\)\@<!\\]+ contains=@tclCommandCluster

syn keyword nimKeyword       addr and as asm atomic
syn keyword nimKeyword       bind block break
syn keyword nimKeyword       case cast concept const continue converter
syn keyword nimKeyword       defer discard distinct div do
syn keyword nimKeyword       elif else end enum except export
syn keyword nimKeyword       finally for from
syn keyword nimKeyword       generic
syn keyword nimKeyword       if import in include interface is isnot iterator
syn keyword nimKeyword       let
syn keyword nimKeyword       mixin using mod
syn keyword nimKeyword       nil not notin
syn keyword nimKeyword       object of or out
syn keyword nimKeyword       proc func method macro template nextgroup=nimFunction skipwhite
syn keyword nimKeyword       ptr
syn keyword nimKeyword       raise ref return
syn keyword nimKeyword       shared shl shr static
syn keyword nimKeyword       try tuple type
syn keyword nimKeyword       var vtref vtptr
syn keyword nimKeyword       when while with without
syn keyword nimKeyword       xor
syn keyword nimKeyword       yield

syn match   nimFunction      "[a-zA-Z_][a-zA-Z0-9_]*" contained
syn match   nimClass         "[a-zA-Z_][a-zA-Z0-9_]*" contained
syn keyword nimRepeat        for while
syn keyword nimConditional   if elif else case of
syn keyword nimOperator      and in is not or xor shl shr div
syn match   nimComment       "#.*$" contains=nimTodo,@Spell
syn region  nimComment       start="#\[" end="\]#" contains=nimTodo,@Spell
syn keyword nimTodo          TODO FIXME XXX contained
syn keyword nimBoolean       true false


" Strings
syn region nimString start=+'+ skip=+\\\\\|\\'\|\\$+ excludenl end=+'+ end=+$+ keepend contains=nimEscape,nimEscapeError,@Spell
syn region nimString start=+"+ skip=+\\\\\|\\"\|\\$+ excludenl end=+"+ end=+$+ keepend contains=nimEscape,nimEscapeError,@Spell
syn region nimString start=+"""+ end=+"""+ keepend contains=nimEscape,nimEscapeError,@Spell
syn region nimRawString matchgroup=Normal start=+[rR]"+ end=+"+ skip=+\\\\\|\\"+ contains=@Spell

syn match  nimEscape		+\\[abfnrtv'"\\]+ contained
syn match  nimEscape		"\\\o\{1,3}" contained
syn match  nimEscape		"\\x\x\{2}" contained
syn match  nimEscape		"\(\\u\x\{4}\|\\U\x\{8}\)" contained
syn match  nimEscape		"\\$"

syn match nimEscapeError "\\x\x\=\X" display contained

if nim_highlight_numbers == 1
  " numbers (including longs and complex)
  let s:dec_num = '\d%(_?\d)*'
  let s:int_suf = '%(''%(%(i|I|u|U)%(8|16|32|64)|u|U))'
  let s:float_suf = '%(''%(%(f|F)%(32|64|128)?|d|D))'
  let s:exp = '%([eE][+-]?'.s:dec_num.')'
  exe 'syn match nimNumber /\v<0[bB][01]%(_?[01])*%('.s:int_suf.'|'.s:float_suf.')?>/'
  exe 'syn match nimNumber /\v<0[ocC]\o%(_?\o)*%('.s:int_suf.'|'.s:float_suf.')?>/'
  exe 'syn match nimNumber /\v<0[xX]\x%(_?\x)*%('.s:int_suf.'|'.s:float_suf.')?>/'
  exe 'syn match nimNumber /\v<'.s:dec_num.'%('.s:int_suf.'|'.s:exp.'?'.s:float_suf.'?)>/'
  exe 'syn match nimNumber /\v<'.s:dec_num.'\.'.s:dec_num.s:exp.'?'.s:float_suf.'?>/'
  unlet s:dec_num s:int_suf s:float_suf s:exp
endif

if nim_highlight_builtins == 1
  " builtin functions, types and objects, not really part of the syntax
  syn keyword nimBuiltin int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 float float32 float64
  syn keyword nimBuiltin bool void chr char string cstring pointer range array openarray openArray seq varargs varArgs
  syn keyword nimBuiltin set Byte Natural Positive Conversion
  syn keyword nimBuiltin BiggestInt BiggestFloat cchar cschar cshort cint csize cuchar cushort
  syn keyword nimBuiltin clong clonglong cfloat cdouble clongdouble cuint culong culonglong cchar
  syn keyword nimBuiltin CompileDate CompileTime nimversion nimVersion nimmajor nimMajor
  syn keyword nimBuiltin nimminor nimMinor nimpatch nimPatch cpuendian cpuEndian hostos hostOS hostcpu hostCPU inf
  syn keyword nimBuiltin neginf nan QuitSuccess QuitFailure dbglinehook dbgLineHook stdin
  syn keyword nimBuiltin stdout stderr defined new high low sizeof succ pred
  syn keyword nimBuiltin inc dec newseq newSeq len incl excl card ord chr ze ze64
  syn keyword nimBuiltin tou8 toU8 tou16 toU16 tou32 toU32 abs min max add repr
  syn match   nimBuiltin "\<contains\>"
  syn keyword nimBuiltin tofloat toFloat tobiggestfloat toBiggestFloat toint toInt tobiggestint toBiggestInt
  syn keyword nimBuiltin addquitproc addQuitProc
  syn keyword nimBuiltin copy setlen setLen newstring newString zeromem zeroMem copymem copyMem movemem moveMem
  syn keyword nimBuiltin equalmem equalMem alloc alloc0 realloc dealloc assert
  syn keyword nimBuiltin typedesc typed untyped stmt expr
  syn keyword nimBuiltin echo swap getrefcount getRefcount getcurrentexception getCurrentException Msg
  syn keyword nimBuiltin getoccupiedmem getOccupiedMem getfreemem getFreeMem gettotalmem getTotalMem isnil isNil seqtoptr seqToPtr
  syn keyword nimBuiltin find pop GC_disable GC_enable GC_fullCollect
  syn keyword nimBuiltin GC_setStrategy GC_enableMarkAndSweep GC_Strategy
  syn keyword nimBuiltin GC_disableMarkAnd Sweep GC_getStatistics GC_ref
  syn keyword nimBuiltin GC_ref GC_ref GC_unref GC_unref GC_unref quit
  syn keyword nimBuiltin OpenFile OpenFile CloseFile EndOfFile readChar
  syn keyword nimBuiltin FlushFile readfile readFile readline readLine write writeln writeLn writeline writeLine
  syn keyword nimBuiltin getfilesize getFileSize ReadBytes ReadChars readbuffer readBuffer writebytes writeBytes
  syn keyword nimBuiltin writechars writeChars writebuffer writeBuffer setfilepos setFilePos getfilepos getFilePos
  syn keyword nimBuiltin filehandle fileHandle countdown countup items lines
  syn keyword nimBuiltin FileMode File RootObj FileHandle ByteAddress Endianness
endif

if nim_highlight_exceptions == 1
  " builtin exceptions and warnings
  syn keyword nimException E_Base EAsynch ESynch ESystem EIO EOS
  syn keyword nimException ERessourceExhausted EArithmetic EDivByZero
  syn keyword nimException EOverflow EAccessViolation EAssertionFailed
  syn keyword nimException EControlC EInvalidValue EOutOfMemory EInvalidIndex
  syn keyword nimException EInvalidField EOutOfRange EStackOverflow
  syn keyword nimException ENoExceptionToReraise EInvalidObjectAssignment
  syn keyword nimException EInvalidObject EInvalidLibrary EInvalidKey
  syn keyword nimException EInvalidObjectConversion EFloatingPoint
  syn keyword nimException EFloatInvalidOp EFloatDivByZero EFloatOverflow
  syn keyword nimException EFloatInexact EDeadThread EResourceExhausted
  syn keyword nimException EFloatUnderflow
endif

if nim_highlight_space_errors == 1
  " trailing whitespace
  syn match   nimSpaceError   display excludenl "\S\s\+$"ms=s+1
  " any tabs are illegal in nim
  syn match   nimSpaceError   display "\t"
endif

if nim_highlight_special_vars
  syn keyword nimSpecialVar result
endif

syn sync match nimSync grouphere NONE "):$"
syn sync maxlines=200
syn sync minlines=2000

if v:version >= 508 || !exists('did_nim_syn_inits')
  if v:version <= 508
    let did_nim_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default methods for highlighting.  Can be overridden later
  HiLink nimBrackets       Operator
  HiLink nimKeyword	      Keyword
  HiLink nimFunction	    	Function
  HiLink nimConditional	  Conditional
  HiLink nimRepeat		      Repeat
  HiLink nimString		      String
  HiLink nimRawString	    String
  HiLink nimBoolean        Boolean
  HiLink nimEscape		      Special
  HiLink nimOperator		    Operator
  HiLink nimPreCondit	    PreCondit
  HiLink nimComment		    Comment
  HiLink nimTodo		        Todo
  HiLink nimDecorator	    Define
  HiLink nimSpecialVar	    Identifier
  
  if nim_highlight_numbers == 1
    HiLink nimNumber	Number
  endif
  
  if nim_highlight_builtins == 1
    HiLink nimBuiltin	Number
  endif
  
  if nim_highlight_exceptions == 1
    HiLink nimException	Exception
  endif
  
  if nim_highlight_space_errors == 1
    HiLink nimSpaceError	Error
  endif

  delcommand HiLink
endif

let b:current_syntax = 'nim'

