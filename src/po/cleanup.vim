" Vim script to cleanup a .po file:
" - Remove line numbers (avoids that diffs are messy).
" - Comment-out fuzzy and empty messages.
" - Make sure there is a space before the string (required for Solaris).
" Requires Vim 6.0 or later (because of multi-line search patterns).
diffoff!
silent g/^#: /d
silent g/^#, fuzzy\(, .*\)\=\nmsgid ""\@!/.+1,/^$/-1s/^/#\~ /
silent g/^msgstr"/s//msgstr "/
silent g/^msgid"/s//msgid "/
silent g/^msgstr ""\(\n"\)\@!/?^msgid?,.s/^/#\~ /
