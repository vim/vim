#!/bin/sh -e
#
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
	printf >&2 "Usage: $0 [1 | 2 | 3 | ...]\n"
	exit 0
	;;
esac

tries="${1:-1}"

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
cwd=$(pwd)
trap 'cd "$cwd"'		EXIT HUP INT QUIT TERM
cd "$(dirname "$0")/../../../syntax" || exit 85
set +f
rm testdir/dumps/*.dump || exit 86
spuriosities=''

for f in testdir/input/*.*
do
	test ! -d "$f" || continue
	b=$(basename "$f")
	i=0
	printf "$templet\n\n" "$b"

	while test "$i" -le "$tries"
	do
		make clean "$b" test || :

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
	make clean self-testing test || :

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
