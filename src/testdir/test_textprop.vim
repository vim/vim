" Tests for defining text property types and adding text properties to the
" buffer.

if !has('textprop')
  finish
endif

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
  call prop_add(1, 8, {'length': 5, 'id': 13, 'type': 'three'})
  call prop_add(1, 1, {'length': 13, 'id': 14, 'type': 'whole'})
endfunc

let s:expected_props = [{'col': 1, 'length': 13, 'id': 14, 'type': 'whole', 'start': 1, 'end': 1},
      \ {'col': 1, 'length': 3, 'id': 11, 'type': 'one', 'start': 1, 'end': 1},
      \ {'col': 5, 'length': 3, 'id': 12, 'type': 'two', 'start': 1, 'end': 1},
      \ {'col': 8, 'length': 5, 'id': 13, 'type': 'three', 'start': 1, 'end': 1},
      \ ]

func Test_prop_add()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  call assert_equal(s:expected_props, prop_list(1))
  call assert_fails("call prop_add(10, 1, {'length': 1, 'id': 14, 'type': 'whole'})", 'E966:')
  call assert_fails("call prop_add(1, 22, {'length': 1, 'id': 14, 'type': 'whole'})", 'E964:')
 
  " Insert a line above, text props must still be there.
  call append(0, 'empty')
  call assert_equal(s:expected_props, prop_list(2))
  " Delete a line above, text props must still be there.
  1del
  call assert_equal(s:expected_props, prop_list(1))

  call DeletePropTypes()
  bwipe!
endfunc

func Test_prop_remove()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  let props = deepcopy(s:expected_props)
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


func Test_prop_clear()
  new
  call AddPropTypes()
  call SetupPropsInFirstLine()
  call assert_equal(s:expected_props, prop_list(1))

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
  call assert_equal(s:expected_props, prop_list(1, {'bufnr': bufnr}))

  call prop_clear(1, 1, {'bufnr': bufnr})
  call assert_equal([], prop_list(1, {'bufnr': bufnr}))

  wincmd w
  call DeletePropTypes()
  bwipe!
endfunc

" TODO: screenshot test with highlighting
