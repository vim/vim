" Vim syntax file
" Language:         TeX (core definition)
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-03-26

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

" This follows the grouping (sort of) found at
" http://www.tug.org/utilities/plain/cseq.html#top-fam

syn keyword coretexTodo                         TODO FIXME XXX NOTE

syn match coretexComment                        display contains=coretexTodo
      \ '\\\@<!\%(\\\\\)*\zs%.*$'

syn match   coretexDimension                    display contains=@NoSpell
      \ '[+-]\=\s*\%(\d\+\%([.,]\d*\)\=\|[.,]\d\+\)\s*\%(true\)\=\s*\%(p[tc]\|in\|bp\|c[mc]\|m[mu]\|dd\|sp\|e[mx]\)\>'

syn cluster coretexBox
      \ contains=coretexBoxCommand,coretexBoxInternalQuantity,
      \ coretexBoxParameterDimen,coretexBoxParameterInteger,
      \ coretexBoxParameterToken

syn cluster coretexCharacter
      \ contains=coretexCharacterCommand,coretexCharacterInternalQuantity,
      \ coretexCharacterParameterInteger

syn cluster coretexDebugging
      \ contains=coretexDebuggingCommand,coretexDebuggingParameterInteger,
      \ coretexDebuggingParameterToken

syn cluster coretexFileIO
      \ contains=coretexFileIOCommand,coretexFileIOInternalQuantity,
      \ coretexFileIOParameterToken

syn cluster coretexFonts
      \ contains=coretexFontsCommand,coretexFontsInternalQuantity

syn cluster coretexGlue
      \ contains=coretexGlueCommand,coretexGlueDerivedCommand

syn cluster coretexHyphenation
      \ contains=coretexHyphenationCommand,coretexHyphenationDerivedCommand,
      \ coretexHyphenationInternalQuantity,coretexHyphenationParameterInteger

syn cluster coretexInserts
      \ contains=coretexInsertsCommand,coretexInsertsParameterDimen,
      \ coretexInsertsParameterGlue,coretexInsertsParameterInteger

syn cluster coretexJob
      \ contains=coretexJobCommand,coretexJobInternalQuantity,
      \ coretexJobParameterInteger

syn cluster coretexKern
      \ contains=coretexKernCommand,coretexKernInternalQuantity

syn cluster coretexLogic
      \ contains=coretexLogicCommand

syn cluster coretexMacro
      \ contains=coretexMacroCommand,coretexMacroDerivedCommand,
      \ coretexMacroParameterInteger

syn cluster coretexMarks
      \ contains=coretexMarksCommand

syn cluster coretexMath
      \ contains=coretexMathCommand,coretexMathDerivedCommand,
      \ coretexMathInternalQuantity,coretexMathParameterDimen,
      \ coretexMathParameterGlue,coretexMathParameterInteger,
      \ coretexMathParameterMuglue,coretexMathParameterToken

syn cluster coretexPage
      \ contains=coretexPageInternalQuantity,coretexPageParameterDimen,
      \ coretexPageParameterGlue

syn cluster coretexParagraph
      \ contains=coretexParagraphCommand,coretexParagraphInternalQuantity,
      \ coretexParagraphParameterDimen,coretexParagraphParameterGlue,
      \ coretexParagraphParameterInteger,coretexParagraphParameterToken

syn cluster coretexPenalties
      \ contains=coretexPenaltiesCommand,coretexPenaltiesInternalQuantity,
      \ coretexPenaltiesParameterInteger

syn cluster coretexRegisters
      \ contains=coretexRegistersCommand,coretexRegistersInternalQuantity

syn cluster coretexTables
      \ contains=coretexTablesCommand,coretexTablesParameterGlue,
      \ coretexTablesParameterToken

syn cluster coretexCommand
      \ contains=coretexBoxCommand,coretexCharacterCommand,
      \ coretexDebuggingCommand,coretexFileIOCommand,
      \ coretexFontsCommand,coretexGlueCommand,
      \ coretexHyphenationCommand,coretexInsertsCommand,
      \ coretexJobCommand,coretexKernCommand,coretexLogicCommand,
      \ coretexMacroCommand,coretexMarksCommand,coretexMathCommand,
      \ coretexParagraphCommand,coretexPenaltiesCommand,coretexRegistersCommand,
      \ coretexTablesCommand

syn match   coretexBoxCommand                   display contains=@NoSpell
      \ '\\\%([hv]\=box\|[cx]\=leaders\|copy\|[hv]rule\|lastbox\|setbox\|un[hv]\%(box\|copy\)\|vtop\)\>'
syn match   coretexCharacterCommand             display contains=@NoSpell
      \ '\\\%([] ]\|\%(^^M\|accent\|char\|\%(lower\|upper\)case\|number\|romannumeral\|string\)\>\)'
syn match   coretexDebuggingCommand             display contains=@NoSpell
      \ '\\\%(\%(batch\|\%(non\|error\)stop\|scroll\)mode\|\%(err\)\=message\|meaning\|show\%(box\%(breadth\|depth\)\=\|lists\|the\)\)\>'
syn match   coretexFileIOCommand                display contains=@NoSpell
      \ '\\\%(\%(close\|open\)\%(in\|out\)\|endinput\|immediate\|input\|read\|shipout\|special\|write\)\>'
syn match   coretexFontsCommand                 display contains=@NoSpell
      \ '\\\%(/\|fontname\)\>'
syn match   coretexGlueCommand                  display contains=@NoSpell
      \ '\\\%([hv]\|un\)skip\>'
syn match   coretexHyphenationCommand           display contains=@NoSpell
      \ '\\\%(discretionary\|hyphenation\|patterns\|setlanguage\)\>'
syn match   coretexInsertsCommand               display contains=@NoSpell
      \ '\\\%(insert\|split\%(bot\|first\)mark\|vsplit\)\>'
syn match   coretexJobCommand                   display contains=@NoSpell
      \ '\\\%(dump\|end\|jobname\)\>'
syn match   coretexKernCommand                  display contains=@NoSpell
      \ '\\\%(kern\|lower\|move\%(left\|right\)\|raise\|unkern\)\>'
syn match   coretexLogicCommand                 display contains=@NoSpell
      \ '\\\%(else\|fi\|if[a-zA-Z@]\+\|or\)\>'
"      \ '\\\%(else\|fi\|if\%(case\|cat\|dim\|eof\|false\|[hv]box\|[hmv]mode\|inner\|num\|odd\|true\|void\|x\)\=\|or\)\>'
syn match   coretexMacroCommand                 display contains=@NoSpell
      \ '\\\%(after\%(assignment\|group\)\|\%(begin\|end\)group\|\%(end\)\=csname\|e\=def\|expandafter\|futurelet\|global\|let\|long\|noexpand\|outer\|relax\|the\)\>'
syn match   coretexMarksCommand                 display contains=@NoSpell
      \ '\\\%(bot\|first\|top\)\=mark\>'
syn match   coretexMathCommand                  display contains=@NoSpell
      \ '\\\%(abovewithdelims\|delimiter\|display\%(limits\|style\)\|l\=eqno\|left\|\%(no\)\=limits\|math\%(accent\|bin\|char\|choice\|close\|code\|inner\|op\|open\|ord\|punct\|rel\)\|mkern\|mskip\|muskipdef\|nonscript\|\%(over\|under\)line\|radical\|right\|\%(\%(script\)\{1,2}\|text\)style\|vcenter\)\>'
syn match   coretexParagraphCommand             display contains=@NoSpell
      \ '\\\%(ignorespaces\|indent\|no\%(boundary\|indent\)\|par\|vadjust\)\>'
syn match   coretexPenaltiesCommand             display contains=@NoSpell
      \ '\\\%(un\)\=penalty\>'
syn match   coretexRegistersCommand             display contains=@NoSpell
      \ '\\\%(advance\|\%(count\|dimen\|skip\|toks\)def\|divide\|multiply\)\>'
syn match   coretexTablesCommand                display contains=@NoSpell
      \ '\\\%(cr\|crcr\|[hv]align\|noalign\|omit\|span\)\>'

syn cluster coretexDerivedCommand
      \ contains=coretexGlueDerivedCommand,coretexHyphenationDerivedCommand,
      \ coretexMacroDerivedCommand,coretexMathDerivedCommand

syn match   coretexGlueDerivedCommand           display contains=@NoSpell
      \ '\\\%([hv]fil\%(l\|neg\)\=\|[hv]ss\)\>'
syn match   coretexHyphenationDerivedCommand    display contains=@NoSpell
      \ '\\-'
syn match   coretexMacroDerivedCommand          display contains=@NoSpell
      \ '\\[gx]def\>'
syn match   coretexMathDerivedCommand           display contains=@NoSpell
      \ '\\\%(above\|atop\%(withdelims\)\=\|mathchardef\|over\|overwithdelims\)\>'

syn cluster coretexInternalQuantity
      \ contains=coretexBoxInternalQuantity,coretexCharacterInternalQuantity,
      \ coretexFileIOInternalQuantity,coretexFontsInternalQuantity,
      \ coretexHyphenationInternalQuantity,coretexJobInternalQuantity,
      \ coretexKernInternalQuantity,coretexMathInternalQuantity,
      \ coretexPageInternalQuantity,coretexParagraphInternalQuantity,
      \ coretexPenaltiesInternalQuantity,coretexRegistersInternalQuantity

syn match   coretexBoxInternalQuantity          display contains=@NoSpell
      \ '\\\%(badness\|dp\|ht\|prevdepth\|wd\)\>'
syn match   coretexCharacterInternalQuantity    display contains=@NoSpell
      \ '\\\%(catcode\|chardef\|\%([ul]c\|sf\)code\)\>'
syn match   coretexFileIOInternalQuantity       display contains=@NoSpell
      \ '\\inputlineno\>'
syn match   coretexFontsInternalQuantity        display contains=@NoSpell
      \ '\\\%(font\%(dimen\)\=\|nullfont\)\>'
syn match   coretexHyphenationInternalQuantity  display contains=@NoSpell
      \ '\\hyphenchar\>'
syn match   coretexJobInternalQuantity          display contains=@NoSpell
      \ '\\deadcycles\>'
syn match   coretexKernInternalQuantity         display contains=@NoSpell
      \ '\\lastkern\>'
syn match   coretexMathInternalQuantity         display contains=@NoSpell
      \ '\\\%(delcode\|mathcode\|muskip\|\%(\%(script\)\{1,2}\|text\)font\|skewchar\)\>'
syn match   coretexPageInternalQuantity         display contains=@NoSpell
      \ '\\page\%(depth\|fil\{1,3}stretch\|goal\|shrink\|stretch\|total\)\>'
syn match   coretexParagraphInternalQuantity    display contains=@NoSpell
      \ '\\\%(prevgraf\|spacefactor\)\>'
syn match   coretexPenaltiesInternalQuantity    display contains=@NoSpell
      \ '\\lastpenalty\>'
syn match   coretexRegistersInternalQuantity    display contains=@NoSpell
      \ '\\\%(count\|dimen\|skip\|toks\)\d\+\>'

syn cluster coretexParameterDimen
      \ contains=coretexBoxParameterDimen,coretexInsertsParameterDimen,
      \ coretexMathParameterDimen,coretexPageParameterDimen,
      \ coretexParagraphParameterDimen

syn match   coretexBoxParameterDimen            display contains=@NoSpell
      \ '\\\%(boxmaxdepth\|[hv]fuzz\|overfullrule\)\>'
syn match   coretexInsertsParameterDimen        display contains=@NoSpell
      \ '\\splitmaxdepth\>'
syn match   coretexMathParameterDimen           display contains=@NoSpell
      \ '\\\%(delimitershortfall\|display\%(indent\|width\)\|mathsurround\|nulldelimiterspace\|predisplaysize\|scriptspace\)\>'
syn match   coretexPageParameterDimen           display contains=@NoSpell
      \ '\\\%([hv]offset\|maxdepth\|vsize\)\>'
syn match   coretexParagraphParameterDimen      display contains=@NoSpell
      \ '\\\%(emergencystretch\|\%(hang\|par\)indent\|hsize\|lineskiplimit\)\>'

syn cluster coretexParameterGlue
      \ contains=coretexInsertsParameterGlue,coretexMathParameterGlue,
      \ coretexPageParameterGlue,coretexParagraphParameterGlue,
      \ coretexTablesParameterGlue

syn match   coretexInsertsParameterGlue         display contains=@NoSpell
      \ '\\splittopskip\>'
syn match   coretexMathParameterGlue            display contains=@NoSpell
      \ '\\\%(above\|below\)display\%(short\)\=skip\>'
syn match   coretexPageParameterGlue            display contains=@NoSpell
      \ '\\topskip\>'
syn match   coretexParagraphParameterGlue       display contains=@NoSpell
      \ '\\\%(baseline\|left\|line\|par\%(fill\)\=\|right\|x\=space\)skip\>'
syn match   coretexTablesParameterGlue          display contains=@NoSpell
      \ '\\tabskip\>'

syn cluster coretexParameterInteger
      \ contains=coretexBoxParameterInteger,coretexCharacterParameterInteger,
      \ coretexDebuggingParameterInteger,coretexHyphenationParameterInteger,
      \ coretexInsertsParameterInteger,coretexJobParameterInteger,
      \ coretexMacroParameterInteger,coretexMathParameterInteger,
      \ coretexParagraphParameterInteger,coretexPenaltiesParameterInteger,

syn match   coretexBoxParameterInteger          display contains=@NoSpell
      \ '\\[hv]badness\>'
syn match   coretexCharacterParameterInteger    display contains=@NoSpell
      \ '\\\%(\%(endline\|escape\|newline\)char\)\>'
syn match   coretexDebuggingParameterInteger    display contains=@NoSpell
      \ '\\\%(errorcontextlines\|pausing\|tracing\%(commands\|lostchars\|macros\|online\|output\|pages\|paragraphs\|restores|stats\)\)\>'
syn match   coretexHyphenationParameterInteger  display contains=@NoSpell
      \ '\\\%(defaulthyphenchar\|language\|\%(left\|right\)hyphenmin\|uchyph\)\>'
syn match   coretexInsertsParameterInteger      display contains=@NoSpell
      \ '\\\%(holdinginserts\)\>'
syn match   coretexJobParameterInteger          display contains=@NoSpell
      \ '\\\%(day\|mag\|maxdeadcycles\|month\|time\|year\)\>'
syn match   coretexMacroParameterInteger        display contains=@NoSpell
      \ '\\globaldefs\>'
syn match   coretexMathParameterInteger         display contains=@NoSpell
      \ '\\\%(binoppenalty\|defaultskewchar\|delimiterfactor\|displaywidowpenalty\|fam\|\%(post\|pre\)displaypenalty\|relpenalty\)\>'
syn match   coretexParagraphParameterInteger    display contains=@NoSpell
      \ '\\\%(\%(adj\|\%(double\|final\)hyphen\)demerits\|looseness\|\%(pre\)\=tolerance\)\>'
syn match   coretexPenaltiesParameterInteger    display contains=@NoSpell
      \ '\\\%(broken\|club\|exhyphen\|floating\|hyphen\|interline\|line\|output\|widow\)penalty\>'

syn cluster coretexParameterMuglue
      \ contains=coretexMathParameterMuglue

syn match   coretexMathParameterMuglue          display contains=@NoSpell
      \ '\\\%(med\|thick\|thin\)muskip\>'

syn cluster coretexParameterDimen
      \ contains=coretexBoxParameterToken,coretexDebuggingParameterToken,
      \ coretexFileIOParameterToken,coretexMathParameterToken,
      \ coretexParagraphParameterToken,coretexTablesParameterToken

syn match   coretexBoxParameterToken            display contains=@NoSpell
      \ '\\every[hv]box\>'
syn match   coretexDebuggingParameterToken      display contains=@NoSpell
      \ '\\errhelp\>'
syn match   coretexFileIOParameterToken         display contains=@NoSpell
      \ '\\output\>'
syn match   coretexMathParameterToken           display contains=@NoSpell
      \ '\\every\%(display\|math\)\>'
syn match   coretexParagraphParameterToken      display contains=@NoSpell
      \ '\\everypar\>'
syn match   coretexTablesParameterToken         display contains=@NoSpell
      \ '\\everycr\>'


hi def link coretexCharacter                    Character
hi def link coretexNumber                       Number

hi def link coretexIdentifier                   Identifier

hi def link coretexStatement                    Statement
hi def link coretexConditional                  Conditional

hi def link coretexPreProc                      PreProc
hi def link coretexMacro                        Macro

hi def link coretexType                         Type

hi def link coretexDebug                        Debug

hi def link coretexTodo                         Todo
hi def link coretexComment                      Comment
hi def link coretexDimension                    coretexNumber

hi def link coretexCommand                      coretexStatement
hi def link coretexBoxCommand                   coretexCommand
hi def link coretexCharacterCommand             coretexCharacter
hi def link coretexDebuggingCommand             coretexDebug
hi def link coretexFileIOCommand                coretexCommand
hi def link coretexFontsCommand                 coretexType
hi def link coretexGlueCommand                  coretexCommand
hi def link coretexHyphenationCommand           coretexCommand
hi def link coretexInsertsCommand               coretexCommand
hi def link coretexJobCommand                   coretexPreProc
hi def link coretexKernCommand                  coretexCommand
hi def link coretexLogicCommand                 coretexConditional
hi def link coretexMacroCommand                 coretexMacro
hi def link coretexMarksCommand                 coretexCommand
hi def link coretexMathCommand                  coretexCommand
hi def link coretexParagraphCommand             coretexCommand
hi def link coretexPenaltiesCommand             coretexCommand
hi def link coretexRegistersCommand             coretexCommand
hi def link coretexTablesCommand                coretexCommand

hi def link coretexDerivedCommand               coretexStatement
hi def link coretexGlueDerivedCommand           coretexDerivedCommand
hi def link coretexHyphenationDerivedCommand    coretexDerivedCommand
hi def link coretexMacroDerivedCommand          coretexDerivedCommand
hi def link coretexMathDerivedCommand           coretexDerivedCommand

hi def link coretexInternalQuantity             coretexIdentifier
hi def link coretexBoxInternalQuantity          coretexInternalQuantity
hi def link coretexCharacterInternalQuantity    coretexInternalQuantity
hi def link coretexFileIOInternalQuantity       coretexInternalQuantity
hi def link coretexFontsInternalQuantity        coretexInternalQuantity
hi def link coretexHyphenationInternalQuantity  coretexInternalQuantity
hi def link coretexJobInternalQuantity          coretexInternalQuantity
hi def link coretexKernInternalQuantity         coretexInternalQuantity
hi def link coretexMathInternalQuantity         coretexInternalQuantity
hi def link coretexPageInternalQuantity         coretexInternalQuantity
hi def link coretexParagraphInternalQuantity    coretexInternalQuantity
hi def link coretexPenaltiesInternalQuantity    coretexInternalQuantity
hi def link coretexRegistersInternalQuantity    coretexInternalQuantity

hi def link coretexParameterDimen               coretexNumber
hi def link coretexBoxParameterDimen            coretexParameterDimen
hi def link coretexInsertsParameterDimen        coretexParameterDimen
hi def link coretexMathParameterDimen           coretexParameterDimen
hi def link coretexPageParameterDimen           coretexParameterDimen
hi def link coretexParagraphParameterDimen      coretexParameterDimen

hi def link coretexParameterGlue                coretexNumber
hi def link coretexInsertsParameterGlue         coretexParameterGlue
hi def link coretexMathParameterGlue            coretexParameterGlue
hi def link coretexPageParameterGlue            coretexParameterGlue
hi def link coretexParagraphParameterGlue       coretexParameterGlue
hi def link coretexTablesParameterGlue          coretexParameterGlue

hi def link coretexParameterInteger             coretexNumber
hi def link coretexBoxParameterInteger          coretexParameterInteger
hi def link coretexCharacterParameterInteger    coretexParameterInteger
hi def link coretexDebuggingParameterInteger    coretexParameterInteger
hi def link coretexHyphenationParameterInteger  coretexParameterInteger
hi def link coretexInsertsParameterInteger      coretexParameterInteger
hi def link coretexJobParameterInteger          coretexParameterInteger
hi def link coretexMacroParameterInteger        coretexParameterInteger
hi def link coretexMathParameterInteger         coretexParameterInteger
hi def link coretexParagraphParameterInteger    coretexParameterInteger
hi def link coretexPenaltiesParameterInteger    coretexParameterInteger

hi def link coretexParameterMuglue              coretexNumber
hi def link coretexMathParameterMuglue          coretexParameterMuglue

hi def link coretexParameterToken               coretexIdentifier
hi def link coretexBoxParameterToken            coretexParameterToken
hi def link coretexDebuggingParameterToken      coretexParameterToken
hi def link coretexFileIOParameterToken         coretexParameterToken
hi def link coretexMathParameterToken           coretexParameterToken
hi def link coretexParagraphParameterToken      coretexParameterToken
hi def link coretexTablesParameterToken         coretexParameterToken

let b:current_syntax = "coretex"

let &cpo = s:cpo_save
unlet s:cpo_save

