" Test for pyx* commands and functions with Python 2.

set pyx=2
source check.vim
CheckFeature python

let s:py2pattern = '^2\.[0-7]\.\d\+'
let s:py3pattern = '^3\.\d\+\.\d\+'


func Test_has_pythonx()
  call assert_true(has('pythonx'))
endfunc


func Test_pyx()
  redir => var
  pyx << trim EOF
    import sys
    print(sys.version)
  EOF
  redir END
  call assert_match(s:py2pattern, split(var)[0])
endfunc


func Test_pyxdo()
  pyx import sys
  enew
  pyxdo return sys.version.split("\n")[0]
  call assert_match(s:py2pattern, split(getline('.'))[0])
endfunc


func Test_pyxeval()
  pyx import sys
  call assert_match(s:py2pattern, split('sys.version'->pyxeval())[0])
endfunc


" Test for pyxeval with locals
func Test_python_pyeval_locals()
  let str = 'a string'
  let num = 0xbadb33f
  let d = {'a': 1, 'b': 2, 'c': str}
  let l = [ str, num, d ]

  let locals = #{
        \ s: str,
        \ n: num,
        \ d: d,
        \ l: l,
        \ }

  " check basics
  call assert_equal('a string', pyxeval('s', locals))
  call assert_equal(0xbadb33f, pyxeval('n', locals))
  call assert_equal(d, pyxeval('d', locals))
  call assert_equal(l, pyxeval('l', locals))

  py << trim EOF
  def __UpdateDict(d, upd):
    d.update(upd)
    return d

  def __ExtendList(l, *args):
    l.extend(*args)
    return l
  EOF

  " check assign to dict member works like bindeval
  call assert_equal(3, pyxeval('__UpdateDict( d, {"c": 3} )["c"]', locals))
  call assert_equal(3, d['c'])

  " check append lo list
  call assert_equal(4, pyxeval('len(__ExtendList(l, ["new item"]))', locals))
  call assert_equal("new item", l[-1])

  " check calling a function
  let StrLen = function('strlen')
  call assert_equal(3, pyxeval('f("abc")', {'f': StrLen}))
endfunc

func Test_pyxfile()
  " No special comments nor shebangs
  redir => var
  pyxfile pyxfile/pyx.py
  redir END
  call assert_match(s:py2pattern, split(var)[0])

  " Python 2 special comment
  redir => var
  pyxfile pyxfile/py2_magic.py
  redir END
  call assert_match(s:py2pattern, split(var)[0])

  " Python 2 shebang
  redir => var
  pyxfile pyxfile/py2_shebang.py
  redir END
  call assert_match(s:py2pattern, split(var)[0])

  if has('python3')
    " Python 3 special comment
    redir => var
    pyxfile pyxfile/py3_magic.py
    redir END
    call assert_match(s:py3pattern, split(var)[0])

    " Python 3 shebang
    redir => var
    pyxfile pyxfile/py3_shebang.py
    redir END
    call assert_match(s:py3pattern, split(var)[0])
  endif
endfunc

func Test_Catch_Exception_Message()
  try
    pyx raise RuntimeError( 'TEST' )
  catch /.*/
    call assert_match( '^Vim(.*):RuntimeError: TEST$', v:exception )
  endtry
endfunc

" Test for various heredoc syntaxes
func Test_pyx2_heredoc()
  pyx << END
result='A'
END
  pyx <<
result+='B'
.
  pyx << trim END
    result+='C'
  END
  pyx << trim
    result+='D'
  .
  pyx << trim eof
    result+='E'
  eof
  pyx << trimm
result+='F'
trimm
  call assert_equal('ABCDEF', pyxeval('result'))
endfunc

" vim: shiftwidth=2 sts=2 expandtab
