" Test commands that are not compiled in a :def function

source check.vim
source vim9.vim

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

def Test_assign_list()
  let l: list<string> = []
  l[0] = 'value'
  assert_equal('value', l[0])

  l[1] = 'asdf'
  assert_equal('value', l[0])
  assert_equal('asdf', l[1])
  assert_equal('asdf', l[-1])
  assert_equal('value', l[-2])

  let nrl: list<number> = []
  for i in range(5)
    nrl[i] = i
  endfor
  assert_equal([0, 1, 2, 3, 4], nrl)
enddef

def Test_assign_dict()
  let d: dict<string> = {}
  d['key'] = 'value'
  assert_equal('value', d['key'])

  d[123] = 'qwerty'
  assert_equal('qwerty', d[123])
  assert_equal('qwerty', d['123'])

  let nrd: dict<number> = {}
  for i in range(3)
    nrd[i] = i
  endfor
  assert_equal({'0': 0, '1': 1, '2': 2}, nrd)
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

def Test_method_cal_linebreak()
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


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
