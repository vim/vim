" Tests for Perl interface

if !has('perl')
  finish
end

func Test_change_buffer()
  call setline(line('$'), ['1 line 1'])
  perl VIM::DoCommand("normal /^1\n")
  perl $curline = VIM::Eval("line('.')")
  perl $curbuf->Set($curline, "1 changed line 1")
  call assert_equal('1 changed line 1', getline('$'))
endfunc

func Test_evaluate_list()
  call setline(line('$'), ['2 line 2'])
  perl VIM::DoCommand("normal /^2\n")
  perl $curline = VIM::Eval("line('.')")
  let l = ["abc", "def"]
  perl << EOF
  $l = VIM::Eval("l");
  $curbuf->Append($curline, $l);
EOF
  normal j
  .perldo s|\n|/|g
  call assert_equal('abc/def/', getline('$'))
endfunc

fu <SID>catch_peval(expr)
  try
    call perleval(a:expr)
  catch
    return v:exception
  endtry
  call assert_true(0, 'no exception for `perleval("'.a:expr.'")`')
  return ''
endfunc

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
endfunc

function Test_perldo()
  sp __TEST__
  exe 'read ' g:testname
  perldo s/perl/vieux_chameau/g
  1
  call assert_false(search('\Cperl'))
  bw!

  " Check deleting lines does not trigger ml_get error.
  new
  call setline(1, ['one', 'two', 'three'])
  perldo VIM::DoCommand("%d_")
  bwipe!

  " Check switching to another buffer does not trigger ml_get error.
  new
  let wincount = winnr('$')
  call setline(1, ['one', 'two', 'three'])
  perldo VIM::DoCommand("new")
  call assert_equal(wincount + 1, winnr('$'))
  bwipe!
  bwipe!
endfunc

function Test_VIM_package()
  perl VIM::DoCommand('let l:var = "foo"')
  call assert_equal(l:var, 'foo')

  set noet
  perl VIM::SetOption('et')
  call assert_true(&et)
endfunc

function Test_stdio()
  redir =>l:out
  perl <<EOF
    VIM::Msg("&VIM::Msg");
    print "STDOUT";
    print STDERR "STDERR";
EOF
  redir END
  call assert_equal(['&VIM::Msg', 'STDOUT', 'STDERR'], split(l:out, "\n"))
endfunc

function Test_SvREFCNT()
  new t
  perl <<--perl
  my ($b, $w);
  $b = $curbuf for 0 .. 10;
  $w = $curwin for 0 .. 10;
  VIM::DoCommand('bw! t');
  if (exists &Internals::SvREFCNT) {
      my $cb = Internals::SvREFCNT($$b);
      my $cw = Internals::SvREFCNT($$w);
      VIM::Eval("assert_equal(2, $cb)");
      VIM::Eval("assert_equal(2, $cw)");
  }
  VIM::Eval("assert_false($$b)");
  VIM::Eval("assert_false($$w)");
--perl
endfunc
