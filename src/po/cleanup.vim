" Vim script to cleanup a .po file: comment-out fuzzy and empty messages.
" Make sure there is a space before the string (required for Solaris).
" Requires Vim 6.0 (because of multi-line search patterns).
g/^#, fuzzy\(, .*\)\=\nmsgid ""\@!/.+1,/^$/-1s/^/#\~ /
g/^msgstr"/s//msgstr "/
g/^msgid"/s//msgid "/
g/^msgstr ""\(\n"\)\@!/?^msgid?,.s/^/#\~ /
