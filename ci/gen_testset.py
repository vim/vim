#!/usr/bin/env python3

import argparse
import itertools
import json
import re
import subprocess


def generate_testset(n):
    cp = subprocess.run(["make", "-C", "src/testdir", "-npq"], capture_output=True)

    tests = set()
    for line in cp.stdout.decode().split("\n"):
        if re.match(r"^(NEW_TESTS_RES|TEST_VIM9_RES) = ", line):
            tests.update(re.findall(r"\btest\w+\.res\b", line))

    tests = sorted(list(tests))
    # move test_alot*.res to the end
    tests = (
        [t for t in tests if not t.startswith("test_alot")]
        + [t for t in tests if t.startswith("test_alot_")]
        + ["test_alot.res"]
    )
    targets = tests

    if n > 1:
        targets = [ts for ts in itertools.batched(tests, n)]
        targets = [[t for t in ts if t] for ts in itertools.zip_longest(*targets)]

    return targets


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("n", type=int, nargs="?", default=1)
    args = parser.parse_args()

    print(json.dumps(generate_testset(args.n)))


if __name__ == "__main__":
    main()
