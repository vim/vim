" Tests for the exists() function

import './util/vim9.vim' as v9

func Test_exists()
  augroup myagroup
      autocmd! BufEnter       *.my     echo "myfile edited"
      autocmd! FuncUndefined  UndefFun exec "fu UndefFun()\nendfu"
  augroup END
  set rtp+=./sautest

  " valid autocmd group
  call assert_equal(1, exists('#myagroup'))
  " valid autocmd group with garbage
  call assert_equal(0, exists('#myagroup+b'))
  " Valid autocmd group and event
  call assert_equal(1, exists('#myagroup#BufEnter'))
  " Valid autocmd group, event and pattern
  call assert_equal(1, exists('#myagroup#BufEnter#*.my'))
  " Valid autocmd event
  call assert_equal(1, exists('#BufEnter'))
  " Valid autocmd event and pattern
  call assert_equal(1, exists('#BufEnter#*.my'))
  " Non-existing autocmd group or event
  call assert_equal(0, exists('#xyzagroup'))
  " Non-existing autocmd group and valid autocmd event
  call assert_equal(0, exists('#xyzagroup#BufEnter'))
  " Valid autocmd group and event with no matching pattern
  call assert_equal(0, exists('#myagroup#CmdwinEnter'))
  " Valid autocmd group and non-existing autocmd event
  call assert_equal(0, exists('#myagroup#xyzacmd'))
  " Valid autocmd group and event and non-matching pattern
  call assert_equal(0, exists('#myagroup#BufEnter#xyzpat'))
  " Valid autocmd event and non-matching pattern
  call assert_equal(0, exists('#BufEnter#xyzpat'))
  " Empty autocmd group, event and pattern
  call assert_equal(0, exists('###'))
  " Empty autocmd group and event or empty event and pattern
  call assert_equal(0, exists('##'))
  " Valid autocmd event
  call assert_equal(1, exists('##FileReadCmd'))
  " Non-existing autocmd event
  call assert_equal(0, exists('##MySpecialCmd'))

  " Existing and working option (long form)
  call assert_equal(1, exists('&textwidth'))
  " Existing and working option (short form)
  call assert_equal(1, exists('&tw'))
  " Existing and working option with garbage
  call assert_equal(0, exists('&tw-'))
  " Global option
  call assert_equal(1, exists('&g:errorformat'))
  " Local option
  call assert_equal(1, exists('&l:errorformat'))
  " Negative form of existing and working option (long form)
  call assert_equal(0, exists('&nojoinspaces'))
  " Negative form of existing and working option (short form)
  call assert_equal(0, exists('&nojs'))
  " Non-existing option
  call assert_equal(0, exists('&myxyzoption'))

  " Existing and working option (long form)
  call assert_equal(1, exists('+incsearch'))
  " Existing and working option with garbage
  call assert_equal(0, exists('+incsearch!1'))
  " Existing and working option (short form)
  call assert_equal(1, exists('+is'))
  " Existing option that is hidden.
  call assert_equal(0, exists('+autoprint'))

  " Existing environment variable
  let $EDITOR_NAME = 'Vim Editor'
  call assert_equal(1, exists('$EDITOR_NAME'))
  if has('unix')
    " ${name} environment variables are supported only on Unix-like systems
    call assert_equal(1, exists('${VIM}'))
  endif
  " Non-existing environment variable
  call assert_equal(0, exists('$NON_ENV_VAR'))

  " Valid internal function
  call assert_equal(1, exists('*bufnr'))
  " Valid internal function with ()
  call assert_equal(1, exists('*bufnr()'))
  " Non-existing internal function
  call assert_equal(0, exists('*myxyzfunc'))
  " Valid internal function with garbage
  call assert_equal(0, exists('*bufnr&6'))
  " Valid user defined function
  call assert_equal(1, exists('*Test_exists'))
  " Non-existing user defined function
  call assert_equal(0, exists('*MyxyzFunc'))
  " Function that may be created by FuncUndefined event
  call assert_equal(0, exists('*UndefFun'))
  " Function that may be created by script autoloading
  call assert_equal(0, exists('*footest#F'))

  call assert_equal(has('float'), exists('*acos'))
  call assert_equal(1, exists('?acos'))
  call assert_equal(has('win32'), exists('*debugbreak'))
  call assert_equal(1, exists('?debugbreak'))

  " Valid internal command (full match)
  call assert_equal(2, exists(':edit'))
  " Valid internal command (full match) with garbage
  call assert_equal(0, exists(':edit/a'))
  " Valid internal command (partial match)
  call assert_equal(1, exists(':q'))
  " Valid internal command with a digit
  call assert_equal(2, exists(':2match'))
  " Non-existing internal command
  call assert_equal(0, exists(':invalidcmd'))
  " Internal command with a count
  call assert_equal(0, exists(':3buffer'))

  " Valid internal command (full match)
  call assert_equal(2, exists(':k'))
  " Non-existing internal command (':k' with arg 'e')
  call assert_equal(0, exists(':ke'))
  " Valid internal command (partial match)
  call assert_equal(1, exists(':kee'))

  " User defined command (full match)
  command! MyCmd :echo 'My command'
  call assert_equal(2, exists(':MyCmd'))
  " User defined command (partial match)
  command! MyOtherCmd :echo 'Another command'
  call assert_equal(3, exists(':My'))

  " Command modifier
  call assert_equal(2, exists(':rightbelow'))

  " Non-existing user defined command (full match)
  delcommand MyCmd
  call assert_equal(0, exists(':MyCmd'))

  " Non-existing user defined command (partial match)
  delcommand MyOtherCmd
  call assert_equal(0, exists(':My'))

  " Valid local variable
  let local_var = 1
  call assert_equal(1, exists('local_var'))
  " Valid local variable with garbage
  call assert_equal(0, exists('local_var%n'))
  " Non-existing local variable
  unlet local_var
  call assert_equal(0, exists('local_var'))

  " Non-existing autoload variable that may be autoloaded
  call assert_equal(0, exists('footest#x'))

  " Valid local list
  let local_list = ["blue", "orange"]
  call assert_equal(1, exists('local_list'))
  " Valid local list item
  call assert_equal(1, exists('local_list[1]'))
  " Valid local list item with garbage
  call assert_equal(0, exists('local_list[1]+5'))
  " Invalid local list item
  call assert_equal(0, exists('local_list[2]'))
  " Non-existing local list
  unlet local_list
  call assert_equal(0, exists('local_list'))
  " Valid local dictionary
  let local_dict = {"xcord":100, "ycord":2}
  call assert_equal(1, exists('local_dict'))
  " Non-existing local dictionary
  unlet local_dict
  call assert_equal(0, exists('local_dict'))
  " Existing local curly-brace variable
  let str = "local"
  let curly_{str}_var = 1
  call assert_equal(1, exists('curly_{str}_var'))
  " Non-existing local curly-brace variable
  unlet curly_{str}_var
  call assert_equal(0, exists('curly_{str}_var'))

  " Existing global variable
  let g:global_var = 1
  call assert_equal(1, exists('g:global_var'))
  " Existing global variable with garbage
  call assert_equal(0, exists('g:global_var-n'))
  " Non-existing global variable
  unlet g:global_var
  call assert_equal(0, exists('g:global_var'))
  " Existing global list
  let g:global_list = ["blue", "orange"]
  call assert_equal(1, exists('g:global_list'))
  " Non-existing global list
  unlet g:global_list
  call assert_equal(0, exists('g:global_list'))
  " Existing global dictionary
  let g:global_dict = {"xcord":100, "ycord":2}
  call assert_equal(1, exists('g:global_dict'))
  " Non-existing global dictionary
  unlet g:global_dict
  call assert_equal(0, exists('g:global_dict'))
  " Existing global curly-brace variable
  let str = "global"
  let g:curly_{str}_var = 1
  call assert_equal(1, exists('g:curly_{str}_var'))
  " Non-existing global curly-brace variable
  unlet g:curly_{str}_var
  call assert_equal(0, exists('g:curly_{str}_var'))

  " Existing window variable
  let w:window_var = 1
  call assert_equal(1, exists('w:window_var'))
  " Non-existing window variable
  unlet w:window_var
  call assert_equal(0, exists('w:window_var'))
  " Existing window list
  let w:window_list = ["blue", "orange"]
  call assert_equal(1, exists('w:window_list'))
  " Non-existing window list
  unlet w:window_list
  call assert_equal(0, exists('w:window_list'))
  " Existing window dictionary
  let w:window_dict = {"xcord":100, "ycord":2}
  call assert_equal(1, exists('w:window_dict'))
  " Non-existing window dictionary
  unlet w:window_dict
  call assert_equal(0, exists('w:window_dict'))
  " Existing window curly-brace variable
  let str = "window"
  let w:curly_{str}_var = 1
  call assert_equal(1, exists('w:curly_{str}_var'))
  " Non-existing window curly-brace variable
  unlet w:curly_{str}_var
  call assert_equal(0, exists('w:curly_{str}_var'))

  " Existing tab variable
  let t:tab_var = 1
  call assert_equal(1, exists('t:tab_var'))
  " Non-existing tab variable
  unlet t:tab_var
  call assert_equal(0, exists('t:tab_var'))
  " Existing tab list
  let t:tab_list = ["blue", "orange"]
  call assert_equal(1, exists('t:tab_list'))
  " Non-existing tab list
  unlet t:tab_list
  call assert_equal(0, exists('t:tab_list'))
  " Existing tab dictionary
  let t:tab_dict = {"xcord":100, "ycord":2}
  call assert_equal(1, exists('t:tab_dict'))
  " Non-existing tab dictionary
  unlet t:tab_dict
  call assert_equal(0, exists('t:tab_dict'))
  " Existing tab curly-brace variable
  let str = "tab"
  let t:curly_{str}_var = 1
  call assert_equal(1, exists('t:curly_{str}_var'))
  " Non-existing tab curly-brace variable
  unlet t:curly_{str}_var
  call assert_equal(0, exists('t:curly_{str}_var'))

  " Existing buffer variable
  let b:buffer_var = 1
  call assert_equal(1, exists('b:buffer_var'))
  " Non-existing buffer variable
  unlet b:buffer_var
  call assert_equal(0, exists('b:buffer_var'))
  " Existing buffer list
  let b:buffer_list = ["blue", "orange"]
  call assert_equal(1, exists('b:buffer_list'))
  " Non-existing buffer list
  unlet b:buffer_list
  call assert_equal(0, exists('b:buffer_list'))
  " Existing buffer dictionary
  let b:buffer_dict = {"xcord":100, "ycord":2}
  call assert_equal(1, exists('b:buffer_dict'))
  " Non-existing buffer dictionary
  unlet b:buffer_dict
  call assert_equal(0, exists('b:buffer_dict'))
  " Existing buffer curly-brace variable
  let str = "buffer"
  let b:curly_{str}_var = 1
  call assert_equal(1, exists('b:curly_{str}_var'))
  " Non-existing buffer curly-brace variable
  unlet b:curly_{str}_var
  call assert_equal(0, exists('b:curly_{str}_var'))

  " Existing Vim internal variable
  call assert_equal(1, exists('v:version'))
  " Non-existing Vim internal variable
  call assert_equal(0, exists('v:non_exists_var'))

  " Existing script-local variable
  let s:script_var = 1
  call assert_equal(1, exists('s:script_var'))
  " Non-existing script-local variable
  unlet s:script_var
  call assert_equal(0, exists('s:script_var'))
  " Existing script-local list
  let s:script_list = ["blue", "orange"]
  call assert_equal(1, exists('s:script_list'))
  " Non-existing script-local list
  unlet s:script_list
  call assert_equal(0, exists('s:script_list'))
  " Existing script-local dictionary
  let s:script_dict = {"xcord":100, "ycord":2}
  call assert_equal(1, exists('s:script_dict'))
  " Non-existing script-local dictionary
  unlet s:script_dict
  call assert_equal(0, exists('s:script_dict'))
  " Existing script curly-brace variable
  let str = "script"
  let s:curly_{str}_var = 1
  call assert_equal(1, exists('s:curly_{str}_var'))
  " Non-existing script-local curly-brace variable
  unlet s:curly_{str}_var
  call assert_equal(0, exists('s:curly_{str}_var'))

  " Existing script-local function
  function! s:my_script_func()
  endfunction

  echo '*s:my_script_func: 1'
  call assert_equal(1, exists('*s:my_script_func'))

  " Non-existing script-local function
  delfunction s:my_script_func

  call assert_equal(0, exists('*s:my_script_func'))
  unlet str

  call assert_equal(1, g:footest#x)
  call assert_equal(0, footest#F())
  call assert_equal(0, UndefFun())
endfunc

" exists() test for Function arguments
func FuncArg_Tests(func_arg, ...)
  call assert_equal(1, exists('a:func_arg'))
  call assert_equal(0, exists('a:non_exists_arg'))
  call assert_equal(1, exists('a:1'))
  call assert_equal(0, exists('a:2'))
endfunc

func Test_exists_funcarg()
  call FuncArg_Tests("arg1", "arg2")
endfunc

func Test_exists_info()
  call assert_equal({'name': 'strlen', 'kind': 'builtin', 'minargs': 1,
        \ 'maxargs': 1, 'method': 1,
        \ 'args': [{'types': ['string', 'number']}], 'returns': 'number'},
        \ exists_info('*strlen'))

  " No arguments, no argument checks and not usable as a method.
  call assert_equal({'name': 'argidx', 'kind': 'builtin', 'minargs': 0,
        \ 'maxargs': 0, 'method': 0, 'args': [], 'returns': 'number'},
        \ exists_info('*argidx'))

  " An argument that accepts several types, and one that is checked against
  " another argument.
  let info = exists_info('*get')
  call assert_equal([2, 3, 1], [info.minargs, info.maxargs, info.method])
  call assert_equal(['blob', 'list<any>', 'tuple<any>', 'dict<any>', 'func'],
        \ info.args[0].types)
  call assert_equal(['string', 'number'], info.args[1].types)
  call assert_equal(['any'], info.args[2].types)
  call assert_equal('any', info.returns)
  call assert_equal(['any'], exists_info('*extend').args[1].types)

  " The base of a method call is the second argument.
  call assert_equal(2, exists_info('*append').method)

  " No maximum number of arguments: the last item is for the rest.
  let info = exists_info('*instanceof')
  call assert_equal(-1, info.maxargs)
  call assert_equal([{'types': ['object<any>']}, {'types': ['class']}],
        \ info.args)

  " The type depends on the number of arguments, and no value at all.
  call assert_equal('any', exists_info('*getline').returns)
  call assert_equal('void', exists_info('*bufload').returns)

  " The type returned for arguments of the given types, or that number of
  " arguments.  Missing required arguments are "any".
  call assert_equal('list<number>',
        \ exists_info('*sort', ['list<number>']).returns)
  call assert_equal('list<string>',
        \ exists_info('*values', ['dict<string>']).returns)
  call assert_equal('number',
        \ exists_info('*remove', ['list<number>', 'number']).returns)
  call assert_equal('list<number>',
        \ exists_info('*remove', ['list<number>', 'number', 'number']).returns)
  call assert_equal('string', exists_info('*getline', ['number']).returns)
  call assert_equal('list<string>',
        \ exists_info('*getline', ['number', 'string']).returns)
  call assert_equal('any', exists_info('*sort', []).returns)
  call assert_equal('any', exists_info('*get', ['blob']).returns)
  call assert_equal('number', exists_info('*strlen', ['string']).returns)
  call assert_equal(exists_info('*strlen'), exists_info('*strlen', ['string']))
  call assert_equal({}, exists_info('*nosuchfunction', ['string']))
  call assert_fails("call exists_info('v:count', ['string'])", 'E118:')
  call assert_fails("call exists_info('+textwidth', ['string'])", 'E118:')
  call assert_fails("call exists_info('*strlen', ['string', 'string'])",
        \ 'E118:')
  call assert_fails("call exists_info('*strlen', ['nosuchtype'])", 'E1010:')
  call assert_fails("call exists_info('*strlen', ['string x'])", 'E1010:')
  call assert_fails("call exists_info('*strlen', 'string')", 'E1211:')

  " Not a builtin function, or not a function at all.
  call assert_equal({}, exists_info('*nosuchfunction'))
  call assert_equal({}, exists_info('*'))
  call assert_equal({}, exists_info(''))
  func s:NotBuiltin()
  endfunc
  call assert_equal({}, exists_info('*s:NotBuiltin'))
  delfunc s:NotBuiltin

  " "?funcname" also gives a builtin that is not implemented in this Vim.
  let info = exists_info('?strlen')
  call assert_equal(v:true, info.available)
  call assert_equal(exists_info('*strlen'), filter(info, 'v:key != "available"'))
  let info = exists_info('?mzeval')
  call assert_equal('mzeval', info.name)
  call assert_equal(exists('*mzeval') ? v:true : v:false, info.available)
  call assert_equal(exists('*mzeval') ? 'mzeval' : '',
        \ get(exists_info('*mzeval'), 'name', ''))
  call assert_equal({}, exists_info('?nosuchfunction'))

  " A predefined Vim variable: its declared type and how it can be used.
  call assert_equal({'name': 'v:count', 'type': 'number',
        \ 'readonly': v:true, 'compat': v:true}, exists_info('v:count'))
  let info = exists_info('v:lnum')
  call assert_equal([v:false, v:false], [info.readonly, info.compat])
  call assert_equal('list<string>', exists_info('v:errors').type)
  call assert_equal('bool', exists_info('v:true').type)
  call assert_equal('dict<any>', exists_info('v:event').type)
  call assert_equal({}, exists_info('v:nosuchvariable'))
  call assert_equal({}, exists_info('v:'))

  " An Ex command, also by an abbreviation, with the attributes of :command.
  call assert_equal({'name': 'substitute', 'kind': 'builtin', 'nargs': '*',
        \ 'range': '.', 'count': v:false, 'bang': v:false, 'bar': v:false,
        \ 'register': v:false, 'addr': 'lines'}, exists_info(':s'))
  let info = exists_info(':write')
  call assert_equal(['%', '?', v:true, v:true],
        \ [info.range, info.nargs, info.bang, info.bar])
  let info = exists_info(':delete')
  call assert_equal([0, v:true, 'none'],
        \ [info.count, info.register, exists_info(':echo').addr])
  let info = exists_info(':bo')
  call assert_equal(['botright', 'modifier'], [info.name, info.kind])
  let info = exists_info(':2match')
  call assert_equal(['match', 'other'], [info.name, info.addr])
  call assert_equal({}, exists_info(':3buffer'))
  call assert_equal({}, exists_info(':nosuchcommand'))
  call assert_equal({}, exists_info(':s garbage'))
  call assert_equal({}, exists_info(':'))

  command! -nargs=1 -range=% -bang -bar -register -complete=file MyCmd echo 1
  let info = exists_info(':MyCmd')
  let sid = str2nr(matchstr(expand('<SID>'), '\d\+'))
  call assert_equal({'name': 'MyCmd', 'kind': 'user', 'nargs': '1',
        \ 'range': '%', 'count': v:false, 'bang': v:true, 'bar': v:true,
        \ 'register': v:true, 'addr': 'lines', 'buffer': v:false,
        \ 'complete': 'file', 'definition': 'echo 1', 'sid': sid,
        \ 'lnum': info.lnum}, info)
  call assert_true(info.lnum > 0)
  command! -buffer -nargs=* -count=5 -addr=buffers MyBufCmd echo 2
  let info = exists_info(':MyBufCmd')
  call assert_equal(['*', 5, '.', 'buffers', v:true, ''],
        \ [info.nargs, info.count, info.range, info.addr, info.buffer,
        \ info.complete])
  command! -nargs=+ -complete=custom,MyCompl MyCustom echo 3
  call assert_equal('custom,MyCompl', exists_info(':MyCustom').complete)
  " An ambiguous abbreviation.
  call assert_equal({}, exists_info(':My'))
  delcommand MyCmd
  delcommand MyBufCmd
  delcommand MyCustom

  " An option, also by the short name; "&opt" also gives a hidden option.
  call assert_equal({'name': 'textwidth', 'shortname': 'tw', 'type': 'number',
        \ 'scope': 'buffer', 'default': 0}, exists_info('+tw'))
  let info = exists_info('+number')
  call assert_equal(['bool', 'window', v:false],
        \ [info.type, info.scope, info.default])
  call assert_equal('global-buffer', exists_info('+autoread').scope)
  call assert_equal('global-window', exists_info('+scrolloff').scope)
  let info = exists_info('+shortmess')
  call assert_equal(['string', 'global', &shortmess],
        \ [info.type, info.scope, info.default])
  call assert_equal('', exists_info('+debug').shortname)
  call assert_equal(exists_info('+tw'), exists_info('+l:textwidth'))
  call assert_equal({}, exists_info('+autoprint'))
  let info = exists_info('&autoprint')
  call assert_equal(['autoprint', v:false], [info.name, info.available])
  call assert_equal(v:true, exists_info('&tw').available)
  call assert_equal(exists_info('+tw'),
        \ filter(exists_info('&g:tw'), 'v:key != "available"'))
  call assert_equal({}, exists_info('+nonumber'))
  call assert_equal({}, exists_info('+tw-'))
  call assert_equal({}, exists_info('+'))

  " What is not supported yet.
  call assert_equal({}, exists_info('$HOME'))

  call assert_equal('number', '*strlen'->exists_info().returns)
  let lines =<< trim END
    assert_equal('string', exists_info('*printf').returns)
    assert_equal(-1, exists_info('*instanceof').maxargs)
    assert_equal('list<string>',
                 exists_info('*sort', ['list<string>']).returns)
    assert_equal('substitute', exists_info(':s').name)
    assert_equal('number', exists_info('+tw').type)
  END
  call v9.CheckDefAndScriptSuccess(lines)
  call v9.CheckDefAndScriptFailure(['exists_info(1)'], ['E1013:', 'E1174:'])
  call v9.CheckDefAndScriptFailure(['exists_info("*sort", "list<string>")'],
        \ ['E1013:', 'E1211:'])
endfunc

" Test for using exists() with class and object variables and methods.
func Test_exists_class_object()
  let lines =<< trim END
    vim9script
    class A
      var var1: number = 10
      static var var2: number = 10
      static def Foo()
      enddef
      def Bar()
      enddef
    endclass

    assert_equal(1, exists("A"))
    var a = A.new()
    assert_equal(1, exists("a"))

    assert_equal(1, exists("a.var1"))
    assert_fails('exists("a.var2")', 'E1375: Class variable "var2" accessible only using class "A"')
    assert_fails('exists("a.var3")', 'E1326: Variable "var3" not found in object "A"')
    assert_equal(1, exists("A.var2"))
    assert_fails('exists("A.var1")', 'E1376: Object variable "var1" accessible only using class "A" object')
    assert_fails('exists("A.var3")', 'E1337: Class variable "var3" not found in class "A"')

    assert_equal(1, exists("a.Bar"))
    assert_fails('exists("a.Barz")', 'E1326: Variable "Barz" not found in object "A"')
    assert_fails('exists("a.Foo")', 'E1326: Variable "Foo" not found in object "A"')
    assert_equal(1, exists("A.Foo"))
    assert_fails('exists("A.Bar")', 'E1337: Class variable "Bar" not found in class "A"')
    assert_fails('exists("A.Barz")', 'E1337: Class variable "Barz" not found in class "A"')

    def Baz()
      assert_equal(1, exists("A"))
      var aa = A.new()
      assert_equal(1, exists("A.var2"))
      assert_fails('exists("A.var1")', 'E1376: Object variable "var1" accessible only using class "A" object')
      assert_fails('exists("A.var3")', 'E1337: Class variable "var3" not found in class "A"')

      assert_equal(1, exists("A.Foo"))
      assert_fails('exists("A.Bar")', 'E1337: Class variable "Bar" not found in class "A"')
      assert_fails('exists("A.Barz")', 'E1337: Class variable "Barz" not found in class "A"')
    enddef
    Baz()
  END
  call v9.CheckSourceSuccess(lines)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
