" Vim syntax file
" Language:	    Terminfo definition
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/terminfo/
" Latest Revision:  2004-05-22
" arch-tag:	    8464dd47-0c5a-47d5-87ed-a2ad99e1196f

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" keywords (define first as to not mess up comments
syn match terminfoKeywords	"[,=#|]"

" todo
syn keyword terminfoTodo	contained TODO FIXME XXX NOTE

" comments
syn region  terminfoComment	matchgroup=terminfoComment start="^#" end="$" contains=terminfoTodo

" numbers
syn match   terminfoNumbers	"\<[0-9]\+\>"

" special keys
syn match   terminfoSpecialChar	"\\\(\o\{3}\|[Eenlrtbfs^\,:0]\)"
syn match   terminfoSpecialChar "\^\a"

" delays
syn match   terminfoDelay	"$<[0-9]\+>"

" boolean capabilities
syn keyword terminfoBooleans	bw am bce ccc xhp xhpa cpix crxw xt xenl eo gn
syn keyword terminfoBooleans	hc chts km daisy hs hls in lpix da db mir msgr
syn keyword terminfoBooleans	nxon xsb npc ndscr nrrmc os mc5i xcpa sam eslok
syn keyword terminfoBooleans	hz ul xon

" numeric capabilities
syn keyword terminfoNumerics	cols it lh lw lines lm xmc ma colors pairs wnum
syn keyword terminfoNumerics	ncv nlab pb vt wsl bitwin bitype bufsz btns
syn keyword terminfoNumerics	spinh spinv maddr mjump mcs npins orc orhi orl
syn keyword terminfoNumerics	orvi cps widcs

" string capabilities
syn keyword terminfoStrings	acsc cbt bel cr cpi lpi chr cvr csr rmp tbc mgc
syn keyword terminfoStrings	clear el1 el ed hpa cmdch cwin cup cud1 home
syn keyword terminfoStrings	civis cub1 mrcup cnorm cuf1 ll cuu1 cvvis defc
syn keyword terminfoStrings	dch1 dl1 dial dsl dclk hd enacs smacs smam blink
syn keyword terminfoStrings	bold smcup smdc dim swidm sdrfq smir sitm slm
syn keyword terminfoStrings	smicm snlq snrmq prot rev invis sshm smso ssubm
syn keyword terminfoStrings	ssupm smul sum smxon ech rmacs rmam sgr0 rmcup
syn keyword terminfoStrings	rmdc rwidm rmir ritm rlm rmicm rshm rmso rsubm
syn keyword terminfoStrings	rsupm rmul rum rmxon pause hook flash ff fsl
syn keyword terminfoStrings	wingo hup is1 is2 is3 if iprog initc initp ich1
syn keyword terminfoStrings	il1 ip ka1 ka3 kb2 kbs kbeg kcbt kc1 kc3 kcan
syn keyword terminfoStrings	ktbc kclr kclo kcmd kcpy kcrt kctab kdch1 kdl1
syn keyword terminfoStrings	kcud1 krmir kend kent kel ked kext
syn match   terminfoStrings	"\<kf\([0-9]\|[0-5][0-9]\|6[0-3]\)\>"
syn keyword terminfoStrings	kfnd khlp khome kich1 kil1 kcub1 kll kmrk
syn keyword terminfoStrings	kmsg kmov knxt knp kopn kopt kpp kprv kprt krdo
syn keyword terminfoStrings	kref krfr krpl krst kres kcuf1 ksav kBEG kCAN
syn keyword terminfoStrings	kCMD kCPY kCRT kDC kDL kslt kEND kEOL kEXT kind
syn keyword terminfoStrings	kFND kHLP kHOM kIC kLFT kMSG kMOV kNXT kOPT kPRV
syn keyword terminfoStrings	kPRT kri kRDO kRPL kRIT kRES kSAV kSPD khts kUND
syn keyword terminfoStrings	kspd kund kcuu1 rmkx smkx lf0 lf1 lf10 lf2 lf3
syn keyword terminfoStrings	lf4 lf5 lf6 lf7 lf8 lf9 fln rmln smln rmm smm
syn keyword terminfoStrings	mhpa mcud1 mcub1 mcuf1 mvpa mcuu1 nel porder oc
syn keyword terminfoStrings	op pad dch dl cud mcud ich indn il cub mcub cuf
syn keyword terminfoStrings	mcuf rin cuu mccu pfkey pfloc pfx pln mc0 mc5p
syn keyword terminfoStrings	mc4 mc5 pulse qdial rmclk rep rfi rs1 rs2 rs3 rf
syn keyword terminfoStrings	rc vpa sc ind ri scs sgr setbsmgb smgbp sclk scp
syn keyword terminfoStrings	setb setf smgl smglp smgr smgrp hts smgt smgtp
syn keyword terminfoStrings	wind sbim scsd rbim rcsd subcs supcs ht docr
syn keyword terminfoStrings	tsl tone uc hu
syn match   terminfoStrings	"\<u[0-9]\>"
syn keyword terminfoStrings	wait xoffc xonc zerom
syn keyword terminfoStrings	scesa bicr binel birep csnm csin colornm defbi
syn keyword terminfoStrings	devt dispc endbi smpch smsc rmpch rmsc getm
syn keyword terminfoStrings	kmous minfo pctrm pfxl reqmp scesc s0ds s1ds
syn keyword terminfoStrings	s2ds s3ds setab setaf setcolor smglr slines
syn keyword terminfoStrings	smgtb ehhlm elhlm erhlm ethlm evhlm sgr1
syn keyword terminfoStrings	slengthsL

" parameterized strings
syn match terminfoParameters	"%[%dcspl+*/mAO&|^=<>!~i?te;-]"
syn match terminfoParameters	"%\('[A-Z]'\|{[0-9]\{1,2}}\|p[1-9]\|P[a-z]\|g[A-Z]\)"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_terminfo_syn_inits")
  if version < 508
    let did_terminfo_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink terminfoComment	Comment
  HiLink terminfoTodo		Todo
  HiLink terminfoNumbers	Number
  HiLink terminfoSpecialChar	SpecialChar
  HiLink terminfoDelay		Special
  HiLink terminfoBooleans	Type
  HiLink terminfoNumerics	Type
  HiLink terminfoStrings	Type
  HiLink terminfoParameters	Keyword
  HiLink terminfoKeywords	Keyword
  delcommand HiLink
endif

let b:current_syntax = "terminfo"

" vim: set sts=2 sw=2:
