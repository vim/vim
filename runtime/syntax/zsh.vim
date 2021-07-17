" Vim syntax file
" Language:             Zsh shell script
" Maintainer:           Christian Brabandt <cb@256bit.org>
" Previous Maintainer:  Nikolai Weibull <now@bitwi.se>
" Latest Revision:      2020-11-21
" License:              Vim (see :h license)
" Repository:           https://github.com/chrisbra/vim-zsh

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

function! s:ContainedGroup()
  " needs 7.4.2008 for execute() function
  let result='TOP'
    " vim-pandoc syntax defines the @langname cluster for embedded syntax languages
    " However, if no syntax is defined yet, `syn list @zsh` will return
    " "No syntax items defined", so make sure the result is actually a valid syn cluster
    for cluster in ['markdownHighlightzsh', 'zsh']
      try
      " markdown syntax defines embedded clusters as @markdownhighlight<lang>,
      " pandoc just uses @<lang>, so check both for both clusters
        let a=split(execute('syn list @'. cluster), "\n")
        if len(a) == 2 && a[0] =~# '^---' && a[1] =~? cluster
          return  '@'. cluster
        endif
      catch /E392/
        " ignore
      endtry
    endfor
    return result
endfunction

let s:contained=s:ContainedGroup()

syn iskeyword @,48-57,_,192-255,#,-
if get(g:, 'zsh_fold_enable', 0)
    setlocal foldmethod=syntax
endif

syn match   zshPOSIXQuoted      '\\[xX][0-9a-fA-F]\{1,2}'
syn match   zshPOSIXQuoted      '\\[0-7]\{1,3}'
syn match   zshPOSIXQuoted      '\\u[0-9a-fA-F]\{1,4}'
syn match   zshPOSIXQuoted      '\\U[1-9a-fA-F]\{1,8}'
syn match   zshQuoted           '\\.'
syn region  zshString           matchgroup=zshStringDelimiter start=+"+ end=+"+
                                \ contains=zshQuoted,@zshDerefs,@zshSubst fold
syn region  zshString           matchgroup=zshStringDelimiter start=+'+ end=+'+ fold
syn region  zshPOSIXString      matchgroup=zshStringDelimiter start=+\$'+
                                \ skip=+\\[\\']+ end=+'+ contains=zshPOSIXQuoted,zshQuoted
syn match   zshJobSpec          '%\(\d\+\|?\=\w\+\|[%+-]\)'

syn keyword zshPrecommand       noglob nocorrect exec command builtin - time

syn keyword zshDelimiter        do done end

syn keyword zshConditional      if then elif else fi case in esac select

syn keyword zshRepeat           while until repeat

syn keyword zshRepeat           for foreach nextgroup=zshVariable skipwhite

syn keyword zshException        always

syn keyword zshKeyword          function nextgroup=zshKSHFunction skipwhite

syn match   zshKSHFunction      contained '\w\S\+'
syn match   zshFunction         '^\s*\k\+\ze\s*()'

syn match   zshOperator         '||\|&&\|;\|&!\='

syn match   zshRedir            '\d\=\(<\|<>\|<<<\|<&\s*[0-9p-]\=\)'
syn match   zshRedir            '\d\=\(>\|>>\|>&\s*[0-9p-]\=\|&>\|>>&\|&>>\)[|!]\='
syn match   zshRedir            '|&\='

syn region  zshHereDoc          matchgroup=zshRedir
                                \ start='<\@<!<<\s*\z([^<]\S*\)'
                                \ end='^\z1\>'
                                \ contains=@zshSubst,@zshDerefs,zshQuoted,zshPOSIXString
syn region  zshHereDoc          matchgroup=zshRedir
                                \ start='<\@<!<<\s*\\\z(\S\+\)'
                                \ end='^\z1\>'
                                \ contains=@zshSubst,@zshDerefs,zshQuoted,zshPOSIXString
syn region  zshHereDoc          matchgroup=zshRedir
                                \ start='<\@<!<<-\s*\\\=\z(\S\+\)'
                                \ end='^\s*\z1\>'
                                \ contains=@zshSubst,@zshDerefs,zshQuoted,zshPOSIXString
syn region  zshHereDoc          matchgroup=zshRedir
                                \ start=+<\@<!<<\s*\(["']\)\z(\S\+\)\1+
                                \ end='^\z1\>'
syn region  zshHereDoc          matchgroup=zshRedir
                                \ start=+<\@<!<<-\s*\(["']\)\z(\S\+\)\1+
                                \ end='^\s*\z1\>'

syn match   zshVariable         '\<\h\w*' contained

syn match   zshVariableDef      '\<\h\w*\ze+\=='
" XXX: how safe is this?
syn region  zshVariableDef      oneline
                                \ start='\$\@<!\<\h\w*\[' end='\]\ze+\?=\?'
                                \ contains=@zshSubst

syn cluster zshDerefs           contains=zshShortDeref,zshLongDeref,zshDeref,zshDollarVar

syn match zshShortDeref       '\$[!#$*@?_-]\w\@!'
syn match zshShortDeref       '\$[=^~]*[#+]*\d\+\>'

syn match zshLongDeref        '\$\%(ARGC\|argv\|status\|pipestatus\|CPUTYPE\|EGID\|EUID\|ERRNO\|GID\|HOST\|LINENO\|LOGNAME\)'
syn match zshLongDeref        '\$\%(MACHTYPE\|OLDPWD OPTARG\|OPTIND\|OSTYPE\|PPID\|PWD\|RANDOM\|SECONDS\|SHLVL\|signals\)'
syn match zshLongDeref        '\$\%(TRY_BLOCK_ERROR\|TTY\|TTYIDLE\|UID\|USERNAME\|VENDOR\|ZSH_NAME\|ZSH_VERSION\|REPLY\|reply\|TERM\)'

syn match zshDollarVar        '\$\h\w*'
syn match zshDeref            '\$[=^~]*[#+]*\h\w*\>'

syn match   zshCommands         '\%(^\|\s\)[.:]\ze\s'
syn keyword zshCommands         alias autoload bg bindkey break bye cap cd
                                \ chdir clone comparguments compcall compctl
                                \ compdescribe compfiles compgroups compquote
                                \ comptags comptry compvalues continue dirs
                                \ disable disown echo echotc echoti emulate
                                \ enable eval exec exit export false fc fg
                                \ functions getcap getln getopts hash history
                                \ jobs kill let limit log logout popd print
                                \ printf pushd pushln pwd r read
                                \ rehash return sched set setcap shift
                                \ source stat suspend test times trap true
                                \ ttyctl type ulimit umask unalias unfunction
                                \ unhash unlimit unset  vared wait
                                \ whence where which zcompile zformat zftp zle
                                \ zmodload zparseopts zprof zpty zrecompile
                                \ zregexparse zsocket zstyle ztcp

" Options, generated by from the zsh source with:
"
"     #!/bin/zsh
"     topdir=/path/to/zsh/source
"     all=()
"     for opt in $(grep '^pindex([A-Za-z_]*)$' $topdir/Doc/Zsh/options.yo); do
"     	x=${${(L)opt#pindex\(}%\)}
"     	[[ $x =~ '^no' ]] || all+=(${x})
"     done
"     print "syn match   zshOption nextgroup=zshOption,zshComment skipwhite contained /\\\v"
"     print "            \\\ <%(no_?)?%("
"     print "            \\\ ${(oj:|:)all//_/_?}"
"     print "            \\\ )>/"
syn case ignore
syn match   zshOptStart
            \ /\v^\s*%(%(un)?setopt|set\s+[-+]o)/
            \ nextgroup=zshOption skipwhite
syn match   zshOption nextgroup=zshOption,zshComment skipwhite contained /\v
            \ <%(no_?)?%(
            \ auto_?cd|autocd|auto_?pushd|autopushd|cdable_?vars|cdablevars|cd_?silent|cdsilent|chase_?dots|chasedots|chase_?links|chaselinks|posix_?cd|posixcd|pushd_?ignore_?dups|pushdignoredups|pushd_?minus|pushdminus|pushd_?silent|pushdsilent|pushd_?to_?home|pushdtohome|always_?last_?prompt|alwayslastprompt|always_?to_?end|alwaystoend|auto_?list|autolist|auto_?menu|automenu|auto_?name_?dirs|autonamedirs|auto_?param_?keys|autoparamkeys|auto_?param_?slash|autoparamslash|auto_?remove_?slash|autoremoveslash|bash_?auto_?list|bashautolist|complete_?aliases|completealiases|complete_?in_?word|completeinword|glob_?complete|globcomplete|hash_?list_?all|hashlistall|list_?ambiguous|listambiguous|list_?beep|listbeep|list_?packed|listpacked|list_?rows_?first|listrowsfirst|list_?types|listtypes|menu_?complete|menucomplete|rec_?exact|recexact|bad_?pattern|badpattern|bare_?glob_?qual|bareglobqual|brace_?ccl|braceccl|case_?glob|caseglob|case_?match|casematch|case_?paths|casepaths|csh_?null_?glob|cshnullglob|equals|extended_?glob|extendedglob|force_?float|forcefloat|glob|glob_?assign|globassign|glob_?dots|globdots|glob_?star_?short|globstarshort|glob_?subst|globsubst|hist_?subst_?pattern|histsubstpattern|ignore_?braces|ignorebraces|ignore_?close_?braces|ignoreclosebraces|ksh_?glob|kshglob|magic_?equal_?subst|magicequalsubst|mark_?dirs|markdirs|multibyte|null_?glob|nullglob|numeric_?glob_?sort|numericglobsort|rc_?expand_?param|rcexpandparam|rematch_?pcre|rematchpcre|sh_?glob|shglob|unset|warn_?create_?global|warncreateglobal|warn_?nested_?var|warnnestedvar|append_?history|appendhistory|bang_?hist|banghist|extended_?history|extendedhistory|hist_?allow_?clobber|histallowclobber|hist_?beep|histbeep|hist_?expire_?dups_?first|histexpiredupsfirst|hist_?fcntl_?lock|histfcntllock|hist_?find_?no_?dups|histfindnodups|hist_?ignore_?all_?dups|histignorealldups|hist_?ignore_?dups|histignoredups|hist_?ignore_?space|histignorespace|hist_?lex_?words|histlexwords|hist_?no_?functions|histnofunctions|hist_?no_?store|histnostore|hist_?reduce_?blanks|histreduceblanks|hist_?save_?by_?copy|histsavebycopy|hist_?save_?no_?dups|histsavenodups|hist_?verify|histverify|inc_?append_?history|incappendhistory|inc_?append_?history_?time|incappendhistorytime|share_?history|sharehistory|all_?export|allexport|global_?export|globalexport|global_?rcs|globalrcs|rcs|aliases|clobber|clobber_?empty|clobberempty|correct|correct_?all|correctall|dvorak|flow_?control|flowcontrol|ignore_?eof|ignoreeof|interactive_?comments|interactivecomments|hash_?cmds|hashcmds|hash_?dirs|hashdirs|hash_?executables_?only|hashexecutablesonly|mail_?warning|mailwarning|path_?dirs|pathdirs|path_?script|pathscript|print_?eight_?bit|printeightbit|print_?exit_?value|printexitvalue|rc_?quotes|rcquotes|rm_?star_?silent|rmstarsilent|rm_?star_?wait|rmstarwait|short_?loops|shortloops|short_?repeat|shortrepeat|sun_?keyboard_?hack|sunkeyboardhack|auto_?continue|autocontinue|auto_?resume|autoresume|bg_?nice|bgnice|check_?jobs|checkjobs|check_?running_?jobs|checkrunningjobs|hup|long_?list_?jobs|longlistjobs|monitor|posix_?jobs|posixjobs|prompt_?bang|promptbang|prompt_?cr|promptcr|prompt_?sp|promptsp|prompt_?percent|promptpercent|prompt_?subst|promptsubst|transient_?rprompt|transientrprompt|alias_?func_?def|aliasfuncdef|c_?bases|cbases|c_?precedences|cprecedences|debug_?before_?cmd|debugbeforecmd|err_?exit|errexit|err_?return|errreturn|eval_?lineno|evallineno|exec|function_?argzero|functionargzero|local_?loops|localloops|local_?options|localoptions|local_?patterns|localpatterns|local_?traps|localtraps|multi_?func_?def|multifuncdef|multios|octal_?zeroes|octalzeroes|pipe_?fail|pipefail|source_?trace|sourcetrace|typeset_?silent|typesetsilent|typeset_?to_?unset|typesettounset|verbose|xtrace|append_?create|appendcreate|bash_?rematch|bashrematch|bsd_?echo|bsdecho|continue_?on_?error|continueonerror|csh_?junkie_?history|cshjunkiehistory|csh_?junkie_?loops|cshjunkieloops|csh_?junkie_?quotes|cshjunkiequotes|csh_?nullcmd|cshnullcmd|ksh_?arrays|ksharrays|ksh_?autoload|kshautoload|ksh_?option_?print|kshoptionprint|ksh_?typeset|kshtypeset|ksh_?zero_?subscript|kshzerosubscript|posix_?aliases|posixaliases|posix_?argzero|posixargzero|posix_?builtins|posixbuiltins|posix_?identifiers|posixidentifiers|posix_?strings|posixstrings|posix_?traps|posixtraps|sh_?file_?expansion|shfileexpansion|sh_?nullcmd|shnullcmd|sh_?option_?letters|shoptionletters|sh_?word_?split|shwordsplit|traps_?async|trapsasync|interactive|login|privileged|restricted|shin_?stdin|shinstdin|single_?command|singlecommand|beep|combining_?chars|combiningchars|emacs|overstrike|single_?line_?zle|singlelinezle|vi|zle|brace_?expand|braceexpand|dot_?glob|dotglob|hash_?all|hashall|hist_?append|histappend|hist_?expand|histexpand|log|mail_?warn|mailwarn|one_?cmd|onecmd|physical|prompt_?vars|promptvars|stdin|track_?all|trackall
            \ )>/
syn case match

syn keyword zshTypes            float integer local typeset declare private readonly

" XXX: this may be too much
" syn match   zshSwitches         '\s\zs--\=[a-zA-Z0-9-]\+'

syn match   zshNumber           '[+-]\=\<\d\+\>'
syn match   zshNumber           '[+-]\=\<0x\x\+\>'
syn match   zshNumber           '[+-]\=\<0\o\+\>'
syn match   zshNumber           '[+-]\=\d\+#[-+]\=\w\+\>'
syn match   zshNumber           '[+-]\=\d\+\.\d\+\>'

" TODO: $[...] is the same as $((...)), so add that as well.
syn cluster zshSubst            contains=zshSubst,zshOldSubst,zshMathSubst
exe 'syn region  zshSubst       matchgroup=zshSubstDelim transparent start=/\$(/ skip=/\\)/ end=/)/ contains='.s:contained. '  fold'
syn region  zshParentheses      transparent start='(' skip='\\)' end=')' fold
syn region  zshGlob             start='(#' end=')'
syn region  zshMathSubst        matchgroup=zshSubstDelim transparent
                                \ start='\%(\$\?\)[<=>]\@<!((' skip='\\)' end='))'
                                \ contains=zshParentheses,@zshSubst,zshNumber,
                                \ @zshDerefs,zshString keepend fold
" The ms=s+1 prevents matching zshBrackets several times on opening brackets
" (see https://github.com/chrisbra/vim-zsh/issues/21#issuecomment-576330348)
syn region  zshBrackets         contained transparent start='{'ms=s+1 skip='\\}'
                                \ end='}' fold
exe 'syn region  zshBrackets    transparent start=/{/ms=s+1 skip=/\\}/ end=/}/ contains='.s:contained. ' fold'

syn region  zshSubst            matchgroup=zshSubstDelim start='\${' skip='\\}'
                                \ end='}' contains=@zshSubst,zshBrackets,zshQuoted,zshString fold
exe 'syn region  zshOldSubst    matchgroup=zshSubstDelim start=/`/ skip=/\\[\\`]/ end=/`/ contains='.s:contained. ',zshOldSubst fold'

syn sync    minlines=50 maxlines=90
syn sync    match zshHereDocSync    grouphere   NONE '<<-\=\s*\%(\\\=\S\+\|\(["']\)\S\+\1\)'
syn sync    match zshHereDocEndSync groupthere  NONE '^\s*EO\a\+\>'

syn keyword zshTodo             contained TODO FIXME XXX NOTE

syn region  zshComment          oneline start='\%(^\|\s\+\)#' end='$'
                                \ contains=zshTodo,@Spell fold

syn region  zshComment          start='^\s*#' end='^\%(\s*#\)\@!'
                                \ contains=zshTodo,@Spell fold

syn match   zshPreProc          '^\%1l#\%(!\|compdef\|autoload\).*$'

hi def link zshTodo             Todo
hi def link zshComment          Comment
hi def link zshPreProc          PreProc
hi def link zshQuoted           SpecialChar
hi def link zshPOSIXQuoted      SpecialChar
hi def link zshString           String
hi def link zshStringDelimiter  zshString
hi def link zshPOSIXString      zshString
hi def link zshJobSpec          Special
hi def link zshPrecommand       Special
hi def link zshDelimiter        Keyword
hi def link zshConditional      Conditional
hi def link zshException        Exception
hi def link zshRepeat           Repeat
hi def link zshKeyword          Keyword
hi def link zshFunction         None
hi def link zshKSHFunction      zshFunction
hi def link zshHereDoc          String
hi def link zshOperator         None
hi def link zshRedir            Operator
hi def link zshVariable         None
hi def link zshVariableDef      zshVariable
hi def link zshDereferencing    PreProc
hi def link zshShortDeref       zshDereferencing
hi def link zshLongDeref        zshDereferencing
hi def link zshDeref            zshDereferencing
hi def link zshDollarVar        zshDereferencing
hi def link zshCommands         Keyword
hi def link zshOptStart         Keyword
hi def link zshOption           Constant
hi def link zshTypes            Type
hi def link zshSwitches         Special
hi def link zshNumber           Number
hi def link zshSubst            PreProc
hi def link zshMathSubst        zshSubst
hi def link zshOldSubst         zshSubst
hi def link zshSubstDelim       zshSubst
hi def link zshGlob             zshSubst

let b:current_syntax = "zsh"

let &cpo = s:cpo_save
unlet s:cpo_save
