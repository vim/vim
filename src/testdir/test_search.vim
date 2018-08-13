" Test for the search command

source shared.vim
source screendump.vim

func Test_search_cmdline()
  if !exists('+incsearch')
    return
  endif
  " need to disable char_avail,
  " so that expansion of commandline works
  call test_override("char_avail", 1)
  new
  call setline(1, ['  1', '  2 these', '  3 the', '  4 their', '  5 there', '  6 their', '  7 the', '  8 them', '  9 these', ' 10 foobar'])
  " Test 1
  " CTRL-N / CTRL-P skips through the previous search history
  set noincsearch
  :1
  call feedkeys("/foobar\<cr>", 'tx')
  call feedkeys("/the\<cr>",'tx')
  call assert_equal('the', @/)
  call feedkeys("/thes\<C-P>\<C-P>\<cr>",'tx')
  call assert_equal('foobar', @/)

  " Test 2
  " Ctrl-G goes from one match to the next
  " until the end of the buffer
  set incsearch nowrapscan
  :1
  " first match
  call feedkeys("/the\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  :1
  " second match
  call feedkeys("/the\<C-G>\<cr>", 'tx')
  call assert_equal('  3 the', getline('.'))
  call assert_equal([0, 0, 0, 0], getpos('"'))
  :1
  " third match
  call feedkeys("/the".repeat("\<C-G>", 2)."\<cr>", 'tx')
  call assert_equal('  4 their', getline('.'))
  :1
  " fourth match
  call feedkeys("/the".repeat("\<C-G>", 3)."\<cr>", 'tx')
  call assert_equal('  5 there', getline('.'))
  :1
  " fifth match
  call feedkeys("/the".repeat("\<C-G>", 4)."\<cr>", 'tx')
  call assert_equal('  6 their', getline('.'))
  :1
  " sixth match
  call feedkeys("/the".repeat("\<C-G>", 5)."\<cr>", 'tx')
  call assert_equal('  7 the', getline('.'))
  :1
  " seventh match
  call feedkeys("/the".repeat("\<C-G>", 6)."\<cr>", 'tx')
  call assert_equal('  8 them', getline('.'))
  :1
  " eigth match
  call feedkeys("/the".repeat("\<C-G>", 7)."\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))
  :1
  " no further match
  call feedkeys("/the".repeat("\<C-G>", 8)."\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))
  call assert_equal([0, 0, 0, 0], getpos('"'))

  " Test 3
  " Ctrl-G goes from one match to the next
  " and continues back at the top
  set incsearch wrapscan
  :1
  " first match
  call feedkeys("/the\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  :1
  " second match
  call feedkeys("/the\<C-G>\<cr>", 'tx')
  call assert_equal('  3 the', getline('.'))
  :1
  " third match
  call feedkeys("/the".repeat("\<C-G>", 2)."\<cr>", 'tx')
  call assert_equal('  4 their', getline('.'))
  :1
  " fourth match
  call feedkeys("/the".repeat("\<C-G>", 3)."\<cr>", 'tx')
  call assert_equal('  5 there', getline('.'))
  :1
  " fifth match
  call feedkeys("/the".repeat("\<C-G>", 4)."\<cr>", 'tx')
  call assert_equal('  6 their', getline('.'))
  :1
  " sixth match
  call feedkeys("/the".repeat("\<C-G>", 5)."\<cr>", 'tx')
  call assert_equal('  7 the', getline('.'))
  :1
  " seventh match
  call feedkeys("/the".repeat("\<C-G>", 6)."\<cr>", 'tx')
  call assert_equal('  8 them', getline('.'))
  :1
  " eigth match
  call feedkeys("/the".repeat("\<C-G>", 7)."\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))
  :1
  " back at first match
  call feedkeys("/the".repeat("\<C-G>", 8)."\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))

  " Test 4
  " CTRL-T goes to the previous match
  set incsearch nowrapscan
  $
  " first match
  call feedkeys("?the\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))
  $
  " first match
  call feedkeys("?the\<C-G>\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))
  $
  " second match
  call feedkeys("?the".repeat("\<C-T>", 1)."\<cr>", 'tx')
  call assert_equal('  8 them', getline('.'))
  $
  " last match
  call feedkeys("?the".repeat("\<C-T>", 7)."\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  $
  " last match
  call feedkeys("?the".repeat("\<C-T>", 8)."\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))

  " Test 5
  " CTRL-T goes to the previous match
  set incsearch wrapscan
  $
  " first match
  call feedkeys("?the\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))
  $
  " first match at the top
  call feedkeys("?the\<C-G>\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  $
  " second match
  call feedkeys("?the".repeat("\<C-T>", 1)."\<cr>", 'tx')
  call assert_equal('  8 them', getline('.'))
  $
  " last match
  call feedkeys("?the".repeat("\<C-T>", 7)."\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  $
  " back at the bottom of the buffer
  call feedkeys("?the".repeat("\<C-T>", 8)."\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))

  " Test 6
  " CTRL-L adds to the search pattern
  set incsearch wrapscan
  1
  " first match
  call feedkeys("/the\<c-l>\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  1
  " go to next match of 'thes'
  call feedkeys("/the\<c-l>\<C-G>\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))
  1
  " wrap around
  call feedkeys("/the\<c-l>\<C-G>\<C-G>\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  1
  " wrap around
  set nowrapscan
  call feedkeys("/the\<c-l>\<C-G>\<C-G>\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))

  " Test 7
  " <bs> remove from match, but stay at current match
  set incsearch wrapscan
  1
  " first match
  call feedkeys("/thei\<cr>", 'tx')
  call assert_equal('  4 their', getline('.'))
  1
  " delete one char, add another
  call feedkeys("/thei\<bs>s\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  1
  " delete one char, add another,  go to previous match, add one char
  call feedkeys("/thei\<bs>s\<bs>\<C-T>\<c-l>\<cr>", 'tx')
  call assert_equal('  9 these', getline('.'))
  1
  " delete all chars, start from the beginning again
  call feedkeys("/them". repeat("\<bs>",4).'the\>'."\<cr>", 'tx')
  call assert_equal('  3 the', getline('.'))

  " clean up
  call test_override("char_avail", 0)
  bw!
endfunc

func Test_search_cmdline2()
  if !exists('+incsearch')
    return
  endif
  " need to disable char_avail,
  " so that expansion of commandline works
  call test_override("char_avail", 1)
  new
  call setline(1, ['  1', '  2 these', '  3 the theother'])
  " Test 1
  " Ctrl-T goes correctly back and forth
  set incsearch
  1
  " first match
  call feedkeys("/the\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))
  1
  " go to next match (on next line)
  call feedkeys("/the\<C-G>\<cr>", 'tx')
  call assert_equal('  3 the theother', getline('.'))
  1
  " go to next match (still on line 3)
  call feedkeys("/the\<C-G>\<C-G>\<cr>", 'tx')
  call assert_equal('  3 the theother', getline('.'))
  1
  " go to next match (still on line 3)
  call feedkeys("/the\<C-G>\<C-G>\<C-G>\<cr>", 'tx')
  call assert_equal('  3 the theother', getline('.'))
  1
  " go to previous match (on line 3)
  call feedkeys("/the\<C-G>\<C-G>\<C-G>\<C-T>\<cr>", 'tx')
  call assert_equal('  3 the theother', getline('.'))
  1
  " go to previous match (on line 3)
  call feedkeys("/the\<C-G>\<C-G>\<C-G>\<C-T>\<C-T>\<cr>", 'tx')
  call assert_equal('  3 the theother', getline('.'))
  1
  " go to previous match (on line 2)
  call feedkeys("/the\<C-G>\<C-G>\<C-G>\<C-T>\<C-T>\<C-T>\<cr>", 'tx')
  call assert_equal('  2 these', getline('.'))

  " Test 2: keep the view,
  " after deleting a character from the search cmd
  call setline(1, ['  1', '  2 these', '  3 the', '  4 their', '  5 there', '  6 their', '  7 the', '  8 them', '  9 these', ' 10 foobar'])
  resize 5
  1
  call feedkeys("/foo\<bs>\<cr>", 'tx')
  redraw
  call assert_equal({'lnum': 10, 'leftcol': 0, 'col': 4, 'topfill': 0, 'topline': 6, 'coladd': 0, 'skipcol': 0, 'curswant': 4}, winsaveview())

  " remove all history entries
  for i in range(10)
      call histdel('/')
  endfor

  " Test 3: reset the view,
  " after deleting all characters from the search cmd
  norm! 1gg0
  " unfortunately, neither "/foo\<c-w>\<cr>", nor "/foo\<bs>\<bs>\<bs>\<cr>",
  " nor "/foo\<c-u>\<cr>" works to delete the commandline.
  " In that case Vim should return "E35 no previous regular expression",
  " but it looks like Vim still sees /foo and therefore the test fails.
  " Therefore, disableing this test
  "call assert_fails(feedkeys("/foo\<c-w>\<cr>", 'tx'), 'E35')
  "call assert_equal({'lnum': 1, 'leftcol': 0, 'col': 0, 'topfill': 0, 'topline': 1, 'coladd': 0, 'skipcol': 0, 'curswant': 0}, winsaveview())

  " clean up
  set noincsearch
  call test_override("char_avail", 0)
  bw!
endfunc

func Test_use_sub_pat()
  split
  let @/ = ''
  func X()
    s/^/a/
    /
  endfunc
  call X()
  bwipe!
endfunc

func Test_searchpair()
  new
  call setline(1, ['other code here', '', '[', '" cursor here', ']'])
  4
  let a = searchpair('\[','',']','bW')
  call assert_equal(3, a)
  set nomagic
  4
  let a = searchpair('\[','',']','bW')
  call assert_equal(3, a)
  set magic
  q!
endfunc

func Test_searchpair_errors()
  call assert_fails("call searchpair([0], 'middle', 'end', 'bW', 'skip', 99, 100)", 'E730: using List as a String')
  call assert_fails("call searchpair('start', {-> 0}, 'end', 'bW', 'skip', 99, 100)", 'E729: using Funcref as a String')
  call assert_fails("call searchpair('start', 'middle', {'one': 1}, 'bW', 'skip', 99, 100)", 'E731: using Dictionary as a String')
  call assert_fails("call searchpair('start', 'middle', 'end', 'flags', 'skip', 99, 100)", 'E475: Invalid argument: flags')
  call assert_fails("call searchpair('start', 'middle', 'end', 'bW', 0, 99, 100)", 'E475: Invalid argument: 0')
  call assert_fails("call searchpair('start', 'middle', 'end', 'bW', 'func', -99, 100)", 'E475: Invalid argument: -99')
  call assert_fails("call searchpair('start', 'middle', 'end', 'bW', 'func', 99, -100)", 'E475: Invalid argument: -100')
endfunc

func Test_searchpair_skip()
    func Zero()
	return 0
    endfunc
    func Partial(x)
	return a:x
    endfunc
    new
    call setline(1, ['{', 'foo', 'foo', 'foo', '}'])
    3 | call assert_equal(1, searchpair('{', '', '}', 'bWn', ''))
    3 | call assert_equal(1, searchpair('{', '', '}', 'bWn', '0'))
    3 | call assert_equal(1, searchpair('{', '', '}', 'bWn', {-> 0}))
    3 | call assert_equal(1, searchpair('{', '', '}', 'bWn', function('Zero')))
    3 | call assert_equal(1, searchpair('{', '', '}', 'bWn', function('Partial', [0])))
    bw!
endfunc

func Test_searchpair_leak()
  new
  call setline(1, 'if one else another endif')

  " The error in the skip expression caused memory to leak.
  call assert_fails("call searchpair('\\<if\\>', '\\<else\\>', '\\<endif\\>', '', '\"foo\" 2')", 'E15:')

  bwipe!
endfunc

func Test_searchc()
  " These commands used to cause memory overflow in searchc().
  new
  norm ixx
  exe "norm 0t\u93cf"
  bw!
endfunc

func Cmdline3_prep()
  " need to disable char_avail,
  " so that expansion of commandline works
  call test_override("char_avail", 1)
  new
  call setline(1, ['  1', '  2 the~e', '  3 the theother'])
  set incsearch
endfunc

func Incsearch_cleanup()
  set noincsearch
  call test_override("char_avail", 0)
  bw!
endfunc

func Test_search_cmdline3()
  if !exists('+incsearch')
    return
  endif
  call Cmdline3_prep()
  1
  " first match
  call feedkeys("/the\<c-l>\<cr>", 'tx')
  call assert_equal('  2 the~e', getline('.'))

  call Incsearch_cleanup()
endfunc

func Test_search_cmdline3s()
  if !exists('+incsearch')
    return
  endif
  call Cmdline3_prep()
  1
  call feedkeys(":%s/the\<c-l>/xxx\<cr>", 'tx')
  call assert_equal('  2 xxxe', getline('.'))
  undo
  call feedkeys(":%subs/the\<c-l>/xxx\<cr>", 'tx')
  call assert_equal('  2 xxxe', getline('.'))
  undo
  call feedkeys(":%substitute/the\<c-l>/xxx\<cr>", 'tx')
  call assert_equal('  2 xxxe', getline('.'))

  call Incsearch_cleanup()
endfunc

func Test_search_cmdline3g()
  if !exists('+incsearch')
    return
  endif
  call Cmdline3_prep()
  1
  call feedkeys(":g/the\<c-l>/d\<cr>", 'tx')
  call assert_equal('  3 the theother', getline(2))
  undo
  call feedkeys(":global/the\<c-l>/d\<cr>", 'tx')
  call assert_equal('  3 the theother', getline(2))
  undo
  call feedkeys(":g!/the\<c-l>/d\<cr>", 'tx')
  call assert_equal(1, line('$'))
  call assert_equal('  2 the~e', getline(1))
  undo
  call feedkeys(":global!/the\<c-l>/d\<cr>", 'tx')
  call assert_equal(1, line('$'))
  call assert_equal('  2 the~e', getline(1))

  call Incsearch_cleanup()
endfunc

func Test_search_cmdline3v()
  if !exists('+incsearch')
    return
  endif
  call Cmdline3_prep()
  1
  call feedkeys(":v/the\<c-l>/d\<cr>", 'tx')
  call assert_equal(1, line('$'))
  call assert_equal('  2 the~e', getline(1))
  undo
  call feedkeys(":vglobal/the\<c-l>/d\<cr>", 'tx')
  call assert_equal(1, line('$'))
  call assert_equal('  2 the~e', getline(1))

  call Incsearch_cleanup()
endfunc

func Test_search_cmdline4()
  if !exists('+incsearch')
    return
  endif
  " need to disable char_avail,
  " so that expansion of commandline works
  call test_override("char_avail", 1)
  new
  call setline(1, ['  1 the first', '  2 the second', '  3 the third'])
  set incsearch
  $
  call feedkeys("?the\<c-g>\<cr>", 'tx')
  call assert_equal('  3 the third', getline('.'))
  $
  call feedkeys("?the\<c-g>\<c-g>\<cr>", 'tx')
  call assert_equal('  1 the first', getline('.'))
  $
  call feedkeys("?the\<c-g>\<c-g>\<c-g>\<cr>", 'tx')
  call assert_equal('  2 the second', getline('.'))
  $
  call feedkeys("?the\<c-t>\<cr>", 'tx')
  call assert_equal('  1 the first', getline('.'))
  $
  call feedkeys("?the\<c-t>\<c-t>\<cr>", 'tx')
  call assert_equal('  3 the third', getline('.'))
  $
  call feedkeys("?the\<c-t>\<c-t>\<c-t>\<cr>", 'tx')
  call assert_equal('  2 the second', getline('.'))
  " clean up
  set noincsearch
  call test_override("char_avail", 0)
  bw!
endfunc

func Test_search_cmdline5()
  if !exists('+incsearch')
    return
  endif
  " Do not call test_override("char_avail", 1) so that <C-g> and <C-t> work
  " regardless char_avail.
  new
  call setline(1, ['  1 the first', '  2 the second', '  3 the third'])
  set incsearch
  1
  call feedkeys("/the\<c-g>\<c-g>\<cr>", 'tx')
  call assert_equal('  3 the third', getline('.'))
  $
  call feedkeys("?the\<c-t>\<c-t>\<c-t>\<cr>", 'tx')
  call assert_equal('  2 the second', getline('.'))
  " clean up
  set noincsearch
  bw!
endfunc

func Test_search_cmdline6()
  " Test that consecutive matches
  " are caught by <c-g>/<c-t>
  if !exists('+incsearch')
    return
  endif
  " need to disable char_avail,
  " so that expansion of commandline works
  call test_override("char_avail", 1)
  new
  call setline(1, [' bbvimb', ''])
  set incsearch
  " first match
  norm! gg0
  call feedkeys("/b\<cr>", 'tx')
  call assert_equal([0,1,2,0], getpos('.'))
  " second match
  norm! gg0
  call feedkeys("/b\<c-g>\<cr>", 'tx')
  call assert_equal([0,1,3,0], getpos('.'))
  " third match
  norm! gg0
  call feedkeys("/b\<c-g>\<c-g>\<cr>", 'tx')
  call assert_equal([0,1,7,0], getpos('.'))
  " first match again
  norm! gg0
  call feedkeys("/b\<c-g>\<c-g>\<c-g>\<cr>", 'tx')
  call assert_equal([0,1,2,0], getpos('.'))
  set nowrapscan
  " last match
  norm! gg0
  call feedkeys("/b\<c-g>\<c-g>\<c-g>\<cr>", 'tx')
  call assert_equal([0,1,7,0], getpos('.'))
  " clean up
  set wrapscan&vim
  set noincsearch
  call test_override("char_avail", 0)
  bw!
endfunc

func Test_search_cmdline7()
  " Test that an pressing <c-g> in an empty command line
  " does not move the cursor
  if !exists('+incsearch')
    return
  endif
  " need to disable char_avail,
  " so that expansion of commandline works
  call test_override("char_avail", 1)
  new
  let @/ = 'b'
  call setline(1, [' bbvimb', ''])
  set incsearch
  " first match
  norm! gg0
  " moves to next match of previous search pattern, just like /<cr>
  call feedkeys("/\<c-g>\<cr>", 'tx')
  call assert_equal([0,1,2,0], getpos('.'))
  " moves to next match of previous search pattern, just like /<cr>
  call feedkeys("/\<cr>", 'tx')
  call assert_equal([0,1,3,0], getpos('.'))
  " moves to next match of previous search pattern, just like /<cr>
  call feedkeys("/\<c-t>\<cr>", 'tx')
  call assert_equal([0,1,7,0], getpos('.'))

  " using an offset uses the last search pattern
  call cursor(1, 1)
  call setline(1, ['1 bbvimb', ' 2 bbvimb'])
  let @/ = 'b'
  call feedkeys("//e\<c-g>\<cr>", 'tx')
  call assert_equal('1 bbvimb', getline('.'))
  call assert_equal(4, col('.'))

  set noincsearch
  call test_override("char_avail", 0)
  bw!
endfunc

func Test_search_cmdline8()
  " Highlighting is cleared in all windows
  " since hls applies to all windows
  if !exists('+incsearch') || !has('terminal') || has('gui_running') || winwidth(0) < 30
    return
  endif
  if has("win32")
    throw "Skipped: Bug with sending <ESC> to terminal window not fixed yet"
  endif
  let h = winheight(0)
  if h < 3
    return
  endif
  " Prepare buffer text
  let lines = ['abb vim vim vi', 'vimvivim']
  call writefile(lines, 'Xsearch.txt')
  let buf = term_start([GetVimProg(), '--clean', '-c', 'set noswapfile', 'Xsearch.txt'], {'term_rows': 3})

  call WaitForAssert({-> assert_equal(lines, [term_getline(buf, 1), term_getline(buf, 2)])})

  call term_sendkeys(buf, ":set incsearch hlsearch\<cr>")
  call term_sendkeys(buf, ":14vsp\<cr>")
  call term_sendkeys(buf, "/vim\<cr>")
  call term_sendkeys(buf, "/b\<esc>")
  call term_sendkeys(buf, "gg0")
  call term_wait(buf, 500)
  let screen_line = term_scrape(buf, 1)
  let [a0,a1,a2,a3] = [screen_line[3].attr, screen_line[4].attr,
        \ screen_line[18].attr, screen_line[19].attr]
  call assert_notequal(a0, a1)
  call assert_notequal(a0, a3)
  call assert_notequal(a1, a2)
  call assert_equal(a0, a2)
  call assert_equal(a1, a3)
  " clean up
  call delete('Xsearch.txt')

  bwipe!
endfunc

" Tests for regexp with various magic settings
func Test_search_regexp()
  enew!

  put ='1 a aa abb abbccc'
  exe 'normal! /a*b\{2}c\+/e' . "\<CR>"
  call assert_equal([0, 2, 17, 0], getpos('.'))

  put ='2 d dd dee deefff'
  exe 'normal! /\Md\*e\{2}f\+/e' . "\<CR>"
  call assert_equal([0, 3, 17, 0], getpos('.'))

  set nomagic
  put ='3 g gg ghh ghhiii'
  exe 'normal! /g\*h\{2}i\+/e' . "\<CR>"
  call assert_equal([0, 4, 17, 0], getpos('.'))

  put ='4 j jj jkk jkklll'
  exe 'normal! /\mj*k\{2}l\+/e' . "\<CR>"
  call assert_equal([0, 5, 17, 0], getpos('.'))

  put ='5 m mm mnn mnnooo'
  exe 'normal! /\vm*n{2}o+/e' . "\<CR>"
  call assert_equal([0, 6, 17, 0], getpos('.'))

  put ='6 x ^aa$ x'
  exe 'normal! /\V^aa$' . "\<CR>"
  call assert_equal([0, 7, 5, 0], getpos('.'))

  set magic
  put ='7 (a)(b) abbaa'
  exe 'normal! /\v(a)(b)\2\1\1/e' . "\<CR>"
  call assert_equal([0, 8, 14, 0], getpos('.'))

  put ='8 axx [ab]xx'
  exe 'normal! /\V[ab]\(\[xy]\)\1' . "\<CR>"
  call assert_equal([0, 9, 7, 0], getpos('.'))

  set undolevels=100
  put ='9 foobar'
  put =''
  exe "normal! a\<C-G>u\<Esc>"
  normal G
  exe 'normal! dv?bar?' . "\<CR>"
  call assert_equal('9 foo', getline('.'))
  call assert_equal([0, 10, 5, 0], getpos('.'))
  call assert_equal(10, line('$'))
  normal u
  call assert_equal('9 foobar', getline('.'))
  call assert_equal([0, 10, 6, 0], getpos('.'))
  call assert_equal(11, line('$'))

  set undolevels&
  enew!
endfunc

func Test_search_cmdline_incsearch_highlight()
  if !exists('+incsearch')
    return
  endif
  set incsearch hlsearch
  " need to disable char_avail,
  " so that expansion of commandline works
  call test_override("char_avail", 1)
  new
  call setline(1, ['aaa  1 the first', '  2 the second', '  3 the third'])

  1
  call feedkeys("/second\<cr>", 'tx')
  call assert_equal('second', @/)
  call assert_equal('  2 the second', getline('.'))

  " Canceling search won't change @/
  1
  let @/ = 'last pattern'
  call feedkeys("/third\<C-c>", 'tx')
  call assert_equal('last pattern', @/)
  call feedkeys("/third\<Esc>", 'tx')
  call assert_equal('last pattern', @/)
  call feedkeys("/3\<bs>\<bs>", 'tx')
  call assert_equal('last pattern', @/)
  call feedkeys("/third\<c-g>\<c-t>\<Esc>", 'tx')
  call assert_equal('last pattern', @/)

  " clean up
  set noincsearch nohlsearch
  bw!
endfunc

func Test_search_cmdline_incsearch_highlight_attr()
  if !exists('+incsearch') || !has('terminal') || has('gui_running')
    return
  endif
  let h = winheight(0)
  if h < 3
    return
  endif

  " Prepare buffer text
  let lines = ['abb vim vim vi', 'vimvivim']
  call writefile(lines, 'Xsearch.txt')
  let buf = term_start([GetVimProg(), '--clean', '-c', 'set noswapfile', 'Xsearch.txt'], {'term_rows': 3})

  call WaitForAssert({-> assert_equal(lines, [term_getline(buf, 1), term_getline(buf, 2)])})
  " wait for vim to complete initialization
  call term_wait(buf)

  " Get attr of normal(a0), incsearch(a1), hlsearch(a2) highlight
  call term_sendkeys(buf, ":set incsearch hlsearch\<cr>")
  call term_sendkeys(buf, '/b')
  call term_wait(buf, 200)
  let screen_line1 = term_scrape(buf, 1)
  call assert_true(len(screen_line1) > 2)
  " a0: attr_normal
  let a0 = screen_line1[0].attr
  " a1: attr_incsearch
  let a1 = screen_line1[1].attr
  " a2: attr_hlsearch
  let a2 = screen_line1[2].attr
  call assert_notequal(a0, a1)
  call assert_notequal(a0, a2)
  call assert_notequal(a1, a2)
  call term_sendkeys(buf, "\<cr>gg0")

  " Test incremental highlight search
  call term_sendkeys(buf, "/vim")
  call term_wait(buf, 200)
  " Buffer:
  " abb vim vim vi
  " vimvivim
  " Search: /vim
  let attr_line1 = [a0,a0,a0,a0,a1,a1,a1,a0,a2,a2,a2,a0,a0,a0]
  let attr_line2 = [a2,a2,a2,a0,a0,a2,a2,a2]
  call assert_equal(attr_line1, map(term_scrape(buf, 1)[:len(attr_line1)-1], 'v:val.attr'))
  call assert_equal(attr_line2, map(term_scrape(buf, 2)[:len(attr_line2)-1], 'v:val.attr'))

  " Test <C-g>
  call term_sendkeys(buf, "\<C-g>\<C-g>")
  call term_wait(buf, 200)
  let attr_line1 = [a0,a0,a0,a0,a2,a2,a2,a0,a2,a2,a2,a0,a0,a0]
  let attr_line2 = [a1,a1,a1,a0,a0,a2,a2,a2]
  call assert_equal(attr_line1, map(term_scrape(buf, 1)[:len(attr_line1)-1], 'v:val.attr'))
  call assert_equal(attr_line2, map(term_scrape(buf, 2)[:len(attr_line2)-1], 'v:val.attr'))

  " Test <C-t>
  call term_sendkeys(buf, "\<C-t>")
  call term_wait(buf, 200)
  let attr_line1 = [a0,a0,a0,a0,a2,a2,a2,a0,a1,a1,a1,a0,a0,a0]
  let attr_line2 = [a2,a2,a2,a0,a0,a2,a2,a2]
  call assert_equal(attr_line1, map(term_scrape(buf, 1)[:len(attr_line1)-1], 'v:val.attr'))
  call assert_equal(attr_line2, map(term_scrape(buf, 2)[:len(attr_line2)-1], 'v:val.attr'))

  " Type Enter and a1(incsearch highlight) should become a2(hlsearch highlight)
  call term_sendkeys(buf, "\<cr>")
  call term_wait(buf, 200)
  let attr_line1 = [a0,a0,a0,a0,a2,a2,a2,a0,a2,a2,a2,a0,a0,a0]
  let attr_line2 = [a2,a2,a2,a0,a0,a2,a2,a2]
  call assert_equal(attr_line1, map(term_scrape(buf, 1)[:len(attr_line1)-1], 'v:val.attr'))
  call assert_equal(attr_line2, map(term_scrape(buf, 2)[:len(attr_line2)-1], 'v:val.attr'))

  " Test nohlsearch. a2(hlsearch highlight) should become a0(normal highlight)
  call term_sendkeys(buf, ":1\<cr>")
  call term_sendkeys(buf, ":set nohlsearch\<cr>")
  call term_sendkeys(buf, "/vim")
  call term_wait(buf, 200)
  let attr_line1 = [a0,a0,a0,a0,a1,a1,a1,a0,a0,a0,a0,a0,a0,a0]
  let attr_line2 = [a0,a0,a0,a0,a0,a0,a0,a0]
  call assert_equal(attr_line1, map(term_scrape(buf, 1)[:len(attr_line1)-1], 'v:val.attr'))
  call assert_equal(attr_line2, map(term_scrape(buf, 2)[:len(attr_line2)-1], 'v:val.attr'))
  call delete('Xsearch.txt')

  call delete('Xsearch.txt')
  bwipe!
endfunc

func Test_incsearch_scrolling()
  if !CanRunVimInTerminal()
    return
  endif
  call assert_equal(0, &scrolloff)
  call writefile([
	\ 'let dots = repeat(".", 120)',
	\ 'set incsearch cmdheight=2 scrolloff=0',
	\ 'call setline(1, [dots, dots, dots, "", "target", dots, dots])',
	\ 'normal gg',
	\ 'redraw',
	\ ], 'Xscript')
  let buf = RunVimInTerminal('-S Xscript', {'rows': 9, 'cols': 70})
  " Need to send one key at a time to force a redraw
  call term_sendkeys(buf, '/')
  sleep 100m
  call term_sendkeys(buf, 't')
  sleep 100m
  call term_sendkeys(buf, 'a')
  sleep 100m
  call term_sendkeys(buf, 'r')
  sleep 100m
  call term_sendkeys(buf, 'g')
  call VerifyScreenDump(buf, 'Test_incsearch_scrolling_01', {})

  call term_sendkeys(buf, "\<Esc>")
  call StopVimInTerminal(buf)
  call delete('Xscript')
endfunc

func Test_incsearch_substitute()
  if !exists('+incsearch')
    return
  endif
  call test_override("char_avail", 1)
  new
  set incsearch
  for n in range(1, 10)
    call setline(n, 'foo ' . n)
  endfor
  4
  call feedkeys(":.,.+2s/foo\<BS>o\<BS>o/xxx\<cr>", 'tx')
  call assert_equal('foo 3', getline(3))
  call assert_equal('xxx 4', getline(4))
  call assert_equal('xxx 5', getline(5))
  call assert_equal('xxx 6', getline(6))
  call assert_equal('foo 7', getline(7))

  call Incsearch_cleanup()
endfunc

" Similar to Test_incsearch_substitute() but with a screendump halfway.
func Test_incsearch_substitute_dump()
  if !exists('+incsearch')
    return
  endif
  if !CanRunVimInTerminal()
    return
  endif
  call writefile([
	\ 'set incsearch hlsearch scrolloff=0',
	\ 'for n in range(1, 10)',
	\ '  call setline(n, "foo " . n)',
	\ 'endfor',
	\ '3',
	\ ], 'Xis_subst_script')
  let buf = RunVimInTerminal('-S Xis_subst_script', {'rows': 9, 'cols': 70})
  " Give Vim a chance to redraw to get rid of the spaces in line 2 caused by
  " the 'ambiwidth' check.
  sleep 100m

  " Need to send one key at a time to force a redraw.
  " Select three lines at the cursor with typed pattern.
  call term_sendkeys(buf, ':.,.+2s/')
  sleep 100m
  call term_sendkeys(buf, 'f')
  sleep 100m
  call term_sendkeys(buf, 'o')
  sleep 100m
  call term_sendkeys(buf, 'o')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_incsearch_substitute_01', {})
  call term_sendkeys(buf, "\<Esc>")

  " Select three lines at the cursor using previous pattern.
  call term_sendkeys(buf, "/foo\<CR>")
  sleep 100m
  call term_sendkeys(buf, ':.,.+2s//')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_incsearch_substitute_02', {})

  " Deleting last slash should remove the match.
  call term_sendkeys(buf, "\<BS>")
  sleep 100m
  call VerifyScreenDump(buf, 'Test_incsearch_substitute_03', {})
  call term_sendkeys(buf, "\<Esc>")

  " Reverse range is accepted
  call term_sendkeys(buf, ':5,2s/foo')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_incsearch_substitute_04', {})
  call term_sendkeys(buf, "\<Esc>")

  " White space after the command is skipped
  call term_sendkeys(buf, ':2,3sub  /fo')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_incsearch_substitute_05', {})
  call term_sendkeys(buf, "\<Esc>")

  call StopVimInTerminal(buf)
  call delete('Xis_subst_script')
endfunc

func Test_search_undefined_behaviour()
  if !has("terminal")
    return
  endif
  let h = winheight(0)
  if h < 3
    return
  endif
  " did cause an undefined left shift
  let g:buf = term_start([GetVimProg(), '--clean', '-e', '-s', '-c', 'call search(getline("."))', 'samples/test000'], {'term_rows': 3})
  call assert_equal([''], getline(1, '$'))
  call term_sendkeys(g:buf, ":qa!\<cr>")
  bwipe!
endfunc

func Test_search_undefined_behaviour2()
  call search("\%UC0000000")
endfunc

" Test for search('multi-byte char', 'bce')
func Test_search_multibyte()
  if !has('multi_byte')
    return
  endif
  let save_enc = &encoding
  set encoding=utf8
  enew!
  call append('$', 'Ａ')
  call cursor(2, 1)
  call assert_equal(2, search('Ａ', 'bce', line('.')))
  enew!
  let &encoding = save_enc
endfunc

" This was causing E874.  Also causes an invalid read?
func Test_look_behind()
  new
  call setline(1, '0\|\&\n\@<=') 
  call search(getline("."))
  bwipe!
endfunc

func Test_search_sentence()
  new
  " this used to cause a crash
  call assert_fails("/\\%')", 'E486')
  call assert_fails("/", 'E486')
  /\%'(
  /
endfunc
