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

imenu ShellMemu.Statements.for	for  in <CR>do<CR><CR>done<esc>ki	<esc>kk0elli
imenu ShellMemu.Statements.case	case  in<CR>) ;;<CR>esac<esc>bki	<esc>k0elli
imenu ShellMemu.Statements.if	if   <CR>then<CR><CR>fi<esc>ki	<esc>kk0elli
imenu ShellMemu.Statements.if-else	if   <CR>then<CR><CR>else<CR><CR>fi<esc>ki	<esc>kki	<esc>kk0elli
imenu ShellMemu.Statements.elif	elif   <CR>then<CR><CR><esc>ki	<esc>kk0elli
imenu ShellMemu.Statements.while	while   do<CR><CR>done<esc>ki	<esc>kk0elli
imenu ShellMemu.Statements.break	break 
imenu ShellMemu.Statements.continue	continue 
imenu ShellMemu.Statements.function	() {<CR><CR>}<esc>ki	<esc>k0i
imenu ShellMemu.Statements.return	return 
imenu ShellMemu.Statements.return-true	return 0
imenu ShellMemu.Statements.return-false	return 1
imenu ShellMemu.Statements.exit	exit 
imenu ShellMemu.Statements.shift	shift 
imenu ShellMemu.Statements.trap	trap 
imenu ShellMemu.Test.existence	[ -e  ]<esc>hi
imenu ShellMemu.Test.existence\ -\ file		[ -f  ]<esc>hi
imenu ShellMemu.Test.existence\ -\ file\ (not\ empty)	[ -s  ]<esc>hi
imenu ShellMemu.Test.existence\ -\ directory	[ -d  ]<esc>hi
imenu ShellMemu.Test.existence\ -\ executable	[ -x  ]<esc>hi
imenu ShellMemu.Test.existence\ -\ readable	[ -r  ]<esc>hi
imenu ShellMemu.Test.existence\ -\ writable	[ -w  ]<esc>hi
imenu ShellMemu.Test.String\ is\ empty [ x = "x$" ]<esc>hhi
imenu ShellMemu.Test.String\ is\ not\ empty [ x != "x$" ]<esc>hhi
imenu ShellMemu.Test.Strings\ is\ equal [ "" = "" ]<esc>hhhhhhhi
imenu ShellMemu.Test.Strings\ is\ not\ equal [ "" != "" ]<esc>hhhhhhhhi
imenu ShellMemu.Test.Value\ is\ greater\ than [  -gt  ]<esc>hhhhhhi
imenu ShellMemu.Test.Value\ is\ greater\ equal [  -ge  ]<esc>hhhhhhi
imenu ShellMemu.Test.Value\ is\ equal [  -eq  ]<esc>hhhhhhi
imenu ShellMemu.Test.Value\ is\ not\ equal [  -ne  ]<esc>hhhhhhi
imenu ShellMemu.Test.Value\ is\ less\ than [  -lt  ]<esc>hhhhhhi
imenu ShellMemu.Test.Value\ is\ less\ equal [  -le  ]<esc>hhhhhhi
imenu ShellMemu.ParmSub.Substitute\ word\ if\ parm\ not\ set ${:-}<esc>hhi
imenu ShellMemu.ParmSub.Set\ parm\ to\ word\ if\ not\ set ${:=}<esc>hhi
imenu ShellMemu.ParmSub.Substitute\ word\ if\ parm\ set\ else\ nothing ${:+}<esc>hhi
imenu ShellMemu.ParmSub.If\ parm\ not\ set\ print\ word\ and\ exit ${:?}<esc>hhi
imenu ShellMemu.SpShVars.Number\ of\ positional\ parameters ${#}
imenu ShellMemu.SpShVars.All\ positional\ parameters\ (quoted\ spaces) ${*}
imenu ShellMemu.SpShVars.All\ positional\ parameters\ (unquoted\ spaces) ${@}
imenu ShellMemu.SpShVars.Flags\ set ${-}
imenu ShellMemu.SpShVars.Return\ code\ of\ last\ command ${?}
imenu ShellMemu.SpShVars.Process\ number\ of\ this\ shell ${$}
imenu ShellMemu.SpShVars.Process\ number\ of\ last\ background\ command ${!}
imenu ShellMemu.Environ.HOME ${HOME}
imenu ShellMemu.Environ.PATH ${PATH}
imenu ShellMemu.Environ.CDPATH ${CDPATH}
imenu ShellMemu.Environ.MAIL ${MAIL}
imenu ShellMemu.Environ.MAILCHECK ${MAILCHECK}
imenu ShellMemu.Environ.PS1 ${PS1}
imenu ShellMemu.Environ.PS2 ${PS2}
imenu ShellMemu.Environ.IFS ${IFS}
imenu ShellMemu.Environ.SHACCT ${SHACCT}
imenu ShellMemu.Environ.SHELL ${SHELL}
imenu ShellMemu.Environ.LC_CTYPE ${LC_CTYPE}
imenu ShellMemu.Environ.LC_MESSAGES ${LC_MESSAGES}
imenu ShellMemu.Builtins.cd cd
imenu ShellMemu.Builtins.echo echo
imenu ShellMemu.Builtins.eval eval
imenu ShellMemu.Builtins.exec exec
imenu ShellMemu.Builtins.export export
imenu ShellMemu.Builtins.getopts getopts
imenu ShellMemu.Builtins.hash hash
imenu ShellMemu.Builtins.newgrp newgrp
imenu ShellMemu.Builtins.pwd pwd
imenu ShellMemu.Builtins.read read
imenu ShellMemu.Builtins.readonly readonly
imenu ShellMemu.Builtins.return return
imenu ShellMemu.Builtins.times times
imenu ShellMemu.Builtins.type type
imenu ShellMemu.Builtins.umask umask
imenu ShellMemu.Builtins.wait wait
imenu ShellMemu.Set.set set
imenu ShellMemu.Set.unset unset
imenu ShellMemu.Set.Mark\ created\ or\ modified\ variables\ for\ export set -a
imenu ShellMemu.Set.Exit\ when\ command\ returns\ non-zero\ status set -e
imenu ShellMemu.Set.Disable\ file\ name\ expansion set -f
imenu ShellMemu.Set.Locate\ and\ remember\ function\ commands\ when\ being\ looked\ up set -h
imenu ShellMemu.Set.All\ keyword\ arguments\ are\ placed\ in\ the\ environment\ for\ a\ command set -k
imenu ShellMemu.Set.Read\ commands\ but\ do\ not\ execute\ them set -n
imenu ShellMemu.Set.Exit\ after\ reading\ and\ executing\ one\ command set -t
imenu ShellMemu.Set.Treat\ unset\ variables\ as\ an\ error\ when\ performing\ parameter\  expansion set -u
imenu ShellMemu.Set.Print\ shell\ input\ lines\ as\ they\ are\ read set -v
imenu ShellMemu.Set.Print\ commands\ and\ their\ arguments\ as\ they\ are\ executed set -x

" Restore the previous value of 'cpoptions'.
let &cpo = s:cpo_save
unlet s:cpo_save
