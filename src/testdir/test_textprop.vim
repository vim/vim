" Tests for defining text property types and adding text properties to the
" buffer.

if !has('textprop')
  finish
endif

source screendump.vim

" test length zero

func Test_proptype_global()
  call prop_type_add('comment', {'highlight': 'Directory', 'priority': 123, 'start_incl': 1, 'end_incl': 1})
  let proptypes = prop_type_list()
  call assert_equal(1, len(proptypes))
  call assert_equal('comment', proptypes[0])

  let proptype = prop_type_get('comment')
  call assert_equal('Directory', proptype['highlight'])
  call assert_equal(123, proptype['priority'])
  call assert_equal(1, proptype['start_incl'])
  call assert_equal(1, proptype['end_incl'])

  call prop_type_delete('comment')
  call assert_equal(0, len(prop_type_list()))

  call prop_type_add('one', {})
  call assert_equal(1, len(prop_type_list()))
  let proptype = prop_type_get('one')
  call assert_false(has_key(proptype, 'highlight'))
  call assert_equal(0, proptype['priority'])
  call assert_equal(0, proptype['start_incl'])
  call assert_equal(0, proptype['end_incl'])

  call prop_type_add('two', {})
  call assert_equal(2, len(prop_type_list()))
  call prop_type_delete('one')
  call assert_equal(1, len(prop_type_list()))
  call prop_type_delete('two')
  call assert_equal(0, len(prop_type_list()))
endfunc

func Test_proptype_buf()
  let bufnr = bufnr('')
  call prop_type_add('comment', {'bufnr': bufnr, 'highlight': 'Directory', 'priority': 123, 'start_incl': 1, 'end_incl': 1})
  let proptypes = prop_type_list({'bufnr': bufnr})
  call assert_equal(1, len(proptypes))
  call assert_equal('comment', proptypes[0])

  let proptype = prop_type_get('comment', {'bufnr': bufnr})
  call assert_equal('Directory', proptype['highlight'])
  call assert_equal(123, proptype['priority'])
  call assert_equal(1, proptype['start_incl'])
  call assert_equal(1, proptype['end_incl'])

  call prop_type_delete('comment', {'bufnr': bufnr})
  call assert_equal(0, len(prop_type_list({'bufnr': bufnr})))

  call prop_type_add('one', {'bufnr': bufnr})
  let proptype = prop_type_get('one', {'bufnr': bufnr})
  call assert_false(has_key(proptype, 'highlight'))
  call assert_equal(0, proptype['priority'])
  call assert_equal(0, proptype['start_incl'])
  call assert_equal(0, proptype['end_incl'])

  call prop_type_add('two', {'bufnr': bufnr})
  call assert_equal(2, len(prop_type_list({'bufnr': bufnr})))
  call prop_type_delete('one', {'bufnr': bufnr})
  call assert_equal(1, len(prop_type_list({'bufnr': bufnr})))
  call prop_type_delete('two', {'bufnr': bufnr})
  call assert_equal(0, len(prop_type_list({'bufnr': bufnr})))
endfunc

func AddPropTypes()
  call prop_type_add('one', {})
  call prop_type_add('two', {})
  call prop_type_add('three', {})
  call prop_type_add('whole', {})
endfunc

func DeletePropTypes()
  call prop_type_delete('one')
  call prop_type_delete('two')
  call prop_type_delete('three')
  call prop_type_delete('whole')
endfunc

func SetupPropsInFirstLine()
  call setline(1, 'one two three')
  call prop_add(1, 1, {'length': 3, 'id': 11, 'type': 'one'})
  call prop_add(1, 5, {'length': 3, 'id': 12, 'type': 'two'})
  call prop_add(1, 9, {'length': 5, 'id': 13, 'type': 'three'})
  call prop_add(1, 1, {'length': 13, 'id': 14, 'type': 'whole'})
endfunc

func Get_expected_props()
  return [
      \ {'col': 1, 'length': 13, 'id': 14, 'type': 'whole', 'start': 1, 'end': 1},
      \ {'col': 1, 'length': 3, 'id': 11, 'type': 'one', 'start': 1, 'end': 1},
      \ {'col': 5, 'length': 3, 'id': 12, 'type': 'two', 'start': 1, 'end': 1},
      \ {'col': 9, 'length': 5, 'id': 13, 'type': 'three', 'start': 1, 'end': 1},
      \ ]
endfunc

func Test_prop_add()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  let expected_props = Get_expected_props()
  call assert_equal(expected_props, prop_list(1))
  call assert_fails("call prop_add(10, 1, {'length': 1, 'id': 14, 'type': 'whole'})", 'E966:')
  call assert_fails("call prop_add(1, 22, {'length': 1, 'id': 14, 'type': 'whole'})", 'E964:')
 
  " Insert a line above, text props must still be there.
  call append(0, 'empty')
  call assert_equal(expected_props, prop_list(2))
  " Delete a line above, text props must still be there.
  1del
  call assert_equal(expected_props, prop_list(1))

  " Prop without length or end column is zero length
  call prop_clear(1)
  call prop_add(1, 5, {'type': 'two'})
  let expected = [{'col': 5, 'length': 0, 'type': 'two', 'id': 0, 'start': 1, 'end': 1}]
  call assert_equal(expected, prop_list(1))

  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_remove()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  let props = Get_expected_props()
  call assert_equal(props, prop_list(1))

  " remove by id
  call prop_remove({'id': 12}, 1)
  unlet props[2]
  call assert_equal(props, prop_list(1))

  " remove by type
  call prop_remove({'type': 'one'}, 1)
  unlet props[1]
  call assert_equal(props, prop_list(1))

  call DeletePropTypes()
  bwipe!
endfunc

func SetupOneLine()
  call setline(1, 'xonex xtwoxx')
  call AddPropTypes()
  call prop_add(1, 2, {'length': 3, 'id': 11, 'type': 'one'})
  call prop_add(1, 8, {'length': 3, 'id': 12, 'type': 'two'})
  let expected = [
	\ {'col': 2, 'length': 3, 'id': 11, 'type': 'one', 'start': 1, 'end': 1},
	\ {'col': 8, 'length': 3, 'id': 12, 'type': 'two', 'start': 1, 'end': 1},
	\]
  call assert_equal(expected, prop_list(1))
  return expected
endfunc

func Test_prop_add_remove_buf()
  new
  let bufnr = bufnr('')
  call AddPropTypes()
  call setline(1, 'one two three')
  wincmd w
  call prop_add(1, 1, {'length': 3, 'id': 11, 'type': 'one', 'bufnr': bufnr})
  call prop_add(1, 5, {'length': 3, 'id': 12, 'type': 'two', 'bufnr': bufnr})
  call prop_add(1, 11, {'length': 3, 'id': 13, 'type': 'three', 'bufnr': bufnr})
  
  let props = [
	\ {'col': 1, 'length': 3, 'id': 11, 'type': 'one', 'start': 1, 'end': 1},
	\ {'col': 5, 'length': 3, 'id': 12, 'type': 'two', 'start': 1, 'end': 1},
	\ {'col': 11, 'length': 3, 'id': 13, 'type': 'three', 'start': 1, 'end': 1},
	\]
  call assert_equal(props, prop_list(1, {'bufnr': bufnr}))
 
  " remove by id
  call prop_remove({'id': 12, 'bufnr': bufnr}, 1)
  unlet props[1]
  call assert_equal(props, prop_list(1, {'bufnr': bufnr}))

  " remove by type
  call prop_remove({'type': 'one', 'bufnr': bufnr}, 1)
  unlet props[0]
  call assert_equal(props, prop_list(1, {'bufnr': bufnr}))

  call DeletePropTypes()
  wincmd w
  bwipe!
endfunc

func Test_prop_backspace()
  new
  set bs=2
  let expected = SetupOneLine() " 'xonex xtwoxx'

  exe "normal 0li\<BS>\<Esc>fxli\<BS>\<Esc>"
  call assert_equal('one xtwoxx', getline(1))
  let expected[0].col = 1
  let expected[1].col = 6
  call assert_equal(expected, prop_list(1))

  call DeletePropTypes()
  bwipe!
  set bs&
endfunc

func Test_prop_replace()
  new
  set bs=2
  let expected = SetupOneLine() " 'xonex xtwoxx'

  exe "normal 0Ryyy\<Esc>"
  call assert_equal('yyyex xtwoxx', getline(1))
  call assert_equal(expected, prop_list(1))

  exe "normal ftRyy\<BS>"
  call assert_equal('yyyex xywoxx', getline(1))
  call assert_equal(expected, prop_list(1))

  exe "normal 0fwRyy\<BS>"
  call assert_equal('yyyex xyyoxx', getline(1))
  call assert_equal(expected, prop_list(1))

  exe "normal 0foRyy\<BS>\<BS>"
  call assert_equal('yyyex xyyoxx', getline(1))
  call assert_equal(expected, prop_list(1))

  call DeletePropTypes()
  bwipe!
  set bs&
endfunc

func Test_prop_clear()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  call assert_equal(Get_expected_props(), prop_list(1))

  call prop_clear(1)
  call assert_equal([], prop_list(1))

  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_clear_buf()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  let bufnr = bufnr('')
  wincmd w
  call assert_equal(Get_expected_props(), prop_list(1, {'bufnr': bufnr}))

  call prop_clear(1, 1, {'bufnr': bufnr})
  call assert_equal([], prop_list(1, {'bufnr': bufnr}))

  wincmd w
  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_setline()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  call assert_equal(Get_expected_props(), prop_list(1))

  call setline(1, 'foobar')
  call assert_equal([], prop_list(1))

  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_setbufline()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  let bufnr = bufnr('')
  wincmd w
  call assert_equal(Get_expected_props(), prop_list(1, {'bufnr': bufnr}))

  call setbufline(bufnr, 1, 'foobar')
  call assert_equal([], prop_list(1, {'bufnr': bufnr}))

  wincmd w
  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_substitute()
  new
  " Set first line to 'one two three'
  call AddPropTypes()
  call SetupPropsInFirstLine()
  let expected_props = Get_expected_props()
  call assert_equal(expected_props, prop_list(1))

  " Change "n" in "one" to XX: 'oXXe two three'
  s/n/XX/
  let expected_props[0].length += 1
  let expected_props[1].length += 1
  let expected_props[2].col += 1
  let expected_props[3].col += 1
  call assert_equal(expected_props, prop_list(1))

  " Delete "t" in "two" and "three" to XX: 'oXXe wo hree'
  s/t//g
  let expected_props[0].length -= 2
  let expected_props[2].length -= 1
  let expected_props[3].length -= 1
  let expected_props[3].col -= 1
  call assert_equal(expected_props, prop_list(1))

  " Split the line by changing w to line break: 'oXXe ', 'o hree'
  " The long prop is split and spans both lines.
  " The props on "two" and "three" move to the next line.
  s/w/\r/
  let new_props = [
	\ copy(expected_props[0]),
	\ copy(expected_props[2]),
	\ copy(expected_props[3]),
	\ ]
  let expected_props[0].length = 5
  unlet expected_props[3]
  unlet expected_props[2]
  call assert_equal(expected_props, prop_list(1))

  let new_props[0].length = 6
  let new_props[1].col = 1
  let new_props[1].length = 1
  let new_props[2].col = 3
  call assert_equal(new_props, prop_list(2))

  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_change_indent()
  call prop_type_add('comment', {'highlight': 'Directory'})
  new
  call setline(1, ['    xxx', 'yyyyy'])
  call prop_add(2, 2, {'length': 2, 'type': 'comment'})
  let expect = {'col': 2, 'length': 2, 'type': 'comment', 'start': 1, 'end': 1, 'id': 0}
  call assert_equal([expect], prop_list(2))

  set shiftwidth=3
  normal 2G>>
  call assert_equal('   yyyyy', getline(2))
  let expect.col += 3
  call assert_equal([expect], prop_list(2))

  normal 2G==
  call assert_equal('    yyyyy', getline(2))
  let expect.col = 6
  call assert_equal([expect], prop_list(2))

  call prop_clear(2)
  call prop_add(2, 2, {'length': 5, 'type': 'comment'})
  let expect.col = 2
  let expect.length = 5
  call assert_equal([expect], prop_list(2))

  normal 2G<<
  call assert_equal(' yyyyy', getline(2))
  let expect.length = 2
  call assert_equal([expect], prop_list(2))

  set shiftwidth&
  call prop_type_delete('comment')
endfunc

" Setup a three line prop in lines 2 - 4.
" Add short props in line 1 and 5.
func Setup_three_line_prop()
  new
  call setline(1, ['one', 'twotwo', 'three', 'fourfour', 'five'])
  call prop_add(1, 2, {'length': 1, 'type': 'comment'})
  call prop_add(2, 4, {'end_lnum': 4, 'end_col': 5, 'type': 'comment'})
  call prop_add(5, 2, {'length': 1, 'type': 'comment'})
endfunc

func Test_prop_multiline()
  call prop_type_add('comment', {'highlight': 'Directory'})
  new
  call setline(1, ['xxxxxxx', 'yyyyyyyyy', 'zzzzzzzz'])

  " start halfway line 1, end halfway line 3
  call prop_add(1, 3, {'end_lnum': 3, 'end_col': 5, 'type': 'comment'})
  let expect1 = {'col': 3, 'length': 6, 'type': 'comment', 'start': 1, 'end': 0, 'id': 0}
  call assert_equal([expect1], prop_list(1))
  let expect2 = {'col': 1, 'length': 10, 'type': 'comment', 'start': 0, 'end': 0, 'id': 0}
  call assert_equal([expect2], prop_list(2))
  let expect3 = {'col': 1, 'length': 4, 'type': 'comment', 'start': 0, 'end': 1, 'id': 0}
  call assert_equal([expect3], prop_list(3))
  call prop_clear(1, 3)

  " include all three lines
  call prop_add(1, 1, {'end_lnum': 3, 'end_col': 999, 'type': 'comment'})
  let expect1.col = 1
  let expect1.length = 8
  call assert_equal([expect1], prop_list(1))
  call assert_equal([expect2], prop_list(2))
  let expect3.length = 9
  call assert_equal([expect3], prop_list(3))
  call prop_clear(1, 3)

  bwipe!

  " Test deleting the first line of a multi-line prop.
  call Setup_three_line_prop()
  let expect_short = {'col': 2, 'length': 1, 'type': 'comment', 'start': 1, 'end': 1, 'id': 0}
  call assert_equal([expect_short], prop_list(1))
  let expect2 = {'col': 4, 'length': 4, 'type': 'comment', 'start': 1, 'end': 0, 'id': 0}
  call assert_equal([expect2], prop_list(2))
  2del
  call assert_equal([expect_short], prop_list(1))
  let expect2 = {'col': 1, 'length': 6, 'type': 'comment', 'start': 1, 'end': 0, 'id': 0}
  call assert_equal([expect2], prop_list(2))
  bwipe!

  " Test deleting the last line of a multi-line prop.
  call Setup_three_line_prop()
  let expect3 = {'col': 1, 'length': 6, 'type': 'comment', 'start': 0, 'end': 0, 'id': 0}
  call assert_equal([expect3], prop_list(3))
  let expect4 = {'col': 1, 'length': 4, 'type': 'comment', 'start': 0, 'end': 1, 'id': 0}
  call assert_equal([expect4], prop_list(4))
  4del
  let expect3.end = 1
  call assert_equal([expect3], prop_list(3))
  call assert_equal([expect_short], prop_list(4))
  bwipe!

  " Test appending a line below the multi-line text prop start.
  call Setup_three_line_prop()
  let expect2 = {'col': 4, 'length': 4, 'type': 'comment', 'start': 1, 'end': 0, 'id': 0}
  call assert_equal([expect2], prop_list(2))
  call append(2, "new line")
  call assert_equal([expect2], prop_list(2))
  let expect3 = {'col': 1, 'length': 9, 'type': 'comment', 'start': 0, 'end': 0, 'id': 0}
  call assert_equal([expect3], prop_list(3))
  bwipe!

  call prop_type_delete('comment')
endfunc

func Test_prop_byteoff()
  call prop_type_add('comment', {'highlight': 'Directory'})
  new
  call setline(1, ['line1', 'second line', ''])
  set ff=unix
  call assert_equal(19, line2byte(3))
  call prop_add(1, 1, {'end_col': 3, 'type': 'comment'})
  call assert_equal(19, line2byte(3))

  bwipe!
  call prop_type_delete('comment')
endfunc

func Test_prop_undo()
  new
  call prop_type_add('comment', {'highlight': 'Directory'})
  call setline(1, ['oneone', 'twotwo', 'three'])
  " Set 'undolevels' to break changes into undo-able pieces.
  set ul&

  call prop_add(1, 3, {'end_col': 5, 'type': 'comment'})
  let expected = [{'col': 3, 'length': 2, 'id': 0, 'type': 'comment', 'start': 1, 'end': 1} ]
  call assert_equal(expected, prop_list(1))

  " Insert a character, then undo.
  exe "normal 0lllix\<Esc>"
  set ul&
  let expected[0].length = 3
  call assert_equal(expected, prop_list(1))
  undo
  let expected[0].length = 2
  call assert_equal(expected, prop_list(1))

  " Delete a character, then undo
  exe "normal 0lllx"
  set ul&
  let expected[0].length = 1
  call assert_equal(expected, prop_list(1))
  undo
  let expected[0].length = 2
  call assert_equal(expected, prop_list(1))

  " Delete the line, then undo
  1d
  set ul&
  call assert_equal([], prop_list(1))
  undo
  call assert_equal(expected, prop_list(1))

  " Insert a character, delete two characters, then undo with "U"
  exe "normal 0lllix\<Esc>"
  set ul&
  let expected[0].length = 3
  call assert_equal(expected, prop_list(1))
  exe "normal 0lllxx"
  set ul&
  let expected[0].length = 1
  call assert_equal(expected, prop_list(1))
  normal U
  let expected[0].length = 2
  call assert_equal(expected, prop_list(1))

  bwipe!
  call prop_type_delete('comment')
endfunc

" screenshot test with textprop highlighting
funct Test_textprop_screenshots()
  if !CanRunVimInTerminal() || &encoding != 'utf-8'
    return
  endif
  call writefile([
	\ "call setline(1, ['One two', 'Numbér 123 änd thœn 4¾7.', '--aa--bb--cc--dd--'])",
	\ "hi NumberProp ctermfg=blue",
	\ "hi LongProp ctermbg=yellow",
	\ "call prop_type_add('number', {'highlight': 'NumberProp'})",
	\ "call prop_type_add('long', {'highlight': 'LongProp'})",
	\ "call prop_type_add('start', {'highlight': 'NumberProp', 'start_incl': 1})",
	\ "call prop_type_add('end', {'highlight': 'NumberProp', 'end_incl': 1})",
	\ "call prop_type_add('both', {'highlight': 'NumberProp', 'start_incl': 1, 'end_incl': 1})",
	\ "call prop_add(1, 4, {'end_lnum': 3, 'end_col': 3, 'type': 'long'})",
	\ "call prop_add(2, 9, {'length': 3, 'type': 'number'})",
	\ "call prop_add(2, 24, {'length': 4, 'type': 'number'})",
	\ "call prop_add(3, 3, {'length': 2, 'type': 'number'})",
	\ "call prop_add(3, 7, {'length': 2, 'type': 'start'})",
	\ "call prop_add(3, 11, {'length': 2, 'type': 'end'})",
	\ "call prop_add(3, 15, {'length': 2, 'type': 'both'})",
	\ "set number",
	\ "hi clear SpellBad",
	\ "set spell",
	\ "normal 3G0llix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>",
	\ "normal 3G0lli\<BS>\<Esc>",
	\], 'XtestProp')
  let buf = RunVimInTerminal('-S XtestProp', {'rows': 6})
  call VerifyScreenDump(buf, 'Test_textprop_01', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestProp')
endfunc
