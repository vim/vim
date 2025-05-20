#!/bin/ksh

# Rendering namespace variables
echo ${.foo.bar[adsf]} ${foo.bar[1][2]} ${foo.bar[1][az]} ${.foo.bar[1][2]}
echo ${.foo[var]} ${.foo.bar[1]} ${.foo.bar[*]} ${foo.bar##baz} ${.foo.bar##baz}
echo ${.foo.bar[3]##baz} ${.foo.bar[z]##baz} ${sh.version/V/b} ${.sh.version/V/b}
echo ${foo/%bar/foo} ${foo/#bar/foo} ${foo.bar/%bar/foo} ${foo.bar[d]/#bar/foo}
echo ${.foo/%barfoo} ${.foo.bar/#bar/foo} ${.bar.foo/%bar/foo} ${.bar/#bar/foo}
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

# ;& and ;;& in case statements
case x in
	bar) false ;&
	foo) true ;;&
	*) print ${$bar} ;;	# 93v-
esac

# ksh88 and ksh93 non-dot special variables
print ${ RANDOM= SRANDOM= SHLVL= JOBMAX= KSH_VERSION= FIGNORE= LC_TIME= LC_NUMERIC= LC_MESSAGES= LC_CTYPE= LC_COLLATE= LC_ALL= LANG= FPATH= PS4= OPTIND= OPTARG= true ;}
print $(LINENO= SECONDS= TMOUT= PPID= LINES= COLUMNS= VISUAL= OLDPWD= PS3= MAILPATH= CDPATH= FCEDIT= HISTCMD= HISTEDIT= HISTSIZE= HISTFILE= ENV= MAILCHECK= EDITOR= SHELL= false)
print $(REPLY= MAIL= HOME= PWD= IFS= PS2= PS1= PATH= ERRNO= :)  # ERRNO is provided by ksh88 but not ksh93
