#!/bin/mksh

# Rendering namespace variables
echo ${.foo.bar[adsf]} ${foo.bar[1][2]} ${foo.bar[1][az]} ${.foo.bar[1][2]}
echo ${.foo[var]} ${.foo.bar[1]} ${.foo.bar[*]} ${foo.bar##baz} ${.foo.bar##baz}
echo ${.foo.bar[3]##baz} ${.foo.bar[z]##baz} ${sh.version/V/b} ${.sh.version/V/b}
echo ${foo/%bar/foo} ${foo/#bar/foo} ${foo.bar/%bar/foo} ${foo.bar[d]/#bar/foo}
echo ${.foo/%barfoo} ${.foo.bar/#bar/foo} ${.bar.foo/%bar/foo} ${.bar/#bar/foo}
echo ${foo/%barfoo} ${foo/bar/foo} ${barfoo//bar/foo} ${bar/#bar/foo}
echo ${.sh.version^^} ${.sh.version,,} ${KSH_VERSION^} ${KSH_VERSION,}

# 'alarm' builtin (present in ksh93u+, ksh93v- and the 93u+m dev branch).
alarm --man
# The fds and pids builtins. These ksh93 builtins have existed since 2005-05-22
# and 2008-06-02, respectively. However, these were not readily enabled; in
# 93u+m these can be enabled with the builtin command if libcmd.so is present,
# either via 'builtin -f' or (in more recent commits) with a regular invocation
# of the 'builtin' built-in.
# cf. https://github.com/ksh93/ksh/commit/f15e2c41
builtin fds pids
fds; pids

# Unix commands which are provided by ksh as builtins via libcmd.so
basename
cat
chgrp
chmod
chown
cksum
cmp
comm
cp
cut
date
dirname
egrep		# Obsolescent
expr
fgrep		# Obsolescent
fmt
fold
getconf
grep
head
iconv		# 93v-
id
join
ln
logname
ls		# 93v-
md5sum
mkdir
mkfifo
mktemp
mv
od		# 93v-
paste
pathchk
readlink	# 93v-
realpath	# 93v-
rev
rm
rmdir
sha1sum		# 93v-
sha256sum	# 93v-
sha2sum		# 93v-
sha384sum	# 93v-
sha512sum	# 93v-
stty
sum
sync
tail
tee
tr		# 93v-
tty
uname
uniq
vmstate		# Obsolescent; only available in 93v- and older
wc
xargs		# 93v-
xgrep		# 93v-

# SHA command not provided as a builtin but included here for completeness
sha224sum

# poll builtin (93v-)
poll --man

# mkservice and eloop (rarely provided; requires SHOPT_MKSERVICE)
mkservice --man; eloop --help

# some mksh builtins
bind; rename

# ;& and ;;& in case statements
case x in
	bar) false ${baz:1} ;&
	foo) true ${foo:0:0} ;;&
	*) print ${$bar} ;;	# 93v-
esac

# Below is subshare syntax supported by both ksh93 and mksh.
print ${ echo one }
print ${	echo two
}
print ${
echo three	}
print ${ echo 'four'; }
print ${ echo 'five' ;}
print ${ echo 'six'
}
print ${	echo 'seven'	}
echo ${ print 'eight'	}
typeset nine=${ pwd; }

# Value substitutions of the form ${|command} are only
# supported by mksh, not ksh93.
if ! command eval '((.sh.version >= 20070703))' 2>/dev/null; then
	valsubfunc() {
		REPLY=$1
	}
	echo ${|valsubfunc ten}
	print "${|valsubfunc eleven;}"
	printf '%s' "${|valsubfunc twelve	}"
	unlucky=${|valsubfunc thirteen
}
	typeset notafloat=${|valsubfunc notanumber	}
	print $unlucky $notanumber
	${|echo foo}
	${|echo bar
}
fi

# ======
# Shared-state command substitutions using the syntax ${<file;}
# are only supported by ksh93, not mksh.
echo ${
	printf %s str
} > /tmp/strfile
echo ${</tmp/strfile;}

exit 0
# ksh88 and ksh93 non-dot special variables
print ${ RANDOM= SRANDOM= SHLVL= JOBMAX= KSH_VERSION= FIGNORE= LC_TIME= LC_NUMERIC= LC_MESSAGES= LC_CTYPE= LC_COLLATE= LC_ALL= LANG= FPATH= PS4= OPTIND= OPTARG= true ;}
print $(LINENO= SECONDS= TMOUT= PPID= LINES= COLUMNS= VISUAL= OLDPWD= PS3= MAILPATH= CDPATH= FCEDIT= HISTCMD= HISTEDIT= HISTSIZE= HISTFILE= ENV= MAILCHECK= EDITOR= SHELL= false)
print $(REPLY= MAIL= HOME= PWD= IFS= PS2= PS1= PATH= SH_OPTIONS= ERRNO= COMP_CWORD= COMP_LINE= COMP_POINT= COMP_WORDS= COMP_KEY= COMPREPLY= COMP_WORDBREAKS= COMP_TYPE= compgen)
print $(BASHPID= EPOCHREALTIME= EXECSHELL= KSHEGID= KSHGID= KSHUID= KSH_MATCH= PATHSEP= PGRP= PIPESTATUS= TMPDIR= USER_ID= VPATH= CSWIDTH= complete)
