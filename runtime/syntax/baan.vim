" Vim syntax file"
" Language:	Baan
" Maintainer:	Erwin Smit / Her van de Vliert
" Last change:	30-10-2001"

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

"************************************* 3GL ************************************"
syn match baan3gl "#ident"
syn match baan3gl "#include"
syn match baan3gl "#define"
syn match baan3gl "#undef"
syn match baan3gl "#pragma"
syn keyword baanConditional if then else case endif while endwhile endfor endcase
syn keyword baan3gl at based break bset call common const continue default double
syn keyword baan3gl empty extern fixed function ge global goto gt le lt mb
syn keyword baan3gl multibyte ne ofr prompt ref repeat static step stop string
syn keyword baan3gl true false until void wherebind
syn keyword baan3gl and or to not in
syn keyword baan3gl domain table eq input end long dim return at base print
syn match baan3gl "\<for\>" contains=baansql
syn match baan3gl "on case"
syn match baan3gl "e\=n\=d\=dllusage"

"************************************* SQL ************************************"
syn keyword baansqlh where reference selecterror selectbind selectdo selectempty
syn keyword baansqlh selecteos whereused endselect unref setunref clearunref
syn keyword baansqlh from select clear skip rows
syn keyword baansql between inrange having
syn match baansql "as set with \d\+ rows"
syn match baansql "as prepared set"
syn match baansql "as prepared set with \d\+ rows"
syn match baansql "refers to"
syn match baansql "with retry"
syn match baansql "with retry repeat last row"
syn match baansql "for update"
syn match baansql "order by"
syn match baansql "group by"
syn match baansql "commit\.transaction()"
syn match baansql "abort\.transaction()"
syn match baansql "db\.columns\.to\.record"
syn match baansql "db\.record\.to\.columns"
syn match baansql "db\.bind"
syn match baansql "db\.change\.order"
syn match baansql "\<db\.eq"
syn match baansql "\<db\.first"
syn match baansql "\<db\.gt"
syn match baansql "\<db\.ge"
syn match baansql "\<db\.le"
syn match baansql "\<db\.next"
syn match baansql "\<db\.prev"
syn match baansql "\<db\.insert"
syn match baansql "\<db\.delete"
syn match baansql "\<db\.update"
syn match baansql "\<db\.create\.table"
syn match baansql "db\.set\.to\.default"
syn match baansql "db\.retry"
syn match baansql "DB\.RETRY"
syn match baansql "db\.delayed\.lock"
syn match baansql "db\.retry\.point()"
syn match baansql "db\.retry\.hit()"
syn match baansql "db\.return\.dupl"
syn match baansql "db\.skip\.dupl"
syn match baansql "db\.row\.length"

"************************************* 4GL ************************************"
" Program section
syn match baan4glh "declaration:"
syn match baan4glh "functions:"
syn match baan4glh "before\.program:"
syn match baan4glh "on\.error:"
syn match baan4glh "after\.program:"
syn match baan4glh "after\.update.db.commit:"
syn match baan4glh "before\.display\.object:"

" Form section
syn match baan4glh "form\.\d\+:"
syn match baan4glh "form\.all:"
syn match baan4glh "form\.other:"
syn match baan4gl "init\.form:"
syn match baan4gl "before\.form:"
syn match baan4gl "after\.form:"

" Choice section
syn match baan4glh "choice\.start\.set:"
syn match baan4glh "choice\.first\.view:"
syn match baan4glh "choice\.next\.view:"
syn match baan4glh "choice\.prev\.view:"
syn match baan4glh "choice\.last\.view:"
syn match baan4glh "choice\.def\.find:"
syn match baan4glh "choice\.find\.data:"
syn match baan4glh "choice\.first\.set:"
syn match baan4glh "choice\.next\.set:"
syn match baan4glh "choice\.display\.set:"
syn match baan4glh "choice\.prev\.set:"
syn match baan4glh "choice\.rotate\.curr:"
syn match baan4glh "choice\.last\.set:"
syn match baan4glh "choice\.add\.set:"
syn match baan4glh "choice\.update\.db:"
syn match baan4glh "choice\.dupl\.occur:"
syn match baan4glh "choice\.recover\.set:"
syn match baan4glh "choice\.mark\.delete:"
syn match baan4glh "choice\.mark\.occur:"
syn match baan4glh "choice\.change\.order:"
syn match baan4glh "choice\.modify\.set:"
syn match baan4glh "choice\.restart\.input:"
syn match baan4glh "choice\.print\.data:"
syn match baan4glh "choice\.create\.job:"
syn match baan4glh "choice\.form\.tab\.change:"
syn match baan4glh "choice\.first\.frm:"
syn match baan4glh "choice\.next\.frm:"
syn match baan4glh "choice\.prev\.frm:"
syn match baan4glh "choice\.last\.frm:"
syn match baan4glh "choice\.resize\.frm:"
syn match baan4glh "choice\.cmd\.options:"
syn match baan4glh "choice\.zoom:"
syn match baan4glh "choice\.interrupt:"
syn match baan4glh "choice\.end\.program:"
syn match baan4glh "choice\.abort\.program:"
syn match baan4glh "choice\.cont\.process:"
syn match baan4glh "choice\.text\.manager:"
syn match baan4glh "choice\.run\.job:"
syn match baan4glh "choice\.global\.delete:"
syn match baan4glh "choice\.global\.copy:"
syn match baan4glh "choice\.save\.defaults"
syn match baan4glh "choice\.get\.defaults:"
syn match baan4glh "choice\.start\.chart:"
syn match baan4glh "choice\.start\.query:"
syn match baan4glh "choice\.user\.\d:"
syn match baan4glh "choice\.ask\.helpinfo:"
syn match baan4glh "choice\.calculator:"
syn match baan4glh "choice\.calendar:"
syn match baan4glh "choice\.bms:"
syn match baan4glh "choice\.cmd\.whats\.this:"
syn match baan4glh "choice\.help\.index:"
syn match baan4gl "before\.choice:"
syn match baan4gl "on\.choice:"
syn match baan4gl "after\.choice:"

" Field section
syn match baan4glh "field\.\l\{5}\d\{3}\.\l\{4}\.\=c\=:"
syn match baan4glh "field\.e\..\+:"
syn match baan4glh "field\.all:"
syn match baan4glh "field\.other:"
syn match baan4gl "init\.field:"
syn match baan4gl "before\.field:"
syn match baan4gl "before\.input:"
syn match baan4gl "before\.display:"
syn match baan4gl "before\.zoom:"
syn match baan4gl "before\.checks:"
syn match baan4gl "domain\.error:"
syn match baan4gl "ref\.input:"
syn match baan4gl "ref\.display:"
syn match baan4gl "check\.input:"
syn match baan4gl "on\.input:"
syn match baan4gl "when\.field\.changes:"
syn match baan4gl "after\.zoom:"
syn match baan4gl "after\.input:"
syn match baan4gl "after\.display:"
syn match baan4gl "after\.field:"

" Group section
syn match baan4glh "group\.\d\+:"
syn match baan4gl "init\.group:"
syn match baan4gl "before\.group:"
syn match baan4gl "after\.group:"

" Zoom section
syn match baan4glh "zoom\.from\..\+:"
syn match baan4gl "on\.entry:"
syn match baan4gl "on\.exit:"
" Main table section
syn match baan4glh "main\.table\.io:"
syn match baan4gl "before\.read:"
syn match baan4gl "after\.read:"
syn match baan4gl "before\.write:"
syn match baan4gl "after\.write:"
syn match baan4gl "after\.skip\.write:"
syn match baan4gl "before\.rewrite:"
syn match baan4gl "after\.rewrite:"
syn match baan4gl "after\.skip\.rewrite:"
syn match baan4gl "before\.delete:"
syn match baan4gl "after\.delete:"
syn match baan4gl "after\.skip\.delete:"
syn match baan4gl "read\.view:"

"number without a dot."
syn match  baanNumber		"\<\-\=\d\+\>"
"number with dot"
syn match  baanNumber		"\<\-\=\d\+\.\d*\>"
"number starting with a dot"
syn match  baanNumber		"\<\-\=\.\d\+\>"

" String"
syn region  baanString	start=+"+  skip=+""+  end=+"+
" Comment"
syn match   baanComment "|$"
syn match   baanComment "|.$"
syn match   baanComment "|[^ ]"
syn match   baanComment	"|[^#].*[^ ]"
syn match   baanCommenth "^|#lra.*$"
syn match   baanCommenth "^|#mdm.*$"
syn match   baanCommenth "^|#[0-9][0-9][0-9][0-9][0-9].*$"
syn match   baanCommenth "^|#N\=o\=Include.*$"
syn region  baanComment start="dllusage" end="enddllusage"
" Oldcode"
syn match   baanUncommented	"^|[^*#].*[^ ]"
" SpaceError"
syn match  BaanSpaces	" "
syn match  baanSpaceError	"\s*$"
syn match  baanSpaceError	"        "

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_baan_syn_inits")
  if version < 508
    let did_c_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink baanConditional	Conditional
  HiLink baan3gl		Statement
  HiLink baan4gl		Statement
  HiLink baan4glh		Statement
  HiLink baansql		Statement
  HiLink baansqlh		Statement
  HiLink baanNumber		Number
  HiLink baanString		String
  HiLink baanComment		Comment
  HiLink baanCommenth		Comment
  HiLink baanUncommented	Comment

  delcommand HiLink
endif

let b:current_syntax = "baan"

" vim: ts=8
