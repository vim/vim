" Vim syntax file
" Language:     Nu
" Maintainer:   Pete Cruz <iPetesta@gmail.com>
" Last Change:  2025 Aug 12

if exists('b:current_syntax')
  finish
endif

syn iskeyword @,192-255,-,_

syn keyword nuCommand "\<alias\>" display
syn keyword nuCommand "\<all\>" display
syn keyword nuCommand "\<ansi\>" display
syn match nuCommand "\<ansi gradient\>" display
syn match nuCommand "\<ansi link\>" display
syn match nuCommand "\<ansi strip\>" display
syn keyword nuCommand "\<any\>" display
syn keyword nuCommand "\<append\>" display
syn keyword nuCommand "\<ast\>" display
syn keyword nuCommand "\<banner\>" display
syn keyword nuCommand "\<bits\>" display
syn match nuCommand "\<bits and\>" display
syn match nuCommand "\<bits not\>" display
syn match nuCommand "\<bits or\>" display
syn match nuCommand "\<bits rol\>" display
syn match nuCommand "\<bits ror\>" display
syn match nuCommand "\<bits shl\>" display
syn match nuCommand "\<bits shr\>" display
syn match nuCommand "\<bits xor\>" display
syn keyword nuCommand "\<break\>" display
syn keyword nuCommand "\<bytes\>" display
syn match nuCommand "\<bytes add\>" display
syn match nuCommand "\<bytes at\>" display
syn match nuCommand "\<bytes build\>" display
syn match nuCommand "\<bytes collect\>" display
syn match nuCommand "\<bytes ends-with\>" display
syn match nuCommand "\<bytes index-of\>" display
syn match nuCommand "\<bytes length\>" display
syn match nuCommand "\<bytes remove\>" display
syn match nuCommand "\<bytes replace\>" display
syn match nuCommand "\<bytes reverse\>" display
syn match nuCommand "\<bytes starts-with\>" display
syn keyword nuCommand "\<cal\>" display
syn keyword nuCommand "\<cd\>" display
syn keyword nuCommand "\<char\>" display
syn keyword nuCommand "\<clear\>" display
syn keyword nuCommand "\<collect\>" display
syn keyword nuCommand "\<columns\>" display
syn keyword nuCommand "\<commandline\>" display
syn keyword nuCommand "\<compact\>" display
syn keyword nuCommand "\<complete\>" display
syn keyword nuCommand "\<config\>" display
syn match nuCommand "\<config env\>" display
syn match nuCommand "\<config nu\>" display
syn match nuCommand "\<config reset\>" display
syn keyword nuCommand "\<const\>" nextgroup=nuIdentifier,nuSubCommand,nuDefFlag skipwhite display
syn keyword nuCommand "\<continue\>" display
syn keyword nuCommand "\<cp\>" display
syn match nuCommand "\<cp-old\>" display
syn match nuCommand "\<create_left_prompt\>" display
syn match nuCommand "\<create_right_prompt\>" display
syn keyword nuCommand "\<date\>" display
syn match nuCommand "\<date format\>" display
syn match nuCommand "\<date humanize\>" display
syn match nuCommand "\<date list-timezone\>" display
syn match nuCommand "\<date now\>" display
syn match nuCommand "\<date to-record\>" display
syn match nuCommand "\<date to-table\>" display
syn match nuCommand "\<date to-timezone\>" display
syn keyword nuCommand "\<debug\>" display
syn match nuCommand "\<debug info\>" display
syn keyword nuCommand "\<decode\>" display
syn match nuCommand "\<decode base64\>" display
syn match nuCommand "\<decode hex\>" display
syn keyword nuCommand "\<def\>" nextgroup=nuIdentifier,nuSubCommand,nuDefFlag skipwhite display
syn match nuCommand "\<def-env\>" nextgroup=nuIdentifier,nuSubCommand,nuDefFlag skipwhite display
syn keyword nuCommand "\<default\>" display
syn match nuCommand "\<describe\>" display
syn match nuCommand "\<detect columns\>" display
syn keyword nuCommand "\<drop\>" display
syn keyword nuCommand "\<dfr\>" display
syn match nuCommand "\<dfr agg\>" display
syn match nuCommand "\<dfr agg-groups\>" display
syn match nuCommand "\<dfr all-false\>" display
syn match nuCommand "\<dfr all-true\>" display
syn match nuCommand "\<dfr append\>" display
syn match nuCommand "\<dfr arg-max\>" display
syn match nuCommand "\<dfr arg-min\>" display
syn match nuCommand "\<dfr arg-sort\>" display
syn match nuCommand "\<dfr arg-true\>" display
syn match nuCommand "\<dfr arg-unique\>" display
syn match nuCommand "\<dfr arg-where\>" display
syn match nuCommand "\<dfr as\>" display
syn match nuCommand "\<dfr as-date\>" display
syn match nuCommand "\<dfr as-datetime\>" display
syn match nuCommand "\<dfr cache\>" display
syn match nuCommand "\<dfr col\>" display
syn match nuCommand "\<dfr collect\>" display
syn match nuCommand "\<dfr columns\>" display
syn match nuCommand "\<dfr concat-str\>" display
syn match nuCommand "\<dfr concatenate\>" display
syn match nuCommand "\<dfr contains\>" display
syn match nuCommand "\<dfr count\>" display
syn match nuCommand "\<dfr count-null\>" display
syn match nuCommand "\<dfr cumulative\>" display
syn match nuCommand "\<dfr datepart\>" display
syn match nuCommand "\<dfr drop\>" display
syn match nuCommand "\<dfr drop-duplicates\>" display
syn match nuCommand "\<dfr drop-nulls\>" display
syn match nuCommand "\<dfr dtypes\>" display
syn match nuCommand "\<dfr dummies\>" display
syn match nuCommand "\<dfr explode\>" display
syn match nuCommand "\<dfr expr-not\>" display
syn match nuCommand "\<dfr fetch\>" display
syn match nuCommand "\<dfr fill-nan\>" display
syn match nuCommand "\<dfr fill-null\>" display
syn match nuCommand "\<dfr filter\>" display
syn match nuCommand "\<dfr filter-with\>" display
syn match nuCommand "\<dfr first\>" display
syn match nuCommand "\<dfr flatten\>" display
syn match nuCommand "\<dfr get\>" display
syn match nuCommand "\<dfr get-day\>" display
syn match nuCommand "\<dfr get-hour\>" display
syn match nuCommand "\<dfr get-minute\>" display
syn match nuCommand "\<dfr get-month\>" display
syn match nuCommand "\<dfr get-nanosecond\>" display
syn match nuCommand "\<dfr get-ordinal\>" display
syn match nuCommand "\<dfr get-second\>" display
syn match nuCommand "\<dfr get-week\>" display
syn match nuCommand "\<dfr get-weekday\>" display
syn match nuCommand "\<dfr get-year\>" display
syn match nuCommand "\<dfr group-by\>" display
syn match nuCommand "\<dfr implode\>" display
syn match nuCommand "\<dfr into-df\>" display
syn match nuCommand "\<dfr into-lazy\>" display
syn match nuCommand "\<dfr into-nu\>" display
syn match nuCommand "\<dfr is-duplicated\>" display
syn match nuCommand "\<dfr is-in\>" display
syn match nuCommand "\<dfr is-not-null\>" display
syn match nuCommand "\<dfr is-null\>" display
syn match nuCommand "\<dfr is-unique\>" display
syn match nuCommand "\<dfr join\>" display
syn match nuCommand "\<dfr last\>" display
syn match nuCommand "\<dfr lit\>" display
syn match nuCommand "\<dfr lowercase\>" display
syn match nuCommand "\<dfr ls\>" display
syn match nuCommand "\<dfr max\>" display
syn match nuCommand "\<dfr mean\>" display
syn match nuCommand "\<dfr median\>" display
syn match nuCommand "\<dfr melt\>" display
syn match nuCommand "\<dfr min\>" display
syn match nuCommand "\<dfr n-unique\>" display
syn match nuCommand "\<dfr not\>" display
syn match nuCommand "\<dfr open\>" display
syn match nuCommand "\<dfr otherwise\>" display
syn match nuCommand "\<dfr quantile\>" display
syn match nuCommand "\<dfr query\>" display
syn match nuCommand "\<dfr rename\>" display
syn match nuCommand "\<dfr replace\>" display
syn match nuCommand "\<dfr replace-all\>" display
syn match nuCommand "\<dfr reverse\>" display
syn match nuCommand "\<dfr rolling\>" display
syn match nuCommand "\<dfr sample\>" display
syn match nuCommand "\<dfr select\>" display
syn match nuCommand "\<dfr set\>" display
syn match nuCommand "\<dfr set-with-idx\>" display
syn match nuCommand "\<dfr shape\>" display
syn match nuCommand "\<dfr shift\>" display
syn match nuCommand "\<dfr slice\>" display
syn match nuCommand "\<dfr sort-by\>" display
syn match nuCommand "\<dfr std\>" display
syn match nuCommand "\<dfr str-lengths\>" display
syn match nuCommand "\<dfr str-slice\>" display
syn match nuCommand "\<dfr strftime\>" display
syn match nuCommand "\<dfr sum\>" display
syn match nuCommand "\<dfr summary\>" display
syn match nuCommand "\<dfr take\>" display
syn match nuCommand "\<dfr to-arrow\>" display
syn match nuCommand "\<dfr to-avro\>" display
syn match nuCommand "\<dfr to-csv\>" display
syn match nuCommand "\<dfr to-jsonl\>" display
syn match nuCommand "\<dfr to-parquet\>" display
syn match nuCommand "\<dfr unique\>" display
syn match nuCommand "\<dfr uppercase\>" display
syn match nuCommand "\<dfr value-counts\>" display
syn match nuCommand "\<dfr var\>" display
syn match nuCommand "\<dfr when\>" display
syn match nuCommand "\<dfr with-column\>" display
syn keyword nuCommand "\<do\>" display
syn keyword nuCommand "\<drop\>" display
syn match nuCommand "\<drop column\>" display
syn match nuCommand "\<drop nth\>" display
syn keyword nuCommand "\<du\>" display
syn keyword nuCommand "\<each\>" display
syn match nuCommand "\<each while\>" display
syn keyword nuCommand "\<echo\>" display
syn keyword nuCommand "\<encode\>" display
syn match nuCommand "\<encode base64\>" display
syn match nuCommand "\<encode hex\>" display
syn keyword nuCommand "\<add\>" display
syn keyword nuCommand "\<enumerate\>" display
syn match nuCommand "\<error make\>" display
syn keyword nuCommand "\<every\>" display
syn keyword nuCommand "\<exec\>" display
syn keyword nuCommand "\<exists\>" display
syn keyword nuCommand "\<exit\>" display
syn keyword nuCommand "\<explain\>" display
syn keyword nuCommand "\<explore\>" display
syn keyword nuCommand "\<export\>" display
syn match nuCommand "\<export alias\>" display
syn match nuCommand "\<export const\>" nextgroup=nuIdentifier skipwhite display
syn match nuCommand "\<export def\>" nextgroup=nuIdentifier skipwhite display
syn match nuCommand "\<export def-env\>" nextgroup=nuIdentifier skipwhite display
syn match nuCommand "\<export extern\>" nextgroup=nuIdentifier skipwhite display
syn match nuCommand "\<export extern-wrapped\>" nextgroup=nuIdentifier skipwhite display
syn match nuCommand "\<export module\>" nextgroup=nuIdentifier skipwhite display
syn match nuCommand "\<export use\>" display
syn match nuCommand "\<export-env\>" display
syn keyword nuCommand "\<extern\>" display
syn match nuCommand "\<extern-wrapped\>" display
syn keyword nuCommand "\<fill\>" display
syn keyword nuCommand "\<filter\>" display
syn keyword nuCommand "\<find\>" display
syn keyword nuCommand "\<first\>" display
syn keyword nuCommand "\<flatten\>" display
syn keyword nuCommand "\<fmt\>" display
syn keyword nuCommand "\<for\>" display
syn keyword nuCommand "\<format\>" display
syn match nuCommand "\<format date\>" display
syn match nuCommand "\<format duration\>" display
syn match nuCommand "\<format filesize\>" display
syn keyword nuCommand "\<from\>" nextgroup=nuProperty skipwhite display
syn match nuCommand "\<from csv\>" display
syn match nuCommand "\<from json\>" display
syn match nuCommand "\<from nuon\>" display
syn match nuCommand "\<from ods\>" display
syn match nuCommand "\<from ssv\>" display
syn match nuCommand "\<from toml\>" display
syn match nuCommand "\<from tsv\>" display
syn match nuCommand "\<from url\>" display
syn match nuCommand "\<from xlsx\>" display
syn match nuCommand "\<from xml\>" display
syn match nuCommand "\<from yaml\>" display
syn match nuCommand "\<from yml\>" display
syn keyword nuCommand "\<goto\>" display
syn keyword nuCommand "\<get\>" nextgroup=nuProperty skipwhite display
syn keyword nuCommand "\<glob\>" display
syn keyword nuCommand "\<grid\>" display
syn keyword nuCommand "\<group\>" display
syn match nuCommand "\<group-by\>" nextgroup=nuProperty skipwhite display
syn keyword nuCommand "\<hash\>" display
syn match nuCommand "\<hash md5\>" display
syn match nuCommand "\<hash sha256\>" display
syn keyword nuCommand "\<headers\>" display
syn keyword nuCommand "\<help\>" display
syn match nuCommand "\<help aliases\>" display
syn match nuCommand "\<help commands\>" display
syn match nuCommand "\<help escapes\>" display
syn match nuCommand "\<help externs\>" display
syn match nuCommand "\<help modules\>" display
syn match nuCommand "\<help operators\>" display
syn keyword nuCommand "\<hide\>" display
syn match nuCommand "\<hide-env\>" display
syn keyword nuCommand "\<histogram\>" display
syn keyword nuCommand "\<history\>" display
syn match nuCommand "\<history session\>" display
syn keyword nuCommand "\<http\>" display
syn match nuCommand "\<http delete\>" display
syn match nuCommand "\<http get\>" display
syn match nuCommand "\<http head\>" display
syn match nuCommand "\<http options\>" display
syn match nuCommand "\<http patch\>" display
syn match nuCommand "\<http post\>" display
syn match nuCommand "\<http put\>" display
syn keyword nuCommand "\<if\>" display
syn keyword nuCommand "\<ignore\>" display
syn keyword nuCommand "\<input\>" display
syn match nuCommand "\<input list\>" display
syn match nuCommand "\<input listen\>" display
syn keyword nuCommand "\<insert\>" display
syn keyword nuCommand "\<inspect\>" display
syn keyword nuCommand "\<into\>" display
syn match nuCommand "\<into binary\>" display
syn match nuCommand "\<into bits\>" display
syn match nuCommand "\<into bool\>" display
syn match nuCommand "\<into datetime\>" display
syn match nuCommand "\<into duration\>" display
syn match nuCommand "\<into filesize\>" display
syn match nuCommand "\<into float\>" display
syn match nuCommand "\<into int\>" display
syn match nuCommand "\<into record\>" display
syn match nuCommand "\<into sqlite\>" display
syn match nuCommand "\<into string\>" display
syn match nuCommand "\<into value\>" display
syn match nuCommand "\<is-admin\>" display
syn match nuCommand "\<is-empty\>" display
syn keyword nuCommand "\<items\>" display
syn keyword nuCommand "\<join\>" display
syn keyword nuCommand "\<keybindings\>" display
syn match nuCommand "\<keybindings default\>" display
syn match nuCommand "\<keybindings list\>" display
syn match nuCommand "\<keybindings listen\>" display
syn keyword nuCommand "\<kill\>" display
syn keyword nuCommand "\<last\>" display
syn match nuCommand "\<lazy make\>" display
syn keyword nuCommand "\<length\>" display
syn keyword nuCommand "\<let\>" nextgroup=nuIdentifier skipwhite display
syn match nuCommand "\<let-env\>" nextgroup=nuIdentifier skipwhite display
syn keyword nuCommand "\<lines\>" display
syn match nuCommand "\<load-env\>" display
syn keyword nuCommand "\<loop\>" display
syn keyword nuCommand "\<ls\>" display
syn keyword nuCommand "\<match\>" display
syn keyword nuCommand "\<math\>" display
syn match nuCommand "\<math abs\>" display
syn match nuCommand "\<math arccos\>" display
syn match nuCommand "\<math arccosh\>" display
syn match nuCommand "\<math arcsin\>" display
syn match nuCommand "\<math arcsinh\>" display
syn match nuCommand "\<math arctan\>" display
syn match nuCommand "\<math arctanh\>" display
syn match nuCommand "\<math avg\>" display
syn match nuCommand "\<math ceil\>" display
syn match nuCommand "\<math cos\>" display
syn match nuCommand "\<math cosh\>" display
syn match nuCommand "\<math exp\>" display
syn match nuCommand "\<math floor\>" display
syn match nuCommand "\<math ln\>" display
syn match nuCommand "\<math log\>" display
syn match nuCommand "\<math max\>" display
syn match nuCommand "\<math median\>" display
syn match nuCommand "\<math min\>" display
syn match nuCommand "\<math mode\>" display
syn match nuCommand "\<math product\>" display
syn match nuCommand "\<math round\>" display
syn match nuCommand "\<math sin\>" display
syn match nuCommand "\<math sinh\>" display
syn match nuCommand "\<math sqrt\>" display
syn match nuCommand "\<math stddev\>" display
syn match nuCommand "\<math sum\>" display
syn match nuCommand "\<math tan\>" display
syn match nuCommand "\<math tanh\>" display
syn match nuCommand "\<math variance\>" display
syn keyword nuCommand "\<merge\>" display
syn keyword nuCommand "\<metadata\>" display
syn keyword nuCommand "\<mkdir\>" display
syn match nuCommand "\<module\>" nextgroup=nuIdentifier skipwhite display
syn keyword nuCommand "\<move\>" display
syn match nuCommand "\<mut\>" nextgroup=nuIdentifier skipwhite display
syn keyword nuCommand "\<mv\>" display
syn keyword nuCommand "\<next\>" display
syn match nuCommand "\<nu-check\>" display
syn match nuCommand "\<nu-highlight\>" display
syn keyword nuCommand "\<open\>" display
syn keyword nuCommand "\<overlay\>" display
syn match nuCommand "\<overlay hide\>" display
syn match nuCommand "\<overlay list\>" display
syn match nuCommand "\<overlay new\>" display
syn match nuCommand "\<overlay use\>" nextgroup=nuIdentifier skipwhite display
syn keyword nuCommand "\<prev\>" display
syn match nuCommand "\<par-each\>" display
syn keyword nuCommand "\<parse\>" display
syn keyword nuCommand "\<path\>" display
syn match nuCommand "\<path basename\>" display
syn match nuCommand "\<path dirname\>" display
syn match nuCommand "\<path exists\>" display
syn match nuCommand "\<path expand\>" display
syn match nuCommand "\<path join\>" display
syn match nuCommand "\<path parse\>" display
syn match nuCommand "\<path relative-to\>" display
syn match nuCommand "\<path split\>" display
syn match nuCommand "\<path type\>" display
syn keyword nuCommand "\<port\>" display
syn keyword nuCommand "\<prepend\>" display
syn keyword nuCommand "\<print\>" display
syn keyword nuCommand "\<profile\>" display
syn keyword nuCommand "\<ps\>" display
syn keyword nuCommand "\<pwd\>" display
syn match nuCommand "\<query db\>" display
syn keyword nuCommand "\<random\>" display
syn match nuCommand "\<random bool\>" display
syn match nuCommand "\<random chars\>" display
syn match nuCommand "\<random dice\>" display
syn match nuCommand "\<random float\>" display
syn match nuCommand "\<random int\>" display
syn match nuCommand "\<random integer\>" display
syn match nuCommand "\<random uuid\>" display
syn keyword nuCommand "\<range\>" display
syn keyword nuCommand "\<reduce\>" display
syn keyword nuCommand "\<register\>" display
syn keyword nuCommand "\<reject\>" display
syn keyword nuCommand "\<rename\>" display
syn keyword nuCommand "\<return\>" display
syn keyword nuCommand "\<reverse\>" display
syn keyword nuCommand "\<rm\>" display
syn keyword nuCommand "\<roll\>" display
syn match nuCommand "\<roll down\>" display
syn match nuCommand "\<roll left\>" display
syn match nuCommand "\<roll right\>" display
syn match nuCommand "\<roll up\>" display
syn keyword nuCommand "\<rotate\>" display
syn match nuCommand "\<run-external\>" display
syn keyword nuCommand "\<save\>" display
syn keyword nuCommand "\<schema\>" display
syn keyword nuCommand "\<scope\>" display
syn match nuCommand "\<scope aliases\>" display
syn match nuCommand "\<scope commands\>" display
syn match nuCommand "\<scope engine-stats\>" display
syn match nuCommand "\<scope externs\>" display
syn match nuCommand "\<scope modules\>" display
syn match nuCommand "\<scope variables\>" display
syn keyword nuCommand "\<select\>" display
syn keyword nuCommand "\<seq\>" display
syn match nuCommand "\<seq char\>" display
syn match nuCommand "\<seq date\>" display
syn keyword nuCommand "\<show\>" display
syn keyword nuCommand "\<shuffle\>" display
syn keyword nuCommand "\<size\>" display
syn keyword nuCommand "\<skip\>" display
syn match nuCommand "\<skip until\>" display
syn match nuCommand "\<skip while\>" display
syn keyword nuCommand "\<sleep\>" display
syn keyword nuCommand "\<sort\>" display
syn match nuCommand "\<sort-by\>" nextgroup=nuProperty skipwhite display
syn keyword nuCommand "\<source\>" display
syn match nuCommand "\<source-env\>" display
syn keyword nuCommand "\<split\>" display
syn match nuCommand "\<split chars\>" display
syn match nuCommand "\<split column\>" display
syn match nuCommand "\<split list\>" display
syn match nuCommand "\<split row\>" display
syn match nuCommand "\<split words\>" display
syn match nuCommand "\<split-by\>" nextgroup=nuProperty skipwhite display
syn keyword nuCommand "\<start\>" display
syn keyword nuCommand "\<str\>" display
syn match nuCommand "\<str camel-case\>" display
syn match nuCommand "\<str capitalize\>" display
syn match nuCommand "\<str contains\>" display
syn match nuCommand "\<str distance\>" display
syn match nuCommand "\<str downcase\>" display
syn match nuCommand "\<str ends-with\>" display
syn match nuCommand "\<str expand\>" display
syn match nuCommand "\<str index-of\>" display
syn match nuCommand "\<str join\>" display
syn match nuCommand "\<str kebab-case\>" display
syn match nuCommand "\<str length\>" display
syn match nuCommand "\<str pascal-case\>" display
syn match nuCommand "\<str replace\>" display
syn match nuCommand "\<str reverse\>" display
syn match nuCommand "\<str screaming-snake-case\>" display
syn match nuCommand "\<str snake-case\>" display
syn match nuCommand "\<str starts-with\>" display
syn match nuCommand "\<str substring\>" display
syn match nuCommand "\<str title-case\>" display
syn match nuCommand "\<str trim\>" display
syn match nuCommand "\<str upcase\>" display
syn keyword nuCommand "\<sys\>" display
syn match nuCommand "\<sys cpu\>" display
syn match nuCommand "\<sys disks\>" display
syn match nuCommand "\<sys host\>" display
syn match nuCommand "\<sys mem\>" display
syn match nuCommand "\<sys net\>" display
syn match nuCommand "\<sys temp\>" display
syn match nuCommand "\<sys users\>" display
syn keyword nuCommand "\<table\>" display
syn keyword nuCommand "\<take\>" display
syn match nuCommand "\<take until\>" display
syn match nuCommand "\<take while\>" display
syn match nuCommand "\<term size\>" display
syn keyword nuCommand "\<timeit\>" display
syn keyword nuCommand "\<to\>" display
syn match nuCommand "\<to csv\>" display
syn match nuCommand "\<to html\>" display
syn match nuCommand "\<to json\>" display
syn match nuCommand "\<to md\>" display
syn match nuCommand "\<to nuon\>" display
syn match nuCommand "\<to text\>" display
syn match nuCommand "\<to toml\>" display
syn match nuCommand "\<to tsv\>" display
syn match nuCommand "\<to xml\>" display
syn match nuCommand "\<to yaml\>" display
syn keyword nuCommand "\<touch\>" display
syn keyword nuCommand "\<transpose\>" display
syn keyword nuCommand "\<try\>" display
syn keyword nuCommand "\<tutor\>" display
syn keyword nuCommand "\<unfold\>" display
syn keyword nuCommand "\<uniq\>" display
syn match nuCommand "\<uniq-by\>" nextgroup=nuProperty skipwhite display
syn keyword nuCommand "\<update\>" display
syn match nuCommand "\<update cells\>" display
syn keyword nuCommand "\<upsert\>" display
syn keyword nuCommand "\<url\>" display
syn match nuCommand "\<url build-query\>" display
syn match nuCommand "\<url decode\>" display
syn match nuCommand "\<url encode\>" display
syn match nuCommand "\<url join\>" display
syn match nuCommand "\<url parse\>" display
syn keyword nuCommand "\<use\>" nextgroup=nuIdentifier skipwhite display
syn keyword nuCommand "\<values\>" display
syn keyword nuCommand "\<version\>" display
syn keyword nuCommand "\<view\>" display
syn match nuCommand "\<view files\>" display
syn match nuCommand "\<view source\>" display
syn match nuCommand "\<view span\>" display
syn keyword nuCommand "\<watch\>" display
syn keyword nuCommand "\<where\>" nextgroup=nuProperty skipwhite display
syn keyword nuCommand "\<which\>" display
syn keyword nuCommand "\<while\>" display
syn keyword nuCommand "\<whoami\>" display
syn keyword nuCommand "\<window\>" display
syn match nuCommand "\<with-env\>" display
syn keyword nuCommand "\<wrap\>" display
syn keyword nuCommand "\<zip\>" display

syn match nuNumber "\([a-zA-Z_\.]\+\d*\)\@<!\d\+" nextgroup=nuUnit,nuDuration
syn match nuNumber "\([a-zA-Z]\)\@<!\.\d\+" nextgroup=nuUnit,nuDuration
syn match nuNumber "\([a-zA-Z]\)\@<!_\d\+" nextgroup=nuUnit,nuDuration,nuNumber
syn match nuNumber "\d\+[eE][+-]\?\d\+" nextgroup=nuUnit,nuDuration
syn match nuNumber "\d\+\.\d\+[eE]\?[+-]\d\+" nextgroup=nuUnit,nuDuration

syn keyword nuTodo contained TODO FIXME NOTE
syn match nuComment "#.*$" contains=nuTodo

syn match nuOperator "=" display
syn match nuOperator "-" display
syn match nuOperator "?" display
syn match nuOperator "<" display
syn match nuOperator ">" display
syn match nuOperator "+" display
syn match nuOperator "/" display
syn match nuOperator "\*" display
syn match nuOperator "!=" display
syn match nuOperator "=\~" display
syn match nuOperator "\!\~" display
syn match nuOperator "\<in\>" nextgroup=nuProperty skipwhite display
syn match nuOperator "\<not-in\>" nextgroup=nuProperty skipwhite display
syn match nuOperator "\<not\>" display
syn match nuOperator "\<and\>" nextgroup=nuProperty skipwhite display
syn match nuOperator "\<or\>" nextgroup=nuProperty skipwhite display
syn match nuOperator "\<xor\>" nextgroup=nuProperty skipwhite display
syn match nuOperator "\<bit-or\>" display
syn match nuOperator "\<bit-xor\>" display
syn match nuOperator "\<bit-and\>" display
syn match nuOperator "\<bit-shl\>" display
syn match nuOperator "\<bit-shr\>" display
syn match nuOperator "\<starts-with\>" display
syn match nuOperator "\<ends-with\>" display
syn match nuOperator "\.\.\." display

syn match nuVar "\$[^?\])} \t]\+"

syn match nuIdentifier :\(-\+\)\@![^? \t"=]\+: contained

syn region nuSubCommand start=/"/ skip=/\\./ end=/"/ contained

syn match nuProperty '\w\+' contained

syn keyword nuType any binary bool cell-path closure datetime directory duration error filesize float glob int list nothing number path range record string table true false null

syn keyword nuConditional if then else

syn match nuUnit "b\>" contained
syn match nuUnit "kb\>" contained
syn match nuUnit "mb\>" contained
syn match nuUnit "gb\>" contained
syn match nuUnit "tb\>" contained
syn match nuUnit "pb\>" contained
syn match nuUnit "eb\>" contained
syn match nuUnit "kib\>" contained
syn match nuUnit "mib\>" contained
syn match nuUnit "gib\>" contained
syn match nuUnit "tib\>" contained
syn match nuUnit "pib\>" contained
syn match nuUnit "eib\>" contained

syn match nuDuration "ns\>" contained
syn match nuDuration "us\>" contained
syn match nuDuration "ms\>" contained
syn match nuDuration "sec\>" contained
syn match nuDuration "min\>" contained
syn match nuDuration "hr\>" contained
syn match nuDuration "day\>" contained
syn match nuDuration "wk\>" contained

syn match nuFlag "\<-\k\+"

syn match nuDefFlag "\<--env\>" display contained nextgroup=nuIdentifier skipwhite
syn match nuDefFlag "\<--wrapped\>" display contained nextgroup=nuIdentifier skipwhite

syn match nuSysEsc "\^\k\+" display

syn match nuSquareBracket "\[" display
syn match nuSquareBracket "\]" display
syn match nuSquareBracket ":" display

syn region nuString start=/\v"/ skip=/\v\\./ end=/\v"/ contains=nuEscaped
syn region nuString start='\'' end='\''
syn region nuString start='`' end='`'

syn region nuStrInt start=/$'/ end=/'/ contains=nuNested
syn region nuStrInt start=/$"/ skip=/\\./ end=/"/ contains=nuNested,nuEscaped

syn region nuNested start="("hs=s+1 end=")"he=e-1 contained contains=nuAnsi
syn match nuAnsi "ansi[a-zA-Z0-9;' -]\+)"me=e-1 contained

syn match nuClosure "|\(\w\|, \)\+|"

syn match nuDot ")\.\(\k\|\.\)\+"ms=s+1 display

syn match nuEscaped "\\\\" display
syn match nuEscaped :\\": display
syn match nuEscaped "\\n" display
syn match nuEscaped "\\t" display
syn match nuEscaped "\\r" display

let b:current_syntax = 'nu'

hi def link nuCommand Keyword
hi def link nuComment Comment
hi def link nuTodo Todo
hi def link nuString Constant
hi def link nuChar Constant
hi def link nuOperator Operator
hi def link nuVar PreProc
hi def link nuSquareBracket Special
hi def link nuIdentifier Identifier
hi def link nuType Type
hi def link nuUnit Type
hi def link nuDuration Type
hi def link nuProperty Special
hi def link nuSubCommand Identifier
hi def link nuStrInt Constant
hi def link nuNested PreProc
hi def link nuFlag Special
hi def link nuEscaped Special
hi def link nuConditional Type
hi def link nuClosure Type
hi def link nuNumber Number
hi def link nuDot Special
hi def link nuSysEsc PreProc
hi def link nuAnsi Special
hi def link nuDefFlag Special
