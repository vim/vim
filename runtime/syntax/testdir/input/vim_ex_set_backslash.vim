" Vim :set command (escaped characters)
" VIM_TEST_SETUP hi link vimSetEscape      DiffAdd
" VIM_TEST_SETUP hi link vimSetBarEscape   DiffDelete
" VIM_TEST_SETUP hi link vimSetQuoteEscape DiffDelete
" VIM_TEST_SETUP hi link vimSetEqual       DiffChange


echo "-- 1 --"
set titlestring=\afoo\abar\a " comment
echo &titlestring
set titlestring=\afoo\abar\a
echo &titlestring
set titlestring=\ foo\ bar\  " comment
echo &titlestring
set titlestring=\ foo\ bar\ 
echo &titlestring
set titlestring=\|foo\|bar\| " comment
echo &titlestring
set titlestring=\|foo\|bar\|
echo &titlestring
set titlestring=\"foo\"bar\" " comment
echo &titlestring
set titlestring=\"foo\"bar\"
echo &titlestring

echo "-- 2 --"
set titlestring=\\afoo\\abar\\a " comment
echo &titlestring
set titlestring=\\afoo\\abar\\a
echo &titlestring
set titlestring=\\ foo\\ bar\\  " comment
echo &titlestring
set titlestring=\\ foo\\ bar\\ 
echo &titlestring
set titlestring=\\|foo\\|bar\\| " comment
echo &titlestring
set titlestring=\\|foo\\|bar\\|
echo &titlestring
set titlestring=\\"foo\\"bar\\" " comment
echo &titlestring
set titlestring=\\"foo\\"bar\\"
echo &titlestring

echo "-- 3 --"
set titlestring=\\\afoo\\\abar\\\a " comment
echo &titlestring
set titlestring=\\\afoo\\\abar\\\a
echo &titlestring
set titlestring=\\\ foo\\\ bar\\\  " comment
echo &titlestring
set titlestring=\\\ foo\\\ bar\\\ 
echo &titlestring
set titlestring=\\\|foo\\\|bar\\\| " comment
echo &titlestring
set titlestring=\\\|foo\\\|bar\\\|
echo &titlestring
set titlestring=\\\"foo\\\"bar\\\" " comment
echo &titlestring
set titlestring=\\\"foo\\\"bar\\\"
echo &titlestring

echo "-- 4 --"
set titlestring=\\\\afoo\\\\abar\\\\a " comment
echo &titlestring
set titlestring=\\\\afoo\\\\abar\\\\a
echo &titlestring
set titlestring=\\\\ foo\\\\ bar\\\\  " comment
echo &titlestring
set titlestring=\\\\ foo\\\\ bar\\\\ 
echo &titlestring
set titlestring=\\\\|foo\\\\|bar\\\\| " comment
echo &titlestring
set titlestring=\\\\|foo\\\\|bar\\\\|
echo &titlestring
set titlestring=\\\\"foo\\\\"bar\\\\" " comment
echo &titlestring
set titlestring=foo\\\\"\\\\"bar\\\\"
echo &titlestring

