" Vim :abbreviate commands

abbrev <buffer> foo foobar
cabbrev <buffer> cfoo cfoobar
iabbrev <buffer> ifoo cfoobar

abbrev <expr> <buffer> foo foobar
cabbrev <expr> <buffer> cfoo cfoobar
iabbrev <expr> <buffer> ifoo cfoobar

noreabbrev <buffer> foo foobar
cnoreabbrev <buffer> cfoo cfoobar
inoreabbrev <buffer> ifoo cfoobar

abbrev <expr> <buffer> foo foobar
cabbrev <expr> <buffer> cfoo cfoobar
iabbrev <expr> <buffer> ifoo cfoobar

unabbrev <buffer> foo
cunabbrev <buffer> cfoo
iunabbrev <buffer> ifoo

abclear <buffer>
cabclear <buffer>
iabclear <buffer>
