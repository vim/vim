" Test commands that are not compiled in a :def function

source check.vim
source vim9.vim
source view_util.vim

def Test_edit_wildcards()
  let filename = 'Xtest'
  edit `=filename`
  assert_equal('Xtest', bufname())

  let filenr = 123
  edit Xtest`=filenr`
  assert_equal('Xtest123', bufname())

  filenr = 77
  edit `=filename``=filenr`
  assert_equal('Xtest77', bufname())

  edit X`=filename`xx`=filenr`yy
  assert_equal('XXtestxx77yy', bufname())
enddef

def Test_hardcopy_wildcards()
  CheckUnix
  CheckFeature postscript

  let outfile = 'print'
  hardcopy > X`=outfile`.ps
  assert_true(filereadable('Xprint.ps'))

  delete('Xprint.ps')
enddef

def Test_syn_include_wildcards()
  writefile(['syn keyword Found found'], 'Xthemine.vim')
  let save_rtp = &rtp
  &rtp = '.'

  let fname = 'mine'
  syn include @Group Xthe`=fname`.vim
  assert_match('Found.* contained found', execute('syn list Found'))

  &rtp = save_rtp
  delete('Xthemine.vim')
enddef

def Test_echo_linebreak()
  let lines =<< trim END
      vim9script
      redir @a
      echo 'one'
            .. 'two'
      redir END
      assert_equal("\nonetwo", @a)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      redir @a
      echo 11 +
            77
            - 22
      redir END
      assert_equal("\n66", @a)
  END
  CheckScriptSuccess(lines)
enddef

def Test_if_linebreak()
  let lines =<< trim END
      vim9script
      if 1 &&
            2
            || 3
        g:res = 42
      endif
      assert_equal(42, g:res)
  END
  CheckScriptSuccess(lines)
  unlet g:res

  lines =<< trim END
      vim9script
      if 1 &&
            0
        g:res = 0
      elseif 0 ||
              0
              || 1
        g:res = 12
      endif
      assert_equal(12, g:res)
  END
  CheckScriptSuccess(lines)
  unlet g:res
enddef

def Test_while_linebreak()
  let lines =<< trim END
      vim9script
      let nr = 0
      while nr <
              10 + 3
            nr = nr
                  + 4
      endwhile
      assert_equal(16, nr)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let nr = 0
      while nr
            <
              10
              +
              3
            nr = nr
                  +
                  4
      endwhile
      assert_equal(16, nr)
  END
  CheckScriptSuccess(lines)
enddef

def Test_for_linebreak()
  let lines =<< trim END
      vim9script
      let nr = 0
      for x
            in
              [1, 2, 3, 4]
          nr = nr + x
      endfor
      assert_equal(10, nr)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let nr = 0
      for x
            in
              [1, 2,
                  3, 4
                  ]
          nr = nr
                 +
                  x
      endfor
      assert_equal(10, nr)
  END
  CheckScriptSuccess(lines)
enddef

def Test_method_call_linebreak()
  let lines =<< trim END
      vim9script
      let res = []
      func RetArg(
            arg
            )
            let s:res = a:arg
      endfunc
      [1,
          2,
          3]->RetArg()
      assert_equal([1, 2, 3], res)
  END
  CheckScriptSuccess(lines)
enddef

def Test_dict_member()
   let test: dict<list<number>> = {'data': [3, 1, 2]}
   test.data->sort()
   assert_equal(#{data: [1, 2, 3]}, test)
   test.data
      ->reverse()
   assert_equal(#{data: [3, 2, 1]}, test)

  let lines =<< trim END
      vim9script
      let test: dict<list<number>> = {'data': [3, 1, 2]}
      test.data->sort()
      assert_equal(#{data: [1, 2, 3]}, test)
  END
  CheckScriptSuccess(lines)
enddef

def Test_bar_after_command()
  def RedrawAndEcho()
    let x = 'did redraw'
    redraw | echo x
  enddef
  RedrawAndEcho()
  assert_match('did redraw', Screenline(&lines))

  def CallAndEcho()
    let x = 'did redraw'
    reg_executing() | echo x
  enddef
  CallAndEcho()
  assert_match('did redraw', Screenline(&lines))

  if has('unix')
    # bar in filter write command does not start new command
    def WriteToShell()
      new
      setline(1, 'some text')
      w !cat | cat > Xoutfile
      bwipe!
    enddef
    WriteToShell()
    assert_equal(['some text'], readfile('Xoutfile'))
    delete('Xoutfile')

    # bar in filter read command does not start new command
    def ReadFromShell()
      new
      r! echo hello there | cat > Xoutfile
      r !echo again | cat >> Xoutfile
      bwipe!
    enddef
    ReadFromShell()
    assert_equal(['hello there', 'again'], readfile('Xoutfile'))
    delete('Xoutfile')
  endif
enddef

def Test_filter_is_not_modifier()
  let tags = [{'a': 1, 'b': 2}, {'x': 3, 'y': 4}]
  filter(tags, { _, v -> has_key(v, 'x') ? 1 : 0 })
  assert_equal([#{x: 3, y: 4}], tags)
enddef

def Test_eval_command()
  let from = 3
  let to = 5
  g:val = 111
  def Increment(nrs: list<number>)
    for nr in nrs
      g:val += nr
    endfor
  enddef
  eval range(from, to)
        ->Increment()
  assert_equal(111 + 3 + 4 + 5, g:val)
  unlet g:val
enddef

def Test_map_command()
  let lines =<< trim END
      nnoremap <F3> :echo 'hit F3 #'<CR>
      assert_equal(":echo 'hit F3 #'<CR>", maparg("<F3>", "n"))
  END
  CheckDefSuccess(lines)
  CheckScriptSuccess(['vim9script'] + lines)
enddef

def Test_normal_command()
  new
  setline(1, 'doesnotexist')
  let caught = 0
  try
    exe "norm! \<C-]>"
  catch /E433/
    caught = 2
  endtry
  assert_equal(2, caught)

  try
    exe "norm! 3\<C-]>"
  catch /E433/
    caught = 3
  endtry
  assert_equal(3, caught)
  bwipe!
enddef

def Test_put_command()
  new
  @p = 'ppp'
  put p
  assert_equal('ppp', getline(2))

  put ='below'
  assert_equal('below', getline(3))
  put! ='above'
  assert_equal('above', getline(3))
  assert_equal('below', getline(4))

  bwipe!
enddef

def Test_command_star_range()
  new
  setline(1, ['xxx foo xxx', 'xxx bar xxx', 'xxx foo xx bar'])
  setpos("'<", [0, 1, 0, 0])
  setpos("'>", [0, 3, 0, 0])
  :*s/\(foo\|bar\)/baz/g
  getline(1, 3)->assert_equal(['xxx baz xxx', 'xxx baz xxx', 'xxx baz xx baz'])

  bwipe!
enddef



" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
