" Tests for encryption.

source check.vim
CheckFeature cryptv

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

func Test_head_only_4()
  CheckFeature sodium
  call Common_head_only('VimCrypt~04!abc')
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
  call assert_equal('*****', &key)
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

func Test_crypt_sodium()
  CheckFeature sodium
  call Crypt_uncrypt('xchacha20')
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

func Uncrypt_stable_xxd(method, hex, key, uncrypted_text)
  " use xxd to write the binary content
  call system('xxd -r >Xtest.txt', a:hex)
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

func Test_uncrypt_blowfish2a()
  call Uncrypt_stable('blowfish', "VimCrypt~03!\u001e\u00d1N\u00e3;\u00d3\u00c0\u00a0^C)\u0004\u00f7\u007f.\u00b6\u00abF\u000eS\u0019\u00e0\u008b6\u00d2[T\u00cb\u00a7\u0085\u00d8\u00be9\u000b\u00812\u000bQ\u00b3\u00cc@\u0097\u000f\u00df\u009a\u00adIv\u00aa.\u00d8\u00c9\u00ee\u009e`\u00bd$\u00af%\u00d0", "barburp", ["abcdefghijklmnopqrstuvwxyz", "!@#$%^&*()_+=-`~"])
endfunc

func Test_uncrypt_blowfish2()
  call Uncrypt_stable('blowfish2', "VimCrypt~03!\u001e\u00d1N\u00e3;\u00d3\u00c0\u00a0^C)\u0004\u00f7\u007f.\u00b6\u00abF\u000eS\u0019\u00e0\u008b6\u00d2[T\u00cb\u00a7\u0085\u00d8\u00be9\u000b\u00812\u000bQ\u00b3\u00cc@\u0097\u000f\u00df\u009a\u00adIv\u00aa.\u00d8\u00c9\u00ee\u009e`\u00bd$\u00af%\u00d0", "barburp", ["abcdefghijklmnopqrstuvwxyz", "!@#$%^&*()_+=-`~"])
endfunc

func Test_uncrypt_xchacha20()
  CheckFeature sodium
  let hex=['00000000: 5669 6d43 7279 7074 7e30 3421 6b7d e607  vimCrypt~04!k}..',
        \  '00000010: 4ea4 e99f 923e f67f 7b59 a80d 3bca 2f06  N....>..{Y..;./.',
        \  '00000020: fa11 b951 8d09 0dc9 470f e7cf 8b90 4310  ...Q....G.....C.',
        \  '00000030: 653b b83b e493 378b 0390 0e38 f912 626b  e;.;..7....8..bk',
        \  '00000040: a02e 4697 0254 2625 2d8e 3a0b 784b e89c  ..F..T&%-.:.xK..',
        \  '00000050: 0c67 a975 3c17 9319 8ffd 1463 7783 a1f3  .g.u<......cw...',
        \  '00000060: d917 dcb3 8b3e ecd7 c7d4 086b 6059 7ead  .....>.....k`Y~.',
        \  '00000070: 9b07 f96b 5c1b 4d08 cd91 f208 5221 7484  ...k\.M.....R!t.',
        \  '00000080: 72be 0136 84a1 d3                        r..6...']
  " the file should be in latin1 encoding, this makes sure that readfile()
  " retries several times converting the multi-byte characters
  call Uncrypt_stable_xxd('xchacha20', hex, "sodium_crypt", ["abcdefghijklmnopqrstuvwxyzäöü", "ZZZ_äüöÄÜÖ_!@#$%^&*()_+=-`~"])
endfunc

func Test_uncrypt_xchacha20_invalid()
  CheckFeature sodium

  " load an invalid encrypted file and verify it can be decrypted with an
  " error message
  try
    call feedkeys(":split samples/crypt_sodium_invalid.txt\<CR>sodium\<CR>", 'xt')
    call assert_false(1, 'should not happen')
  catch
    call assert_exception('pre-mature')
  endtry
  call assert_match("Note: Encryption of swapfile not supported, disabling swap- and undofile", execute(':5messages'))

  call assert_equal(0, &swapfile)
  call assert_equal("xchacha20", &cryptmethod)
  call assert_equal('311111111111111111111111', getline('$'))
  bw!
endfunc

func Test_uncrypt_xchacha20_2()
  CheckFeature sodium

  sp Xcrypt_sodium.txt
  " Create a larger file, so that Vim will write in several blocks
  call setline(1, range(1,4000))
  call assert_equal(1, &swapfile)
  set cryptmethod=xchacha20
  call feedkeys(":X\<CR>sodium\<CR>sodium\<CR>", 'xt')
  " swapfile disabled
  call assert_equal(0, &swapfile)
  call assert_match("Note: Encryption of swapfile not supported, disabling swap- and undofile", execute(':messages'))
  w!
  " encrypted using xchacha20
  call assert_match("\[xchacha20\]", execute(':messages'))
  bw!
  call feedkeys(":sp Xcrypt_sodium.txt\<CR>sodium\<CR>", 'xt')
  " successfully decrypted
  call assert_equal(range(1, 4000)->map( {_, v -> string(v)}), getline(1,'$'))
  set key=
  w! ++ff=unix
  " enryption removed (on MS-Windows the .* matches [unix])
  call assert_match('"Xcrypt_sodium.txt".*4000L, 18893B written', execute(':message'))
  bw!
  call delete('Xcrypt_sodium.txt')
  set cryptmethod&vim
endfunc

func Test_uncrypt_xchacha20_3_persistent_undo()
  CheckFeature sodium
  CheckFeature persistent_undo

  sp Xcrypt_sodium_undo.txt
  set cryptmethod=xchacha20 undofile
  call feedkeys(":X\<CR>sodium\<CR>sodium\<CR>", 'xt')
  call assert_equal(0, &undofile)
  let ufile=undofile(@%)
  call append(0, ['monday', 'tuesday', 'wednesday', 'thursday', 'friday'])
  call cursor(1, 1)

  set undolevels=100
  normal dd
  set undolevels=100
  normal dd
  set undolevels=100
  normal dd
  set undolevels=100
  w!
  bw!
  call feedkeys(":sp Xcrypt_sodium_undo.txt\<CR>sodium\<CR>", 'xt')
  " should fail
  norm! u
  call assert_match('Already at oldest change', execute(':1mess'))
  call assert_fails('verbose rundo ' .. fnameescape(ufile), 'E822')
  bw!
  set undolevels& cryptmethod& undofile&
  call delete('Xcrypt_sodium_undo.txt')
endfunc

func Test_encrypt_xchacha20_missing()
  if has("sodium")
    return
  endif
  sp Xcrypt_sodium_undo.txt
  call assert_fails(':set cryptmethod=xchacha20', 'E474')
  bw!
  set cm&
endfunc

func Test_uncrypt_unknown_method()
  split Xuncrypt_unknown.txt
  set bin noeol key= fenc=latin1
  call setline(1, "VimCrypt~93!\u001e\u00d1")
  w!
  bwipe!
  set nobin
  call assert_fails(":split Xuncrypt_unknown.txt", 'E821:')

  bwipe!
  call delete('Xuncrypt_unknown.txt')
  set key=
endfunc

func Test_crypt_key_mismatch()
  set cryptmethod=blowfish

  split Xtest.txt
  call setline(1, 'nothing')
  call feedkeys(":X\<CR>foobar\<CR>nothing\<CR>", 'xt')
  call assert_match("Keys don't match!", execute(':2messages'))
  call assert_equal('', &key)
  call feedkeys("\<CR>\<CR>", 'xt')

  set cryptmethod&
  bwipe!
endfunc

func Test_crypt_set_key_changes_buffer()

  new Xtest1.txt
  call setline(1, 'nothing')
  set cryptmethod=blowfish2
  call feedkeys(":X\<CR>foobar\<CR>foobar\<CR>", 'xt')
  call assert_fails(":q", "E37:")
  w
  set key=anotherkey
  call assert_fails(":bw")
  w
  call feedkeys(":X\<CR>foobar\<CR>foobar\<CR>", 'xt')
  call assert_fails(":bw")
  w
  let winnr = winnr()
  wincmd p
  call setwinvar(winnr, '&key', 'yetanotherkey')
  wincmd p
  call assert_fails(":bw")
  w

  set cryptmethod&
  set key=
  bwipe!
  call delete('Xtest1.txt')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
