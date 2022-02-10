" When you're writing shell scripts and you are in doubt which test to use,
" which shell environment variables are defined, what the syntax of the case
" statement is, and you need to invoke 'man sh'?
"
" Your problems are over now!
"
" Attached is a Vim script file for turning gvim into a shell script editor.
" It may also be used as an example how to use menus in Vim.
"
" Maintainer: Ada (Haowen) Yu <me@yuhaowen.com>
" Original author: Lennart Schultz <les@dmi.min.dk> (mail unreachable)

" Make sure the '<' and 'C' flags are not included in 'cpoptions', otherwise
" <CR> would not be recognized.  See ":help 'cpoptions'".
let s:cpo_save = &cpo
set cpo&vim

imenu Stmts.for	for  in <CR>do<CR><CR>done<esc>ki	<esc>kk0elli
imenu Stmts.case	case  in<CR>) ;;<CR>esac<esc>bki	<esc>k0elli
imenu Stmts.if	if   <CR>then<CR><CR>fi<esc>ki	<esc>kk0elli
imenu Stmts.if-else	if   <CR>then<CR><CR>else<CR><CR>fi<esc>ki	<esc>kki	<esc>kk0elli
imenu Stmts.elif	elif   <CR>then<CR><CR><esc>ki	<esc>kk0elli
imenu Stmts.while	while   do<CR><CR>done<esc>ki	<esc>kk0elli
imenu Stmts.break	break 
imenu Stmts.continue	continue 
imenu Stmts.function	() {<CR><CR>}<esc>ki	<esc>k0i
imenu Stmts.return	return 
imenu Stmts.return-true	return 0
imenu Stmts.return-false	return 1
imenu Stmts.exit	exit 
imenu Stmts.shift	shift 
imenu Stmts.trap	trap 
imenu Test.existence	[ -e  ]<esc>hi
imenu Test.existence\ -\ file		[ -f  ]<esc>hi
imenu Test.existence\ -\ file\ (not\ empty)	[ -s  ]<esc>hi
imenu Test.existence\ -\ directory	[ -d  ]<esc>hi
imenu Test.existence\ -\ executable	[ -x  ]<esc>hi
imenu Test.existence\ -\ readable	[ -r  ]<esc>hi
imenu Test.existence\ -\ writable	[ -w  ]<esc>hi
imenu Test.String\ is\ empty [ x = "x$" ]<esc>hhi
imenu Test.String\ is\ not\ empty [ x != "x$" ]<esc>hhi
imenu Test.Strings\ is\ equal [ "" = "" ]<esc>hhhhhhhi
imenu Test.Strings\ is\ not\ equal [ "" != "" ]<esc>hhhhhhhhi
imenu Test.Values\ is\ greater\ than [  -gt  ]<esc>hhhhhhi
imenu Test.Values\ is\ greater\ equal [  -ge  ]<esc>hhhhhhi
imenu Test.Values\ is\ equal [  -eq  ]<esc>hhhhhhi
imenu Test.Values\ is\ not\ equal [  -ne  ]<esc>hhhhhhi
imenu Test.Values\ is\ less\ than [  -lt  ]<esc>hhhhhhi
imenu Test.Values\ is\ less\ equal [  -le  ]<esc>hhhhhhi
imenu ParmSub.Substitute\ word\ if\ parm\ not\ set ${:-}<esc>hhi
imenu ParmSub.Set\ parm\ to\ word\ if\ not\ set ${:=}<esc>hhi
imenu ParmSub.Substitute\ word\ if\ parm\ set\ else\ nothing ${:+}<esc>hhi
imenu ParmSub.If\ parm\ not\ set\ print\ word\ and\ exit ${:?}<esc>hhi
imenu SpShVars.Number\ of\ positional\ parameters ${#}
imenu SpShVars.All\ positional\ parameters\ (quoted\ spaces) ${*}
imenu SpShVars.All\ positional\ parameters\ (unquoted\ spaces) ${@}
imenu SpShVars.Flags\ set ${-}
imenu SpShVars.Return\ code\ of\ last\ command ${?}
imenu SpShVars.Process\ number\ of\ this\ shell ${$}
imenu SpShVars.Process\ number\ of\ last\ background\ command ${!}
imenu Environ.HOME ${HOME}
imenu Environ.PATH ${PATH}
imenu Environ.CDPATH ${CDPATH}
imenu Environ.MAIL ${MAIL}
imenu Environ.MAILCHECK ${MAILCHECK}
imenu Environ.PS1 ${PS1}
imenu Environ.PS2 ${PS2}
imenu Environ.IFS ${IFS}
imenu Environ.SHACCT ${SHACCT}
imenu Environ.SHELL ${SHELL}
imenu Environ.LC_CTYPE ${LC_CTYPE}
imenu Environ.LC_MESSAGES ${LC_MESSAGES}
imenu Builtins.cd cd
imenu Builtins.echo echo
imenu Builtins.eval eval
imenu Builtins.exec exec
imenu Builtins.export export
imenu Builtins.getopts getopts
imenu Builtins.hash hash
imenu Builtins.newgrp newgrp
imenu Builtins.pwd pwd
imenu Builtins.read read
imenu Builtins.readonly readonly
imenu Builtins.return return
imenu Builtins.times times
imenu Builtins.type type
imenu Builtins.umask umask
imenu Builtins.wait wait
imenu Set.set set
imenu Set.unset unset
imenu Set.mark\ modified\ or\ modified\ variables set -a
imenu Set.exit\ when\ command\ returns\ non-zero\ exit\ code set -e
imenu Set.Disable\ file\ name\ generation set -f
imenu Set.remember\ function\ commands set -h
imenu Set.All\ keyword\ arguments\ are\ placed\ in\ the\ environment set -k
imenu Set.Read\ commands\ but\ do\ not\ execute\ them set -n
imenu Set.Exit\ after\ reading\ and\ executing\ one\ command set -t
imenu Set.Treat\ unset\ variables\ as\ an\ error\ when\ substituting set -u
imenu Set.Print\ shell\ input\ lines\ as\ they\ are\ read set -v
imenu Set.Print\ commands\ and\ their\ arguments\ as\ they\ are\ executed set -x

" Restore the previous value of 'cpoptions'.
let &cpo = s:cpo_save
unlet s:cpo_save
