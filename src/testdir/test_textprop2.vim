" Additional tests for defining text property types and adding text properties
" to the buffer.

CheckFeature textprop
CheckFeature python3

source util/screendump.vim
import './util/vim9.vim' as v9
import './util/textprop_support.vim' as ts

" Clean up for a BufferState model based test.
func CleanupModelTest(model)
  let _ = a:model.DeletePropertyTypes()
  bwipe!
endfunc

" Set up buffer content and properties for a set of tests.
func Setup_multiline_props_1()
  new
  let buf_spec = [
      \  '#12345 123456 1234567 12345678',
      \  '|Line1|Line.2|Line..3|Line...4|',
      \  '        11111_1111111_11111111',
      \  '         2222                 ',
      \  '                22222         ',
      \  '           33_3333333_33333333',
      \  '                        222222',
      \]
  let model = s:ts.LoadBufferSpec(buf_spec)

  " Sanity check model before returning it.
  let _ = model.CheckBufferContent()
  return model
endfunc

" The substitute command should adjust marks when one or more whole lines are
" deleted.
"
" This was added for PR-PAUL, to check no regressions were created by changes
" to the substitute command implementation. It also checks other ways of
" deleting whole lines to catch any introduction of inconsistent behaviour.
func Test_subst_adjusts_marks()
  let buf_spec = [
      \  '#123456 1234567 12345678 123456789',
      \  '|Line.1|Line..2|Line...3|Line....4|',
      \  ' 111111_1111111_11111111_111111111',
      \]
  let orig_model = s:ts.LoadBufferSpec(buf_spec)
  let marks = [[0, 1, 1], [0, 2, 1], [0, 3, 1], [0, 4, 1]]
  let orig_marks = []
  for mark in marks
    let mark_copy = mark->copy()
    " TODO: Why does mark->copy(0) give 'Missing parentheses:' error?
    call add(mark_copy, 0)
    call add(orig_marks, mark_copy)
  endfor

  func! Init_test_subst_adjusts_marks() closure
    let model = s:ts.LoadBufferSpec(buf_spec)
    call setpos("'a", marks[0])
    call setpos("'b", marks[1])
    call setpos("'c", marks[2])
    call setpos("'d", marks[3])
    return model
  endfunc

  func! LoadAndEdit(sub_cmd, del_args, msg) closure
    let model = Init_test_subst_adjusts_marks()
    set undolevels&
    execute a:sub_cmd
    let _ = model.DeleteText(a:del_args[0], a:del_args[1], a:del_args[2])
    let _ = model.CheckBufferContent(a:msg)

    return model
  endfunc

  func! LoadEditAndCheck(model_del_args, edits, expected) closure
    for edit in a:edits
      let msg = printf('Edit command = "%s"', edit)
      let model = LoadAndEdit(edit, a:model_del_args, msg)
      call assert_equal(a:expected[0], getpos("'a"), msg . ', mark a')
      call assert_equal(a:expected[1], getpos("'b"), msg . ', mark b')
      call assert_equal(a:expected[2], getpos("'c"), msg . ', mark c')
      call assert_equal(a:expected[3], getpos("'d"), msg . ', mark d')

      :undo
      let _ = orig_model.CheckBufferContent(msg .. ', post-undo')
      call assert_equal(orig_marks[0], getpos("'a"), msg . ', mark a')
      call assert_equal(orig_marks[1], getpos("'b"), msg . ', mark b')
      call assert_equal(orig_marks[2], getpos("'c"), msg . ', mark c')
      call assert_equal(orig_marks[3], getpos("'d"), msg . ', mark d')
    endfor
    return model
  endfunc

  let model = LoadEditAndCheck(
    \ [1, 1, 7],
    \ [':1 substitute/Line.1\n//', ':1 delete', 'normal 1GVx'],
    \ [ [0, 0, 0, 0], [0, 1, 1, 0], [0, 2, 1, 0], [0, 3, 1, 0]],
  \)
  return

  let model = LoadEditAndCheck(
  \   [2, 1, 8],
  \   [
  \     ':2 substitute/Line..2\n//', ':1 substitute/\nLine..2//',
  \     '2: delete', 'normal 2GVx',
  \   ],
  \   [[0, 1, 1, 0], [0, 0, 0, 0], [0, 2, 1, 0], [0, 3, 1, 0]],
  \ )

  let model = LoadEditAndCheck(
    \ [4, 1, 10],
    \ [':3 substitute/\nLine....4//', '4: delete', 'normal 4GVx'],
    \ [[0, 1, 1, 0], [0, 2, 1, 0], [0, 3, 1, 0], [0, 0, 0, 0]],
  \)

  let model = LoadEditAndCheck(
    \ [2, 1, 17],
    \ [
    \   ':2,3 substitute/Line.*[23]\n//',
    \   ':2,3 substitute/\%(Line[.]*[23]\n\)*',
    \   '2,3: delete', 'normal 2GVjx',
    \ ],
    \ [[0, 1, 1, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 2, 1, 0]],
  \)

  let model = LoadEditAndCheck(
  \ [1, 1, 24],
  \ [
  \    ':1,$ substitute/Line.*[123]\n//',
  \    ':1,$ substitute/\%(Line[.]*[123]\n\)*',
  \    '1,3: delete', 'normal 1GVjjx',
  \ ],
  \ [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 1, 1, 0]],
  \)

  let model = LoadEditAndCheck(
  \ [1, 1, 34],
  \ [
  \   ':1,$ substitute/Line.*[1234]\n//',
  \   ':1,$ substitute/\%(Line[.]*[1234]\n\)*//',
  \   '1,4: delete',
  \   'normal 1GVjjjx',
  \ ],
  \ [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
  \)

  let model = LoadEditAndCheck(
  \ [3, 1, 19],
  \ [
  \   ':2,$ substitute/\n\%(Line.*[34]\n\?\)*//',
  \   '3,4: delete', 'normal 3GVjx',
  \ ],
  \ [[0, 1, 1, 0], [0, 2, 1, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
  \)

  let model = LoadEditAndCheck(
  \ [2, 1, 27],
  \ [
  \   ':1,$ substitute/\n\%(Line.*[234]\n\?\)*//',
  \   '2,4: delete', 'normal 2GVjjx',
  \ ],
  \ [[0, 1, 1, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
  \)

  call CleanupModelTest(model)
endfunc

" The substitute command should correctly drop floating, virtual
" properties when lines are deleted.
func Test_multiline_substitute_del_lines_drops_virt_text_props()
  let buf_spec = [
      \  '#123456 1234567 12345678 123456789',
      \  '|Line.1|Line..2|Line...3|Line....4|',
      \  ' 1                                  text="virt-a" text_align="right"',
      \  ' 2                                  text="virt-b" text_align="right"',
      \  '        3                           text="virt-c" text_align="right"',
      \  '        4                           text="virt-d" text_align="right"',
      \  ' 8                                  text="virt-k"',
      \  '                4                   text="virt-e" text_align="right"',
      \  '                         3          text="virt-g" text_align="right"',
      \  '                         7          text="virt-h" text_align="right"',
      \  ' 2222                         333   end_incl',
      \]

  func! LoadEditAndCheck(sub_cmd, del_args, ins_args) closure
    let model = s:ts.LoadBufferSpec(buf_spec)
    execute a:sub_cmd
    let _ = model.DeleteText(a:del_args[0], a:del_args[1], a:del_args[2])
    let _ = model.InsertText(a:ins_args[0], a:ins_args[1], a:ins_args[2])
    let _ = model.CheckBufferContent()
    return model
  endfunc

  let model = LoadEditAndCheck(
    \ '1,2 substitute /e.1\nL/e.1 L/',
    \ [1, 4, 5], [1, 3, 'e.1 L'])
  call assert_equal(4, len(model.ExpectedPropList(1)))

  let model = LoadEditAndCheck(
    \ '1,3 substitute /e.1\nLine..2\nL/e.1 L/',
    \ [1, 4, 13], [1, 3, 'e.1 L'])
  call assert_equal(3, len(model.ExpectedPropList(1)))

  let model = LoadEditAndCheck(
    \ '1,4 substitute /e.1\nLine..2\nLine...3\nL/e.1 L/',
    \ [1, 4, 22], [1, 3, 'e.1 L'])
  call assert_equal(5, len(model.ExpectedPropList(1)))

  let buf_spec = [
      \  '#123456 1234567 12345678 123456789',
      \  '|Line.1|Line..2|Line...3|Line....4|',
      \  ' 1                                  text="virt-a" text_align="right"',
      \  ' 2                                  text="virt-b" text_align="right"',
      \  '        3                           text="virt-c" text_align="right"',
      \  '        4                           text="virt-d" text_align="right"',
      \  '    8                               text="virt-k"',
      \  '                4                   text="virt-e" text_align="right"',
      \  '                         3          text="virt-g" text_align="right"',
      \  '                         7          text="virt-h" text_align="right"',
      \  ' 2222                         333   end_incl',
      \]

  let model = LoadEditAndCheck(
    \ '1,2 substitute /e.1\nL/e.1 L/',
    \ [1, 4, 5], [1, 3, 'e.1 L'])
  call assert_equal(4, len(model.ExpectedPropList(1)))

  call CleanupModelTest(model)
endfunc

" Deletion of text starting a multiline property should adjust next line.
func Test_text_deletion_of_start_to_eol_adjusts_multiline_property()
  let buf_spec = [
      \  '#123456 1234567 12345678 123456789',
      \  '|Line.1|Line..2|Line...3|Line....4|',
      \  '    111111111111111                ',
      \]

  func! LoadEditAndCheckOne(edit, del_args) closure
    let model = s:ts.LoadBufferSpec(buf_spec)
    execute a:edit
    let _ = model.DeleteText(a:del_args[0], a:del_args[1], a:del_args[2])
    let _ = model.CheckBufferContent(printf('op="%s"', a:edit))
    call CleanupModelTest(model)
    return model
  endfunc

  func! LoadEditAndCheck(del_edits, del_args)
    for edit in a:del_edits
      let model = LoadEditAndCheckOne(edit, a:del_args)
    endfor
    return model
  endfunc

  "
  " First sanity check some cases where the property is *not* deleted.
  "
  let model = LoadEditAndCheck(
    \ ['normal 1G03l2x'],
    \ [1, 4, 2])
  call assert_equal(2, model.ExpectedPropList(1)[0]['length'])

  "
  " Then cases where the property should be deleted.
  "
  let model = LoadEditAndCheck(
    \ ['normal 1G03ld$', 'normal 1G03l3x', 'normal 1G03lv  x', '1 substitute /e.1//'],
    \ [1, 4, 3])
  call assert_equal([], model.ExpectedPropList(1))
endfunc

" Deletion of text ending a multiline property should adjust previous line.
func Test_text_deletion_of_end_to_sol_adjusts_multiline_property()
  let buf_spec = [
      \  '#123456 1234567 12345678 123456789',
      \  '|Line.1|Line..2|Line...3|Line....4|',
      \  '    111111111111111                ',
      \]

  func! LoadEditAndCheckOne(edit, del_args) closure
    let model = s:ts.LoadBufferSpec(buf_spec)
    execute a:edit
    let _ = model.DeleteText(a:del_args[0], a:del_args[1], a:del_args[2])
    let _ = model.CheckBufferContent(printf('op="%s"', a:edit))
    call CleanupModelTest(model)
    return model
  endfunc

  func! LoadEditAndCheck(del_edits, del_args)
    for edit in a:del_edits
      let model = LoadEditAndCheckOne(edit, a:del_args)
    endfor
    return model
  endfunc

  "
  " First sanity check some cases where the property is *not* deleted.
  "
  let model = LoadEditAndCheck(
    \ ['normal 3G02x'],
    \ [3, 1, 2])
  call assert_equal(0, model.ExpectedPropList(3)[0]['start'])

  "
  " Then cases where the property should be deleted.
  "
  let model = LoadEditAndCheck(
    \ ['normal 3G03x', 'normal 3G0v  x', '3 substitute /Lin//'],
    \ [3, 1, 3])
  call assert_equal([], model.ExpectedPropList(3))
endfunc

" Inline text properties should be removed when surrounding text is removed.
func Test_text_deletion_removes_inline_virtual_text()
  let buf_spec_1 = [
      \  '#1234567899 123456789 123456789 ',
      \  '|The line with properties....|',
      \  '       2                         text="xxx"',
      \]

  let buf_spec_2 = [
      \  '#1234567899 123456789 123456789 ',
      \  '|The line with properties....|',
      \  '       2                         text="xxx" start_incl',
      \]

  let buf_spec_3 = [
      \  '#1234567899 123456789 123456789 ',
      \  '|The line with properties....|',
      \  '       2                         text="xxx" end_incl',
      \]

  let buf_spec_4 = [
      \  '#1234567899 123456789 123456789 ',
      \  '|The line with properties....|',
      \  '       2                         text="xxx" start_incl end_incl',
      \]

  func! LoadEditAndCheckOne(spec_name, edit, del_args) closure
    let spec = eval(a:spec_name)
    let model = s:ts.LoadBufferSpec(spec)
    execute a:edit
    let _ = model.DeleteText(a:del_args[0], a:del_args[1], a:del_args[2])
    let _ = model.CheckBufferContent(printf('spec=%s op="%s"', a:spec_name, a:edit))
    call CleanupModelTest(model)
    return model
  endfunc

  func! LoadEditAndCheck(spec_name, del_edits, del_args)
    for edit in a:del_edits
      let model = LoadEditAndCheckOne(a:spec_name, edit, a:del_args)
    endfor
    return model
  endfunc

  func! LoadEditAndCheckSpecs(spec_names, del_edits, del_args)
    for name in a:spec_names
      let model = LoadEditAndCheck(name, a:del_edits, a:del_args)
    endfor
    return model
  endfunc

  "
  " First sanity check some cases where the property is *not* deleted.
  "
  let model = LoadEditAndCheckSpecs(
    \ ['buf_spec_1', 'buf_spec_2', 'buf_spec_3', 'buf_spec_4'],
    \ ['normal 1G05lx', '1 substitute /i//', 'normal 1G05lvx'],
    \ [1, 6, 1])
  call assert_equal(6, model.ExpectedPropList(1)[0]['col'])

  let model = LoadEditAndCheckSpecs(
    \ ['buf_spec_1', 'buf_spec_2', 'buf_spec_3', 'buf_spec_4'],
    \ ['normal 1G06lx', '1 substitute /n//', 'normal 1G06lvx'],
    \ [1, 7, 1])
  call assert_equal(7, model.ExpectedPropList(1)[0]['col'])

  "
  " Cases where the property should be deleted.
  "
  let model = LoadEditAndCheckSpecs(
    \ ['buf_spec_1', 'buf_spec_2', 'buf_spec_3', 'buf_spec_4'],
    \ ['normal 1G05l2x', '1 substitute /in//', 'normal 1G05lv x'],
    \ [1, 6, 2])
  call assert_equal([], model.ExpectedPropList(1))

endfunc

" Removing a multiline property from the last line should fix the property
" on the penultimate line.
func Test_multiline_prop_partial_remove_last_using_remove()
  let model = Setup_multiline_props_1()

  call prop_remove({'type': '3'}, 4)
  let _ = model.RemovePropertyFromLine(4, '3')
  let _ = model.CheckBufferContent()

  call assert_equal(1, model.ExpectedPropForType(3, '3')['end'])

  call CleanupModelTest(model)
endfunc

" Removing a multiline property from the penultimate line should fix the
" properties on the previous and last lines.
func Test_multiline_prop_partial_remove_penultimate_using_remove()
  let model = Setup_multiline_props_1()

  call prop_remove({'type': '3'}, 3)
  let _ = model.RemovePropertyFromLine(3, '3')
  let _ = model.CheckBufferContent()

  call assert_equal(1, model.ExpectedPropForType(2, '3')['end'])
  call assert_equal(1, model.ExpectedPropForType(4, '3')['start'])

  call CleanupModelTest(model)
endfunc

" Removing all properties from the first line should fix the properties
" on the second line.
func Test_multiline_prop_partial_remove_first_using_clear()
  let model = Setup_multiline_props_1()

  call prop_clear(2)
  for type_name in '123'
    let _ = model.RemovePropertyFromLine(2, type_name)
  endfor
  let _ = model.CheckBufferContent()

  call assert_equal(1, model.ExpectedPropForType(3, '3')['start'])
  call assert_equal(1, model.ExpectedPropForType(3, '1')['start'])

  call CleanupModelTest(model)
endfunc

" Removing all multiline properties from the last line should fix the
" properties on the penultimate line.
func Test_multiline_prop_partial_remove_last_using_clear()
  let model = Setup_multiline_props_1()

  call prop_clear(4)
  for type_name in '123'
    let _ = model.RemovePropertyFromLine(4, type_name)
  endfor
  let _ = model.CheckBufferContent()

  call assert_equal(1, model.ExpectedPropForType(3, '3')['end'])
  call assert_equal(1, model.ExpectedPropForType(3, '1')['end'])

  call CleanupModelTest(model)
endfunc

" Removing all multiline properties from the penultimate line should fix the
" properties on the previous and last lines.
func Test_multiline_prop_partial_remove_penultimate_using_clear()
  let model = Setup_multiline_props_1()

  call prop_clear(3)
  for type_name in '123'
    let _ = model.RemovePropertyFromLine(3, type_name)
  endfor
  let _ = model.CheckBufferContent()

  call assert_equal(1, model.ExpectedPropForType(2, '3')['end'])
  call assert_equal(1, model.ExpectedPropForType(4, '3')['start'])
  call assert_equal(1, model.ExpectedPropForType(2, '1')['end'])
  call assert_equal(1, model.ExpectedPropForType(4, '1')['start'])

  call CleanupModelTest(model)
endfunc

" Deleting the first line with multiline properties should fix the properties
" on the second line.
func Test_multiline_prop_delete_first_line()
  let model = Setup_multiline_props_1()

  :2 delete
  let _ = model.DeleteText(2, 1, 7)   "Delete 'Line.2'.
  let _ = model.CheckBufferContent()

  call assert_equal(1, model.ExpectedPropForType(2, '1')['start'])
  call assert_equal(1, model.ExpectedPropForType(2, '3')['start'])

  call CleanupModelTest(model)
endfunc

" Deleting the last line with multiline properties should fix the properties
" on the penultimate line.
func Test_multiline_prop_delete_last_line()
  let model = Setup_multiline_props_1()

  :4 delete
  let _ = model.DeleteText(4, 1, 9)   "Delete 'Line...4'.
  let _ = model.CheckBufferContent()

  call assert_equal(1, model.ExpectedPropForType(3, '1')['end'])
  call assert_equal(1, model.ExpectedPropForType(3, '3')['end'])

  call CleanupModelTest(model)
endfunc

" Deleting the penultimate line with multiline properties should keep
" the properties spanning lines.
func Test_multiline_prop_delete_penultimate_line()
  let model = Setup_multiline_props_1()

  :3 delete
  let _ = model.DeleteText(3, 1, 8)   "Delete 'Line..3'.
  let _ = model.CheckBufferContent()

  call assert_equal(0, model.ExpectedPropForType(2, '1')['end'])
  call assert_equal(0, model.ExpectedPropForType(2, '3')['end'])
  call assert_equal(0, model.ExpectedPropForType(3, '1')['start'])
  call assert_equal(0, model.ExpectedPropForType(3, '3')['start'])

  call CleanupModelTest(model)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
