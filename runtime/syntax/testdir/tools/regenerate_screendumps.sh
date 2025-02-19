#!/bin/sh -e
#
# The following steps are to be taken by this script:
# 1) Remove all files from the "dumps" directory.
# 2) Generate screendumps for each syntax test and each self-test.
# 3) Unconditionally move each batch of screendumps to "dumps"; if generated
#	files differ on repeated runs, always remove these files from "dumps".
# 4) Repeat steps 2) and 3) once or as many times as requested with the "$1"
#	argument.
# 5) Summarise any differences.
#
# Provided that "git difftool" is set up (see src/testdir/commondumps.vim),
# run "git difftool HEAD -- '**/*.dump'" to collate tracked and generated
# screendumps.

case "$1" in
-h | --help)
	printf >&2 "Usage: [time VIM_SYNTAX_TEST_LOG=/tmp/log] $0 [1 | 2 | ...]\n"
	exit 0
	;;
esac

tries="${1:-1}"
shift $#

case "$tries" in
0* | *[!0-9]*)
	exit 80
	;;
esac

test -x "$(command -v make)"	|| exit 81
test -x "$(command -v git)"	|| exit 82

case "$(git status --porcelain=v1)" in
'')	;;
*)	printf >&2 'Resolve ALL changes before proceeding.\n'
	exit 83
	;;
esac

templet=$(printf "\t\t\t\t$(tput rev)%%s$(tput sgr0)") || exit 84
cd "$(dirname "$0")/../../../syntax" || exit 85
set +f
rm testdir/dumps/*.dump || exit 86
spuriosities=''

# Because the clean target of Make will be executed before each syntax test,
# this environment variable needs to be pointed to an existing file that is
# created in a directory not affectable by the target.
if test -w "$VIM_SYNTAX_TEST_LOG"
then
	log=-e VIM_SYNTAX_TEST_LOG="$VIM_SYNTAX_TEST_LOG"
else
	log=
fi

for f in testdir/input/*.*
do
	test ! -d "$f" || continue
	b=$(basename "$f")
	i=0
	printf "$templet\n\n" "$b"

	while test "$i" -le "$tries"
	do
		make $log clean "$b" test || :

		case "$i" in
		0)	mv testdir/failed/*.dump testdir/dumps/
			;;
		*)	case "$(printf '%s' testdir/failed/*.dump)" in
			testdir/failed/\*.dump)
				# (Repeatable) success.
				;;
			*)	spuriosities="${spuriosities}${b} "
				p=${b%.*}
				rm -f testdir/dumps/"$p"_[0-9][0-9].dump \
					testdir/dumps/"$p"_[0-9][0-9][0-9].dump \
					testdir/dumps/"$p"_[0-9][0-9][0-9][0-9].dump
				;;
			esac
			;;
		esac

		i=$(($i + 1))
		sleep 1
	done
done

# For a 20-file set, initially fail for a series of: 1-6, 7-12, 13-18, 19-20.
tries=$(($tries + 3))
i=0

while test "$i" -le "$tries"
do
	make $log clean self-testing test || :

	case "$i" in
	[0-3])	mv testdir/failed/dots_*.dump testdir/dumps/
		;;
	*)	case "$(printf '%s' testdir/failed/*.dump)" in
		testdir/failed/\*.dump)
			# (Repeatable) success.
			;;
		*)	spuriosities="${spuriosities}dots_xy "
			rm -f testdir/dumps/dots_*.dump
			;;
		esac
		;;
	esac

	sleep 1
	i=$(($i + 1))
done

make clean
git diff --compact-summary

if test -n "$spuriosities"
then
	printf '\n%s\n' "$spuriosities"
	exit 87
fi

# vim:sw=8:ts=8:noet:nosta:
