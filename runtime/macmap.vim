" System gvimrc file for Mac OS X
" Author:  	Benji Fisher <benji@member.AMS.org>
" Last Change: Thu Mar 09 09:00 AM 2006 EST
"
" Define Mac-standard keyboard shortcuts.

" Save and restore compatible mode.
let s:save_cpo = &cpo
set cpo&vim

nnoremap <D-n> :confirm enew<CR>
vmap <D-n> <Esc><D-n>gv
imap <D-n> <C-O><D-n>
cmap <D-n> <C-C><D-n>
omap <D-n> <Esc><D-n>

nnoremap <D-o> :browse confirm e<CR>
vmap <D-o> <Esc><D-o>gv
imap <D-o> <C-O><D-o>
cmap <D-o> <C-C><D-o>
omap <D-o> <Esc><D-o>

nnoremap <silent> <D-w> :if winheight(2) < 0 <Bar>
	\   confirm enew <Bar>
	\ else <Bar>
	\   confirm close <Bar>
	\ endif<CR>
vmap <D-w> <Esc><D-w>gv
imap <D-w> <C-O><D-w>
cmap <D-w> <C-C><D-w>
omap <D-w> <Esc><D-w>

nnoremap <silent> <D-s> :if expand("%") == ""<Bar>browse confirm w<Bar>
	\ else<Bar>confirm w<Bar>endif<CR>
vmap <D-s> <Esc><D-s>gv
imap <D-s> <C-O><D-s>
cmap <D-s> <C-C><D-s>
omap <D-s> <Esc><D-s>

nnoremap <D-S-s> :browse confirm saveas<CR>
vmap <D-S-s> <Esc><D-s>gv
imap <D-S-s> <C-O><D-s>
cmap <D-S-s> <C-C><D-s>
omap <D-S-s> <Esc><D-s>

" From the Edit menu of SimpleText:
nnoremap <D-z> u
vmap <D-z> <Esc><D-z>gv
imap <D-z> <C-O><D-z>
cmap <D-z> <C-C><D-z>
omap <D-z> <Esc><D-z>

vnoremap <D-x> "+x

vnoremap <D-c> "+y

cnoremap <D-c> <C-Y>

nnoremap <D-v> "+gP
cnoremap <D-v> <C-R>+
execute 'vnoremap <script> <D-v>' paste#paste_cmd['v']
execute 'inoremap <script> <D-v>' paste#paste_cmd['i']

nnoremap <silent> <D-a> :if &slm != ""<Bar>exe ":norm gggH<C-O>G"<Bar>
	\ else<Bar>exe ":norm ggVG"<Bar>endif<CR>
vmap <D-a> <Esc><D-a>
imap <D-a> <Esc><D-a>
cmap <D-a> <C-C><D-a>
omap <D-a> <Esc><D-a>

nnoremap <D-f> /
vmap <D-f> <Esc><D-f>
imap <D-f> <Esc><D-f>
cmap <D-f> <C-C><D-f>
omap <D-f> <Esc><D-f>

nnoremap <D-g> n
vmap <D-g> <Esc><D-g>
imap <D-g> <C-O><D-g>
cmap <D-g> <C-C><D-g>
omap <D-g> <Esc><D-g>

let &cpo = s:save_cpo
