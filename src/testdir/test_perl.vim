" Tests for Perl interface

if !has('perl')
  finish
end

set nocp viminfo+=nviminfo

fu <SID>catch_peval(expr)
  try
    call perleval(a:expr)
  catch
    return v:exception
  endtry
  call assert_true(0, 'no exception for `perleval("'.a:expr.'")`')
  return ''
endf

function Test_perleval()
  call assert_false(perleval('undef'))

  " scalar
  call assert_equal(0, perleval('0'))
  call assert_equal(2, perleval('2'))
  call assert_equal(-2, perleval('-2'))
  if has('float')
    call assert_equal(2.5, perleval('2.5'))
  else
    call assert_equal(2, perleval('2.5'))
  end

  sandbox call assert_equal(2, perleval('2'))

  call assert_equal('abc', perleval('"abc"'))
  call assert_equal("abc\ndef", perleval('"abc\0def"'))

  " ref
  call assert_equal([], perleval('[]'))
  call assert_equal(['word', 42, [42],{}], perleval('["word", 42, [42], {}]'))

  call assert_equal({}, perleval('{}'))
  call assert_equal({'foo': 'bar'}, perleval('{foo => "bar"}'))

  perl our %h; our @a;
  let a = perleval('[\(%h, %h, @a, @a)]')
  call assert_true((a[0] is a[1]))
  call assert_true((a[2] is a[3]))
  perl undef %h; undef @a;

  call assert_true(<SID>catch_peval('{"" , 0}') =~ 'Malformed key Dictionary')
  call assert_true(<SID>catch_peval('{"\0" , 0}') =~ 'Malformed key Dictionary')
  call assert_true(<SID>catch_peval('{"foo\0bar" , 0}') =~ 'Malformed key Dictionary')

  call assert_equal('*VIM', perleval('"*VIM"'))
  call assert_true(perleval('\\0') =~ 'SCALAR(0x\x\+)')
endf

function Test_perldo()
  sp __TEST__
  exe 'read ' g:testname
  perldo s/perl/vieux_chameau/g
  1
  call assert_false(search('\Cperl'))
  bw!
endf

function Test_VIM_package()
  perl VIM::DoCommand('let l:var = "foo"')
  call assert_equal(l:var, 'foo')

  set noet
  perl VIM::SetOption('et')
  call assert_true(&et)
endf
