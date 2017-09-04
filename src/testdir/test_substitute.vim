" Tests for multi-line regexps with ":s".

function! Test_multiline_subst()
  enew!
  call append(0, ["1 aa",
	      \ "bb",
	      \ "cc",
	      \ "2 dd",
	      \ "ee",
	      \ "3 ef",
	      \ "gh",
	      \ "4 ij",
	      \ "5 a8",
	      \ "8b c9",
	      \ "9d",
	      \ "6 e7",
	      \ "77f",
	      \ "xxxxx"])

  1
  " test if replacing a line break works with a back reference
  /^1/,/^2/s/\n\(.\)/ \1/
  " test if inserting a line break works with a back reference
  /^3/,/^4/s/\(.\)$/\r\1/
  " test if replacing a line break with another line break works
  /^5/,/^6/s/\(\_d\{3}\)/x\1x/
  call assert_equal('1 aa bb cc 2 dd ee', getline(1))
  call assert_equal('3 e', getline(2))
  call assert_equal('f', getline(3))
  call assert_equal('g', getline(4))
  call assert_equal('h', getline(5))
  call assert_equal('4 i', getline(6))
  call assert_equal('j', getline(7))
  call assert_equal('5 ax8', getline(8))
  call assert_equal('8xb cx9', getline(9))
  call assert_equal('9xd', getline(10))
  call assert_equal('6 ex7', getline(11))
  call assert_equal('7x7f', getline(12))
  call assert_equal('xxxxx', getline(13))
  enew!
endfunction

function! Test_substitute_variants()
  " Validate that all the 2-/3-letter variants which embed the flags into the
  " command name actually work.
  enew!
  let ln = 'Testing string'
  let variants = [
	\ { 'cmd': ':s/Test/test/c', 'exp': 'testing string', 'prompt': 'y' },
	\ { 'cmd': ':s/foo/bar/ce', 'exp': ln },
	\ { 'cmd': ':s/t/r/cg', 'exp': 'Tesring srring', 'prompt': 'a' },
	\ { 'cmd': ':s/t/r/ci', 'exp': 'resting string', 'prompt': 'y' },
	\ { 'cmd': ':s/t/r/cI', 'exp': 'Tesring string', 'prompt': 'y' },
	\ { 'cmd': ':s/t/r/cn', 'exp': ln },
	\ { 'cmd': ':s/t/r/cp', 'exp': 'Tesring string', 'prompt': 'y' },
	\ { 'cmd': ':s/t/r/cl', 'exp': 'Tesring string', 'prompt': 'y' },
	\ { 'cmd': ':s/t/r/gc', 'exp': 'Tesring srring', 'prompt': 'a' },
	\ { 'cmd': ':s/foo/bar/ge', 'exp': ln },
	\ { 'cmd': ':s/t/r/g', 'exp': 'Tesring srring' },
	\ { 'cmd': ':s/t/r/gi', 'exp': 'resring srring' },
	\ { 'cmd': ':s/t/r/gI', 'exp': 'Tesring srring' },
	\ { 'cmd': ':s/t/r/gn', 'exp': ln },
	\ { 'cmd': ':s/t/r/gp', 'exp': 'Tesring srring' },
	\ { 'cmd': ':s/t/r/gl', 'exp': 'Tesring srring' },
	\ { 'cmd': ':s//r/gr', 'exp': 'Testr strr' },
	\ { 'cmd': ':s/t/r/ic', 'exp': 'resting string', 'prompt': 'y' },
	\ { 'cmd': ':s/foo/bar/ie', 'exp': ln },
	\ { 'cmd': ':s/t/r/i', 'exp': 'resting string' },
	\ { 'cmd': ':s/t/r/iI', 'exp': 'Tesring string' },
	\ { 'cmd': ':s/t/r/in', 'exp': ln },
	\ { 'cmd': ':s/t/r/ip', 'exp': 'resting string' },
	\ { 'cmd': ':s//r/ir', 'exp': 'Testr string' },
	\ { 'cmd': ':s/t/r/Ic', 'exp': 'Tesring string', 'prompt': 'y' },
	\ { 'cmd': ':s/foo/bar/Ie', 'exp': ln },
	\ { 'cmd': ':s/t/r/Ig', 'exp': 'Tesring srring' },
	\ { 'cmd': ':s/t/r/Ii', 'exp': 'resting string' },
	\ { 'cmd': ':s/t/r/I', 'exp': 'Tesring string' },
	\ { 'cmd': ':s/t/r/Ip', 'exp': 'Tesring string' },
	\ { 'cmd': ':s/t/r/Il', 'exp': 'Tesring string' },
	\ { 'cmd': ':s//r/Ir', 'exp': 'Testr string' },
	\ { 'cmd': ':s//r/rc', 'exp': 'Testr string', 'prompt': 'y' },
	\ { 'cmd': ':s//r/rg', 'exp': 'Testr strr' },
	\ { 'cmd': ':s//r/ri', 'exp': 'Testr string' },
	\ { 'cmd': ':s//r/rI', 'exp': 'Testr string' },
	\ { 'cmd': ':s//r/rn', 'exp': 'Testing string' },
	\ { 'cmd': ':s//r/rp', 'exp': 'Testr string' },
	\ { 'cmd': ':s//r/rl', 'exp': 'Testr string' },
	\ { 'cmd': ':s//r/r', 'exp': 'Testr string' },
	\]

  for var in variants
    for run in [1, 2]
      let cmd = var.cmd
      if run == 2 && cmd =~ "/.*/.*/."
	" Change  :s/from/to/{flags}  to  :s{flags}
	let cmd = substitute(cmd, '/.*/', '', '')
      endif
      call setline(1, [ln])
      let msg = printf('using "%s"', cmd)
      let @/='ing'
      let v:errmsg = ''
      call feedkeys(cmd . "\<CR>" . get(var, 'prompt', ''), 'ntx')
      " No error should exist (matters for testing e flag)
      call assert_equal('', v:errmsg, msg)
      call assert_equal(var.exp, getline('.'), msg)
    endfor
  endfor
endfunction

func Test_substitute_repeat()
  " This caused an invalid memory access.
  split Xfile
  s/^/x
  call feedkeys("Qsc\<CR>y", 'tx')
  bwipe!
endfunc

" Test for *sub-replace-special* and *sub-replace-expression* on substitute().
func Test_sub_replace_1()
  " Run the tests with 'magic' on
  set magic
  set cpo&
  call assert_equal('AA', substitute('A', 'A', '&&', ''))
  call assert_equal('&', substitute('B', 'B', '\&', ''))
  call assert_equal('C123456789987654321', substitute('C123456789', 'C\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)', '\0\9\8\7\6\5\4\3\2\1', ''))
  call assert_equal('d', substitute('D', 'D', 'd', ''))
  call assert_equal('~', substitute('E', 'E', '~', ''))
  call assert_equal('~', substitute('F', 'F', '\~', ''))
  call assert_equal('Gg', substitute('G', 'G', '\ugg', ''))
  call assert_equal('Hh', substitute('H', 'H', '\Uh\Eh', ''))
  call assert_equal('iI', substitute('I', 'I', '\lII', ''))
  call assert_equal('jJ', substitute('J', 'J', '\LJ\EJ', ''))
  call assert_equal('Kk', substitute('K', 'K', '\Uk\ek', ''))
  call assert_equal("l\<C-V>\<C-M>l",
			\ substitute('lLl', 'L', "\<C-V>\<C-M>", ''))
  call assert_equal("m\<C-M>m", substitute('mMm', 'M', '\r', ''))
  call assert_equal("n\<C-V>\<C-M>n",
			\ substitute('nNn', 'N', "\\\<C-V>\<C-M>", ''))
  call assert_equal("o\no", substitute('oOo', 'O', '\n', ''))
  call assert_equal("p\<C-H>p", substitute('pPp', 'P', '\b', ''))
  call assert_equal("q\tq", substitute('qQq', 'Q', '\t', ''))
  call assert_equal('r\r', substitute('rRr', 'R', '\\', ''))
  call assert_equal('scs', substitute('sSs', 'S', '\c', ''))
  call assert_equal("u\nu", substitute('uUu', 'U', "\n", ''))
  call assert_equal("v\<C-H>v", substitute('vVv', 'V', "\b", ''))
  call assert_equal("w\\w", substitute('wWw', 'W', "\\", ''))
  call assert_equal("x\<C-M>x", substitute('xXx', 'X', "\r", ''))
  call assert_equal("YyyY", substitute('Y', 'Y', '\L\uyYy\l\EY', ''))
  call assert_equal("zZZz", substitute('Z', 'Z', '\U\lZzZ\u\Ez', ''))
endfunc

func Test_sub_replace_2()
  " Run the tests with 'magic' off
  set nomagic
  set cpo&
  call assert_equal('AA', substitute('A', 'A', '&&', ''))
  call assert_equal('&', substitute('B', 'B', '\&', ''))
  call assert_equal('C123456789987654321', substitute('C123456789', 'C\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)', '\0\9\8\7\6\5\4\3\2\1', ''))
  call assert_equal('d', substitute('D', 'D', 'd', ''))
  call assert_equal('~', substitute('E', 'E', '~', ''))
  call assert_equal('~', substitute('F', 'F', '\~', ''))
  call assert_equal('Gg', substitute('G', 'G', '\ugg', ''))
  call assert_equal('Hh', substitute('H', 'H', '\Uh\Eh', ''))
  call assert_equal('iI', substitute('I', 'I', '\lII', ''))
  call assert_equal('jJ', substitute('J', 'J', '\LJ\EJ', ''))
  call assert_equal('Kk', substitute('K', 'K', '\Uk\ek', ''))
  call assert_equal("l\<C-V>\<C-M>l",
			\ substitute('lLl', 'L', "\<C-V>\<C-M>", ''))
  call assert_equal("m\<C-M>m", substitute('mMm', 'M', '\r', ''))
  call assert_equal("n\<C-V>\<C-M>n",
			\ substitute('nNn', 'N', "\\\<C-V>\<C-M>", ''))
  call assert_equal("o\no", substitute('oOo', 'O', '\n', ''))
  call assert_equal("p\<C-H>p", substitute('pPp', 'P', '\b', ''))
  call assert_equal("q\tq", substitute('qQq', 'Q', '\t', ''))
  call assert_equal('r\r', substitute('rRr', 'R', '\\', ''))
  call assert_equal('scs', substitute('sSs', 'S', '\c', ''))
  call assert_equal("t\<C-M>t", substitute('tTt', 'T', "\r", ''))
  call assert_equal("u\nu", substitute('uUu', 'U', "\n", ''))
  call assert_equal("v\<C-H>v", substitute('vVv', 'V', "\b", ''))
  call assert_equal('w\w', substitute('wWw', 'W', "\\", ''))
  call assert_equal('XxxX', substitute('X', 'X', '\L\uxXx\l\EX', ''))
  call assert_equal('yYYy', substitute('Y', 'Y', '\U\lYyY\u\Ey', ''))
endfunc

func Test_sub_replace_3()
  set magic&
  set cpo&
  call assert_equal('a\a', substitute('aAa', 'A', '\="\\"', ''))
  call assert_equal('b\\b', substitute('bBb', 'B', '\="\\\\"', ''))
  call assert_equal("c\rc", substitute('cCc', 'C', "\\=\"\r\"", ''))
  call assert_equal("d\\\rd", substitute('dDd', 'D', "\\=\"\\\\\r\"", ''))
  call assert_equal("e\\\\\re", substitute('eEe', 'E', "\\=\"\\\\\\\\\r\"", ''))
  call assert_equal('f\rf', substitute('fFf', 'F', '\="\\r"', ''))
  call assert_equal('j\nj', substitute('jJj', 'J', '\="\\n"', ''))
  call assert_equal("k\<C-M>k", substitute('kKk', 'K', '\="\r"', ''))
  call assert_equal("l\nl", substitute('lLl', 'L', '\="\n"', ''))
endfunc

" Test for submatch() on substitute().
func Test_sub_replace_4()
  set magic&
  set cpo&
  call assert_equal('a\a', substitute('aAa', 'A',
		\ '\=substitute(submatch(0), ".", "\\", "")', ''))
  call assert_equal('b\b', substitute('bBb', 'B',
		\ '\=substitute(submatch(0), ".", "\\\\", "")', ''))
  call assert_equal("c\<C-V>\<C-M>c", substitute('cCc', 'C', '\=substitute(submatch(0), ".", "\<C-V>\<C-M>", "")', ''))
  call assert_equal("d\<C-V>\<C-M>d", substitute('dDd', 'D', '\=substitute(submatch(0), ".", "\\\<C-V>\<C-M>", "")', ''))
  call assert_equal("e\\\<C-V>\<C-M>e", substitute('eEe', 'E', '\=substitute(submatch(0), ".", "\\\\\<C-V>\<C-M>", "")', ''))
  call assert_equal("f\<C-M>f", substitute('fFf', 'F', '\=substitute(submatch(0), ".", "\\r", "")', ''))
  call assert_equal("j\nj", substitute('jJj', 'J', '\=substitute(submatch(0), ".", "\\n", "")', ''))
  call assert_equal("k\rk", substitute('kKk', 'K', '\=substitute(submatch(0), ".", "\r", "")', ''))
  call assert_equal("l\nl", substitute('lLl', 'L', '\=substitute(submatch(0), ".", "\n", "")', ''))
endfunc

func Test_sub_replace_5()
  set magic&
  set cpo&
  call assert_equal('A123456789987654321', substitute('A123456789',
		\ 'A\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)',
		\ '\=submatch(0) . submatch(9) . submatch(8) . ' .
		\ 'submatch(7) . submatch(6) . submatch(5) . ' .
		\ 'submatch(4) . submatch(3) . submatch(2) . submatch(1)',
		\ ''))
   call assert_equal("[['A123456789'], ['9'], ['8'], ['7'], ['6'], " .
		\ "['5'], ['4'], ['3'], ['2'], ['1']]",
		\ substitute('A123456789',
		\ 'A\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)\(.\)',
		\ '\=string([submatch(0, 1), submatch(9, 1), ' .
		\ 'submatch(8, 1), submatch(7, 1), submatch(6, 1), ' .
		\ 'submatch(5, 1), submatch(4, 1), submatch(3, 1), ' .
		\ 'submatch(2, 1), submatch(1, 1)])',
		\ ''))
endfunc

func Test_sub_replace_6()
  set magic&
  set cpo+=/
  call assert_equal('a', substitute('A', 'A', 'a', ''))
  call assert_equal('%', substitute('B', 'B', '%', ''))
  set cpo-=/
  call assert_equal('c', substitute('C', 'C', 'c', ''))
  call assert_equal('%', substitute('D', 'D', '%', ''))
endfunc

func Test_sub_replace_7()
  set magic&
  set cpo&
  call assert_equal('AA', substitute('AA', 'A.', '\=submatch(0)', ''))
  call assert_equal("B\nB", substitute("B\nB", 'B.', '\=submatch(0)', ''))
  call assert_equal("['B\n']B", substitute("B\nB", 'B.', '\=string(submatch(0, 1))', ''))
  call assert_equal('-abab', substitute('-bb', '\zeb', 'a', 'g'))
  call assert_equal('c-cbcbc', substitute('-bb', '\ze', 'c', 'g'))
endfunc

" Test for *:s%* on :substitute.
func Test_sub_replace_8()
  new
  set magic&
  set cpo&
  $put =',,X'
  s/\(^\|,\)\ze\(,\|X\)/\1N/g
  call assert_equal('N,,NX', getline("$"))
  $put =',,Y'
  let cmd = ':s/\(^\|,\)\ze\(,\|Y\)/\1N/gc'
  call feedkeys(cmd . "\<CR>a", "xt")
  call assert_equal('N,,NY', getline("$"))
  :$put =',,Z'
  let cmd = ':s/\(^\|,\)\ze\(,\|Z\)/\1N/gc'
  call feedkeys(cmd . "\<CR>yy", "xt")
  call assert_equal('N,,NZ', getline("$"))
  enew! | close
endfunc

func Test_sub_replace_9()
  new
  set magic&
  set cpo&
  $put ='xxx'
  call feedkeys(":s/x/X/gc\<CR>yyq", "xt")
  call assert_equal('XXx', getline("$"))
  enew! | close
endfunc

func Test_sub_replace_10()
   set magic&
   set cpo&
   call assert_equal('a1a2a3a', substitute('123', '\zs', 'a', 'g'))
   call assert_equal('aaa', substitute('123', '\zs.', 'a', 'g'))
   call assert_equal('1a2a3a', substitute('123', '.\zs', 'a', 'g'))
   call assert_equal('a1a2a3a', substitute('123', '\ze', 'a', 'g'))
   call assert_equal('a1a2a3', substitute('123', '\ze.', 'a', 'g'))
   call assert_equal('aaa', substitute('123', '.\ze', 'a', 'g'))
   call assert_equal('aa2a3a', substitute('123', '1\|\ze', 'a', 'g'))
   call assert_equal('1aaa', substitute('123', '1\zs\|[23]', 'a', 'g'))
endfunc
