" test_word_boundary_rules.vim
"
" Requires: +eval, +multibyte, UTF-8 encoding

" --------------------------------------------------
" Tests for WB6/WB7 Word Boundary Rules from UAX #29
" --------------------------------------------------

" Test: 'w' in non-compatible mode should treat apostrophe as part of word
func Test_wb_apostrophe()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["don't stop"])
  normal! gg0w
  call assert_equal(7, col('.'))
endfunc

" Test: 'w' in 'compatible' mode should stop at the apostrophe
func Test_wb_apostrophe_compat()
  set compatible
  call setline(1, ["don't stop"])
  normal! gg0w
  call assert_equal(4, col('.'))
endfunc

" Test: 'w' in non-compatible mode should treat U+2019 as part of word
func Test_wb_closing_single_quote()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["don’t stop"])
  normal! gg0w
  call assert_equal(9, col('.'))
endfunc

" Test: 'w' in 'compatible' mode should stop at U+2019
func Test_wb_closing_single_quote_compat()
  set compatible
  call setline(1, ["don’t stop"])
  normal! gg0w
  call assert_equal(4, col('.'))
endfunc

" Test: 'w' in non-compatible mode should treat vim.org as one word
func Test_wb_url()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["Visit vim.org now!"])
  normal! gg02w
  call assert_equal(15, col('.'))
endfunc

" Test: 'w' in 'compatible' mode should treat vim.org as several words
func Test_wb_url_compat()
  set compatible
  call setline(1, ["Visit vim.org now!"])
  normal! gg02w
  call assert_equal(10, col('.'))
endfunc


" ---------------------------------------------------
" Tests for WB11/WB12 Word Boundary Rules from UAX #29
" ---------------------------------------------------

" Test: 'w' in non-compatible mode should treat whole numbers as one word
func Test_wb_number()
  set nocompatible
  set encoding=utf-8
  call setline(1, ["This will cost you 7'250.50 pounds."])
  normal! gg05w
  call assert_equal(29, col('.'))
endfunc

" Test: 'w' in 'compatible' mode should stop at the apostrophe
func Test_wb_number_compat()
  set compatible
  call setline(1, ["This will cost you 7'250.50 pounds."])
  normal! gg05w
  call assert_equal(21, col('.'))
endfunc
