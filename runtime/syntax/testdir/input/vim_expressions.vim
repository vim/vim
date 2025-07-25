" Vim expressions


" String

echo 'It''s a string'
echo 'tab: \t, new line: \n, backslash: \\'
echo "tab: \t, new line: \n, backslash: \\"

" string starts immediately after line continuation character - tests a
" comment/string distinguishing implementation quirk
echo "foo"
      \"bar"
      \ "baz"
echo 'foo'
      \'bar'
      \ 'baz'

" String escape sequences

echo "\316 - \31 - \3 - \x1f - \xf - \X1F - \XF - \u02a4 - \U000002a4 - \b - \e - \f - \n - \r - \t - \\ - \" - \<C-W>"
echo '\316 \31 \3 \x1f \xf \X1F \XF \u02a4 \U000002a4 \b \e \f \n \r \t \\ \" \<C-W>'
echo "\3160 - \x1f0 - \X1F0 - \u02a40 - \U000002a40"

echo $"\316 - \31 - \3 - \x1f - \xf - \X1F - \XF - \u02a4 - \U000002a4 - \b - \e - \f - \n - \r - \t - \\ - \" - \<C-W>"
echo $'\316 \31 \3 \x1f \xf \X1F \XF \u02a4 \U000002a4 \b \e \f \n \r \t \\ \" \<C-W>'
echo $"\3160 - \x1f0 - \X1F0 - \u02a40 - \U000002a40"

echo "\<C-a>"
echo "\<*C-a>"
echo "\<C->>"
echo "\<*C->>"
echo "\<C->>>"
echo "\<*C->>>"

echo ""
echo "\""
echo "foo\""
echo "\"foo"
echo "foo\"bar"

echo ''
echo ''''
echo '''foo'
echo 'foo'''
echo 'foo''bar'

" Unreported issue (incorrectly matches as vimString vimMark vimOper NONE)
" https://github.com/tpope/vim-unimpaired/blob/6d44a6dc2ec34607c41ec78acf81657248580bf1/plugin/unimpaired.vim#L232
let cmd = 'put!=repeat(nr2char(10), v:count1)|silent '']+'


" String interpolation

echo 'Don''t highlight interpolation: {{ {1 + 2} }}'
echo "Don't highlight interpolation: {{ {1 + 2} }}"
echo $'Highlight interpolation:\t{{ { string({'foo': 'bar'}) } }}'
echo $'Highlight interpolation:\t{{ { $'nested: {{ {1 + 2} }}' } }}'
echo $"Highlight interpolation:\t{{ { string({"foo": "bar"}) } }}"
echo $"Highlight interpolation:\t{{ { $"nested: {{ {1 + 2} }}" } }}"


" Continued string

let s = "
      "\ comment
      \ part 1
      "\ comment
      \ part 2
      "\ comment
      \" " tail comment

let s = "\"
      \\" part 1 \"
      "\ escape sequence
      \ \"part 2\"
      \\"" " tail comment

let s = '
      "\ comment
      \ part 1
      "\ comment
      \ part 2
      "\ comment
      \' " tail comment

let s = '''
      \'' part 1 ''
      "\ escape sequence
      \ ''part 2''
      \''' " tail comment

let s = $"
      "\ comment
      \ part 1
      "\ comment
      \ part 2
      "\ comment
      \" " tail comment

let s = $'
      "\ comment
      \ part 1
      "\ comment
      \ part 2
      "\ comment
      \' " tail comment

call strlen("part 1
      "\ comment
      \ part 2")

call append(0, "part 1
      "\ comment
      \ part 2")


" Number

" Hexadecimal
echo  0xFF
echo  0XFF
echo -0xFF
echo -0XFF

" Decimal
echo  255
echo -255

" Octal
echo  0377
echo  0o377
echo  0O377
echo -0377
echo -0o377
echo -0O377

" Binary
echo  0b11111111
echo  0B11111111
echo -0b11111111
echo -0B11111111

" Float
echo 123.456
echo +0.0001
echo 55.0
echo -0.123
echo 1.234e03
echo 1.0E-6
echo -3.1416e+88

" Blob
echo 0z
echo 0zFF00ED015DAF
echo 0zFF00.ED01.5DAF
echo 0zFF.00.ED.01.5D.AF

" List

echo []
echo [42]
echo [[11, 12], [21, 22], [31, 32]]
echo [1,
      \ 2,
      \ 3,
      \ 4
      \]
echo [1, 'two', 1 + 2, "fo" .. "ur"]

" Issue #5830 (Incorrect syntax highlighting in Vim script when omitting space in list of string)
let l = ['a','b','c']

" Dictionary

echo {}
echo { 'foo': 21 * 2 }
echo { "foo": 21 * 2 }
echo {    42: 21 * 2 }

echo { "foo":  { 'bar': 21 * 2 } }
echo { "foo":  { "bar": 21 * 2 } }
echo { "foo":  {    42: 21 * 2 } }
echo { "foo": #{   bar: 21 * 2 } }
echo { "foo": #{ -bar-: 21 * 2 } }
echo { "foo": #{    42: 21 * 2 } }

echo { 'foo':  { 'bar': 21 * 2 } }
echo { 'foo':  { "bar": 21 * 2 } }
echo { 'foo':  {    42: 21 * 2 } }
echo { 'foo': #{   bar: 21 * 2 } }
echo { 'foo': #{ -bar-: 21 * 2 } }
echo { 'foo': #{    42: 21 * 2 } }

echo {    42:  { 'bar': 21 * 2 } }
echo {    42:  { "bar": 21 * 2 } }
echo {    42:  {    42: 21 * 2 } }
echo {    42: #{   bar: 21 * 2 } }
echo {    42: #{ -bar-: 21 * 2 } }
echo {    42: #{    42: 21 * 2 } }

echo {
      "\ comment
      \ "foo": { "bar": 21 * 2 }
      \}

" TODO: arbitrary expression keys

" Literal Dictionary

echo #{}
echo #{   foo: 21 * 2 }
echo #{ -foo-: 21 * 2 }
echo #{    42: 21 * 2 }

echo #{ foo: #{   bar: 21 * 2 } }
echo #{ foo: #{ -bar-: 21 * 2 } }
echo #{ foo: #{    42: 21 * 2 } }
echo #{ foo:  { "bar": 21 * 2 } }
echo #{ foo:  { 'bar': 21 * 2 } }
echo #{ foo:  {    42: 21 * 2 } }

echo #{ -foo-: #{   bar: 21 * 2 } }
echo #{ -foo-: #{ -bar-: 21 * 2 } }
echo #{ -foo-: #{    42: 21 * 2 } }
echo #{ -foo-:  { "bar": 21 * 2 } }
echo #{ -foo-:  { 'bar': 21 * 2 } }
echo #{ -foo-:  {    42: 21 * 2 } }

echo #{ 42: #{   bar: 21 * 2 } }
echo #{ 42: #{ -bar-: 21 * 2 } }
echo #{ 42: #{    42: 21 * 2 } }
echo #{ 42:  { "bar": 21 * 2 } }
echo #{ 42:  { 'bar': 21 * 2 } }
echo #{ 42:  {    42: 21 * 2 } }

echo #{
      "\ comment
      \  foo: #{
      \    bar: 21 * 2
      \  }
      \}

" match as keys not scope dictionaries
echo #{ b: 42, w: 42, t: 42, g: 42, l: 42, s: 42, a: 42, v: 42  }

" Tuple

echo ()
echo (42,)
echo ((11, 12), (21, 22), (31, 32))
echo (1,
      \ 2,
      \ 3,
      \ 4
      \)
echo (1, 'two', 1 + 2, "fo" .. "ur")

echo foo + (42, 87)
echo (42, 87) + foo

" Register

echo @" @@
echo @0 @1 @2 @3 @4 @5 @6 @7 @8 @9
echo @a @b @c @d @e @f @g @h @i @j @k @l @m @n @o @p @q @r @s @t @u @v @w @x @y @z
echo @A @B @C @D @E @F @G @H @I @J @K @L @M @N @O @P @Q @R @S @T @U @V @W @X @Y @Z
echo @- @: @. @% @# @= @* @+ @~ @_ @/

" read-only @:, @., @%, @~
let @" = "foo" 
let @0 = "foo"
let @1 = "foo"
let @9 = "foo"
let @a = "foo"
let @k = "foo"
let @z = "foo"
let @A = "foo"
let @K = "foo"
let @Z = "foo"
let @- = "foo"
let @# = "foo"
let @= = "foo"
let @* = "foo"
let @+ = "foo"
let @_ = "foo"
let @/ = "foo"

" Operators

" Ternary
echo expr ? expr : expr

echo lnum == 1 ? "top" : lnum
echo lnum == 1 ? "top" : lnum == 1000 ? "last" : lnum

echo lnum == 1
      \	? "top"
      \	: lnum == 1000
      \		? "last"
      \		: lnum
echo lnum == 1 ?
      \	"top" :
      \	lnum == 1000 ?
      \		"last" :
      \		lnum

echo 1 ? 1 : 0
echo "foo" ? "foo" : "bar"
echo foo ? foo : bar
echo g:foo ? g:foo : g:bar
echo $FOO ? $FOO : $BAR
echo True() ? True() : False()
echo @a ? @a : @b
echo (1) ? (1) : (0)

" Falsy
echo expr ?? expr

echo theList ?? 'list is empty'
echo GetName() ?? 'unknown'

echo theList
      \ ?? 'list is empty'
echo theList ??
      \ 'list is empty'

echo 1 ?? 1
echo "foo" ?? "foo"
echo foo ?? foo
echo g:foo ?? g:foo
echo $FOO ?? $FOO
echo True() ?? True()
echo @a ?? @a
echo (1) ?? (1)

" Comparison - using 'ignorcase'
echo expr ==     expr
echo expr !=     expr
echo expr >      expr
echo expr >=     expr
echo expr <      expr
echo expr <=     expr
echo expr =~     expr
echo expr !~     expr
echo expr is     expr
echo expr isnot  expr

" Comparison - match case
echo expr ==#    expr
echo expr !=#    expr
echo expr >#     expr
echo expr >=#    expr
echo expr <#     expr
echo expr <=#    expr
echo expr =~#    expr
echo expr !~#    expr
echo expr is#    expr
echo expr isnot# expr

" Comparison - ignore case
echo expr ==?    expr
echo expr !=?    expr
echo expr >?     expr
echo expr >=?    expr
echo expr <?     expr
echo expr <=?    expr
echo expr =~?    expr
echo expr !~?    expr
echo expr is?    expr
echo expr isnot? expr

" Unreported issue ("is" incorrectly matches as "echo vimNumber *vimCommand* vimNumber")
echo 42 is 42

" Line continuation
let foo = foo +
      \
      "\ comment
      \
      "\ comment
      \ bar +
      \ "baz"

let foo = foo +
      "\ comment
      \
      "\ comment
      \
      \ bar +
      \ "baz"

let foo = foo +
      "\ "comment string"
      \ bar

" Function calls

call Foo(v:true, v:false, v:null)


" Issue #16221 (vimString becomes vimVar when preceded by !)

let bar = !'g:bar'->exists()


" Issue #14423 (vim.vim: Opt out of vimSearch*)

?truthy
let truthy = 0
\   ? (0
\   )
\   : (1
\   )
echo truthy

function Foo()
  ?truthy
  let truthy = 0
  \   ? (0
  \   )
  \   : (1
  \   )
  echo truthy
endfunction

