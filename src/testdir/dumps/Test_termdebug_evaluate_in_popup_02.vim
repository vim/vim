" Only keep screen lines relevant to the actual popup and evaluation.
" Especially the top lines are too instable and cause flakiness between
" different systems and tool versions.
normal! G
normal! 8k
normal! dgg
