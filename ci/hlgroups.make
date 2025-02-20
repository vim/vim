# vim: ft=make
SHELL = /bin/bash

# Default target to actually run the comparison:
.PHONY: check
.INTERMEDIATE: hlgroups deflinks hlgroups.stripped

hlgroups:
	grep '\*hl-' ../runtime/doc/*txt | sed -E 's/.*:<?\s*//' | sed 's/ /\n/g' | sed 's/hl-//' | sed 's/\*//g' | sort > hlgroups

deflinks: ../src/highlight.c
	grep '"default link'  $< | sed 's/.*default link\s*\(.*\)\s.*/\1/' | sort > deflinks

hlgroups.stripped: hlgroups.ignore hlgroups
	grep -v -x -F -f hlgroups.ignore hlgroups > hlgroups.stripped

check: hlgroups.stripped deflinks
	diff hlgroups.stripped deflinks

