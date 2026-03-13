" Tests for :dirmark

vim9script

def ListMarks(): list<string>
  return execute(':dirmark')->trim()->split('[ \t\n]\+')
enddef

def AssertMarks(want: list<string>)
  assert_equal(want, ListMarks())
enddef

# Test :dirmark and :undirmark
def g:Test_dirmark()
  :mapclear
  :mapclear!

  :dirmark
  AssertMarks(['No', 'dirmark', 'found'])

  # :dirmark a directory, make sure :dirmark reports it.
  :dirmark dir /hello/world
  AssertMarks(['dir', '/hello/world'])

  # Add more, and remove the previous.
  :dirmark s32 C:\Windows\system32
  :dirmark vim /usr/share/vim/vim92
  :undirmark dir
  AssertMarks(['vim', '/usr/share/vim/vim92', 's32', 'C:\Windows\system32'])

  # Make sure :undirmark errors for unknown marks.
  var did_err = false
  try
    :undirmark xxunknownuserxx
  catch /E1573/
    did_err = true
  endtry
  assert_equal(true, did_err, 'expected E1573 error on :undirmark xxunknownuserxx')
  AssertMarks(['vim', '/usr/share/vim/vim92', 's32', 'C:\Windows\system32'])

  # Should not affect maplist()
  assert_equal([], maplist())

  :undirmark s32
  :undirmark vim
enddef

# Make sure <buffer> works as expected, and takes takes precedence over global.
def g:Test_dirmark_buffer()
  :dirmark <buffer> dir /buf
  :dirmark          dir /global

  AssertMarks(['dir', '@/buf', 'dir', '/global'])
  assert_equal('/buf/file', expandcmd('~dir/file'))

  :undirmark <buffer> dir
  AssertMarks(['dir', '/global'])
  assert_equal('/global/file', expandcmd('~dir/file'))

  :undirmark dir
enddef

# Test expand() and expandcmd()
def g:Test_dirmark_expand()
  :dirmark zxc /abc/def

  assert_equal('/abc/def', expandcmd('~zxc'))
  assert_equal('/abc/def/', expandcmd('~zxc/'))
  assert_equal('/abc/def/foo', expandcmd('~zxc/foo'))
  assert_equal('~xxunknownuserxx', expandcmd('~xxunknownuserxx'))

  assert_equal('/abc/def', expand('~zxc'))
  assert_equal('/abc/def/', expand('~zxc/'))
  assert_equal('~xxunknownuserxx', expand('~xxunknownuserxx'))

  :undirmark zxc
enddef

# Test on commandline
def g:Test_dirmark_cmdline()
  :dirmark zxc /tmp

  :next ~zxc/file
  call assert_equal('/tmp/file', expand('%'))

  :undirmark zxc
enddef

# Make sure ~ and other dirmarks can be used on the right-hand side.
def g:Test_dirmark_nest()
  :dirmark c    ~/code
  :dirmark cm   ~c/Misc

  AssertMarks([
    'cm', '~/code/Misc'->expandcmd(),
    'c',  '~/code'->expandcmd(),
  ])

  :undirmark c
  :undirmark cm
enddef

defcompile

# vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
