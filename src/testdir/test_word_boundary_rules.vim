" test_word_boundary_rules.vim
"
" Tests Vim's word motion ('w') against Unicode Word Boundary rules (UAX #29).
"
" Requires: +eval, +multibyte, UTF-8 encoding

" ---------------------------------------------------------------------
" Tests for English Word Boundaries (WB6/WB7 from UAX #29)
" ---------------------------------------------------------------------

" Tests that 'w' in nocompatible mode treats an apostrophe as part of a word.
func Test_wb_apostrophe()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["don't stop"])
  normal! gg0w
  " 'w' should move past "don't" and land on 's' of "stop" at column 7.
  call assert_equal(7, col('.'))
endfunc

" Tests that 'w' in compatible mode breaks at an apostrophe.
func Test_wb_apostrophe_compat()
  set compatible
  set encoding=utf-8
  call setline(1, ["don't stop"])
  normal! gg0w
  " 'w' moves from 'd' and stops at the non-keyword apostrophe at column 4.
  call assert_equal(4, col('.'))
endfunc

" Tests that 'w' treats a Unicode apostrophe (U+2019) as part of a word.
func Test_wb_closing_single_quote()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["don’t stop"])
  normal! gg0w
  " The word "don’t" contains a 3-byte char (’).
  " Bytes: d(1)+o(1)+n(1)+’(3)+t(1) + space(1) = 8. Cursor lands at column 9.
  call assert_equal(9, col('.'))
endfunc

" Tests that 'w' in compatible mode breaks at a Unicode apostrophe (U+2019).
func Test_wb_closing_single_quote_compat()
  set compatible
  set encoding=utf-8
  call setline(1, ["don’t stop"])
  normal! gg0w
  call assert_equal(4, col('.'))
endfunc

" Tests that 'w' in nocompatible mode treats a URL-like string as one word.
func Test_wb_url()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["Visit vim.org now!"])
  normal! gg02w
  " 'w' should move past "Visit" and the single word "vim.org".
  call assert_equal(15, col('.'))
endfunc

" Tests that 'w' in compatible mode treats a URL-like string as several words.
func Test_wb_url_compat()
  set compatible
  set encoding=utf-8
  call setline(1, ["Visit vim.org now!"])
  normal! gg02w
  " 'w' moves past "Visit" and "vim", stopping at the '.' at column 10.
  call assert_equal(10, col('.'))
endfunc


" ---------------------------------------------------------------------
" Tests for Hebrew-Specific Word Boundaries (WB7a/WB7b/WB7c)
" ---------------------------------------------------------------------

" Tests simple word motion between two Hebrew words.
func Test_wb_hebrew_simple_word_hop()
  set nocompatible
  set encoding=utf-8
  " Line: "שלום עולם" (Shalom Olam), using no combining characters.
  call setline(1, ['שלום עולם'])
  normal! gg0w
  " 'w' moves past 'שלום' to 'ע'.
  " 'שלום' is 4 chars * 2 bytes/char = 8 bytes. Space is 1 byte.
  " Cursor lands on 'ע' at byte column 10.
  call assert_equal(10, col('.'))
endfunc

" Tests that a word with a geresh (') is treated as one word (WB7a).
func Test_wb_hebrew_geresh()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["זהו ד' קומה"])
  normal! gg0w
  " 'w' should move past 'זהו' to the start of the word "ד'".
  " 'זהו' is 3 chars * 2 bytes/char = 6 bytes. Space is 1 byte.
  " Cursor lands on 'ד', which is at byte column 8.
  call assert_equal(8, col('.'))
endfunc

" Tests that 'w' in compatible mode treats blocks of non-keyword chars as words.
func Test_wb_hebrew_geresh_compat()
  set compatible
  set encoding=utf-8
  call setline(1, ["זהו ד' קומה"])
  " In 'compatible' mode, Hebrew letters aren't 'iskeyword' chars, so 'w'
  " treats 'זהו' and 'ד' as separate "words" of non-keyword characters.
  normal! gg0w
  " The 'w' motion moves from 'זהו' to the start of the next word, 'ד'.
  " 'זהו' is 6 bytes. The space is 1 byte. Cursor lands on 'ד' at column 8.
  call assert_equal(8, col('.'))
endfunc

" Tests that a word with a gershayim (") is treated as one word (WB7b/WB7c).
func Test_wb_hebrew_gershayim()
  set nocompatible
  set encoding=utf-8
  " Sentence: "הגשתי דו"ח למנהל" (I submitted a report to the manager)
  call setline(1, ['הגשתי דו"ח למנהל'])
  normal! gg02w
  " 'w' should move past 'הגשתי' and the single word 'דו"ח', landing on 'למנהל'.
  " Bytes: 'הגשתי'(10) + ' '(1) + 'דו"ח'(7) + ' '(1) = 19 bytes before 'למנהל'.
  " Cursor lands on 'ל' of 'למנהל' at byte column 20.
  call assert_equal(20, col('.'))
endfunc

" Tests that 'w' in compatible mode breaks at the gershayim (").
func Test_wb_hebrew_gershayim_compat()
  set compatible
  set encoding=utf-8
  " Sentence: "הגשתי דו"ח למנהל"
  call setline(1, ['הגשתי דו"ח למנהל'])
  " Move cursor to the start of 'דו"ח', then find the next word boundary.
  normal! gg0w
  normal! w
  " First 'w' lands on 'ד'. Second 'w' moves from 'ד' and stops at the
  " non-keyword gershayim character.
  " Bytes before 'דו"ח': 'הגשתי'(10) + ' '(1) = 11 bytes.
  " Bytes in 'דו"ח' before quote: 'ד'(2) + 'ו'(2) = 4 bytes.
  " Cursor lands on '"' at byte column 11 + 4 + 1 = 16.
  call assert_equal(16, col('.'))
endfunc


" ---------------------------------------------------------------------
" Tests for Number Boundaries (WB11/WB12)
" ---------------------------------------------------------------------

" Tests that 'w' in nocompatible mode treats complex numbers as one word.
func Test_wb_number()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["This will cost you 7'250.50 pounds."])
  normal! gg05w
  " The 5th 'w' moves past "7'250.50" to "pounds" at column 29.
  call assert_equal(29, col('.'))
endfunc

" Tests that 'w' in compatible mode breaks at separators in numbers.
func Test_wb_number_compat()
  set compatible
  set encoding=utf-8
  call setline(1, ["This will cost you 7'250.50 pounds."])
  normal! gg05w
  " The 5th 'w' lands on the apostrophe separator at column 21.
  call assert_equal(21, col('.'))
endfunc
