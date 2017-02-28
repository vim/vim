" Simplistic testing of Arabic mode.

if !has('arabic') || !has('multi_byte')
  finish
endif

set encoding=utf-8
scriptencoding utf-8

source view_util.vim

" Return list of Unicode characters at line lnum.
" Combining characters are treated as a single item.
func s:get_chars(lnum)
  call cursor(a:lnum, 1)
  let chars = []
  let numchars = strchars(getline('.'), 1)
  for i in range(1, numchars)
    exe 'norm ' i . '|'
    let c=execute('ascii')
    let c=substitute(c, '\n\?<.\{-}Hex\s*', 'U+', 'g')
    let c=substitute(c, ',\s*Octal\s*\d*', '', 'g')
    call add(chars, c)
  endfor
  return chars
endfunc

func Test_arabic_toggle()
  set arabic
  call assert_equal(1, &rightleft)
  call assert_equal(1, &arabicshape)
  call assert_equal('arabic', &keymap)
  call assert_equal(1, &delcombine)

  set iminsert=1 imsearch=1
  set arabic&
  call assert_equal(0, &rightleft)
  call assert_equal(1, &arabicshape)
  call assert_equal('arabic', &keymap)
  call assert_equal(1, &delcombine)
  call assert_equal(0, &iminsert)
  call assert_equal(-1, &imsearch)

  set arabicshape& keymap= delcombine&
endfunc

func Test_arabic_input()
  new
  set arabic
  " Typing sghl in Arabic insert mode should show the
  " Arabic word 'Salaam' i.e. 'peace', spelled:
  " SEEN, LAM, ALEF, MEEM.
  " See: https://www.mediawiki.org/wiki/VisualEditor/Typing/Right-to-left
  call feedkeys('isghl!', 'tx')
  call assert_match("^ *!\uFEE1\uFEFC\uFEB3$", ScreenLines(1, &columns)[0])
  call assert_equal([
  \ 'U+0633',
  \ 'U+0644 U+0627',
  \ 'U+0645',
  \ 'U+21'], s:get_chars(1))

  " Without shaping, it should give individual Arabic letters.
  set noarabicshape
  call assert_match("^ *!\u0645\u0627\u0644\u0633$", ScreenLines(1, &columns)[0])
  call assert_equal([
  \ 'U+0633',
  \ 'U+0644',
  \ 'U+0627',
  \ 'U+0645',
  \ 'U+21'], s:get_chars(1))

  set arabic& arabicshape&
  bwipe!
endfunc

func Test_arabic_toggle_keymap()
  new
  set arabic
  call feedkeys("i12\<C-^>12\<C-^>12", 'tx')
  call assert_match("^ *٢١21٢١$", ScreenLines(1, &columns)[0])
  call assert_equal('١٢12١٢', getline('.'))
  set arabic&
  bwipe!
endfunc

func Test_delcombine()
  new
  set arabic
  call feedkeys("isghl\<BS>\<BS>", 'tx')
  call assert_match("^ *\uFEDE\uFEB3$", ScreenLines(1, &columns)[0])
  call assert_equal(['U+0633', 'U+0644'], s:get_chars(1))

  " Now the same with 'nodelcombine'
  set nodelcombine
  %d
  call feedkeys("isghl\<BS>\<BS>", 'tx')
  call assert_match("^ *\uFEB1$", ScreenLines(1, &columns)[0])
  call assert_equal(['U+0633'], s:get_chars(1))
  set arabic&
  bwipe!
endfunc
