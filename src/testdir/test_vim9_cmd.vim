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


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
