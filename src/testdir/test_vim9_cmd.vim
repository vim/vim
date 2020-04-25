" Test commands that are not compiled in a :def function

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


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
