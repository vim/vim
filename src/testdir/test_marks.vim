" Test for marks

" Test that a deleted mark is restored after delete-undo-redo-undo.
func Test_Restore_DelMark()
  enew!
  call append(0, ["	textline A", "	textline B", "	textline C"])
  normal! 2gg
  set nocp viminfo+=nviminfo
  exe "normal! i\<C-G>u\<Esc>"
  exe "normal! maddu\<C-R>u"
  let pos = getpos("'a")
  call assert_equal(2, pos[1])
  call assert_equal(1, pos[2])
  enew!
endfunc

" Test that CTRL-A and CTRL-X updates last changed mark '[, '].
func Test_Incr_Marks()
  enew!
  call append(0, ["123 123 123", "123 123 123", "123 123 123"])
  normal! gg
  execute "normal! \<C-A>`[v`]rAjwvjw\<C-X>`[v`]rX"
  call assert_equal("AAA 123 123", getline(1))
  call assert_equal("123 XXXXXXX", getline(2))
  call assert_equal("XXX 123 123", getline(3))
  enew!
endfunc

func Test_previous_jump_mark()
  new
  call setline(1, ['']->repeat(6))
  normal Ggg
  call assert_equal(6, getpos("''")[1])
  normal jjjjj
  call assert_equal(6, getpos("''")[1])
  bwipe!
endfunc

func Test_visual_setpos_reverse_endpoint()
  new
  call setline(1, 'abcdefghijk')
  for command in ["gg0v6l\<Esc>", "gg0v6lo\<Esc>"]
    execute 'normal! ' .. command
    call setpos("'<", getpos("'<"))
    call setpos("'>", getpos("'>"))
    call assert_equal([[0, 1, 1, 0], [0, 1, 7, 0]],
          \ [getpos("'<"), getpos("'>")], command)
    normal! gvy
    call assert_equal('abcdefg', getreg('"'), command)

    execute 'normal! ' .. command
    call setpos("'<", [0, 1, 2, 0])
    call assert_equal([[0, 1, 2, 0], [0, 1, 7, 0]],
          \ [getpos("'<"), getpos("'>")], command)
    normal! gvy
    call assert_equal('bcdefg', getreg('"'), command)

    execute 'normal! ' .. command
    call setpos("'>", [0, 1, 6, 0])
    call assert_equal([[0, 1, 1, 0], [0, 1, 6, 0]],
          \ [getpos("'<"), getpos("'>")], command)
    normal! gvy
    call assert_equal('abcdef', getreg('"'), command)
  endfor
  bwipe!
endfunc

func Test_visual_setpos_stable_crossing()
  new
  call setline(1, 'abcdefghijk')
  for reversed in [0, 1]
    for positions in [[1, 7, 8, 10], [5, 9, 2, 4], [1, 7, 5, 8]]
      for order in [['<', '>'], ['>', '<']]
        execute "normal! gg0" .. repeat('l', positions[0] - 1) .. 'v'
              \ .. repeat('l', positions[1] - positions[0])
              \ .. (reversed ? 'o' : '') .. "\<Esc>"
        let expected = positions[0 : 1]
        for mark in order
          let slot = mark == '<' ? 0 : 1
          let expected[slot] = positions[slot + 2]
          call setpos("'" .. mark, [0, 1, expected[slot], 0])
          call assert_equal([[0, 1, min(expected), 0],
                \ [0, 1, max(expected), 0]],
                \ [getpos("'<"), getpos("'>")],
                \ string([reversed, positions, order, mark]))
        endfor
        normal! gvy
        call assert_equal(strpart(getline(1), min(expected) - 1,
              \ max(expected) - min(expected) + 1), getreg('"'),
              \ string([reversed, positions, order]))
      endfor
    endfor
  endfor
  bwipe!
endfunc

func Test_visual_setpos_endpoint_history()
  new
  call setline(1, 'abcdefghijk')
  execute "normal! gg0v6l\<Esc>"
  call setpos("'<", [0, 1, 8, 0])
  let crossed = [getpos("'<"), getpos("'>")]
  call setpos("'<", [0, 1, 5, 0])
  call assert_equal([[0, 1, 5, 0], [0, 1, 7, 0]],
        \ [getpos("'<"), getpos("'>")])
  normal! gvy
  call assert_equal('efg', getreg('"'))

  " The same anchor/cursor coordinates have a different assignment history.
  execute "normal! gg06lvlo\<Esc>"
  call assert_equal(crossed, [getpos("'<"), getpos("'>")])
  call setpos("'<", [0, 1, 5, 0])
  call assert_equal([[0, 1, 5, 0], [0, 1, 8, 0]],
        \ [getpos("'<"), getpos("'>")])
  normal! gvy
  call assert_equal('efgh', getreg('"'))

  " A sorted getter does not rebind the setter after a crossing.
  execute "normal! gg0v6l\<Esc>"
  call setpos("'<", [0, 1, 8, 0])
  call setpos("'<", getpos("'<"))
  call assert_equal([[0, 1, 7, 0], [0, 1, 7, 0]],
        \ [getpos("'<"), getpos("'>")])
  bwipe!
endfunc

func Test_visual_setpos_unset_identity()
  new
  call setline(1, 'abcdefghijk')
  for command in ["gg0v6l\<Esc>", "gg0v6lo\<Esc>"]
    for mark in ['<', '>']
      for method in ['setpos', 'delmarks']
        execute 'normal! ' .. command
        if method == 'setpos'
          call setpos("'" .. mark, [0, 0, 0, 0])
        else
          execute 'delmarks ' .. mark
        endif
        let remaining = mark == '<' ? 7 : 1
        call assert_equal([[0, 1, remaining, 0], [0, 1, remaining, 0]],
              \ [getpos("'<"), getpos("'>")], string([command, mark, method]))
        call setpos("'" .. mark, [0, 1, mark == '<' ? 2 : 6, 0])
        call setpos("'" .. (mark == '<' ? '>' : '<'),
              \ [0, 1, mark == '<' ? 10 : 2, 0])
        call assert_equal([[0, 1, 2, 0], [0, 1, mark == '<' ? 10 : 6, 0]],
              \ [getpos("'<"), getpos("'>")], string([command, mark, method]))
      endfor
    endfor
    execute 'normal! ' .. command
    delmarks <>
    call assert_equal([[0, 0, 0, 0], [0, 0, 0, 0]],
          \ [getpos("'<"), getpos("'>")])
    call setpos("'<", [0, 1, 7, 0])
    call setpos("'>", [0, 1, 1, 0])
    call setpos("'<", [0, 1, 4, 0])
    call assert_equal([[0, 1, 1, 0], [0, 1, 4, 0]],
          \ [getpos("'<"), getpos("'>")])
  endfor

  " delmarks! does not clear Visual marks or their endpoint identities.
  execute "normal! gg0v6lo\<Esc>"
  delmarks!
  call setpos("'<", [0, 1, 8, 0])
  call assert_equal([[0, 1, 7, 0], [0, 1, 8, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'>", [0, 1, 10, 0])
  call assert_equal([[0, 1, 8, 0], [0, 1, 10, 0]],
        \ [getpos("'<"), getpos("'>")])

  " Equal endpoints bind '<' to the anchor and '>' to the cursor.
  execute "normal! gg03lv\<Esc>"
  call setpos("'<", [0, 1, 6, 0])
  call setpos("'>", [0, 1, 8, 0])
  call assert_equal([[0, 1, 6, 0], [0, 1, 8, 0]],
        \ [getpos("'<"), getpos("'>")])
  normal! gvy
  call assert_equal('fgh', getreg('"'))
  bwipe!
endfunc

func Test_visual_setcharpos_crossing()
  new
  call setline(1, 'aβcδεzqt')
  for command in ["gg0v4l\<Esc>", "gg0v4lo\<Esc>"]
    execute 'normal! ' .. command
    call setcharpos("'<", [0, 1, 6, 0])
    call setcharpos("'>", [0, 1, 7, 0])
    call assert_equal([[0, 1, 6, 0], [0, 1, 7, 0]],
          \ [getcharpos("'<"), getcharpos("'>")])
    call assert_equal([[0, 1, 9, 0], [0, 1, 10, 0]],
          \ [getpos("'<"), getpos("'>")])
    normal! gvy
    call assert_equal('zq', getreg('"'))
  endfor
  bwipe!
endfunc

func Test_visual_setpos_virtual_identity()
  let save_ve = &virtualedit
  defer execute('let &virtualedit = ' .. string(save_ve))
  set virtualedit=all
  new
  call setline(1, 'abc')
  for command in ["gg$v2l\<Esc>", "gg$v2lo\<Esc>"]
    execute 'normal! ' .. command
    call setpos("'<", [0, 1, 3, 3])
    call setpos("'>", [0, 1, 3, 5])
    call assert_equal([[0, 1, 3, 3], [0, 1, 3, 5]],
          \ [getpos("'<"), getpos("'>")])
    normal! gv
    call assert_equal([[0, 1, 3, 3], [0, 1, 3, 5]],
          \ command =~ 'o' ? [getpos('.'), getpos('v')]
          \ : [getpos('v'), getpos('.')])
    normal! y
    call assert_equal('c', getreg('"'))
  endfor
  bwipe!
endfunc

func Test_visual_setpos_line_and_block()
  new
  call setline(1, repeat(['abcdefghijk'], 5))
  for command in ["ggV2j\<Esc>", "ggV2jo\<Esc>"]
    execute 'normal! ' .. command
    call setpos("'<", [0, 4, 5, 0])
    call setpos("'>", [0, 5, 2, 0])
    call assert_equal([[0, 4, 1, 0], [0, 5, v:maxcol, 0]],
          \ [getpos("'<"), getpos("'>")])
    normal! gvy
    call assert_equal("abcdefghijk\nabcdefghijk\n", getreg('"'))
  endfor
  for corners in [[1, 2, 3, 6], [1, 6, 3, 2],
        \ [3, 2, 1, 6], [3, 6, 1, 2]]
    call cursor(corners[0], corners[1])
    execute "normal! \<C-V>" .. corners[2] .. 'G0'
          \ .. repeat('l', corners[3] - 1) .. "\<Esc>"
    call setpos("'<", [0, 4, 5, 0])
    call setpos("'>", [0, 5, 2, 0])
    call assert_equal([[0, 4, 5, 0], [0, 5, 2, 0]],
          \ [getpos("'<"), getpos("'>")], string(corners))
    normal! gvy
    call assert_equal("bcde\nbcde", getreg('"'), string(corners))
  endfor
  bwipe!
endfunc

func Test_visual_setpos_new_selection_identity()
  new
  call setline(1, 'abcdefghijk')
  execute "normal! gg0v6lo\<Esc>"
  call feedkeys('gg02lv2l', 'xt')
  call assert_equal('v', mode())
  call setpos("'<", [0, 1, 8, 0])
  call setpos("'>", [0, 1, 10, 0])
  call assert_equal([[0, 1, 8, 0], [0, 1, 10, 0]],
        \ [getpos("'<"), getpos("'>")])
  call feedkeys("\<Esc>", 'xt')
  call assert_equal([[0, 1, 3, 0], [0, 1, 5, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'<", [0, 1, 6, 0])
  call assert_equal([[0, 1, 5, 0], [0, 1, 6, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'>", [0, 1, 9, 0])
  normal! gvy
  call assert_equal('fghi', getreg('"'))
  bwipe!
endfunc

func Test_visual_setpos_active_gv_identity()
  new
  call setline(1, 'abcdefghijk')
  execute "normal! gg0v6l\<Esc>"
  call feedkeys('gg08lv3hgv', 'xt')
  call assert_equal('v', mode())
  call assert_equal([0, 1, 1, 0], getpos('v'))
  call assert_equal([0, 1, 7, 0], getpos('.'))
  call assert_equal([[0, 1, 6, 0], [0, 1, 9, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'<", [0, 1, 10, 0])
  call assert_equal([[0, 1, 9, 0], [0, 1, 10, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'>", [0, 1, 11, 0])
  call feedkeys('gv', 'xt')
  call assert_equal([0, 1, 11, 0], getpos('v'))
  call assert_equal([0, 1, 10, 0], getpos('.'))
  call assert_equal([[0, 1, 1, 0], [0, 1, 7, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'<", [0, 1, 8, 0])
  call assert_equal([[0, 1, 7, 0], [0, 1, 8, 0]],
        \ [getpos("'<"), getpos("'>")])
  call feedkeys("\<Esc>", 'xt')
  bwipe!
endfunc

func Test_visual_setpos_put_and_operator()
  new
  call setline(1, 'abcdefghijk')
  for command in ['gg0v6ly', 'gg0v6loy']
    execute 'normal! ' .. command
    call setpos("'<", [0, 1, 8, 0])
    call assert_equal([[0, 1, 7, 0], [0, 1, 8, 0]],
          \ [getpos("'<"), getpos("'>")], command)
    call setpos("'>", [0, 1, 10, 0])
    normal! gvy
    call assert_equal('hij', getreg('"'), command)
  endfor

  call setreg('a', 'XYZ')
  normal! gg02lv2lo"ap
  call assert_equal('abXYZfghijk', getline(1))
  call assert_equal([[0, 1, 3, 0], [0, 1, 5, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'<", [0, 1, 6, 0])
  call assert_equal([[0, 1, 5, 0], [0, 1, 6, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'>", [0, 1, 8, 0])
  normal! gvy
  call assert_equal('fgh', getreg('"'))
  bwipe!
endfunc

func Test_visual_setpos_adjust_identity()
  new
  call setline(1, ['a', 'b', 'c', 'd', 'e', 'f'])
  execute "normal! 3G0vgg\<Esc>"
  call setpos("'<", [0, 4, 1, 0])
  call setpos("'>", [0, 5, 1, 0])
  call append(0, 'pad')
  call assert_equal([[0, 5, 1, 0], [0, 6, 1, 0]],
        \ [getpos("'<"), getpos("'>")])
  call setpos("'<", [0, 3, 1, 0])
  call assert_equal([[0, 3, 1, 0], [0, 6, 1, 0]],
        \ [getpos("'<"), getpos("'>")])
  normal! gvy
  call assert_equal("b\nc\nd\ne", getreg('"'))
  bwipe!
endfunc

func Test_visual_setpos_target_buffer()
  new
  call setline(1, 'abcdefghijk')
  let first = bufnr()
  let firstwin = win_getid()
  execute "normal! gg0v6lo\<Esc>"
  new
  call setline(1, 'abcdefghijk')
  let second = bufnr()
  let secondwin = win_getid()
  execute "normal! gg0v6l\<Esc>"
  call setpos("'<", [first, 1, 8, 0])
  call win_gotoid(firstwin)
  call assert_equal([[0, 1, 7, 0], [0, 1, 8, 0]],
        \ [getpos("'<"), getpos("'>")])
  call win_gotoid(secondwin)
  call setpos("'>", [first, 1, 10, 0])
  call assert_equal([[0, 1, 1, 0], [0, 1, 7, 0]],
        \ [getpos("'<"), getpos("'>")])
  call win_gotoid(firstwin)
  call assert_equal([[0, 1, 8, 0], [0, 1, 10, 0]],
        \ [getpos("'<"), getpos("'>")])
  normal! gvy
  call assert_equal('hij', getreg('"'))
  execute 'bwipe! ' .. first
  execute 'bwipe! ' .. second
endfunc

func Test_setpos()
  new Xone
  let onebuf = bufnr('%')
  let onewin = win_getid()
  call setline(1, ['aaa', 'bbb', 'ccc'])
  new Xtwo
  let twobuf = bufnr('%')
  let twowin = win_getid()
  call setline(1, ['aaa', 'bbb', 'ccc'])

  " setpos() uses the same buffer-relative visual marks as getpos()
  new Xvisual
  call setline(1, 'hello world')
  for normal_cmd in ["normal! gg0vw\<Esc>", "normal! gg0vwo\<Esc>"]
    execute normal_cmd
    call setpos("'<", [0, 1, 2, 0])
    call assert_equal([[0, 1, 2, 0], [0, 1, 7, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)
    call setpos("'>", [0, 1, 6, 0])
    call assert_equal([[0, 1, 2, 0], [0, 1, 6, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)

    execute normal_cmd
    call setpos("'>", [0, 1, 6, 0])
    call assert_equal([[0, 1, 1, 0], [0, 1, 6, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)

    " Crossings preserve setter identities while getters stay ordered.
    call setpos("'<", [0, 1, 8, 0])
    call assert_equal([[0, 1, 6, 0], [0, 1, 8, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)
    call setpos("'>", [0, 1, 10, 0])
    call assert_equal([[0, 1, 8, 0], [0, 1, 10, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)
    call setpos("'>", [0, 1, 4, 0])
    call assert_equal([[0, 1, 4, 0], [0, 1, 8, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)
    call setpos("'<", [0, 1, 2, 0])
    call assert_equal([[0, 1, 2, 0], [0, 1, 4, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)

    " Deleting and recreating either end preserves the other end.
    execute normal_cmd
    call setpos("'<", [0, 0, 0, 0])
    call assert_equal([[0, 1, 7, 0], [0, 1, 7, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)
    call setpos("'<", [0, 1, 2, 0])
    call assert_equal([[0, 1, 2, 0], [0, 1, 7, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)
    normal! gvy
    call assert_equal('ello w', getreg('"'), normal_cmd)

    execute normal_cmd
    call setpos("'>", [0, 0, 0, 0])
    call assert_equal([[0, 1, 1, 0], [0, 1, 1, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)
    call setpos("'>", [0, 1, 6, 0])
    call assert_equal([[0, 1, 1, 0], [0, 1, 6, 0]],
          \ [getpos("'<"), getpos("'>")], normal_cmd)
    normal! gvy
    call assert_equal('hello ', getreg('"'), normal_cmd)
  endfor

  " While Visual mode is active the marks still refer to the last completed
  " selection.  Leaving Visual mode replaces them with the current selection.
  execute "normal! gg0vwo\<Esc>"
  call feedkeys("gg02lv2l", 'xt')
  call assert_equal('v', mode())
  call setpos("'<", [0, 1, 2, 0])
  call assert_equal([[0, 1, 2, 0], [0, 1, 7, 0]],
        \ [getpos("'<"), getpos("'>")])
  call feedkeys("\<Esc>", 'xt')
  call assert_equal([[0, 1, 3, 0], [0, 1, 5, 0]],
        \ [getpos("'<"), getpos("'>")])

  " Moving one logical mark past the other must not discard the old range.
  %delete _
  call setline(1, ['one', 'two', 'three'])
  normal! 2GVjy
  call setpos("'>", [0, 1, 1, 0])
  call assert_equal([[0, 1, 1, 0], [0, 2, v:maxcol, 0]],
        \ [getpos("'<"), getpos("'>")])
  '<,'>d
  call assert_equal(['three'], getline(1, '$'))

  " Check crossing the line boundary in the other direction as well.
  call setline(1, ['one', 'two', 'three'])
  normal! ggVjy
  call setpos("'<", [0, 3, 1, 0])
  call assert_equal([[0, 2, 1, 0], [0, 3, v:maxcol, 0]],
        \ [getpos("'<"), getpos("'>")])
  '<,'>d
  call assert_equal(['one'], getline(1, '$'))
  bwipe!

  " visual marks can still be initialized independently
  new Xvisual
  call setline(1, 'hello world')
  call setpos("'<", [0, 1, 2, 0])
  call setpos("'>", [0, 1, 4, 0])
  call assert_equal([[0, 1, 2, 0], [0, 1, 4, 0]],
        \ [getpos("'<"), getpos("'>")])
  bwipe!

  " setcharpos() uses the same visual mark path
  new Xvisual
  call setline(1, 'aβcδεz')
  execute "normal! gg0v$\<Esc>"
  call setcharpos("'<", [0, 1, 2, 0])
  call setcharpos("'>", [0, 1, 5, 0])
  call assert_equal([[0, 1, 2, 0], [0, 1, 5, 0]],
        \ [getcharpos("'<"), getcharpos("'>")])
  bwipe!

  " for the cursor the buffer number is ignored
  call setpos(".", [0, 2, 1, 0])
  call assert_equal([0, 2, 1, 0], getpos("."))
  call setpos(".", [onebuf, 3, 3, 0])
  call assert_equal([0, 3, 3, 0], getpos("."))

  call setpos("''", [0, 1, 3, 0])
  call assert_equal([0, 1, 3, 0], getpos("''"))
  call setpos("''", [onebuf, 2, 2, 0])
  call assert_equal([0, 2, 2, 0], getpos("''"))

  " buffer-local marks
  for mark in ["'a", "'\"", "'[", "']", "'<", "'>"]
    call win_gotoid(twowin)
    call setpos(mark, [0, 2, 1, 0])
    call assert_equal([0, 2, 1, 0], getpos(mark), "for mark " . mark)
    call setpos(mark, [onebuf, 1, 3, 0])
    call win_gotoid(onewin)
    call assert_equal([0, 1, 3, 0], getpos(mark), "for mark " . mark)
  endfor

  " global marks
  call win_gotoid(twowin)
  call setpos("'N", [0, 2, 1, 0])
  call assert_equal([twobuf, 2, 1, 0], getpos("'N"))
  call setpos("'N", [onebuf, 1, 3, 0])
  call assert_equal([onebuf, 1, 3, 0], getpos("'N"))

  " try invalid column and check virtcol()
  call win_gotoid(onewin)
  call setpos("'a", [0, 1, 2, 0])
  call assert_equal([0, 1, 2, 0], getpos("'a"))
  call setpos("'a", [0, 1, -5, 0])
  call assert_equal([0, 1, 2, 0], getpos("'a"))
  call setpos("'a", [0, 1, 0, 0])
  call assert_equal([0, 1, 1, 0], getpos("'a"))
  call setpos("'a", [0, 1, 4, 0])
  call assert_equal([0, 1, 4, 0], getpos("'a"))
  call assert_equal(4, virtcol("'a"))
  call setpos("'a", [0, 1, 5, 0])
  call assert_equal([0, 1, 5, 0], getpos("'a"))
  call assert_equal(4, virtcol("'a"))
  call setpos("'a", [0, 1, 21341234, 0])
  call assert_equal([0, 1, 21341234, 0], getpos("'a"))
  call assert_equal(4, virtcol("'a"))

  " Test with invalid buffer number, line number and column number
  call cursor(2, 2)
  call setpos('.', [-1, 1, 1, 0])
  call assert_equal([2, 2], [line('.'), col('.')])
  call setpos('.', [0, -1, 1, 0])
  call assert_equal([2, 2], [line('.'), col('.')])
  call setpos('.', [0, 1, -1, 0])
  call assert_equal([2, 2], [line('.'), col('.')])

  call assert_fails("call setpos('ab', [0, 1, 1, 0])", 'E474:')

  bwipe!
  call win_gotoid(twowin)
  bwipe!
endfunc

func Test_marks_cmd()
  new Xone
  call setline(1, ['aaa', 'bbb'])
  norm! maG$mB
  w!
  new Xtwo
  call setline(1, ['ccc', 'ddd'])
  norm! $mcGmD
  exe "norm! GVgg\<Esc>G"
  w!

  b Xone
  let a = split(execute('marks'), "\n")
  call assert_equal(9, len(a))
  call assert_equal(['mark line  col file/text',
        \ " '      2    0 bbb",
        \ ' a      1    0 aaa',
        \ ' B      2    2 bbb',
        \ ' D      2    0 Xtwo',
        \ ' "      1    0 aaa',
        \ ' [      1    0 aaa',
        \ ' ]      2    0 bbb',
        \ ' .      2    0 bbb'], a)

  b Xtwo
  let a = split(execute('marks'), "\n")
  call assert_equal(11, len(a))
  call assert_equal(['mark line  col file/text',
        \ " '      1    0 ccc",
        \ ' c      1    2 ccc',
        \ ' B      2    2 Xone',
        \ ' D      2    0 ddd',
        \ ' "      2    0 ddd',
        \ ' [      1    0 ccc',
        \ ' ]      2    0 ddd',
        \ ' .      2    0 ddd',
        \ ' <      1    0 ccc',
        \ ' >      2    0 ddd'], a)
  norm! Gdd
  w!
  let a = split(execute('marks <>'), "\n")
  call assert_equal(3, len(a))
  call assert_equal(['mark line  col file/text',
        \ ' <      1    0 ccc',
        \ ' >      2    0 -invalid-'], a)

  b Xone
  delmarks aB
  let a = split(execute('marks aBcD'), "\n")
  call assert_equal(2, len(a))
  call assert_equal('mark line  col file/text', a[0])
  call assert_equal(' D      2    0 Xtwo', a[1])

  b Xtwo
  delmarks cD
  call assert_fails('marks aBcD', 'E283:')

  call delete('Xone')
  call delete('Xtwo')
  %bwipe
endfunc

func Test_marks_cmd_multibyte()
  new Xone
  call setline(1, [repeat('á', &columns)])
  norm! ma

  let a = split(execute('marks a'), "\n")
  call assert_equal(2, len(a))
  let expected = ' a      1    0 ' . repeat('á', &columns - 16)
  call assert_equal(expected, a[1])

  bwipe!
endfunc

func Test_delmarks()
  new
  norm mx
  norm `x
  delmarks x
  call assert_fails('norm `x', 'E20:')

  " Deleting an already deleted mark should not fail.
  delmarks x

  " getpos() should return all zeros after deleting a filemark.
  norm mA
  delmarks A
  call assert_equal([0, 0, 0, 0], getpos("'A"))

  " Test deleting a range of marks.
  norm ma
  norm mb
  norm mc
  norm mz
  delmarks b-z
  norm `a
  call assert_fails('norm `b', 'E20:')
  call assert_fails('norm `c', 'E20:')
  call assert_fails('norm `z', 'E20:')
  call assert_fails('delmarks z-b', 'E475:')

  call assert_fails('delmarks', 'E471:')
  call assert_fails('delmarks /', 'E475:')

  " Test delmarks!
  norm mx
  norm `x
  delmarks!
  call assert_fails('norm `x', 'E20:')
  call assert_fails('delmarks! x', 'E474:')

  bwipe!
endfunc

func Test_mark_error()
  call assert_fails('mark', 'E471:')
  call assert_fails('mark xx', 'E488:')
  call assert_fails('mark _', 'E191:')
  call assert_beeps('normal! m~')

  call setpos("'k", [0, 100, 1, 0])
  call assert_fails("normal 'k", 'E19:')
endfunc

" Test for :lockmarks when pasting content
func Test_lockmarks_with_put()
  new
  call append(0, repeat(['sky is blue'], 4))
  normal gg
  1,2yank r
  put r
  normal G
  lockmarks put r
  call assert_equal(2, line("'["))
  call assert_equal(3, line("']"))

  bwipe!
endfunc

" Test for :k command to set a mark
func Test_marks_k_cmd()
  new
  call setline(1, ['foo', 'bar', 'baz', 'qux'])
  1,3kr
  call assert_equal([0, 3, 1, 0], getpos("'r"))
  " whitespace before mark
  4k f
  call assert_equal([0, 4, 1, 0], getpos("'f"))
  :2     k	 g
  call assert_equal([0, 2, 1, 0], getpos("'g"))
  bw!
  call assert_fails(':kz7', 'E488: Trailing characters: z7')
  call assert_fails(':execute ":k^"', 'E191: Argument must be a letter or forward/backward quote')
endfunc

" Test for file marks (A-Z)
func Test_file_mark()
  new Xone
  call setline(1, ['aaa', 'bbb'])
  norm! G$mB
  w!
  new Xtwo
  call setline(1, ['ccc', 'ddd'])
  norm! GmD
  w!

  enew
  normal! `B
  call assert_equal('Xone', bufname())
  call assert_equal([2, 3], [line('.'), col('.')])
  normal! 'D
  call assert_equal('Xtwo', bufname())
  call assert_equal([2, 1], [line('.'), col('.')])

  call delete('Xone')
  call delete('Xtwo')
endfunc

" Test for the getmarklist() function
func Test_getmarklist()
  new
  " global marks
  delmarks A-Z 0-9 \" ^.[]
  call assert_equal([], getmarklist())
  call setline(1, ['one', 'two', 'three'])
  mark A
  call cursor(3, 5)
  normal mN
  call assert_equal([{'file' : '', 'mark' : "'A", 'pos' : [bufnr(), 1, 1, 0]},
        \ {'file' : '', 'mark' : "'N", 'pos' : [bufnr(), 3, 5, 0]}],
        \ getmarklist())
  " buffer local marks
  delmarks!
  call assert_equal([{'mark' : "''", 'pos' : [bufnr(), 1, 1, 0]},
        \ {'mark' : "'\"", 'pos' : [bufnr(), 1, 1, 0]}], getmarklist(bufnr()))
  call cursor(2, 2)
  normal mr
  call assert_equal({'mark' : "'r", 'pos' : [bufnr(), 2, 2, 0]},
        \ bufnr()->getmarklist()[0])
  call assert_equal([], {}->getmarklist())
  normal! yy
  call assert_equal([
        \ {'mark': "'[", 'pos': [bufnr(), 2, 1, 0]},
        \ {'mark': "']", 'pos': [bufnr(), 2, v:maxcol, 0]},
        \ ], getmarklist(bufnr())[-2:])
  bw!
endfunc

" This was using freed memory
func Test_jump_mark_autocmd()
  next 00
  edit 0
  sargument
  au BufEnter 0 all
  sil norm 

  au! BufEnter
  bwipe!
endfunc

func Test_mark_formatprg_on_empty()
  new
  if has('win32')
    setl formatprg=more
  else
    setl formatprg=cat
  endif
  call assert_equal([0, 0], [line("'["), col("'[")])
  call assert_equal([0, 0], [line("']"), col("']")])
  let v:errmsg = ''
  try
    norm! gqG
  catch
    call assert_report('gqG on empty buffer should not fail: ' .. v:exception)
  endtry
  call assert_true(empty(v:errmsg))
  " col() is 1-based, so 1 == first column (:marks shows it as 0)
  call assert_equal([1, 1], [line("'["), col("'[")])
  call assert_equal([1, 1], [line("']"), col("']")])
  bwipe!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
