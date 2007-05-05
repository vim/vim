" Vim syntax file
" Language:		Ruby
" Maintainer:		Doug Kearns <dougkearns@gmail.com>
" Info:			$Id$
" URL:			http://vim-ruby.rubyforge.org
" Anon CVS:		See above site
" Release Coordinator:	Doug Kearns <dougkearns@gmail.com>
" ----------------------------------------------------------------------------
"
" Previous Maintainer:	Mirko Nasato
" Thanks to perl.vim authors, and to Reimer Behrends. :-) (MN)
" ----------------------------------------------------------------------------

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if has("folding") && exists("ruby_fold")
  setlocal foldmethod=syntax
endif

if exists("ruby_space_errors")
  if !exists("ruby_no_trail_space_error")
    syn match rubySpaceError display excludenl "\s\+$"
  endif
  if !exists("ruby_no_tab_space_error")
    syn match rubySpaceError display " \+\t"me=e-1
  endif
endif

" Operators
if exists("ruby_operators")
  syn match  rubyOperator	 "\%(\^\|\~\|\%(class\s*\)\@<!<<\|<=>\|<=\|\%(<\|\<class\s\+\u\w*\s*\)\@<!<[^<]\@=\|===\|==\|=\~\|>>\|>=\|>\||\|-\|/\|\*\*\|\*\|&\|%\|+\)"
  syn match  rubyPseudoOperator  "\%(-=\|/=\|\*\*=\|\*=\|&&\|&=\|&&=\|||\||=\|||=\|%=\|+=\|!\~\|!=\)"
  syn region rubyBracketOperator matchgroup=rubyOperator start="\%([_[:lower:]]\w*[?!=]\=\|}\)\@<=\[\s*" end="\s*]"
endif

" Expression Substitution and Backslash Notation
syn match rubyEscape	"\\\\\|\\[abefnrstv]\|\\\o\{1,3}\|\\x\x\{1,2}"							 contained display
syn match rubyEscape	"\%(\\M-\\C-\|\\C-\\M-\|\\M-\\c\|\\c\\M-\|\\c\|\\C-\|\\M-\)\%(\\\o\{1,3}\|\\x\x\{1,2}\|\\\=\S\)" contained display

syn region rubyInterpolation	      matchgroup=rubyInterpolationDelimiter start="#{" end="}" contained contains=TOP
syn match  rubyInterpolation	      "#\%(\$\|@@\=\)\w\+"    display contained  contains=rubyInterpolationDelimiter,rubyInstanceVariable,rubyClassVariable,rubyGlobalVariable,rubyPredefinedVariable
syn match  rubyInterpolationDelimiter "#\ze\%(\$\|@@\=\)\w\+" display contained
syn region rubyNoInterpolation	      start="\\#{" end="}"    contained
syn match  rubyNoInterpolation	      "\\#{"		      display contained
syn match  rubyNoInterpolation	      "\\#\%(\$\|@@\=\)\w\+"  display contained

syn match rubyDelimEscape	"\\[(<{\[)>}\]]" transparent display contained contains=NONE

syn region rubyNestedParentheses	start="("	end=")"		skip="\\\\\|\\)"	transparent contained contains=@rubyStringSpecial,rubyNestedParentheses,rubyDelimEscape
syn region rubyNestedCurlyBraces	start="{"	end="}"		skip="\\\\\|\\}"	transparent contained contains=@rubyStringSpecial,rubyNestedCurlyBraces,rubyDelimEscape
syn region rubyNestedAngleBrackets	start="<"	end=">"		skip="\\\\\|\\>"	transparent contained contains=@rubyStringSpecial,rubyNestedAngleBrackets,rubyDelimEscape
syn region rubyNestedSquareBrackets	start="\["	end="\]"	skip="\\\\\|\\\]"	transparent contained contains=@rubyStringSpecial,rubyNestedSquareBrackets,rubyDelimEscape

syn cluster rubyStringSpecial		contains=rubyInterpolation,rubyNoInterpolation,rubyEscape
syn cluster rubyExtendedStringSpecial	contains=@rubyStringSpecial,rubyNestedParentheses,rubyNestedCurlyBraces,rubyNestedAngleBrackets,rubyNestedSquareBrackets

" Numbers and ASCII Codes
syn match rubyASCIICode	"\w\@<!\%(?\%(\\M-\\C-\|\\C-\\M-\|\\M-\\c\|\\c\\M-\|\\c\|\\C-\|\\M-\)\=\%(\\\o\{1,3}\|\\x\x\{1,2}\|\\\=\S\)\)"
syn match rubyInteger	"\<0[xX]\x\+\%(_\x\+\)*\>"								display
syn match rubyInteger	"\<\%(0[dD]\)\=\%(0\|[1-9]\d*\%(_\d\+\)*\)\>"						display
syn match rubyInteger	"\<0[oO]\=\o\+\%(_\o\+\)*\>"								display
syn match rubyInteger	"\<0[bB][01]\+\%(_[01]\+\)*\>"								display
syn match rubyFloat	"\<\%(0\|[1-9]\d*\%(_\d\+\)*\)\.\d\+\%(_\d\+\)*\>"					display
syn match rubyFloat	"\<\%(0\|[1-9]\d*\%(_\d\+\)*\)\%(\.\d\+\%(_\d\+\)*\)\=\%([eE][-+]\=\d\+\%(_\d\+\)*\)\>"	display

" Identifiers
syn match rubyLocalVariableOrMethod "\<[_[:lower:]][_[:alnum:]]*[?!=]\=" contains=NONE display transparent
syn match rubyBlockArgument	    "&[_[:lower:]][_[:alnum:]]"		 contains=NONE display transparent

syn match  rubyConstant			"\%(\%([.@$]\@<!\.\)\@<!\<\|::\)\_s*\zs\u\w*\%(\>\|::\)\@=\%(\s*(\)\@!"
syn match  rubyClassVariable		"@@\h\w*" display
syn match  rubyInstanceVariable		"@\h\w*"  display
syn match  rubyGlobalVariable		"$\%(\h\w*\|-.\)"
syn match  rubySymbol			":\@<!:\%(\^\|\~\|<<\|<=>\|<=\|<\|===\|==\|=\~\|>>\|>=\|>\||\|-@\|-\|/\|\[]=\|\[]\|\*\*\|\*\|&\|%\|+@\|+\|`\)"
syn match  rubySymbol			":\@<!:\$\%(-.\|[`~<=>_,;:!?/.'"@$*\&+0]\)"
syn match  rubySymbol			":\@<!:\%(\$\|@@\=\)\=\h\w*[?!=]\="
syn region rubySymbol			start=":\@<!:\"" end="\"" skip="\\\\\|\\\""
syn region rubySymbol			start=":\@<!:\"" end="\"" skip="\\\\\|\\\"" contains=@rubyStringSpecial fold
if exists("ruby_operators")
  syn match  rubyBlockParameter		"\%(\%(\%(\<do\>\|{\)\s*\)|\s*\)\@<=[( ,a-zA-Z0-9_*)]\+\%(\s*|\)\@=" display
else
  syn match  rubyBlockParameter		"\%(\%(\<do\>\|{\)\s*\)\@<=|\s*\zs[( ,a-zA-Z0-9_*)]\+\ze\s*|" display
endif

syn match rubyPredefinedVariable #$[!$&"'*+,./0:;<=>?@\`~1-9]#
syn match rubyPredefinedVariable "$_\>"											   display
syn match rubyPredefinedVariable "$-[0FIKadilpvw]\>"									   display
syn match rubyPredefinedVariable "$\%(deferr\|defout\|stderr\|stdin\|stdout\)\>"					   display
syn match rubyPredefinedVariable "$\%(DEBUG\|FILENAME\|KCODE\|LOADED_FEATURES\|LOAD_PATH\|PROGRAM_NAME\|SAFE\|VERBOSE\)\>" display
syn match rubyPredefinedConstant "\%(\%(\.\@<!\.\)\@<!\|::\)\_s*\zs\%(MatchingData\|ARGF\|ARGV\|ENV\)\>\%(\s*(\)\@!"
syn match rubyPredefinedConstant "\%(\%(\.\@<!\.\)\@<!\|::\)\_s*\zs\%(DATA\|FALSE\|NIL\|RUBY_PLATFORM\|RUBY_RELEASE_DATE\)\>\%(\s*(\)\@!"
syn match rubyPredefinedConstant "\%(\%(\.\@<!\.\)\@<!\|::\)\_s*\zs\%(RUBY_VERSION\|STDERR\|STDIN\|STDOUT\|TOPLEVEL_BINDING\|TRUE\)\>\%(\s*(\)\@!"
"Obsolete Global Constants
"syn match rubyPredefinedConstant "\%(::\)\=\zs\%(PLATFORM\|RELEASE_DATE\|VERSION\)\>"
"syn match rubyPredefinedConstant "\%(::\)\=\zs\%(NotImplementError\)\>"

" Normal Regular Expression
syn region rubyRegexp matchgroup=rubyRegexpDelimiter start="\%(\%(^\|\<\%(and\|or\|while\|until\|unless\|if\|elsif\|when\|not\|then\)\|[;\~=!|&(,[>]\)\s*\)\@<=/" end="/[iomx]*" skip="\\\\\|\\/" contains=@rubyStringSpecial fold
syn region rubyRegexp matchgroup=rubyRegexpDelimiter start="\%(\<\%(split\|scan\|gsub\|sub\)\s*\)\@<=/" end="/[iomx]*" skip="\\\\\|\\/" contains=@rubyStringSpecial fold

" Normal String and Shell Command Output
syn region rubyString matchgroup=rubyStringDelimiter start="\"" end="\"" skip="\\\\\|\\\"" contains=@rubyStringSpecial fold
syn region rubyString matchgroup=rubyStringDelimiter start="'"	end="'"  skip="\\\\\|\\'"			       fold
syn region rubyString matchgroup=rubyStringDelimiter start="`"	end="`"  skip="\\\\\|\\`"  contains=@rubyStringSpecial fold

" Generalized Regular Expression
syn region rubyRegexp matchgroup=rubyRegexpDelimiter start="%r\z([~`!@#$%^&*_\-+=|\:;"',.?/]\)"	end="\z1[iomx]*" skip="\\\\\|\\\z1" contains=@rubyStringSpecial fold
syn region rubyRegexp matchgroup=rubyRegexpDelimiter start="%r{"				end="}[iomx]*"	 skip="\\\\\|\\}"   contains=@rubyStringSpecial,rubyNestedCurlyBraces,rubyDelimEscape fold
syn region rubyRegexp matchgroup=rubyRegexpDelimiter start="%r<"				end=">[iomx]*"	 skip="\\\\\|\\>"   contains=@rubyStringSpecial,rubyNestedAngleBrackets,rubyDelimEscape fold
syn region rubyRegexp matchgroup=rubyRegexpDelimiter start="%r\["				end="\][iomx]*"	 skip="\\\\\|\\\]"  contains=@rubyStringSpecial,rubyNestedSquareBrackets,rubyDelimEscape fold
syn region rubyRegexp matchgroup=rubyRegexpDelimiter start="%r("				end=")[iomx]*"	 skip="\\\\\|\\)"   contains=@rubyStringSpecial,rubyNestedParentheses,rubyDelimEscape fold

" Generalized Single Quoted String, Symbol and Array of Strings
syn region rubyString matchgroup=rubyStringDelimiter start="%[qsw]\z([~`!@#$%^&*_\-+=|\:;"',.?/]\)" end="\z1" skip="\\\\\|\\\z1" fold
syn region rubyString matchgroup=rubyStringDelimiter start="%[qsw]{"				    end="}"   skip="\\\\\|\\}"	 fold	contains=rubyNestedCurlyBraces,rubyDelimEscape
syn region rubyString matchgroup=rubyStringDelimiter start="%[qsw]<"				    end=">"   skip="\\\\\|\\>"	 fold	contains=rubyNestedAngleBrackets,rubyDelimEscape
syn region rubyString matchgroup=rubyStringDelimiter start="%[qsw]\["				    end="\]"  skip="\\\\\|\\\]"	 fold	contains=rubyNestedSquareBrackets,rubyDelimEscape
syn region rubyString matchgroup=rubyStringDelimiter start="%[qsw]("				    end=")"   skip="\\\\\|\\)"	 fold	contains=rubyNestedParentheses,rubyDelimEscape

" Generalized Double Quoted String and Array of Strings and Shell Command Output
" Note: %= is not matched here as the beginning of a double quoted string
syn region rubyString matchgroup=rubyStringDelimiter start="%\z([~`!@#$%^&*_\-+|\:;"',.?/]\)"	    end="\z1" skip="\\\\\|\\\z1" contains=@rubyStringSpecial fold
syn region rubyString matchgroup=rubyStringDelimiter start="%[QWx]\z([~`!@#$%^&*_\-+=|\:;"',.?/]\)" end="\z1" skip="\\\\\|\\\z1" contains=@rubyStringSpecial fold
syn region rubyString matchgroup=rubyStringDelimiter start="%[QWx]\={"				    end="}"   skip="\\\\\|\\}"	 contains=@rubyStringSpecial,rubyNestedCurlyBraces,rubyDelimEscape fold
syn region rubyString matchgroup=rubyStringDelimiter start="%[QWx]\=<"				    end=">"   skip="\\\\\|\\>"	 contains=@rubyStringSpecial,rubyNestedAngleBrackets,rubyDelimEscape fold
syn region rubyString matchgroup=rubyStringDelimiter start="%[QWx]\=\["				    end="\]"  skip="\\\\\|\\\]"	 contains=@rubyStringSpecial,rubyNestedSquareBrackets,rubyDelimEscape fold
syn region rubyString matchgroup=rubyStringDelimiter start="%[QWx]\=("				    end=")"   skip="\\\\\|\\)"	 contains=@rubyStringSpecial,rubyNestedParentheses,rubyDelimEscape fold

" Here Document
syn region rubyHeredocStart matchgroup=rubyStringDelimiter start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-\=\zs\%(\h\w*\)+   end=+$+ oneline contains=TOP
syn region rubyHeredocStart matchgroup=rubyStringDelimiter start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-\=\zs"\%([^"]*\)"+ end=+$+ oneline contains=TOP
syn region rubyHeredocStart matchgroup=rubyStringDelimiter start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-\=\zs'\%([^']*\)'+ end=+$+ oneline contains=TOP
syn region rubyHeredocStart matchgroup=rubyStringDelimiter start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-\=\zs`\%([^`]*\)`+ end=+$+ oneline contains=TOP

syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<\z(\h\w*\)\ze+hs=s+2    matchgroup=rubyStringDelimiter end=+^\z1$+ contains=rubyHeredocStart,@rubyStringSpecial fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<"\z([^"]*\)"\ze+hs=s+2  matchgroup=rubyStringDelimiter end=+^\z1$+ contains=rubyHeredocStart,@rubyStringSpecial fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<'\z([^']*\)'\ze+hs=s+2  matchgroup=rubyStringDelimiter end=+^\z1$+ contains=rubyHeredocStart		      fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<`\z([^`]*\)`\ze+hs=s+2  matchgroup=rubyStringDelimiter end=+^\z1$+ contains=rubyHeredocStart,@rubyStringSpecial fold keepend

syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-\z(\h\w*\)\ze+hs=s+3    matchgroup=rubyStringDelimiter end=+^\s*\zs\z1$+ contains=rubyHeredocStart,@rubyStringSpecial fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-"\z([^"]*\)"\ze+hs=s+3  matchgroup=rubyStringDelimiter end=+^\s*\zs\z1$+ contains=rubyHeredocStart,@rubyStringSpecial fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-'\z([^']*\)'\ze+hs=s+3  matchgroup=rubyStringDelimiter end=+^\s*\zs\z1$+ contains=rubyHeredocStart		     fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-`\z([^`]*\)`\ze+hs=s+3  matchgroup=rubyStringDelimiter end=+^\s*\zs\z1$+ contains=rubyHeredocStart,@rubyStringSpecial fold keepend

if exists('main_syntax') && main_syntax == 'eruby'
  let b:ruby_no_expensive = 1
end

syn match  rubyAliasDeclaration    "[^[:space:];#.()]\+"  contained contains=rubySymbol,rubyGlobalVariable,rubyPredefinedVariable nextgroup=rubyAliasDeclaration2 skipwhite
syn match  rubyAliasDeclaration2   "[^[:space:];#.()]\+"  contained contains=rubySymbol,rubyGlobalVariable,rubyPredefinedVariable
syn match  rubyMethodDeclaration   "[^[:space:];#(]\+"	  contained contains=rubyConstant,rubyBoolean,rubyPseudoVariable,rubyInstanceVariable,rubyClassVariable,rubyGlobalVariable
syn match  rubyClassDeclaration    "[^[:space:];#<]\+"	  contained contains=rubyConstant
syn match  rubyModuleDeclaration   "[^[:space:];#]\+"	  contained contains=rubyConstant
syn match  rubyFunction "\<[_[:alpha:]][_[:alnum:]]*[?!=]\=[[:alnum:].:?!=]\@!" contained containedin=rubyMethodDeclaration
syn match  rubyFunction "\%(\s\|^\)\@<=[_[:alpha:]][_[:alnum:]]*[?!=]\=\%(\s\|$\)\@=" contained containedin=rubyAliasDeclaration,rubyAliasDeclaration2
syn match  rubyFunction "\%([[:space:].]\|^\)\@<=\%(\[\]=\=\|\*\*\|[+-]@\=\|[*/%|&^~]\|<<\|>>\|[<>]=\=\|<=>\|===\|==\|=\~\|`\)\%([[:space:];#(]\|$\)\@=" contained containedin=rubyAliasDeclaration,rubyAliasDeclaration2,rubyMethodDeclaration
" Expensive Mode - colorize *end* according to opening statement
if !exists("b:ruby_no_expensive") && !exists("ruby_no_expensive")
  syn match  rubyDefine "\<alias\>"		nextgroup=rubyAliasDeclaration	skipwhite skipnl
  syn match  rubyDefine "\<def\>"		nextgroup=rubyMethodDeclaration skipwhite skipnl
  syn match  rubyClass	"\<class\>"		nextgroup=rubyClassDeclaration	skipwhite skipnl
  syn match  rubyModule "\<module\>"		nextgroup=rubyModuleDeclaration skipwhite skipnl
  syn region rubyBlock start="\<def\>"		matchgroup=rubyDefine end="\%(\<def\_s\+\)\@<!\<end\>" contains=TOP fold
  syn region rubyBlock start="\<class\>"	matchgroup=rubyClass  end="\<end\>" contains=TOP fold
  syn region rubyBlock start="\<module\>"	matchgroup=rubyModule end="\<end\>" contains=TOP fold

  " modifiers
  syn match  rubyConditional "\<\%(if\|unless\)\>"   display
  syn match  rubyRepeat	     "\<\%(while\|until\)\>" display

  " *do* requiring *end*
  syn region rubyDoBlock matchgroup=rubyControl start="\<do\>" end="\<end\>" contains=TOP fold

  " *{* requiring *}*
  syn region rubyCurlyBlock start="{" end="}" contains=TOP fold

  " statements without *do*
  syn region rubyNoDoBlock	  matchgroup=rubyControl     start="\<begin\>" end="\<end\>" contains=TOP fold
  syn region rubyCaseBlock	  matchgroup=rubyConditional start="\<case\>"  end="\<end\>" contains=TOP fold
  syn region rubyConditionalBlock matchgroup=rubyConditional start="\%(\%(^\|\.\.\.\=\|[{:,;([<>~\*/%&^|+-]\|\%(\<[_[:lower:]][_[:alnum:]]*\)\@<![!=?]\)\s*\)\@<=\%(if\|unless\)\>" end="\<end\>" contains=TOP fold
  syn keyword rubyConditional then else when  contained containedin=rubyCaseBlock
  syn keyword rubyConditional then else elsif contained containedin=rubyConditionalBlock

  " statement with optional *do*
  syn region rubyOptDoLine matchgroup=rubyRepeat start="\<for\>" start="\%(\%(^\|\.\.\.\=\|[{:,;([<>~\*/%&^|+-]\|\%(\<[_[:lower:]][_[:alnum:]]*\)\@<![!=?]\)\s*\)\@<=\<\%(until\|while\)\>" end="\%(\<do\>\|:\)" end="\ze\%(;\|$\)" oneline contains=TOP
  syn region rubyOptDoBlock start="\<for\>" start="\%(\%(^\|\.\.\.\=\|[{:,;([<>~\*/%&^|+-]\|\%(\<[_[:lower:]][_[:alnum:]]*\)\@<![!=?]\)\s*\)\@<=\<\%(until\|while\)\>" matchgroup=rubyRepeat end="\<end\>" contains=TOP nextgroup=rubyOptDoLine fold

  if !exists("ruby_minlines")
    let ruby_minlines = 50
  endif
  exec "syn sync minlines=" . ruby_minlines

else
  syn match   rubyControl "\<def\>"	nextgroup=rubyMethodDeclaration skipwhite skipnl
  syn match   rubyControl "\<class\>"	nextgroup=rubyClassDeclaration	skipwhite skipnl
  syn match   rubyControl "\<module\>"	nextgroup=rubyModuleDeclaration skipwhite skipnl
  syn keyword rubyControl case begin do for if unless while until else elsif then when end
  syn keyword rubyKeyword alias
endif

" Keywords
" Note: the following keywords have already been defined:
" begin case class def do end for if module unless until while
syn keyword rubyControl		and break ensure in next not or redo rescue retry return
syn match   rubyOperator	"\<defined?" display
syn keyword rubyKeyword		super undef yield
syn keyword rubyBoolean		true false
syn keyword rubyPseudoVariable	nil self __FILE__ __LINE__
syn keyword rubyBeginEnd	BEGIN END

" Special Methods
if !exists("ruby_no_special_methods")
  syn keyword rubyAccess    public protected private
  syn keyword rubyAttribute attr attr_accessor attr_reader attr_writer
  syn match   rubyControl   "\<\%(exit!\|\%(abort\|at_exit\|exit\|fork\|loop\|trap\)\>\)"
  syn keyword rubyEval	    eval class_eval instance_eval module_eval
  syn keyword rubyException raise fail catch throw
  syn keyword rubyInclude   autoload extend include load require
  syn keyword rubyKeyword   callcc caller lambda proc
endif

" Comments and Documentation
syn match   rubySharpBang     "\%^#!.*" display
syn keyword rubyTodo	      FIXME NOTE TODO XXX contained
syn match   rubyComment       "#.*" contains=rubySharpBang,rubySpaceError,rubyTodo,@Spell
if !exists("ruby_no_comment_fold")
  syn region rubyMultilineComment start="\%(\%(^\s*#.*\n\)\@<!\%(^\s*#.*\n\)\)\%(\(^\s*#.*\n\)\{1,}\)\@=" end="\%(^\s*#.*\n\)\@<=\%(^\s*#.*\n\)\%(^\s*#\)\@!" contains=rubyComment transparent fold keepend
  syn region rubyDocumentation	  start="^=begin\s*$" end="^=end\s*$" contains=rubySpaceError,rubyTodo,@Spell fold
else
  syn region rubyDocumentation	  start="^=begin\s*$" end="^=end\s*$" contains=rubySpaceError,rubyTodo,@Spell
endif

" Note: this is a hack to prevent 'keywords' being highlighted as such when called as methods with an explicit receiver
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(alias\|and\|begin\|break\|case\|class\|def\|defined\|do\|else\)\>"			transparent contains=NONE
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(elsif\|end\|ensure\|false\|for\|if\|in\|module\|next\|nil\)\>"			transparent contains=NONE
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(not\|or\|redo\|rescue\|retry\|return\|self\|super\|then\|true\)\>"			transparent contains=NONE
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(undef\|unless\|until\|when\|while\|yield\|BEGIN\|END\|__FILE__\|__LINE__\)\>"	transparent contains=NONE

syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(abort\|at_exit\|attr\|attr_accessor\|attr_reader\)\>"	transparent contains=NONE
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(attr_writer\|autoload\|callcc\|catch\|caller\)\>"		transparent contains=NONE
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(eval\|class_eval\|instance_eval\|module_eval\|exit\)\>"	transparent contains=NONE
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(extend\|fail\|fork\|include\|lambda\)\>"			transparent contains=NONE
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(load\|loop\|private\|proc\|protected\)\>"			transparent contains=NONE
syn match rubyKeywordAsMethod "\%(\%(\.\@<!\.\)\|::\)\_s*\%(public\|require\|raise\|throw\|trap\)\>"			transparent contains=NONE

" __END__ Directive
syn region rubyData matchgroup=rubyDataDirective start="^__END__$" end="\%$" fold

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_ruby_syntax_inits")
  if version < 508
    let did_ruby_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink rubyClass			rubyDefine
  HiLink rubyModule			rubyDefine
  HiLink rubyDefine			Define
  HiLink rubyFunction			Function
  HiLink rubyConditional		Conditional
  HiLink rubyRepeat			Repeat
  HiLink rubyControl			Statement
  HiLink rubyInclude			Include
  HiLink rubyInteger			Number
  HiLink rubyASCIICode			Character
  HiLink rubyFloat			Float
  HiLink rubyBoolean			Boolean
  HiLink rubyException			Exception
  if !exists("ruby_no_identifiers")
    HiLink rubyIdentifier		Identifier
  else
    HiLink rubyIdentifier		NONE
  endif
  HiLink rubyClassVariable		rubyIdentifier
  HiLink rubyConstant			Type
  HiLink rubyGlobalVariable		rubyIdentifier
  HiLink rubyBlockParameter		rubyIdentifier
  HiLink rubyInstanceVariable		rubyIdentifier
  HiLink rubyPredefinedIdentifier	rubyIdentifier
  HiLink rubyPredefinedConstant		rubyPredefinedIdentifier
  HiLink rubyPredefinedVariable		rubyPredefinedIdentifier
  HiLink rubySymbol			Constant
  HiLink rubyKeyword			Keyword
  HiLink rubyOperator			Operator
  HiLink rubyPseudoOperator		rubyOperator
  HiLink rubyBeginEnd			Statement
  HiLink rubyAccess			Statement
  HiLink rubyAttribute			Statement
  HiLink rubyEval			Statement
  HiLink rubyPseudoVariable		Constant

  HiLink rubyComment			Comment
  HiLink rubyData			Comment
  HiLink rubyDataDirective		Delimiter
  HiLink rubyDocumentation		Comment
  HiLink rubyEscape			Special
  HiLink rubyInterpolationDelimiter	Delimiter
  HiLink rubyNoInterpolation		rubyString
  HiLink rubySharpBang			PreProc
  HiLink rubyRegexpDelimiter		rubyStringDelimiter
  HiLink rubyStringDelimiter		Delimiter
  HiLink rubyRegexp			rubyString
  HiLink rubyString			String
  HiLink rubyTodo			Todo

  HiLink rubyError			Error
  HiLink rubySpaceError			rubyError

  delcommand HiLink
endif

let b:current_syntax = "ruby"

" vim: nowrap sw=2 sts=2 ts=8 noet ff=unix:
