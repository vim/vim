" Vim syntax file
" Language:		Algol 68
" Version:		0.4
" Last Change:		2026 Mar 25
" Maintainer:		Janis Papanagnou
" Previous Maintainer:	NevilleD.ALGOL_68@sgr-a.net

if exists("b:current_syntax")
  finish
endif


"syn case ignore
syn sync minlines=250 maxlines=500

" Algol68 Final Report, unrevised
syn keyword algol68ProProc	PRIORITY THEF
syn keyword algol68Operator	BTB CTB CONJ QUOTE CT CTAB EITHER SIGN


" Algol68 Revised Report
syn keyword algol68Boolean	TRUE FALSE
syn keyword algol68Conditional	IF THEN ELSE ELIF FI
syn keyword algol68Conditional	CASE IN OUT OUSE ESAC
syn keyword algol68Constant	NIL SKIP EMPTY
syn keyword algol68PreProc	MODE OP PRIO PROC
syn keyword algol68Label	GOTO 
syn match algol68label          "\<GO TO\>"
syn keyword algol68Operator	ABS REPR ROUND ENTIER ARG BIN LENG SHORTEN ODD
syn keyword algol68Operator	SHL SHR UP DOWN LEVEL LWB UPB I
syn keyword algol68Operator	OVER MOD ELEM
syn keyword algol68Operator	LT LE GE GT
syn keyword algol68Operator	EQ NE
syn keyword algol68Operator	AND OR XOR NOT
syn keyword algol68PreProc	ANDF ORF ANDTH OREL ANDTHEN ORELSE
syn keyword algol68Operator	MINUSAB PLUSAB TIMESAB DIVAB OVERAB MODAB PLUSTO
syn keyword algol68Operator	IS ISNT OF AT
syn keyword algol68Operator	SORT ELEMS
syn keyword algol68Repeat	FOR FROM BY UPTO DOWNTO TO WHILE DO UNTIL OD
syn keyword algol68Statement	PAR BEGIN END EXIT
syn keyword algol68Struct	STRUCT
syn keyword algol68PreProc	VECTOR
syn keyword algol68Type		FLEX HEAP LOC LONG REF SHORT
syn keyword algol68Type		VOID BOOL INT REAL COMPL CHAR STRING COMPLEX
syn keyword algol68Type		BITS BYTES FILE CHANNEL PIPE SEMA SOUND
syn keyword algol68Type		FORMAT STRUCT UNION 

    " 20011222az: Added new items.
syn keyword algol68Todo contained	TODO FIXME XXX DEBUG NOTE

    " 20010723az: When wanted, highlight the trailing whitespace -- this is
    " based on c_space_errors; to enable, use "algol68_space_errors".
if exists("algol68_space_errors")
  if !exists("algol68_no_trail_space_error")
    syn match algol68SpaceError "\s\+$"
  endif
  if !exists("algol68_no_tab_space_error")
    syn match algol68SpaceError " \+\t"me=e-1
  endif
endif


" String
if !exists("algol68_one_line_string")
  syn region  algol68String matchgroup=algol68String start=+"+ end=+"+ contains=algol68StringEscape
  if exists("algol68_gpc")
    syn region  algol68String matchgroup=algol68String start=+'+ end=+'+ contains=algol68StringEscapeGPC
  else
    syn region  algol68StringError matchgroup=algol68StringError start=+'+ end=+'+ contains=algol68StringEscape
  endif
else
  "wrong strings
  syn region  algol68StringError matchgroup=algol68StringError start=+"+ end=+"+ end=+$+ contains=algol68StringEscape
  if exists("algol68_gpc")
    syn region  algol68StringError matchgroup=algol68StringError start=+'+ end=+'+ end=+$+ contains=algol68StringEscapeGPC
  else
    syn region  algol68StringError matchgroup=algol68StringError start=+'+ end=+'+ end=+$+ contains=algol68StringEscape
  endif

  "right strings
  syn region  algol68String matchgroup=algol68String start=+"+ end=+"+ oneline contains=algol68StringEscape
  " To see the start and end of strings:
  " syn region  algol68String matchgroup=algol68StringError start=+'+ end=+'+ oneline contains=algol68StringEscape
  if exists("algol68_gpc")
    syn region  algol68String matchgroup=algol68String start=+'+ end=+'+ oneline contains=algol68StringEscapeGPC
  else
    syn region  algol68StringError matchgroup=algol68StringError start=+'+ end=+'+ oneline contains=algol68StringEscape
  endif
end
syn match   algol68StringEscape		contained '""'
syn match   algol68StringEscapeGPC	contained "''"


syn match   algol68Identifier		"\<[a-z][a-z0-9_]*\>"


if exists("algol68_symbol_operator")
" syn match algol68SymbolOperator "₁₀"
  syn match algol68SymbolOperator "\\" 
"  syn match algol68SymbolOperator "≤" "<=" "≥" ">=" ">" "<"
"  syn match algol68SymbolOperator "≠" "/=" "~=" ":~=:" ":≠:" ":/=:" ":=:" ":=" ":=:="
""  syn match algol68SymbolOperator "[⍧¢£#]"
"  syn match algol68SymbolOperator "⌈" "⌊"
"  syn match algol68SymbolOperator "⎕"
"  syn match algol68SymbolOperator "[.][.]|‥"
"  syn match algol68SymbolOperator "¬" "~"
"  syn match algol68SymbolOperator "÷" "×"
"  syn match algol68SymbolOperator "⊥"
"  syn match algol68SymbolOperator "[○∅]"
"  syn match algol68SymbolOperator "↑" "↓"
"  syn match algol68SymbolOperator "∧" "∨" "&"
"  syn match algol68SymbolOperator "→"
"  syn match algol68SymbolOperator "[☩✠᛭]"
"  syn match algol68SymbolOperator "╭" "╰"
"9      +*, i 	+×, ⊥ 		
"8 	shl, shr, **, up, down, lwb, upb 	↑, ↓, ⌊, ⌈ 		╰, ╭
"7 	*, /, %, over, %*, mod, elem 	×, ÷, ÷×, ÷*, %×, ⌷ 		÷:
"6 	-, + 			
"5 	<, lt, <=, le, >=, ge, >, gt 	≤, ≥ 	
"4 	=, eq, /=, ne 	≠ 	~= 	
"3 	&, and 	∧ 	
"2 	or 	∨ 	
"1 	minusab, plusab, timesab, divab, overab, modab, plusto,
"
"-:=, +:=, *:=, /:=, %:=, %*:=, +=:
"	×:=, ÷:=, ÷×:=, ÷*:=, %×:= 		÷::=
"effectively 0 	 :=, =:, = , :=:, :/=:, is, isnt, of, at 	 :≠:, : 	 :~=: 	 ::, .., is not, →, @

"""" additional ALGOL characters/operators from html4 std
"  cent ¢    pound £  yen ¥    euro €    curren ¤
"  plusmn ±  minus −  times ×  divide ÷  oplus ⊕  otimes ⊗
"  larr ←    uarr ↑   rarr →   darr ↓    harr ↔   crarr ↵
"  and ∧     or ∨     not ¬
"  cong ≅    asymp ≈  prop ∝   ne ≠
"  empty ∅   infin ∞
"  perp ⊥    lceil ⌈  rceil ⌉  lfloor ⌊  rfloor ⌋
"  equiv ≡   sup ⊃    nabla ∇  loz ◊     deg °    # [[GOST 10859]] ALGOL characters
"  radic √   part ∂   int ∫    prod ∏    sum ∑    # bonus operators
""""

  syn match   algol68SymbolOperator      "[+\-/*=]"
"  syn match   algol68SymbolOperator      "[<>]=\="
  syn match   algol68SymbolOperator      "<>"
  syn match   algol68SymbolOperator      ":="
"  syn match   algol68SymbolOperator      "[()]"
  syn match   algol68SymbolOperator      "\.\."
"  syn match   algol68SymbolOperator       "[\^.]"
"  syn match   algol68MatrixDelimiter	"[][]"
  "if you prefer you can highlight the range
  syn match  algol68MatrixDelimiter	"[\d\+\.\.\d\+]"
endif

syn match  algol68Number		"-\=\<\d\+\>"
syn match  algol68Float		"-\=\<\d\+\.\d\+\>"
" add subscr 10
syn match  algol68Float		"-\=\<\d\+\.\d\+[eE\\⏨]-\=\d\+\>" 
syn match  algol68Float		"-\=\<\d\+\.\d\+₁₀-\=\d\+\>" 
"syn match  algol68HexNumber	"\$[0-9a-fA-F]\+\>"
syn match  algol68HexNumber	"\<2r[01]\+\>"
syn match  algol68HexNumber	"\<4r[0-3]\+\>"
syn match  algol68HexNumber	"\<8r[0-7]\+\>"
syn match  algol68HexNumber	"\<16r[0-7a-f]\+\>"

if exists("algol68_no_tabs")
  syn match algol68ShowTab "\t"
endif


syn region algol68Special	start="\$"  end="\$" contains=algol68String
syn region algol68Comment	start="¢"  end="¢" contains=algol68Todo,algol68SpaceError
syn region algol68Comment	start="£"  end="£" contains=algol68Todo,algol68SpaceError
syn region algol68Comment	start="#"  end="#" contains=algol68Todo,algol68SpaceError
syn region algol68Comment	start="\<CO\>"  end="\<CO\>" contains=algol68Todo,algol68SpaceError
syn region algol68Comment	start="\<COMMENT\>"  end="\<COMMENT\>" contains=algol68Todo,algol68SpaceError
syn region algol68PreProc	start="\<PR\>"  end="\<PR\>" contains=algol68Todo,algol68SpaceError
syn region algol68PreProc	start="\<PRAGMAT\>"  end="\<PRAGMAT\>" contains=algol68Todo,algol68SpaceError
" algol68r
syn region algol68Comment	start="{"  end="}" contains=algol68Todo,algol68SpaceError
syn region algol68Comment	start="{{{"  end="}}}" contains=algol68Todo,algol68SpaceError

" ALGOL 68r
syn keyword algol68PreProc DECS CONTEXT configinfo A68CONFIG KEEP FINISH USE SYSPROCS IOSTATE FORALL
" ALGOL 68c
syn keyword algol68PreProc USING ENVIRON FOREACH ASSERT

if !exists("algol68_traditional")


"  THE STANDARD ENVIRONMENT

"      Enquiries
  syn match algol68Predefined "\<\%(blank\|formfeed\|newline\|null\|tab\)\s*char\%(acter\)\?\>"
  syn match algol68Predefined "\<\%(max\s*abs\|exp\|error\)\s*char\>"
  syn match algol68Predefined "\<\%(\%(long\s*\)\?long\s*\)\?max\s*\%(bits\|int\)\>"
  syn match algol68Predefined "\<\%(\%(long\s*\)\?long\s*\)\?\%(max\|min\|small\)\s*real\>"
  syn match algol68Predefined "\<\%(\%(long\s*\)\?long\s*\)\?\%(bits\|bytes\|exp\|int\|real\)\s*width\>"
  syn match algol68Predefined "\<\%(bits\|bytes\|compl\|int\|real\)\s*\%(lengths\|shorths\)\>"
  syn match algol68Predefined "\<\%(blank\|flip\|flop\)\>"

"      Transput Files and Channels
  syn match algol68Predefined "\<stand\s*\%(in\|out\|back\|error\)\%(\s*channel\)\?\>"
  syn match algol68Predefined "\<stand\s*draw\s*channel\>"

"      Transput Event Routines
  syn match algol68Function "\<on\s*\%(\%(line\|page\|\%(logical\s*\|physical\s*\)\?file\|format\)\s*\)end\>"
  syn match algol68Function "\<on\s*\%(\%(format\|value\|open\|transput\)\s*\)error\>"

"      Connections to Files
  syn match algol68Function "\<\%(open\|establish\|append\|create\|associate\|close\|lock\|erase\|scratch\)\>"

"      Positioning on Files
  syn match algol68Function "\<new\s*line\>"
  syn match algol68Function "\<new\s*page\>"
  syn match algol68Function "\<back\s*space\>"
  syn match algol68Function "\<\%(reset\|rewind\|rewrite\|set\|seek\|space\)\>"

"      I/O on Files (Standard)
  syn match algol68Function "\<\%(get\|put\|print\|read\|write\)\%(f\|\s*bin\)\?\>"

"      I/O on Files (Algol68C)
  syn match algol68Function "\<\%(print\|read\)\s*\%(\%(long\s*\)\?long\s*\)\?\%(int\|real\|complex\|bits\)\>"
  syn match algol68Function "\<\%(print\|read\)\s*\%(bool\|char\|string\)\>"
  syn match algol68Function "\<read\s*line\>"

"      Enquiries on Files
  syn match algol68Function "\<\%(get\|put\|bin\|set\|reset\|rewind\|reidf\|draw\)\s*possible\>"
  syn match algol68Function "\<end\s*of\s*\%(file\|line\)\>"
  syn match algol68Function "\<\%(make\s*\)\?term\>"
  syn match algol68Function "\<\%(compressible\|eof\|eoln\)\>"

"      Keyboard Control
  syn match algol68Function "\<\%(cooked\|raw\)\>"

"      Math Constants
  syn match algol68Predefined "\<\%(\%(long\s*\)\?long\s*\)\?\%(min\s*real\|\%(minus\s*\)\?infinity\|\%(min\s*\)\?inf\)\>"
  syn match algol68Predefined "\<\%(\%(\%(long\s*\)\?long\s*\)\|[qd]\)\?pi\>"
  syn match algol68Predefined "\<mp\s*radix\>"
  syn match algol68Predefined "\<nan\>"

"      Math Basic Functions
  syn match algol68Function "\<\%(\%(\%(long\s*\)\?long\s*\)\|[qd]\)\?\%(sqrt\|cbrt\|curt\|exp\|ln\|log\)\>"
  syn match algol68Function "\<ln\s*abs\>"

"      Math Trigonometric Functions
  syn match algol68Function "\<\%(\%(long\s*\)\?long\s*\)\?\%(arc\s*\|a\)\?\%(sin\|cos\|tan\|cot\|sec\|csc\)\%(h\|\%(\s*dg\)\)\?\>"
  syn match algol68Function "\<\%(\%(\%(long\s*\)\?long\s*\)\|[qd]\)\?\%(arc\s*\|a\)\?tan2\%(\s*dg\)\?\>"
  " long-long-sinpi/cospi/tanpi/cotpi
  syn match algol68Function "\<\%(\%(long\s*\)\?long\s*\)\?\%(sin\|cos\|tan\|cot\)\s*pi\>"
  syn match algol68Function "\<ln\s*\%(sinh\|cosh\)\>"
  " a special case in Genie?
  syn match algol68Function "\<atan\s*int\>"

"      Random Number Generator
  syn match algol68Function "\<\%(\%(long\s*\)\?long\s*\)\?\%(next\s*\)\?random\>"
  syn match algol68Function "\<first\s*random\>"
  syn match algol68Function "\<rnd\>"

"      Garbage Collection and Memory
  syn match algol68Function "\<collect\s*seconds\>"
  syn match algol68Function "\<garbage\%(\s*\%(collections\|freed\|refused\|seconds\)\)\?\>"
  syn match algol68Function "\<gc\s*heap\>"
  syn match algol68Function "\<on\s*gc\s*event\>"
  syn match algol68Function "\<preemptive\s*\%(gc\|sweep\%(\s*heap\)\?\)\>"
  syn match algol68Function "\<sweep\s*heap\>"
  syn match algol68Function "\<sweeps\%(\s*refused\)\?\>"
  syn match algol68Function "\<\%(system\s*\)\?\%(heap\|stack\)\s*pointer\>"
  syn match algol68Function "\<\%(actual\|system\)\s*stack\s*size\>"
  syn match algol68Function "\<\%(blocks\|collections\)\>"

"      I/O on Strings
  syn match algol68Function "\<\%(puts\|gets\|string\)f\?\>"
"      Character Type Tests
  syn match algol68Function "\<is\s*\%(alnum\|alpha\|cntrl\|digit\|graph\|lower\|print\|punct\|space\|upper\|xdigit\)\>"
"      Operations on Characters
  syn match algol68Function "\<to\s*\%(upper\|lower\)\>"
"      Search in Strings
  syn match algol68Function "\<\%(char\|last\s*char\|string\)\s*in\s*string\>"

"      Time and Date
  syn match algol68Function "\<\%(cpu\|wall\|utc\|local\)\s*time\>"
  syn match algol68Function "\<\%(wall\s*\)\?clock\>"
  syn match algol68Function "\<\%(wall\s*\)\?seconds\>"
  syn match algol68Function "\<sleep\>"

"      Type Operations
  syn match algol68Function "\<\%(long\s*\)\?\%(bits\|bytes\)\s*pack\>"
  syn match algol68Function "\<\%(long\s*long\s*\)\?bits\s*pack\>"
  syn match algol68Function "\<\%(bits\|whole\|fixed\|float\|real\)\>"

"      Runtime
  syn match algol68Function "\<\%(program\s*\)\?idf\>"
  syn match algol68Function "\<\%(backtrace\|break\|debug\|monitor\|abend\|evaluate\|system\)\>"
  syn match algol68Function "\<\%(i32\|i64\|r64\|r128\)mach\>"


"  UNIX EXTENSIONS

"      Environment Functions
  syn match algol68Function "\<\%(a68g\s*\)\?\%(argc\|argv\)\>"
  syn match algol68Function "\<get\s*env\>"
  syn match algol68Function "\<reset\s*errno\>"
  syn match algol68Function "\<str\s*error\>"
  syn match algol68Function "\<\%(get\|set\)\s*pwd\>"
  syn match algol68Function "\<\%(rows\|columns\|abend\)\>"
  syn match algol68Predefined "\<errno\>"

"      Processes
  syn match algol68Function "\<execve\%(\s*child\%(\s*pipe\)\?\|\s*output\)\?\>"
  syn match algol68Function "\<exec\%(\s*sub\%(\s*pipeline\|\s*output\)\?\)\?\>"
  syn match algol68Function "\<fork\>"
  syn match algol68Function "\<wait\s*pid\>"
  syn match algol68Function "\<create\s*pipe\>"
  syn match algol68Function "\<peek\s*char\>"
  syn match algol68Function "\<sig\s*segv\>"

"      File types and attributes
  syn match algol68Function "\<file\s*is\s*\%(block\s*device\|char\s*device\|directory\|regular\|fifo\|link\)\>"
  syn match algol68Function "\<file\s*mode\>"
  syn match algol68Function "\<get\s*directory\>"
  syn match algol68Function "\<real\s*path\>"

"      Fetching web page contents and sending requests
  syn match algol68Function "\<https\?\s*\%(content\|timeout\)\>"
  syn match algol68Function "\<tcp\s*request\>"

"      Regular expressions in string manipulation
  syn match algol68Function "\<grep\s*in\s*\%(sub\)\?string\>"
  syn match algol68Function "\<sub\s*in\s*string\>"

"      Curses support
  syn match algol68Function "\<curses\s*\%(start\|end\|clear\|refresh\|get\s*char\|put\s*char\|move\|lines\|columns\)\>"
  syn match algol68Function "\<curses\s*\%(green\|cyan\|red\|yellow\|magenta\|blue\|white\)\%(inverse\)\?\>"
  syn match algol68Function "\<curses\s*del\s*char\>"


"  POSTGRESQL CLIENT ROUTINES

"      Connecting to a server
  syn match algol68Function "\<pq\s*\%(connect\s*db\|finish\|reset\|parameter\s*status\)\>"

"      Sending queries and retrieving results
  syn match algol68Function "\<pq\s*\%(exec\|ntuples\|nfields\|fname\|fnumber\|fformat\|get\s*is\s*null\|get\s*value\|cmd\s*status\|cmd\s*tuples\)\>"

"      Connection status information
  syn match algol68Function "\<pq\s*\%(\%(result\s*\)\?error\s*message\|db\|user\|pass\|host\|port\|tty\|options\|\%(protocol\|server\)\s*version\|socket\|backend\s*pid\)\>"


"  SOUND

  syn match algol68Function "\<\%(new\|get\|set\)\s*sound\>"
  syn keyword algol68Operator RESOLUTION CHANNELS RATE SAMPLES


"  DRAWING USING THE GNU PLOTTING UTILITIES

"      Setting up a graphics device
  syn match algol68Function "\<draw\s*\%(device\|erase\|show\|move\|aspect\|fill\s*style\|line\s*style\|line\s*width\|clear\|flush\)\>"
  syn match algol68Function "\<make\s*device\>"

"      Specifying colours
  syn match algol68Function "\<draw\s*\%(\%(background\s*\)\?colou\?r\%(\s*name\)\?\)\>"
  syn match algol68Function "\<draw\s*get\s*colou\?r\s*name\>"

"      Drawing objects
  syn match algol68Function "\<draw\s*\%(point\|line\|rect\|circle\|ball\|star\)\>"

"      Drawing text
  syn match algol68Function "\<draw\s*\%(text\%(\s*angle\)\?\|font\s*\%(name\|size\)\)\>"


"  EXTRA NUMERICAL PROCEDURES

"      COMPLEX Functions
  syn match algol68Function "\<\%(\%(long\s*\)\?long\s*\)\?complex\s*\%(sqrt\|exp\|ln\)\>"
  syn match algol68Function "\<\%(\%(long\s*\)\?long\s*\)\?complex\s*\%(arc\s*\)\?\%(sin\|cos\|tan\)h\?\>"
  " cas casin casinh dcas dcasin dcasinh qcas qcasin qcasinh longcas longlongcas
  syn match algol68Function "\<\%(\%(\%(long\s*\)\?long\s*\)\|[dq]\?\)cas\%(inh\?\)\?\>"
  " a special case in Genie?
  syn match algol68Function "\<long\s*complex\s*atanh\>"

"      REAL Airy Functions
  syn match algol68Function "\<airy\s*[ab]i\%(\s*derivative\)\?\>"
  syn match algol68Function "\<airy\s*[ab]i\%(\s*deriv\)\?\%(\s*scaled\)\?\>"
  syn match algol68Function "\<airy\s*zero\s*[ab]i\%(\s*deriv\)\?\>"

"      REAL Bessel Functions
  syn match algol68Function "\<bessel\s*\%(jn\|yn\|in\|exp\s*in\|kn\|exp\s*kn\|jl\|yl\|exp\s*il\|exp\s*kl\|jnu\|ynu\|inu\|exp\s*inu\|knu\|exp\s*knu\)\>"

  " only a few could be sensibly merged; we keep them apart
  syn match algol68Function "\<bessel\s*\%(il[012]\?\s*scaled\)\>"
  syn match algol68Function "\<bessel\s*\%(in[01]\%(\s*scaled\)\?\)\>"
  syn match algol68Function "\<bessel\s*\%(in\s*u\?scaled\)\>"
  syn match algol68Function "\<bessel\s*\%(j\%(\l[012]\|n[01]\)\)\>"
  syn match algol68Function "\<bessel\s*\%(kl[012]\?\s*scaled\)\>"
  syn match algol68Function "\<bessel\s*\%(kn[01]\%(\s*scaled\)\?\)\>"
  syn match algol68Function "\<bessel\s*\%(kn\s*[u_]\?scaled\)\>"
  syn match algol68Function "\<bessel\s*lnknu\>"
  syn match algol68Function "\<bessel\s*\%(y\%(\l[012]\|n[01]\)\)\>"
  syn match algol68Function "\<bessel\s*zero\s*j\%([01]\|nu\)\>"

"      REAL Elliptic Integrals
  syn match algol68Function "\<elliptic\s*integral\s*\%(k\|e\|rf\<rd\|rj\|rc\)\>"

"      REAL Error and Gamma Functions
  syn match algol68Function "\<\%(ln\s*\)\?\%(fact\|choose\)\>"
  syn match algol68Function "\<prime\s*factors\>"
  syn match algol68Function "\<\%(\%(long\s*\)\?long\s*\)\?\%(inv\%(erse\)\?\s*\)\?erfc\?\>"
  syn match algol68Function "\<mpfr\s*\%(\%(long\s*\)\?long\s*\)\?\%(inv\s*\)\?erfc\?\>"
  syn match algol68Function "\<\%(\%(\%(\%(mpfr\s*\)\?long\s*\)\?long\s*\)\|\%(d\|\%(mpfr\s*\)\?q\)\)\?\%(beta\|gamma\)\%(inc\s*g\?f\?\)\?\>"
  syn match algol68Function "\<\%(\%(\%(\%(mpfr\s*\)\?long\s*\)\?long\s*\)\|\%(d\|\%(mpfr\s*\)\?q\)\)\?ln\s*\%(beta\|gamma\)\>"
  syn match algol68Function "\<mpfr\s*mp\>"
  " is the following a special case in Genie?
  syn match algol68Function "\<mpfr\s*\%(long\s*\|d\)gamma\s*inc\>"
  syn match algol68Function "gamma\s*\%(\%(inc\s*\%(gsl\|[pq]\)\)\|inv\|star\)\>"
  syn match algol68Function "\<lj[ef]\s*126\>"
  syn match algol68Function "\<ln1p\>"



"      Scaling Factors

  " strangely missing some common factors (hecto, deca, deci, centi),
  " also myria, and the more extreme factors (quetta, ronna, ronto, quecto)
  syn match algol68Predefined "\<num\s*\%(yotta\|zetta\|exa\|peta\|tera\|giga\|mega\|kilo\|milli\|micro\|nano\|pico\|femto\|atto\|zepto\|yocto\)\>"


"      Physical Constants

"          Fundamental Constants
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(boltzmann\|faraday\|gauss\|hectare\|\%(kilometers\|miles\)\s*per\s*hour\|micron\|molar\s*gas\|planck\s*constant\%(\s*bar\)\?\|speed\s*of\s*light\|standard\s*gas\s*volume\)\>"
  syn match algol68Predefined "\<mksa\s*vacuum\s*\%(permeability\|permittivity\)\>"
  syn match algol68Predefined "\<num\s*avogadro\>"

"          Astronomy and Astrophysics
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(astronomical\s*unit\|grav\s*accel\|gravitational\s*constant\|light\s*year\|parsec\|solar\s*mass\)\>"

"          Atomic and Nuclear Physics
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(angstrom\|barn\|bohr\s*magneton\|bohr\s*radius\|electron\s*\%(charge\|magnetic\s*moment\|volt\)\|mass\s*\%(electron\|muon\|neutron\|proton\)\|nuclear\s*magneton\|proton\s*magnetic\s*moment\|rydberg\|unified\s*atomic\s*mass\)\>"
  syn match algol68Predefined "\<num\s*fine\s*structure\>"

"          Time
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(day\|hour\|minute\|week\)\>"

"          Imperial units
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(foot\|inch\|mil\|mile\|yard\|\%(tex\)\?point\)\>"

"          Nautical units
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(fathom\|knot\|nautical\s*mile\)\>"

"          Volume
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(acre\|\%(canadian\|uk\|us\)\s*gallon\|liter\|pint\|quart\|cup\|fluid\s*ounce\|\%(table\|tea\)\s*spoon\)\>"

"          Mass and weight
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(carat\|\%(gram\|\%(kilo\s*\)\?pound\)\s*force\|\%(metric\s*\|uk\s*\)\?ton\|\%(ounce\|pound\)\s*mass\|poundal\|troy\s*ounce\)\>"

"          Thermal energy and power
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(btu\|calorie\|horsepower\|therm\)\>"

"          Pressure
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(bar\|inch\s*of\s*\%(mercury\|water\)\|meter\s*of\s*mercury\|psi\|std\s*atmosphere\|torr\)\>"

"          Viscosity
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(poise\|stokes\)\>"

"          Light and illumination
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(footcandle\|footlambert\|lambert\|lumen\|lux\|phot\|stilb\)\>"

"          Radioactivity
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(curie\|rad\|roentgen\)\>"

"          Force and energy
  syn match algol68Predefined "\<\%(cgs\|mksa\)\s*\%(dyne\|erg\|joule\|newton\)\>"


" Functions from GSL

  syn match algol68Function "\<angle\s*restrict\s*\%(pos\|symm\)\>"
  syn match algol68Function "\<conicalp\s*\%([01]\|cylreg\|m\?half\|sph\s*reg\)\>"
  syn match algol68Function "\<cholesky\s*\%(decomp\|solve\)\>"
  syn match algol68Function "\<debye\s*[1-6]\>"
  syn match algol68Function "\<ellint\s*\%([defp]\|[ekp]\s*comp\|r[cdfj]\)\>"
  syn match algol68Function "\<\%(expint\s*\%(3\|e[12in]\)\|expm1\|exprel[2n]\?\)\>"
  syn match algol68Function "\<fermi\s*dirac\s*\%([012]\|3\?half\|inc0\|int\|m1\|mhalf\)\>"
  syn match algol68Function "\<fft\s*\%(complex\s*\)\?\%(forward\|backward\|inverse\)\>"
  syn match algol68Function "\<\%(gegenpoly\|laguerre\)\s*[123n]\s*real\>"
  syn match algol68Function "\<lambert\s*\%(w0\|wm1\)\>"
  syn match algol68Function "\<legendre\s*\%(h3d\%([01]\)\?\|p[123l]\|q[01l]\)\>"
  syn match algol68Function "\<pseudo\s*inv\>"
  syn match algol68Function "\<psi\s*\%(1\%(int\|ply\)\?\|int\|n\)\?\>"
  syn match algol68Function "\<synchrotron\s*[12]\>"
  syn match algol68Function "\<taylor\s*coeff\>"
  syn match algol68Function "\<transport\s*[2-5]\>"
  syn match algol68Function "\<zeta\%(\s*m1\)\?\%(\s*int\)\?\>"
  syn match algol68Function "\<\%(chi\|ci\|clausen\|dawson\|digamma\|dilog\|doublefact\|eta\|eta\s*int\|hermite\s*func\|hypot\|hzeta\|laplace\|shi\|si\|sinc\)\>"
  syn match algol68Function "\<ln1\s*\%(plusx\%(mx\)\?\)\?\>"
  syn match algol68Function "\<\%(compl\s*\)\?\%(matrix\|vector\)\s*echo\>"
  syn match algol68Function "\<print\s*\%(matrix\|vector\)\>"
  syn match algol68Function "\<\%(complex\s*\)\?lu\s*\%(decomp\|det\|inv\|solve\)\>"
  syn match algol68Function "\<left\s*columns\>"
  syn match algol68Function "\<\%(ols\|tls\|pcacv\|pcasvd\|pcr\|pls[12]\)\>"
  syn match algol68Function "\<\%(lnpoch\|poch\s*\%(rel\)\?\)\>"
  syn match algol68Function "\<qr\s*\%(decomp\|\%(ls\s*\)\?solve\)\>"
  syn match algol68Function "\<svd\s*\%(decomp\|solve\)\>"


" Functions from R Mathlib

  syn match algol68Function "\<r\s*[dpqr]n\?\s*binom\>"
  syn match algol68Function "\<r\s*\%(di\|tri\|tetra\|penta\|psi\)\s*gamma\>"
  " note: Genie documents 'r rn chisq' but it's missing in the code?
  syn match algol68Function "\<r\s*[dpqr]n\?\s*chisq\>"
  syn match algol68Function "\<r\s*[dpqr]\%(\s*n\)\?\s*f\>"
  syn match algol68Function "\<r\s*[dpq]\%(\s*n\)\?\s*t\>"
  syn match algol68Function "\<r\s*[dpqr]\s*\%(l\s*\)\?norm\>"
  syn match algol68Function "\<r\s*[dpqr]\s*\%(beta\|cauchy\|exp\|geom\|hyper\|logis\|pois\|sign\s*rank\|t\|unif\|weibull\|wilcox\)\>"
  syn match algol68Function "\<r\s*[pq]\s*tu\s*key\>"


endif

" Define the default highlighting.
hi def link algol68Acces		algol68Statement
hi def link algol68Boolean		Boolean
hi def link algol68Comment		Comment
hi def link algol68Conditional		Conditional
hi def link algol68Constant		Constant
hi def link algol68Delimiter		Identifier
hi def link algol68Directive		algol68Statement
hi def link algol68Exception		Exception
hi def link algol68Float		Float
hi def link algol68Function		Function
hi def link algol68Label		Label
hi def link algol68MatrixDelimiter	Identifier
hi def link algol68Modifier		Type
hi def link algol68HexNumber		Number
hi def link algol68Number		Number
hi def link algol68Operator		Operator
hi def link algol68Predefined		algol68Statement
hi def link algol68PreProc		PreProc
hi def link algol68Repeat		Repeat
hi def link algol68SpaceError		Error
hi def link algol68Statement		Statement
hi def link algol68String		String
hi def link algol68Format	        Special
hi def link algol68StringEscape		Special
hi def link algol68StringEscapeGPC	Special
hi def link algol68StringError		Error
hi def link algol68Struct		algol68Statement
hi def link algol68SymbolOperator	algol68Operator
hi def link algol68Todo			Todo
hi def link algol68Type			Type
hi def link algol68Unclassified		algol68Statement
"  hi def link algol68Asm		Assembler
hi def link algol68Error		Error
hi def link algol68AsmKey		algol68Statement
hi def link algol68ShowTab		Error

let b:current_syntax = "algol68"

" vim: ts=8 sw=2
