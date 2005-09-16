" Vim syntax file
" Language:	Ruby
" Maintainer:	Doug Kearns <djkea2 at gus.gscit.monash.edu.au>
" Info:		$Id$
" URL:		http://vim-ruby.sourceforge.net
" Anon CVS:	See above site
" Licence:	GPL (http://www.gnu.org)
" Disclaimer:
"    This program is distributed in the hope that it will be useful,
"    but WITHOUT ANY WARRANTY; without even the implied warranty of
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
"    GNU General Public License for more details.
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

" Expression Substitution and Backslash Notation
syn match rubyEscape		"\\\\\|\\[abefnrstv]\|\\\o\{1,3}\|\\x\x\{1,2}"								contained display
syn match rubyEscape		"\%(\\M-\\C-\|\\C-\\M-\|\\M-\\c\|\\c\\M-\|\\c\|\\C-\|\\M-\)\%(\\\o\{1,3}\|\\x\x\{1,2}\|\\\=\S\)"	contained display
syn match rubyInterpolation	"#{[^}]*}"				contained
syn match rubyInterpolation	"#\%(\$\|@@\=\)\w\+"			contained display
syn match rubyNoInterpolation	"\\#{[^}]*}"				contained
syn match rubyNoInterpolation	"\\#\%(\$\|@@\=\)\w\+"			contained display

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

syn match  rubyConstant			"\%(\%(\.\@<!\.\)\@<!\<\|::\)\_s*\zs\u\w*\>\%(\s*(\)\@!"
syn match  rubyClassVariable		"@@\h\w*" display
syn match  rubyInstanceVariable		"@\h\w*"  display
syn match  rubyGlobalVariable		"$\%(\h\w*\|-.\)"
syn match  rubySymbol			":\@<!:\%(\^\|\~\|<<\|<=>\|<=\|<\|===\|==\|=\~\|>>\|>=\|>\||\|-@\|-\|/\|\[]=\|\[]\|\*\*\|\*\|&\|%\|+@\|+\|`\)"
syn match  rubySymbol			":\@<!:\$\%(-.\|[`~<=>_,;:!?/.'"@$*\&+0]\)"
syn match  rubySymbol			":\@<!:\%(\$\|@@\=\)\=\h\w*[?!=]\="
syn region rubySymbol			start=":\@<!:\"" end="\"" skip="\\\\\|\\\""
syn match  rubyBlockParameter		"\%(\%(\<do\>\|{\)\s*\)\@<=|\s*\zs[( ,a-zA-Z0-9_*)]\+\ze\s*|" display

syn match rubyPredefinedVariable #$[!$&"'*+,./0:;<=>?@\`~1-9]#
syn match rubyPredefinedVariable "$_\>"								display
syn match rubyPredefinedVariable "$-[0FIKadilpvw]\>"						display
syn match rubyPredefinedVariable "$\%(deferr\|defout\|stderr\|stdin\|stdout\)\>"		display
syn match rubyPredefinedVariable "$\%(DEBUG\|FILENAME\|KCODE\|LOAD_PATH\|SAFE\|VERBOSE\)\>"	display
syn match rubyPredefinedConstant "\%(\%(\.\@<!\.\)\@<!\|::\)\_s*\zs\%(MatchingData\|ARGF\|ARGV\|ENV\)\>\%(\s*(\)\@!"
syn match rubyPredefinedConstant "\%(\%(\.\@<!\.\)\@<!\|::\)\_s*\zs\%(DATA\|FALSE\|NIL\|RUBY_PLATFORM\|RUBY_RELEASE_DATE\)\>\%(\s*(\)\@!"
syn match rubyPredefinedConstant "\%(\%(\.\@<!\.\)\@<!\|::\)\_s*\zs\%(RUBY_VERSION\|STDERR\|STDIN\|STDOUT\|TOPLEVEL_BINDING\|TRUE\)\>\%(\s*(\)\@!"
"Obsolete Global Constants
"syn match rubyPredefinedConstant "\%(::\)\=\zs\%(PLATFORM\|RELEASE_DATE\|VERSION\)\>"
"syn match rubyPredefinedConstant "\%(::\)\=\zs\%(NotImplementError\)\>"

" Normal Regular Expression
syn region rubyString matchgroup=rubyStringDelimiter start="\%(\%(^\|\<\%(and\|or\|while\|until\|unless\|if\|elsif\|when\|not\|then\)\|[\~=!|&(,[]\)\s*\)\@<=/" end="/[iomx]*" skip="\\\\\|\\/" contains=@rubyStringSpecial
syn region rubyString matchgroup=rubyStringDelimiter start="\%(\<\%(split\|scan\|gsub\|sub\)\s*\)\@<=/" end="/[iomx]*" skip="\\\\\|\\/" contains=@rubyStringSpecial

" Normal String and Shell Command Output
syn region rubyString matchgroup=rubyStringDelimiter start="\"" end="\"" skip="\\\\\|\\\"" contains=@rubyStringSpecial
syn region rubyString matchgroup=rubyStringDelimiter start="'"  end="'"  skip="\\\\\|\\'"
syn region rubyString matchgroup=rubyStringDelimiter start="`"  end="`"  skip="\\\\\|\\`"  contains=@rubyStringSpecial

" Generalized Regular Expression
syn region rubyString matchgroup=rubyStringDelimiter start="%r\z([~`!@#$%^&*_\-+=|\:;"',.?/]\)"	end="\z1[iomx]*" skip="\\\\\|\\\z1" contains=@rubyStringSpecial fold
syn region rubyString matchgroup=rubyStringDelimiter start="%r{"				end="}[iomx]*"	 skip="\\\\\|\\}"   contains=@rubyStringSpecial,rubyNestedCurlyBraces,rubyDelimEscape fold
syn region rubyString matchgroup=rubyStringDelimiter start="%r<"				end=">[iomx]*"	 skip="\\\\\|\\>"   contains=@rubyStringSpecial,rubyNestedAngleBrackets,rubyDelimEscape fold
syn region rubyString matchgroup=rubyStringDelimiter start="%r\["				end="\][iomx]*"	 skip="\\\\\|\\\]"  contains=@rubyStringSpecial,rubyNestedSquareBrackets,rubyDelimEscape fold
syn region rubyString matchgroup=rubyStringDelimiter start="%r("				end=")[iomx]*"	 skip="\\\\\|\\)"   contains=@rubyStringSpecial,rubyNestedParentheses,rubyDelimEscape fold

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

syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<\z(\h\w*\)\ze+hs=s+2    matchgroup=rubyStringDelimiter end=+^\z1$+ contains=rubyHeredocStart,@rubyStringSpecial nextgroup=rubyFunction fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<"\z([^"]*\)"\ze+hs=s+2  matchgroup=rubyStringDelimiter end=+^\z1$+ contains=rubyHeredocStart,@rubyStringSpecial nextgroup=rubyFunction fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<'\z([^']*\)'\ze+hs=s+2  matchgroup=rubyStringDelimiter end=+^\z1$+ contains=rubyHeredocStart		      nextgroup=rubyFunction fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<`\z([^`]*\)`\ze+hs=s+2  matchgroup=rubyStringDelimiter end=+^\z1$+ contains=rubyHeredocStart,@rubyStringSpecial nextgroup=rubyFunction fold keepend

syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-\z(\h\w*\)\ze+hs=s+3    matchgroup=rubyStringDelimiter end=+^\s*\zs\z1$+ contains=rubyHeredocStart,@rubyStringSpecial nextgroup=rubyFunction fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-"\z([^"]*\)"\ze+hs=s+3  matchgroup=rubyStringDelimiter end=+^\s*\zs\z1$+ contains=rubyHeredocStart,@rubyStringSpecial nextgroup=rubyFunction fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-'\z([^']*\)'\ze+hs=s+3  matchgroup=rubyStringDelimiter end=+^\s*\zs\z1$+ contains=rubyHeredocStart		     nextgroup=rubyFunction fold keepend
syn region rubyString start=+\%(\%(class\s*\|\%(\.\|::\)\)\_s*\)\@<!<<-`\z([^`]*\)`\ze+hs=s+3  matchgroup=rubyStringDelimiter end=+^\s*\zs\z1$+ contains=rubyHeredocStart,@rubyStringSpecial nextgroup=rubyFunction fold keepend

if exists('main_syntax') && main_syntax == 'eruby'
  let ruby_no_expensive = 1
end

" Expensive Mode - colorize *end* according to opening statement
if !exists("ruby_no_expensive")
  syn region rubyFunction matchgroup=rubyDefine start="\<def\s\+"    end="\ze\%(\s\|(\|;\|$\)" oneline
  syn region rubyClass    matchgroup=rubyDefine start="\<class\s\+"  end="\ze\%(\s\|<\|;\|$\)" oneline
  syn match  rubyDefine   "\<class\ze<<"
  syn region rubyModule   matchgroup=rubyDefine start="\<module\s\+" end="\ze\%(\s\|;\|$\)"    oneline

  syn region rubyBlock start="\<def\>"    matchgroup=rubyDefine end="\<end\>" contains=ALLBUT,@rubyExtendedStringSpecial,rubyTodo nextgroup=rubyFunction fold
  syn region rubyBlock start="\<class\>"  matchgroup=rubyDefine end="\<end\>" contains=ALLBUT,@rubyExtendedStringSpecial,rubyTodo nextgroup=rubyClass    fold
  syn region rubyBlock start="\<module\>" matchgroup=rubyDefine end="\<end\>" contains=ALLBUT,@rubyExtendedStringSpecial,rubyTodo nextgroup=rubyModule   fold

  " modifiers
  syn match  rubyControl "\<\%(if\|unless\|while\|until\)\>" display

  " *do* requiring *end*
  syn region rubyDoBlock matchgroup=rubyControl start="\<do\>" end="\<end\>" contains=ALLBUT,@rubyExtendedStringSpecial,rubyTodo fold

  " *{* requiring *}*
  syn region rubyCurlyBlock start="{" end="}" contains=ALLBUT,@rubyExtendedStringSpecial,rubyTodo fold

  " statements without *do*
  syn region rubyNoDoBlock matchgroup=rubyControl start="\<\%(case\|begin\)\>" start="\%(^\|\.\.\.\=\|[,;=([<>~\*/%!&^|+-]\)\s*\zs\%(if\|unless\)\>" end="\<end\>" contains=ALLBUT,@rubyExtendedStringSpecial,rubyTodo fold

  " statement with optional *do*
  syn region rubyOptDoLine matchgroup=rubyControl start="\<for\>" start="\%(\%(^\|\.\.\.\=\|[,;=([<>~\*/%!&^|+-]\)\s*\)\@<=\<\%(until\|while\)\>" end="\%(\<do\>\|:\)" end="\ze\%(;\|$\)" oneline contains=ALLBUT,@rubyExtendedStringSpecial,rubyTodo
  syn region rubyOptDoBlock start="\<for\>" start="\%(\%(^\|\.\.\.\=\|[,;=([<>~\*/%!&^|+-]\)\s*\)\@<=\<\%(until\|while\)\>" matchgroup=rubyControl end="\<end\>" contains=ALLBUT,@rubyExtendedStringSpecial,rubyTodo nextgroup=rubyOptDoLine fold

  if !exists("ruby_minlines")
    let ruby_minlines = 50
  endif
  exec "syn sync minlines=" . ruby_minlines

else
  syn region  rubyFunction matchgroup=rubyControl start="\<def\s\+"    end="\ze\%(\s\|(\|;\|$\)" oneline
  syn region  rubyClass    matchgroup=rubyControl start="\<class\s\+"  end="\ze\%(\s\|<\|;\|$\)" oneline
  syn match   rubyControl  "\<class\ze<<"
  syn region  rubyModule   matchgroup=rubyControl start="\<module\s\+" end="\ze\%(\s\|;\|$\)"    oneline
  syn keyword rubyControl case begin do for if unless while until end
endif

" Keywords
" Note: the following keywords have already been defined:
" begin case class def do end for if module unless until while
syn keyword rubyControl		and break else elsif ensure in next not or redo rescue retry return then when
syn match   rubyOperator	"\<defined?" display
syn keyword rubyKeyword		alias super undef yield
syn keyword rubyBoolean		true false
syn keyword rubyPseudoVariable	nil self __FILE__ __LINE__
syn keyword rubyBeginEnd	BEGIN END

" Special Methods
if !exists("ruby_no_special_methods")
  syn keyword rubyAccess    public protected private
  syn keyword rubyAttribute attr attr_accessor attr_reader attr_writer
  syn keyword rubyControl   abort at_exit exit fork loop trap
  syn keyword rubyEval      eval class_eval instance_eval module_eval
  syn keyword rubyException raise fail catch throw
  syn keyword rubyInclude   autoload extend include load require
  syn keyword rubyKeyword   callcc caller lambda proc
endif

" Comments and Documentation
syn match   rubySharpBang     "\%^#!.*" display
syn keyword rubyTodo          FIXME NOTE TODO XXX contained
syn match   rubyComment       "#.*" contains=rubySharpBang,rubySpaceError,rubyTodo,@Spell
syn region  rubyDocumentation start="^=begin" end="^=end.*$" contains=rubySpaceError,rubyTodo,@Spell fold

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

  HiLink rubyDefine			Define
  HiLink rubyFunction			Function
  HiLink rubyControl			Statement
  HiLink rubyInclude			Include
  HiLink rubyInteger			Number
  HiLink rubyASCIICode			rubyInteger
  HiLink rubyFloat			Float
  HiLink rubyBoolean			rubyPseudoVariable
  HiLink rubyException			Exception
  HiLink rubyClass			Type
  HiLink rubyModule			Type
  if !exists("ruby_no_identifiers")
    HiLink rubyIdentifier		Identifier
  else
    HiLink rubyIdentifier		NONE
  endif
  HiLink rubyClassVariable		rubyIdentifier
  HiLink rubyConstant			rubyIdentifier
  HiLink rubyGlobalVariable		rubyIdentifier
  HiLink rubyBlockParameter		rubyIdentifier
  HiLink rubyInstanceVariable		rubyIdentifier
  HiLink rubyPredefinedIdentifier	rubyIdentifier
  HiLink rubyPredefinedConstant		rubyPredefinedIdentifier
  HiLink rubyPredefinedVariable		rubyPredefinedIdentifier
  HiLink rubySymbol			rubyIdentifier
  HiLink rubyKeyword			Keyword
  HiLink rubyOperator			Operator
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
  HiLink rubyInterpolation		Special
  HiLink rubyNoInterpolation		rubyString
  HiLink rubySharpBang			PreProc
  HiLink rubyStringDelimiter		Delimiter
  HiLink rubyString			String
  HiLink rubyTodo			Todo

  HiLink rubyError			Error
  HiLink rubySpaceError			rubyError

  delcommand HiLink
endif

let b:current_syntax = "ruby"

" vim: nowrap sw=2 sts=2 ts=8 ff=unix:
