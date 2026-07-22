# vim: ft=make
SHELL = /bin/bash

# Default target to actually run the comparison:
.PHONY: check
.INTERMEDIATE: hlgroups deflinks hlgroups.stripped

check: hlgroups.stripped deflinks
	diff hlgroups.stripped deflinks

hlgroups:
	grep '\*hl-' ../runtime/doc/*txt | sed -E -e 's/.*:<?\s*//' -e 's/hl-//g' -e 's/\*//g' -e 's/ /\n/g' | sort > hlgroups

deflinks: ../src/highlight.c
	grep '"default link'  $< | sed 's/.*default link\s*\(.*\)\s.*/\1/' | sort > deflinks

hlgroups.stripped: hlgroups.ignore hlgroups
	grep -v -x -F -f hlgroups.ignore hlgroups > hlgroups.stripped
