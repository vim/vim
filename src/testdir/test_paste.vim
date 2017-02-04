" Tests for bracketed paste.

" Bracketed paste only works with "xterm".  Not in GUI.
if has('gui_running')
  finish
endif
set term=xterm

func Test_paste_normal_mode()
  new
  " In first column text is inserted
  call setline(1, ['a', 'b', 'c'])
  call cursor(2, 1)
  call feedkeys("\<Esc>[200~foo\<CR>bar\<Esc>[201~", 'xt')
  call assert_equal('foo', getline(2))
  call assert_equal('barb', getline(3))
  call assert_equal('c', getline(4))

  " When repeating text is appended
  normal .
  call assert_equal('barfoo', getline(3))
  call assert_equal('barb', getline(4))
  call assert_equal('c', getline(5))
  bwipe!

  " In second column text is appended
  call setline(1, ['a', 'bbb', 'c'])
  call cursor(2, 2)
  call feedkeys("\<Esc>[200~foo\<CR>bar\<Esc>[201~", 'xt')
  call assert_equal('bbfoo', getline(2))
  call assert_equal('barb', getline(3))
  call assert_equal('c', getline(4))

  " In last column text is appended
  call setline(1, ['a', 'bbb', 'c'])
  call cursor(2, 3)
  call feedkeys("\<Esc>[200~foo\<CR>bar\<Esc>[201~", 'xt')
  call assert_equal('bbbfoo', getline(2))
  call assert_equal('bar', getline(3))
  call assert_equal('c', getline(4))
endfunc

func Test_paste_insert_mode()
  new
  call setline(1, ['a', 'b', 'c'])
  2
  call feedkeys("i\<Esc>[200~foo\<CR>bar\<Esc>[201~ done\<Esc>", 'xt')
  call assert_equal('foo', getline(2))
  call assert_equal('bar doneb', getline(3))
  call assert_equal('c', getline(4))

  normal .
  call assert_equal('bar donfoo', getline(3))
  call assert_equal('bar doneeb', getline(4))
  call assert_equal('c', getline(5))

  set ai et tw=10
  call setline(1, ['a', '    b', 'c'])
  2
  call feedkeys("A\<Esc>[200~foo\<CR> bar bar bar\<Esc>[201~\<Esc>", 'xt')
  call assert_equal('    bfoo', getline(2))
  call assert_equal(' bar bar bar', getline(3))
  call assert_equal('c', getline(4))

  set ai& et& tw=0
  bwipe!
endfunc

func Test_paste_cmdline()
  call feedkeys(":a\<Esc>[200~foo\<CR>bar\<Esc>[201~b\<Home>\"\<CR>", 'xt')
  call assert_equal("\"afoo\<CR>barb", getreg(':'))
endfunc

func Test_paste_visual_mode()
  new
  call setline(1, 'here are some words')
  call feedkeys("0fsve\<Esc>[200~more\<Esc>[201~", 'xt')
  call assert_equal('here are more words', getline(1))
  call assert_equal('some', getreg('-'))

  " include last char in the line
  call feedkeys("0fwve\<Esc>[200~noises\<Esc>[201~", 'xt')
  call assert_equal('here are more noises', getline(1))
  call assert_equal('words', getreg('-'))

  " exclude last char in the line
  call setline(1, 'some words!')
  call feedkeys("0fwve\<Esc>[200~noises\<Esc>[201~", 'xt')
  call assert_equal('some noises!', getline(1))
  call assert_equal('words', getreg('-'))

  " multi-line selection
  call setline(1, ['some words', 'and more'])
  call feedkeys("0fwvj0fd\<Esc>[200~letters\<Esc>[201~", 'xt')
  call assert_equal('some letters more', getline(1))
  call assert_equal("words\nand", getreg('1'))

  bwipe!
endfunc
