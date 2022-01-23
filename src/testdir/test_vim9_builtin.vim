" Test using builtin functions in the Vim9 script language.

source check.vim
source vim9.vim

" Test for passing too many or too few arguments to builtin functions
func Test_internalfunc_arg_error()
  let l =<< trim END
    def! FArgErr(): float
      return ceil(1.1, 2)
    enddef
    defcompile
  END
  call writefile(l, 'Xinvalidarg')
  call assert_fails('so Xinvalidarg', 'E118:', '', 1, 'FArgErr')
  let l =<< trim END
    def! FArgErr(): float
      return ceil()
    enddef
    defcompile
  END
  call writefile(l, 'Xinvalidarg')
  call assert_fails('so Xinvalidarg', 'E119:', '', 1, 'FArgErr')
  call delete('Xinvalidarg')
endfunc

" Test for builtin functions returning different types
func Test_InternalFuncRetType()
  let lines =<< trim END
    def RetFloat(): float
      return ceil(1.456)
    enddef

    def RetListAny(): list<any>
      return items({k: 'v'})
    enddef

    def RetListString(): list<string>
      return split('a:b:c', ':')
    enddef

    def RetListDictAny(): list<dict<any>>
      return getbufinfo()
    enddef

    def RetDictNumber(): dict<number>
      return wordcount()
    enddef

    def RetDictString(): dict<string>
      return environ()
    enddef
  END
  call writefile(lines, 'Xscript')
  source Xscript

  call RetFloat()->assert_equal(2.0)
  call RetListAny()->assert_equal([['k', 'v']])
  call RetListString()->assert_equal(['a', 'b', 'c'])
  call RetListDictAny()->assert_notequal([])
  call RetDictNumber()->assert_notequal({})
  call RetDictString()->assert_notequal({})
  call delete('Xscript')
endfunc

def Test_abs()
  assert_equal(0, abs(0))
  assert_equal(2, abs(-2))
  assert_equal(3, abs(3))
  CheckDefAndScriptFailure(['abs("text")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  if has('float')
    assert_equal(0, abs(0))
    assert_equal(2.0, abs(-2.0))
    assert_equal(3.0, abs(3.0))
  endif
enddef

def Test_add()
  CheckDefAndScriptFailure(['add({}, 1)'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<unknown>', 'E1226: List or Blob required for argument 1'])
  CheckDefFailure(['add([1], "a")'], 'E1012: Type mismatch; expected number but got string')

  var lines =<< trim END
    vim9script
    g:thelist = [1]
    lockvar g:thelist
    def TryChange()
      g:thelist->add(2)
    enddef
    TryChange()
  END
  CheckScriptFailure(lines, 'E741:')
enddef

def Test_add_blob()
  var b1: blob = 0z12
  add(b1, 0x34)
  assert_equal(0z1234, b1)

  var b2: blob # defaults to empty blob
  add(b2, 0x67)
  assert_equal(0z67, b2)

  var lines =<< trim END
      var b: blob
      add(b, "x")
  END
  CheckDefFailure(lines, 'E1012:', 2)

  lines =<< trim END
      add(test_null_blob(), 123)
  END
  CheckDefExecAndScriptFailure(lines, 'E1131:', 1)

  lines =<< trim END
      var b: blob = test_null_blob()
      add(b, 123)
  END
  CheckDefExecFailure(lines, 'E1131:', 2)

  # Getting variable with NULL blob allocates a new blob at script level
  lines =<< trim END
      vim9script
      var b: blob = test_null_blob()
      add(b, 123)
  END
  CheckScriptSuccess(lines)
enddef

def Test_add_list()
  var l: list<number>  # defaults to empty list
  add(l, 9)
  assert_equal([9], l)

  var lines =<< trim END
      var l: list<number>
      add(l, "x")
  END
  CheckDefFailure(lines, 'E1012:', 2)

  lines =<< trim END
      add(test_null_list(), 123)
  END
  CheckDefExecAndScriptFailure(lines, 'E1130:', 1)

  lines =<< trim END
      var l: list<number> = test_null_list()
      add(l, 123)
  END
  CheckDefExecFailure(lines, 'E1130:', 2)

  # Getting variable with NULL list allocates a new list at script level
  lines =<< trim END
      vim9script
      var l: list<number> = test_null_list()
      add(l, 123)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var l: list<string> = ['a']
      l->add(123)
  END
  CheckScriptFailure(lines, 'E1012: Type mismatch; expected string but got number', 3)

  lines =<< trim END
      vim9script
      var l: list<string>
      l->add(123)
  END
  CheckScriptFailure(lines, 'E1012: Type mismatch; expected string but got number', 3)
enddef

def Test_and()
  CheckDefAndScriptFailure(['and("x", 0x2)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['and(0x1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_append()
  new
  setline(1, range(3))
  var res1: number = append(1, 'one')
  assert_equal(0, res1)
  var res2: bool = append(3, 'two')
  assert_equal(false, res2)
  assert_equal(['0', 'one', '1', 'two', '2'], getline(1, 6))

  append(0, 'zero')
  assert_equal('zero', getline(1))
  append(0, {a: 10})
  assert_equal("{'a': 10}", getline(1))
  append(0, function('min'))
  assert_equal("function('min')", getline(1))
  CheckDefAndScriptFailure(['append([1], "x")'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['append("", "x")'], 'E1209: Invalid value for a line number')
  CheckDefExecAndScriptFailure(['append(".a", "x")'], 'E1209: Invalid value for a line number')
  # only get one error
  assert_fails('append("''aa", "x")', ['E1209: Invalid value for a line number: "''aa"', 'E1209:'])
  CheckDefExecAndScriptFailure(['append(-1, "x")'], 'E966: Invalid line number: -1')
  bwipe!
enddef

def Test_appendbufline()
  new
  var bnum: number = bufnr()
  :wincmd w
  appendbufline(bnum, 0, range(3))
  var res1: number = appendbufline(bnum, 1, 'one')
  assert_equal(0, res1)
  var res2: bool = appendbufline(bnum, 3, 'two')
  assert_equal(false, res2)
  assert_equal(['0', 'one', '1', 'two', '2', ''], getbufline(bnum, 1, '$'))
  appendbufline(bnum, 0, 'zero')
  assert_equal(['zero'], getbufline(bnum, 1))
  CheckDefAndScriptFailure(['appendbufline([1], 1, "x")'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['appendbufline(1, [1], "x")'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 2'])
  CheckDefExecAndScriptFailure(['appendbufline(' .. bnum .. ', -1, "x")'], 'E966: Invalid line number: -1')
  CheckDefExecAndScriptFailure(['appendbufline(' .. bnum .. ', "$a", "x")'], 'E1030: Using a String as a Number: "$a"')
  assert_fails('appendbufline(' .. bnum .. ', "$a", "x")', ['E1030: Using a String as a Number: "$a"', 'E1030:'])
  CheckDefAndScriptFailure(['appendbufline(1, 1, {"a": 10})'], ['E1013: Argument 3: type mismatch, expected string but got dict<number>', 'E1224: String, Number or List required for argument 3'])
  bnum->bufwinid()->win_gotoid()
  appendbufline('', 0, 'numbers')
  getline(1)->assert_equal('numbers')
  bwipe!
enddef

def Test_argc()
  CheckDefAndScriptFailure(['argc("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_arglistid()
  CheckDefAndScriptFailure(['arglistid("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['arglistid(1, "y")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['arglistid("x", "y")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_argv()
  CheckDefAndScriptFailure(['argv("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['argv(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['argv("x", "y")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_assert_beeps()
  CheckDefAndScriptFailure(['assert_beeps(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
enddef

def Test_assert_equalfile()
  CheckDefAndScriptFailure(['assert_equalfile(1, "f2")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['assert_equalfile("f1", true)'], ['E1013: Argument 2: type mismatch, expected string but got bool', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['assert_equalfile("f1", "f2", ["a"])'], ['E1013: Argument 3: type mismatch, expected string but got list<string>', 'E1174: String required for argument 3'])
enddef

def Test_assert_exception()
  CheckDefAndScriptFailure(['assert_exception({})'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['assert_exception("E1:", v:null)'], ['E1013: Argument 2: type mismatch, expected string but got special', 'E1174: String required for argument 2'])
enddef

def Test_assert_fails()
  CheckDefAndScriptFailure(['assert_fails([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['assert_fails("a", true)'], ['E1013: Argument 2: type mismatch, expected string but got bool', 'E1222: String or List required for argument 2'])
  CheckDefAndScriptFailure(['assert_fails("a", "b", "c", "d")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  CheckDefAndScriptFailure(['assert_fails("a", "b", "c", 4, 5)'], ['E1013: Argument 5: type mismatch, expected string but got number', 'E1174: String required for argument 5'])
enddef

def Test_assert_inrange()
  CheckDefAndScriptFailure(['assert_inrange("a", 2, 3)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  CheckDefAndScriptFailure(['assert_inrange(1, "b", 3)'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 2'])
  CheckDefAndScriptFailure(['assert_inrange(1, 2, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 3'])
  CheckDefAndScriptFailure(['assert_inrange(1, 2, 3, 4)'], ['E1013: Argument 4: type mismatch, expected string but got number', 'E1174: String required for argument 4'])
enddef

def Test_assert_match()
  CheckDefAndScriptFailure(['assert_match({}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', ''])
  CheckDefAndScriptFailure(['assert_match("a", 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', ''])
  CheckDefAndScriptFailure(['assert_match("a", "b", null)'], ['E1013: Argument 3: type mismatch, expected string but got special', ''])
enddef

def Test_assert_nobeep()
  CheckDefAndScriptFailure(['assert_nobeep(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
enddef

def Test_assert_notmatch()
  CheckDefAndScriptFailure(['assert_notmatch({}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', ''])
  CheckDefAndScriptFailure(['assert_notmatch("a", 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', ''])
  CheckDefAndScriptFailure(['assert_notmatch("a", "b", null)'], ['E1013: Argument 3: type mismatch, expected string but got special', ''])
enddef

def Test_assert_report()
  CheckDefAndScriptFailure(['assert_report([1, 2])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
enddef

def Test_balloon_show()
  CheckGui
  CheckFeature balloon_eval

  assert_fails('balloon_show(10)', 'E1222:')
  assert_fails('balloon_show(true)', 'E1222:')

  CheckDefAndScriptFailure(['balloon_show(1.2)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['balloon_show({"a": 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1222: String or List required for argument 1'])
enddef

def Test_balloon_split()
  CheckFeature balloon_eval_term

  assert_fails('balloon_split([])', 'E1174:')
  assert_fails('balloon_split(true)', 'E1174:')
enddef

def Test_blob2list()
  CheckDefAndScriptFailure(['blob2list(10)'], ['E1013: Argument 1: type mismatch, expected blob but got number', 'E1238: Blob required for argument 1'])
enddef

def Test_browse()
  CheckFeature browse

  CheckDefAndScriptFailure(['browse(2, "title", "dir", "file")'], ['E1013: Argument 1: type mismatch, expected bool but got number', 'E1212: Bool required for argument 1'])
  CheckDefAndScriptFailure(['browse(true, 2, "dir", "file")'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['browse(true, "title", 3, "file")'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  CheckDefAndScriptFailure(['browse(true, "title", "dir", 4)'], ['E1013: Argument 4: type mismatch, expected string but got number', 'E1174: String required for argument 4'])
enddef

def Test_browsedir()
  if has('browse')
    CheckDefAndScriptFailure(['browsedir({}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', 'E1174: String required for argument 1'])
    CheckDefAndScriptFailure(['browsedir("a", [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 2'])
  endif
enddef

def Test_bufadd()
  assert_fails('bufadd([])', 'E1174:')
enddef

def Test_bufexists()
  assert_fails('bufexists(true)', 'E1220:')
  bufexists('')->assert_false()
enddef

def Test_buflisted()
  var res: bool = buflisted('asdf')
  assert_equal(false, res)
  assert_fails('buflisted(true)', 'E1220:')
  assert_fails('buflisted([])', 'E1220:')
  buflisted('')->assert_false()
enddef

def Test_bufload()
  assert_fails('bufload([])', 'E1220:')
  bufload('')->assert_equal(0)
enddef

def Test_bufloaded()
  assert_fails('bufloaded(true)', 'E1220:')
  assert_fails('bufloaded([])', 'E1220:')
  bufloaded('')->assert_false()
enddef

def Test_bufname()
  split SomeFile
  bufname('%')->assert_equal('SomeFile')
  edit OtherFile
  bufname('#')->assert_equal('SomeFile')
  close
  assert_fails('bufname(true)', 'E1220:')
  assert_fails('bufname([])', 'E1220:')
enddef

def Test_bufnr()
  var buf = bufnr()
  bufnr('%')->assert_equal(buf)

  buf = bufnr('Xdummy', true)
  buf->assert_notequal(-1)
  exe 'bwipe! ' .. buf
  CheckDefAndScriptFailure(['bufnr([1])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['bufnr(1, 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
enddef

def Test_bufwinid()
  var origwin = win_getid()
  below split SomeFile
  var SomeFileID = win_getid()
  below split OtherFile
  below split SomeFile
  bufwinid('SomeFile')->assert_equal(SomeFileID)

  win_gotoid(origwin)
  only
  bwipe SomeFile
  bwipe OtherFile

  assert_fails('bufwinid(true)', 'E1220:')
  assert_fails('bufwinid([])', 'E1220:')
enddef

def Test_bufwinnr()
  assert_fails('bufwinnr(true)', 'E1220:')
  assert_fails('bufwinnr([])', 'E1220:')
enddef

def Test_byte2line()
  CheckDefAndScriptFailure(['byte2line("1")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['byte2line([])'], ['E1013: Argument 1: type mismatch, expected number but got list<unknown>', 'E1210: Number required for argument 1'])
  byte2line(0)->assert_equal(-1)
enddef

def Test_byteidx()
  CheckDefAndScriptFailure(['byteidx(1, 2)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['byteidx("a", "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  byteidx('', 0)->assert_equal(0)
  byteidx('', 1)->assert_equal(-1)
enddef

def Test_byteidxcomp()
  CheckDefAndScriptFailure(['byteidxcomp(1, 2)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['byteidxcomp("a", "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_call_call()
  var l = [3, 2, 1]
  call('reverse', [l])
  l->assert_equal([1, 2, 3])

  CheckDefExecAndScriptFailure(['call(123, [2])'], 'E1256: String or function required for argument 1')
  CheckDefExecAndScriptFailure(['call(true, [2])'], 'E1256: String or function required for argument 1')
  CheckDefAndScriptFailure(['call("reverse", 2)'], ['E1013: Argument 2: type mismatch, expected list<any> but got number', 'E1211: List required for argument 2'])
  CheckDefAndScriptFailure(['call("reverse", [2], [1])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_ch_canread()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_canread(10)'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
  endif
enddef

def Test_ch_close()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_close("c")'], ['E1013: Argument 1: type mismatch, expected channel but got string', 'E1217: Channel or Job required for argument 1'])
  endif
enddef

def Test_ch_close_in()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_close_in(true)'], ['E1013: Argument 1: type mismatch, expected channel but got bool', 'E1217: Channel or Job required for argument 1'])
  endif
enddef

def Test_ch_evalexpr()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_evalexpr(1, "a")'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_evalexpr(test_null_channel(), 1, [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
  endif
enddef

def Test_ch_evalraw()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_evalraw(1, "")'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_evalraw(test_null_channel(), 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1221: String or Blob required for argument 2'])
    CheckDefAndScriptFailure(['ch_evalraw(test_null_channel(), "", [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
  endif
enddef

def Test_ch_getbufnr()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_getbufnr(1, "a")'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_getbufnr(test_null_channel(), 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
    # test empty string argument for ch_getbufnr()
    var job: job = job_start(&shell)
    job->ch_getbufnr('')->assert_equal(-1)
    job_stop(job)
  endif
enddef

def Test_ch_getjob()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_getjob(1)'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_getjob({"a": 10})'], ['E1013: Argument 1: type mismatch, expected channel but got dict<number>', 'E1217: Channel or Job required for argument 1'])
    assert_equal(0, ch_getjob(test_null_channel()))
  endif
enddef

def Test_ch_info()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_info([1])'], ['E1013: Argument 1: type mismatch, expected channel but got list<number>', 'E1217: Channel or Job required for argument 1'])
  endif
enddef

def Test_ch_log()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_log(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1174: String required for argument 1'])
    CheckDefAndScriptFailure(['ch_log("a", 1)'], ['E1013: Argument 2: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 2'])
  endif
enddef

def Test_ch_logfile()
  if !has('channel')
    CheckFeature channel
  else
    assert_fails('ch_logfile(true)', 'E1174:')
    assert_fails('ch_logfile("foo", true)', 'E1174:')
    ch_logfile('', '')->assert_equal(0)

    CheckDefAndScriptFailure(['ch_logfile(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
    CheckDefAndScriptFailure(['ch_logfile("a", true)'], ['E1013: Argument 2: type mismatch, expected string but got bool', 'E1174: String required for argument 2'])
  endif
enddef

def Test_ch_open()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_open({"a": 10}, "a")'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
    CheckDefAndScriptFailure(['ch_open("a", [1])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 2'])
    CheckDefExecAndScriptFailure(['ch_open("")'], 'E475: Invalid argument')
  endif
enddef

def Test_ch_read()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_read(1)'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_read(test_null_channel(), [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
  endif
enddef

def Test_ch_readblob()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_readblob(1)'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_readblob(test_null_channel(), [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
  endif
enddef

def Test_ch_readraw()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_readraw(1)'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_readraw(test_null_channel(), [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
  endif
enddef

def Test_ch_sendexpr()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_sendexpr(1, "a")'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_sendexpr(test_null_channel(), 1, [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
  endif
enddef

def Test_ch_sendraw()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_sendraw(1, "")'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_sendraw(test_null_channel(), 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1221: String or Blob required for argument 2'])
    CheckDefAndScriptFailure(['ch_sendraw(test_null_channel(), "", [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
  endif
enddef

def Test_ch_setoptions()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_setoptions(1, {})'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_setoptions(test_null_channel(), [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
  endif
enddef

def Test_ch_status()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['ch_status(1)'], ['E1013: Argument 1: type mismatch, expected channel but got number', 'E1217: Channel or Job required for argument 1'])
    CheckDefAndScriptFailure(['ch_status(test_null_channel(), [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
  endif
enddef

def Test_char2nr()
  char2nr('あ', true)->assert_equal(12354)

  assert_fails('char2nr(true)', 'E1174:')
  CheckDefAndScriptFailure(['char2nr(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['char2nr("a", 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  assert_equal(97, char2nr('a', 1))
  assert_equal(97, char2nr('a', 0))
  assert_equal(97, char2nr('a', true))
  assert_equal(97, char2nr('a', false))
  char2nr('')->assert_equal(0)
enddef

def Test_charclass()
  assert_fails('charclass(true)', 'E1174:')
  charclass('')->assert_equal(0)
enddef

def Test_charcol()
  CheckDefAndScriptFailure(['charcol(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['charcol({a: 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1222: String or List required for argument 1'])
  CheckDefExecAndScriptFailure(['charcol("")'], 'E1209: Invalid value for a line number')
  new
  setline(1, ['abcdefgh'])
  cursor(1, 4)
  assert_equal(4, charcol('.'))
  assert_equal(9, charcol([1, '$']))
  assert_equal(0, charcol([10, '$']))
  bw!
enddef

def Test_charidx()
  CheckDefAndScriptFailure(['charidx(0z10, 1)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['charidx("a", "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['charidx("a", 1, "")'], ['E1013: Argument 3: type mismatch, expected bool but got string', 'E1212: Bool required for argument 3'])
  charidx('', 0)->assert_equal(-1)
  charidx('', 1)->assert_equal(-1)
enddef

def Test_chdir()
  assert_fails('chdir(true)', 'E1174:')
enddef

def Test_cindent()
  CheckDefAndScriptFailure(['cindent([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['cindent(null)'], ['E1013: Argument 1: type mismatch, expected string but got special', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['cindent("")'], 'E1209: Invalid value for a line number')
  assert_equal(-1, cindent(0))
  assert_equal(0, cindent('.'))
enddef

def Test_clearmatches()
  CheckDefAndScriptFailure(['clearmatches("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_col()
  new
  setline(1, 'abcdefgh')
  cursor(1, 4)
  assert_equal(4, col('.'))
  col([1, '$'])->assert_equal(9)
  assert_equal(0, col([10, '$']))

  assert_fails('col(true)', 'E1222:')

  CheckDefAndScriptFailure(['col(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['col({a: 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['col(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1222: String or List required for argument 1'])
  CheckDefExecAndScriptFailure(['col("")'], 'E1209: Invalid value for a line number')
  bw!
enddef

def Test_complete()
  CheckDefAndScriptFailure(['complete("1", [])'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['complete(1, {})'], ['E1013: Argument 2: type mismatch, expected list<any> but got dict<unknown>', 'E1211: List required for argument 2'])
enddef

def Test_complete_add()
  CheckDefAndScriptFailure(['complete_add([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1223: String or Dictionary required for argument 1'])
enddef

def Test_complete_info()
  CheckDefAndScriptFailure(['complete_info("")'], ['E1013: Argument 1: type mismatch, expected list<string> but got string', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['complete_info({})'], ['E1013: Argument 1: type mismatch, expected list<string> but got dict<unknown>', 'E1211: List required for argument 1'])
  assert_equal({'pum_visible': 0, 'mode': '', 'selected': -1, 'items': []}, complete_info())
  assert_equal({'mode': '', 'items': []}, complete_info(['mode', 'items']))
enddef

def Test_confirm()
  if !has('dialog_con') && !has('dialog_gui')
    CheckFeature dialog_con
  endif

  assert_fails('confirm(true)', 'E1174:')
  assert_fails('confirm("yes", true)', 'E1174:')
  assert_fails('confirm("yes", "maybe", 2, true)', 'E1174:')
  CheckDefAndScriptFailure(['confirm(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['confirm("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['confirm("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['confirm("a", "b", 3, 4)'], ['E1013: Argument 4: type mismatch, expected string but got number', 'E1174: String required for argument 4'])
enddef

def Test_copy_return_type()
  var l = copy([1, 2, 3])
  var res = 0
  for n in l
    res += n
  endfor
  res->assert_equal(6)

  var dl = deepcopy([1, 2, 3])
  res = 0
  for n in dl
    res += n
  endfor
  res->assert_equal(6)

  dl = deepcopy([1, 2, 3], true)
enddef

def Test_count()
  count('ABC ABC ABC', 'b', true)->assert_equal(3)
  count('ABC ABC ABC', 'b', false)->assert_equal(0)
  CheckDefAndScriptFailure(['count(10, 1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1225: String, List or Dictionary required for argument 1'])
  CheckDefAndScriptFailure(['count("a", [1], 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
  CheckDefAndScriptFailure(['count("a", [1], 0, "b")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  count([1, 2, 2, 3], 2)->assert_equal(2)
  count([1, 2, 2, 3], 2, false, 2)->assert_equal(1)
  count({a: 1.1, b: 2.2, c: 1.1}, 1.1)->assert_equal(2)
enddef

def Test_cscope_connection()
  CheckFeature cscope
  assert_equal(0, cscope_connection())
  CheckDefAndScriptFailure(['cscope_connection("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['cscope_connection(1, 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['cscope_connection(1, "b", 3)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
enddef

def Test_cursor()
  new
  setline(1, range(4))
  cursor(2, 1)
  assert_equal(2, getcurpos()[1])
  cursor('$', 1)
  assert_equal(4, getcurpos()[1])
  cursor([2, 1])
  assert_equal(2, getcurpos()[1])

  var lines =<< trim END
    cursor('2', 1)
  END
  CheckDefExecAndScriptFailure(lines, 'E1209:')
  CheckDefAndScriptFailure(['cursor(0z10, 1)'], ['E1013: Argument 1: type mismatch, expected number but got blob', 'E1224: String, Number or List required for argument 1'])
  CheckDefAndScriptFailure(['cursor(1, "2")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['cursor(1, 2, "3")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefExecAndScriptFailure(['cursor("", 2)'], 'E1209: Invalid value for a line number')
enddef

def Test_debugbreak()
  CheckMSWindows
  CheckDefAndScriptFailure(['debugbreak("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_deepcopy()
  CheckDefAndScriptFailure(['deepcopy({}, 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
enddef

def Test_delete()
  var res: bool = delete('doesnotexist')
  assert_equal(true, res)

  CheckDefAndScriptFailure(['delete(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['delete("a", 10)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefExecAndScriptFailure(['delete("")'], 'E474: Invalid argument')
enddef

def Test_deletebufline()
  CheckDefAndScriptFailure(['deletebufline([], 2)'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['deletebufline("a", [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 2'])
  CheckDefAndScriptFailure(['deletebufline("a", 2, 0z10)'], ['E1013: Argument 3: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 3'])
  new
  setline(1, ['one', 'two'])
  deletebufline('', 1)
  getline(1, '$')->assert_equal(['two'])

  assert_fails('deletebufline("", "$a", "$b")', ['E1030: Using a String as a Number: "$a"', 'E1030: Using a String as a Number: "$a"'])
  assert_fails('deletebufline("", "$", "$b")', ['E1030: Using a String as a Number: "$b"', 'E1030: Using a String as a Number: "$b"'])

  bwipe!
enddef

def Test_diff_filler()
  CheckDefAndScriptFailure(['diff_filler([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['diff_filler(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['diff_filler("")'], 'E1209: Invalid value for a line number')
  assert_equal(0, diff_filler(1))
  assert_equal(0, diff_filler('.'))
enddef

def Test_diff_hlID()
  CheckDefAndScriptFailure(['diff_hlID(0z10, 1)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['diff_hlID(1, "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefExecAndScriptFailure(['diff_hlID("", 10)'], 'E1209: Invalid value for a line number')
enddef

def Test_digraph_get()
  CheckDefAndScriptFailure(['digraph_get(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefExecAndScriptFailure(['digraph_get("")'], 'E1214: Digraph must be just two characters')
enddef

def Test_digraph_getlist()
  CheckDefAndScriptFailure(['digraph_getlist(10)'], ['E1013: Argument 1: type mismatch, expected bool but got number', 'E1212: Bool required for argument 1'])
  CheckDefAndScriptFailure(['digraph_getlist("")'], ['E1013: Argument 1: type mismatch, expected bool but got string', 'E1212: Bool required for argument 1'])
enddef

def Test_digraph_set()
  CheckDefAndScriptFailure(['digraph_set(10, "a")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['digraph_set("ab", 0z10)'], ['E1013: Argument 2: type mismatch, expected string but got blob', 'E1174: String required for argument 2'])
  CheckDefExecAndScriptFailure(['digraph_set("", "a")'], 'E1214: Digraph must be just two characters')
enddef

def Test_digraph_setlist()
  CheckDefAndScriptFailure(['digraph_setlist("a")'], ['E1013: Argument 1: type mismatch, expected list<string> but got string', 'E1216: digraph_setlist() argument must be a list of lists with two items'])
  CheckDefAndScriptFailure(['digraph_setlist({})'], ['E1013: Argument 1: type mismatch, expected list<string> but got dict<unknown>', 'E1216: digraph_setlist() argument must be a list of lists with two items'])
enddef

def Test_echoraw()
  CheckDefAndScriptFailure(['echoraw(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['echoraw(["x"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
enddef

def Test_escape()
  CheckDefAndScriptFailure(['escape(10, " ")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['escape(true, false)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['escape("a", 10)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  assert_equal('a\:b', escape("a:b", ":"))
  escape('abc', '')->assert_equal('abc')
  escape('', ':')->assert_equal('')
enddef

def Test_eval()
  CheckDefAndScriptFailure(['eval(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['eval(null)'], ['E1013: Argument 1: type mismatch, expected string but got special', 'E1174: String required for argument 1'])
  CheckDefExecAndScriptFailure(['eval("")'], 'E15: Invalid expression')
  assert_equal(2, eval('1 + 1'))
enddef

def Test_executable()
  assert_false(executable(""))
  assert_false(executable(test_null_string()))

  CheckDefExecFailure(['echo executable(123)'], 'E1013:')
  CheckDefExecFailure(['echo executable(true)'], 'E1013:')
enddef

def Test_execute()
  var res = execute("echo 'hello'")
  assert_equal("\nhello", res)
  res = execute(["echo 'here'", "echo 'there'"])
  assert_equal("\nhere\nthere", res)

  CheckDefAndScriptFailure(['execute(123)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1222: String or List required for argument 1'])
  CheckDefFailure(['execute([123])'], 'E1013: Argument 1: type mismatch, expected list<string> but got list<number>')
  CheckDefExecFailure(['echo execute(["xx", 123])'], 'E492')
  CheckDefAndScriptFailure(['execute("xx", 123)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
enddef

def Test_exepath()
  CheckDefExecFailure(['echo exepath(true)'], 'E1013:')
  CheckDefExecFailure(['echo exepath(v:null)'], 'E1013:')
  CheckDefExecFailure(['echo exepath("")'], 'E1175:')
enddef

command DoSomeCommand let g:didSomeCommand = 4

def Test_exists()
  CheckDefAndScriptFailure(['exists(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  call assert_equal(1, exists('&tabstop'))

  var lines =<< trim END
    if exists('+newoption')
      if &newoption == 'ok'
      endif
    endif
  END
  CheckDefFailure(lines, 'E113:')
  CheckScriptSuccess(lines)
enddef

def Test_exists_compiled()
  call assert_equal(1, exists_compiled('&tabstop'))
  CheckDefAndScriptFailure(['exists_compiled(10)'], ['E1232:', 'E1233:'])
  CheckDefAndScriptFailure(['exists_compiled(v:progname)'], ['E1232:', 'E1233:'])

  if exists_compiled('+newoption')
    if &newoption == 'ok'
    endif
  endif
  if exists_compiled('&newoption')
    if &newoption == 'ok'
    endif
  endif
  if exists_compiled('+tabstop')
    assert_equal(8, &tabstop)
  else
    assert_report('tabstop option not existing?')
  endif
  if exists_compiled('&tabstop')
    assert_equal(8, &tabstop)
  else
    assert_report('tabstop option not existing?')
  endif

  if exists_compiled(':DoSomeCommand') >= 2
    DoSomeCommand
  endif
  assert_equal(4, g:didSomeCommand)
  if exists_compiled(':NoSuchCommand') >= 2
    NoSuchCommand
  endif

  var found = false
  if exists_compiled('*CheckScriptSuccess')
    found = true
  endif
  assert_true(found)
  if exists_compiled('*NoSuchFunction')
    NoSuchFunction()
  endif
  if exists_compiled('*no_such_function')
    no_such_function()
  endif
enddef

def Test_expand()
  split SomeFile
  expand('%', true, true)->assert_equal(['SomeFile'])
  close
  CheckDefAndScriptFailure(['expand(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['expand("a", 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  CheckDefAndScriptFailure(['expand("a", true, 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
  expand('')->assert_equal('')

  var caught = false
  try
    echo expand("<sfile>")
  catch /E1245:/
    caught = true
  endtry
  assert_true(caught)
enddef

def Test_expandcmd()
  $FOO = "blue"
  assert_equal("blue sky", expandcmd("`=$FOO .. ' sky'`"))

  assert_equal("yes", expandcmd("`={a: 'yes'}['a']`"))
  expandcmd('')->assert_equal('')

  CheckDefAndScriptFailure(['expandcmd([1])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
enddef

def Test_extend_arg_types()
  g:number_one = 1
  g:string_keep = 'keep'
  var lines =<< trim END
      assert_equal([1, 2, 3], extend([1, 2], [3]))
      assert_equal([3, 1, 2], extend([1, 2], [3], 0))
      assert_equal([1, 3, 2], extend([1, 2], [3], 1))
      assert_equal([1, 3, 2], extend([1, 2], [3], g:number_one))

      assert_equal({a: 1, b: 2, c: 3}, extend({a: 1, b: 2}, {c: 3}))
      assert_equal({a: 1, b: 4}, extend({a: 1, b: 2}, {b: 4}))
      assert_equal({a: 1, b: 2}, extend({a: 1, b: 2}, {b: 4}, 'keep'))
      assert_equal({a: 1, b: 2}, extend({a: 1, b: 2}, {b: 4}, g:string_keep))

      # mix of types is OK without a declaration

      var res: list<dict<any>>
      extend(res, mapnew([1, 2], (_, v) => ({})))
      assert_equal([{}, {}], res)

      var dany: dict<any> = {a: 0}
      dany->extend({b: 'x'})
      assert_equal({a: 0, b: 'x'}, dany)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      assert_equal([1, 2, "x"], extend([1, 2], ["x"]))
      assert_equal([1, "b", 1], extend([1], ["b", 1]))

      assert_equal({a: 1, b: "x"}, extend({a: 1}, {b: "x"}))
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(['extend("a", 1)'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E712: Argument of extend() must be a List or Dictionary'])
  CheckDefAndScriptFailure(['extend([1, 2], 3)'], ['E1013: Argument 2: type mismatch, expected list<any> but got number', 'E712: Argument of extend() must be a List or Dictionary'])
  CheckDefAndScriptFailure(['var ll = [1, 2]', 'extend(ll, ["x"])'], ['E1013: Argument 2: type mismatch, expected list<number> but got list<string>', 'E1013: Argument 2: type mismatch, expected list<number> but got list<string>'])
  CheckDefFailure(['extend([1, 2], [3], "x")'], 'E1013: Argument 3: type mismatch, expected number but got string')

  CheckDefFailure(['extend({a: 1}, 42)'], 'E1013: Argument 2: type mismatch, expected dict<any> but got number')
  CheckDefFailure(['extend({a: 1}, {b: 2}, 1)'], 'E1013: Argument 3: type mismatch, expected string but got number')

  CheckScriptFailure(['vim9script', 'var l = [1]', 'extend(l, ["b", 1])'], 'E1013: Argument 2: type mismatch, expected list<number> but got list<any> in extend()')
enddef

func g:ExtendDict(d)
  call extend(a:d, #{xx: 'x'})
endfunc

def Test_extend_dict_item_type()
  var lines =<< trim END
       var d: dict<number> = {a: 1}
       extend(d, {b: 2})
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
       var d: dict<number> = {a: 1}
       extend(d, {b: 'x'})
  END
  CheckDefAndScriptFailure(lines, 'E1013: Argument 2: type mismatch, expected dict<number> but got dict<string>', 2)

  lines =<< trim END
       var d: dict<number> = {a: 1}
       g:ExtendDict(d)
  END
  CheckDefExecFailure(lines, 'E1012: Type mismatch; expected number but got string', 0)
  CheckScriptFailure(['vim9script'] + lines, 'E1012:', 1)

  lines =<< trim END
       var d: dict<bool>
       extend(d, {b: 0})
  END
  CheckDefAndScriptFailure(lines, 'E1013: Argument 2: type mismatch, expected dict<bool> but got dict<number>', 2)
enddef

func g:ExtendList(l)
  call extend(a:l, ['x'])
endfunc

def Test_extend_list_item_type()
  var lines =<< trim END
       var l: list<number> = [1]
       extend(l, [2])
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
       var l: list<number> = [1]
       extend(l, ['x'])
  END
  CheckDefAndScriptFailure(lines, 'E1013: Argument 2: type mismatch, expected list<number> but got list<string>', 2)

  lines =<< trim END
       var l: list<number> = [1]
       g:ExtendList(l)
  END
  CheckDefExecFailure(lines, 'E1012: Type mismatch; expected number but got string', 0)
  CheckScriptFailure(['vim9script'] + lines, 'E1012:', 1)

  lines =<< trim END
       var l: list<bool>
       extend(l, [0])
  END
  CheckDefAndScriptFailure(lines, 'E1013: Argument 2: type mismatch, expected list<bool> but got list<number>', 2)
enddef

def Test_extend_return_type()
  var l = extend([1, 2], [3])
  var res = 0
  for n in l
    res += n
  endfor
  res->assert_equal(6)
enddef

def Test_extend_with_error_function()
  var lines =<< trim END
      vim9script
      def F()
        {
          var m = 10
        }
        echo m
      enddef

      def Test()
        var d: dict<any> = {}
        d->extend({A: 10, Func: function('F', [])})
      enddef

      Test()
  END
  CheckScriptFailure(lines, 'E1001: Variable not found: m')
enddef

def Test_extendnew()
  assert_equal([1, 2, 'a'], extendnew([1, 2], ['a']))
  assert_equal({one: 1, two: 'a'}, extendnew({one: 1}, {two: 'a'}))

  CheckDefAndScriptFailure(['extendnew({a: 1}, 42)'], ['E1013: Argument 2: type mismatch, expected dict<number> but got number', 'E712: Argument of extendnew() must be a List or Dictionary'])
  CheckDefAndScriptFailure(['extendnew({a: 1}, [42])'], ['E1013: Argument 2: type mismatch, expected dict<number> but got list<number>', 'E712: Argument of extendnew() must be a List or Dictionary'])
  CheckDefAndScriptFailure(['extendnew([1, 2], "x")'], ['E1013: Argument 2: type mismatch, expected list<number> but got string', 'E712: Argument of extendnew() must be a List or Dictionary'])
  CheckDefAndScriptFailure(['extendnew([1, 2], {x: 1})'], ['E1013: Argument 2: type mismatch, expected list<number> but got dict<number>', 'E712: Argument of extendnew() must be a List or Dictionary'])
enddef

def Test_feedkeys()
  CheckDefAndScriptFailure(['feedkeys(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['feedkeys("x", 10)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['feedkeys([], {})'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 1'])
  g:TestVar = 1
  feedkeys(":g:TestVar = 789\n", 'xt')
  assert_equal(789, g:TestVar)
  unlet g:TestVar
enddef

def Test_filereadable()
  assert_false(filereadable(""))
  assert_false(filereadable(test_null_string()))

  CheckDefExecFailure(['echo filereadable(123)'], 'E1013:')
  CheckDefExecFailure(['echo filereadable(true)'], 'E1013:')
enddef

def Test_filewritable()
  assert_false(filewritable(""))
  assert_false(filewritable(test_null_string()))

  CheckDefExecFailure(['echo filewritable(123)'], 'E1013:')
  CheckDefExecFailure(['echo filewritable(true)'], 'E1013:')
enddef

def Test_finddir()
  mkdir('Xtestdir')
  finddir('Xtestdir', '**', -1)->assert_equal(['Xtestdir'])
  var lines =<< trim END
      var l: list<string> = finddir('nothing', '*;', -1)
  END
  CheckDefAndScriptSuccess(lines)
  delete('Xtestdir', 'rf')

  CheckDefAndScriptFailure(['finddir(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['finddir(v:null)'], ['E1013: Argument 1: type mismatch, expected string but got special', 'E1174: String required for argument 1'])
  CheckDefExecFailure(['echo finddir("")'], 'E1175:')
  CheckDefAndScriptFailure(['finddir("a", [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['finddir("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  finddir('abc', '')->assert_equal('')

  CheckDefFailure(['var s: list<string> = finddir("foo")'], 'E1012: Type mismatch; expected list<string> but got string')
  CheckDefFailure(['var s: list<string> = finddir("foo", "path")'], 'E1012: Type mismatch; expected list<string> but got string')
  # with third argument only runtime type checking
  CheckDefCompileSuccess(['var s: list<string> = finddir("foo", "path", 1)'])
enddef

def Test_findfile()
  findfile('runtest.vim', '**', -1)->assert_equal(['runtest.vim'])
  var lines =<< trim END
      var l: list<string> = findfile('nothing', '*;', -1)
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefExecFailure(['findfile(true)'], 'E1013: Argument 1: type mismatch, expected string but got bool')
  CheckDefExecFailure(['findfile(v:null)'], 'E1013: Argument 1: type mismatch, expected string but got special')
  CheckDefExecFailure(['findfile("")'], 'E1175:')
  CheckDefAndScriptFailure(['findfile("a", [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['findfile("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  findfile('abc', '')->assert_equal('')
enddef

def Test_flatten()
  var lines =<< trim END
      echo flatten([1, 2, 3])
  END
  CheckDefAndScriptFailure(lines, 'E1158:')
enddef

def Test_flattennew()
  var lines =<< trim END
      var l = [1, [2, [3, 4]], 5]
      call assert_equal([1, 2, 3, 4, 5], flattennew(l))
      call assert_equal([1, [2, [3, 4]], 5], l)

      call assert_equal([1, 2, [3, 4], 5], flattennew(l, 1))
      call assert_equal([1, [2, [3, 4]], 5], l)

      var ll: list<list<string>> = [['a', 'b', 'c']]
      assert_equal(['a', 'b', 'c'], ll->flattennew())
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(['flattennew({})'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<unknown>', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['flattennew([], "1")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

" Test for float functions argument type
def Test_float_funcs_args()
  CheckFeature float

  # acos()
  CheckDefAndScriptFailure(['acos("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # asin()
  CheckDefAndScriptFailure(['asin("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # atan()
  CheckDefAndScriptFailure(['atan("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # atan2()
  CheckDefAndScriptFailure(['atan2("a", 1.1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  CheckDefAndScriptFailure(['atan2(1.2, "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 2'])
  CheckDefAndScriptFailure(['atan2(1.2)'], ['E119:', 'E119:'])
  # ceil()
  CheckDefAndScriptFailure(['ceil("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # cos()
  CheckDefAndScriptFailure(['cos("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # cosh()
  CheckDefAndScriptFailure(['cosh("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # exp()
  CheckDefAndScriptFailure(['exp("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # float2nr()
  CheckDefAndScriptFailure(['float2nr("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # floor()
  CheckDefAndScriptFailure(['floor("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # fmod()
  CheckDefAndScriptFailure(['fmod(1.1, "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 2'])
  CheckDefAndScriptFailure(['fmod("a", 1.1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  CheckDefAndScriptFailure(['fmod(1.1)'], ['E119:', 'E119:'])
  # isinf()
  CheckDefAndScriptFailure(['isinf("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # isnan()
  CheckDefAndScriptFailure(['isnan("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # log()
  CheckDefAndScriptFailure(['log("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # log10()
  CheckDefAndScriptFailure(['log10("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # pow()
  CheckDefAndScriptFailure(['pow("a", 1.1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  CheckDefAndScriptFailure(['pow(1.1, "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 2'])
  CheckDefAndScriptFailure(['pow(1.1)'], ['E119:', 'E119:'])
  # round()
  CheckDefAndScriptFailure(['round("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # sin()
  CheckDefAndScriptFailure(['sin("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # sinh()
  CheckDefAndScriptFailure(['sinh("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # sqrt()
  CheckDefAndScriptFailure(['sqrt("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # tan()
  CheckDefAndScriptFailure(['tan("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # tanh()
  CheckDefAndScriptFailure(['tanh("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
  # trunc()
  CheckDefAndScriptFailure(['trunc("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1219: Float or Number required for argument 1'])
enddef

def Test_fnameescape()
  CheckDefAndScriptFailure(['fnameescape(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal('\+a\%b\|', fnameescape('+a%b|'))
  fnameescape('')->assert_equal('')
enddef

def Test_fnamemodify()
  CheckDefSuccess(['echo fnamemodify(test_null_string(), ":p")'])
  CheckDefSuccess(['echo fnamemodify("", ":p")'])
  CheckDefSuccess(['echo fnamemodify("file", test_null_string())'])
  CheckDefSuccess(['echo fnamemodify("file", "")'])

  CheckDefExecFailure(['echo fnamemodify(true, ":p")'], 'E1013: Argument 1: type mismatch, expected string but got bool')
  CheckDefExecFailure(['echo fnamemodify(v:null, ":p")'], 'E1013: Argument 1: type mismatch, expected string but got special')
  CheckDefExecFailure(['echo fnamemodify("file", true)'],  'E1013: Argument 2: type mismatch, expected string but got bool')
enddef

def Wrong_dict_key_type(items: list<number>): list<number>
  return filter(items, (_, val) => get({[val]: 1}, 'x'))
enddef

def Test_filter()
  CheckDefAndScriptFailure(['filter(1.1, "1")'], ['E1013: Argument 1: type mismatch, expected list<any> but got float', 'E1251: List, Dictionary, Blob or String required for argument 1'])

  var lines =<< trim END
    def F(i: number, v: any): string
      return 'bad'
    enddef
    echo filter([1, 2, 3], F)
  END
  CheckDefAndScriptFailure(lines, ['E1013: Argument 2: type mismatch, expected func(...): bool', 'E1135: Using a String as a Bool:'])

  assert_equal([], filter([1, 2, 3], '0'))
  assert_equal([1, 2, 3], filter([1, 2, 3], '1'))
  assert_equal({b: 20}, filter({a: 10, b: 20}, 'v:val == 20'))

  def GetFiltered(): list<number>
    var Odd: func = (_, v) => v % 2
    return range(3)->filter(Odd)
  enddef
  assert_equal([1], GetFiltered())
enddef

def Test_filter_wrong_dict_key_type()
  assert_fails('Wrong_dict_key_type([1, v:null, 3])', 'E1013:')
enddef

def Test_filter_return_type()
  var l = filter([1, 2, 3], (_, _) => 1)
  var res = 0
  for n in l
    res += n
  endfor
  res->assert_equal(6)
enddef

def Test_filter_missing_argument()
  var dict = {aa: [1], ab: [2], ac: [3], de: [4]}
  var res = dict->filter((k, _) => k =~ 'a' && k !~ 'b')
  res->assert_equal({aa: [1], ac: [3]})
enddef

def Test_foldclosed()
  CheckDefAndScriptFailure(['foldclosed(function("min"))'], ['E1013: Argument 1: type mismatch, expected string but got func(...): unknown', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['foldclosed("")'], 'E1209: Invalid value for a line number')
  assert_equal(-1, foldclosed(1))
  assert_equal(-1, foldclosed('$'))
enddef

def Test_foldclosedend()
  CheckDefAndScriptFailure(['foldclosedend(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['foldclosedend("")'], 'E1209: Invalid value for a line number')
  assert_equal(-1, foldclosedend(1))
  assert_equal(-1, foldclosedend('w0'))
enddef

def Test_foldlevel()
  CheckDefAndScriptFailure(['foldlevel(0z10)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['foldlevel("")'], 'E1209: Invalid value for a line number')
  assert_equal(0, foldlevel(1))
  assert_equal(0, foldlevel('.'))
enddef

def Test_foldtextresult()
  CheckDefAndScriptFailure(['foldtextresult(1.1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['foldtextresult("")'], 'E1209: Invalid value for a line number')
  assert_equal('', foldtextresult(1))
  assert_equal('', foldtextresult('.'))
enddef

def Test_fullcommand()
  assert_equal('next', fullcommand('n'))
  assert_equal('noremap', fullcommand('no'))
  assert_equal('noremap', fullcommand('nor'))
  assert_equal('normal', fullcommand('norm'))

  assert_equal('', fullcommand('k'))
  assert_equal('keepmarks', fullcommand('ke'))
  assert_equal('keepmarks', fullcommand('kee'))
  assert_equal('keepmarks', fullcommand('keep'))
  assert_equal('keepjumps', fullcommand('keepj'))

  assert_equal('dlist', fullcommand('dl'))
  assert_equal('', fullcommand('dp'))
  assert_equal('delete', fullcommand('del'))
  assert_equal('', fullcommand('dell'))
  assert_equal('', fullcommand('delp'))

  assert_equal('srewind', fullcommand('sre'))
  assert_equal('scriptnames', fullcommand('scr'))
  assert_equal('', fullcommand('scg'))
  fullcommand('')->assert_equal('')
enddef

def Test_funcref()
  CheckDefAndScriptFailure(['funcref("reverse", 2)'], ['E1013: Argument 2: type mismatch, expected list<any> but got number', 'E1211: List required for argument 2'])
  CheckDefAndScriptFailure(['funcref("reverse", [2], [1])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])

  var lines =<< trim END
      vim9script
      def UseBool(b: bool)
      enddef
      def GetRefOk()
        var Ref1: func(bool) = funcref(UseBool)
        var Ref2: func(bool) = funcref('UseBool')
      enddef
      def GetRefBad()
        # only fails at runtime
        var Ref1: func(number) = funcref(UseBool)
      enddef
      defcompile
      GetRefOk()
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def UseBool(b: bool)
      enddef
      def GetRefBad()
        # only fails at runtime
        var Ref1: func(number) = funcref(UseBool)
      enddef
      GetRefBad()
  END
  CheckScriptFailure(lines, 'E1012: Type mismatch; expected func(number) but got func(bool)')
enddef

def Test_function()
  CheckDefExecAndScriptFailure(['function(123)'], 'E1256: String or function required for argument 1')

  CheckDefAndScriptFailure(['function("reverse", 2)'], ['E1013: Argument 2: type mismatch, expected list<any> but got number', 'E1211: List required for argument 2'])
  CheckDefAndScriptFailure(['function("reverse", [2], [1])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])

  var lines =<< trim END
      vim9script
      def UseBool(b: bool)
      enddef
      def GetRefOk()
        var Ref1: func(bool) = function(UseBool)
        var Ref2: func(bool) = function('UseBool')
      enddef
      def GetRefBad()
        # only fails at runtime
        var Ref1: func(number) = function(UseBool)
      enddef
      defcompile
      GetRefOk()
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def UseBool(b: bool)
      enddef
      def GetRefBad()
        # only fails at runtime
        var Ref1: func(number) = function(UseBool)
      enddef
      GetRefBad()
  END
  CheckScriptFailure(lines, 'E1012: Type mismatch; expected func(number) but got func(bool)')
enddef

def Test_garbagecollect()
  garbagecollect(true)
  CheckDefAndScriptFailure(['garbagecollect("1")'], ['E1013: Argument 1: type mismatch, expected bool but got string', 'E1212: Bool required for argument 1'])
  CheckDefAndScriptFailure(['garbagecollect(20)'], ['E1013: Argument 1: type mismatch, expected bool but got number', 'E1212: Bool required for argument 1'])
enddef

def Test_get()
  CheckDefAndScriptFailure(['get("a", 1)'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E896: Argument of get() must be a List, Dictionary or Blob'])
  [3, 5, 2]->get(1)->assert_equal(5)
  [3, 5, 2]->get(3)->assert_equal(0)
  [3, 5, 2]->get(3, 9)->assert_equal(9)
  assert_equal(get(0z102030, 2), 0x30)
  {a: 7, b: 11, c: 13}->get('c')->assert_equal(13)
  {10: 'a', 20: 'b', 30: 'd'}->get(20)->assert_equal('b')
  function('max')->get('name')->assert_equal('max')
  var F: func = function('min', [[5, 8, 6]])
  F->get('name')->assert_equal('min')
  F->get('args')->assert_equal([[5, 8, 6]])

  var lines =<< trim END
      vim9script
      def DoThat(): number
        var Getqflist: func = function('getqflist', [{id: 42}])
        return Getqflist()->get('id', 77)
      enddef
      assert_equal(0, DoThat())
  END
  CheckScriptSuccess(lines)
enddef

def Test_getbufinfo()
  var bufinfo = getbufinfo(bufnr())
  getbufinfo('%')->assert_equal(bufinfo)

  edit Xtestfile1
  hide edit Xtestfile2
  hide enew
  getbufinfo({bufloaded: true, buflisted: true, bufmodified: false})
      ->len()->assert_equal(3)
  bwipe Xtestfile1 Xtestfile2
  CheckDefAndScriptFailure(['getbufinfo(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1174: String required for argument 1'])
enddef

def Test_getbufline()
  e SomeFile
  var buf = bufnr()
  sp Otherfile
  var lines = ['aaa', 'bbb', 'ccc']
  setbufline(buf, 1, lines)
  getbufline('#', 1, '$')->assert_equal(lines)
  getbufline(-1, '$', '$')->assert_equal([])
  getbufline(-1, 1, '$')->assert_equal([])

  assert_fails('getbufline("", "$a", "$b")', ['E1030: Using a String as a Number: "$a"', 'E1030: Using a String as a Number: "$a"'])
  assert_fails('getbufline("", "$", "$b")', ['E1030: Using a String as a Number: "$b"', 'E1030: Using a String as a Number: "$b"'])
  bwipe!

  CheckDefAndScriptFailure(['getbufline([], 2)'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['getbufline("a", [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 2'])
  CheckDefAndScriptFailure(['getbufline("a", 2, 0z10)'], ['E1013: Argument 3: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 3'])
enddef

def Test_getbufvar()
  CheckDefAndScriptFailure(['getbufvar(true, "v")'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['getbufvar(1, 2, 3)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
enddef

def Test_getchangelist()
  new
  setline(1, 'some text')
  var changelist = bufnr()->getchangelist()
  getchangelist('%')->assert_equal(changelist)
  bwipe!
enddef

def Test_getchar()
  while getchar(0)
  endwhile
  getchar(true)->assert_equal(0)
  getchar(1)->assert_equal(0)
  CheckDefAndScriptFailure(['getchar(2)'], ['E1013: Argument 1: type mismatch, expected bool but got number', 'E1212: Bool required for argument 1'])
  CheckDefAndScriptFailure(['getchar("1")'], ['E1013: Argument 1: type mismatch, expected bool but got string', 'E1212: Bool required for argument 1'])
enddef

def Test_getcharpos()
  CheckDefAndScriptFailure(['getcharpos(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['getcharpos(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefExecAndScriptFailure(['getcharpos("")'], 'E1209: Invalid value for a line number')
enddef

def Test_getcharstr()
  CheckDefAndScriptFailure(['getcharstr(2)'], ['E1013: Argument 1: type mismatch, expected bool but got number', 'E1212: Bool required for argument 1'])
  CheckDefAndScriptFailure(['getcharstr("1")'], ['E1013: Argument 1: type mismatch, expected bool but got string', 'E1212: Bool required for argument 1'])
enddef

def Test_getcompletion()
  set wildignore=*.vim,*~
  var l = getcompletion('run', 'file', true)
  l->assert_equal([])
  set wildignore&
  CheckDefAndScriptFailure(['getcompletion(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['getcompletion("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['getcompletion("a", "b", 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
  CheckDefExecAndScriptFailure(['getcompletion("a", "")'], 'E475: Invalid argument')
  getcompletion('', 'messages')->assert_equal(['clear'])
enddef

def Test_getcurpos()
  CheckDefAndScriptFailure(['getcurpos("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_getcursorcharpos()
  CheckDefAndScriptFailure(['getcursorcharpos("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_getcwd()
  CheckDefAndScriptFailure(['getcwd("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['getcwd("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['getcwd(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_getenv()
  if getenv('does-not_exist') == ''
    assert_report('getenv() should return null')
  endif
  if getenv('does-not_exist') == null
  else
    assert_report('getenv() should return null')
  endif
  $SOMEENVVAR = 'some'
  assert_equal('some', getenv('SOMEENVVAR'))
  unlet $SOMEENVVAR
  getenv('')->assert_equal(v:null)
enddef

def Test_getfontname()
  CheckDefAndScriptFailure(['getfontname(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  #getfontname('')->assert_equal('')
enddef

def Test_getfperm()
  assert_equal('', getfperm(""))
  assert_equal('', getfperm(test_null_string()))

  CheckDefExecFailure(['echo getfperm(true)'], 'E1013:')
  CheckDefExecFailure(['echo getfperm(v:null)'], 'E1013:')
enddef

def Test_getfsize()
  assert_equal(-1, getfsize(""))
  assert_equal(-1, getfsize(test_null_string()))

  CheckDefExecFailure(['echo getfsize(true)'], 'E1013:')
  CheckDefExecFailure(['echo getfsize(v:null)'], 'E1013:')
enddef

def Test_getftime()
  assert_equal(-1, getftime(""))
  assert_equal(-1, getftime(test_null_string()))

  CheckDefExecFailure(['echo getftime(true)'], 'E1013:')
  CheckDefExecFailure(['echo getftime(v:null)'], 'E1013:')
enddef

def Test_getftype()
  assert_equal('', getftype(""))
  assert_equal('', getftype(test_null_string()))

  CheckDefExecFailure(['echo getftype(true)'], 'E1013:')
  CheckDefExecFailure(['echo getftype(v:null)'], 'E1013:')
enddef

def Test_getjumplist()
  CheckDefAndScriptFailure(['getjumplist("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['getjumplist("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['getjumplist(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_getline()
  var lines =<< trim END
      new
      setline(1, ['hello', 'there', 'again'])
      assert_equal('hello', getline(1))
      assert_equal('hello', getline('.'))

      normal 2Gvjv
      assert_equal('there', getline("'<"))
      assert_equal('again', getline("'>"))
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      echo getline('1')
  END
  CheckDefExecAndScriptFailure(lines, 'E1209:')
  CheckDefAndScriptFailure(['getline(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['getline(1, true)'], ['E1013: Argument 2: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 2'])
  CheckDefExecAndScriptFailure(['getline("")'], 'E1209: Invalid value for a line number')
enddef

def Test_getloclist()
  CheckDefAndScriptFailure(['getloclist("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['getloclist(1, [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_getloclist_return_type()
  var l = getloclist(1)
  l->assert_equal([])

  var d = getloclist(1, {items: 0})
  d->assert_equal({items: []})
enddef

def Test_getmarklist()
  CheckDefAndScriptFailure(['getmarklist([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  assert_equal([], getmarklist(10000))
  assert_fails('getmarklist("a%b@#")', 'E94:')
enddef

def Test_getmatches()
  CheckDefAndScriptFailure(['getmatches("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_getpos()
  CheckDefAndScriptFailure(['getpos(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal([0, 1, 1, 0], getpos('.'))
  CheckDefExecFailure(['getpos("a")'], 'E1209:')
  CheckDefExecAndScriptFailure(['getpos("")'], 'E1209: Invalid value for a line number')
enddef

def Test_getqflist()
  CheckDefAndScriptFailure(['getqflist([])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 1'])
  call assert_equal({}, getqflist({}))
enddef

def Test_getqflist_return_type()
  var l = getqflist()
  l->assert_equal([])

  var d = getqflist({items: 0})
  d->assert_equal({items: []})
enddef

def Test_getreg()
  var lines = ['aaa', 'bbb', 'ccc']
  setreg('a', lines)
  getreg('a', true, true)->assert_equal(lines)
  assert_fails('getreg("ab")', 'E1162:')
  CheckDefAndScriptFailure(['getreg(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['getreg(".", 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  CheckDefAndScriptFailure(['getreg(".", 1, "b")'], ['E1013: Argument 3: type mismatch, expected bool but got string', 'E1212: Bool required for argument 3'])
  @" = 'A1B2C3'
  getreg('')->assert_equal('A1B2C3')
enddef

def Test_getreg_return_type()
  var s1: string = getreg('"')
  var s2: string = getreg('"', 1)
  var s3: list<string> = getreg('"', 1, 1)
enddef

def Test_getreginfo()
  var text = 'abc'
  setreg('a', text)
  getreginfo('a')->assert_equal({regcontents: [text], regtype: 'v', isunnamed: false})
  assert_fails('getreginfo("ab")', 'E1162:')
  @" = 'D1E2F3'
  getreginfo('').regcontents->assert_equal(['D1E2F3'])
enddef

def Test_getregtype()
  var lines = ['aaa', 'bbb', 'ccc']
  setreg('a', lines)
  getregtype('a')->assert_equal('V')
  assert_fails('getregtype("ab")', 'E1162:')
  setreg('"', 'ABCD', 'b')
  getregtype('')->assert_equal("\<C-V>4")
enddef

def Test_gettabinfo()
  CheckDefAndScriptFailure(['gettabinfo("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_gettabvar()
  CheckDefAndScriptFailure(['gettabvar("a", "b")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['gettabvar(1, 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
enddef

def Test_gettabwinvar()
  CheckDefAndScriptFailure(['gettabwinvar("a", 2, "c")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['gettabwinvar(1, "b", "c", [])'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['gettabwinvar(1, 1, 3, {})'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
enddef

def Test_gettagstack()
  CheckDefAndScriptFailure(['gettagstack("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_gettext()
  CheckDefAndScriptFailure(['gettext(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefExecAndScriptFailure(['gettext("")'], 'E475: Invalid argument')
  assert_equal('abc', gettext("abc"))
  assert_fails('gettext("")', 'E475:')
enddef

def Test_getwininfo()
  CheckDefAndScriptFailure(['getwininfo("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_getwinpos()
  CheckDefAndScriptFailure(['getwinpos("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_getwinvar()
  CheckDefAndScriptFailure(['getwinvar("a", "b")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['getwinvar(1, 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
enddef

def Test_glob()
  glob('runtest.vim', true, true, true)->assert_equal(['runtest.vim'])
  CheckDefAndScriptFailure(['glob(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['glob("a", 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  CheckDefAndScriptFailure(['glob("a", 1, "b")'], ['E1013: Argument 3: type mismatch, expected bool but got string', 'E1212: Bool required for argument 3'])
  CheckDefAndScriptFailure(['glob("a", 1, true, 2)'], ['E1013: Argument 4: type mismatch, expected bool but got number', 'E1212: Bool required for argument 4'])
  glob('')->assert_equal('')
enddef

def Test_glob2regpat()
  CheckDefAndScriptFailure(['glob2regpat(null)'], ['E1013: Argument 1: type mismatch, expected string but got special', 'E1174: String required for argument 1'])
  glob2regpat('')->assert_equal('^$')
enddef

def Test_globpath()
  globpath('.', 'runtest.vim', true, true, true)->assert_equal(['./runtest.vim'])
  CheckDefAndScriptFailure(['globpath(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['globpath("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['globpath("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected bool but got string', 'E1212: Bool required for argument 3'])
  CheckDefAndScriptFailure(['globpath("a", "b", true, "d")'], ['E1013: Argument 4: type mismatch, expected bool but got string', 'E1212: Bool required for argument 4'])
  CheckDefAndScriptFailure(['globpath("a", "b", true, false, "e")'], ['E1013: Argument 5: type mismatch, expected bool but got string', 'E1212: Bool required for argument 5'])
  globpath('', '')->assert_equal('')
enddef

def Test_has()
  has('eval', true)->assert_equal(1)
  CheckDefAndScriptFailure(['has(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['has("a", "b")'], ['E1013: Argument 2: type mismatch, expected bool but got string', 'E1212: Bool required for argument 2'])
  has('')->assert_equal(0)
enddef

def Test_has_key()
  var d = {123: 'xx'}
  assert_true(has_key(d, '123'))
  assert_true(has_key(d, 123))
  assert_false(has_key(d, 'x'))
  assert_false(has_key(d, 99))

  CheckDefAndScriptFailure(['has_key([1, 2], "k")'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 1'])
  CheckDefAndScriptFailure(['has_key({"a": 10}, ["a"])'], ['E1013: Argument 2: type mismatch, expected string but got list<string>', 'E1220: String or Number required for argument 2'])
enddef

def Test_haslocaldir()
  CheckDefAndScriptFailure(['haslocaldir("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['haslocaldir("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['haslocaldir(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_hasmapto()
  hasmapto('foobar', 'i', true)->assert_equal(0)
  iabbrev foo foobar
  hasmapto('foobar', 'i', true)->assert_equal(1)
  iunabbrev foo
  CheckDefAndScriptFailure(['hasmapto(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['hasmapto("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['hasmapto("a", "b", 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
  hasmapto('', '')->assert_equal(0)
enddef

def Test_histadd()
  CheckDefAndScriptFailure(['histadd(1, "x")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['histadd(":", 10)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  histadd("search", 'skyblue')
  assert_equal('skyblue', histget('/', -1))
  histadd("search", '')->assert_equal(0)
enddef

def Test_histdel()
  CheckDefAndScriptFailure(['histdel(1, "x")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['histdel(":", true)'], ['E1013: Argument 2: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 2'])
  histdel('search', '')->assert_equal(0)
enddef

def Test_histget()
  CheckDefAndScriptFailure(['histget(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['histget("a", "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_histnr()
  CheckDefAndScriptFailure(['histnr(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal(-1, histnr('abc'))
enddef

def Test_hlID()
  CheckDefAndScriptFailure(['hlID(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal(0, hlID('NonExistingHighlight'))
  hlID('')->assert_equal(0)
enddef

def Test_hlexists()
  CheckDefAndScriptFailure(['hlexists([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 1'])
  assert_equal(0, hlexists('NonExistingHighlight'))
  hlexists('')->assert_equal(0)
enddef

def Test_hlget()
  CheckDefAndScriptFailure(['hlget([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 1'])
  hlget('')->assert_equal([])
enddef

def Test_hlset()
  CheckDefAndScriptFailure(['hlset("id")'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1211: List required for argument 1'])
  hlset([])->assert_equal(0)
enddef

def Test_iconv()
  CheckDefAndScriptFailure(['iconv(1, "from", "to")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['iconv("abc", 10, "to")'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['iconv("abc", "from", 20)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  assert_equal('abc', iconv('abc', 'fromenc', 'toenc'))
  iconv('', '', '')->assert_equal('')
enddef

def Test_indent()
  CheckDefAndScriptFailure(['indent([1])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['indent(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['indent("")'], 'E1209: Invalid value for a line number')
  CheckDefExecAndScriptFailure(['indent(-1)'], 'E966: Invalid line number: -1')
  assert_equal(0, indent(1))
enddef

def Test_index()
  index(['a', 'b', 'a', 'B'], 'b', 2, true)->assert_equal(3)
  CheckDefAndScriptFailure(['index("a", "a")'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1226: List or Blob required for argument 1'])
  CheckDefFailure(['index(["1"], 1)'], 'E1013: Argument 2: type mismatch, expected string but got number')
  CheckDefAndScriptFailure(['index(0z10, "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['index([1], 1, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['index(0z1020, 10, 1, 2)'], ['E1013: Argument 4: type mismatch, expected bool but got number', 'E1212: Bool required for argument 4'])
enddef

def Test_input()
  CheckDefAndScriptFailure(['input(5)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['input(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['input("p", 10)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['input("p", "q", 20)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
enddef

def Test_inputdialog()
  CheckDefAndScriptFailure(['inputdialog(5)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['inputdialog(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['inputdialog("p", 10)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['inputdialog("p", "q", 20)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
enddef

def Test_inputlist()
  CheckDefAndScriptFailure(['inputlist(10)'], ['E1013: Argument 1: type mismatch, expected list<string> but got number', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['inputlist("abc")'], ['E1013: Argument 1: type mismatch, expected list<string> but got string', 'E1211: List required for argument 1'])
  CheckDefFailure(['inputlist([1, 2, 3])'], 'E1013: Argument 1: type mismatch, expected list<string> but got list<number>')
  feedkeys("2\<CR>", 't')
  var r: number = inputlist(['a', 'b', 'c'])
  assert_equal(2, r)
enddef

def Test_inputsecret()
  CheckDefAndScriptFailure(['inputsecret(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['inputsecret("Pass:", 20)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  feedkeys("\<CR>", 't')
  var ans: string = inputsecret('Pass:', '123')
  assert_equal('123', ans)
enddef

let s:number_one = 1
let s:number_two = 2
let s:string_keep = 'keep'

def Test_insert()
  var l = insert([2, 1], 3)
  var res = 0
  for n in l
    res += n
  endfor
  res->assert_equal(6)

  var m: any = []
  insert(m, 4)
  call assert_equal([4], m)
  extend(m, [6], 0)
  call assert_equal([6, 4], m)

  var lines =<< trim END
      insert(test_null_list(), 123)
  END
  CheckDefExecAndScriptFailure(lines, 'E1130:', 1)

  lines =<< trim END
      insert(test_null_blob(), 123)
  END
  CheckDefExecAndScriptFailure(lines, 'E1131:', 1)

  assert_equal([1, 2, 3], insert([2, 3], 1))
  assert_equal([1, 2, 3], insert([2, 3], s:number_one))
  assert_equal([1, 2, 3], insert([1, 2], 3, 2))
  assert_equal([1, 2, 3], insert([1, 2], 3, s:number_two))
  assert_equal(['a', 'b', 'c'], insert(['b', 'c'], 'a'))
  assert_equal(0z1234, insert(0z34, 0x12))

  CheckDefAndScriptFailure(['insert("a", 1)'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1226: List or Blob required for argument 1'])
  CheckDefFailure(['insert([2, 3], "a")'], 'E1013: Argument 2: type mismatch, expected number but got string')
  CheckDefAndScriptFailure(['insert([2, 3], 1, "x")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
enddef

def Test_invert()
  CheckDefAndScriptFailure(['invert("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_isdirectory()
  CheckDefAndScriptFailure(['isdirectory(1.1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1174: String required for argument 1'])
  assert_false(isdirectory('NonExistingDir'))
  assert_false(isdirectory(''))
enddef

def Test_islocked()
  CheckDefAndScriptFailure(['islocked(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['var n1: number = 10', 'islocked(n1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  g:v1 = 10
  assert_false(islocked('g:v1'))
  lockvar g:v1
  assert_true(islocked('g:v1'))
  unlet g:v1
  islocked('')->assert_equal(-1)
enddef

def Test_items()
  CheckDefFailure(['[]->items()'], 'E1013: Argument 1: type mismatch, expected dict<any> but got list<unknown>')
  assert_equal([['a', 10], ['b', 20]], {'a': 10, 'b': 20}->items())
  assert_equal([], {}->items())
enddef

def Test_job_getchannel()
  if !has('job')
    CheckFeature job
  else
    CheckDefAndScriptFailure(['job_getchannel("a")'], ['E1013: Argument 1: type mismatch, expected job but got string', 'E1218: Job required for argument 1'])
    assert_fails('job_getchannel(test_null_job())', 'E916: not a valid job')
  endif
enddef

def Test_job_info()
  if !has('job')
    CheckFeature job
  else
    CheckDefAndScriptFailure(['job_info("a")'], ['E1013: Argument 1: type mismatch, expected job but got string', 'E1218: Job required for argument 1'])
    assert_fails('job_info(test_null_job())', 'E916: not a valid job')
  endif
enddef

" Test_job_info_return_type() is in test_vim9_fails.vim

def Test_job_setoptions()
  if !has('job')
    CheckFeature job
  else
    CheckDefAndScriptFailure(['job_setoptions(test_null_channel(), {})'], ['E1013: Argument 1: type mismatch, expected job but got channel', 'E1218: Job required for argument 1'])
    CheckDefAndScriptFailure(['job_setoptions(test_null_job(), [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
    assert_equal('fail', job_status(test_null_job()))
  endif
enddef

def Test_job_status()
  if !has('job')
    CheckFeature job
  else
    CheckDefAndScriptFailure(['job_status("a")'], ['E1013: Argument 1: type mismatch, expected job but got string', 'E1218: Job required for argument 1'])
    assert_equal('fail', job_status(test_null_job()))
  endif
enddef

def Test_job_stop()
  if !has('job')
    CheckFeature job
  else
    CheckDefAndScriptFailure(['job_stop("a")'], ['E1013: Argument 1: type mismatch, expected job but got string', 'E1218: Job required for argument 1'])
    CheckDefAndScriptFailure(['job_stop(test_null_job(), true)'], ['E1013: Argument 2: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 2'])
  endif
enddef

def Test_join()
  CheckDefAndScriptFailure(['join("abc")'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['join([], 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  join([''], '')->assert_equal('')
enddef

def Test_js_decode()
  CheckDefAndScriptFailure(['js_decode(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal([1, 2], js_decode('[1,2]'))
  js_decode('')->assert_equal(v:none)
enddef

def Test_json_decode()
  CheckDefAndScriptFailure(['json_decode(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1174: String required for argument 1'])
  assert_equal(1.0, json_decode('1.0'))
  json_decode('')->assert_equal(v:none)
enddef

def Test_keys()
  CheckDefAndScriptFailure(['keys([])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 1'])
  assert_equal(['a'], {a: 'v'}->keys())
  assert_equal([], {}->keys())
enddef

def Test_keys_return_type()
  const var: list<string> = {a: 1, b: 2}->keys()
  var->assert_equal(['a', 'b'])
enddef

def Test_len()
  CheckDefAndScriptFailure(['len(true)'], ['E1013: Argument 1: type mismatch, expected list<any> but got bool', 'E701: Invalid type for len()'])
  assert_equal(2, "ab"->len())
  assert_equal(3, 456->len())
  assert_equal(0, []->len())
  assert_equal(1, {a: 10}->len())
  assert_equal(4, 0z20304050->len())
enddef

def Test_libcall()
  CheckFeature libcall
  CheckDefAndScriptFailure(['libcall(1, "b", 3)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['libcall("a", 2, 3)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['libcall("a", "b", 1.1)'], ['E1013: Argument 3: type mismatch, expected string but got float', 'E1220: String or Number required for argument 3'])
enddef

def Test_libcallnr()
  CheckFeature libcall
  CheckDefAndScriptFailure(['libcallnr(1, "b", 3)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['libcallnr("a", 2, 3)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['libcallnr("a", "b", 1.1)'], ['E1013: Argument 3: type mismatch, expected string but got float', 'E1220: String or Number required for argument 3'])
enddef

def Test_line()
  assert_fails('line(true)', 'E1174:')
  CheckDefAndScriptFailure(['line(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['line(".", "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefExecAndScriptFailure(['line("")'], 'E1209: Invalid value for a line number')
enddef

def Test_line2byte()
  CheckDefAndScriptFailure(['line2byte(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['line2byte("")'], 'E1209: Invalid value for a line number')
  assert_equal(-1, line2byte(1))
  assert_equal(-1, line2byte(10000))
enddef

def Test_lispindent()
  CheckDefAndScriptFailure(['lispindent({})'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['lispindent("")'], 'E1209: Invalid value for a line number')
  CheckDefExecAndScriptFailure(['lispindent(-1)'], 'E966: Invalid line number: -1')
  assert_equal(0, lispindent(1))
enddef

def Test_list2blob()
  CheckDefAndScriptFailure(['list2blob(10)'], ['E1013: Argument 1: type mismatch, expected list<number> but got number', 'E1211: List required for argument 1'])
  CheckDefFailure(['list2blob([0z10, 0z02])'], 'E1013: Argument 1: type mismatch, expected list<number> but got list<blob>')
enddef

def Test_list2str_str2list_utf8()
  var s = "\u3042\u3044"
  var l = [0x3042, 0x3044]
  str2list(s, true)->assert_equal(l)
  list2str(l, true)->assert_equal(s)
enddef

def Test_list2str()
  CheckDefAndScriptFailure(['list2str(".", true)'], ['E1013: Argument 1: type mismatch, expected list<number> but got string', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['list2str([1], 0z10)'], ['E1013: Argument 2: type mismatch, expected bool but got blob', 'E1212: Bool required for argument 2'])
enddef

def SID(): number
  return expand('<SID>')
          ->matchstr('<SNR>\zs\d\+\ze_$')
          ->str2nr()
enddef

def Test_listener_add()
  CheckDefAndScriptFailure(['listener_add("1", true)'], ['E1013: Argument 2: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 2'])
enddef

def Test_listener_flush()
  CheckDefAndScriptFailure(['listener_flush([1])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
enddef

def Test_listener_remove()
  CheckDefAndScriptFailure(['listener_remove("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_luaeval()
  if !has('lua')
    CheckFeature lua
  endif
  CheckDefAndScriptFailure(['luaeval(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  if exists_compiled('*luaeval')
    luaeval('')->assert_equal(v:null)
  endif
enddef

def Test_map()
  if has('channel')
    CheckDefAndScriptFailure(['map(test_null_channel(), "1")'], ['E1013: Argument 1: type mismatch, expected list<any> but got channel', 'E1251: List, Dictionary, Blob or String required for argument 1'])
  endif
  CheckDefAndScriptFailure(['map(1, "1")'], ['E1013: Argument 1: type mismatch, expected list<any> but got number', 'E1251: List, Dictionary, Blob or String required for argument 1'])

  # type of dict remains dict<any> even when type of values changes
  # same for list
  var lines =<< trim END
      var d: dict<any> = {a: 0}
      d->map((k, v) => true)
      d->map((k, v) => 'x')
      assert_equal({a: 'x'}, d)

      d = {a: 0}
      g:gd = d
      map(g:gd, (k, v) => true)
      assert_equal({a: true}, g:gd)

      var l: list<any> = [0]
      l->map((k, v) => true)
      l->map((k, v) => 'x')
      assert_equal(['x'], l)

      l = [1]
      g:gl = l
      map(g:gl, (k, v) => true)
      assert_equal([true], g:gl)
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_map_failure()
  CheckFeature job

  var lines =<< trim END
      vim9script
      writefile([], 'Xtmpfile')
      silent e Xtmpfile
      var d = {[bufnr('%')]: {a: 0}}
      au BufReadPost * Func()
      def Func()
          if d->has_key('')
          endif
          eval d[expand('<abuf>')]->mapnew((_, v: dict<job>) => 0)
      enddef
      e
  END
  CheckScriptFailure(lines, 'E1013:')
  au! BufReadPost
  delete('Xtmpfile')

  lines =<< trim END
      var d: dict<number> = {a: 1}
      g:gd = d
      map(g:gd, (k, v) => true)
  END
  CheckDefExecAndScriptFailure(lines, 'E1012: Type mismatch; expected number but got bool')
enddef

def Test_map_function_arg()
  var lines =<< trim END
      def MapOne(i: number, v: string): string
        return i .. ':' .. v
      enddef
      var l = ['a', 'b', 'c']
      map(l, MapOne)
      assert_equal(['0:a', '1:b', '2:c'], l)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      range(3)->map((a, b, c) => a + b + c)
  END
  CheckDefExecAndScriptFailure(lines, 'E1190: One argument too few')
  lines =<< trim END
      range(3)->map((a, b, c, d) => a + b + c + d)
  END
  CheckDefExecAndScriptFailure(lines, 'E1190: 2 arguments too few')

  lines =<< trim END
    def Map(i: number, v: number): string
      return 'bad'
    enddef
    echo map([1, 2, 3], Map)
  END
  CheckDefAndScriptFailure(lines, ['E1013: Argument 2: type mismatch, expected func(...): number but got func(number, number): string', 'E1012: Type mismatch; expected number but got string in map()'])
enddef

def Test_map_item_type()
  var lines =<< trim END
      var l = ['a', 'b', 'c']
      map(l, (k, v) => k .. '/' .. v )
      assert_equal(['0/a', '1/b', '2/c'], l)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
    var l: list<number> = [0]
    echo map(l, (_, v) => [])
  END
  CheckDefAndScriptFailure(lines, ['E1013: Argument 2: type mismatch, expected func(...): number but got func(any, any): list<unknown>', 'E1012: Type mismatch; expected number but got list<unknown> in map()'], 2)

  lines =<< trim END
    var l: list<number> = range(2)
    echo map(l, (_, v) => [])
  END
  CheckDefAndScriptFailure(lines, ['E1013: Argument 2: type mismatch, expected func(...): number but got func(any, any): list<unknown>', 'E1012: Type mismatch; expected number but got list<unknown> in map()'], 2)

  lines =<< trim END
    var d: dict<number> = {key: 0}
    echo map(d, (_, v) => [])
  END
  CheckDefAndScriptFailure(lines, ['E1013: Argument 2: type mismatch, expected func(...): number but got func(any, any): list<unknown>', 'E1012: Type mismatch; expected number but got list<unknown> in map()'], 2)
enddef

def Test_maparg()
  var lnum = str2nr(expand('<sflnum>'))
  map foo bar
  maparg('foo', '', false, true)->assert_equal({
        lnum: lnum + 1,
        script: 0,
        mode: ' ',
        silent: 0,
        noremap: 0,
        lhs: 'foo',
        lhsraw: 'foo',
        nowait: 0,
        expr: 0,
        sid: SID(),
        scriptversion: 999999,
        rhs: 'bar',
        buffer: 0})
  unmap foo
  CheckDefAndScriptFailure(['maparg(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['maparg("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['maparg("a", "b", 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
  CheckDefAndScriptFailure(['maparg("a", "b", true, 2)'], ['E1013: Argument 4: type mismatch, expected bool but got number', 'E1212: Bool required for argument 4'])
  maparg('')->assert_equal('')
enddef

def Test_maparg_mapset()
  nnoremap <F3> :echo "hit F3"<CR>
  var mapsave = maparg('<F3>', 'n', false, true)
  mapset('n', false, mapsave)

  nunmap <F3>
enddef

def Test_mapcheck()
  iabbrev foo foobar
  mapcheck('foo', 'i', true)->assert_equal('foobar')
  iunabbrev foo
  CheckDefAndScriptFailure(['mapcheck(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['mapcheck("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['mapcheck("a", "b", 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
  mapcheck('')->assert_equal('')
  mapcheck('', '')->assert_equal('')
enddef

def Test_mapnew()
  if has('channel')
    CheckDefAndScriptFailure(['mapnew(test_null_job(), "1")'], ['E1013: Argument 1: type mismatch, expected list<any> but got job', 'E1251: List, Dictionary, Blob or String required for argument 1'])
  endif
  CheckDefAndScriptFailure(['mapnew(1, "1")'], ['E1013: Argument 1: type mismatch, expected list<any> but got number', 'E1251: List, Dictionary, Blob or String required for argument 1'])
enddef

def Test_mapset()
  CheckDefAndScriptFailure(['mapset(1, true, {})'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['mapset("a", 2, {})'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  CheckDefAndScriptFailure(['mapset("a", false, [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_match()
  CheckDefAndScriptFailure(['match(0z12, "p")'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['match(["s"], [2])'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['match("s", "p", "q")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['match("s", "p", 1, "r")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  assert_equal(2, match('ab12cd', '12'))
  assert_equal(-1, match('ab12cd', '34'))
  assert_equal(6, match('ab12cd12ef', '12', 4))
  assert_equal(2, match('abcd', '..', 0, 3))
  assert_equal(1, match(['a', 'b', 'c'], 'b'))
  assert_equal(-1, match(['a', 'b', 'c'], 'd'))
  assert_equal(3, match(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2))
  assert_equal(5, match(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2, 2))
  match('', 'a')->assert_equal(-1)
  match('abc', '')->assert_equal(0)
  match('', '')->assert_equal(0)
enddef

def Test_matchadd()
  CheckDefAndScriptFailure(['matchadd(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['matchadd("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['matchadd("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['matchadd("a", "b", 1, "d")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  CheckDefAndScriptFailure(['matchadd("a", "b", 1, 1, [])'], ['E1013: Argument 5: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 5'])
  matchadd('', 'a')->assert_equal(-1)
  matchadd('Search', '')->assert_equal(-1)
enddef

def Test_matchaddpos()
  CheckDefAndScriptFailure(['matchaddpos(1, [1])'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['matchaddpos("a", "b")'], ['E1013: Argument 2: type mismatch, expected list<any> but got string', 'E1211: List required for argument 2'])
  CheckDefAndScriptFailure(['matchaddpos("a", [1], "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['matchaddpos("a", [1], 1, "d")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  CheckDefAndScriptFailure(['matchaddpos("a", [1], 1, 1, [])'], ['E1013: Argument 5: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 5'])
  matchaddpos('', [1])->assert_equal(-1)
enddef

def Test_matcharg()
  CheckDefAndScriptFailure(['matcharg("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_matchdelete()
  CheckDefAndScriptFailure(['matchdelete("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['matchdelete("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['matchdelete(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_matchend()
  CheckDefAndScriptFailure(['matchend(0z12, "p")'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['matchend(["s"], [2])'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['matchend("s", "p", "q")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['matchend("s", "p", 1, "r")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  assert_equal(4, matchend('ab12cd', '12'))
  assert_equal(-1, matchend('ab12cd', '34'))
  assert_equal(8, matchend('ab12cd12ef', '12', 4))
  assert_equal(4, matchend('abcd', '..', 0, 3))
  assert_equal(1, matchend(['a', 'b', 'c'], 'b'))
  assert_equal(-1, matchend(['a', 'b', 'c'], 'd'))
  assert_equal(3, matchend(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2))
  assert_equal(5, matchend(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2, 2))
  matchend('', 'a')->assert_equal(-1)
  matchend('abc', '')->assert_equal(0)
  matchend('', '')->assert_equal(0)
enddef

def Test_matchfuzzy()
  CheckDefAndScriptFailure(['matchfuzzy({}, "p")'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<unknown>', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['matchfuzzy([], 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['matchfuzzy([], "a", [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
  matchfuzzy(['abc', 'xyz'], '')->assert_equal([])
enddef

def Test_matchfuzzypos()
  CheckDefAndScriptFailure(['matchfuzzypos({}, "p")'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<unknown>', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['matchfuzzypos([], 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['matchfuzzypos([], "a", [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
  matchfuzzypos(['abc', 'xyz'], '')->assert_equal([[], [], []])
enddef

def Test_matchlist()
  CheckDefAndScriptFailure(['matchlist(0z12, "p")'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['matchlist(["s"], [2])'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['matchlist("s", "p", "q")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['matchlist("s", "p", 1, "r")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  var l: list<string> = ['12',  '', '', '', '', '', '', '', '', '']
  assert_equal(l, matchlist('ab12cd', '12'))
  assert_equal([], matchlist('ab12cd', '34'))
  assert_equal(l, matchlist('ab12cd12ef', '12', 4))
  l[0] = 'cd'
  assert_equal(l, matchlist('abcd', '..', 0, 3))
  l[0] = 'b'
  assert_equal(l, matchlist(['a', 'b', 'c'], 'b'))
  assert_equal([], matchlist(['a', 'b', 'c'], 'd'))
  assert_equal(l, matchlist(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2))
  assert_equal(l, matchlist(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2, 2))
  matchlist('', 'a')->assert_equal([])
  matchlist('abc', '')->assert_equal(repeat([''], 10))
  matchlist('', '')->assert_equal(repeat([''], 10))
enddef

def Test_matchstr()
  CheckDefAndScriptFailure(['matchstr(0z12, "p")'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['matchstr(["s"], [2])'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['matchstr("s", "p", "q")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['matchstr("s", "p", 1, "r")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  assert_equal('12', matchstr('ab12cd', '12'))
  assert_equal('', matchstr('ab12cd', '34'))
  assert_equal('12', matchstr('ab12cd12ef', '12', 4))
  assert_equal('cd', matchstr('abcd', '..', 0, 3))
  assert_equal('b', matchstr(['a', 'b', 'c'], 'b'))
  assert_equal('', matchstr(['a', 'b', 'c'], 'd'))
  assert_equal('b', matchstr(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2))
  assert_equal('b', matchstr(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2, 2))
  matchstr('', 'a')->assert_equal('')
  matchstr('abc', '')->assert_equal('')
  matchstr('', '')->assert_equal('')
enddef

def Test_matchstrpos()
  CheckDefAndScriptFailure(['matchstrpos(0z12, "p")'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['matchstrpos(["s"], [2])'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['matchstrpos("s", "p", "q")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['matchstrpos("s", "p", 1, "r")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  assert_equal(['12', 2, 4], matchstrpos('ab12cd', '12'))
  assert_equal(['', -1, -1], matchstrpos('ab12cd', '34'))
  assert_equal(['12', 6, 8], matchstrpos('ab12cd12ef', '12', 4))
  assert_equal(['cd', 2, 4], matchstrpos('abcd', '..', 0, 3))
  assert_equal(['b', 1, 0, 1], matchstrpos(['a', 'b', 'c'], 'b'))
  assert_equal(['', -1, -1, -1], matchstrpos(['a', 'b', 'c'], 'd'))
  assert_equal(['b', 3, 0, 1],
                    matchstrpos(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2))
  assert_equal(['b', 5, 0, 1],
                    matchstrpos(['a', 'b', 'c', 'b', 'd', 'b'], 'b', 2, 2))
  matchstrpos('', 'a')->assert_equal(['', -1, -1])
  matchstrpos('abc', '')->assert_equal(['', 0, 0])
  matchstrpos('', '')->assert_equal(['', 0, 0])
enddef

def Test_max()
  g:flag = true
  var l1: list<number> = g:flag
          ? [1, max([2, 3])]
          : [4, 5]
  assert_equal([1, 3], l1)

  g:flag = false
  var l2: list<number> = g:flag
          ? [1, max([2, 3])]
          : [4, 5]
  assert_equal([4, 5], l2)
  CheckDefAndScriptFailure(['max(5)'], ['E1013: Argument 1: type mismatch, expected list<any> but got number', 'E1227: List or Dictionary required for argument 1'])
enddef

def Test_menu_info()
  CheckDefAndScriptFailure(['menu_info(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['menu_info(10, "n")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['menu_info("File", 10)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  assert_equal({}, menu_info('aMenu'))
enddef

def Test_min()
  g:flag = true
  var l1: list<number> = g:flag
          ? [1, min([2, 3])]
          : [4, 5]
  assert_equal([1, 2], l1)

  g:flag = false
  var l2: list<number> = g:flag
          ? [1, min([2, 3])]
          : [4, 5]
  assert_equal([4, 5], l2)
  CheckDefAndScriptFailure(['min(5)'], ['E1013: Argument 1: type mismatch, expected list<any> but got number', 'E1227: List or Dictionary required for argument 1'])
enddef

def Test_mkdir()
  CheckDefAndScriptFailure(['mkdir(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['mkdir("a", {})'], ['E1013: Argument 2: type mismatch, expected string but got dict<unknown>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['mkdir("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefExecAndScriptFailure(['mkdir("")'], 'E1175: Non-empty string required for argument 1')
  delete('a', 'rf')
enddef

def Test_mode()
  CheckDefAndScriptFailure(['mode("1")'], ['E1013: Argument 1: type mismatch, expected bool but got string', 'E1212: Bool required for argument 1'])
  CheckDefAndScriptFailure(['mode(2)'], ['E1013: Argument 1: type mismatch, expected bool but got number', 'E1212: Bool required for argument 1'])
enddef

def Test_mzeval()
  if !has('mzscheme')
    CheckFeature mzscheme
  endif
  CheckDefAndScriptFailure(['mzeval(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
enddef

def Test_nextnonblank()
  CheckDefAndScriptFailure(['nextnonblank(null)'], ['E1013: Argument 1: type mismatch, expected string but got special', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['nextnonblank("")'], 'E1209: Invalid value for a line number')
  assert_equal(0, nextnonblank(1))
enddef

def Test_nr2char()
  nr2char(97, true)->assert_equal('a')
  CheckDefAndScriptFailure(['nr2char("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['nr2char(1, "a")'], ['E1013: Argument 2: type mismatch, expected bool but got string', 'E1212: Bool required for argument 2'])
enddef

def Test_or()
  CheckDefAndScriptFailure(['or("x", 0x2)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['or(0x1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_pathshorten()
  CheckDefAndScriptFailure(['pathshorten(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['pathshorten("a", "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  pathshorten('')->assert_equal('')
enddef

def Test_perleval()
  if !has('perl')
    CheckFeature perl
  endif
  CheckDefAndScriptFailure(['perleval(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
enddef

def Test_popup_atcursor()
  CheckDefAndScriptFailure(['popup_atcursor({"a": 10}, {})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1224: String, Number or List required for argument 1'])
  CheckDefAndScriptFailure(['popup_atcursor("a", [1, 2])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 2'])

  # Pass variable of type 'any' to popup_atcursor()
  var what: any = 'Hello'
  var popupID = what->popup_atcursor({moved: 'any'})
  assert_equal(0, popupID->popup_getoptions().tabpage)
  popupID->popup_close()
enddef

def Test_popup_beval()
  CheckDefAndScriptFailure(['popup_beval({"a": 10}, {})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1224: String, Number or List required for argument 1'])
  CheckDefAndScriptFailure(['popup_beval("a", [1, 2])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_popup_clear()
  CheckDefAndScriptFailure(['popup_clear(["a"])'], ['E1013: Argument 1: type mismatch, expected bool but got list<string>', 'E1212: Bool required for argument 1'])
  CheckDefAndScriptFailure(['popup_clear(2)'], ['E1013: Argument 1: type mismatch, expected bool but got number', 'E1212: Bool required for argument 1'])
enddef

def Test_popup_close()
  CheckDefAndScriptFailure(['popup_close("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_popup_create()
  # Pass variable of type 'any' to popup_create()
  var what: any = 'Hello'
  var popupID = what->popup_create({})
  assert_equal(0, popupID->popup_getoptions().tabpage)
  popupID->popup_close()
enddef

def Test_popup_dialog()
  CheckDefAndScriptFailure(['popup_dialog({"a": 10}, {})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1224: String, Number or List required for argument 1'])
  CheckDefAndScriptFailure(['popup_dialog("a", [1, 2])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_popup_filter_menu()
  CheckDefAndScriptFailure(['popup_filter_menu("x", "")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_filter_menu(1, 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  var id: number = popup_menu(["one", "two", "three"], {})
  popup_filter_menu(id, '')
  popup_close(id)
enddef

def Test_popup_filter_yesno()
  CheckDefAndScriptFailure(['popup_filter_yesno("x", "")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_filter_yesno(1, 1)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
enddef

def Test_popup_getoptions()
  CheckDefAndScriptFailure(['popup_getoptions("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_getoptions(true)'], ['E1013: Argument 1: type mismatch, expected number but got bool', 'E1210: Number required for argument 1'])
enddef

def Test_popup_getpos()
  CheckDefAndScriptFailure(['popup_getpos("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_getpos(true)'], ['E1013: Argument 1: type mismatch, expected number but got bool', 'E1210: Number required for argument 1'])
enddef

def Test_popup_hide()
  CheckDefAndScriptFailure(['popup_hide("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_hide(true)'], ['E1013: Argument 1: type mismatch, expected number but got bool', 'E1210: Number required for argument 1'])
enddef

def Test_popup_locate()
  CheckDefAndScriptFailure(['popup_locate("a", 20)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_locate(10, "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_popup_menu()
  CheckDefAndScriptFailure(['popup_menu({"a": 10}, {})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1224: String, Number or List required for argument 1'])
  CheckDefAndScriptFailure(['popup_menu("a", [1, 2])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_popup_move()
  CheckDefAndScriptFailure(['popup_move("x", {})'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_move(1, [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_popup_notification()
  CheckDefAndScriptFailure(['popup_notification({"a": 10}, {})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1224: String, Number or List required for argument 1'])
  CheckDefAndScriptFailure(['popup_notification("a", [1, 2])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_popup_setoptions()
  CheckDefAndScriptFailure(['popup_setoptions("x", {})'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_setoptions(1, [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_popup_settext()
  CheckDefAndScriptFailure(['popup_settext("x", [])'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_settext(1, 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1222: String or List required for argument 2'])
enddef

def Test_popup_show()
  CheckDefAndScriptFailure(['popup_show("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['popup_show(true)'], ['E1013: Argument 1: type mismatch, expected number but got bool', 'E1210: Number required for argument 1'])
enddef

def Test_prevnonblank()
  CheckDefAndScriptFailure(['prevnonblank(null)'], ['E1013: Argument 1: type mismatch, expected string but got special', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['prevnonblank("")'], 'E1209: Invalid value for a line number')
  assert_equal(0, prevnonblank(1))
enddef

def Test_printf()
  CheckDefAndScriptFailure(['printf([1])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  printf(0x10)->assert_equal('16')
  assert_equal(" abc", "abc"->printf("%4s"))
enddef

def Test_prompt_getprompt()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['prompt_getprompt([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
    assert_equal('', prompt_getprompt('NonExistingBuf'))
  endif
enddef

def Test_prompt_setcallback()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['prompt_setcallback(true, "1")'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  endif
enddef

def Test_prompt_setinterrupt()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['prompt_setinterrupt(true, "1")'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  endif
enddef

def Test_prompt_setprompt()
  if !has('channel')
    CheckFeature channel
  else
    CheckDefAndScriptFailure(['prompt_setprompt([], "p")'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
    CheckDefAndScriptFailure(['prompt_setprompt(1, [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 2'])
  endif
enddef

def Test_prop_add()
  CheckDefAndScriptFailure(['prop_add("a", 2, {})'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['prop_add(1, "b", {})'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['prop_add(1, 2, [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_prop_add_list()
  CheckDefAndScriptFailure(['prop_add_list([], [])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 1'])
  CheckDefAndScriptFailure(['prop_add_list({}, {})'], ['E1013: Argument 2: type mismatch, expected list<any> but got dict<unknown>', 'E1211: List required for argument 2'])
enddef

def Test_prop_clear()
  CheckDefAndScriptFailure(['prop_clear("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['prop_clear(1, "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['prop_clear(1, 2, [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_prop_find()
  CheckDefAndScriptFailure(['prop_find([1, 2])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 1'])
  CheckDefAndScriptFailure(['prop_find([1, 2], "k")'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 1'])
  CheckDefAndScriptFailure(['prop_find({"a": 10}, ["a"])'], ['E1013: Argument 2: type mismatch, expected string but got list<string>', 'E1174: String required for argument 2'])
  assert_fails("prop_find({}, '')", 'E474:')
enddef

def Test_prop_list()
  CheckDefAndScriptFailure(['prop_list("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['prop_list(1, [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_prop_remove()
  CheckDefAndScriptFailure(['prop_remove([])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 1'])
  CheckDefAndScriptFailure(['prop_remove({}, "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['prop_remove({}, 1, "b")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
enddef

def Test_prop_type_add()
  CheckDefAndScriptFailure(['prop_type_add({"a": 10}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['prop_type_add("a", "b")'], ['E1013: Argument 2: type mismatch, expected dict<any> but got string', 'E1206: Dictionary required for argument 2'])
  assert_fails("prop_type_add('', {highlight: 'Search'})", 'E474:')
enddef

def Test_prop_type_change()
  CheckDefAndScriptFailure(['prop_type_change({"a": 10}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['prop_type_change("a", "b")'], ['E1013: Argument 2: type mismatch, expected dict<any> but got string', 'E1206: Dictionary required for argument 2'])
  assert_fails("prop_type_change('', {highlight: 'Search'})", 'E474:')
enddef

def Test_prop_type_delete()
  CheckDefAndScriptFailure(['prop_type_delete({"a": 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['prop_type_delete({"a": 10}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['prop_type_delete("a", "b")'], ['E1013: Argument 2: type mismatch, expected dict<any> but got string', 'E1206: Dictionary required for argument 2'])
  assert_fails("prop_type_delete('')", 'E474:')
enddef

def Test_prop_type_get()
  CheckDefAndScriptFailure(['prop_type_get({"a": 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['prop_type_get({"a": 10}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['prop_type_get("a", "b")'], ['E1013: Argument 2: type mismatch, expected dict<any> but got string', 'E1206: Dictionary required for argument 2'])
  assert_fails("prop_type_get('')", 'E474:')
enddef

def Test_prop_type_list()
  CheckDefAndScriptFailure(['prop_type_list(["a"])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<string>', 'E1206: Dictionary required for argument 1'])
  CheckDefAndScriptFailure(['prop_type_list(2)'], ['E1013: Argument 1: type mismatch, expected dict<any> but got number', 'E1206: Dictionary required for argument 1'])
enddef

def Test_py3eval()
  if !has('python3')
    CheckFeature python3
  endif
  CheckDefAndScriptFailure(['py3eval([2])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
enddef

def Test_pyeval()
  if !has('python')
    CheckFeature python
  endif
  CheckDefAndScriptFailure(['pyeval([2])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
enddef

def Test_pyxeval()
  if !has('python') && !has('python3')
    CheckFeature python
  endif
  CheckDefAndScriptFailure(['pyxeval([2])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
enddef

def Test_rand()
  CheckDefAndScriptFailure(['rand(10)'], ['E1013: Argument 1: type mismatch, expected list<number> but got number', 'E1211: List required for argument 1'])
  CheckDefFailure(['rand(["a"])'], 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>')
  assert_true(rand() >= 0)
  assert_true(rand(srand()) >= 0)
enddef

def Test_range()
  CheckDefAndScriptFailure(['range("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['range(10, "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['range(10, 20, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
enddef

def Test_readdir()
  eval expand('sautest')->readdir((e) => e[0] !=# '.')
  eval expand('sautest')->readdirex((e) => e.name[0] !=# '.')
  CheckDefAndScriptFailure(['readdir(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['readdir("a", "1", [3])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])
  if has('unix')
    # only fails on Unix-like systems
    assert_fails('readdir("")', 'E484: Can''t open file')
  endif
enddef

def Test_readdirex()
  CheckDefAndScriptFailure(['readdirex(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['readdirex("a", "1", [3])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])
  if has('unix')
    # only fails on Unix-like systems
    assert_fails('readdirex("")', 'E484: Can''t open file')
  endif
enddef

def Test_readblob()
  var blob = 0z12341234
  writefile(blob, 'Xreadblob')
  var read: blob = readblob('Xreadblob')
  assert_equal(blob, read)

  var lines =<< trim END
      var read: list<string> = readblob('Xreadblob')
  END
  CheckDefAndScriptFailure(lines, 'E1012: Type mismatch; expected list<string> but got blob', 1)
  CheckDefExecAndScriptFailure(['readblob("")'], 'E484: Can''t open file <empty>')
  delete('Xreadblob')
enddef

def Test_readfile()
  var text = ['aaa', 'bbb', 'ccc']
  writefile(text, 'Xreadfile')
  var read: list<string> = readfile('Xreadfile')
  assert_equal(text, read)

  var lines =<< trim END
      var read: dict<string> = readfile('Xreadfile')
  END
  CheckDefAndScriptFailure(lines, 'E1012: Type mismatch; expected dict<string> but got list<string>', 1)
  delete('Xreadfile')

  CheckDefAndScriptFailure(['readfile("a", 0z10)'], ['E1013: Argument 2: type mismatch, expected string but got blob', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['readfile("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefExecAndScriptFailure(['readfile("")'], 'E1175: Non-empty string required for argument 1')
enddef

def Test_reduce()
  CheckDefAndScriptFailure(['reduce({a: 10}, "1")'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<number>', 'E1252: String, List or Blob required for argument 1'])
  assert_equal(6, [1, 2, 3]->reduce((r, c) => r + c, 0))
  assert_equal(11, 0z0506->reduce((r, c) => r + c, 0))
enddef

def Test_reltime()
  CheckFeature reltime

  CheckDefExecAndScriptFailure(['[]->reltime()'], 'E474:')
  CheckDefExecAndScriptFailure(['[]->reltime([])'], 'E474:')

  CheckDefAndScriptFailure(['reltime("x")'], ['E1013: Argument 1: type mismatch, expected list<number> but got string', 'E1211: List required for argument 1'])
  CheckDefFailure(['reltime(["x", "y"])'], 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>')
  CheckDefAndScriptFailure(['reltime([1, 2], 10)'], ['E1013: Argument 2: type mismatch, expected list<number> but got number', 'E1211: List required for argument 2'])
  CheckDefFailure(['reltime([1, 2], ["a", "b"])'], 'E1013: Argument 2: type mismatch, expected list<number> but got list<string>')
  var start: list<any> = reltime()
  assert_true(type(reltime(start)) == v:t_list)
  var end: list<any> = reltime()
  assert_true(type(reltime(start, end)) == v:t_list)
enddef

def Test_reltimefloat()
  CheckFeature reltime

  CheckDefExecAndScriptFailure(['[]->reltimefloat()'], 'E474:')

  CheckDefAndScriptFailure(['reltimefloat("x")'], ['E1013: Argument 1: type mismatch, expected list<number> but got string', 'E1211: List required for argument 1'])
  CheckDefFailure(['reltimefloat([1.1])'], 'E1013: Argument 1: type mismatch, expected list<number> but got list<float>')
  assert_true(type(reltimefloat(reltime())) == v:t_float)
enddef

def Test_reltimestr()
  CheckFeature reltime

  CheckDefExecAndScriptFailure(['[]->reltimestr()'], 'E474:')

  CheckDefAndScriptFailure(['reltimestr(true)'], ['E1013: Argument 1: type mismatch, expected list<number> but got bool', 'E1211: List required for argument 1'])
  CheckDefFailure(['reltimestr([true])'], 'E1013: Argument 1: type mismatch, expected list<number> but got list<bool>')
  assert_true(type(reltimestr(reltime())) == v:t_string)
enddef

def Test_remote_expr()
  CheckFeature clientserver
  CheckEnv DISPLAY
  CheckDefAndScriptFailure(['remote_expr(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['remote_expr("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['remote_expr("a", "b", 3)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  CheckDefAndScriptFailure(['remote_expr("a", "b", "c", "d")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  CheckDefExecAndScriptFailure(['remote_expr("", "")'], 'E241: Unable to send to ')
enddef

def Test_remote_foreground()
  CheckFeature clientserver
  # remote_foreground() doesn't fail on MS-Windows
  CheckNotMSWindows
  CheckEnv DISPLAY

  CheckDefAndScriptFailure(['remote_foreground(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_fails('remote_foreground("NonExistingServer")', 'E241:')
  assert_fails('remote_foreground("")', 'E241:')
enddef

def Test_remote_peek()
  CheckFeature clientserver
  CheckEnv DISPLAY
  CheckDefAndScriptFailure(['remote_peek(0z10)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['remote_peek("a5b6c7", [1])'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1174: String required for argument 2'])
  CheckDefExecAndScriptFailure(['remote_peek("")'], 'E573: Invalid server id used')
enddef

def Test_remote_read()
  CheckFeature clientserver
  CheckEnv DISPLAY
  CheckDefAndScriptFailure(['remote_read(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['remote_read("a", "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefExecAndScriptFailure(['remote_read("")'], 'E573: Invalid server id used')
enddef

def Test_remote_send()
  CheckFeature clientserver
  CheckEnv DISPLAY
  CheckDefAndScriptFailure(['remote_send(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['remote_send("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['remote_send("a", "b", 3)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  assert_fails('remote_send("", "")', 'E241:')
enddef

def Test_remote_startserver()
  CheckFeature clientserver
  CheckEnv DISPLAY
  CheckDefAndScriptFailure(['remote_startserver({})'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', 'E1174: String required for argument 1'])
enddef

def Test_remove_const_list()
  var l: list<number> = [1, 2, 3, 4]
  assert_equal([1, 2], remove(l, 0, 1))
  assert_equal([3, 4], l)
enddef

def Test_remove()
  CheckDefAndScriptFailure(['remove("a", 1)'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1228: List, Dictionary or Blob required for argument 1'])
  CheckDefAndScriptFailure(['remove([], "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['remove([], 1, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['remove({}, 1.1)'], ['E1013: Argument 2: type mismatch, expected string but got float', 'E1220: String or Number required for argument 2'])
  CheckDefAndScriptFailure(['remove(0z10, "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['remove(0z20, 1, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  var l: any = [1, 2, 3, 4]
  remove(l, 1)
  assert_equal([1, 3, 4], l)
  remove(l, 0, 1)
  assert_equal([4], l)
  var b: any = 0z1234.5678.90
  remove(b, 1)
  assert_equal(0z1256.7890, b)
  remove(b, 1, 2)
  assert_equal(0z1290, b)
  var d: any = {a: 10, b: 20, c: 30}
  remove(d, 'b')
  assert_equal({a: 10, c: 30}, d)
  var d2: any = {1: 'a', 2: 'b', 3: 'c'}
  remove(d2, 2)
  assert_equal({1: 'a', 3: 'c'}, d2)
enddef

def Test_remove_return_type()
  var l = remove({one: [1, 2], two: [3, 4]}, 'one')
  var res = 0
  for n in l
    res += n
  endfor
  res->assert_equal(3)
enddef

def Test_rename()
  CheckDefAndScriptFailure(['rename(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['rename("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  rename('', '')->assert_equal(0)
enddef

def Test_repeat()
  CheckDefAndScriptFailure(['repeat(1.1, 2)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1224: String, Number or List required for argument 1'])
  CheckDefAndScriptFailure(['repeat({a: 10}, 2)'], ['E1013: Argument 1: type mismatch, expected string but got dict<', 'E1224: String, Number or List required for argument 1'])
  var lines =<< trim END
      assert_equal('aaa', repeat('a', 3))
      assert_equal('111', repeat(1, 3))
      assert_equal([1, 1, 1], repeat([1], 3))
      var s = '-'
      s ..= repeat(5, 3)
      assert_equal('-555', s)
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_resolve()
  CheckDefAndScriptFailure(['resolve([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 1'])
  assert_equal('SomeFile', resolve('SomeFile'))
  resolve('')->assert_equal('')
enddef

def Test_reverse()
  CheckDefAndScriptFailure(['reverse(10)'], ['E1013: Argument 1: type mismatch, expected list<any> but got number', 'E1226: List or Blob required for argument 1'])
  CheckDefAndScriptFailure(['reverse("abc")'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1226: List or Blob required for argument 1'])
enddef

def Test_reverse_return_type()
  var l = reverse([1, 2, 3])
  var res = 0
  for n in l
    res += n
  endfor
  res->assert_equal(6)
enddef

def Test_rubyeval()
  if !has('ruby')
    CheckFeature ruby
  endif
  CheckDefAndScriptFailure(['rubyeval([2])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
enddef

def Test_screenattr()
  CheckDefAndScriptFailure(['screenattr("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['screenattr(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_screenchar()
  CheckDefAndScriptFailure(['screenchar("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['screenchar(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_screenchars()
  CheckDefAndScriptFailure(['screenchars("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['screenchars(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_screenpos()
  CheckDefAndScriptFailure(['screenpos("a", 1, 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['screenpos(1, "b", 1)'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['screenpos(1, 1, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  assert_equal({col: 1, row: 1, endcol: 1, curscol: 1}, screenpos(1, 1, 1))
enddef

def Test_screenstring()
  CheckDefAndScriptFailure(['screenstring("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['screenstring(1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_search()
  new
  setline(1, ['foo', 'bar'])
  var val = 0
  # skip expr returns boolean
  search('bar', 'W', 0, 0, () => val == 1)->assert_equal(2)
  :1
  search('bar', 'W', 0, 0, () => val == 0)->assert_equal(0)
  # skip expr returns number, only 0 and 1 are accepted
  :1
  search('bar', 'W', 0, 0, () => 0)->assert_equal(2)
  :1
  search('bar', 'W', 0, 0, () => 1)->assert_equal(0)
  assert_fails("search('bar', '', 0, 0, () => -1)", 'E1023:')
  assert_fails("search('bar', '', 0, 0, () => -1)", 'E1023:')

  setline(1, "find this word")
  normal gg
  var col = 7
  assert_equal(1, search('this', '', 0, 0, 'col(".") > col'))
  normal 0
  assert_equal([1, 6], searchpos('this', '', 0, 0, 'col(".") > col'))

  col = 5
  normal 0
  assert_equal(0, search('this', '', 0, 0, 'col(".") > col'))
  normal 0
  assert_equal([0, 0], searchpos('this', '', 0, 0, 'col(".") > col'))
  bwipe!
  CheckDefAndScriptFailure(['search(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['search("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['search("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['search("a", "b", 3, "d")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  new
  setline(1, "match this")
  CheckDefAndScriptFailure(['search("a", "", 9, 0, [0])'], ['E1013: Argument 5: type mismatch, expected func(...): any but got list<number>', 'E730: Using a List as a String'])
  bwipe!
enddef

def Test_searchcount()
  new
  setline(1, "foo bar")
  :/foo
  searchcount({recompute: true})
      ->assert_equal({
          exact_match: 1,
          current: 1,
          total: 1,
          maxcount: 99,
          incomplete: 0})
  bwipe!
  CheckDefAndScriptFailure(['searchcount([1])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 1'])
enddef

def Test_searchdecl()
  searchdecl('blah', true, true)->assert_equal(1)
  CheckDefAndScriptFailure(['searchdecl(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['searchdecl("a", 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  CheckDefAndScriptFailure(['searchdecl("a", true, 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
enddef

def Test_searchpair()
  new
  setline(1, "here { and } there")

  normal f{
  var col = 15
  assert_equal(1, searchpair('{', '', '}', '', 'col(".") > col'))
  assert_equal(12, col('.'))
  normal 0f{
  assert_equal([1, 12], searchpairpos('{', '', '}', '', 'col(".") > col'))

  col = 8
  normal 0f{
  assert_equal(0, searchpair('{', '', '}', '', 'col(".") > col'))
  assert_equal(6, col('.'))
  normal 0f{
  assert_equal([0, 0], searchpairpos('{', '', '}', '', 'col(".") > col'))

  # searchpair with empty strings
  normal 8|
  assert_equal(0, searchpair('', '', ''))
  assert_equal([0, 0], searchpairpos('', '', ''))

  var lines =<< trim END
      vim9script
      setline(1, '()')
      normal gg
      func RetList()
        return [0]
      endfunc
      def Fail()
        try
          searchpairpos('(', '', ')', 'nW', 'RetList()')
        catch
          g:caught = 'yes'
        endtry
      enddef
      Fail()
  END
  CheckScriptSuccess(lines)
  assert_equal('yes', g:caught)
  unlet g:caught
  bwipe!

  lines =<< trim END
      echo searchpair("a", "b", "c", "d", "f", 33)
  END
  CheckDefAndScriptFailure(lines, ['E1001: Variable not found: f', 'E475: Invalid argument: d'])

  var errors = ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1']
  CheckDefAndScriptFailure(['searchpair(1, "b", "c")'], errors)
  CheckDefAndScriptFailure(['searchpairpos(1, "b", "c")'], errors)

  errors = ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2']
  CheckDefAndScriptFailure(['searchpair("a", 2, "c")'], errors)
  CheckDefAndScriptFailure(['searchpairpos("a", 2, "c")'], errors)

  errors = ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3']
  CheckDefAndScriptFailure(['searchpair("a", "b", 3)'], errors)
  CheckDefAndScriptFailure(['searchpairpos("a", "b", 3)'], errors)

  errors = ['E1013: Argument 4: type mismatch, expected string but got number', 'E1174: String required for argument 4']
  CheckDefAndScriptFailure(['searchpair("a", "b", "c", 4)'], errors)

  new
  setline(1, "match this")
  errors = ['E1013: Argument 5: type mismatch, expected func(...): any but got list<number>', 'E730: Using a List as a String']
  CheckDefAndScriptFailure(['searchpair("a", "b", "c", "r", [0])'], errors)
  CheckDefAndScriptFailure(['searchpairpos("a", "b", "c", "r", [0])'], errors)
  bwipe!

  errors = ['E1013: Argument 6: type mismatch, expected number but got string', 'E1210: Number required for argument 6']
  CheckDefAndScriptFailure(['searchpair("a", "b", "c", "r", "1", "f")'], errors)
  CheckDefAndScriptFailure(['searchpairpos("a", "b", "c", "r", "1", "f")'], errors)

  errors = ['E1013: Argument 7: type mismatch, expected number but got string', 'E1210: Number required for argument 7']
  CheckDefAndScriptFailure(['searchpair("a", "b", "c", "r", "1", 3, "g")'], errors)
  CheckDefAndScriptFailure(['searchpairpos("a", "b", "c", "r", "1", 3, "g")'], errors)
enddef

def Test_searchpos()
  CheckDefAndScriptFailure(['searchpos(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['searchpos("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['searchpos("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['searchpos("a", "b", 3, "d")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  new
  setline(1, "match this")
  CheckDefAndScriptFailure(['searchpos("a", "", 9, 0, [0])'], ['E1013: Argument 5: type mismatch, expected func(...): any but got list<number>', 'E730: Using a List as a String'])
  bwipe!
enddef

def Test_server2client()
  CheckFeature clientserver
  CheckEnv DISPLAY
  CheckDefAndScriptFailure(['server2client(10, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['server2client("a", 10)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefExecAndScriptFailure(['server2client("", "a")'], 'E573: Invalid server id used')
  CheckDefExecAndScriptFailure(['server2client("", "")'], 'E573: Invalid server id used')
enddef

def Test_shellescape()
  CheckDefAndScriptFailure(['shellescape(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['shellescape("a", 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  if has('unix')
    assert_equal("''", shellescape(''))
  endif
enddef

def Test_set_get_bufline()
  # similar to Test_setbufline_getbufline()
  var lines =<< trim END
      new
      var b = bufnr('%')
      hide
      assert_equal(0, setbufline(b, 1, ['foo', 'bar']))
      assert_equal(['foo'], getbufline(b, 1))
      assert_equal(['bar'], getbufline(b, '$'))
      assert_equal(['foo', 'bar'], getbufline(b, 1, 2))
      exe "bd!" b
      assert_equal([], getbufline(b, 1, 2))

      split Xtest
      setline(1, ['a', 'b', 'c'])
      b = bufnr('%')
      wincmd w

      assert_equal(1, setbufline(b, 5, 'x'))
      assert_equal(1, setbufline(b, 5, ['x']))
      assert_equal(1, setbufline(b, 5, []))
      assert_equal(1, setbufline(b, 5, test_null_list()))

      assert_equal(1, 'x'->setbufline(bufnr('$') + 1, 1))
      assert_equal(1, ['x']->setbufline(bufnr('$') + 1, 1))
      assert_equal(1, []->setbufline(bufnr('$') + 1, 1))
      assert_equal(1, test_null_list()->setbufline(bufnr('$') + 1, 1))

      assert_equal(['a', 'b', 'c'], getbufline(b, 1, '$'))

      assert_equal(0, setbufline(b, 4, ['d', 'e']))
      assert_equal(['c'], b->getbufline(3))
      assert_equal(['d'], getbufline(b, 4))
      assert_equal(['e'], getbufline(b, 5))
      assert_equal([], getbufline(b, 6))
      assert_equal([], getbufline(b, 2, 1))

      if has('job')
        setbufline(b, 2, [function('eval'), {key: 123}, string(test_null_job())])
        assert_equal(["function('eval')",
                        "{'key': 123}",
                        "no process"],
                        getbufline(b, 2, 4))
      endif

      exe 'bwipe! ' .. b
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_setbufvar()
  setbufvar(bufnr('%'), '&syntax', 'vim')
  &syntax->assert_equal('vim')
  setbufvar(bufnr('%'), '&ts', 16)
  &ts->assert_equal(16)
  setbufvar(bufnr('%'), '&ai', true)
  &ai->assert_equal(true)
  setbufvar(bufnr('%'), '&ft', 'filetype')
  &ft->assert_equal('filetype')

  settabwinvar(1, 1, '&syntax', 'vam')
  &syntax->assert_equal('vam')
  settabwinvar(1, 1, '&ts', 15)
  &ts->assert_equal(15)
  setlocal ts=8
  settabwinvar(1, 1, '&list', false)
  &list->assert_equal(false)
  settabwinvar(1, 1, '&list', true)
  &list->assert_equal(true)
  setlocal list&

  setbufvar('%', 'myvar', 123)
  getbufvar('%', 'myvar')->assert_equal(123)

  CheckDefAndScriptFailure(['setbufvar(true, "v", 3)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['setbufvar(1, 2, 3)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  assert_fails('setbufvar("%", "", 10)', 'E461: Illegal variable name')
enddef

def Test_setbufline()
  new
  var bnum = bufnr('%')
  :wincmd w
  setbufline(bnum, 1, range(1, 3))
  setbufline(bnum, 4, 'one')
  setbufline(bnum, 5, 10)
  setbufline(bnum, 6, ['two', 11])
  assert_equal(['1', '2', '3', 'one', '10', 'two', '11'], getbufline(bnum, 1, '$'))
  CheckDefAndScriptFailure(['setbufline([1], 1, "x")'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['setbufline(1, [1], "x")'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 2'])
  CheckDefExecAndScriptFailure(['setbufline(' .. bnum .. ', -1, "x")'], 'E966: Invalid line number: -1')
  CheckDefAndScriptFailure(['setbufline(1, 1, {"a": 10})'], ['E1013: Argument 3: type mismatch, expected string but got dict<number>', 'E1224: String, Number or List required for argument 3'])
  bnum->bufwinid()->win_gotoid()
  setbufline('', 1, 'nombres')
  getline(1)->assert_equal('nombres')
  bw!
enddef

def Test_setcellwidths()
  CheckDefAndScriptFailure(['setcellwidths(1)'], ['E1013: Argument 1: type mismatch, expected list<any> but got number', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['setcellwidths({"a": 10})'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<number>', 'E1211: List required for argument 1'])
enddef

def Test_setcharpos()
  CheckDefAndScriptFailure(['setcharpos(1, [])'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefFailure(['setcharpos(".", ["a"])'], 'E1013: Argument 2: type mismatch, expected list<number> but got list<string>')
  CheckDefAndScriptFailure(['setcharpos(".", 1)'], ['E1013: Argument 2: type mismatch, expected list<number> but got number', 'E1211: List required for argument 2'])
  CheckDefExecAndScriptFailure(['setcharpos("", [0, 1, 1, 1])'], 'E474: Invalid argument')
enddef

def Test_setcharsearch()
  CheckDefAndScriptFailure(['setcharsearch("x")'], ['E1013: Argument 1: type mismatch, expected dict<any> but got string', 'E1206: Dictionary required for argument 1'])
  CheckDefAndScriptFailure(['setcharsearch([])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 1'])
  var d: dict<any> = {char: 'x', forward: 1, until: 1}
  setcharsearch(d)
  assert_equal(d, getcharsearch())
enddef

def Test_setcmdpos()
  CheckDefAndScriptFailure(['setcmdpos("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_setcursorcharpos()
  CheckDefAndScriptFailure(['setcursorcharpos(0z10, 1)'], ['E1013: Argument 1: type mismatch, expected number but got blob', 'E1224: String, Number or List required for argument 1'])
  CheckDefAndScriptFailure(['setcursorcharpos(1, "2")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['setcursorcharpos(1, 2, "3")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefExecAndScriptFailure(['setcursorcharpos("", 10)'], 'E1209: Invalid value for a line number')
enddef

def Test_setenv()
  CheckDefAndScriptFailure(['setenv(1, 2)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal(0, setenv('', ''))
  assert_equal(0, setenv('', v:null))
enddef

def Test_setfperm()
  CheckDefAndScriptFailure(['setfperm(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['setfperm("a", 0z10)'], ['E1013: Argument 2: type mismatch, expected string but got blob', 'E1174: String required for argument 2'])
  CheckDefExecAndScriptFailure(['setfperm("Xfile", "")'], 'E475: Invalid argument')
  CheckDefExecAndScriptFailure(['setfperm("", "")'], 'E475: Invalid argument')
  assert_equal(0, setfperm('', 'rw-r--r--'))
enddef

def Test_setline()
  new
  setline(1, range(1, 4))
  assert_equal(['1', '2', '3', '4'], getline(1, '$'))
  setline(1, ['a', 'b', 'c', 'd'])
  assert_equal(['a', 'b', 'c', 'd'], getline(1, '$'))
  setline(1, 'one')
  assert_equal(['one', 'b', 'c', 'd'], getline(1, '$'))
  setline(1, 10)
  assert_equal(['10', 'b', 'c', 'd'], getline(1, '$'))
  CheckDefAndScriptFailure(['setline([1], "x")'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  CheckDefExecAndScriptFailure(['setline("", "x")'], 'E1209: Invalid value for a line number')
  CheckDefExecAndScriptFailure(['setline(-1, "x")'], 'E966: Invalid line number: -1')
  assert_fails('setline(".a", "x")', ['E1209:', 'E1209:'])
  bw!
enddef

def Test_setloclist()
  var items = [{filename: '/tmp/file', lnum: 1, valid: true}]
  var what = {items: items}
  setqflist([], ' ', what)
  setloclist(0, [], ' ', what)
  CheckDefAndScriptFailure(['setloclist("1", [])'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['setloclist(1, 2)'], ['E1013: Argument 2: type mismatch, expected list<any> but got number', 'E1211: List required for argument 2'])
  CheckDefAndScriptFailure(['setloclist(1, [], 3)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  CheckDefAndScriptFailure(['setloclist(1, [], "a", [])'], ['E1013: Argument 4: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 4'])
enddef

def Test_setmatches()
  CheckDefAndScriptFailure(['setmatches({})'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<unknown>', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['setmatches([], "1")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_setpos()
  CheckDefAndScriptFailure(['setpos(1, [])'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefFailure(['setpos(".", ["a"])'], 'E1013: Argument 2: type mismatch, expected list<number> but got list<string>')
  CheckDefAndScriptFailure(['setpos(".", 1)'], ['E1013: Argument 2: type mismatch, expected list<number> but got number', 'E1211: List required for argument 2'])
  CheckDefExecAndScriptFailure(['setpos("", [0, 1, 1, 1])'], 'E474: Invalid argument')
enddef

def Test_setqflist()
  CheckDefAndScriptFailure(['setqflist(1, "")'], ['E1013: Argument 1: type mismatch, expected list<any> but got number', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['setqflist([], 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['setqflist([], "", [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_setreg()
  setreg('a', ['aaa', 'bbb', 'ccc'])
  var reginfo = getreginfo('a')
  setreg('a', reginfo)
  getreginfo('a')->assert_equal(reginfo)
  assert_fails('setreg("ab", 0)', 'E1162:')
  CheckDefAndScriptFailure(['setreg(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['setreg("a", "b", 3)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  setreg('', '1a2b3c')
  assert_equal('1a2b3c', @")
enddef

def Test_settabvar()
  CheckDefAndScriptFailure(['settabvar("a", "b", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['settabvar(1, 2, "c")'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  assert_fails('settabvar(1, "", 10)', 'E461: Illegal variable name')
enddef

def Test_settabwinvar()
  CheckDefAndScriptFailure(['settabwinvar("a", 2, "c", true)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['settabwinvar(1, "b", "c", [])'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['settabwinvar(1, 1, 3, {})'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  assert_fails('settabwinvar(1, 1, "", 10)', 'E461: Illegal variable name')
enddef

def Test_settagstack()
  CheckDefAndScriptFailure(['settagstack(true, {})'], ['E1013: Argument 1: type mismatch, expected number but got bool', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['settagstack(1, [1])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 2'])
  CheckDefAndScriptFailure(['settagstack(1, {}, 2)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  assert_fails('settagstack(1, {}, "")', 'E962: Invalid action')
enddef

def Test_setwinvar()
  CheckDefAndScriptFailure(['setwinvar("a", "b", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['setwinvar(1, 2, "c")'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  assert_fails('setwinvar(1, "", 10)', 'E461: Illegal variable name')
enddef

def Test_sha256()
  CheckDefAndScriptFailure(['sha256(100)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['sha256(0zABCD)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1174: String required for argument 1'])
  assert_equal('ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad', sha256('abc'))
  assert_equal('e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855', sha256(''))
enddef

def Test_shiftwidth()
  CheckDefAndScriptFailure(['shiftwidth("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_sign_define()
  CheckDefAndScriptFailure(['sign_define({"a": 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['sign_define({"a": 10}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['sign_define("a", ["b"])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<string>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_sign_getdefined()
  CheckDefAndScriptFailure(['sign_getdefined(["x"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['sign_getdefined(2)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  sign_getdefined('')->assert_equal([])
enddef

def Test_sign_getplaced()
  CheckDefAndScriptFailure(['sign_getplaced(["x"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['sign_getplaced(1, ["a"])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<string>', 'E1206: Dictionary required for argument 2'])
  CheckDefAndScriptFailure(['sign_getplaced("a", 1.1)'], ['E1013: Argument 2: type mismatch, expected dict<any> but got float', 'E1206: Dictionary required for argument 2'])
  CheckDefExecAndScriptFailure(['sign_getplaced(bufnr(), {lnum: ""})'], 'E1030: Using a String as a Number:')
  sign_getplaced('')->assert_equal([{signs: [], bufnr: bufnr()}])
enddef

def Test_sign_jump()
  CheckDefAndScriptFailure(['sign_jump("a", "b", "c")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['sign_jump(1, 2, 3)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['sign_jump(1, "b", true)'], ['E1013: Argument 3: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 3'])
enddef

def Test_sign_place()
  CheckDefAndScriptFailure(['sign_place("a", "b", "c", "d")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['sign_place(1, 2, "c", "d")'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['sign_place(1, "b", 3, "d")'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  CheckDefAndScriptFailure(['sign_place(1, "b", "c", 1.1)'], ['E1013: Argument 4: type mismatch, expected string but got float', 'E1220: String or Number required for argument 4'])
  CheckDefAndScriptFailure(['sign_place(1, "b", "c", "d", [1])'], ['E1013: Argument 5: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 5'])
  CheckDefExecAndScriptFailure(['sign_place(0, "", "MySign", bufnr(), {lnum: ""})'], 'E1209: Invalid value for a line number: ""')
  assert_fails("sign_place(0, '', '', '')", 'E155:')
enddef

def Test_sign_placelist()
  CheckDefAndScriptFailure(['sign_placelist("x")'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['sign_placelist({"a": 10})'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<number>', 'E1211: List required for argument 1'])
  CheckDefExecAndScriptFailure(['sign_placelist([{"name": "MySign", "buffer": bufnr(), "lnum": ""}])'], 'E1209: Invalid value for a line number: ""')
  assert_fails('sign_placelist([{name: "MySign", buffer: "", lnum: 1}])', 'E155:')
enddef

def Test_sign_undefine()
  CheckDefAndScriptFailure(['sign_undefine({})'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['sign_undefine([1])'], ['E1013: Argument 1: type mismatch, expected list<string> but got list<number>', 'E155: Unknown sign:'])
enddef

def Test_sign_unplace()
  CheckDefAndScriptFailure(['sign_unplace({"a": 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['sign_unplace({"a": 10}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['sign_unplace("a", ["b"])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<string>', 'E1206: Dictionary required for argument 2'])
enddef

def Test_sign_unplacelist()
  CheckDefAndScriptFailure(['sign_unplacelist("x")'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['sign_unplacelist({"a": 10})'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<number>', 'E1211: List required for argument 1'])
enddef

def Test_simplify()
  CheckDefAndScriptFailure(['simplify(100)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  call assert_equal('NonExistingFile', simplify('NonExistingFile'))
  simplify('')->assert_equal('')
enddef

def Test_slice()
  assert_equal('12345', slice('012345', 1))
  assert_equal('123', slice('012345', 1, 4))
  assert_equal('1234', slice('012345', 1, -1))
  assert_equal('1', slice('012345', 1, -4))
  assert_equal('', slice('012345', 1, -5))
  assert_equal('', slice('012345', 1, -6))

  assert_equal([1, 2, 3, 4, 5], slice(range(6), 1))
  assert_equal([1, 2, 3], slice(range(6), 1, 4))
  assert_equal([1, 2, 3, 4], slice(range(6), 1, -1))
  assert_equal([1], slice(range(6), 1, -4))
  assert_equal([], slice(range(6), 1, -5))
  assert_equal([], slice(range(6), 1, -6))

  assert_equal(0z1122334455, slice(0z001122334455, 1))
  assert_equal(0z112233, slice(0z001122334455, 1, 4))
  assert_equal(0z11223344, slice(0z001122334455, 1, -1))
  assert_equal(0z11, slice(0z001122334455, 1, -4))
  assert_equal(0z, slice(0z001122334455, 1, -5))
  assert_equal(0z, slice(0z001122334455, 1, -6))
  CheckDefAndScriptFailure(['slice({"a": 10}, 1)'], ['E1013: Argument 1: type mismatch, expected list<any> but got dict<number>', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['slice([1, 2, 3], "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['slice("abc", 1, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
enddef

def Test_spellsuggest()
  if !has('spell')
    CheckFeature spell
  else
    spellsuggest('marrch', 1, true)->assert_equal(['March'])
  endif
  CheckDefAndScriptFailure(['spellsuggest(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['spellsuggest("a", "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['spellsuggest("a", 1, 0z01)'], ['E1013: Argument 3: type mismatch, expected bool but got blob', 'E1212: Bool required for argument 3'])
  spellsuggest('')->assert_equal([])
enddef

def Test_sound_playevent()
  CheckFeature sound
  CheckDefAndScriptFailure(['sound_playevent(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
enddef

def Test_sound_playfile()
  CheckFeature sound
  CheckDefAndScriptFailure(['sound_playfile(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
enddef

def Test_sound_stop()
  CheckFeature sound
  CheckDefAndScriptFailure(['sound_stop("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_soundfold()
  CheckDefAndScriptFailure(['soundfold(20)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal('abc', soundfold('abc'))
  assert_equal('', soundfold(''))
enddef

def Test_sort_return_type()
  var res: list<number>
  res = [1, 2, 3]->sort()
enddef

def Test_sort_argument()
  var lines =<< trim END
    var res = ['b', 'a', 'c']->sort('i')
    res->assert_equal(['a', 'b', 'c'])

    def Compare(a: number, b: number): number
      return a - b
    enddef
    var l = [3, 6, 7, 1, 8, 2, 4, 5]
    sort(l, Compare)
    assert_equal([1, 2, 3, 4, 5, 6, 7, 8], l)
  END
  CheckDefAndScriptSuccess(lines)
  CheckDefAndScriptFailure(['sort("a")'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['sort([1], "", [1])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_sort_compare_func_fails()
  var lines =<< trim END
    vim9script
    echo ['a', 'b', 'c']->sort((a: number, b: number) => 0)
  END
  writefile(lines, 'Xbadsort')
  assert_fails('source Xbadsort', ['E1013:', 'E702:'])

  delete('Xbadsort')
enddef

def Test_spellbadword()
  CheckDefAndScriptFailure(['spellbadword(100)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  spellbadword('good')->assert_equal(['', ''])
  spellbadword('')->assert_equal(['', ''])
enddef

def Test_split()
  split('  aa  bb  ', '\W\+', true)->assert_equal(['', 'aa', 'bb', ''])
  CheckDefAndScriptFailure(['split(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['split("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['split("a", "b", 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
  split('')->assert_equal([])
  split('', '')->assert_equal([])
enddef

def Test_srand()
  CheckDefAndScriptFailure(['srand("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  type(srand(100))->assert_equal(v:t_list)
enddef

def Test_state()
  CheckDefAndScriptFailure(['state({})'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', 'E1174: String required for argument 1'])
  assert_equal('', state('a'))
enddef

def Test_str2float()
  if !has('float')
    CheckFeature float
  else
    str2float("1.00")->assert_equal(1.00)
    str2float("2e-2")->assert_equal(0.02)
    str2float('')->assert_equal(0.0)

    CheckDefAndScriptFailure(['str2float(123)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  endif
enddef

def Test_str2list()
  CheckDefAndScriptFailure(['str2list(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['str2list("a", 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  assert_equal([97], str2list('a'))
  assert_equal([97], str2list('a', 1))
  assert_equal([97], str2list('a', true))
  str2list('')->assert_equal([])
enddef

def Test_str2nr()
  str2nr("1'000'000", 10, true)->assert_equal(1000000)
  str2nr('')->assert_equal(0)

  CheckDefAndScriptFailure(['str2nr(123)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['str2nr("123", "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['str2nr("123", 10, "x")'], ['E1013: Argument 3: type mismatch, expected bool but got string', 'E1212: Bool required for argument 3'])
enddef

def Test_strcharlen()
  CheckDefAndScriptFailure(['strcharlen([1])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  "abc"->strcharlen()->assert_equal(3)
  strcharlen(99)->assert_equal(2)
enddef

def Test_strcharpart()
  CheckDefAndScriptFailure(['strcharpart(1, 2)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['strcharpart("a", "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['strcharpart("a", 1, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['strcharpart("a", 1, 1, 2)'], ['E1013: Argument 4: type mismatch, expected bool but got number', 'E1212: Bool required for argument 4'])
  strcharpart('', 0)->assert_equal('')
enddef

def Test_strchars()
  strchars("A\u20dd", true)->assert_equal(1)
  CheckDefAndScriptFailure(['strchars(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['strchars("a", 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
  assert_equal(3, strchars('abc'))
  assert_equal(3, strchars('abc', 1))
  assert_equal(3, strchars('abc', true))
  strchars('')->assert_equal(0)
enddef

def Test_strdisplaywidth()
  CheckDefAndScriptFailure(['strdisplaywidth(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['strdisplaywidth("a", "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  strdisplaywidth('')->assert_equal(0)
enddef

def Test_strftime()
  if exists('*strftime')
    CheckDefAndScriptFailure(['strftime(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
    CheckDefAndScriptFailure(['strftime("a", "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
    strftime('')->assert_equal('')
  endif
enddef

def Test_strgetchar()
  CheckDefAndScriptFailure(['strgetchar(1, 1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['strgetchar("a", "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  strgetchar('', 0)->assert_equal(-1)
  strgetchar('', 1)->assert_equal(-1)
enddef

def Test_stridx()
  CheckDefAndScriptFailure(['stridx([1], "b")'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['stridx("a", {})'], ['E1013: Argument 2: type mismatch, expected string but got dict<unknown>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['stridx("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  stridx('', '')->assert_equal(0)
  stridx('', 'a')->assert_equal(-1)
  stridx('a', '')->assert_equal(0)
enddef

def Test_strlen()
  CheckDefAndScriptFailure(['strlen([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  "abc"->strlen()->assert_equal(3)
  strlen(99)->assert_equal(2)
enddef

def Test_strpart()
  CheckDefAndScriptFailure(['strpart(1, 2)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['strpart("a", "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['strpart("a", 1, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['strpart("a", 1, 1, 2)'], ['E1013: Argument 4: type mismatch, expected bool but got number', 'E1212: Bool required for argument 4'])
  strpart('', 0)->assert_equal('')
enddef

def Test_strptime()
  CheckFunction strptime
  if exists_compiled('*strptime')
    CheckDefAndScriptFailure(['strptime(10, "2021")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
    CheckDefAndScriptFailure(['strptime("%Y", 2021)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
    assert_true(strptime('%Y', '2021') != 0)
    assert_true(strptime('%Y', '') == 0)
  endif
enddef

def Test_strridx()
  CheckDefAndScriptFailure(['strridx([1], "b")'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['strridx("a", {})'], ['E1013: Argument 2: type mismatch, expected string but got dict<unknown>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['strridx("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  strridx('', '')->assert_equal(0)
  strridx('', 'a')->assert_equal(-1)
  strridx('a', '')->assert_equal(1)
enddef

def Test_strtrans()
  CheckDefAndScriptFailure(['strtrans(20)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal('abc', strtrans('abc'))
  strtrans('')->assert_equal('')
enddef

def Test_strwidth()
  CheckDefAndScriptFailure(['strwidth(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal(4, strwidth('abcd'))
  strwidth('')->assert_equal(0)
enddef

def Test_submatch()
  var pat = 'A\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)'
  var Rep = () => range(10)->mapnew((_, v) => submatch(v, true))->string()
  var actual = substitute('A123456789', pat, Rep, '')
  var expected = "[['A123456789'], ['1'], ['2'], ['3'], ['4'], ['5'], ['6'], ['7'], ['8'], ['9']]"
  actual->assert_equal(expected)
  CheckDefAndScriptFailure(['submatch("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['submatch(1, "a")'], ['E1013: Argument 2: type mismatch, expected bool but got string', 'E1212: Bool required for argument 2'])
enddef

def Test_substitute()
  var res = substitute('A1234', '\d', 'X', '')
  assert_equal('AX234', res)

  if has('job')
    assert_fails('"text"->substitute(".*", () => test_null_job(), "")', 'E908: using an invalid value as a String: job')
    assert_fails('"text"->substitute(".*", () => test_null_channel(), "")', 'E908: using an invalid value as a String: channel')
  endif
  CheckDefAndScriptFailure(['substitute(1, "b", "1", "d")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['substitute("a", 2, "1", "d")'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['substitute("a", "b", "1", 4)'], ['E1013: Argument 4: type mismatch, expected string but got number', 'E1174: String required for argument 4'])
  substitute('', '', '', '')->assert_equal('')
enddef

def Test_swapinfo()
  CheckDefAndScriptFailure(['swapinfo({})'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', 'E1174: String required for argument 1'])
  call swapinfo('x')->assert_equal({error: 'Cannot open file'})
  call swapinfo('')->assert_equal({error: 'Cannot open file'})
enddef

def Test_swapname()
  CheckDefAndScriptFailure(['swapname([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  assert_fails('swapname("NonExistingBuf")', 'E94:')
enddef

def Test_synID()
  new
  setline(1, "text")
  synID(1, 1, true)->assert_equal(0)
  bwipe!
  CheckDefAndScriptFailure(['synID(0z10, 1, true)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['synID("a", true, false)'], ['E1013: Argument 2: type mismatch, expected number but got bool', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['synID(1, 1, 2)'], ['E1013: Argument 3: type mismatch, expected bool but got number', 'E1212: Bool required for argument 3'])
  CheckDefExecAndScriptFailure(['synID("", 10, true)'], 'E1209: Invalid value for a line number')
enddef

def Test_synIDattr()
  CheckDefAndScriptFailure(['synIDattr("a", "b")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['synIDattr(1, 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['synIDattr(1, "b", 3)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  synIDattr(1, '', '')->assert_equal('')
enddef

def Test_synIDtrans()
  CheckDefAndScriptFailure(['synIDtrans("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_synconcealed()
  CheckDefAndScriptFailure(['synconcealed(0z10, 1)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['synconcealed(1, "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  if has('conceal')
    CheckDefExecAndScriptFailure(['synconcealed("", 4)'], 'E1209: Invalid value for a line number')
  endif
enddef

def Test_synstack()
  CheckDefAndScriptFailure(['synstack(0z10, 1)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['synstack(1, "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefExecAndScriptFailure(['synstack("", 4)'], 'E1209: Invalid value for a line number')
enddef

def Test_system()
  CheckDefAndScriptFailure(['system(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['system("a", {})'], ['E1013: Argument 2: type mismatch, expected string but got dict<unknown>', 'E1224: String, Number or List required for argument 2'])
  assert_equal("123\n", system('echo 123'))
enddef

def Test_systemlist()
  CheckDefAndScriptFailure(['systemlist(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['systemlist("a", {})'], ['E1013: Argument 2: type mismatch, expected string but got dict<unknown>', 'E1224: String, Number or List required for argument 2'])
  if has('win32')
    call assert_equal(["123\r"], systemlist('echo 123'))
  else
    call assert_equal(['123'], systemlist('echo 123'))
  endif
enddef

def Test_tabpagebuflist()
  CheckDefAndScriptFailure(['tabpagebuflist("t")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  assert_equal([bufnr('')], tabpagebuflist())
  assert_equal([bufnr('')], tabpagebuflist(1))
enddef

def Test_tabpagenr()
  CheckDefAndScriptFailure(['tabpagenr(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefExecAndScriptFailure(['tabpagenr("")'], 'E15: Invalid expression')
  assert_equal(1, tabpagenr('$'))
  assert_equal(1, tabpagenr())
enddef

def Test_tabpagewinnr()
  CheckDefAndScriptFailure(['tabpagewinnr("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['tabpagewinnr(1, 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefExecAndScriptFailure(['tabpagewinnr(1, "")'], 'E15: Invalid expression')
enddef

def Test_taglist()
  CheckDefAndScriptFailure(['taglist([1])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['taglist("a", [2])'], ['E1013: Argument 2: type mismatch, expected string but got list<number>', 'E1174: String required for argument 2'])
  taglist('')->assert_equal(0)
  taglist('', '')->assert_equal(0)
enddef

def Test_term_dumpload()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_dumpload({"a": 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['term_dumpload({"a": 10}, "b")'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['term_dumpload("a", ["b"])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<string>', 'E1206: Dictionary required for argument 2'])
  CheckDefExecAndScriptFailure(['term_dumpload("")'], 'E485: Can''t read file')
enddef

def Test_term_dumpdiff()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_dumpdiff(1, "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['term_dumpdiff("a", 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['term_dumpdiff("a", "b", [1])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])
  CheckDefExecAndScriptFailure(['term_dumpdiff("", "")'], 'E485: Can''t read file')
enddef

def Test_term_dumpwrite()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_dumpwrite(true, "b")'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_dumpwrite(1, 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['term_dumpwrite("a", "b", [1])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_term_getaltscreen()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_getaltscreen(true)'], ['E1013: Argument 1: type mismatch, expected string but got bool', 'E1220: String or Number required for argument 1'])
enddef

def Test_term_getansicolors()
  CheckRunVimInTerminal
  CheckFeature termguicolors
  CheckDefAndScriptFailure(['term_getansicolors(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1220: String or Number required for argument 1'])
enddef

def Test_term_getattr()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_getattr("x", "a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['term_getattr(1, 2)'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
enddef

def Test_term_getcursor()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_getcursor({"a": 10})'], ['E1013: Argument 1: type mismatch, expected string but got dict<number>', 'E1220: String or Number required for argument 1'])
enddef

def Test_term_getjob()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_getjob(0z10)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 1'])
enddef

def Test_term_getline()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_getline(1.1, 1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_getline(1, 1.1)'], ['E1013: Argument 2: type mismatch, expected string but got float', 'E1220: String or Number required for argument 2'])
enddef

def Test_term_getscrolled()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_getscrolled(1.1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1220: String or Number required for argument 1'])
enddef

def Test_term_getsize()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_getsize(1.1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1220: String or Number required for argument 1'])
enddef

def Test_term_getstatus()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_getstatus(1.1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1220: String or Number required for argument 1'])
enddef

def Test_term_gettitle()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_gettitle(1.1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1220: String or Number required for argument 1'])
enddef

def Test_term_gettty()
  if !has('terminal')
    CheckFeature terminal
  else
    var buf = Run_shell_in_terminal({})
    term_gettty(buf, true)->assert_notequal('')
    StopShellInTerminal(buf)
  endif
  CheckDefAndScriptFailure(['term_gettty([1])'], ['E1013: Argument 1: type mismatch, expected string but got list<number>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_gettty(1, 2)'], ['E1013: Argument 2: type mismatch, expected bool but got number', 'E1212: Bool required for argument 2'])
enddef

def Test_term_scrape()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_scrape(1.1, 1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_scrape(1, 1.1)'], ['E1013: Argument 2: type mismatch, expected string but got float', 'E1220: String or Number required for argument 2'])
enddef

def Test_term_sendkeys()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_sendkeys([], "p")'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_sendkeys(1, [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 2'])
enddef

def Test_term_setansicolors()
  CheckRunVimInTerminal

  if has('termguicolors') || has('gui')
    CheckDefAndScriptFailure(['term_setansicolors([], "p")'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
    CheckDefAndScriptFailure(['term_setansicolors(10, {})'], ['E1013: Argument 2: type mismatch, expected list<any> but got dict<unknown>', 'E1211: List required for argument 2'])
  else
    throw 'Skipped: Only works with termguicolors or gui feature'
  endif
enddef

def Test_term_setapi()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_setapi([], "p")'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_setapi(1, [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 2'])
enddef

def Test_term_setkill()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_setkill([], "p")'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_setkill(1, [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 2'])
enddef

def Test_term_setrestore()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_setrestore([], "p")'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_setrestore(1, [])'], ['E1013: Argument 2: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 2'])
enddef

def Test_term_setsize()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_setsize(1.1, 2, 3)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_setsize(1, "2", 3)'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['term_setsize(1, 2, "3")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
enddef

def Test_term_start()
  if !has('terminal')
    CheckFeature terminal
  else
    botright new
    var winnr = winnr()
    term_start(&shell, {curwin: true})
    winnr()->assert_equal(winnr)
    bwipe!
  endif
  CheckDefAndScriptFailure(['term_start({})'], ['E1013: Argument 1: type mismatch, expected string but got dict<unknown>', 'E1222: String or List required for argument 1'])
  CheckDefAndScriptFailure(['term_start([], [])'], ['E1013: Argument 2: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 2'])
  CheckDefAndScriptFailure(['term_start("", "")'], ['E1013: Argument 2: type mismatch, expected dict<any> but got string', 'E1206: Dictionary required for argument 2'])
  CheckDefExecAndScriptFailure(['term_start("")'], 'E474: Invalid argument')
enddef

def Test_term_wait()
  CheckRunVimInTerminal
  CheckDefAndScriptFailure(['term_wait(0z10, 1)'], ['E1013: Argument 1: type mismatch, expected string but got blob', 'E1220: String or Number required for argument 1'])
  CheckDefAndScriptFailure(['term_wait(1, "a")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_test_alloc_fail()
  CheckDefAndScriptFailure(['test_alloc_fail("a", 10, 20)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['test_alloc_fail(10, "b", 20)'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['test_alloc_fail(10, 20, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
enddef

def Test_test_feedinput()
  CheckDefAndScriptFailure(['test_feedinput(test_void())'], ['E1013: Argument 1: type mismatch, expected string but got void', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['test_feedinput(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
enddef

def Test_test_getvalue()
  CheckDefAndScriptFailure(['test_getvalue(1.1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1174: String required for argument 1'])
enddef

def Test_test_gui_drop_files()
  CheckGui
  CheckDefAndScriptFailure(['test_gui_drop_files("a", 1, 1, 0)'], ['E1013: Argument 1: type mismatch, expected list<string> but got string', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['test_gui_drop_files(["x"], "", 1, 0)'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['test_gui_drop_files(["x"], 1, "", 0)'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['test_gui_drop_files(["x"], 1, 1, "")'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
enddef

def Test_test_gui_mouse_event()
  CheckGui
  CheckDefAndScriptFailure(['test_gui_mouse_event(1.1, 1, 1, 1, 1)'], ['E1013: Argument 1: type mismatch, expected number but got float', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['test_gui_mouse_event(1, "1", 1, 1, 1)'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['test_gui_mouse_event(1, 1, "1", 1, 1)'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  CheckDefAndScriptFailure(['test_gui_mouse_event(1, 1, 1, "1", 1)'], ['E1013: Argument 4: type mismatch, expected number but got string', 'E1210: Number required for argument 4'])
  CheckDefAndScriptFailure(['test_gui_mouse_event(1, 1, 1, 1, "1")'], ['E1013: Argument 5: type mismatch, expected number but got string', 'E1210: Number required for argument 5'])
enddef

def Test_test_ignore_error()
  CheckDefAndScriptFailure(['test_ignore_error([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 1'])
  test_ignore_error('RESET')
enddef

def Test_test_option_not_set()
  CheckDefAndScriptFailure(['test_option_not_set([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 1'])
enddef

def Test_test_override()
  CheckDefAndScriptFailure(['test_override(1, 1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['test_override("a", "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_test_scrollbar()
  CheckGui
  CheckDefAndScriptFailure(['test_scrollbar(1, 2, 3)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['test_scrollbar("a", "b", 3)'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['test_scrollbar("a", 2, "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
enddef

def Test_test_setmouse()
  CheckDefAndScriptFailure(['test_setmouse("a", 10)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['test_setmouse(10, "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_test_settime()
  CheckDefAndScriptFailure(['test_settime([1])'], ['E1013: Argument 1: type mismatch, expected number but got list<number>', 'E1210: Number required for argument 1'])
enddef

def Test_test_srand_seed()
  CheckDefAndScriptFailure(['test_srand_seed([1])'], ['E1013: Argument 1: type mismatch, expected number but got list<number>', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['test_srand_seed("10")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_timer_info()
  CheckDefAndScriptFailure(['timer_info("id")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  assert_equal([], timer_info(100))
  assert_equal([], timer_info())
enddef

def Test_timer_pause()
  CheckDefAndScriptFailure(['timer_pause("x", 1)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['timer_pause(1, "a")'], ['E1013: Argument 2: type mismatch, expected bool but got string', 'E1212: Bool required for argument 2'])
enddef

def Test_timer_paused()
  var id = timer_start(50, () => 0)
  timer_pause(id, true)
  var info = timer_info(id)
  info[0]['paused']->assert_equal(1)
  timer_stop(id)
enddef

def Test_timer_start()
  CheckDefAndScriptFailure(['timer_start("a", "1")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['timer_start(1, "1", [1])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_timer_stop()
  CheckDefAndScriptFailure(['timer_stop("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  assert_equal(0, timer_stop(100))
enddef

def Test_tolower()
  CheckDefAndScriptFailure(['tolower(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  tolower('')->assert_equal('')
enddef

def Test_toupper()
  CheckDefAndScriptFailure(['toupper(1)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  toupper('')->assert_equal('')
enddef

def Test_tr()
  CheckDefAndScriptFailure(['tr(1, "a", "b")'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['tr("a", 1, "b")'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['tr("a", "a", 1)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
  tr('', '', '')->assert_equal('')
  tr('ab', '', '')->assert_equal('ab')
  assert_fails("tr('ab', 'ab', '')", 'E475:')
  assert_fails("tr('ab', '', 'AB')", 'E475:')
enddef

def Test_trim()
  CheckDefAndScriptFailure(['trim(["a"])'], ['E1013: Argument 1: type mismatch, expected string but got list<string>', 'E1174: String required for argument 1'])
  CheckDefAndScriptFailure(['trim("a", ["b"])'], ['E1013: Argument 2: type mismatch, expected string but got list<string>', 'E1174: String required for argument 2'])
  CheckDefAndScriptFailure(['trim("a", "b", "c")'], ['E1013: Argument 3: type mismatch, expected number but got string', 'E1210: Number required for argument 3'])
  trim('')->assert_equal('')
  trim('', '')->assert_equal('')
enddef

def Test_typename()
  if has('float')
    assert_equal('func([unknown], [unknown]): float', typename(function('pow')))
  endif
  assert_equal('func', test_null_partial()->typename())
  assert_equal('list<unknown>', test_null_list()->typename())
  assert_equal('dict<unknown>', test_null_dict()->typename())
  if has('job')
    assert_equal('job', test_null_job()->typename())
  endif
  if has('channel')
    assert_equal('channel', test_null_channel()->typename())
  endif
enddef

def Test_undofile()
  CheckDefAndScriptFailure(['undofile(10)'], ['E1013: Argument 1: type mismatch, expected string but got number', 'E1174: String required for argument 1'])
  assert_equal('.abc.un~', fnamemodify(undofile('abc'), ':t'))
  undofile('')->assert_equal('')
enddef

def Test_uniq()
  CheckDefAndScriptFailure(['uniq("a")'], ['E1013: Argument 1: type mismatch, expected list<any> but got string', 'E1211: List required for argument 1'])
  CheckDefAndScriptFailure(['uniq([1], "", [1])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<number>', 'E1206: Dictionary required for argument 3'])

  CheckDefFailure(['var l: list<number> = uniq(["a", "b"])'], 'E1012: Type mismatch; expected list<number> but got list<string>')
enddef

def Test_values()
  CheckDefAndScriptFailure(['values([])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 1'])
  assert_equal([], {}->values())
  assert_equal(['sun'], {star: 'sun'}->values())
enddef

def Test_virtcol()
  CheckDefAndScriptFailure(['virtcol(1.1)'], ['E1013: Argument 1: type mismatch, expected string but got float', 'E1222: String or List required for argument 1'])
  CheckDefExecAndScriptFailure(['virtcol("")'], 'E1209: Invalid value for a line number')
  new
  setline(1, ['abcdefgh'])
  cursor(1, 4)
  assert_equal(4, virtcol('.'))
  assert_equal(4, virtcol([1, 4]))
  assert_equal(9, virtcol([1, '$']))
  assert_equal(0, virtcol([10, '$']))
  bw!
enddef

def Test_visualmode()
  CheckDefAndScriptFailure(['visualmode("1")'], ['E1013: Argument 1: type mismatch, expected bool but got string', 'E1212: Bool required for argument 1'])
  CheckDefAndScriptFailure(['visualmode(2)'], ['E1013: Argument 1: type mismatch, expected bool but got number', 'E1212: Bool required for argument 1'])
enddef

def Test_win_execute()
  assert_equal("\n" .. winnr(), win_execute(win_getid(), 'echo winnr()'))
  assert_equal("\n" .. winnr(), 'echo winnr()'->win_execute(win_getid()))
  assert_equal("\n" .. winnr(), win_execute(win_getid(), 'echo winnr()', 'silent'))
  assert_equal('', win_execute(342343, 'echo winnr()'))
  CheckDefAndScriptFailure(['win_execute("a", "b", "c")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['win_execute(1, 2, "c")'], ['E1013: Argument 2: type mismatch, expected string but got number', 'E1222: String or List required for argument 2'])
  CheckDefAndScriptFailure(['win_execute(1, "b", 3)'], ['E1013: Argument 3: type mismatch, expected string but got number', 'E1174: String required for argument 3'])
enddef

def Test_win_findbuf()
  CheckDefAndScriptFailure(['win_findbuf("a")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  assert_equal([], win_findbuf(1000))
  assert_equal([win_getid()], win_findbuf(bufnr('')))
enddef

def Test_win_getid()
  CheckDefAndScriptFailure(['win_getid(".")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['win_getid(1, ".")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  assert_equal(win_getid(), win_getid(1, 1))
enddef

def Test_win_gettype()
  CheckDefAndScriptFailure(['win_gettype("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_win_gotoid()
  CheckDefAndScriptFailure(['win_gotoid("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_win_id2tabwin()
  CheckDefAndScriptFailure(['win_id2tabwin("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_win_id2win()
  CheckDefAndScriptFailure(['win_id2win("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_win_screenpos()
  CheckDefAndScriptFailure(['win_screenpos("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_win_splitmove()
  split
  win_splitmove(1, 2, {vertical: true, rightbelow: true})
  close
  CheckDefAndScriptFailure(['win_splitmove("a", 2)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['win_splitmove(1, "b")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
  CheckDefAndScriptFailure(['win_splitmove(1, 2, [])'], ['E1013: Argument 3: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 3'])
enddef

def Test_winbufnr()
  CheckDefAndScriptFailure(['winbufnr("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_winheight()
  CheckDefAndScriptFailure(['winheight("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_winlayout()
  CheckDefAndScriptFailure(['winlayout("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_winnr()
  CheckDefAndScriptFailure(['winnr([])'], ['E1013: Argument 1: type mismatch, expected string but got list<unknown>', 'E1174: String required for argument 1'])
  CheckDefExecAndScriptFailure(['winnr("")'], 'E15: Invalid expression')
  assert_equal(1, winnr())
  assert_equal(1, winnr('$'))
enddef

def Test_winrestcmd()
  split
  var cmd = winrestcmd()
  wincmd _
  exe cmd
  assert_equal(cmd, winrestcmd())
  close
enddef

def Test_winrestview()
  CheckDefAndScriptFailure(['winrestview([])'], ['E1013: Argument 1: type mismatch, expected dict<any> but got list<unknown>', 'E1206: Dictionary required for argument 1'])
  :%d _
  setline(1, 'Hello World')
  winrestview({lnum: 1, col: 6})
  assert_equal([1, 7], [line('.'), col('.')])
enddef

def Test_winsaveview()
  var view: dict<number> = winsaveview()

  var lines =<< trim END
      var view: list<number> = winsaveview()
  END
  CheckDefAndScriptFailure(lines, 'E1012: Type mismatch; expected list<number> but got dict<number>', 1)
enddef

def Test_winwidth()
  CheckDefAndScriptFailure(['winwidth("x")'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
enddef

def Test_xor()
  CheckDefAndScriptFailure(['xor("x", 0x2)'], ['E1013: Argument 1: type mismatch, expected number but got string', 'E1210: Number required for argument 1'])
  CheckDefAndScriptFailure(['xor(0x1, "x")'], ['E1013: Argument 2: type mismatch, expected number but got string', 'E1210: Number required for argument 2'])
enddef

def Test_writefile()
  CheckDefExecAndScriptFailure(['writefile(["a"], "")'], 'E482: Can''t create file <empty>')
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
