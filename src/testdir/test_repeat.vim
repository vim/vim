" Tests for repeat commands and related functions
" - . (dot command)
" - @: (repeat last command-line)
" - setrepeat() and getrepeat()

func Test_setrepeat_getrepeat_basic()
  " Test basic dictionary set and get
  call setrepeat({'cmd': 'dd'})
  let result = getrepeat()
  call assert_equal('dd', result.cmd)
  call assert_equal(v:t_dict, type(result))
endfunc

func Test_setrepeat_with_text()
  " Test insert mode command with text
  call setrepeat({'cmd': 'i', 'text': 'Hello'})
  let result = getrepeat()
  call assert_equal('i', result.cmd)
  call assert_equal('Hello', result.text)
endfunc

func Test_setrepeat_normal_command()
  " Test various normal mode commands
  call setrepeat({'cmd': '3x'})
  call assert_equal('3x', getrepeat().cmd)

  call setrepeat({'cmd': 'yy'})
  call assert_equal('yy', getrepeat().cmd)
endfunc

func Test_setrepeat_empty_text()
  " Test with empty text field
  call setrepeat({'cmd': 'i', 'text': ''})
  let result = getrepeat()
  call assert_equal('i', result.cmd)
  call assert_equal('', result.text)
endfunc

func Test_setrepeat_overwrite()
  " Test that new setrepeat overwrites previous
  call setrepeat({'cmd': 'dd'})
  call setrepeat({'cmd': 'yy'})
  call assert_equal('yy', getrepeat().cmd)
endfunc

func Test_setrepeat_roundtrip()
  " Test save and restore
  call setrepeat({'cmd': 'o', 'text': 'line'})
  let saved = getrepeat()

  call setrepeat({'cmd': 'dd'})
  call assert_equal('dd', getrepeat().cmd)

  " Restore
  call setrepeat(saved)
  let result = getrepeat()
  call assert_equal('o', result.cmd)
  call assert_equal('line', result.text)
endfunc

func Test_getrepeat_without_setrepeat()
  " Test getrepeat when no setrepeat was called
  " Should return a dictionary with empty or limited info
  let result = getrepeat()
  call assert_equal(v:t_dict, type(result))
  call assert_true(has_key(result, 'cmd'))
endfunc

func Test_setrepeat_dot_command_delete()
  " Test that . command actually works after setrepeat
  new
  call setline(1, ['line1', 'line2', 'line3'])

  " Set repeat to delete a line
  call setrepeat({'cmd': 'dd'})

  " Execute . command
  normal! 1G
  normal .

  " line1 should be deleted
  call assert_equal(['line2', 'line3'], getline(1, '$'))

  bwipe!
endfunc

func Test_setrepeat_dot_command_insert()
  " Test . command with insert mode
  new
  call setline(1, [''])

  " Set repeat to insert text
  call setrepeat({'cmd': 'i', 'text': 'Hello'})

  " Execute . command
  normal! 1G
  normal .

  " Should have inserted 'Hello'
  call assert_equal('Hello', getline(1))

  bwipe!
endfunc

func Test_setrepeat_dot_command_append()
  " Test . command with append
  new
  call setline(1, ['First'])

  " Set repeat to append text
  call setrepeat({'cmd': 'a', 'text': 'Second'})

  " Execute . command
  normal! 1G$
  normal .

  " Should have appended 'Second'
  call assert_equal('FirstSecond', getline(1))

  bwipe!
endfunc

" Test_setrepeat_dot_command_change
func Test_setrepeat_dot_command_change()
  new
  call setline(1, ['old word'])

  " Set repeat to change word
  call setrepeat({'cmd': 'cw', 'text': 'new'})

  " Execute . command on 'word'
  normal! 1G
  normal w
  normal .

  " Should have changed 'word' to 'new'
  call assert_equal('old new', getline(1))

  bwipe!
endfunc

" Test nested setrepeat inside a user function/autocmd
func Test_setrepeat_nested_userfunc()
  new
  call setline(1, ['alpha beta'])
  function! s:inner()
    " inside user function: setrepeat and rely on outer restore not to stomp it
    call setrepeat({'cmd':'cw','text':'gamma'})
  endfunction
  call s:inner()
  normal! 1G
  normal w
  normal .
  call assert_equal('alpha gamma', getline(1))
  bwipe!
endfunc

func Test_setrepeat_dot_multiple_times()
  " Test that . can be used multiple times
  new
  call setline(1, ['1', '2', '3'])

  call setrepeat({'cmd': 'dd'})

  " Use . three times
  normal! 1G
  normal .
  normal .
  normal .

  " All lines should be deleted
  call assert_equal([''], getline(1, '$'))

  bwipe!
endfunc

func Test_setrepeat_save_restore_dot()
  " Test save/restore preserves . functionality
  new
  call setline(1, ['aaa', 'bbb', 'ccc'])

  " Set and save first repeat
  call setrepeat({'cmd': 'dd'})
  let saved = getrepeat()

  " Change to different repeat
  call setrepeat({'cmd': 'yy'})

  " Restore original
  call setrepeat(saved)

  " . should delete line
  normal! 1G
  normal .
  call assert_equal(['bbb', 'ccc'], getline(1, '$'))

  bwipe!
endfunc

func Test_getrepeat_user_insert()
  " Test getrepeat after user insert operation
  new

  execute "normal! iHello\<Esc>"

  let result = getrepeat()
  call assert_equal('i', result.cmd)
  call assert_equal('Hello', result.text)

  bwipe!
endfunc

func Test_getrepeat_user_append()
  " Test getrepeat after user append operation
  new
  call setline(1, 'First')

  execute "normal! ASecond\<Esc>"

  let result = getrepeat()
  call assert_equal('A', result.cmd)
  call assert_equal('Second', result.text)

  bwipe!
endfunc

func Test_getrepeat_user_open()
  " Test getrepeat after open line
  new

  execute "normal! oNew line\<Esc>"

  let result = getrepeat()
  call assert_equal('o', result.cmd)
  call assert_equal('New line', result.text)

  bwipe!
endfunc

func Test_setrepeat_insert_mode_exit()
  " Test that dot command with insert mode exits to normal mode
  new

  " Set repeat for insert mode
  call setrepeat({'cmd': 'i', 'text': 'Hello'})

  " Execute dot command - use normal! to execute dot
  normal! .

  " Check that we're in normal mode, not insert mode
  call assert_equal('n', mode())

  " Check that text was inserted
  call assert_equal('Hello', getline(1))

  bwipe!
endfunc

func Test_setrepeat_append_mode_exit()
  " Test append mode also exits to normal mode
  new
  call setline(1, 'Start')

  call setrepeat({'cmd': 'a', 'text': 'End'})

  " Move to end of line and execute dot
  normal! $
  normal! .

  call assert_equal('n', mode())
  call assert_equal('StartEnd', getline(1))

  bwipe!
endfunc

func Test_setrepeat_open_line_mode_exit()
  " Test 'o' command exits to normal mode
  new

  call setrepeat({'cmd': 'o', 'text': 'New line'})
  normal! .

  call assert_equal('n', mode())
  call assert_equal(['', 'New line'], getline(1, '$'))

  bwipe!
endfunc

func Test_setrepeat_normal_mode_stays_normal()
  " Test that non-insert commands stay in normal mode
  new
  call setline(1, ['line1', 'line2'])

  call setrepeat({'cmd': 'dd'})
  normal! .

  call assert_equal('n', mode())
  call assert_equal(['line2'], getline(1, '$'))

  bwipe!
endfunc

func Test_setrepeat_capital_commands()
  " Test 'I', 'A', 'O' through setrepeat (not just user input)
  new
  call setline(1, '  text')

  call setrepeat({'cmd': 'I', 'text': 'Start'})
  normal! .
  call assert_equal('n', mode())

  bwipe!
endfunc

func Test_setrepeat_multibyte()
  new
  call setrepeat({'cmd': 'i', 'text': '日本語'})
  normal! .
  call assert_equal('日本語', getline(1))
  bwipe!
endfunc

func Test_setrepeat_special_chars()
  new
  call setrepeat({'cmd': 'i', 'text': "Tab\tHere"})
  normal! .
  call assert_equal("Tab\tHere", getline(1))
  bwipe!
endfunc

func Test_getrepeat_user_overrides_setrepeat()
  " Test that user operations override setrepeat() value
  new

  " Set repeat programmatically
  call setrepeat({'cmd': 'dd'})
  call assert_equal('dd', getrepeat().cmd)

  " User performs insert operation
  execute "normal! iHello\<Esc>"

  " getrepeat() should now return user operation, not setrepeat() value
  let result = getrepeat()
  call assert_equal('i', result.cmd)
  call assert_equal('Hello', result.text)

  bwipe!
endfunc

" Test: multibyte inserted text is handled properly
func Test_setrepeat_multibyte_insert()
  new
  call setline(1, ['old word'])
  " use multi-byte characters as inserted text
  call setrepeat({'cmd':'cw', 'text':'日本'})
  normal! 1G
  normal w
  normal .
  call assert_equal('old 日本', getline(1))
  bwipe!
endfunc

" Test: long inserted text does not crash and repeats correctly
func Test_setrepeat_long_text()
  new
  let long = repeat('abcd', 1024) " ~4KiB text
  call setline(1, ['first'])
  call setrepeat({'cmd':'cw', 'text': long})
  normal! 1G
  normal w
  normal .
  " ensure new text has been inserted (length check rather than full equality)
  let got = getline(1)
  call assert_true(stridx(got, 'abcd') >= 0)
  bwipe!
endfunc

" Test: save/restore with dd/yy variations (regression guard)
func Test_setrepeat_save_restore_dd()
  new
  call setline(1, ['one','two','three'])
  " set repeat for deleting a line
  call setrepeat({'cmd':'dd'})
  " Simulate a nested save/restore by calling a function that triggers it
  function! s:inner2()
    " call setrepeat inside function to ensure saved copies get updated
    call setrepeat({'cmd':'dd'})
  endfunction
  call s:inner2()
  normal! 1G
  normal .
  " first line should be deleted
  call assert_equal(['two','three'], getline(1, '$'))
  bwipe!
endfunc

" setrepeat() must not change the last-insert register ('.')
func Test_setrepeat_does_not_change_dot_register()
  new
  " prepare a known last-insert text by doing an actual insert
  call setline(1, ['line'])
  execute "normal! 1G0iorig\<Esc>"
  let before = getreg('.')
  " set repeat to a non-insert command; should not clobber register '.'
  call setrepeat({'cmd': 'dd'})
  call assert_equal(before, getreg('.'))
  bwipe!
endfunc

" setrepeat() should set getreg('.') to the inserted text only
func Test_setrepeat_change_sets_dot_to_inserted_text()
  new
  call setline(1, ['alpha beta'])
  " ensure previous last-insert is something else
  execute "normal! 1G0iold\<Esc>"
  " now set repeat to a change with inserted text
  call setrepeat({'cmd': 'cw', 'text': 'gamma'})
  " getreg('.') must equal the inserted text (not include the motion)
  call assert_equal('gamma', getreg('.'))
  bwipe!
endfunc

" Error cases: missing/invalid args should fail with E475
func Test_setrepeat_errors_missing_and_types()
  " ensure a known repeat state for the "no-change" check
  call setrepeat({'cmd': 'cw', 'text': 'x'})
  let before = getrepeat()

  " missing cmd key: should fail and not change getrepeat()
  call assert_fails('call setrepeat({})', 'E474:')
  call assert_equal(before, getrepeat())

  " cmd is convertable string
  call setrepeat({"cmd": 123})
  " text is convertable string
  call setrepeat({"cmd": "i", "text": 123})

  " non-dict argument
  call assert_fails('call setrepeat(123)', 'E1206:')

  " cmd not a string
  call assert_fails('call setrepeat({"cmd": {}})', 'E731:')
  " text not a string (should be invalid)
  call assert_fails('call setrepeat({"cmd": "i", "text": []})', 'E730:')
endfunc

" Count behavior: '3dd' should delete 3 lines when repeated
func Test_setrepeat_count_behavior()
  new
  call setline(1, ['one','two','three','four','five'])
  " set repeat to delete 3 lines
  call setrepeat({'cmd': '3dd'})
  normal! 1G
  normal .
  " expect that first three lines were deleted, leaving 'four','five'
  call assert_equal(['four','five'], getline(1, '$'))
  bwipe!
endfunc

" setrepeat should not clobber other named registers
func Test_setrepeat_does_not_clobber_named_register()
  new
  call setline(1, ['alpha beta'])
  " put something in register 'a'
  call setreg('a', 'keepme')
  " set repeat to a change; this should set getreg('.') but not change 'a'
  call setrepeat({'cmd': 'cw', 'text': 'gamma'})
  call assert_equal('keepme', getreg('a'))
  " dot register should be set to the inserted text (existing test covers this
  " but we assert again)
  call assert_equal('gamma', getreg('.'))
  bwipe!
endfunc

func Test_setrepeat_operator_motion_ciquote()
  new
  call setline(1, ['"inner" rest'])
  " set repeat: change inner-quote content to 'X'
  call setrepeat({'cmd': 'ci"', 'text': 'X'})

  " Ensure cursor is at line start, then find the quote and apply .
  call cursor(1, 1)
  normal f"
  normal .

  " Correct behavior: quotes remain, inner text replaced -> '"X" rest'
  call assert_equal(['"X" rest'], getline(1, '$'))
  bwipe!
endfunc

" Change whole line with 'cc'
func Test_setrepeat_change_line_cc()
  new
  call setline(1, ['old'])
  " set repeat to change whole line to 'new'
  call setrepeat({'cmd': 'cc', 'text': 'new'})
  " go to first line and run dot
  normal! 1G
  normal .
  call assert_equal('new', getline(1))
  bwipe!
endfunc

" Change to end of line with 'C'
func Test_setrepeat_change_to_end_C()
  new
  call setline(1, ['old tail'])
  " set repeat: change to end-of-line and insert 'tail2'
  call setrepeat({'cmd': 'C', 'text': 'tail2'})
  " move cursor to the start of the old tail (column 5) then run dot
  call cursor(1, 5)
  normal .
  call assert_equal('old tail2', getline(1))
  bwipe!
endfunc

" Substitute single character with 's'
func Test_setrepeat_substitute_s()
  new
  call setline(1, ['abcd'])
  " set repeat: substitute single char with 'X'
  call setrepeat({'cmd': 's', 'text': 'X'})
  " go to beginning and run dot to replace first character
  normal! 1G
  normal .
  call assert_equal('Xbcd', getline(1))
  bwipe!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
