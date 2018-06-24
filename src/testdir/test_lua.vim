" Tests for Lua.
" TODO: move tests from test85.in here.

if !has('lua')
  finish
endif

func Test_luado()
  new
  call setline(1, ['one', 'two', 'three'])
  luado vim.command("%d_")
  bwipe!

  " Check switching to another buffer does not trigger ml_get error.
  new
  let wincount = winnr('$')
  call setline(1, ['one', 'two', 'three'])
  luado vim.command("new")
  call assert_equal(wincount + 1, winnr('$'))
  bwipe!
  bwipe!
endfunc

" Test vim.window().height
func Test_window_height()
  new
  lua vim.window().height = 2
  call assert_equal(2, winheight(0))
  lua vim.window().height = vim.window().height + 1
  call assert_equal(3, winheight(0))
  bwipe!
endfunc

" Test vim.window().width
func Test_window_width()
  vert new
  lua vim.window().width = 2
  call assert_equal(2, winwidth(0))
  lua vim.window().width = vim.window().width + 1
  call assert_equal(3, winwidth(0))
  bwipe!
endfunc

" Test vim.window().line and vim.window.col
func Test_window_line_col()
  new
  call setline(1, ['line1', 'line2', 'line3'])
  lua vim.window().line = 2
  lua vim.window().col = 4
  call assert_equal([0, 2, 4, 0], getpos('.'))
  lua vim.window().line = vim.window().line + 1
  lua vim.window().col = vim.window().col - 1
  call assert_equal([0, 3, 3, 0], getpos('.'))
  bwipe!
endfunc

" Test setting the current window
func Test_window_set_current()
  new Xfoo1
  lua w1 = vim.window()
  new Xfoo2
  lua w2 = vim.window()

  call assert_equal('Xfoo2', bufname('%'))
  lua w1()
  call assert_equal('Xfoo1', bufname('%'))
  lua w2()
  call assert_equal('Xfoo2', bufname('%'))

  lua w1 = nil
  lua w2 = nil
  %bwipe!
endfunc

" Test vim.window().buffer
func Test_window_buffer()
  new Xfoo1
  lua w1 = vim.window()
  lua b1 = w1.buffer()
  new Xfoo2
  lua w2 = vim.window()
  lua b2 = w2.buffer()

  lua b1()
  call assert_equal('Xfoo1', bufname('%'))
  lua b2()
  call assert_equal('Xfoo2', bufname('%'))

  lua b1 = nil
  lua b2 = nil
  %bwipe!
endfunc

" Test vim.window():isvalid()
func Test_window_isvalid()
  new Xfoo
  lua w = vim.window()
  call assert_true(luaeval('w:isvalid()'))

  " FIXME: how to test the case isvalid() returning v:false?
  " isvalid() gives errors when the window is deleted. Is it a bug?

  lua w = nil
  bwipe!
endfunc

" Test vim.buffer().name and vim.buffer().fname
func Test_buffer_name()
  new
  " FIXME: for an unnamed buffer, I would expect
  " vim.buffer().name to give an empty string, but
  " it returns 0. Is it a bug?
  " so this assert_equal is commented out.
  " call assert_equal('', luaeval('vim.buffer().name'))
  bwipe!
  new Xfoo
  call assert_equal('Xfoo', luaeval('vim.buffer().name'))
  call assert_equal(expand('%:p'), luaeval('vim.buffer().fname'))
  bwipe!
endfunc

" Test vim.buffer().number
func Test_buffer_number()
  " All numbers in Lua are floating points number (no integers).
  call assert_equal(bufnr('%'), float2nr(luaeval('vim.buffer().number')))
endfunc

" Test inserting lines in buffer.
func Test_buffer_insert()
  new
  lua vim.buffer()[1] = '3'
  lua vim.buffer():insert('1', 0)
  lua vim.buffer():insert('2', 1)
  lua vim.buffer():insert('4', 10)

  call assert_equal(['1', '2', '3', '4'], getline(1, '$'))
  bwipe!
endfunc

" Test deleting line in buffer
func Test_buffer_delete()
  new
  call setline(1, ['1', '2', '3'])
  lua vim.buffer()[2] = nil
  call assert_equal(['1', '3'], getline(1, '$'))
  bwipe!
endfunc

" Test #vim.buffer() i.e. number of lines in buffer
func Test_buffer_number_lines()
  new
  call setline(1, ['a', 'b', 'c'])
  call assert_equal(3.0, luaeval('#vim.buffer()'))
  bwipe!
endfunc

" Test vim.buffer():next() and vim.buffer():previous()
" Note that these functions get the next or previous buffers
" but do not switch buffer.
func Test_buffer_next_previous()
  new Xfoo1
  new Xfoo2
  new Xfoo3
  b Xfoo2

  lua bn = vim.buffer():next()
  lua bp = vim.buffer():previous()

  call assert_equal('Xfoo2', luaeval('vim.buffer().name'))
  call assert_equal('Xfoo1', luaeval('bp.name'))
  call assert_equal('Xfoo3', luaeval('bn.name'))

  call assert_equal('Xfoo2', bufname('%'))

  lua bn()
  call assert_equal('Xfoo3', luaeval('vim.buffer().name'))
  call assert_equal('Xfoo3', bufname('%'))

  lua bp()
  call assert_equal('Xfoo1', luaeval('vim.buffer().name'))
  call assert_equal('Xfoo1', bufname('%'))

  lua bn = nil
  lua bp = nil
  %bwipe!
endfunc

" Test vim.buffer():isvalid()
func Test_buffer_isvalid()
  new Xfoo
  lua b = vim.buffer()
  call assert_true(luaeval('b:isvalid()'))

  " FIXME: how to test the case isvalid() returning v:false?
  " isvalid() gives errors when the buffer is wiped. Is it a bug?

  lua b = nil
  bwipe!
endfunc

func Test_list()
  call assert_equal([], luaeval('vim.list()'))

  " Same example as in :help lua-vim.
  " FIXME: test is disabled because it does not work.
  " See https://github.com/vim/vim/issues/3086
  " lua t = {math.pi, false, say = 'hi'}
  " call assert_equal([3.141593, 0], luaeval('vim.list(t)'))
 
  let l = []
  lua l = vim.eval('l')
  lua l:add(123)
  lua l:add('abc')
  lua l:add(true)
  lua l:add(false)
  lua l:add(vim.eval("[1, 2, 3]"))
  lua l:add(vim.eval("{'a':1, 'b':2, 'c':3}"))
  call assert_equal([123.0, 'abc', v:true, v:false, [1, 2, 3], {'a': 1, 'b': 2, 'c': 3}], l)

  lua l[0] = 124
  lua l[4] = nil
  lua l:insert('first')
  call assert_equal(['first', 124.0, 'abc', v:true, v:false, {'a': 1, 'b': 2, 'c': 3}], l)

  lua l = nil
endfunc

func Test_dict()
  call assert_equal({}, luaeval('vim.dict()'))

  " Same example as in :help lua-vim.
  " FIXME: test is disabled because it does not work.
  " See https://github.com/vim/vim/issues/3086
  " lua t = {math.pi, false, say = 'hi'}
  " call assert_equal({'say' : 'hi'}, luaeval('vim.dict(t)'))

  let d = {}
  lua d = vim.eval('d')
  lua d[0] = 123
  lua d[1] = "abc"
  lua d[2] = true
  lua d[3] = false
  lua d[4] = vim.eval("[1, 2, 3]")
  lua d[5] = vim.eval("{'a':1, 'b':2, 'c':3}")
  call assert_equal({'0':123.0, '1':'abc', '2':v:true, '3':v:false, '4': [1, 2, 3], '5': {'a':1, 'b':2, 'c':3}}, d)

  lua d[0] = 124
  lua d[4] = nil
  call assert_equal({'0':124.0, '1':'abc', '2':v:true, '3':v:false, '5': {'a':1, 'b':2, 'c':3}}, d)

  lua d = nil
endfunc

func Test_type()
  " The following values are identical to Lua's type function.
  call assert_equal('string',   luaeval('vim.type("foo")'))
  call assert_equal('number',   luaeval('vim.type(1)'))
  call assert_equal('number',   luaeval('vim.type(1.2)'))
  call assert_equal('function', luaeval('vim.type(print)'))
  call assert_equal('table',    luaeval('vim.type({})'))
  call assert_equal('boolean',  luaeval('vim.type(true)'))
  call assert_equal('boolean',  luaeval('vim.type(false)'))
  call assert_equal('nil',      luaeval('vim.type(nil)'))

  " The following values are specific to Vim.
  call assert_equal('window',   luaeval('vim.type(vim.window())'))
  call assert_equal('buffer',   luaeval('vim.type(vim.buffer())'))
  call assert_equal('list',     luaeval('vim.type(vim.list())'))
  call assert_equal('dict',     luaeval('vim.type(vim.dict())'))
endfunc

" Test vim.open('...')
func Test_open()
  let a = execute('ls')
  call assert_notmatch('XOpen', a)

  " Open a buffer XOpen1, but do not jump to it.
  lua b = vim.open('XOpen1')
  call assert_equal('XOpen1', luaeval('b.name'))
  call assert_equal('', bufname('%'))

  let a = execute('ls')
  call assert_match('XOpen1', a)

  call assert_notequal('XOpen2', bufname('%'))

  " Open a buffer XOpen2 and jump to it.
  lua b = vim.open('XOpen2')()
  call assert_equal('XOpen2', luaeval('b.name'))
  call assert_equal('XOpen2', bufname('%'))

  lua b = nil
  bwipe%
endfunc
