" Tests for defining text property types and adding text properties to the
" buffer.

source check.vim
CheckFeature textprop

source screendump.vim

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
  let proptype = 'one'->prop_type_get()
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
  call assert_equal(0, len({'bufnr': bufnr}->prop_type_list()))

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

  call assert_fails("call prop_type_add('one', {'bufnr': 98764})", "E158:")
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
  eval 1->prop_add(5, {'length': 3, 'id': 12, 'type': 'two'})
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

func Test_prop_find()
  new
  call setline(1, ['one one one', 'twotwo', 'three', 'fourfour', 'five', 'sixsix'])
 
  " Add two text props on lines 1 and 5, and one spanning lines 2 to 4. 
  call prop_type_add('prop_name', {'highlight': 'Directory'})
  call prop_add(1, 5, {'type': 'prop_name', 'id': 10, 'length': 3})
  call prop_add(2, 4, {'type': 'prop_name', 'id': 11, 'end_lnum': 4, 'end_col': 9})
  call prop_add(5, 4, {'type': 'prop_name', 'id': 12, 'length': 1})

  let expected = [
    \ {'lnum': 1, 'col': 5, 'length': 3, 'id': 10, 'type': 'prop_name', 'start': 1, 'end': 1},
    \ {'lnum': 2, 'col': 4, 'id': 11, 'type': 'prop_name', 'start': 1, 'end': 0},
    \ {'lnum': 5, 'col': 4, 'length': 1, 'id': 12, 'type': 'prop_name', 'start': 1, 'end': 1}
    \ ]

  " Starting at line 5 col 1 this should find the prop at line 5 col 4.
  call cursor(5,1)
  let result = prop_find({'type': 'prop_name'}, 'f')
  call assert_equal(expected[2], result)

  " With skipstart left at false (default), this should find the prop at line
  " 5 col 4.
  let result = prop_find({'type': 'prop_name', 'lnum': 5, 'col': 4}, 'b')
  call assert_equal(expected[2], result)

  " With skipstart set to true, this should skip the prop at line 5 col 4.
  let result = prop_find({'type': 'prop_name', 'lnum': 5, 'col': 4, 'skipstart': 1}, 'b')
  unlet result.length
  call assert_equal(expected[1], result)

  " Search backwards from line 1 col 10 to find the prop on the same line.
  let result = prop_find({'type': 'prop_name', 'lnum': 1, 'col': 10}, 'b')
  call assert_equal(expected[0], result)

  " with skipstart set to false, if the start position is anywhere between the
  " start and end lines of a text prop (searching forward or backward), the
  " result should be the prop on the first line (the line with 'start' set to 1).
  call cursor(3,1)
  let result = prop_find({'type': 'prop_name'}, 'f')
  unlet result.length
  call assert_equal(expected[1], result)
  let result = prop_find({'type': 'prop_name'}, 'b')
  unlet result.length
  call assert_equal(expected[1], result)

  " with skipstart set to true, if the start position is anywhere between the
  " start and end lines of a text prop (searching forward or backward), all lines
  " of the prop will be skipped.
  let result = prop_find({'type': 'prop_name', 'skipstart': 1}, 'b')
  call assert_equal(expected[0], result)
  let result = prop_find({'type': 'prop_name', 'skipstart': 1}, 'f')
  call assert_equal(expected[2], result)

  " Use skipstart to search through all props with type name 'prop_name'.
  " First forward...
  let lnum = 1
  let col = 1
  let i = 0
  for exp in expected
    let result = prop_find({'type': 'prop_name', 'lnum': lnum, 'col': col, 'skipstart': 1}, 'f')
    if !has_key(exp, "length")
      unlet result.length
    endif
    call assert_equal(exp, result)
    let lnum = result.lnum
    let col = result.col
    let i = i + 1
  endfor

  " ...then backwards.
  let lnum = 6
  let col = 4
  let i = 2
  while i >= 0
    let result = prop_find({'type': 'prop_name', 'lnum': lnum, 'col': col, 'skipstart': 1}, 'b')
    if !has_key(expected[i], "length")
      unlet result.length
    endif
    call assert_equal(expected[i], result)
    let lnum = result.lnum
    let col = result.col
    let i = i - 1
  endwhile 

  " Starting from line 6 col 1 search backwards for prop with id 10.
  call cursor(6,1)
  let result = prop_find({'id': 10, 'skipstart': 1}, 'b')
  call assert_equal(expected[0], result)

  " Starting from line 1 col 1 search forwards for prop with id 12.
  call cursor(1,1)
  let result = prop_find({'id': 12}, 'f')
  call assert_equal(expected[2], result)

  " Search for a prop with an unknown id.
  let result = prop_find({'id': 999}, 'f')
  call assert_equal({}, result)

  " Search backwards from the proceeding position of the prop with id 11
  " (at line num 2 col 4). This should return an empty dict.
  let result = prop_find({'id': 11, 'lnum': 2, 'col': 3}, 'b')
  call assert_equal({}, result)

  " When lnum is given and col is omitted, use column 1.
  let result = prop_find({'type': 'prop_name', 'lnum': 1}, 'f')
  call assert_equal(expected[0], result)

  call prop_clear(1,6)
  call prop_type_delete('prop_name')
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
  call prop_type_add('included', {'start_incl': 1, 'end_incl': 1})
  call prop_add(1, 5, #{type: 'included'})
  let expected = [#{col: 5, length: 0, type: 'included', id: 0, start: 1, end: 1}]
  call assert_equal(expected, prop_list(1))

  " Inserting text makes the prop bigger.
  exe "normal 5|ixx\<Esc>"
  let expected = [#{col: 5, length: 2, type: 'included', id: 0, start: 1, end: 1}]
  call assert_equal(expected, prop_list(1))

  call assert_fails("call prop_add(1, 5, {'type': 'two', 'bufnr': 234343})", 'E158:')

  call DeletePropTypes()
  call prop_type_delete('included')
  bwipe!
endfunc

func Test_prop_remove()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  let props = Get_expected_props()
  call assert_equal(props, prop_list(1))

  " remove by id
  call assert_equal(1, {'id': 12}->prop_remove(1))
  unlet props[2]
  call assert_equal(props, prop_list(1))

  " remove by type
  call assert_equal(1, prop_remove({'type': 'one'}, 1))
  unlet props[1]
  call assert_equal(props, prop_list(1))

  " remove from unknown buffer
  call assert_fails("call prop_remove({'type': 'one', 'bufnr': 123456}, 1)", 'E158:')

  call DeletePropTypes()
  bwipe!

  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  call prop_add(1, 6, {'length': 2, 'id': 11, 'type': 'three'})
  let props = Get_expected_props()
  call insert(props, {'col': 6, 'length': 2, 'id': 11, 'type': 'three', 'start': 1, 'end': 1}, 3)
  call assert_equal(props, prop_list(1))
  call assert_equal(1, prop_remove({'type': 'three', 'id': 11, 'both': 1, 'all': 1}, 1))
  unlet props[3]
  call assert_equal(props, prop_list(1))

  call assert_fails("call prop_remove({'id': 11, 'both': 1})", 'E860')
  call assert_fails("call prop_remove({'type': 'three', 'both': 1})", 'E860')

  call DeletePropTypes()
  bwipe!
endfunc

func SetupOneLine()
  call setline(1, 'xonex xtwoxx')
  normal gg0
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
  for lnum in range(1, 4)
    call setline(lnum, 'one two three')
  endfor
  wincmd w
  for lnum in range(1, 4)
    call prop_add(lnum, 1, {'length': 3, 'id': 11, 'type': 'one', 'bufnr': bufnr})
    call prop_add(lnum, 5, {'length': 3, 'id': 12, 'type': 'two', 'bufnr': bufnr})
    call prop_add(lnum, 11, {'length': 3, 'id': 13, 'type': 'three', 'bufnr': bufnr})
  endfor

  let props = [
	\ {'col': 1, 'length': 3, 'id': 11, 'type': 'one', 'start': 1, 'end': 1},
	\ {'col': 5, 'length': 3, 'id': 12, 'type': 'two', 'start': 1, 'end': 1},
	\ {'col': 11, 'length': 3, 'id': 13, 'type': 'three', 'start': 1, 'end': 1},
	\]
  call assert_equal(props, prop_list(1, {'bufnr': bufnr}))

  " remove by id
  let before_props = deepcopy(props)
  unlet props[1]

  call prop_remove({'id': 12, 'bufnr': bufnr}, 1)
  call assert_equal(props, prop_list(1, {'bufnr': bufnr}))
  call assert_equal(before_props, prop_list(2, {'bufnr': bufnr}))
  call assert_equal(before_props, prop_list(3, {'bufnr': bufnr}))
  call assert_equal(before_props, prop_list(4, {'bufnr': bufnr}))

  call prop_remove({'id': 12, 'bufnr': bufnr}, 3, 4)
  call assert_equal(props, prop_list(1, {'bufnr': bufnr}))
  call assert_equal(before_props, prop_list(2, {'bufnr': bufnr}))
  call assert_equal(props, prop_list(3, {'bufnr': bufnr}))
  call assert_equal(props, prop_list(4, {'bufnr': bufnr}))

  call prop_remove({'id': 12, 'bufnr': bufnr})
  for lnum in range(1, 4)
    call assert_equal(props, prop_list(lnum, {'bufnr': bufnr}))
  endfor

  " remove by type
  let before_props = deepcopy(props)
  unlet props[0]

  call prop_remove({'type': 'one', 'bufnr': bufnr}, 1)
  call assert_equal(props, prop_list(1, {'bufnr': bufnr}))
  call assert_equal(before_props, prop_list(2, {'bufnr': bufnr}))
  call assert_equal(before_props, prop_list(3, {'bufnr': bufnr}))
  call assert_equal(before_props, prop_list(4, {'bufnr': bufnr}))

  call prop_remove({'type': 'one', 'bufnr': bufnr}, 3, 4)
  call assert_equal(props, prop_list(1, {'bufnr': bufnr}))
  call assert_equal(before_props, prop_list(2, {'bufnr': bufnr}))
  call assert_equal(props, prop_list(3, {'bufnr': bufnr}))
  call assert_equal(props, prop_list(4, {'bufnr': bufnr}))

  call prop_remove({'type': 'one', 'bufnr': bufnr})
  for lnum in range(1, 4)
    call assert_equal(props, prop_list(lnum, {'bufnr': bufnr}))
  endfor

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

func Test_prop_open_line()
  new

  " open new line, props stay in top line
  let expected = SetupOneLine() " 'xonex xtwoxx'
  exe "normal o\<Esc>"
  call assert_equal('xonex xtwoxx', getline(1))
  call assert_equal('', getline(2))
  call assert_equal(expected, prop_list(1))
  call DeletePropTypes()

  " move all props to next line
  let expected = SetupOneLine() " 'xonex xtwoxx'
  exe "normal 0i\<CR>\<Esc>"
  call assert_equal('', getline(1))
  call assert_equal('xonex xtwoxx', getline(2))
  call assert_equal(expected, prop_list(2))
  call DeletePropTypes()

  " split just before prop, move all props to next line
  let expected = SetupOneLine() " 'xonex xtwoxx'
  exe "normal 0li\<CR>\<Esc>"
  call assert_equal('x', getline(1))
  call assert_equal('onex xtwoxx', getline(2))
  let expected[0].col -= 1
  let expected[1].col -= 1
  call assert_equal(expected, prop_list(2))
  call DeletePropTypes()

  " split inside prop, split first prop
  let expected = SetupOneLine() " 'xonex xtwoxx'
  exe "normal 0lli\<CR>\<Esc>"
  call assert_equal('xo', getline(1))
  call assert_equal('nex xtwoxx', getline(2))
  let exp_first = [deepcopy(expected[0])]
  let exp_first[0].length = 1
  call assert_equal(exp_first, prop_list(1))
  let expected[0].col = 1
  let expected[0].length = 2
  let expected[1].col -= 2
  call assert_equal(expected, prop_list(2))
  call DeletePropTypes()

  " split just after first prop, second prop move to next line
  let expected = SetupOneLine() " 'xonex xtwoxx'
  exe "normal 0fea\<CR>\<Esc>"
  call assert_equal('xone', getline(1))
  call assert_equal('x xtwoxx', getline(2))
  let exp_first = expected[0:0]
  call assert_equal(exp_first, prop_list(1))
  let expected = expected[1:1]
  let expected[0].col -= 4
  call assert_equal(expected, prop_list(2))
  call DeletePropTypes()

  bwipe!
  set bs&
endfunc

func Test_prop_clear()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  call assert_equal(Get_expected_props(), prop_list(1))

  eval 1->prop_clear()
  call assert_equal([], 1->prop_list())

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
  eval 'comment'->prop_type_add({'highlight': 'Directory'})
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

func Test_prop_line2byte()
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

func Test_prop_byte2line()
  new
  set ff=unix
  call setline(1, ['one one', 'two two', 'three three', 'four four', 'five'])
  call assert_equal(4, byte2line(line2byte(4)))
  call assert_equal(5, byte2line(line2byte(5)))

  call prop_type_add('prop', {'highlight': 'Directory'})
  call prop_add(3, 1, {'length': 5, 'type': 'prop'})
  call assert_equal(4, byte2line(line2byte(4)))
  call assert_equal(5, byte2line(line2byte(5)))

  bwipe!
  call prop_type_delete('prop')
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

  " substitute a word, then undo
  call setline(1, 'the number 123 is highlighted.')
  call prop_add(1, 12, {'length': 3, 'type': 'comment'})
  let expected = [{'col': 12, 'length': 3, 'id': 0, 'type': 'comment', 'start': 1, 'end': 1} ]
  call assert_equal(expected, prop_list(1))
  set ul&
  1s/number/foo
  let expected[0].col = 9
  call assert_equal(expected, prop_list(1))
  undo
  let expected[0].col = 12
  call assert_equal(expected, prop_list(1))
  call prop_clear(1)

  " substitute with backslash
  call setline(1, 'the number 123 is highlighted.')
  call prop_add(1, 12, {'length': 3, 'type': 'comment'})
  let expected = [{'col': 12, 'length': 3, 'id': 0, 'type': 'comment', 'start': 1, 'end': 1} ]
  call assert_equal(expected, prop_list(1))
  1s/the/\The
  call assert_equal(expected, prop_list(1))
  1s/^/\\
  let expected[0].col += 1
  call assert_equal(expected, prop_list(1))
  1s/^/\~
  let expected[0].col += 1
  call assert_equal(expected, prop_list(1))
  1s/123/12\\3
  let expected[0].length += 1
  call assert_equal(expected, prop_list(1))
  call prop_clear(1)

  bwipe!
  call prop_type_delete('comment')
endfunc

func Test_prop_delete_text()
  new
  call prop_type_add('comment', {'highlight': 'Directory'})
  call setline(1, ['oneone', 'twotwo', 'three'])

  " zero length property
  call prop_add(1, 3, {'type': 'comment'})
  let expected = [{'col': 3, 'length': 0, 'id': 0, 'type': 'comment', 'start': 1, 'end': 1} ]
  call assert_equal(expected, prop_list(1))

  " delete one char moves the property
  normal! x
  let expected = [{'col': 2, 'length': 0, 'id': 0, 'type': 'comment', 'start': 1, 'end': 1} ]
  call assert_equal(expected, prop_list(1))

  " delete char of the property has no effect
  normal! lx
  let expected = [{'col': 2, 'length': 0, 'id': 0, 'type': 'comment', 'start': 1, 'end': 1} ]
  call assert_equal(expected, prop_list(1))

  " delete more chars moves property to first column, is not deleted
  normal! 0xxxx
  let expected = [{'col': 1, 'length': 0, 'id': 0, 'type': 'comment', 'start': 1, 'end': 1} ]
  call assert_equal(expected, prop_list(1))

  bwipe!
  call prop_type_delete('comment')
endfunc

" screenshot test with textprop highlighting
func Test_textprop_screenshot_various()
  CheckScreendump
  " The Vim running in the terminal needs to use utf-8.
  if g:orig_encoding != 'utf-8'
    throw 'Skipped: not using utf-8'
  endif
  call writefile([
	\ "call setline(1, ["
	\	.. "'One two',"
	\	.. "'Numbér 123 änd thœn 4¾7.',"
	\	.. "'--aa--bb--cc--dd--',"
	\	.. "'// comment with error in it',"
	\	.. "'first line',"
	\	.. "'  second line  ',"
	\	.. "'third line',"
	\	.. "'   fourth line',"
	\	.. "])",
	\ "hi NumberProp ctermfg=blue",
	\ "hi LongProp ctermbg=yellow",
	\ "hi BackgroundProp ctermbg=lightgrey",
	\ "hi UnderlineProp cterm=underline",
	\ "call prop_type_add('number', {'highlight': 'NumberProp'})",
	\ "call prop_type_add('long', {'highlight': 'NumberProp'})",
	\ "call prop_type_change('long', {'highlight': 'LongProp'})",
	\ "call prop_type_add('start', {'highlight': 'NumberProp', 'start_incl': 1})",
	\ "call prop_type_add('end', {'highlight': 'NumberProp', 'end_incl': 1})",
	\ "call prop_type_add('both', {'highlight': 'NumberProp', 'start_incl': 1, 'end_incl': 1})",
	\ "call prop_type_add('background', {'highlight': 'BackgroundProp', 'combine': 0})",
	\ "call prop_type_add('backgroundcomb', {'highlight': 'NumberProp', 'combine': 1})",
	\ "eval 'backgroundcomb'->prop_type_change({'highlight': 'BackgroundProp'})",
	\ "call prop_type_add('error', {'highlight': 'UnderlineProp'})",
	\ "call prop_add(1, 4, {'end_lnum': 3, 'end_col': 3, 'type': 'long'})",
	\ "call prop_add(2, 9, {'length': 3, 'type': 'number'})",
	\ "call prop_add(2, 24, {'length': 4, 'type': 'number'})",
	\ "call prop_add(3, 3, {'length': 2, 'type': 'number'})",
	\ "call prop_add(3, 7, {'length': 2, 'type': 'start'})",
	\ "call prop_add(3, 11, {'length': 2, 'type': 'end'})",
	\ "call prop_add(3, 15, {'length': 2, 'type': 'both'})",
	\ "call prop_add(4, 6, {'length': 3, 'type': 'background'})",
	\ "call prop_add(4, 12, {'length': 10, 'type': 'backgroundcomb'})",
	\ "call prop_add(4, 17, {'length': 5, 'type': 'error'})",
	\ "call prop_add(5, 7, {'length': 4, 'type': 'long'})",
	\ "call prop_add(6, 1, {'length': 8, 'type': 'long'})",
	\ "call prop_add(8, 1, {'length': 1, 'type': 'long'})",
	\ "call prop_add(8, 11, {'length': 4, 'type': 'long'})",
	\ "set number cursorline",
	\ "hi clear SpellBad",
	\ "set spell",
	\ "syn match Comment '//.*'",
	\ "hi Comment ctermfg=green",
	\ "normal 3G0llix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>lllix\<Esc>",
	\ "normal 3G0lli\<BS>\<Esc>",
	\ "normal 6G0i\<BS>\<Esc>",
	\ "normal 3J",
	\ "normal 3G",
	\], 'XtestProp')
  let buf = RunVimInTerminal('-S XtestProp', {'rows': 8})
  call VerifyScreenDump(buf, 'Test_textprop_01', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestProp')
endfunc

func RunTestVisualBlock(width, dump)
  call writefile([
	\ "call setline(1, ["
	\	.. "'xxxxxxxxx 123 x',"
	\	.. "'xxxxxxxx 123 x',"
	\	.. "'xxxxxxx 123 x',"
	\	.. "'xxxxxx 123 x',"
	\	.. "'xxxxx 123 x',"
	\	.. "'xxxx 123 xx',"
	\	.. "'xxx 123 xxx',"
	\	.. "'xx 123 xxxx',"
	\	.. "'x 123 xxxxx',"
	\	.. "' 123 xxxxxx',"
	\	.. "])",
	\ "hi SearchProp ctermbg=yellow",
	\ "call prop_type_add('search', {'highlight': 'SearchProp'})",
	\ "call prop_add(1, 11, {'length': 3, 'type': 'search'})",
	\ "call prop_add(2, 10, {'length': 3, 'type': 'search'})",
	\ "call prop_add(3, 9, {'length': 3, 'type': 'search'})",
	\ "call prop_add(4, 8, {'length': 3, 'type': 'search'})",
	\ "call prop_add(5, 7, {'length': 3, 'type': 'search'})",
	\ "call prop_add(6, 6, {'length': 3, 'type': 'search'})",
	\ "call prop_add(7, 5, {'length': 3, 'type': 'search'})",
	\ "call prop_add(8, 4, {'length': 3, 'type': 'search'})",
	\ "call prop_add(9, 3, {'length': 3, 'type': 'search'})",
	\ "call prop_add(10, 2, {'length': 3, 'type': 'search'})",
	\ "normal 1G6|\<C-V>" .. repeat('l', a:width - 1) .. "10jx",
	\], 'XtestPropVis')
  let buf = RunVimInTerminal('-S XtestPropVis', {'rows': 12})
  call VerifyScreenDump(buf, 'Test_textprop_vis_' .. a:dump, {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPropVis')
endfunc

" screenshot test with Visual block mode operations
func Test_textprop_screenshot_visual()
  CheckScreendump

  " Delete two columns while text props are three chars wide.
  call RunTestVisualBlock(2, '01')

  " Same, but delete four columns
  call RunTestVisualBlock(4, '02')
endfunc

func Test_textprop_after_tab()
  CheckScreendump

  let lines =<< trim END
       call setline(1, [
             \ "\txxx",
             \ "x\txxx",
             \ ])
       hi SearchProp ctermbg=yellow
       call prop_type_add('search', {'highlight': 'SearchProp'})
       call prop_add(1, 2, {'length': 3, 'type': 'search'})
       call prop_add(2, 3, {'length': 3, 'type': 'search'})
  END
  call writefile(lines, 'XtestPropTab')
  let buf = RunVimInTerminal('-S XtestPropTab', {'rows': 6})
  call VerifyScreenDump(buf, 'Test_textprop_tab', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPropTab')
endfunc

func Test_textprop_with_syntax()
  CheckScreendump

  let lines =<< trim END
       call setline(1, [
             \ "(abc)",
             \ ])
       syn match csParens "[()]" display
       hi! link csParens MatchParen

       call prop_type_add('TPTitle', #{ highlight: 'Title' })
       call prop_add(1, 2, #{type: 'TPTitle', end_col: 5})
  END
  call writefile(lines, 'XtestPropSyn')
  let buf = RunVimInTerminal('-S XtestPropSyn', {'rows': 6})
  call VerifyScreenDump(buf, 'Test_textprop_syn_1', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPropSyn')
endfunc

" Adding a text property to a new buffer should not fail
func Test_textprop_empty_buffer()
  call prop_type_add('comment', {'highlight': 'Search'})
  new
  call prop_add(1, 1, {'type': 'comment'})
  close
  call prop_type_delete('comment')
endfunc

" Adding a text property with invalid highlight should be ignored.
func Test_textprop_invalid_highlight()
  call assert_fails("call prop_type_add('dni', {'highlight': 'DoesNotExist'})", 'E970:')
  new
  call setline(1, ['asdf','asdf'])
  call prop_add(1, 1, {'length': 4, 'type': 'dni'})
  redraw
  bwipe!
  call prop_type_delete('dni')
endfunc

" Adding a text property to an empty buffer and then editing another
func Test_textprop_empty_buffer_next()
  call prop_type_add("xxx", {})
  call prop_add(1, 1, {"type": "xxx"})
  next X
  call prop_type_delete('xxx')
endfunc

func Test_textprop_remove_from_buf()
  new
  let buf = bufnr('')
  call prop_type_add('one', {'bufnr': buf})
  call prop_add(1, 1, {'type': 'one', 'id': 234})
  file x
  edit y
  call prop_remove({'id': 234, 'bufnr': buf}, 1)
  call prop_type_delete('one', {'bufnr': buf})
  bwipe! x
  close
endfunc

func Test_textprop_in_unloaded_buf()
  edit Xaaa
  call setline(1, 'aaa')
  write
  edit Xbbb
  call setline(1, 'bbb')
  write
  let bnr = bufnr('')
  edit Xaaa

  call prop_type_add('ErrorMsg', #{highlight:'ErrorMsg'})
  call assert_fails("call prop_add(1, 1, #{end_lnum: 1, endcol: 2, type: 'ErrorMsg', bufnr: bnr})", 'E275:')
  exe 'buf ' .. bnr
  call assert_equal('bbb', getline(1))
  call assert_equal(0, prop_list(1)->len())

  bwipe! Xaaa
  bwipe! Xbbb
  cal delete('Xaaa')
  cal delete('Xbbb')
endfunc

func Test_proptype_substitute2()
  new
  " text_prop.vim
  call setline(1, [
        \ 'The   num  123 is smaller than 4567.',
        \ '123 The number 123 is smaller than 4567.',
        \ '123 The number 123 is smaller than 4567.'])

  call prop_type_add('number', {'highlight': 'ErrorMsg'})

  call prop_add(1, 12, {'length': 3, 'type': 'number'})
  call prop_add(2, 1, {'length': 3, 'type': 'number'})
  call prop_add(3, 36, {'length': 4, 'type': 'number'})
  set ul&
  let expected = [{'id': 0, 'col': 13, 'end': 1, 'type': 'number', 'length': 3, 'start': 1}, 
        \ {'id': 0, 'col': 1, 'end': 1, 'type': 'number', 'length': 3, 'start': 1}, 
        \ {'id': 0, 'col': 50, 'end': 1, 'type': 'number', 'length': 4, 'start': 1}]
  " Add some text in between
  %s/\s\+/   /g
  call assert_equal(expected, prop_list(1) + prop_list(2) + prop_list(3)) 

  " remove some text
  :1s/[a-z]\{3\}//g
  let expected = [{'id': 0, 'col': 10, 'end': 1, 'type': 'number', 'length': 3, 'start': 1}]
  call assert_equal(expected, prop_list(1))
  bwipe!
endfunc

func SaveOptions()
  let d = #{tabstop: &tabstop,
	  \ softtabstop: &softtabstop,
	  \ shiftwidth: &shiftwidth,
	  \ expandtab: &expandtab,
	  \ foldmethod: '"' .. &foldmethod .. '"',
	  \ }
  return d
endfunc

func RestoreOptions(dict)
  for name in keys(a:dict)
    exe 'let &' .. name .. ' = ' .. a:dict[name]
  endfor
endfunc

func Test_textprop_noexpandtab()
  new
  let save_dict = SaveOptions()

  set tabstop=8
  set softtabstop=4
  set shiftwidth=4
  set noexpandtab
  set foldmethod=marker

  call feedkeys("\<esc>\<esc>0Ca\<cr>\<esc>\<up>", "tx")
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call prop_add(1, 1, {'end_col': 2, 'type': 'test'})
  call feedkeys("0i\<tab>", "tx")
  call prop_remove({'type': 'test'})
  call prop_add(1, 2, {'end_col': 3, 'type': 'test'})
  call feedkeys("A\<left>\<tab>", "tx")
  call prop_remove({'type': 'test'})
  try
    " It is correct that this does not pass
    call prop_add(1, 6, {'end_col': 7, 'type': 'test'})
    " Has already collapsed here, start_col:6 does not result in an error
    call feedkeys("A\<left>\<tab>", "tx")
  catch /^Vim\%((\a\+)\)\=:E964/
  endtry
  call prop_remove({'type': 'test'})
  call prop_type_delete('test')

  call RestoreOptions(save_dict)
  bwipe!
endfunc

func Test_textprop_noexpandtab_redraw()
  new
  let save_dict = SaveOptions()

  set tabstop=8
  set softtabstop=4
  set shiftwidth=4
  set noexpandtab
  set foldmethod=marker

  call feedkeys("\<esc>\<esc>0Ca\<cr>\<space>\<esc>\<up>", "tx")
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call prop_add(1, 1, {'end_col': 2, 'type': 'test'})
  call feedkeys("0i\<tab>", "tx")
  " Internally broken at the next line
  call feedkeys("A\<left>\<tab>", "tx")
  redraw
  " Index calculation failed internally on next line
  call prop_add(1, 1, {'end_col': 2, 'type': 'test'})
  call prop_remove({'type': 'test', 'all': v:true})
  call prop_type_delete('test')
  call prop_type_delete('test')

  call RestoreOptions(save_dict)
  bwipe!
endfunc

func Test_textprop_ins_str()
  new
  call setline(1, 'just some text')
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call prop_add(1, 1, {'end_col': 2, 'type': 'test'})
  call assert_equal([{'id': 0, 'col': 1, 'end': 1, 'type': 'test', 'length': 1, 'start': 1}], prop_list(1))

  call feedkeys("foi\<F8>\<Esc>", "tx")
  call assert_equal('just s<F8>ome text', getline(1))
  call assert_equal([{'id': 0, 'col': 1, 'end': 1, 'type': 'test', 'length': 1, 'start': 1}], prop_list(1))

  bwipe!
  call prop_remove({'type': 'test'})
  call prop_type_delete('test')
endfunc
