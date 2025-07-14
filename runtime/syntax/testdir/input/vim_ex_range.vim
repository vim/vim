" Ex command ranges


'<,'>print
'(,')print
'{,'}print
'[,']print

:'<,'>print
:'(,')print
:'{,'}print
:'[,']print

echo | '<,'>print
echo | '(,')print
echo | '{,'}print
echo | '[,']print

" bare mark ranges

'a
'k
'z
'A
'K
'Z
'0
'9
'[
']
'{
'}
'(
')
'<
'>
'`
''
'"
'^
'.

 :'a
: 'a
:'a
:'k
:'z
:'A
:'K
:'Z
:'0
:'9
:'[
:']
:'{
:'}
:'(
:')
:'<
:'>
:'`
:''
:'"
:'^
:'.

echo |'a
echo| 'a
echo | 'a
echo | 'k
echo | 'z
echo | 'A
echo | 'K
echo | 'Z
echo | '0
echo | '9
echo | '[
echo | ']
echo | '{
echo | '}
echo | '(
echo | ')
echo | '<
echo | '>
echo | '`
echo | ''
echo | '"
echo | '^
echo | '.

