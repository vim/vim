" Vim script to cleanup a .po file:
" - Remove line numbers (avoids that diffs are messy).
" - Comment-out fuzzy and empty messages.
" - Make sure there is a space before the string (required for Solaris).
" Requires Vim 6.0 or later (because of multi-line search patterns).
g/^#: /d
g/^#, fuzzy\(, .*\)\=\nmsgid ""\@!/.+1,/^$/-1s/^/#\~ /
g/^msgstr"/s//msgstr "/
g/^msgid"/s//msgid "/
g/^msgstr ""\(\n"\)\@!/?^msgid?,.s/^/#\~ /
