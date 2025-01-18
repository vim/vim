" String

echo 'It''s a string'
echo 'tab: \t, new line: \n, backslash: \\'
echo "tab: \t, new line: \n, backslash: \\"

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

echo $''''
echo $'''foo'
echo $'foo'''
echo $'foo''bar'

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

" Operators

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

" Unreported issue (incorrectly matches as "echo vimNumber *vimCommand* vimNumber")
echo 42 is 42

" Issue #16221 (vimString becomes vimVar when preceded by !)
let bar = !'g:bar'->exists()

