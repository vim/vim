" Vim :map commands

map!
map! lhs rhs
map 
map lhs rhs

map <buffer><expr><nowait><script><silent><special><unique> lhs rhs
map <buffer> <expr> <nowait> <script> <silent> <special> <unique> lhs rhs

" :map special arguments are case sensitive, <BUFFER> is the LHS
map <BUFFER> rhs

map <Leader>lhs      rhs
map <LocalLeader>lhs rhs

map lhs <Leader>rhs
map lhs <LocalLeader>rhs

map lhs <Plug>rhs

mapclear  <buffer>
mapclear! <buffer>
nmapclear <buffer>
vmapclear <buffer>
xmapclear <buffer>
smapclear <buffer>
omapclear <buffer>
imapclear <buffer>
lmapclear <buffer>
cmapclear <buffer>
tmapclear <buffer>


" :help map-bar

" <Bar>     '<' is not in 'cpoptions'
map _l :!ls <Bar> more^M:echo "rhs"<CR>
" \|        'b' is not in 'cpoptions'
map _l :!ls \| more^M:echo "rhs"<CR>
" ^V|       always, in Vim and Vi
map _l :!ls | more^M:echo "rhs"<CR>

map lhs :search('foo\\|bar')<CR>:echo "rhs"<CR>


" multiline RHS

map <Leader>baz 
  \ :echo (<Bar>
  \
  \'bar')<CR>
  "\ comment

map lhs 
  "\ comment
  \ echo "foo"

map lhs
  "\ comment
  \ echo "foo"

map lhs 
  "\ comment
  \ echo "foo"

map l hs
  "\ comment
  \ echo "foo"

map l hs 
  "\ comment
  \ echo "foo"

map lhs rhs
map l h s  rhs

map lhs
  "\ comment (matches as RHS but harmless)
echo "clear"


" Differentiate map() from :map

map ( :echo "open-paren"<CR>

call map(list, 'v:val')
call map (list, 'v:val')

function Foo()
  map ( :echo "open-paren"<CR>
  call map(list, 'v:val')
  call map (list, 'v:val')
endfunction

def Foo()
  map ( :echo "open-paren"<CR>
  map(list, 'v:val')
  # :map LHS=(list, RHS='v:val')
  map (list, 'v:val')
enddef


" Issue  #12672

nnoremap <Leader>foo :echo call(
  "\ comment
  \ {x->x},
  \ ['foo'])<CR>

nnoremap <Leader>bar :echo (
  \
  \ 'bar')<CR>


" Example:
"   /autoload/netrw.vim

if !hasmapto('<Plug>NetrwOpenFile')          |nmap <buffer> <silent> <nowait> %	<Plug>NetrwOpenFile|endif
