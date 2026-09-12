#!/usr/bin/env python
#
# Start Vim with no console attached at all and pipes on every standard
# handle.  Used by test_startup.vim.
#
# Vim script cannot start a process this way: DETACHED_PROCESS is a
# CreateProcess() flag that job_start() does not offer.  Without a console
# there is no console device to fall back to, so mch_init_c() has to leave
# the pipes alone and Vim has to report that it has no terminal.
#
# Vim is given "--ttyfail", so it exits(1) from check_tty() before sourcing
# anything.  The script it is handed writes a file, which must therefore not
# appear.
#
# One line of "name=value" pairs is printed for the Vim test to check.
#
# This requires Python 3.

import os
import subprocess
import sys

DETACHED_PROCESS = 0x00000008
PROBE = 'Xnoconsole.vim'
RESULT = 'Xnoconsole_result'


def main():
    if len(sys.argv) < 2:
        sys.stderr.write('usage: test_mswin_console.py <vim-executable>\n')
        return 2

    with open(PROBE, 'w') as f:
        f.write('call writefile(["reached"], "' + RESULT + '")\n')
        f.write('qall!\n')
    if os.path.exists(RESULT):
        os.remove(RESULT)

    proc = subprocess.Popen(
        [sys.argv[1], '-u', 'NONE', '-i', 'NONE', '--ttyfail', '-S', PROBE],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        creationflags=DETACHED_PROCESS)
    try:
        err = proc.communicate(timeout=30)[1]
        code = proc.returncode
    except subprocess.TimeoutExpired:
        proc.kill()
        err = proc.communicate()[1]
        code = -1
    err = err.decode('utf-8', 'replace')

    print('exit=%d out_warn=%d in_warn=%d reached=%d' % (
        code,
        'Output is not to a terminal' in err,
        'Input is not from a terminal' in err,
        os.path.exists(RESULT)))

    for name in (PROBE, RESULT):
        if os.path.exists(name):
            os.remove(name)
    return 0


if __name__ == '__main__':
    sys.exit(main())
