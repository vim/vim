scriptencoding utf-8

function Test_environ()
  unlet! $TESTENV
  call assert_equal(0, has_key(environ(), 'TESTENV'))
  let $TESTENV = 'foo'
  call assert_equal(1, has_key(environ(), 'TESTENV'))
  let $TESTENV = 'こんにちわ'
  call assert_equal('こんにちわ', environ()['TESTENV'])
endfunc

function Test_getenv()
  unlet! $TESTENV
  call assert_equal(v:null, getenv('TESTENV'))
  let $TESTENV = 'foo'
  call assert_equal('foo', getenv('TESTENV'))
endfunc

function Test_setenv()
  unlet! $TESTENV
  call setenv('TEST ENV', 'foo')
  call assert_equal('foo', getenv('TEST ENV'))
  call setenv('TEST ENV', v:null)
  call assert_equal(v:null, getenv('TEST ENV'))
endfunc

function Test_external_env()
  call setenv('FOO', 'HelloWorld')
  if has('win32')
    let result = system('echo %FOO%')
  else
    let result = system('echo $FOO')
  endif
  let result = substitute(result, '[ \r\n]', '', 'g')
  let result = iconv(result, 'char', &encoding)
  call assert_equal('HelloWorld', result)

  call setenv('FOO', v:null)
  if has('win32')
    let result = system('set | grep ^FOO=')
  else
    let result = system('env | grep ^FOO=')
  endif
  call assert_equal('', result)
endfunc
