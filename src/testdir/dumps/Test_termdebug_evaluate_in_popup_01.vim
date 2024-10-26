" replace hex addresses with |0|x|f@12|
:%s/|0|x|\(\(\w\|@\)\+|\)\+/|0|x|f@12|/g

" Only keep screen lines relevant to the actual popup and evaluation.
" Especially the top lines are too instable and cause flakiness between
" different systems and tool versions.
normal! G
normal! 8k
normal! dgg
