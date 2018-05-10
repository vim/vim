#!/usr/bin/python3
#
# Program that writes a number to stdout repeatedly

from __future__ import print_function
import sys
import time

if __name__ == "__main__":

    done = 0
    while done < 10:
        done = done + 1
        print(done)
        sys.stdout.flush()
        time.sleep(0.05)  # sleep 50 msec
