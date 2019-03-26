" Tests for Lua.

if !has('lua')
  finish
endif

func TearDown()
  " Run garbage collection after each test to exercise luaV_setref().
  call test_garbagecollect_now()
endfunc

" Check that switching to another buffer does not trigger ml_get error.
func Test_command_new_no_ml_get_error()
  new
  let wincount = winnr('$')
  call setline(1, ['one', 'two', 'three'])
  luado vim.command("new")
  call assert_equal(wincount + 1, winnr('$'))
  %bwipe!
endfunc

" Test vim.command()
func Test_command()
  new
  call setline(1, ['one', 'two', 'three'])
  luado vim.command("1,2d_")
  call assert_equal(['three'], getline(1, '$'))
  bwipe!
endfunc

" Test vim.eval()
func Test_eval()
  " lua.eval with a number
  lua v = vim.eval('123')
  call assert_equal('number', luaeval('vim.type(v)'))
  call assert_equal(123.0, luaeval('v'))

  " lua.eval with a string
  lua v = vim.eval('"abc"')
  call assert_equal('string', luaeval('vim.type(v)'))
  call assert_equal('abc', luaeval('v'))

  " lua.eval with a list
  lua v = vim.eval("['a']")
  call assert_equal('list', luaeval('vim.type(v)'))
  call assert_equal(['a'], luaeval('v'))

  " lua.eval with a dict
  lua v = vim.eval("{'a':'b'}")
  call assert_equal('dict', luaeval('vim.type(v)'))
  call assert_equal({'a':'b'}, luaeval('v'))

  " lua.eval with a blob
  lua v = vim.eval("0z00112233.deadbeef")
  call assert_equal('blob', luaeval('vim.type(v)'))
  call assert_equal(0z00112233.deadbeef, luaeval('v'))

  call assert_fails('lua v = vim.eval(nil)',
        \ "[string \"vim chunk\"]:1: bad argument #1 to 'eval' (string expected, got nil)")
  call assert_fails('lua v = vim.eval(true)',
        \ "[string \"vim chunk\"]:1: bad argument #1 to 'eval' (string expected, got boolean)")
  call assert_fails('lua v = vim.eval({})',
        \ "[string \"vim chunk\"]:1: bad argument #1 to 'eval' (string expected, got table)")
  call assert_fails('lua v = vim.eval(print)',
        \ "[string \"vim chunk\"]:1: bad argument #1 to 'eval' (string expected, got function)")
  call assert_fails('lua v = vim.eval(vim.buffer())',
        \ "[string \"vim chunk\"]:1: bad argument #1 to 'eval' (string expected, got userdata)")

  lua v = nil
endfunc

" Test vim.window()
func Test_window()
  e Xfoo2
  new Xfoo1

  " Window 1 (top window) contains Xfoo1
  " Window 2 (bottom window) contains Xfoo2
  call assert_equal('Xfoo1', luaeval('vim.window(1):buffer().name'))
  call assert_equal('Xfoo2', luaeval('vim.window(2):buffer().name'))

  " Window 3 does not exist so vim.window(3) should return nil
  call assert_equal('nil', luaeval('tostring(vim.window(3))'))

  %bwipe!
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

  call assert_fails('lua vim.window().line = 10',
        \           '[string "vim chunk"]:1: line out of range')
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

  lua w1, w2 = nil
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

  lua b1, b2, w1, w2 = nil
  %bwipe!
endfunc

" Test vim.window():previous() and vim.window():next()
func Test_window_next_previous()
  new Xfoo1
  new Xfoo2
  new Xfoo3
  wincmd j

  call assert_equal('Xfoo2', luaeval('vim.window().buffer().name'))
  call assert_equal('Xfoo1', luaeval('vim.window():next():buffer().name'))
  call assert_equal('Xfoo3', luaeval('vim.window():previous():buffer().name'))

  %bwipe!
endfunc

" Test vim.window():isvalid()
func Test_window_isvalid()
  new Xfoo
  lua w = vim.window()
  call assert_true(luaeval('w:isvalid()'))

  " FIXME: how to test the case when isvalid() returns v:false?
  " isvalid() gives errors when the window is deleted. Is it a bug?

  lua w = nil
  bwipe!
endfunc

" Test vim.buffer() with and without argument
func Test_buffer()
  new Xfoo1
  let bn1 = bufnr('%')
  new Xfoo2
  let bn2 = bufnr('%')

  " Test vim.buffer() without argument.
  call assert_equal('Xfoo2', luaeval("vim.buffer().name"))

  " Test vim.buffer() with string argument.
  call assert_equal('Xfoo1', luaeval("vim.buffer('Xfoo1').name"))
  call assert_equal('Xfoo2', luaeval("vim.buffer('Xfoo2').name"))

  " Test vim.buffer() with integer argument.
  call assert_equal('Xfoo1', luaeval("vim.buffer(" . bn1 . ").name"))
  call assert_equal('Xfoo2', luaeval("vim.buffer(" . bn2 . ").name"))

  lua bn1, bn2 = nil
  %bwipe!
endfunc

" Test vim.buffer().name and vim.buffer().fname
func Test_buffer_name()
  new
  call assert_equal('', luaeval('vim.buffer().name'))
  call assert_equal('', luaeval('vim.buffer().fname'))
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

  call assert_fails('lua vim.buffer()[3] = nil',
        \           '[string "vim chunk"]:1: invalid line number')
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

  lua bn, bp = nil
  %bwipe!
endfunc

" Test vim.buffer():isvalid()
func Test_buffer_isvalid()
  new Xfoo
  lua b = vim.buffer()
  call assert_true(luaeval('b:isvalid()'))

  " FIXME: how to test the case when isvalid() returns v:false?
  " isvalid() gives errors when the buffer is wiped. Is it a bug?

  lua b = nil
  bwipe!
endfunc

func Test_list()
  call assert_equal([], luaeval('vim.list()'))

  let l = []
  lua l = vim.eval('l')
  lua l:add(123)
  lua l:add('abc')
  lua l:add(true)
  lua l:add(false)
  lua l:add(nil)
  lua l:add(vim.eval("[1, 2, 3]"))
  lua l:add(vim.eval("{'a':1, 'b':2, 'c':3}"))
  call assert_equal([123.0, 'abc', v:true, v:false, v:null, [1, 2, 3], {'a': 1, 'b': 2, 'c': 3}], l)
  call assert_equal(7.0, luaeval('#l'))
  call assert_match('^list: \%(0x\)\?\x\+$', luaeval('tostring(l)'))

  lua l[0] = 124
  lua l[5] = nil
  lua l:insert('first')
  lua l:insert('xx', 3)
  call assert_equal(['first', 124.0, 'abc', 'xx', v:true, v:false, v:null, {'a': 1, 'b': 2, 'c': 3}], l)

  lockvar 1 l
  call assert_fails('lua l:add("x")', '[string "vim chunk"]:1: list is locked')

  lua l = nil
endfunc

func Test_list_table()
  " See :help lua-vim
  " Non-numeric keys should not be used to initialize the list
  " so say = 'hi' should be ignored.
  lua t = {3.14, 'hello', false, true, say = 'hi'}
  call assert_equal([3.14, 'hello', v:false, v:true], luaeval('vim.list(t)'))
  lua t = nil

  call assert_fails('lua vim.list(1)', '[string "vim chunk"]:1: table expected, got number')
  call assert_fails('lua vim.list("x")', '[string "vim chunk"]:1: table expected, got string')
  call assert_fails('lua vim.list(print)', '[string "vim chunk"]:1: table expected, got function')
  call assert_fails('lua vim.list(true)', '[string "vim chunk"]:1: table expected, got boolean')
endfunc

" Test l() i.e. iterator on list
func Test_list_iter()
  lua l = vim.list():add('foo'):add('bar')
  lua str = ''
  lua for v in l() do str = str .. v end
  call assert_equal('foobar', luaeval('str'))

  lua str, l = nil
endfunc

func Test_recursive_list()
  lua l = vim.list():add(1):add(2)
  lua l = l:add(l)

  call assert_equal(1.0, luaeval('l[0]'))
  call assert_equal(2.0, luaeval('l[1]'))

  call assert_equal(1.0, luaeval('l[2][0]'))
  call assert_equal(2.0, luaeval('l[2][1]'))

  call assert_equal(1.0, luaeval('l[2][2][0]'))
  call assert_equal(2.0, luaeval('l[2][2][1]'))

  call assert_equal('[1.0, 2.0, [...]]', string(luaeval('l')))

  call assert_match('^list: \%(0x\)\?\x\+$', luaeval('tostring(l)'))
  call assert_equal(luaeval('tostring(l)'), luaeval('tostring(l[2])'))

  call assert_equal(luaeval('l'), luaeval('l[2]'))
  call assert_equal(luaeval('l'), luaeval('l[2][2]'))

  lua l = nil
endfunc

func Test_dict()
  call assert_equal({}, luaeval('vim.dict()'))

  let d = {}
  lua d = vim.eval('d')
  lua d[0] = 123
  lua d[1] = "abc"
  lua d[2] = true
  lua d[3] = false
  lua d[4] = vim.eval("[1, 2, 3]")
  lua d[5] = vim.eval("{'a':1, 'b':2, 'c':3}")
  call assert_equal({'0':123.0, '1':'abc', '2':v:true, '3':v:false, '4': [1, 2, 3], '5': {'a':1, 'b':2, 'c':3}}, d)
  call assert_equal(6.0, luaeval('#d'))
  call assert_match('^dict: \%(0x\)\?\x\+$', luaeval('tostring(d)'))

  call assert_equal('abc', luaeval('d[1]'))

  lua d[0] = 124
  lua d[4] = nil
  call assert_equal({'0':124.0, '1':'abc', '2':v:true, '3':v:false, '5': {'a':1, 'b':2, 'c':3}}, d)

  lockvar 1 d
  call assert_fails('lua d[6] = 1', '[string "vim chunk"]:1: dict is locked')

  lua d = nil
endfunc

func Test_dict_table()
  lua t = {key1 = 'x', key2 = 3.14, key3 = true, key4 = false}
  call assert_equal({'key1': 'x', 'key2': 3.14, 'key3': v:true, 'key4': v:false},
        \           luaeval('vim.dict(t)'))

  " Same example as in :help lua-vim.
  lua t = {math.pi, false, say = 'hi'}
  " FIXME: commented out as it currently does not work as documented:
  " Expected {'say': 'hi'}
  " but got {'1': 3.141593, '2': v:false, 'say': 'hi'}
  " Is the documentation or the code wrong?
  "call assert_equal({'say' : 'hi'}, luaeval('vim.dict(t)'))
  lua t = nil

  call assert_fails('lua vim.dict(1)', '[string "vim chunk"]:1: table expected, got number')
  call assert_fails('lua vim.dict("x")', '[string "vim chunk"]:1: table expected, got string')
  call assert_fails('lua vim.dict(print)', '[string "vim chunk"]:1: table expected, got function')
  call assert_fails('lua vim.dict(true)', '[string "vim chunk"]:1: table expected, got boolean')
endfunc

" Test d() i.e. iterator on dictionary
func Test_dict_iter()
  let d = {'a': 1, 'b':2}
  lua d = vim.eval('d')
  lua str = ''
  lua for k,v in d() do str = str .. k ..':' .. v .. ',' end
  call assert_equal('a:1,b:2,', luaeval('str'))

  lua str, d = nil
endfunc

func Test_blob()
  call assert_equal(0z, luaeval('vim.blob("")'))
  call assert_equal(0z31326162, luaeval('vim.blob("12ab")'))
  call assert_equal(0z00010203, luaeval('vim.blob("\x00\x01\x02\x03")'))
  call assert_equal(0z8081FEFF, luaeval('vim.blob("\x80\x81\xfe\xff")'))

  lua b = vim.blob("\x00\x00\x00\x00")
  call assert_equal(0z00000000, luaeval('b'))
  call assert_equal(4.0, luaeval('#b'))
  lua b[0], b[1], b[2], b[3] = 1, 32, 256, 0xff
  call assert_equal(0z012000ff, luaeval('b'))
  lua b[4] = string.byte("z", 1)
  call assert_equal(0z012000ff.7a, luaeval('b'))
  call assert_equal(5.0, luaeval('#b'))
  call assert_fails('lua b[#b+1] = 0x80', '[string "vim chunk"]:1: index out of range')
  lua b:add("12ab")
  call assert_equal(0z012000ff.7a313261.62, luaeval('b'))
  call assert_equal(9.0, luaeval('#b'))
  call assert_fails('lua b:add(nil)', '[string "vim chunk"]:1: string expected, got nil')
  call assert_fails('lua b:add(true)', '[string "vim chunk"]:1: string expected, got boolean')
  call assert_fails('lua b:add({})', '[string "vim chunk"]:1: string expected, got table')
  lua b = nil
endfunc

func Test_funcref()
  function I(x)
    return a:x
  endfunction
  let R = function('I')
  lua i1 = vim.funcref"I"
  lua i2 = vim.eval"R"
  lua msg = "funcref|test|" .. (#i2(i1) == #i1(i2) and "OK" or "FAIL")
  lua msg = vim.funcref"tr"(msg, "|", " ")
  call assert_equal("funcref test OK", luaeval('msg'))

  " dict funcref
  function Mylen() dict
    return len(self.data)
  endfunction
  let l = [0, 1, 2, 3]
  let mydict = {'data': l}
  lua d = vim.eval"mydict"
  lua d.len = vim.funcref"Mylen" -- assign d as 'self'
  lua res = (d.len() == vim.funcref"len"(vim.eval"l")) and "OK" or "FAIL"
  call assert_equal("OK", luaeval('res'))
  call assert_equal(function('Mylen', {'data': l, 'len': function('Mylen')}), mydict.len)

  lua i1, i2, msg, d, res = nil
endfunc

" Test vim.type()
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
  call assert_equal('funcref',  luaeval('vim.type(vim.funcref("Test_type"))'))
endfunc

" Test vim.open()
func Test_open()
  call assert_notmatch('XOpen', execute('ls'))

  " Open a buffer XOpen1, but do not jump to it.
  lua b = vim.open('XOpen1')
  call assert_equal('XOpen1', luaeval('b.name'))
  call assert_equal('', bufname('%'))

  call assert_match('XOpen1', execute('ls'))
  call assert_notequal('XOpen2', bufname('%'))

  " Open a buffer XOpen2 and jump to it.
  lua b = vim.open('XOpen2')()
  call assert_equal('XOpen2', luaeval('b.name'))
  call assert_equal('XOpen2', bufname('%'))

  lua b = nil
  %bwipe!
endfunc

" Test vim.line()
func Test_line()
  new
  call setline(1, ['first line', 'second line'])
  1
  call assert_equal('first line', luaeval('vim.line()'))
  2
  call assert_equal('second line', luaeval('vim.line()'))
  bwipe!
endfunc

" Test vim.beep()
func Test_beep()
  call assert_beeps('lua vim.beep()')
endfunc

" Test errors in luaeval()
func Test_luaeval_error()
  " Compile error
  call assert_fails("call luaeval('-nil')",
  \ '[string "luaeval"]:1: attempt to perform arithmetic on a nil value')
  call assert_fails("call luaeval(']')",
  \ "[string \"luaeval\"]:1: unexpected symbol near ']'")
endfunc

" Test :luafile foo.lua
func Test_luafile()
  call delete('Xlua_file')
  call writefile(["str = 'hello'", "num = 123.0" ], 'Xlua_file')
  call setfperm('Xlua_file', 'r-xr-xr-x')

  luafile Xlua_file
  call assert_equal('hello', luaeval('str'))
  call assert_equal(123.0, luaeval('num'))

  lua str, num = nil
  call delete('Xlua_file')
endfunc

" Test :luafile %
func Test_luafile_percent()
  new Xlua_file
  append
    str, num = 'foo', 321.0
    print(string.format('str=%s, num=%d', str, num))
.
  w!
  luafile %
  let msg = split(execute('message'), "\n")[-1]
  call assert_equal('str=foo, num=321', msg)

  lua str, num = nil
  call delete('Xlua_file')
  bwipe!
endfunc

" Test :luafile with syntax error
func Test_luafile_error()
  new Xlua_file
  call writefile(['nil = 0' ], 'Xlua_file')
  call setfperm('Xlua_file', 'r-xr-xr-x')

  call assert_fails('luafile Xlua_file', "Xlua_file:1: unexpected symbol near 'nil'")

  call delete('Xlua_file')
  bwipe!
endfunc

func Test_set_cursor()
  " Check that setting the cursor position works.
  new
  call setline(1, ['first line', 'second line'])
  normal gg
  lua << EOF
w = vim.window()
w.line = 1
w.col = 5
EOF
  call assert_equal([1, 5], [line('.'), col('.')])

  " Check that movement after setting cursor position keeps current column.
  normal j
  call assert_equal([2, 5], [line('.'), col('.')])
endfunc
