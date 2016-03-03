#!/usr/bin/python
#
# Server that will communicate over stdin/stderr
#
# This requires Python 2.6 or later.

from __future__ import print_function
import sys

if __name__ == "__main__":

    if len(sys.argv) > 1:
        print(sys.argv[1])

    while True:
        typed = sys.stdin.readline()
        if typed.startswith("quit"):
            print("Goodbye!")
            sys.stdout.flush()
            break
        if typed.startswith("echo "):
            print(typed[5:-1])
            sys.stdout.flush()
        if typed.startswith("echoerr"):
            print(typed[8:-1], file=sys.stderr)
            sys.stderr.flush()
        if typed.startswith("double"):
            print(typed[7:-1] + "\nAND " + typed[7:-1])
            sys.stdout.flush()

