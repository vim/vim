#!/bin/sh
# Create a per-test workdir with symlinks to shared resources.
#
# The layout mirrors the real directory tree so that relative paths work:
#   workdir/$test/         - mimics src/     (has src/* symlinks)
#   workdir/$test/testdir/ - mimics testdir/ (has testdir/* symlinks)
#
# Tests cd into workdir/$test/testdir/ so '../' = src/, '../../' = vim root.
#
# Level-1 symlinks (workdir/runtime, etc.) are created once by the nolog
# Makefile target; this script creates only levels 2 and 3 per test.
#
# Usage: setup_workdir.sh <testname>

testname=$1
if [ -z "$testname" ]; then
    echo "Usage: setup_workdir.sh <testname>" >&2
    exit 1
fi

# Compute the absolute path to testdir from this script's location.
TESTDIR=$(cd "$(dirname "$0")" && pwd)

mkdir -p workdir/"$testname"/testdir

# Level 2: src/ contents into workdir/$test/ for ../ references.
for f in "$TESTDIR"/../*; do
    case "$(basename "$f")" in testdir) continue;; esac
    rm -f workdir/"$testname"/"$(basename "$f")" 2>/dev/null
    ln -s "$f" workdir/"$testname"/"$(basename "$f")" 2>/dev/null || true
done

# Level 3: testdir/ contents into workdir/$test/testdir/.
for f in "$TESTDIR"/*; do
    case "$(basename "$f")" in workdir) continue;; esac
    rm -f workdir/"$testname"/testdir/"$(basename "$f")" 2>/dev/null
    ln -s "$f" workdir/"$testname"/testdir/"$(basename "$f")" 2>/dev/null || true
done
