" Test for textobjects

source check.vim
CheckFeature textobjects

scriptencoding utf8

func CpoM(line, useM, expected)
  new

  if a:useM
    set cpoptions+=M
  else
    set cpoptions-=M
  endif

  call setline(1, a:line)

  call setreg('"', '')
  normal! ggfrmavi)y
  call assert_equal(getreg('"'), a:expected[0])

  call setreg('"', '')
  normal! `afbmavi)y
  call assert_equal(getreg('"'), a:expected[1])

  call setreg('"', '')
  normal! `afgmavi)y
  call assert_equal(getreg('"'), a:expected[2])

  q!
endfunc

func Test_inner_block_without_cpo_M()
  call CpoM('(red \(blue) green)', 0, ['red \(blue', 'red \(blue', ''])
endfunc

func Test_inner_block_with_cpo_M_left_backslash()
  call CpoM('(red \(blue) green)', 1, ['red \(blue) green', 'blue', 'red \(blue) green'])
endfunc

func Test_inner_block_with_cpo_M_right_backslash()
  call CpoM('(red (blue\) green)', 1, ['red (blue\) green', 'blue\', 'red (blue\) green'])
endfunc

func Test_quote_selection_selection_exclusive()
  new
  call setline(1, "a 'bcde' f")
  set selection=exclusive
  exe "norm! fdvhi'y"
  call assert_equal('bcde', @")
  set selection&vim
  bw!
endfunc

func Test_quote_selection_selection_exclusive_abort()
  new
  set selection=exclusive
  call setline(1, "'abzzc'")
  let exp_curs = [0, 1, 6, 0]
  call cursor(1,1)
  exe 'norm! fcdvi"'
  " make sure to end visual mode to have a clear state
  exe "norm! \<esc>"
  call assert_equal(exp_curs, getpos('.'))
  call cursor(1,1)
  exe 'norm! fcvi"'
  exe "norm! \<esc>"
  call assert_equal(exp_curs, getpos('.'))
  call cursor(1,2)
  exe 'norm! vfcoi"'
  exe "norm! \<esc>"
  let exp_curs = [0, 1, 2, 0]
  let exp_visu = [0, 1, 7, 0]
  call assert_equal(exp_curs, getpos('.'))
  call assert_equal(exp_visu, getpos("'>"))
  set selection&vim
  bw!
endfunc

" Tests for string and html text objects
func Test_string_html_objects()
  enew!

  let t = '"wo\"rd\\" foo'
  put =t
  normal! da"
  call assert_equal('foo', getline('.'))

  let t = "'foo' 'bar' 'piep'"
  put =t
  normal! 0va'a'rx
  call assert_equal("xxxxxxxxxxxx'piep'", getline('.'))

  let t = "bla bla `quote` blah"
  put =t
  normal! 02f`da`
  call assert_equal("bla bla blah", getline('.'))

  let t = 'out " in "noXno"'
  put =t
  normal! 0fXdi"
  call assert_equal('out " in ""', getline('.'))

  let t = "\"'\" 'blah' rep 'buh'"
  put =t
  normal! 03f'vi'ry
  call assert_equal("\"'\" 'blah'yyyyy'buh'", getline('.'))

  set quoteescape=+*-
  let t = "bla `s*`d-`+++`l**` b`la"
  put =t
  normal! di`
  call assert_equal("bla `` b`la", getline('.'))

  let t = 'voo "nah" sdf " asdf" sdf " sdf" sd'
  put =t
  normal! $F"va"oha"i"rz
  call assert_equal('voo "zzzzzzzzzzzzzzzzzzzzzzzzzzzzsd', getline('.'))

  let t = "-<b>asdf<i>Xasdf</i>asdf</b>-"
  put =t
  normal! fXdit
  call assert_equal('-<b>asdf<i></i>asdf</b>-', getline('.'))

  let t = "-<b>asdX<i>a<i />sdf</i>asdf</b>-"
  put =t
  normal! 0fXdit
  call assert_equal('-<b></b>-', getline('.'))

  let t = "-<b>asdf<i>Xasdf</i>asdf</b>-"
  put =t
  normal! fXdat
  call assert_equal('-<b>asdfasdf</b>-', getline('.'))

  let t = "-<b>asdX<i>as<b />df</i>asdf</b>-"
  put =t
  normal! 0fXdat
  call assert_equal('--', getline('.'))

  let t = "-<b>\ninnertext object\n</b>"
  put =t
  normal! dit
  call assert_equal('-<b></b>', getline('.'))

  set quoteescape&
  enew!
endfunc

func Test_empty_html_tag()
  new
  call setline(1, '<div></div>')
  normal 0citxxx
  call assert_equal('<div>xxx</div>', getline(1))

  call setline(1, '<div></div>')
  normal 0f<cityyy
  call assert_equal('<div>yyy</div>', getline(1))

  call setline(1, '<div></div>')
  normal 0f<vitsaaa
  call assert_equal('aaa', getline(1))

  bwipe!
endfunc

" Tests for match() and matchstr()
func Test_match()
  call assert_equal("b", matchstr("abcd", ".", 0, 2))
  call assert_equal("bc", matchstr("abcd", "..", 0, 2))
  call assert_equal("c", matchstr("abcd", ".", 2, 0))
  call assert_equal("a", matchstr("abcd", ".", 0, -1))
  call assert_equal(-1, match("abcd", ".", 0, 5))
  call assert_equal(0 , match("abcd", ".", 0, -1))
  call assert_equal(0 , match('abc', '.', 0, 1))
  call assert_equal(1 , match('abc', '.', 0, 2))
  call assert_equal(2 , match('abc', '.', 0, 3))
  call assert_equal(-1, match('abc', '.', 0, 4))
  call assert_equal(1 , match('abc', '.', 1, 1))
  call assert_equal(2 , match('abc', '.', 2, 1))
  call assert_equal(-1, match('abc', '.', 3, 1))
  call assert_equal(3 , match('abc', '$', 0, 1))
  call assert_equal(-1, match('abc', '$', 0, 2))
  call assert_equal(3 , match('abc', '$', 1, 1))
  call assert_equal(3 , match('abc', '$', 2, 1))
  call assert_equal(3 , match('abc', '$', 3, 1))
  call assert_equal(-1, match('abc', '$', 4, 1))
  call assert_equal(0 , match('abc', '\zs', 0, 1))
  call assert_equal(1 , match('abc', '\zs', 0, 2))
  call assert_equal(2 , match('abc', '\zs', 0, 3))
  call assert_equal(3 , match('abc', '\zs', 0, 4))
  call assert_equal(-1, match('abc', '\zs', 0, 5))
  call assert_equal(1 , match('abc', '\zs', 1, 1))
  call assert_equal(2 , match('abc', '\zs', 2, 1))
  call assert_equal(3 , match('abc', '\zs', 3, 1))
  call assert_equal(-1, match('abc', '\zs', 4, 1))
endfunc

" This was causing an illegal memory access
func Test_inner_tag()
  new
  norm ixxx
  call feedkeys("v", 'xt')
  insert
x
x
.
  norm it
  q!
endfunc

func Test_sentence()
  enew!
  call setline(1, 'A sentence.  A sentence?  A sentence!')

  normal yis
  call assert_equal('A sentence.', @")
  normal yas
  call assert_equal('A sentence.  ', @")

  normal )

  normal yis
  call assert_equal('A sentence?', @")
  normal yas
  call assert_equal('A sentence?  ', @")

  normal )

  normal yis
  call assert_equal('A sentence!', @")
  normal yas
  call assert_equal('  A sentence!', @")

  normal 0
  normal 2yis
  call assert_equal('A sentence.  ', @")
  normal 3yis
  call assert_equal('A sentence.  A sentence?', @")
  normal 2yas
  call assert_equal('A sentence.  A sentence?  ', @")

  %delete _
endfunc

func Test_sentence_with_quotes()
  enew!
  call setline(1, 'A "sentence."  A sentence.')

  normal yis
  call assert_equal('A "sentence."', @")
  normal yas
  call assert_equal('A "sentence."  ', @")

  normal )

  normal yis
  call assert_equal('A sentence.', @")
  normal yas
  call assert_equal('  A sentence.', @")

  %delete _
endfunc

func Test_sentence_with_cursor_on_delimiter()
  enew!
  call setline(1, "A '([sentence.])'  A sentence.")

  normal! 15|yis
  call assert_equal("A '([sentence.])'", @")
  normal! 15|yas
  call assert_equal("A '([sentence.])'  ", @")

  normal! 16|yis
  call assert_equal("A '([sentence.])'", @")
  normal! 16|yas
  call assert_equal("A '([sentence.])'  ", @")

  normal! 17|yis
  call assert_equal("A '([sentence.])'", @")
  normal! 17|yas
  call assert_equal("A '([sentence.])'  ", @")

  %delete _
endfunc

function! Test_match_textobject()
  " Tests every combination of the following:
  " - every non-whitespace printable ASCII character (bang/33 to tilde/126)
  "   and a multi-byte
  " - im (i) or am (a)
  " - in-line (l) or entire-line (L)
  " - single line (s) or multi-line (S)
  "   that . will properly repeat

  let filler = '   '
  let testchars = map(range(33, 126), 'nr2char(v:val)')
  call add(testchars, '‽')
  for c in testchars
    for ia in ['i', 'a']
      for lL in ['l', 'L']
	for sS in ['s', 'S']
	  let surround = (lL ==# 'l') ? 'X' : ''
	  if c ==# 'X' && surround ==# 'X'
	    let surround = 'Y'
	  endif
	  if sS ==# 's'
	    let text = [surround.c.filler.c.surround]
	  else
	    let text = [surround.c.filler, filler.c.surround]
	  endif

	  let exp = (ia ==# 'i')
		\ ? (surround.c.'OK'.c.surround)
		\ : (surround.'OK'.surround)

	  " Replace text object with OK
	  call setline(line('.'), text)
	  call feedkeys('2|c'.ia.'m'.c.'OK', 'nx')
	  let l = getline('.')
	  call assert_equal(exp, l,
		\'Expected '''.exp.''' but got '''.l.''' performing c'.ia.'m'.c.'OK')

	  " Repeat with .
	  call setline(line('.'), text)
	  call feedkeys('2|.', 'nx')
	  let l = getline('.')
	  call assert_equal(exp, l,
		\'Expected '''.exp.''' but got '''.l.''' repeating c'.ia.'m'.c.'OK')
	endfor
      endfor
    endfor
  endfor
endfunction

function! Test_match_single_char_filler()
  for c in [',', '‽']
    call setline(line('.'), 'foo'.c.'i'.c.'foo')
    call feedkeys('0ficim'.c.'OK', 'nx')
    call assert_equal('foo'.c.'OK'.c.'foo', getline('.'))

    call setline(line('.'), 'foo'.c.'i'.c.'foo')
    call feedkeys('0ficam'.c.'OK', 'nx')
    call assert_equal('fooOKfoo', getline('.'))
  endfor
endfunction

function! Test_match_count()
  for c in [',', '‽']
    " Test explicit count (1im/1am)
    call setline(line('.'), 'foo'.c.'bar'.c.'i'.c.'baz'.c.'quux')
    call feedkeys('0fic1im'.c.'OK', 'nx')
    call assert_equal('foo'.c.'bar'.c.'OK'.c.'baz'.c.'quux', getline('.'))

    call setline(line('.'), 'foo'.c.'bar'.c.'i'.c.'baz'.c.'quux')
    call feedkeys('0fic1am'.c.'OK', 'nx')
    call assert_equal('foo'.c.'barOKbaz'.c.'quux', getline('.'))

    call setline(line('.'), 'foo'.c.c.'i'.c.c.'bar')
    call feedkeys('0fic1im'.c.'OK', 'nx')
    call assert_equal('foo'.c.c.'OK'.c.c.'bar', getline('.'))

    call setline(line('.'), 'foo'.c.c.'i'.c.c.'bar')
    call feedkeys('0fic1am'.c.'OK', 'nx')
    call assert_equal('foo'.c.'OK'.c.'bar', getline('.'))

    " Test count > 1
    call setline(line('.'), 'foo'.c.'bar'.c.'i'.c.'baz'.c.'quux')
    call feedkeys('0fic2im'.c.'OK', 'nx')
    call assert_equal('foo'.c.'OK'.c.'quux', getline('.'))

    call setline(line('.'), 'foo'.c.'bar'.c.'i'.c.'baz'.c.'quux')
    call feedkeys('0fic2am'.c.'OK', 'nx')
    call assert_equal('fooOKquux', getline('.'))

    call setline(line('.'), 'foo'.c.c.'i'.c.c.'bar')
    call feedkeys('0fic1im'.c.'OK', 'nx')
    call assert_equal('foo'.c.c.'OK'.c.c.'bar', getline('.'))

    call setline(line('.'), 'foo'.c.c.'i'.c.c.'bar')
    call feedkeys('0fic1am'.c.'OK', 'nx')
    call assert_equal('foo'.c.'OK'.c.'bar', getline('.'))

    " Test explicit 1 count with match at edges of line
    call setline(line('.'), c.'i'.c)
    call feedkeys('0fic1im'.c.'OK', 'nx')
    call assert_equal(c.'OK'.c, getline('.'))

    call setline(line('.'), c.'i'.c)
    call feedkeys('0fic1am'.c.'OK', 'nx')
    call assert_equal('OK', getline('.'))

    call setline(line('.'), c.c.'i'.c.c)
    call feedkeys('0fic1im'.c.'OK', 'nx')
    call assert_equal(c.c.'OK'.c.c, getline('.'))

    call setline(line('.'), c.c.'i'.c.c)
    call feedkeys('0fic1am'.c.'OK', 'nx')
    call assert_equal(c.'OK'.c, getline('.'))

    " Test count > 1 with match at edges of line
    call setline(line('.'), c.'foo'.c.'i'.c.'bar'.c)
    call feedkeys('0fic2im'.c.'OK', 'nx')
    call assert_equal(c.'OK'.c, getline('.'))

    call setline(line('.'), c.'foo'.c.'i'.c.'bar'.c)
    call feedkeys('0fic2am'.c.'OK', 'nx')
    call assert_equal('OK', getline('.'))

    call setline(line('.'), c.c.'i'.c.c)
    call feedkeys('0fic2im'.c.'OK', 'nx')
    call assert_equal(c.'OK'.c, getline('.'))

    call setline(line('.'), c.c.'i'.c.c)
    call feedkeys('0fic2am'.c.'OK', 'nx')
    call assert_equal('OK', getline('.'))
  endfor
endfunction!

function! Test_match_visual()
  for c in [',', '‽']
    " Single char between matches should immediately expand for im
    let one = 'X'.c.'foo'.c.'bar'.c.'i'.c.'baz'.c.'quux'.c.'X'
    " Two chars between characters should first select both chars for im
    let two = 'X'.c.'foo'.c.'bar'.c.'ii'.c.'baz'.c.'quux'.c.'X'

    " Basic visual selection works
    call setline(line('.'), one)
    call setreg('"', '')
    call feedkeys('0fivim'.c.'y', 'nx')
    call assert_equal('bar'.c.'i'.c.'baz', getreg('"'))

    call setreg('"', '')
    call feedkeys('0fivam'.c.'y', 'nx')
    call assert_equal(c.'i'.c, getreg('"'))

    call setline(line('.'), two)
    call setreg('"', '')
    call feedkeys('0fivim'.c.'y', 'nx')
    call assert_equal('ii', getreg('"'))

    call setreg('"', '')
    call feedkeys('0fivam'.c.'y', 'nx')
    call assert_equal(c.'ii'.c, getreg('"'))

    " Visual selection with count
    call setline(line('.'), one)
    call setreg('"', '')
    call feedkeys('0fiv2im'.c.'y', 'nx')
    call assert_equal('foo'.c.'bar'.c.'i'.c.'baz'.c.'quux', getreg('"'))

    call setreg('"', '')
    call feedkeys('0fiv2am'.c.'y', 'nx')
    call assert_equal(c.'bar'.c.'i'.c.'baz'.c, getreg('"'))

    call setline(line('.'), two)
    call setreg('"', '')
    call feedkeys('0fiv2im'.c.'y', 'nx')
    call assert_equal('bar'.c.'ii'.c.'baz', getreg('"'))

    call setreg('"', '')
    call feedkeys('0fiv2am'.c.'y', 'nx')
    call assert_equal(c.'bar'.c.'ii'.c.'baz'.c, getreg('"'))

    " Expand an existing visual selection
    call setline(line('.'), one)
    call setreg('"', '')
    call feedkeys('0fivim'.c.'im'.c.'y', 'nx')
    call assert_equal('foo'.c.'bar'.c.'i'.c.'baz'.c.'quux', getreg('"'))

    call setreg('"', '')
    call feedkeys('0fivam'.c.'am'.c.'y', 'nx')
    call assert_equal(c.'bar'.c.'i'.c.'baz'.c, getreg('"'))

    call setreg('"', '')
    call feedkeys('0fivim'.c.'am'.c.'y', 'nx')
    call assert_equal(c.'bar'.c.'i'.c.'baz'.c, getreg('"'))

    call setreg('"', '')
    call feedkeys('0fivam'.c.'im'.c.'y', 'nx')
    call assert_equal('bar'.c.'i'.c.'baz', getreg('"'))

    call setline(line('.'), two)
    call setreg('"', '')
    call feedkeys('0fivim'.c.'im'.c.'y', 'nx')
    call assert_equal('bar'.c.'ii'.c.'baz', getreg('"'))

    call setreg('"', '')
    call feedkeys('0fivam'.c.'am'.c.'y', 'nx')
    call assert_equal(c.'bar'.c.'ii'.c.'baz'.c, getreg('"'))

    call setreg('"', '')
    call feedkeys('0fivim'.c.'am'.c.'y', 'nx')
    call assert_equal(c.'ii'.c, getreg('"'))

    call setreg('"', '')
    call feedkeys('0fivam'.c.'im'.c.'y', 'nx')
    call assert_equal('bar'.c.'ii'.c.'baz', getreg('"'))
  endfor
endfunction

function! Test_match_fail()
  " Unbalanced on the right
  call setline(line('.'), 'foo,i')
  call feedkeys('0ficim,G', 'nx')
  call assert_equal('foo,i', getline('.'))
  call feedkeys('0ficam,G', 'nx')
  call assert_equal('foo,i', getline('.'))

  " Unbalanced on the left
  call setline(line('.'), 'i,foo')
  call feedkeys('0ficim,G', 'nx')
  call assert_equal('i,foo', getline('.'))
  call feedkeys('0ficam,G', 'nx')
  call assert_equal('i,foo', getline('.'))

  " Selecting too many matches
  call setline(line('.'), '1,i,1')
  call feedkeys('0fic2im,G', 'nx')
  call assert_equal('1,i,1', getline('.'))
  call feedkeys('0fic2am,G', 'nx')
  call assert_equal('1,i,1', getline('.'))
endfunction
