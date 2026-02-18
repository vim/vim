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

func Test_setrepeat_dot_command_change()
  " Test . command with change word
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

" vim: shiftwidth=2 sts=2 expandtab
