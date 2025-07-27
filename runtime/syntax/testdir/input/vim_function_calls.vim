" Vim function calls
" VIM_TEST_SETUP hi link vimUserFunc Todo


call abs(42)


" Command/function distinction

" append browse call chdir confirm copy delete eval execute filter function
" insert join map match mode sort split substitute swapname type

let append = append(42)
call append(42)
" command
append
.
" bad command
append(42)
append (42)

let browse = browse(42)
call browse(42)
" command
browse(42)
browse (42)

let call = call(42)
call call(42)
" command
call(42)
call (42)

let chdir = chdir(42)
call chdir(42)
" command
chdir(42)
chdir (42)

let confirm = confirm(42)
call confirm(42)
" command
confirm(42)
confirm (42)

let copy = copy(42)
call copy(42)
" command
copy(42)
copy (42)

let delete = delete(42)
call delete(42)
" command
delete(42)
delete (42)

let eval = eval(42)
call eval(42)
" command
eval(42)
eval (42)

let execute = execute(42)
call execute(42)
" command
execute(42)
execute (42)

let filter = filter(42)
call filter(42)
" command
filter(42)
filter (42)

let function = function(42)
call function(42)
" command
function(42)
function (42)

let insert = insert(42)
call insert(42)
" command
insert
.
" bad command
insert(42)
insert (42)

let join = join(42)
call join(42)
" command
join(42)
join (42)

let map = map(42)
call map(42)
" command
map(42)
map (42)

let match = match(42)
call match(42)
" command
match(42)
match (42)

let sort = sort(42)
call sort(42)
" command
sort(42)
sort (42)

let split = split(42)
call split(42)
" command
split(42)
split (42)

let substitute = substitute(42)
call substitute(42)
" command
substitute(42)
substitute (42)

let swapname = swapname(42)
call swapname(42)
" command
swapname(42)
swapname (42)

let type = type(42)
call type(42)
" Vim9 command
" type(42)
" type (42)

let uniq = uniq(42)
call uniq(42)
" command
uniq(42)
uniq (42)


" Errors

let foo = foo(42)
call foo(42)

let if = if(42)
call if(42)
" command
if(42) | .. | endif
if (42) | .. | endif

let echo = echo(42)
call echo(42)
" command
echo(42)
echo (42)


" Expressions

let foo = abs(42)

echo abs(42)
echo (abs(42))
echo abs(42) + foo
echo foo + abs(42)

echo Foo()
echo (Foo())
echo Foo() + bar
echo bar + Foo()


" Scope modifiers and qualified names

let foo = s:foo(42)
call s:foo(42)

let foo = g:foo(42)
call g:foo(42)

let foo = b:foo(42)
call b:foo(42)

let foo = w:foo(42)
call w:foo(42)

let foo = t:foo(42)
call t:foo(42)

let foo = l:foo(42)
call l:foo(42)

let foo = a:foo(42)
call a:foo(42)

let foo = v:foo(42)
call v:foo(42)


let foo = module.foo(42)
call module.foo(42)

let foo = s:module.foo(42)
call module.foo(42)

let foo = g:module.foo(42)
call g:module.foo(42)

let foo = b:module.foo(42)
call b:module.foo(42)

let foo = w:module.foo(42)
call w:module.foo(42)

let foo = t:module.foo(42)
call t:module.foo(42)

let foo = l:module.foo(42)
call l:module.foo(42)

let foo = a:module.foo(42)
call a:module.foo(42)

let foo = v:module.foo(42)
call v:module.foo(42)


let foo = module#foo(42)
call module#foo(42)

let foo = g:module#foo(42)
call g:module#foo(42)


" Not builtin functions

call s:substitute()
call g:substitute()
call b:substitute()
call w:substitute()
call t:substitute()
call l:substitute()
call a:substitute()
call v:substitute()

call <SID>substitute()

call s:substitute.substitute()
call g:substitute.substitute()
call b:substitute.substitute()
call w:substitute.substitute()
call t:substitute.substitute()
call l:substitute.substitute()
call a:substitute.substitute()
call v:substitute.substitute()

call substitute#substitute()
call g:substitute#substitute()


" Chained function calls

call module.foo().bar()
call module.foo().substitute()


" Issue #17766 (valid function call highlighted as error)

call module[0].foo()
call module[0].substitute()

