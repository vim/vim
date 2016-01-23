"Tests for :olffiles  vim: setf=vim :

func Test_oldfiles()
  let ol = ['a.vim', 'b.vim', 'c.txt', 'd.txt']
  call extend(v:oldfiles, ol)
  redir @q
  ol
  return
  redir END
  call assert_equal(split(@q, "\n"), ['1: a.vim', '2: b.vim', '3: c.txt', '4: d.txt'])
  redir @q
  ol /\.txt$/
  redir END
  call assert_equal(split(@q, "\n"), ['3: c.txt', '4: d.txt'])
  redir q
  ol /b[^/]*$/
  redir END
  call assert_equal(split(@q, "\n"), ['2: b.vim'])
endfunc
