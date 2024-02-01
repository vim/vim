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

" String interpolation

echo 'Don''t highlight interpolation: {{ {1 + 2} }}'
echo "Don't highlight interpolation: {{ {1 + 2} }}"
echo $'Highlight interpolation:\t{{ { string({'foo': 'bar'}) } }}'
echo $'Highlight interpolation:\t{{ { $'nested: {{ {1 + 2} }}' } }}'
echo $"Highlight interpolation:\t{{ { string({"foo": "bar"}) } }}"
echo $"Highlight interpolation:\t{{ { $"nested: {{ {1 + 2} }}" } }}"

