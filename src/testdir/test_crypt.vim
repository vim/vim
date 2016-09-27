" Tests for encryption.

if !has('cryptv')
  finish
endif

func Common_head_only(text)
  " This was crashing Vim
  split Xtest.txt
  call setline(1, a:text)
  wq
  call feedkeys(":split Xtest.txt\<CR>foobar\<CR>", "tx")
  call delete('Xtest.txt')
  call assert_match('VimCrypt', getline(1))
  bwipe!
endfunc

func Test_head_only_2()
  call Common_head_only('VimCrypt~02!abc')
endfunc

func Test_head_only_3()
  call Common_head_only('VimCrypt~03!abc')
endfunc

func Crypt_uncrypt(method)
  exe "set cryptmethod=" . a:method
  " If the blowfish test fails 'cryptmethod' will be 'zip' now.
  call assert_equal(a:method, &cryptmethod)

  split Xtest.txt
  let text = ['01234567890123456789012345678901234567',
	\ 'line 2  foo bar blah',
	\ 'line 3 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx']
  call setline(1, text)
  call feedkeys(":X\<CR>foobar\<CR>foobar\<CR>", 'xt')
  w!
  bwipe!
  call feedkeys(":split Xtest.txt\<CR>foobar\<CR>", 'xt')
  call assert_equal(text, getline(1, 3))
  set key= cryptmethod&
  bwipe!
  call delete('Xtest.txt')
endfunc

func Test_crypt_zip()
  call Crypt_uncrypt('zip')
endfunc

func Test_crypt_blowfish()
  call Crypt_uncrypt('blowfish')
endfunc

func Test_crypt_blowfish2()
  call Crypt_uncrypt('blowfish2')
endfunc

func Uncrypt_stable(method, crypted_text, key, uncrypted_text)
  split Xtest.txt
  set bin noeol key= fenc=latin1
  exe "set cryptmethod=" . a:method
  call setline(1, a:crypted_text)
  w!
  bwipe!
  set nobin
  call feedkeys(":split Xtest.txt\<CR>" . a:key . "\<CR>", 'xt')
  call assert_equal(a:uncrypted_text, getline(1, len(a:uncrypted_text)))
  bwipe!
  call delete('Xtest.txt')
  set key=
endfunc

func Test_uncrypt_zip()
  call Uncrypt_stable('zip', "VimCrypt~01!\u0006\u001clV'\u00de}Mg\u00a0\u00ea\u00a3V\u00a9\u00e7\u0007E#3\u008e2U\u00e9\u0097", "foofoo", ["1234567890", "aábbccddeëff"])
endfunc

func Test_uncrypt_blowfish()
  call Uncrypt_stable('blowfish', "VimCrypt~02!k)\u00be\u0017\u0097#\u0016\u00ddS\u009c\u00f5=\u00ba\u00e0\u00c8#\u00a5M\u00b4\u0086J\u00c3A\u00cd\u00a5M\u00b4\u0086!\u0080\u0015\u009b\u00f5\u000f\u00e1\u00d2\u0019\u0082\u0016\u0098\u00f7\u000d\u00da", "barbar", ["asdfasdfasdf", "0001112223333"])
endfunc

func Test_uncrypt_blowfish2()
  call Uncrypt_stable('blowfish', "VimCrypt~03!\u001e\u00d1N\u00e3;\u00d3\u00c0\u00a0^C)\u0004\u00f7\u007f.\u00b6\u00abF\u000eS\u0019\u00e0\u008b6\u00d2[T\u00cb\u00a7\u0085\u00d8\u00be9\u000b\u00812\u000bQ\u00b3\u00cc@\u0097\u000f\u00df\u009a\u00adIv\u00aa.\u00d8\u00c9\u00ee\u009e`\u00bd$\u00af%\u00d0", "barburp", ["abcdefghijklmnopqrstuvwxyz", "!@#$%^&*()_+=-`~"])
endfunc
