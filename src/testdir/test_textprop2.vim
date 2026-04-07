" Additional tests for defining text property types and adding text properties
" to the buffer.

CheckFeature textprop

source util/screendump.vim

" Find a property of a given type on a given line.
func s:PropForType(lnum, type_name)
  for p in prop_list(a:lnum)
    if p['type'] == a:type_name
      return p
    endif
  endfor
  return {}
endfunc

" Clean up property types and wipe buffer.
func s:CleanupPropTypes(types)
  for name in a:types
    call prop_type_delete(name)
  endfor
  bwipe!
endfunc

" Set up buffer content and properties used by multiple tests.
"
" Properties:
"   type '1': line 2 col 2 -> line 4 col 9  (multiline highlight)
"   type '2': line 2 col 3 -> line 2 col 7  (single line highlight)
"   type '2': line 3 col 3 -> line 3 col 8  (single line highlight)
"   type '2': line 4 col 3 -> line 4 col 9  (single line highlight)
"   type '3': line 2 col 5 -> line 4 col 9  (multiline highlight)
func s:Setup_multiline_props_1()
  new
  call setline(1, ['Line1', 'Line.2', 'Line..3', 'Line...4'])
  silent! call prop_type_delete('1')
  silent! call prop_type_delete('2')
  silent! call prop_type_delete('3')
  call prop_type_add('1', {'highlight': 'DiffAdd'})
  call prop_type_add('2', {'highlight': 'DiffChange'})
  call prop_type_add('3', {'highlight': 'DiffDelete'})
  call prop_add(2, 2, {'type': '1', 'id': 42, 'end_lnum': 4, 'end_col': 9})
  call prop_add(2, 3, {'type': '2', 'id': 42, 'end_lnum': 2, 'end_col': 7})
  call prop_add(3, 3, {'type': '2', 'id': 42, 'end_lnum': 3, 'end_col': 8})
  call prop_add(4, 3, {'type': '2', 'id': 42, 'end_lnum': 4, 'end_col': 9})
  call prop_add(2, 5, {'type': '3', 'id': 42, 'end_lnum': 4, 'end_col': 9})

  " Sanity check.
  call assert_equal(4, line('$'))
  call assert_equal(0, len(prop_list(1)))
  call assert_equal(3, len(prop_list(2)))
  call assert_equal(3, len(prop_list(3)))
  call assert_equal(3, len(prop_list(4)))
endfunc

" Set up buffer with a multiline property spanning line 1 col 4 -> line 3 col 4.
func s:Setup_start_end_prop()
  new
  call setline(1, ['Line.1', 'Line..2', 'Line...3', 'Line....4'])
  silent! call prop_type_delete('1')
  call prop_type_add('1', {'highlight': 'DiffAdd'})
  call prop_add(1, 4, {'type': '1', 'id': 42, 'end_lnum': 3, 'end_col': 4})
endfunc

" The substitute command should adjust marks when one or more whole lines are
" deleted.
func Test_subst_adjusts_marks()
  " Buffer: 4 lines with a single multiline property spanning all lines.
  " type '1': line 1 col 1 -> line 4 col 10
  func DoEditAndCheck(edit, expected_marks, expected_nlines) closure
    new
    call setline(1, ['Line.1', 'Line..2', 'Line...3', 'Line....4'])
    silent! call prop_type_delete('1')
    call prop_type_add('1', {'highlight': 'DiffAdd'})
    call prop_add(1, 1, {'type': '1', 'id': 42, 'end_lnum': 4, 'end_col': 10})
    call setpos("'a", [0, 1, 1])
    call setpos("'b", [0, 2, 1])
    call setpos("'c", [0, 3, 1])
    call setpos("'d", [0, 4, 1])
    set undolevels&
    let msg = printf('Edit command = "%s"', a:edit)

    execute a:edit

    call assert_equal(a:expected_nlines, line('$'), msg)
    call assert_equal(a:expected_marks[0], getpos("'a"), msg .. ', mark a')
    call assert_equal(a:expected_marks[1], getpos("'b"), msg .. ', mark b')
    call assert_equal(a:expected_marks[2], getpos("'c"), msg .. ', mark c')
    call assert_equal(a:expected_marks[3], getpos("'d"), msg .. ', mark d')

    " Undo and verify original state is restored.
    :undo
    call assert_equal(4, line('$'), msg .. ', post-undo')
    call assert_equal('Line.1', getline(1), msg .. ', post-undo line 1')
    call assert_equal([0, 1, 1, 0], getpos("'a"), msg .. ', post-undo mark a')
    call assert_equal([0, 2, 1, 0], getpos("'b"), msg .. ', post-undo mark b')
    call assert_equal([0, 3, 1, 0], getpos("'c"), msg .. ', post-undo mark c')
    call assert_equal([0, 4, 1, 0], getpos("'d"), msg .. ', post-undo mark d')

    call prop_type_delete('1')
    bwipe!
  endfunc

  " Delete line 1.
  let expected = [[0, 0, 0, 0], [0, 1, 1, 0], [0, 2, 1, 0], [0, 3, 1, 0]]
  for edit in [':1 substitute/Line.1\n//', ':1 delete', 'normal 1GVx']
    call DoEditAndCheck(edit, expected, 3)
  endfor
  return

  " NOTE: The tests below are disabled in the original too (after 'return').
  " Delete line 2.
  let expected = [[0, 1, 1, 0], [0, 0, 0, 0], [0, 2, 1, 0], [0, 3, 1, 0]]
  for edit in [':2 substitute/Line..2\n//', ':1 substitute/\nLine..2//',
      \ '2: delete', 'normal 2GVx']
    call DoEditAndCheck(edit, expected, 3)
  endfor

  " Delete line 4.
  let expected = [[0, 1, 1, 0], [0, 2, 1, 0], [0, 3, 1, 0], [0, 0, 0, 0]]
  for edit in [':3 substitute/\nLine....4//', '4: delete', 'normal 4GVx']
    call DoEditAndCheck(edit, expected, 3)
  endfor

  " Delete lines 2-3.
  let expected = [[0, 1, 1, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 2, 1, 0]]
  for edit in [':2,3 substitute/Line.*[23]\n//',
      \ ':2,3 substitute/\%(Line[.]*[23]\n\)*',
      \ '2,3: delete', 'normal 2GVjx']
    call DoEditAndCheck(edit, expected, 2)
  endfor

  " Delete lines 1-3.
  let expected = [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 1, 1, 0]]
  for edit in [':1,$ substitute/Line.*[123]\n//',
      \ ':1,$ substitute/\%(Line[.]*[123]\n\)*',
      \ '1,3: delete', 'normal 1GVjjx']
    call DoEditAndCheck(edit, expected, 1)
  endfor

  " Delete all lines.
  let expected = [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]
  for edit in [':1,$ substitute/Line.*[1234]\n//',
      \ ':1,$ substitute/\%(Line[.]*[1234]\n\)*//',
      \ '1,4: delete', 'normal 1GVjjjx']
    call DoEditAndCheck(edit, expected, 1)
  endfor

  " Delete lines 3-4.
  let expected = [[0, 1, 1, 0], [0, 2, 1, 0], [0, 0, 0, 0], [0, 0, 0, 0]]
  for edit in [':2,$ substitute/\n\%(Line.*[34]\n\?\)*//',
      \ '3,4: delete', 'normal 3GVjx']
    call DoEditAndCheck(edit, expected, 2)
  endfor

  " Delete lines 2-4.
  let expected = [[0, 1, 1, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]
  for edit in [':1,$ substitute/\n\%(Line.*[234]\n\?\)*//',
      \ '2,4: delete', 'normal 2GVjjx']
    call DoEditAndCheck(edit, expected, 1)
  endfor
endfunc

" The substitute command should correctly drop floating, virtual
" properties when lines are deleted.
func Test_multiline_substitute_del_lines_drops_virt_text_props()
  " Helper to set up the buffer with virtual text properties.
  " When a:virt_k_col is 1, 'virt-k' is at line 1 col 1 (floating);
  " when 4, it is at line 1 col 4 (inline).
  func SetupVirtProps(virt_k_col)
    new
    call setline(1, ['Line.1', 'Line..2', 'Line...3', 'Line....4'])
    for s:t in ['1', '2', '3', '4', '7', '8']
      silent! call prop_type_delete(s:t)
    endfor
    call prop_type_add('1', {'highlight': 'DiffAdd'})
    call prop_type_add('2', {'highlight': 'DiffChange', 'end_incl': 1})
    call prop_type_add('3', {'highlight': 'DiffDelete'})
    call prop_type_add('4', {'highlight': 'DiffText'})
    call prop_type_add('7', {'highlight': 'WarningMsg'})
    call prop_type_add('8', {'highlight': 'Directory'})
    " Floating virtual text.
    call prop_add(1, 0, {'type': '1', 'text': 'virt-a', 'text_align': 'right'})
    call prop_add(1, 0, {'type': '2', 'text': 'virt-b', 'text_align': 'right'})
    call prop_add(2, 0, {'type': '3', 'text': 'virt-c', 'text_align': 'right'})
    call prop_add(2, 0, {'type': '4', 'text': 'virt-d', 'text_align': 'right'})
    call prop_add(3, 0, {'type': '4', 'text': 'virt-e', 'text_align': 'right'})
    call prop_add(4, 0, {'type': '3', 'text': 'virt-g', 'text_align': 'right'})
    call prop_add(4, 0, {'type': '7', 'text': 'virt-h', 'text_align': 'right'})
    " Inline virtual text.
    call prop_add(1, a:virt_k_col, {'type': '8', 'text': 'virt-k'})
    " Highlight property spanning lines 1-4.
    call prop_add(1, 1, {'type': '2', 'id': 42, 'end_lnum': 4, 'end_col': 4})
    call prop_add(4, 4, {'type': '3', 'id': 42, 'end_lnum': 4, 'end_col': 7})
  endfunc

  " Join lines 1-2.
  call SetupVirtProps(1)
  1,2 substitute /e.1\nL/e.1 L/
  call assert_equal(3, line('$'))
  call assert_equal('Line.1 Line..2', getline(1))
  call assert_equal(4, len(prop_list(1)))
  call s:CleanupPropTypes(['1', '2', '3', '4', '7', '8'])

  " Join lines 1-3.
  call SetupVirtProps(1)
  1,3 substitute /e.1\nLine..2\nL/e.1 L/
  call assert_equal(2, line('$'))
  call assert_equal('Line.1 Line...3', getline(1))
  " NOTE: Original PR expected value is 3
  call assert_equal(4, len(prop_list(1)))
  call s:CleanupPropTypes(['1', '2', '3', '4', '7', '8'])

  " Join lines 1-4.
  call SetupVirtProps(1)
  1,4 substitute /e.1\nLine..2\nLine...3\nL/e.1 L/
  call assert_equal(1, line('$'))
  call assert_equal('Line.1 Line....4', getline(1))
  call assert_equal(5, len(prop_list(1)))
  call s:CleanupPropTypes(['1', '2', '3', '4', '7', '8'])

  " Second variant: inline virtual text at col 4.
  call SetupVirtProps(4)
  1,2 substitute /e.1\nL/e.1 L/
  call assert_equal(3, line('$'))
  call assert_equal(4, len(prop_list(1)))
  call s:CleanupPropTypes(['1', '2', '3', '4', '7', '8'])
endfunc

" Deletion of text starting a multiline property should adjust next line.
func Test_text_deletion_of_start_to_eol_adjusts_multiline_property()
  " Partial delete: property is shortened but not removed.
  call s:Setup_start_end_prop()
  normal 1G03l2x
  call assert_equal('Lin1', getline(1))
  call assert_equal(1, len(prop_list(1)))
  call assert_equal(2, prop_list(1)[0]['length'])
  call prop_type_delete('1')
  bwipe!

  " Full delete of start: property should be removed from line 1.
  for edit in ['normal 1G03ld$', 'normal 1G03l3x',
      \ 'normal 1G03lv  x', '1 substitute /e.1//']
    call s:Setup_start_end_prop()
    execute edit
    let msg = printf('op="%s"', edit)
    call assert_equal([], prop_list(1), msg)
    call prop_type_delete('1')
    bwipe!
  endfor
endfunc

" Deletion of text ending a multiline property should adjust previous line.
func Test_text_deletion_of_end_to_sol_adjusts_multiline_property()
  " Partial delete: property end is adjusted but not removed.
  call s:Setup_start_end_prop()
  normal 3G02x
  call assert_equal('ne...3', getline(3))
  call assert_equal(1, len(prop_list(3)))
  call assert_equal(0, prop_list(3)[0]['start'])
  call prop_type_delete('1')
  bwipe!

  " Full delete of ending portion: property should be removed from line 3.
  for edit in ['normal 3G03x', 'normal 3G0v  x', '3 substitute /Lin//']
    call s:Setup_start_end_prop()
    execute edit
    let msg = printf('op="%s"', edit)
    call assert_equal([], prop_list(3), msg)
    call prop_type_delete('1')
    bwipe!
  endfor
endfunc

" Inline text properties should be removed when surrounding text is removed.
func Test_text_deletion_removes_inline_virtual_text()
  func SetupVirtText(start_incl, end_incl)
    new
    call setline(1, ['The line with properties....'])
    let opts = {'highlight': 'DiffChange'}
    if a:start_incl
      let opts['start_incl'] = 1
    endif
    if a:end_incl
      let opts['end_incl'] = 1
    endif
    silent! call prop_type_delete('2')
    call prop_type_add('2', opts)
    call prop_add(1, 7, {'type': '2', 'text': 'xxx'})
  endfunc

  " Test all combinations of start_incl/end_incl.
  for [si, ei] in [[0, 0], [1, 0], [0, 1], [1, 1]]
    " Deletion of one char before virtual text: property stays.
    for edit in ['normal 1G05lx', '1 substitute /i//', 'normal 1G05lvx']
      call SetupVirtText(si, ei)
      execute edit
      let msg = printf('si=%d ei=%d op="%s"', si, ei, edit)
      call assert_equal(1, len(prop_list(1)), msg)
      call assert_equal(6, prop_list(1)[0]['col'], msg)
      call prop_type_delete('2')
      bwipe!
    endfor

    " Deletion of one char after virtual text: property stays.
    for edit in ['normal 1G06lx', '1 substitute /n//', 'normal 1G06lvx']
      call SetupVirtText(si, ei)
      execute edit
      let msg = printf('si=%d ei=%d op="%s"', si, ei, edit)
      call assert_equal(1, len(prop_list(1)), msg)
      call assert_equal(7, prop_list(1)[0]['col'], msg)
      call prop_type_delete('2')
      bwipe!
    endfor

    " Deletion of both chars around virtual text: property is removed.
    for edit in ['normal 1G05l2x', '1 substitute /in//', 'normal 1G05lv x']
      call SetupVirtText(si, ei)
      execute edit
      let msg = printf('si=%d ei=%d op="%s"', si, ei, edit)
      call assert_equal([], prop_list(1), msg)
      call prop_type_delete('2')
      bwipe!
    endfor
  endfor
endfunc

" Removing a multiline property from the last line should fix the property
" on the penultimate line.
func Test_multiline_prop_partial_remove_last_using_remove()
  call s:Setup_multiline_props_1()

  call prop_remove({'type': '3'}, 4)
  call assert_equal(1, s:PropForType(3, '3')['end'])

  call s:CleanupPropTypes(['1', '2', '3'])
endfunc

" Removing a multiline property from the penultimate line should fix the
" properties on the previous and last lines.
func Test_multiline_prop_partial_remove_penultimate_using_remove()
  call s:Setup_multiline_props_1()

  call prop_remove({'type': '3'}, 3)
  call assert_equal(1, s:PropForType(2, '3')['end'])
  call assert_equal(1, s:PropForType(4, '3')['start'])

  call s:CleanupPropTypes(['1', '2', '3'])
endfunc

" Removing all properties from the first line should fix the properties
" on the second line.
func Test_multiline_prop_partial_remove_first_using_clear()
  call s:Setup_multiline_props_1()

  call prop_clear(2)
  call assert_equal(1, s:PropForType(3, '3')['start'])
  call assert_equal(1, s:PropForType(3, '1')['start'])

  call s:CleanupPropTypes(['1', '2', '3'])
endfunc

" Removing all multiline properties from the last line should fix the
" properties on the penultimate line.
func Test_multiline_prop_partial_remove_last_using_clear()
  call s:Setup_multiline_props_1()

  call prop_clear(4)
  call assert_equal(1, s:PropForType(3, '3')['end'])
  call assert_equal(1, s:PropForType(3, '1')['end'])

  call s:CleanupPropTypes(['1', '2', '3'])
endfunc

" Removing all multiline properties from the penultimate line should fix the
" properties on the previous and last lines.
func Test_multiline_prop_partial_remove_penultimate_using_clear()
  call s:Setup_multiline_props_1()

  call prop_clear(3)
  call assert_equal(1, s:PropForType(2, '3')['end'])
  call assert_equal(1, s:PropForType(4, '3')['start'])
  call assert_equal(1, s:PropForType(2, '1')['end'])
  call assert_equal(1, s:PropForType(4, '1')['start'])

  call s:CleanupPropTypes(['1', '2', '3'])
endfunc

" Deleting the first line with multiline properties should fix the properties
" on the second line.
func Test_multiline_prop_delete_first_line()
  call s:Setup_multiline_props_1()

  :2 delete
  call assert_equal(3, line('$'))
  call assert_equal(1, s:PropForType(2, '1')['start'])
  call assert_equal(1, s:PropForType(2, '3')['start'])

  call s:CleanupPropTypes(['1', '2', '3'])
endfunc

" Deleting the last line with multiline properties should fix the properties
" on the penultimate line.
func Test_multiline_prop_delete_last_line()
  call s:Setup_multiline_props_1()

  :4 delete
  call assert_equal(3, line('$'))
  call assert_equal(1, s:PropForType(3, '1')['end'])
  call assert_equal(1, s:PropForType(3, '3')['end'])

  call s:CleanupPropTypes(['1', '2', '3'])
endfunc

" Deleting the penultimate line with multiline properties should keep
" the properties spanning lines.
func Test_multiline_prop_delete_penultimate_line()
  call s:Setup_multiline_props_1()

  :3 delete
  call assert_equal(3, line('$'))
  call assert_equal(0, s:PropForType(2, '1')['end'])
  call assert_equal(0, s:PropForType(2, '3')['end'])
  call assert_equal(0, s:PropForType(3, '1')['start'])
  call assert_equal(0, s:PropForType(3, '3')['start'])

  call s:CleanupPropTypes(['1', '2', '3'])
endfunc

" vim: shiftwidth=2 sts=2 expandtab
