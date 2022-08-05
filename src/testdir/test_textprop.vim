" Tests for defining text property types and adding text properties to the
" buffer.

source check.vim
CheckFeature textprop

source screendump.vim
import './vim9.vim' as v9

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
  call prop_type_add('comment', #{bufnr: bufnr, highlight: 'Directory', priority: 123, start_incl: 1, end_incl: 1})
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

def Test_proptype_buf_list()
  new
  var bufnr = bufnr('')
  try
    prop_type_add('global', {})
    prop_type_add('local', {bufnr: bufnr})

    prop_add(1, 1, {type: 'global'})
    prop_add(1, 1, {type: 'local'})

    assert_equal([
      {type: 'local',  type_bufnr: bufnr, id: 0, col: 1, end: 1, length: 0, start: 1},
      {type: 'global', type_bufnr: 0,     id: 0, col: 1, end: 1, length: 0, start: 1},
    ], prop_list(1))
    assert_equal(
      {lnum: 1, id: 0, col: 1, type_bufnr: bufnr, end: 1, type: 'local', length: 0, start: 1},
      prop_find({lnum: 1, type: 'local'}))
    assert_equal(
      {lnum: 1, id: 0, col: 1, type_bufnr: 0, end: 1, type: 'global', length: 0, start: 1},
      prop_find({lnum: 1, type: 'global'}))

    prop_remove({type: 'global'}, 1)
    prop_remove({type: 'local'}, 1)
  finally
    prop_type_delete('global')
    prop_type_delete('local', {bufnr: bufnr})
    bwipe!
  endtry
enddef

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
      \ #{type_bufnr: 0, col: 1, length: 13, id: 14, type: 'whole', start: 1, end: 1},
      \ #{type_bufnr: 0, col: 1, length: 3,  id: 11, type: 'one',   start: 1, end: 1},
      \ #{type_bufnr: 0, col: 5, length: 3,  id: 12, type: 'two',   start: 1, end: 1},
      \ #{type_bufnr: 0, col: 9, length: 5,  id: 13, type: 'three', start: 1, end: 1},
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
    \ #{type_bufnr: 0, lnum: 1, col: 5, length: 3, id: 10, type: 'prop_name', start: 1, end: 1},
    \ #{type_bufnr: 0, lnum: 2, col: 4, id: 11, type: 'prop_name', start: 1, end: 0},
    \ #{type_bufnr: 0, lnum: 5, col: 4, length: 1, id: 12, type: 'prop_name', start: 1, end: 1}
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

  " Negative ID is possible, just like prop is not found.
  call assert_equal({}, prop_find({'id': -1}))
  call assert_equal({}, prop_find({'id': -2}))

  call prop_clear(1, 6)

  " Default ID is zero
  call prop_add(5, 4, {'type': 'prop_name', 'length': 1})
  call assert_equal(#{lnum: 5, id: 0, col: 4, type_bufnr: 0, end: 1, type: 'prop_name', length: 1, start: 1}, prop_find({'id': 0}))

  call prop_type_delete('prop_name')
  call prop_clear(1, 6)
  bwipe!
endfunc

def Test_prop_find2()
  # Multiple props per line, start on the first, should find the second.
  new
  ['the quikc bronw fox jumsp over the layz dog']->repeat(2)->setline(1)
  prop_type_add('misspell', {highlight: 'ErrorMsg'})
  for lnum in [1, 2]
    for col in [8, 14, 24, 38]
      prop_add(lnum, col, {type: 'misspell', length: 2})
    endfor
  endfor
  cursor(1, 8)
  var expected = {type_bufnr: 0, lnum: 1, id: 0, col: 14, end: 1, type: 'misspell', length: 2, start: 1}
  var result = prop_find({type: 'misspell', skipstart: true}, 'f')
  assert_equal(expected, result)

  prop_type_delete('misspell')
  bwipe!
enddef

func Test_prop_find_smaller_len_than_match_col()
  new
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call setline(1, ['xxxx', 'x'])
  call prop_add(1, 4, {'type': 'test'})
  call assert_equal(
        \ #{type_bufnr: 0, id: 0, lnum: 1, col: 4, type: 'test', length: 0, start: 1, end: 1},
        \ prop_find({'type': 'test', 'lnum': 2, 'col': 1}, 'b'))
  bwipe!
  call prop_type_delete('test')
endfunc

func Test_prop_find_with_both_option_enabled()
  " Initialize
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  let props = Get_expected_props()->map({_, v -> extend(v, {'lnum': 1})})
  " Test
  call assert_fails("call prop_find({'both': 1})", 'E968:')
  call assert_fails("call prop_find({'id': 11, 'both': 1})", 'E860:')
  call assert_fails("call prop_find({'type': 'three', 'both': 1})", 'E860:')
  call assert_equal({}, prop_find({'id': 11, 'type': 'three', 'both': 1}))
  call assert_equal({}, prop_find({'id': 130000, 'type': 'one', 'both': 1}))
  call assert_equal(props[2], prop_find({'id': 12, 'type': 'two', 'both': 1}))
  call assert_equal(props[0], prop_find({'id': 14, 'type': 'whole', 'both': 1}))
  " Clean up
  call DeletePropTypes()
  bwipe!
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
  let expected = [#{type_bufnr: 0, col: 5, length: 0, type: 'included', id: 0, start: 1, end: 1}]
  call assert_equal(expected, prop_list(1))

  " Inserting text makes the prop bigger.
  exe "normal 5|ixx\<Esc>"
  let expected = [#{type_bufnr: 0, col: 5, length: 2, type: 'included', id: 0, start: 1, end: 1}]
  call assert_equal(expected, prop_list(1))

  call assert_fails("call prop_add(1, 5, {'type': 'two', 'bufnr': 234343})", 'E158:')

  call DeletePropTypes()
  call prop_type_delete('included')
  bwipe!
endfunc

" Test for the prop_add_list() function
func Test_prop_add_list()
  new
  call AddPropTypes()
  call setline(1, ['one one one', 'two two two', 'six six six', 'ten ten ten'])
  call prop_add_list(#{type: 'one', id: 2},
        \ [[1, 1, 1, 3], [2, 5, 2, 7], [3, 6, 4, 6]])
  call assert_equal([#{id: 2, col: 1, type_bufnr: 0, end: 1, type: 'one',
        \ length: 2, start: 1}], prop_list(1))
  call assert_equal([#{id: 2, col: 5, type_bufnr: 0, end: 1, type: 'one',
        \ length: 2, start: 1}], prop_list(2))
  call assert_equal([#{id: 2, col: 6, type_bufnr: 0, end: 0, type: 'one',
        \ length: 7, start: 1}], prop_list(3))
  call assert_equal([#{id: 2, col: 1, type_bufnr: 0, end: 1, type: 'one',
        \ length: 5, start: 0}], prop_list(4))
  call assert_fails('call prop_add_list([1, 2], [[1, 1, 3]])', 'E1206:')
  call assert_fails('call prop_add_list({}, {})', 'E1211:')
  call assert_fails('call prop_add_list({}, [[1, 1, 3]])', 'E965:')
  call assert_fails('call prop_add_list(#{type: "abc"}, [[1, 1, 1, 3]])', 'E971:')
  call assert_fails('call prop_add_list(#{type: "one"}, [[]])', 'E474:')
  call assert_fails('call prop_add_list(#{type: "one"}, [[1, 1, 1, 1], {}])', 'E714:')
  call assert_fails('call prop_add_list(#{type: "one"}, [[1, 1, "a"]])', 'E474:')
  call assert_fails('call prop_add_list(#{type: "one"}, [[2, 2]])', 'E474:')
  call assert_fails('call prop_add_list(#{type: "one"}, [[1, 1, 2], [2, 2]])', 'E474:')
  call assert_fails('call prop_add_list(#{type: "one"}, [[1, 1, 1, 2], [4, 1, 5, 2]])', 'E966:')
  call assert_fails('call prop_add_list(#{type: "one"}, [[3, 1, 1, 2]])', 'E966:')
  call assert_fails('call prop_add_list(#{type: "one"}, [[2, 2, 2, 2], [3, 20, 3, 22]])', 'E964:')
  call assert_fails('eval #{type: "one"}->prop_add_list([[2, 2, 2, 2], [3, 20, 3, 22]])', 'E964:')
  call assert_fails('call prop_add_list(test_null_dict(), [[2, 2, 2]])', 'E965:')
  call assert_fails('call prop_add_list(#{type: "one"}, test_null_list())', 'E714:')
  call assert_fails('call prop_add_list(#{type: "one"}, [test_null_list()])', 'E714:')
  call DeletePropTypes()
  bw!
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
  call insert(props, #{type_bufnr: 0, col: 6, length: 2, id: 11, type: 'three', start: 1, end: 1}, 3)
  call assert_equal(props, prop_list(1))
  call assert_equal(1, prop_remove({'type': 'three', 'id': 11, 'both': 1, 'all': 1}, 1))
  unlet props[3]
  call assert_equal(props, prop_list(1))

  call assert_fails("call prop_remove({'id': 11, 'both': 1})", 'E860:')
  call assert_fails("call prop_remove({'type': 'three', 'both': 1})", 'E860:')

  call DeletePropTypes()
  bwipe!
endfunc

def Test_prop_add_vim9()
  prop_type_add('comment', {
      highlight: 'Directory',
      priority: 123,
      start_incl: true,
      end_incl: true,
      combine: false,
    })
  prop_type_delete('comment')
enddef

def Test_prop_remove_vim9()
  new
  g:AddPropTypes()
  g:SetupPropsInFirstLine()
  assert_equal(1, prop_remove({type: 'three', id: 13, both: true, all: true}))
  g:DeletePropTypes()
  bwipe!
enddef

func SetupOneLine()
  call setline(1, 'xonex xtwoxx')
  normal gg0
  call AddPropTypes()
  call prop_add(1, 2, {'length': 3, 'id': 11, 'type': 'one'})
  call prop_add(1, 8, {'length': 3, 'id': 12, 'type': 'two'})
  let expected = [
	\ #{type_bufnr: 0, col: 2, length: 3, id: 11, type: 'one', start: 1, end: 1},
	\ #{type_bufnr: 0, col: 8, length: 3, id: 12, type: 'two', start: 1, end: 1},
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
	\ #{type_bufnr: 0, col: 1, length: 3, id: 11, type: 'one', start: 1, end: 1},
	\ #{type_bufnr: 0, col: 5, length: 3, id: 12, type: 'two', start: 1, end: 1},
	\ #{type_bufnr: 0, col: 11, length: 3, id: 13, type: 'three', start: 1, end: 1},
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

func Test_prop_change()
  new
  let expected = SetupOneLine() " 'xonex xtwoxx'

  " Characterwise.
  exe "normal 7|c$\<Esc>"
  call assert_equal('xonex ', getline(1))
  call assert_equal(expected[:0], prop_list(1))
  " Linewise.
  exe "normal cc\<Esc>"
  call assert_equal('', getline(1))
  call assert_equal([], prop_list(1))

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

  " Replace three 1-byte chars with three 2-byte ones.
  exe "normal 0l3rø"
  call assert_equal('yøøøx xyyoxx', getline(1))
  let expected[0].length += 3
  let expected[1].col += 3
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
  let exp_first[0].end = 0
  call assert_equal(exp_first, prop_list(1))
  let expected[0].col = 1
  let expected[0].length = 2
  let expected[0].start = 0
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

  " split at the space character with 'ai' active, the leading space is removed
  " in the second line and the prop is shifted accordingly.
  let expected = SetupOneLine() " 'xonex xtwoxx'
  set ai
  exe "normal 6|i\<CR>\<Esc>"
  call assert_equal('xonex', getline(1))
  call assert_equal('xtwoxx', getline(2))
  let expected[1].col -= 6
  call assert_equal(expected, prop_list(1) + prop_list(2))
  set ai&
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
  let expected_props[0].end = 0
  unlet expected_props[3]
  unlet expected_props[2]
  call assert_equal(expected_props, prop_list(1))

  let new_props[0].length = 6
  let new_props[0].start = 0
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
  let expect = #{type_bufnr: 0, col: 2, length: 2, type: 'comment', start: 1, end: 1, id: 0}
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
  let expect1 = #{type_bufnr: 0, col: 3, length: 6, type: 'comment', start: 1, end: 0, id: 0}
  call assert_equal([expect1], prop_list(1))
  let expect2 = #{type_bufnr: 0, col: 1, length: 10, type: 'comment', start: 0, end: 0, id: 0}
  call assert_equal([expect2], prop_list(2))
  let expect3 = #{type_bufnr: 0, col: 1, length: 4, type: 'comment', start: 0, end: 1, id: 0}
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
  let expect_short = #{type_bufnr: 0, col: 2, length: 1, type: 'comment', start: 1, end: 1, id: 0}
  call assert_equal([expect_short], prop_list(1))
  let expect2 = #{type_bufnr: 0, col: 4, length: 4, type: 'comment', start: 1, end: 0, id: 0}
  call assert_equal([expect2], prop_list(2))
  2del
  call assert_equal([expect_short], prop_list(1))
  let expect2 = #{type_bufnr: 0, col: 1, length: 6, type: 'comment', start: 1, end: 0, id: 0}
  call assert_equal([expect2], prop_list(2))
  bwipe!

  " Test deleting the last line of a multi-line prop.
  call Setup_three_line_prop()
  let expect3 = #{type_bufnr: 0, col: 1, length: 6, type: 'comment', start: 0, end: 0, id: 0}
  call assert_equal([expect3], prop_list(3))
  let expect4 = #{type_bufnr: 0, col: 1, length: 4, type: 'comment', start: 0, end: 1, id: 0}
  call assert_equal([expect4], prop_list(4))
  4del
  let expect3.end = 1
  call assert_equal([expect3], prop_list(3))
  call assert_equal([expect_short], prop_list(4))
  bwipe!

  " Test appending a line below the multi-line text prop start.
  call Setup_three_line_prop()
  let expect2 = #{type_bufnr: 0, col: 4, length: 4, type: 'comment', start: 1, end: 0, id: 0}
  call assert_equal([expect2], prop_list(2))
  call append(2, "new line")
  call assert_equal([expect2], prop_list(2))
  let expect3 = #{type_bufnr: 0, col: 1, length: 9, type: 'comment', start: 0, end: 0, id: 0}
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

  new
  setlocal ff=unix
  call setline(1, range(500))
  call assert_equal(1491, line2byte(401))
  call prop_add(2, 1, {'type': 'comment'})
  call prop_add(222, 1, {'type': 'comment'})
  call assert_equal(1491, line2byte(401))
  call prop_remove({'type': 'comment'})
  call assert_equal(1491, line2byte(401))
  bwipe!

  new
  setlocal ff=unix
  call setline(1, range(520))
  call assert_equal(1491, line2byte(401))
  call prop_add(2, 1, {'type': 'comment'})
  call assert_equal(1491, line2byte(401))
  2delete
  call assert_equal(1489, line2byte(400))
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

func Test_prop_goto_byte()
  new
  call setline(1, '')
  call setline(2, 'two three')
  call setline(3, '')
  call setline(4, 'four five')

  call prop_type_add('testprop', {'highlight': 'Directory'})
  call search('^two')
  call prop_add(line('.'), col('.'), {
        \ 'length': len('two'),
        \ 'type':   'testprop'
        \ })

  call search('two \zsthree')
  let expected_pos = line2byte(line('.')) + col('.') - 1
  exe expected_pos .. 'goto'
  let actual_pos = line2byte(line('.')) + col('.') - 1
  eval actual_pos->assert_equal(expected_pos)

  call search('four \zsfive')
  let expected_pos = line2byte(line('.')) + col('.') - 1
  exe expected_pos .. 'goto'
  let actual_pos = line2byte(line('.')) + col('.') - 1
  eval actual_pos->assert_equal(expected_pos)

  call prop_type_delete('testprop')
  bwipe!
endfunc

func Test_prop_undo()
  new
  call prop_type_add('comment', {'highlight': 'Directory'})
  call setline(1, ['oneone', 'twotwo', 'three'])
  " Set 'undolevels' to break changes into undo-able pieces.
  set ul&

  call prop_add(1, 3, {'end_col': 5, 'type': 'comment'})
  let expected = [#{type_bufnr: 0, col: 3, length: 2, id: 0, type: 'comment', start: 1, end: 1}]
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
  let expected = [#{type_bufnr: 0, col: 12, length: 3, id: 0, type: 'comment', start: 1, end: 1} ]
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
  let expected = [#{type_bufnr: 0, col: 12, length: 3, id: 0, type: 'comment', start: 1, end: 1} ]
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
  let expected = [#{type_bufnr: 0, col: 3, length: 0, id: 0, type: 'comment', start: 1, end: 1} ]
  call assert_equal(expected, prop_list(1))

  " delete one char moves the property
  normal! x
  let expected = [#{type_bufnr: 0, col: 2, length: 0, id: 0, type: 'comment', start: 1, end: 1} ]
  call assert_equal(expected, prop_list(1))

  " delete char of the property has no effect
  normal! lx
  let expected = [#{type_bufnr: 0, col: 2, length: 0, id: 0, type: 'comment', start: 1, end: 1} ]
  call assert_equal(expected, prop_list(1))

  " delete more chars moves property to first column, is not deleted
  normal! 0xxxx
  let expected = [#{type_bufnr: 0, col: 1, length: 0, id: 0, type: 'comment', start: 1, end: 1} ]
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

func Test_textprop_hl_override()
  CheckScreendump

  let lines =<< trim END
      call setline(1, ['One one one one one', 'Two two two two two', 'Three three three three'])
      hi OverProp ctermfg=blue ctermbg=yellow
      hi CursorLine cterm=bold,underline ctermfg=red ctermbg=green
      hi Vsual ctermfg=cyan ctermbg=grey
      call prop_type_add('under', #{highlight: 'OverProp'})
      call prop_type_add('over', #{highlight: 'OverProp', override: 1})
      call prop_add(1, 5, #{type: 'under', length: 4})
      call prop_add(1, 13, #{type: 'over', length: 4})
      call prop_add(2, 5, #{type: 'under', length: 4})
      call prop_add(2, 13, #{type: 'over', length: 4})
      call prop_add(3, 5, #{type: 'under', length: 4})
      call prop_add(3, 13, #{type: 'over', length: 4})
      set cursorline
      2
  END
  call writefile(lines, 'XtestOverProp')
  let buf = RunVimInTerminal('-S XtestOverProp', {'rows': 8})
  call VerifyScreenDump(buf, 'Test_textprop_hl_override_1', {})

  call term_sendkeys(buf, "3Gllv$hh")
  call VerifyScreenDump(buf, 'Test_textprop_hl_override_2', {})
  call term_sendkeys(buf, "\<Esc>")

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestOverProp')
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

func Test_textprop_nowrap_scrolled()
  CheckScreendump

  let lines =<< trim END
       vim9script
       set nowrap
       setline(1, 'The number 123 is smaller than 4567.' .. repeat('X', &columns))
       prop_type_add('number', {'highlight': 'ErrorMsg'})
       prop_add(1, 12, {'length': 3, 'type': 'number'})
       prop_add(1, 32, {'length': 4, 'type': 'number'})
       feedkeys('gg20zl', 'nxt')
  END
  call writefile(lines, 'XtestNowrap')
  let buf = RunVimInTerminal('-S XtestNowrap', {'rows': 6})
  call VerifyScreenDump(buf, 'Test_textprop_nowrap_01', {})

  call term_sendkeys(buf, "$")
  call VerifyScreenDump(buf, 'Test_textprop_nowrap_02', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestNowrap')
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
  let expected = [
        \ #{type_bufnr: 0, id: 0, col: 13, end: 1, type: 'number', length: 3, start: 1},
        \ #{type_bufnr: 0, id: 0, col: 1,  end: 1, type: 'number', length: 3, start: 1},
        \ #{type_bufnr: 0, id: 0, col: 50, end: 1, type: 'number', length: 4, start: 1}]

  " TODO
  return
  " Add some text in between
  %s/\s\+/   /g
  call assert_equal(expected, prop_list(1) + prop_list(2) + prop_list(3))

  " remove some text
  :1s/[a-z]\{3\}//g
  let expected = [{'id': 0, 'col': 10, 'end': 1, 'type': 'number', 'length': 3, 'start': 1}]
  call assert_equal(expected, prop_list(1))
  bwipe!
endfunc

" This was causing property corruption.
func Test_proptype_substitute3()
  new
  call setline(1, ['abcxxx', 'def'])
  call prop_type_add("test", {"highlight": "Search"})
  call prop_add(1, 2, {"end_lnum": 2, "end_col": 2, "type": "test"})
  %s/x\+$//
  redraw

  call prop_type_delete('test')
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
  call assert_equal([#{type_bufnr: 0, id: 0, col: 1, end: 1, type: 'test', length: 1, start: 1}], prop_list(1))

  call feedkeys("foi\<F8>\<Esc>", "tx")
  call assert_equal('just s<F8>ome text', getline(1))
  call assert_equal([#{type_bufnr: 0, id: 0, col: 1, end: 1, type: 'test', length: 1, start: 1}], prop_list(1))

  bwipe!
  call prop_remove({'type': 'test'})
  call prop_type_delete('test')
endfunc

func Test_find_prop_later_in_line()
  new
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call setline(1, 'just some text')
  call prop_add(1, 1, {'length': 4, 'type': 'test'})
  call prop_add(1, 10, {'length': 3, 'type': 'test'})

  call assert_equal(
        \ #{type_bufnr: 0, id: 0, lnum: 1, col: 10, end: 1, type: 'test', length: 3, start: 1},
        \ prop_find(#{type: 'test', lnum: 1, col: 6}))

  bwipe!
  call prop_type_delete('test')
endfunc

func Test_find_zerowidth_prop_sol()
  new
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call setline(1, 'just some text')
  call prop_add(1, 1, {'length': 0, 'type': 'test'})

  call assert_equal(
        \ #{type_bufnr: 0, id: 0, lnum: 1, col: 1, end: 1, type: 'test', length: 0, start: 1},
        \ prop_find(#{type: 'test', lnum: 1}))

  bwipe!
  call prop_type_delete('test')
endfunc

" Test for passing invalid arguments to prop_xxx() functions
func Test_prop_func_invalid_args()
  call assert_fails('call prop_clear(1, 2, [])', 'E715:')
  call assert_fails('call prop_clear(-1, 2)', 'E16:')
  call assert_fails('call prop_find(test_null_dict())', 'E715:')
  call assert_fails('call prop_find({"bufnr" : []})', 'E730:')
  call assert_fails('call prop_find({})', 'E968:')
  call assert_fails('call prop_find({}, "x")', 'E474:')
  call assert_fails('call prop_find({"lnum" : -2})', 'E16:')
  call assert_fails('call prop_list(1, [])', 'E715:')
  call assert_fails('call prop_list(-1, {})', 'E16:')
  call assert_fails('call prop_remove([])', 'E474:')
  call assert_fails('call prop_remove({}, -2)', 'E16:')
  call assert_fails('call prop_remove({})', 'E968:')
  call assert_fails('call prop_type_add([], {})', 'E730:')
  call assert_fails("call prop_type_change('long', {'xyz' : 10})", 'E971:')
  call assert_fails("call prop_type_delete([])", 'E730:')
  call assert_fails("call prop_type_delete('xyz', [])", 'E715:')
  call assert_fails("call prop_type_get([])", 'E730:')
  call assert_fails("call prop_type_get('', [])", 'E474:')
  call assert_fails("call prop_type_list([])", 'E715:')
  call assert_fails("call prop_type_add('yyy', 'not_a_dict')", 'E715:')
  call assert_fails("call prop_add(1, 5, {'type':'missing_type', 'length':1})", 'E971:')
  call assert_fails("call prop_add(1, 5, {'type': ''})", 'E971:')
  call assert_fails('call prop_add(1, 1, 0)', 'E715:')

  new
  call setline(1, ['first', 'second'])
  call prop_type_add('xxx', {})

  call assert_fails("call prop_type_add('xxx', {})", 'E969:')
  call assert_fails("call prop_add(2, 0, {'type': 'xxx'})", 'E964:')
  call assert_fails("call prop_add(2, 3, {'type': 'xxx', 'end_lnum':1})", 'E475:')
  call assert_fails("call prop_add(2, 3, {'type': 'xxx', 'end_lnum':3})", 'E966:')
  call assert_fails("call prop_add(2, 3, {'type': 'xxx', 'length':-1})", 'E475:')
  call assert_fails("call prop_add(2, 3, {'type': 'xxx', 'end_col':0})", 'E475:')
  call assert_fails("call prop_add(2, 3, {'length':1})", 'E965:')

  call prop_type_delete('xxx')
  bwipe!
endfunc

func Test_prop_split_join()
  new
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call setline(1, 'just some text')
  call prop_add(1, 6, {'length': 4, 'type': 'test'})

  " Split in middle of "some"
  execute "normal! 8|i\<CR>"
  call assert_equal(
        \ [#{type_bufnr: 0, id: 0, col: 6, end: 0, type: 'test', length: 2, start: 1}],
        \ prop_list(1))
  call assert_equal(
        \ [#{type_bufnr: 0, id: 0, col: 1, end: 1, type: 'test', length: 2, start: 0}],
        \ prop_list(2))

  " Join the two lines back together
  normal! 1GJ
  call assert_equal([#{type_bufnr: 0, id: 0, col: 6, end: 1, type: 'test', length: 5, start: 1}], prop_list(1))

  bwipe!
  call prop_type_delete('test')
endfunc

func Test_prop_increment_decrement()
  new
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call setline(1, 'its 998 times')
  call prop_add(1, 5, {'length': 3, 'type': 'test'})

  exe "normal! 0f9\<C-A>"
  eval getline(1)->assert_equal('its 999 times')
  eval prop_list(1)->assert_equal([
        \ #{type_bufnr: 0, id: 0, col: 5, end: 1, type: 'test', length: 3, start: 1}])

  exe "normal! 0f9\<C-A>"
  eval getline(1)->assert_equal('its 1000 times')
  eval prop_list(1)->assert_equal([
        \ #{type_bufnr: 0, id: 0, col: 5, end: 1, type: 'test', length: 4, start: 1}])

  bwipe!
  call prop_type_delete('test')
endfunc

func Test_prop_block_insert()
  new
  call prop_type_add('test', {'highlight': 'ErrorMsg'})
  call setline(1, ['one ', 'two '])
  call prop_add(1, 1, {'length': 3, 'type': 'test'})
  call prop_add(2, 1, {'length': 3, 'type': 'test'})

  " insert "xx" in the first column of both lines
  exe "normal! gg0\<C-V>jIxx\<Esc>"
  eval getline(1, 2)->assert_equal(['xxone ', 'xxtwo '])
  let expected = [#{type_bufnr: 0, id: 0, col: 3, end: 1, type: 'test', length: 3, start: 1}]
  eval prop_list(1)->assert_equal(expected)
  eval prop_list(2)->assert_equal(expected)

  " insert "yy" inside the text props to make them longer
  exe "normal! gg03l\<C-V>jIyy\<Esc>"
  eval getline(1, 2)->assert_equal(['xxoyyne ', 'xxtyywo '])
  let expected[0].length = 5
  eval prop_list(1)->assert_equal(expected)
  eval prop_list(2)->assert_equal(expected)

  " insert "zz" after the text props, text props don't change
  exe "normal! gg07l\<C-V>jIzz\<Esc>"
  eval getline(1, 2)->assert_equal(['xxoyynezz ', 'xxtyywozz '])
  eval prop_list(1)->assert_equal(expected)
  eval prop_list(2)->assert_equal(expected)

  bwipe!
  call prop_type_delete('test')
endfunc

" this was causing an ml_get error because w_botline was wrong
func Test_prop_one_line_window()
  enew
  call range(2)->setline(1)
  call prop_type_add('testprop', {})
  call prop_add(1, 1, {'type': 'testprop'})
  call popup_create('popup', {'textprop': 'testprop'})
  $
  new
  wincmd _
  call feedkeys("\r", 'xt')
  redraw

  call popup_clear()
  call prop_type_delete('testprop')
  close
  bwipe!
endfunc

def Test_prop_column_zero_error()
  prop_type_add('proptype', {highlight: 'Search'})
  var caught = false
  try
    popup_create([{
            text: 'a',
            props: [{col: 0, length: 1, type: 'type'}],
     }], {})
  catch /E964:/
    caught = true
  endtry
  assert_true(caught)

  popup_clear()
  prop_type_delete('proptype')
enddef

" This was calling ml_append_int() and copy a text property from a previous
" line at the wrong moment.  Exact text length matters.
def Test_prop_splits_data_block()
  new
  var lines: list<string> = [repeat('x', 35)]->repeat(41)
			+ [repeat('!', 35)]
			+ [repeat('x', 35)]->repeat(56)
  lines->setline(1)
  prop_type_add('someprop', {highlight: 'ErrorMsg'})
  prop_add(1, 27, {end_lnum: 1, end_col: 70, type: 'someprop'})
  prop_remove({type: 'someprop'}, 1)
  prop_add(35, 22, {end_lnum: 43, end_col: 43, type: 'someprop'})
  prop_remove({type: 'someprop'}, 35, 43)
  assert_equal([], prop_list(42))

  bwipe!
  prop_type_delete('someprop')
enddef

" This was calling ml_delete_int() and try to change text properties.
def Test_prop_add_delete_line()
  new
  var a = 10
  var b = 20
  repeat([''], a)->append('$')
  prop_type_add('Test', {highlight: 'ErrorMsg'})
  for lnum in range(1, a)
    for col in range(1, b)
      prop_add(1, 1, {end_lnum: lnum, end_col: col, type: 'Test'})
    endfor
  endfor

  # check deleting lines is OK
  :5del
  :1del
  :$del

  prop_type_delete('Test')
  bwipe!
enddef

" This test is to detect a regression related to #10430. It is not an attempt
" fully cover deleting lines in the presence of multi-line properties.
def Test_delete_line_within_multiline_prop()
  new
  setline(1, '# Top.')
  append(1, ['some_text = """', 'A string.', '"""', '# Bottom.'])
  prop_type_add('Identifier', {'highlight': 'ModeMsg', 'priority': 0, 'combine': 0, 'start_incl': 0, 'end_incl': 0})
  prop_type_add('String', {'highlight': 'MoreMsg', 'priority': 0, 'combine': 0, 'start_incl': 0, 'end_incl': 0})
  prop_add(2, 1, {'type': 'Identifier', 'end_lnum': 2, 'end_col': 9})
  prop_add(2, 13, {'type': 'String', 'end_lnum': 4, 'end_col': 4})

  # The property for line 3 should extend into the previous and next lines.
  var props = prop_list(3)
  var prop = props[0]
  assert_equal(1, len(props))
  assert_equal(0, prop['start'])
  assert_equal(0, prop['end'])

  # This deletion should run without raising an exception.
  try
    :2 del
  catch
    assert_report('Line delete should have workd, but it raised an error.')
  endtry

  # The property for line 2 (was 3) should no longer extend into the previous
  # line.
  props = prop_list(2)
  prop = props[0]
  assert_equal(1, len(props))
  assert_equal(1, prop['start'], 'Property was not changed to start within the line.')

  # This deletion should run without raising an exception.
  try
    :3 del
  catch
    assert_report('Line delete should have workd, but it raised an error.')
  endtry

  # The property for line 2 (originally 3) should no longer extend into the next
  # line.
  props = prop_list(2)
  prop = props[0]
  assert_equal(1, len(props))
  assert_equal(1, prop['end'], 'Property was not changed to end within the line.')

  prop_type_delete('Identifier')
  prop_type_delete('String')
  bwip!
enddef

func Test_prop_in_linebreak()
  CheckRunVimInTerminal

  let lines =<< trim END
    set breakindent linebreak breakat+=]
    call printf('%s]%s', repeat('x', 50), repeat('x', 70))->setline(1)
    call prop_type_add('test', #{highlight: 'ErrorMsg'})
    call prop_add(1, 51, #{length: 1, type: 'test'})
  END
  call writefile(lines, 'XscriptPropLinebreak')
  let buf = RunVimInTerminal('-S XscriptPropLinebreak', #{rows: 10})
  call VerifyScreenDump(buf, 'Test_prop_linebreak', {})

  call StopVimInTerminal(buf)
  call delete('XscriptPropLinebreak')
endfunc

func Test_prop_after_tab()
  CheckRunVimInTerminal

  let lines =<< trim END
    set breakindent linebreak breakat+=]
    call setline(1, "\t[xxx]")
    call prop_type_add('test', #{highlight: 'ErrorMsg'})
    call prop_add(1, 2, #{length: 1, type: 'test'})
  END
  call writefile(lines, 'XscriptPropAfterTab')
  let buf = RunVimInTerminal('-S XscriptPropAfterTab', #{rows: 10})
  call VerifyScreenDump(buf, 'Test_prop_after_tab', {})

  call StopVimInTerminal(buf)
  call delete('XscriptPropAfterTab')
endfunc

func Test_prop_after_linebreak()
  CheckRunVimInTerminal

  let lines =<< trim END
      set linebreak wrap
      call printf('%s+(%s)', 'x'->repeat(&columns / 2), 'x'->repeat(&columns / 2))->setline(1)
      call prop_type_add('test', #{highlight: 'ErrorMsg'})
      call prop_add(1, (&columns / 2) + 2, #{length: 1, type: 'test'})
  END
  call writefile(lines, 'XscriptPropAfterLinebreak')
  let buf = RunVimInTerminal('-S XscriptPropAfterLinebreak', #{rows: 10})
  call VerifyScreenDump(buf, 'Test_prop_after_linebreak', {})

  call StopVimInTerminal(buf)
  call delete('XscriptPropAfterLinebreak')
endfunc

" Buffer number of 0 should be ignored, as if the parameter wasn't passed.
def Test_prop_bufnr_zero()
  new
  try
    var bufnr = bufnr('')
    setline(1, 'hello')
    prop_type_add('bufnr-global', {highlight: 'ErrorMsg'})
    prop_type_add('bufnr-buffer', {highlight: 'StatusLine', bufnr: bufnr})

    prop_add(1, 1, {type: 'bufnr-global', length: 1})
    prop_add(1, 2, {type: 'bufnr-buffer', length: 1})

    var list = prop_list(1)
    assert_equal([
       {id: 0, col: 1, type_bufnr: 0,         end: 1, type: 'bufnr-global', length: 1, start: 1},
       {id: 0, col: 2, type_bufnr: bufnr, end: 1, type: 'bufnr-buffer', length: 1, start: 1},
    ], list)

    assert_equal(
      {highlight: 'ErrorMsg', end_incl: 0, start_incl: 0, priority: 0, combine: 1},
      prop_type_get('bufnr-global', {bufnr: list[0].type_bufnr}))

    assert_equal(
      {highlight: 'StatusLine', end_incl: 0, start_incl: 0, priority: 0, bufnr: bufnr, combine: 1},
      prop_type_get('bufnr-buffer', {bufnr: list[1].type_bufnr}))
  finally
    bwipe!
    prop_type_delete('bufnr-global')
  endtry
enddef

" Tests for the prop_list() function
func Test_prop_list()
  let lines =<< trim END
    new
    call g:AddPropTypes()
    call setline(1, repeat([repeat('a', 60)], 10))
    call prop_add(1, 4, {'type': 'one', 'id': 5, 'end_col': 6})
    call prop_add(1, 5, {'type': 'two', 'id': 10, 'end_col': 7})
    call prop_add(3, 12, {'type': 'one', 'id': 20, 'end_col': 14})
    call prop_add(3, 13, {'type': 'two', 'id': 10, 'end_col': 15})
    call prop_add(5, 20, {'type': 'one', 'id': 10, 'end_col': 22})
    call prop_add(5, 21, {'type': 'two', 'id': 20, 'end_col': 23})
    call assert_equal([
          \ {'id': 5, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1},
          \ {'id': 10, 'col': 5, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1}], prop_list(1))
    #" text properties between a few lines
    call assert_equal([
          \ {'lnum': 3, 'id': 20, 'col': 12, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1},
          \ {'lnum': 3, 'id': 10, 'col': 13, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1},
          \ {'lnum': 5, 'id': 10, 'col': 20, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1},
          \ {'lnum': 5, 'id': 20, 'col': 21, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1}],
          \ prop_list(2, {'end_lnum': 5}))
    #" text properties across all the lines
    call assert_equal([
          \ {'lnum': 1, 'id': 5, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1},
          \ {'lnum': 3, 'id': 20, 'col': 12, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1},
          \ {'lnum': 5, 'id': 10, 'col': 20, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1}],
          \ prop_list(1, {'types': ['one'], 'end_lnum': -1}))
    #" text properties with the specified identifier
    call assert_equal([
          \ {'lnum': 3, 'id': 20, 'col': 12, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1},
          \ {'lnum': 5, 'id': 20, 'col': 21, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1}],
          \ prop_list(1, {'ids': [20], 'end_lnum': 10}))
    #" text properties of the specified type and id
    call assert_equal([
          \ {'lnum': 1, 'id': 10, 'col': 5, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1},
          \ {'lnum': 3, 'id': 10, 'col': 13, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1}],
          \ prop_list(1, {'types': ['two'], 'ids': [10], 'end_lnum': 20}))
    call assert_equal([], prop_list(1, {'ids': [40, 50], 'end_lnum': 10}))
    call assert_equal([], prop_list(6, {'end_lnum': 10}))
    call assert_equal([], prop_list(2, {'end_lnum': 2}))
    #" error cases
    call assert_fails("echo prop_list(1, {'end_lnum': -20})", 'E16:')
    call assert_fails("echo prop_list(4, {'end_lnum': 2})", 'E16:')
    call assert_fails("echo prop_list(1, {'end_lnum': '$'})", 'E889:')
    call assert_fails("echo prop_list(1, {'types': ['blue'], 'end_lnum': 10})",
          \ 'E971:')
    call assert_fails("echo prop_list(1, {'types': ['one', 'blue'],
          \ 'end_lnum': 10})", 'E971:')
    call assert_fails("echo prop_list(1, {'types': ['one', 10],
          \ 'end_lnum': 10})", 'E928:')
    call assert_fails("echo prop_list(1, {'types': ['']})", 'E971:')
    call assert_equal([], prop_list(2, {'types': []}))
    call assert_equal([], prop_list(2, {'types': test_null_list()}))
    call assert_fails("call prop_list(1, {'types': {}})", 'E714:')
    call assert_fails("call prop_list(1, {'types': 'one'})", 'E714:')
    call assert_equal([], prop_list(2, {'types': ['one'],
          \ 'ids': test_null_list()}))
    call assert_equal([], prop_list(2, {'types': ['one'], 'ids': []}))
    call assert_fails("call prop_list(1, {'types': ['one'], 'ids': {}})",
          \ 'E714:')
    call assert_fails("call prop_list(1, {'types': ['one'], 'ids': 10})",
          \ 'E714:')
    call assert_fails("call prop_list(1, {'types': ['one'], 'ids': [[]]})",
          \ 'E745:')
    call assert_fails("call prop_list(1, {'types': ['one'], 'ids': [10, []]})",
          \ 'E745:')

    #" get text properties from a non-current buffer
    wincmd w
    call assert_equal([
          \ {'lnum': 1, 'id': 5, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \ 'type': 'one', 'length': 2, 'start': 1},
          \ {'lnum': 1, 'id': 10, 'col': 5, 'type_bufnr': 0, 'end': 1,
          \ 'type': 'two', 'length': 2, 'start': 1},
          \ {'lnum': 3, 'id': 20, 'col': 12, 'type_bufnr': 0, 'end': 1,
          \ 'type': 'one', 'length': 2, 'start': 1},
          \ {'lnum': 3, 'id': 10, 'col': 13, 'type_bufnr': 0, 'end': 1,
          \ 'type': 'two', 'length': 2, 'start': 1}],
          \ prop_list(1, {'bufnr': winbufnr(1), 'end_lnum': 4}))
    wincmd w

    #" get text properties after clearing all the properties
    call prop_clear(1, line('$'))
    call assert_equal([], prop_list(1, {'end_lnum': 10}))

    call prop_add(2, 4, {'type': 'one', 'id': 5, 'end_col': 6})
    call prop_add(2, 4, {'type': 'two', 'id': 10, 'end_col': 6})
    call prop_add(2, 4, {'type': 'three', 'id': 15, 'end_col': 6})
    #" get text properties with a list of types
    call assert_equal([
          \ {'id': 10, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1},
          \ {'id': 5, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1}],
          \ prop_list(2, {'types': ['one', 'two']}))
    call assert_equal([
          \ {'id': 15, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'three', 'length': 2, 'start': 1},
          \ {'id': 5, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1}],
          \ prop_list(2, {'types': ['one', 'three']}))
    #" get text properties with a list of identifiers
    call assert_equal([
          \ {'id': 10, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1},
          \ {'id': 5, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1}],
          \ prop_list(2, {'ids': [5, 10, 20]}))
    call prop_clear(1, line('$'))
    call assert_equal([], prop_list(2, {'types': ['one', 'two']}))
    call assert_equal([], prop_list(2, {'ids': [5, 10, 20]}))

    #" get text properties from a hidden buffer
    edit! Xaaa
    call setline(1, repeat([repeat('b', 60)], 10))
    call prop_add(1, 4, {'type': 'one', 'id': 5, 'end_col': 6})
    call prop_add(4, 8, {'type': 'two', 'id': 10, 'end_col': 10})
    VAR bnr = bufnr()
    hide edit Xbbb
    call assert_equal([
          \ {'lnum': 1, 'id': 5, 'col': 4, 'type_bufnr': 0, 'end': 1,
          \  'type': 'one', 'length': 2, 'start': 1},
          \ {'lnum': 4, 'id': 10, 'col': 8, 'type_bufnr': 0, 'end': 1,
          \  'type': 'two', 'length': 2, 'start': 1}],
          \ prop_list(1, {'bufnr': bnr,
          \ 'types': ['one', 'two'], 'ids': [5, 10], 'end_lnum': -1}))
    #" get text properties from an unloaded buffer
    bunload! Xaaa
    call assert_equal([], prop_list(1, {'bufnr': bnr, 'end_lnum': -1}))

    call g:DeletePropTypes()
    :%bw!
  END
  call v9.CheckLegacyAndVim9Success(lines)
endfunc

func Test_prop_find_prev_on_same_line()
  new

  call setline(1, 'the quikc bronw fox jumsp over the layz dog')
  call prop_type_add('misspell', #{highlight: 'ErrorMsg'})
  for col in [8, 14, 24, 38]
    call prop_add(1, col, #{type: 'misspell', length: 2})
  endfor

  call cursor(1,18)
  let expected = [
    \ #{lnum: 1, id: 0, col: 14, end: 1, type: 'misspell', type_bufnr: 0, length: 2, start: 1},
    \ #{lnum: 1, id: 0, col: 24, end: 1, type: 'misspell', type_bufnr: 0, length: 2, start: 1}
    \ ]

  let result = prop_find(#{type: 'misspell'}, 'b')
  call assert_equal(expected[0], result)
  let result = prop_find(#{type: 'misspell'}, 'f')
  call assert_equal(expected[1], result)

  call prop_type_delete('misspell')
  bwipe!
endfunc

func Test_prop_spell()
  new
  set spell
  call AddPropTypes()

  call setline(1, ["helo world", "helo helo helo"])
  call prop_add(1, 1, #{type: 'one', length: 4})
  call prop_add(1, 6, #{type: 'two', length: 5})
  call prop_add(2, 1, #{type: 'three', length: 4})
  call prop_add(2, 6, #{type: 'three', length: 4})
  call prop_add(2, 11, #{type: 'three', length: 4})

  " The first prop over 'helo' increases its length after the word is corrected
  " to 'Hello', the second one is shifted to the right.
  let expected = [
      \ {'id': 0, 'col': 1, 'type_bufnr': 0, 'end': 1, 'type': 'one',
      \ 'length': 5, 'start': 1},
      \ {'id': 0, 'col': 7, 'type_bufnr': 0, 'end': 1, 'type': 'two',
      \ 'length': 5, 'start': 1}
      \ ]
  call feedkeys("z=1\<CR>", 'xt')

  call assert_equal('Hello world', getline(1))
  call assert_equal(expected, prop_list(1))

  " Repeat the replacement done by z=
  spellrepall

  let expected = [
      \ {'id': 0, 'col': 1, 'type_bufnr': 0, 'end': 1, 'type': 'three',
      \ 'length': 5, 'start': 1},
      \ {'id': 0, 'col': 7, 'type_bufnr': 0, 'end': 1, 'type': 'three',
      \ 'length': 5, 'start': 1},
      \ {'id': 0, 'col': 13, 'type_bufnr': 0, 'end': 1, 'type': 'three',
      \ 'length': 5, 'start': 1}
      \ ]
  call assert_equal('Hello Hello Hello', getline(2))
  call assert_equal(expected, prop_list(2))

  call DeletePropTypes()
  set spell&
  bwipe!
endfunc

func Test_prop_shift_block()
  new
  call AddPropTypes()

  call setline(1, ['some     highlighted text']->repeat(2))
  call prop_add(1, 10, #{type: 'one', length: 11})
  call prop_add(2, 10, #{type: 'two', length: 11})

  call cursor(1, 1)
  call feedkeys("5l\<c-v>>", 'nxt')
  call cursor(2, 1)
  call feedkeys("5l\<c-v><", 'nxt')

  let expected = [
      \ {'lnum': 1, 'id': 0, 'col': 8, 'type_bufnr': 0, 'end': 1, 'type': 'one',
      \ 'length': 11, 'start' : 1},
      \ {'lnum': 2, 'id': 0, 'col': 6, 'type_bufnr': 0, 'end': 1, 'type': 'two',
      \ 'length': 11, 'start' : 1}
      \ ]
  call assert_equal(expected, prop_list(1, #{end_lnum: 2}))

  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_insert_multiline()
  new
  call AddPropTypes()

  call setline(1, ['foobar', 'barbaz'])
  call prop_add(1, 4, #{end_lnum: 2, end_col: 4, type: 'one'})

  call feedkeys("1Goquxqux\<Esc>", 'nxt')
  call feedkeys("2GOquxqux\<Esc>", 'nxt')

  let lines =<< trim END
      foobar
      quxqux
      quxqux
      barbaz
  END
  call assert_equal(lines, getline(1, '$'))
  let expected = [
      \ {'lnum': 1, 'id': 0, 'col': 4, 'type_bufnr': 0, 'end': 0, 'type': 'one',
      \ 'length': 4 ,'start': 1},
      \ {'lnum': 2, 'id': 0, 'col': 1, 'type_bufnr': 0, 'end': 0, 'type': 'one',
      \ 'length': 7, 'start': 0},
      \ {'lnum': 3, 'id': 0, 'col': 1, 'type_bufnr': 0, 'end': 0, 'type': 'one',
      \ 'length': 7, 'start': 0},
      \ {'lnum': 4, 'id': 0, 'col': 1, 'type_bufnr': 0, 'end': 1, 'type': 'one',
      \ 'length': 3, 'start': 0}
      \ ]
  call assert_equal(expected, prop_list(1, #{end_lnum: 10}))

  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_blockwise_change()
  new
  call AddPropTypes()

  call setline(1, ['foooooo', 'bar', 'baaaaz'])
  call prop_add(1, 1, #{end_col: 3, type: 'one'})
  call prop_add(2, 1, #{end_col: 3, type: 'two'})
  call prop_add(3, 1, #{end_col: 3, type: 'three'})

  " Replace the first two columns with '123', since 'start_incl' is false the
  " prop is not extended.
  call feedkeys("gg\<c-v>2jc123\<Esc>", 'nxt')

  let lines =<< trim END
      123oooooo
      123ar
      123aaaaz
  END
  call assert_equal(lines, getline(1, '$'))
  let expected = [
      \ {'lnum': 1, 'id': 0, 'col': 4, 'type_bufnr': 0, 'end': 1, 'type': 'one',
      \ 'length': 1, 'start': 1},
      \ {'lnum': 2, 'id': 0, 'col': 4, 'type_bufnr': 0, 'end': 1, 'type': 'two',
      \ 'length': 1, 'start': 1},
      \ {'lnum': 3, 'id': 0, 'col': 4, 'type_bufnr': 0, 'end': 1 ,
      \ 'type': 'three', 'length': 1, 'start': 1}
      \ ]
  call assert_equal(expected, prop_list(1, #{end_lnum: 10}))

  call DeletePropTypes()
  bwipe!
endfunc

func Do_test_props_do_not_affect_byte_offsets(ff, increment)
  new
  let lcount = 410

  " File format affects byte-offset calculations, so make sure it is known.
  exec 'setlocal fileformat=' . a:ff

  " Fill the buffer with varying length lines. We need a suitably large number
  " to force Vim code through paths wehere previous error have occurred. This
  " is more 'art' than 'science'.
  let text = 'a'
  call setline(1, text)
  let offsets = [1]
  for idx in range(lcount)
      call add(offsets, offsets[idx] + len(text) + a:increment)
      if (idx % 6) == 0
          let text = text . 'a'
      endif
      call append(line('$'), text)
  endfor

  " Set a property that spans a few lines to cause Vim's internal buffer code
  " to perform a reasonable amount of rearrangement.
  call prop_type_add('one', {'highlight': 'ErrorMsg'})
  call prop_add(1, 1, {'type': 'one', 'end_lnum': 6, 'end_col': 2})

  for idx in range(lcount)
      let boff = line2byte(idx + 1)
      call assert_equal(offsets[idx], boff, 'Bad byte offset at line ' . (idx + 1))
  endfor

  call prop_type_delete('one')
  bwipe!
endfunc

func Test_props_do_not_affect_byte_offsets()
  call Do_test_props_do_not_affect_byte_offsets('unix', 1)
endfunc

func Test_props_do_not_affect_byte_offsets_dos()
  call Do_test_props_do_not_affect_byte_offsets('dos', 2)
endfunc

func Test_props_do_not_affect_byte_offsets_editline()
  new
  let lcount = 410

  " File format affects byte-offset calculations, so make sure it is known.
  setlocal fileformat=unix

  " Fill the buffer with varying length lines. We need a suitably large number
  " to force Vim code through paths wehere previous error have occurred. This
  " is more 'art' than 'science'.
  let text = 'aa'
  call setline(1, text)
  let offsets = [1]
  for idx in range(lcount)
      call add(offsets, offsets[idx] + len(text) + 1)
      if (idx % 6) == 0
          let text = text . 'a'
      endif
      call append(line('$'), text)
  endfor

  " Set a property that just covers the first line. When this test was
  " developed, this did not trigger a byte-offset error.
  call prop_type_add('one', {'highlight': 'ErrorMsg'})
  call prop_add(1, 1, {'type': 'one', 'end_lnum': 1, 'end_col': 3})

  for idx in range(lcount)
      let boff = line2byte(idx + 1)
      call assert_equal(offsets[idx], boff,
          \ 'Confounding bad byte offset at line ' . (idx + 1))
  endfor

  " Insert text in the middle of the first line, keeping the property
  " unchanged.
  :1
  normal aHello
  for idx in range(1, lcount)
      let offsets[idx] = offsets[idx] + 5
  endfor

  for idx in range(lcount)
      let boff = line2byte(idx + 1)
      call assert_equal(offsets[idx], boff,
          \ 'Bad byte offset at line ' . (idx + 1))
  endfor

  call prop_type_delete('one')
  bwipe!
endfunc

func Test_prop_inserts_text()
  CheckRunVimInTerminal

  " Just a basic check for now
  let lines =<< trim END
      call setline(1, 'insert some text here and other text there and some more text after wrapping')
      call prop_type_add('someprop', #{highlight: 'ErrorMsg'})
      call prop_type_add('otherprop', #{highlight: 'Search'})
      call prop_type_add('moreprop', #{highlight: 'DiffAdd'})
      call prop_add(1, 18, #{type: 'someprop', text: 'SOME '})
      call prop_add(1, 38, #{type: 'otherprop', text: "OTHER\t"})
      call prop_add(1, 69, #{type: 'moreprop', text: 'MORE '})
      normal $

      call setline(2, 'prepost')
      call prop_type_add('multibyte', #{highlight: 'Visual'})
      call prop_add(2, 4, #{type: 'multibyte', text: 'söme和平téxt'})

      call setline(3, '')
      call prop_add(3, 1, #{type: 'someprop', text: 'empty line'})
  END
  call writefile(lines, 'XscriptPropsWithText')
  let buf = RunVimInTerminal('-S XscriptPropsWithText', #{rows: 6, cols: 60})
  call VerifyScreenDump(buf, 'Test_prop_inserts_text_1', {})

  call term_sendkeys(buf, ":set signcolumn=yes\<CR>")
  call VerifyScreenDump(buf, 'Test_prop_inserts_text_2', {})

  call term_sendkeys(buf, "2G$")
  call VerifyScreenDump(buf, 'Test_prop_inserts_text_3', {})

  call term_sendkeys(buf, "3G")
  call VerifyScreenDump(buf, 'Test_prop_inserts_text_4', {})

  call StopVimInTerminal(buf)
  call delete('XscriptPropsWithText')
endfunc

func Test_props_with_text_after()
  CheckRunVimInTerminal

  let lines =<< trim END
      call setline(1, 'some text here and other text there')
      call prop_type_add('rightprop', #{highlight: 'ErrorMsg'})
      call prop_type_add('afterprop', #{highlight: 'Search'})
      call prop_type_add('belowprop', #{highlight: 'DiffAdd'})
      call prop_add(1, 0, #{type: 'rightprop', text: ' RIGHT ', text_align: 'right'})
      call prop_add(1, 0, #{type: 'afterprop', text: "\tAFTER\t", text_align: 'after'})
      call prop_add(1, 0, #{type: 'belowprop', text: ' BELOW ', text_align: 'below'})

      call setline(2, 'Last line.')
      call prop_add(2, 0, #{type: 'afterprop', text: ' After Last ', text_align: 'after'})
      normal G$

      call setline(3, 'right here')
      call prop_add(3, 0, #{type: 'rightprop', text: 'söme和平téxt', text_align: 'right'})
  END
  call writefile(lines, 'XscriptPropsWithTextAfter')
  let buf = RunVimInTerminal('-S XscriptPropsWithTextAfter', #{rows: 6, cols: 60})
  call VerifyScreenDump(buf, 'Test_prop_with_text_after_1', {})

  call StopVimInTerminal(buf)
  call delete('XscriptPropsWithTextAfter')
endfunc

func Test_props_with_text_after_joined()
  CheckRunVimInTerminal

  let lines =<< trim END
      call setline(1, ['one', 'two', 'three', 'four'])
      call prop_type_add('afterprop', #{highlight: 'Search'})
      call prop_add(1, 0, #{type: 'afterprop', text: ' ONE', text_align: 'after'})
      call prop_add(4, 0, #{type: 'afterprop', text: ' FOUR', text_align: 'after'})
      normal ggJ
      normal GkJ

      call setline(3, ['a', 'b', 'c', 'd', 'e', 'f'])
      call prop_add(3, 0, #{type: 'afterprop', text: ' AAA', text_align: 'after'})
      call prop_add(5, 0, #{type: 'afterprop', text: ' CCC', text_align: 'after'})
      call prop_add(7, 0, #{type: 'afterprop', text: ' EEE', text_align: 'after'})
      call prop_add(8, 0, #{type: 'afterprop', text: ' FFF', text_align: 'after'})
      normal 3G6J
  END
  call writefile(lines, 'XscriptPropsWithTextAfterJoined')
  let buf = RunVimInTerminal('-S XscriptPropsWithTextAfterJoined', #{rows: 6, cols: 60})
  call VerifyScreenDump(buf, 'Test_prop_with_text_after_joined_1', {})

  call StopVimInTerminal(buf)
  call delete('XscriptPropsWithTextAfterJoined')
endfunc

func Test_props_with_text_after_truncated()
  CheckRunVimInTerminal

  let lines =<< trim END
      call setline(1, ['one two three four five six seven'])
      call prop_type_add('afterprop', #{highlight: 'Search'})
      call prop_add(1, 0, #{type: 'afterprop', text: ' ONE and TWO and THREE and FOUR and FIVE'})

      call setline(2, ['one two three four five six seven'])
      call prop_add(2, 0, #{type: 'afterprop', text: ' one AND two AND three AND four AND five', text_align: 'right'})

      call setline(3, ['one two three four five six seven'])
      call prop_add(3, 0, #{type: 'afterprop', text: ' one AND two AND three AND four AND five lets wrap after some more text', text_align: 'below'})

      call setline(4, ['cursor here'])
      normal 4Gfh
  END
  call writefile(lines, 'XscriptPropsWithTextAfterTrunc')
  let buf = RunVimInTerminal('-S XscriptPropsWithTextAfterTrunc', #{rows: 9, cols: 60})
  call VerifyScreenDump(buf, 'Test_prop_with_text_after_trunc_1', {})

  call term_sendkeys(buf, ":37vsp\<CR>gg")
  call VerifyScreenDump(buf, 'Test_prop_with_text_after_trunc_2', {})

  call term_sendkeys(buf, ":36wincmd |\<CR>")
  call term_sendkeys(buf, "2G$")
  call VerifyScreenDump(buf, 'Test_prop_with_text_after_trunc_3', {})

  call term_sendkeys(buf, ":33wincmd |\<CR>")
  call VerifyScreenDump(buf, 'Test_prop_with_text_after_trunc_4', {})

  call term_sendkeys(buf, ":18wincmd |\<CR>")
  call term_sendkeys(buf, "0fx")
  call VerifyScreenDump(buf, 'Test_prop_with_text_after_trunc_5', {})

  call StopVimInTerminal(buf)
  call delete('XscriptPropsWithTextAfterTrunc')
endfunc

func Test_props_with_text_after_wraps()
  CheckRunVimInTerminal

  let lines =<< trim END
      call setline(1, ['one two three four five six seven'])
      call prop_type_add('afterprop', #{highlight: 'Search'})
      call prop_add(1, 0, #{type: 'afterprop', text: ' ONE and TWO and THREE and FOUR and FIVE', text_wrap: 'wrap'})

      call setline(2, ['one two three four five six seven'])
      call prop_add(2, 0, #{type: 'afterprop', text: ' one AND two AND three AND four AND five', text_align: 'right', text_wrap: 'wrap'})

      call setline(3, ['one two three four five six seven'])
      call prop_add(3, 0, #{type: 'afterprop', text: ' one AND two AND three AND four AND five lets wrap after some more text', text_align: 'below', text_wrap: 'wrap'})

      call setline(4, ['cursor here'])
      normal 4Gfh
  END
  call writefile(lines, 'XscriptPropsWithTextAfterWraps')
  let buf = RunVimInTerminal('-S XscriptPropsWithTextAfterWraps', #{rows: 9, cols: 60})
  call VerifyScreenDump(buf, 'Test_prop_with_text_after_wraps_1', {})

  call StopVimInTerminal(buf)
  call delete('XscriptPropsWithTextAfterWraps')
endfunc

func Test_removed_prop_with_text_cleans_up_array()
  new
  call setline(1, 'some text here')
  call prop_type_add('some', #{highlight: 'ErrorMsg'})
  let id1 = prop_add(1, 5, #{type: 'some', text: "SOME"})
  call assert_equal(-1, id1)
  let id2 = prop_add(1, 10, #{type: 'some', text: "HERE"})
  call assert_equal(-2, id2)

  " removing the props resets the index
  call prop_remove(#{id: id1})
  call prop_remove(#{id: id2})
  let id1 = prop_add(1, 5, #{type: 'some', text: "SOME"})
  call assert_equal(-1, id1)

  call prop_type_delete('some')
  bwipe!
endfunc

def Test_insert_text_before_virtual_text()
  new foobar
  setline(1, '12345678')
  prop_type_add('test', {highlight: 'Search'})
  prop_add(1, 5, {
    type: 'test',
    text: ' virtual text '
    })
  normal! f4axyz
  normal! f5iXYZ
  assert_equal('1234xyzXYZ5678', getline(1))

  prop_type_delete('test')
  bwipe!
enddef

" vim: shiftwidth=2 sts=2 expandtab
