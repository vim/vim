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
  " command name actually work
  enew!
  let ln = 'Testing string'
  let variants = [
	\ { 'cmd': ':sc/Test/test/', 'exp': 'testing string', 'prompt': 'y' },
	\ { 'cmd': ':sce/foo/bar/', 'exp': ln },
	\ { 'cmd': ':scg/t/r/', 'exp': 'Tesring srring', 'prompt': 'a' },
	\ { 'cmd': ':sci/t/r/', 'exp': 'resting string', 'prompt': 'y' },
	\ { 'cmd': ':scI/t/r/', 'exp': 'Tesring string', 'prompt': 'y' },
	\ { 'cmd': ':scn/t/r/', 'exp': ln },
	\ { 'cmd': ':scp/t/r/', 'exp': 'Tesring string', 'prompt': 'y' },
	\ { 'cmd': ':scl/t/r/', 'exp': 'Tesring string', 'prompt': 'y' },
	\ { 'cmd': ':sgc/t/r/', 'exp': 'Tesring srring', 'prompt': 'a' },
	\ { 'cmd': ':sge/foo/bar/', 'exp': ln },
	\ { 'cmd': ':sg/t/r/', 'exp': 'Tesring srring' },
	\ { 'cmd': ':sgi/t/r/', 'exp': 'resring srring' },
	\ { 'cmd': ':sgI/t/r/', 'exp': 'Tesring srring' },
	\ { 'cmd': ':sgn/t/r/', 'exp': ln },
	\ { 'cmd': ':sgp/t/r/', 'exp': 'Tesring srring' },
	\ { 'cmd': ':sgl/t/r/', 'exp': 'Tesring srring' },
	\ { 'cmd': ':sgr//r/', 'exp': 'Testr strr' },
	\ { 'cmd': ':sic/t/r/', 'exp': 'resting string', 'prompt': 'y' },
	\ { 'cmd': ':sie/foo/bar/', 'exp': ln },
	\ { 'cmd': ':si/t/r/', 'exp': 'resting string' },
	\ { 'cmd': ':siI/t/r/', 'exp': 'Tesring string' },
	\ { 'cmd': ':sin/t/r/', 'exp': ln },
	\ { 'cmd': ':sip/t/r/', 'exp': 'resting string' },
	\ { 'cmd': ':sir//r/', 'exp': 'Testr string' },
	\ { 'cmd': ':sIc/t/r/', 'exp': 'Tesring string', 'prompt': 'y' },
	\ { 'cmd': ':sIe/foo/bar/', 'exp': ln },
	\ { 'cmd': ':sIg/t/r/', 'exp': 'Tesring srring' },
	\ { 'cmd': ':sIi/t/r/', 'exp': 'resting string' },
	\ { 'cmd': ':sI/t/r/', 'exp': 'Tesring string' },
	\ { 'cmd': ':sIp/t/r/', 'exp': 'Tesring string' },
	\ { 'cmd': ':sIl/t/r/', 'exp': 'Tesring string' },
	\ { 'cmd': ':sIr//r/', 'exp': 'Testr string' },
	\ { 'cmd': ':src//r/', 'exp': 'Testr string', 'prompt': 'y' },
	\ { 'cmd': ':srg//r/', 'exp': 'Testr strr' },
	\ { 'cmd': ':sri//r/', 'exp': 'Testr string' },
	\ { 'cmd': ':srI//r/', 'exp': 'Testr string' },
	\ { 'cmd': ':srn//r/', 'exp': 'Testing string' },
	\ { 'cmd': ':srp//r/', 'exp': 'Testr string' },
	\ { 'cmd': ':srl//r/', 'exp': 'Testr string' },
	\ { 'cmd': ':sr//r/', 'exp': 'Testr string' },
	\]

  for var in variants
    call setline(1, [ln])
    let msg = printf('using "%s"', var.cmd)
    let @/='ing'
    let v:errmsg = ''
    call feedkeys(var.cmd . "\<CR>" . get(var, 'prompt', ''), 'ntx')
    " No error should exist (matters for testing e flag)
    call assert_equal('', v:errmsg, msg)
    call assert_equal(var.exp, getline('.'), msg)
  endfor
endfunction
