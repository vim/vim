vim9script
# Vim9 function calls
# VIM_TEST_SETUP hi link vimUserFunc Todo
# VIM_TEST_SETUP hi link vim9UserFunc Todo


call abs(42)
abs(42)


# Command/function distinction

# append browse call chdir confirm copy delete eval execute filter function
# insert join map match mode sort split substitute swapname type

var append = append(42)
call append(42)
# function
append(42)
# Legacy command
# append (42)

var browse = browse(42)
call browse(42)
# function
browse(42)
# command
browse (42)

var call = call(42)
call call(42)
# function
call(42)
# command
call (42)

var chdir = chdir(42)
call chdir(42)
# function
chdir(42)
# command
chdir (42)

var confirm = confirm(42)
call confirm(42)
# function
confirm(42)
# command
confirm (42)

var copy = copy(42)
call copy(42)
# function
copy(42)
# command
copy (42)

var delete = delete(42)
call delete(42)
# function
delete(42)
# command
delete (42)

var eval = eval(42)
call eval(42)
# function
eval(42)
# command
eval (42)

var execute = execute(42)
call execute(42)
# function
execute(42)
# command
execute (42)

var filter = filter(42)
call filter(42)
# function
filter(42)
# command
filter (42)

var function = function(42)
call function(42)
# function
function(42)
# command
function (42)

var insert = insert(42)
call insert(42)
# function
insert(42)
# Legacy command
# insert (42)

var join = join(42)
call join(42)
# function
join(42)
# command
join (42)

var map = map(42)
call map(42)
# function
map(42)
# command
map (42)

var match = match(42)
call match(42)
# function
match(42)
# command
match (42)

var sort = sort(42)
call sort(42)
# function
sort(42)
# command
sort (42)

var split = split(42)
call split(42)
# function
split(42)
# command
split (42)

var substitute = substitute(42)
call substitute(42)
# function
substitute(42)
# command
substitute (42)

var swapname = swapname(42)
call swapname(42)
# function
swapname(42)
# command
swapname (42)

var type = type(42)
call type(42)
# function
type(42)
# command
type (42)

var uniq = uniq(42)
call uniq(42)
# function
uniq(42)
# command
uniq (42)


# Errors

var foo = foo(42)
call foo(42)
foo(42)

var if = if(42)
call if(42)
# function
if(42) | .. | endif
# command
if (42) | .. | endif

var echo = echo(42)
call echo(42)
# function
echo(42)
# command
echo (42)


# Expressions

var foo = abs(42)

abs(42)
call abs(42)
echo "Foo" | abs(42)

echo abs(42)
echo (abs(42))
echo abs(42) + foo
echo foo + abs(42)

Foo()
call Foo()
echo "Foo" | Foo()

echo Foo()
echo (Foo())
echo Foo() + bar
echo bar + Foo()


# Scope modifiers and qualified names

var foo = g:foo(42)
call g:foo(42)

var foo = b:foo(42)
call b:foo(42)

var foo = w:foo(42)
call w:foo(42)

var foo = t:foo(42)
call t:foo(42)

var foo = v:foo(42)
call v:foo(42)


var foo = module.foo(42)
call module.foo(42)
module.foo(42)

var foo = g:module.foo(42)
call g:module.foo(42)
g:module.foo(42)

var foo = b:module.foo(42)
call b:module.foo(42)
b:module.foo(42)

var foo = w:module.foo(42)
call w:module.foo(42)
w:module.foo(42)

var foo = t:module.foo(42)
call t:module.foo(42)
t:module.foo(42)

var foo = v:module.foo(42)
call v:module.foo(42)
v:module.foo(42)


var foo = module#foo(42)
call module#foo(42)
module#foo(42)

var foo = g:module#foo(42)
call g:module#foo(42)
g:module#foo(42)


# User, not builtin, functions

call g:substitute()
call b:substitute()
call w:substitute()
call t:substitute()
call v:substitute()

call <SID>substitute()

call g:substitute.substitute()
call b:substitute.substitute()
call w:substitute.substitute()
call t:substitute.substitute()
call v:substitute.substitute()

call substitute#substitute()
call g:substitute#substitute()

g:substitute()
b:substitute()
w:substitute()
t:substitute()
v:substitute()

<SID>substitute()

g:substitute.substitute()
b:substitute.substitute()
w:substitute.substitute()
t:substitute.substitute()
v:substitute.substitute()

substitute#substitute()
g:substitute#substitute()


# Chained function calls

module.foo().bar()
module.foo().substitute()


# Issue 16721 (Vim script highlight of builtin function after |)

&directory = $'{$MYVIMDIR}/.data/swap/'
&backupdir = $'{$MYVIMDIR}/.data/backup//'
&undodir = $'{$MYVIMDIR}/.data/undo//'
if !isdirectory(&undodir)   | mkdir(&undodir, "p")   | endif
if !isdirectory(&backupdir) | mkdir(&backupdir, "p") | endif
if !isdirectory(&directory) | mkdir(&directory, "p") | endif


# Issue #17766 (valid function call highlighted as error)

module[0].foo()
module[0].substitute()

