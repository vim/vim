" Test for insert completion

source screendump.vim
source check.vim

" Test for insert expansion
func Test_ins_complete()
  edit test_ins_complete.vim
  " The files in the current directory interferes with the files
  " used by this test. So use a separate directory for the test.
  call mkdir('Xdir')
  cd Xdir

  set ff=unix
  call writefile(["test11\t36Gepeto\t/Tag/",
	      \ "asd\ttest11file\t36G",
	      \ "Makefile\tto\trun"], 'Xtestfile')
  call writefile(['', 'start of testfile',
	      \ 'ru',
	      \ 'run1',
	      \ 'run2',
	      \ 'STARTTEST',
	      \ 'ENDTEST',
	      \ 'end of testfile'], 'Xtestdata')
  set ff&

  enew!
  edit Xtestdata
  new
  call append(0, ['#include "Xtestfile"', ''])
  call cursor(2, 1)

  set cot=
  set cpt=.,w
  " add-expands (word from next line) from other window
  exe "normal iru\<C-N>\<C-N>\<C-X>\<C-N>\<Esc>\<C-A>"
  call assert_equal('run1 run3', getline('.'))
  " add-expands (current buffer first)
  exe "normal o\<C-P>\<C-X>\<C-N>"
  call assert_equal('run3 run3', getline('.'))
  " Local expansion, ends in an empty line (unless it becomes a global
  " expansion)
  exe "normal o\<C-X>\<C-P>\<C-P>\<C-P>\<C-P>\<C-P>"
  call assert_equal('', getline('.'))
  " starts Local and switches to global add-expansion
  exe "normal o\<C-X>\<C-P>\<C-P>\<C-X>\<C-X>\<C-N>\<C-X>\<C-N>\<C-N>"
  call assert_equal('run1 run2', getline('.'))

  set cpt=.,w,i
  " i-add-expands and switches to local
  exe "normal OM\<C-N>\<C-X>\<C-N>\<C-X>\<C-N>\<C-X>\<C-X>\<C-X>\<C-P>"
  call assert_equal("Makefile\tto\trun3", getline('.'))
  " add-expands lines (it would end in an empty line if it didn't ignored
  " itself)
  exe "normal o\<C-X>\<C-L>\<C-X>\<C-L>\<C-P>\<C-P>"
  call assert_equal("Makefile\tto\trun3", getline('.'))
  call assert_equal("Makefile\tto\trun3", getline(line('.') - 1))

  set cpt=kXtestfile
  " checks k-expansion, and file expansion (use Xtest11 instead of test11,
  " because TEST11.OUT may match first on DOS)
  write Xtest11.one
  write Xtest11.two
  exe "normal o\<C-N>\<Esc>IX\<Esc>A\<C-X>\<C-F>\<C-N>"
  call assert_equal('Xtest11.two', getline('.'))

  " use CTRL-X CTRL-F to complete Xtest11.one, remove it and then use CTRL-X
  " CTRL-F again to verify this doesn't cause trouble.
  exe "normal oXt\<C-X>\<C-F>\<BS>\<BS>\<BS>\<BS>\<BS>\<BS>\<BS>\<BS>\<C-X>\<C-F>"
  call assert_equal('Xtest11.one', getline('.'))
  normal ddk

  set cpt=w
  " checks make_cyclic in other window
  exe "normal oST\<C-N>\<C-P>\<C-P>\<C-P>\<C-P>"
  call assert_equal('STARTTEST', getline('.'))

  set cpt=u nohid
  " checks unloaded buffer expansion
  only
  exe "normal oEN\<C-N>"
  call assert_equal('ENDTEST', getline('.'))
  " checks adding mode abortion
  exe "normal ounl\<C-N>\<C-X>\<C-X>\<C-P>"
  call assert_equal('unless', getline('.'))

  set cpt=t,d def=^\\k* tags=Xtestfile notagbsearch
  " tag expansion, define add-expansion interrupted
  exe "normal o\<C-X>\<C-]>\<C-X>\<C-D>\<C-X>\<C-D>\<C-X>\<C-X>\<C-D>\<C-X>\<C-D>\<C-X>\<C-D>\<C-X>\<C-D>"
  call assert_equal('test11file	36Gepeto	/Tag/ asd', getline('.'))
  " t-expansion
  exe "normal oa\<C-N>\<Esc>"
  call assert_equal('asd', getline('.'))

  %bw!
  call delete('Xtestfile')
  call delete('Xtest11.one')
  call delete('Xtest11.two')
  call delete('Xtestdata')
  set cpt& cot& def& tags& tagbsearch& hidden&
  cd ..
  call delete('Xdir', 'rf')
endfunc

func Test_omni_dash()
  func Omni(findstart, base)
    if a:findstart
        return 5
    else
        echom a:base
	return ['-help', '-v']
    endif
  endfunc
  set omnifunc=Omni
  new
  exe "normal Gofind -\<C-x>\<C-o>"
  call assert_equal("\n-\nmatch 1 of 2", execute(':2mess'))

  bwipe!
  delfunc Omni
  set omnifunc=
endfunc

func Test_completefunc_args()
  let s:args = []
  func! CompleteFunc(findstart, base)
    let s:args += [[a:findstart, empty(a:base)]]
  endfunc
  new

  set completefunc=CompleteFunc
  call feedkeys("i\<C-X>\<C-U>\<Esc>", 'x')
  call assert_equal([1, 1], s:args[0])
  call assert_equal(0, s:args[1][0])
  set completefunc=

  let s:args = []
  set omnifunc=CompleteFunc
  call feedkeys("i\<C-X>\<C-O>\<Esc>", 'x')
  call assert_equal([1, 1], s:args[0])
  call assert_equal(0, s:args[1][0])
  set omnifunc=

  bwipe!
  unlet s:args
  delfunc CompleteFunc
endfunc

func s:CompleteDone_CompleteFuncNone( findstart, base )
  if a:findstart
    return 0
  endif

  return v:none
endfunc

func s:CompleteDone_CompleteFuncDict( findstart, base )
  if a:findstart
    return 0
  endif

  return {
	  \ 'words': [
	    \ {
	      \ 'word': 'aword',
	      \ 'abbr': 'wrd',
	      \ 'menu': 'extra text',
	      \ 'info': 'words are cool',
	      \ 'kind': 'W',
	      \ 'user_data': 'test'
	    \ }
	  \ ]
	\ }
endfunc

func s:CompleteDone_CheckCompletedItemNone()
  let s:called_completedone = 1
endfunc

func s:CompleteDone_CheckCompletedItemDict(pre)
  call assert_equal( 'aword',          v:completed_item[ 'word' ] )
  call assert_equal( 'wrd',            v:completed_item[ 'abbr' ] )
  call assert_equal( 'extra text',     v:completed_item[ 'menu' ] )
  call assert_equal( 'words are cool', v:completed_item[ 'info' ] )
  call assert_equal( 'W',              v:completed_item[ 'kind' ] )
  call assert_equal( 'test',           v:completed_item[ 'user_data' ] )

  if a:pre
    call assert_equal('function', complete_info().mode)
  endif

  let s:called_completedone = 1
endfunc

func Test_CompleteDoneNone()
  au CompleteDone * :call <SID>CompleteDone_CheckCompletedItemNone()
  let oldline = join(map(range(&columns), 'nr2char(screenchar(&lines-1, v:val+1))'), '')

  set completefunc=<SID>CompleteDone_CompleteFuncNone
  execute "normal a\<C-X>\<C-U>\<C-Y>"
  set completefunc&
  let newline = join(map(range(&columns), 'nr2char(screenchar(&lines-1, v:val+1))'), '')

  call assert_true(s:called_completedone)
  call assert_equal(oldline, newline)

  let s:called_completedone = 0
  au! CompleteDone
endfunc

func Test_CompleteDoneDict()
  au CompleteDonePre * :call <SID>CompleteDone_CheckCompletedItemDict(1)
  au CompleteDone * :call <SID>CompleteDone_CheckCompletedItemDict(0)

  set completefunc=<SID>CompleteDone_CompleteFuncDict
  execute "normal a\<C-X>\<C-U>\<C-Y>"
  set completefunc&

  call assert_equal('test', v:completed_item[ 'user_data' ])
  call assert_true(s:called_completedone)

  let s:called_completedone = 0
  au! CompleteDone
endfunc

func s:CompleteDone_CompleteFuncDictNoUserData(findstart, base)
  if a:findstart
    return 0
  endif

  return {
	  \ 'words': [
	    \ {
	      \ 'word': 'aword',
	      \ 'abbr': 'wrd',
	      \ 'menu': 'extra text',
	      \ 'info': 'words are cool',
	      \ 'kind': 'W',
	      \ 'user_data': ['one', 'two'],
	    \ }
	  \ ]
	\ }
endfunc

func s:CompleteDone_CheckCompletedItemDictNoUserData()
  call assert_equal( 'aword',          v:completed_item[ 'word' ] )
  call assert_equal( 'wrd',            v:completed_item[ 'abbr' ] )
  call assert_equal( 'extra text',     v:completed_item[ 'menu' ] )
  call assert_equal( 'words are cool', v:completed_item[ 'info' ] )
  call assert_equal( 'W',              v:completed_item[ 'kind' ] )
  call assert_equal( ['one', 'two'],   v:completed_item[ 'user_data' ] )

  let s:called_completedone = 1
endfunc

func Test_CompleteDoneDictNoUserData()
  au CompleteDone * :call <SID>CompleteDone_CheckCompletedItemDictNoUserData()

  set completefunc=<SID>CompleteDone_CompleteFuncDictNoUserData
  execute "normal a\<C-X>\<C-U>\<C-Y>"
  set completefunc&

  call assert_equal(['one', 'two'], v:completed_item[ 'user_data' ])
  call assert_true(s:called_completedone)

  let s:called_completedone = 0
  au! CompleteDone
endfunc

func s:CompleteDone_CompleteFuncList(findstart, base)
  if a:findstart
    return 0
  endif

  return [ 'aword' ]
endfunc

func s:CompleteDone_CheckCompletedItemList()
  call assert_equal( 'aword', v:completed_item[ 'word' ] )
  call assert_equal( '',      v:completed_item[ 'abbr' ] )
  call assert_equal( '',      v:completed_item[ 'menu' ] )
  call assert_equal( '',      v:completed_item[ 'info' ] )
  call assert_equal( '',      v:completed_item[ 'kind' ] )
  call assert_equal( '',      v:completed_item[ 'user_data' ] )

  let s:called_completedone = 1
endfunc

func Test_CompleteDoneList()
  au CompleteDone * :call <SID>CompleteDone_CheckCompletedItemList()

  set completefunc=<SID>CompleteDone_CompleteFuncList
  execute "normal a\<C-X>\<C-U>\<C-Y>"
  set completefunc&

  call assert_equal('', v:completed_item[ 'user_data' ])
  call assert_true(s:called_completedone)

  let s:called_completedone = 0
  au! CompleteDone
endfunc

func Test_CompleteDone_undo()
  au CompleteDone * call append(0, "prepend1")
  new
  call setline(1, ["line1", "line2"])
  call feedkeys("Go\<C-X>\<C-N>\<CR>\<ESC>", "tx")
  call assert_equal(["prepend1", "line1", "line2", "line1", ""],
              \     getline(1, '$'))
  undo
  call assert_equal(["line1", "line2"], getline(1, '$'))
  bwipe!
  au! CompleteDone
endfunc

func CompleteTest(findstart, query)
  if a:findstart
    return col('.')
  endif
  return ['matched']
endfunc

func Test_completefunc_info()
  new
  set completeopt=menuone
  set completefunc=CompleteTest
  call feedkeys("i\<C-X>\<C-U>\<C-R>\<C-R>=string(complete_info())\<CR>\<ESC>", "tx")
  call assert_equal("matched{'pum_visible': 1, 'mode': 'function', 'selected': -1, 'items': [{'word': 'matched', 'menu': '', 'user_data': '', 'info': '', 'kind': '', 'abbr': ''}]}", getline(1))
  bwipe!
  set completeopt&
  set completefunc&
endfunc

" Check that when using feedkeys() typeahead does not interrupt searching for
" completions.
func Test_compl_feedkeys()
  new
  set completeopt=menuone,noselect
  call feedkeys("ajump ju\<C-X>\<C-N>\<C-P>\<ESC>", "tx")
  call assert_equal("jump jump", getline(1))
  bwipe!
  set completeopt&
endfunc

func Test_compl_in_cmdwin()
  set wildmenu wildchar=<Tab>
  com! -nargs=1 -complete=command GetInput let input = <q-args>
  com! -buffer TestCommand echo 'TestCommand'

  let input = ''
  call feedkeys("q:iGetInput T\<C-x>\<C-v>\<CR>", 'tx!')
  call assert_equal('TestCommand', input)

  let input = ''
  call feedkeys("q::GetInput T\<Tab>\<CR>:q\<CR>", 'tx!')
  call assert_equal('T', input)

  delcom TestCommand
  delcom GetInput
  set wildmenu& wildchar&
endfunc

" Test for insert path completion with completeslash option
func Test_ins_completeslash()
  CheckMSWindows

  call mkdir('Xdir')
  let orig_shellslash = &shellslash
  set cpt&
  new

  set noshellslash

  set completeslash=
  exe "normal oXd\<C-X>\<C-F>"
  call assert_equal('Xdir\', getline('.'))

  set completeslash=backslash
  exe "normal oXd\<C-X>\<C-F>"
  call assert_equal('Xdir\', getline('.'))

  set completeslash=slash
  exe "normal oXd\<C-X>\<C-F>"
  call assert_equal('Xdir/', getline('.'))

  set shellslash

  set completeslash=
  exe "normal oXd\<C-X>\<C-F>"
  call assert_equal('Xdir/', getline('.'))

  set completeslash=backslash
  exe "normal oXd\<C-X>\<C-F>"
  call assert_equal('Xdir\', getline('.'))

  set completeslash=slash
  exe "normal oXd\<C-X>\<C-F>"
  call assert_equal('Xdir/', getline('.'))
  %bw!
  call delete('Xdir', 'rf')

  set noshellslash
  set completeslash=slash
  call assert_true(stridx(globpath(&rtp, 'syntax/*.vim', 1, 1)[0], '\') != -1)

  let &shellslash = orig_shellslash
  set completeslash=
endfunc

func Test_pum_with_folds_two_tabs()
  CheckScreendump

  let lines =<< trim END
    set fdm=marker
    call setline(1, ['" x {{{1', '" a some text'])
    call setline(3, range(&lines)->map({_, val -> '" a' .. val}))
    norm! zm
    tab sp
    call feedkeys('2Gzv', 'xt')
    call feedkeys("0fa", 'xt')
  END

  call writefile(lines, 'Xpumscript')
  let buf = RunVimInTerminal('-S Xpumscript', #{rows: 10})
  call TermWait(buf, 50)
  call term_sendkeys(buf, "a\<C-N>")
  call VerifyScreenDump(buf, 'Test_pum_with_folds_two_tabs', {})

  call term_sendkeys(buf, "\<Esc>")
  call StopVimInTerminal(buf)
  call delete('Xpumscript')
endfunc

func Test_pum_with_preview_win()
  CheckScreendump

  let lines =<< trim END
      funct Omni_test(findstart, base)
	if a:findstart
	  return col(".") - 1
	endif
	return [#{word: "one", info: "1info"}, #{word: "two", info: "2info"}, #{word: "three", info: "3info"}]
      endfunc
      set omnifunc=Omni_test
      set completeopt+=longest
  END

  call writefile(lines, 'Xpreviewscript')
  let buf = RunVimInTerminal('-S Xpreviewscript', #{rows: 12})
  call TermWait(buf, 50)
  call term_sendkeys(buf, "Gi\<C-X>\<C-O>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, "\<C-N>")
  call VerifyScreenDump(buf, 'Test_pum_with_preview_win', {})

  call term_sendkeys(buf, "\<Esc>")
  call StopVimInTerminal(buf)
  call delete('Xpreviewscript')
endfunc

" Test for inserting the tag search pattern in insert mode
func Test_ins_compl_tag_sft()
  call writefile([
        \ "!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "first\tXfoo\t/^int first() {}$/",
        \ "second\tXfoo\t/^int second() {}$/",
        \ "third\tXfoo\t/^int third() {}$/"],
        \ 'Xtags')
  set tags=Xtags
  let code =<< trim [CODE]
    int first() {}
    int second() {}
    int third() {}
  [CODE]
  call writefile(code, 'Xfoo')

  enew
  set showfulltag
  exe "normal isec\<C-X>\<C-]>\<C-N>\<CR>"
  call assert_equal('int second() {}', getline(1))
  set noshowfulltag

  call delete('Xtags')
  call delete('Xfoo')
  set tags&
  %bwipe!
endfunc

" Test for 'completefunc' deleting text
func Test_completefunc_error()
  new
  " delete text when called for the first time
  func CompleteFunc(findstart, base)
    if a:findstart == 1
      normal dd
      return col('.') - 1
    endif
    return ['a', 'b']
  endfunc
  set completefunc=CompleteFunc
  call setline(1, ['', 'abcd', ''])
  call assert_fails('exe "normal 2G$a\<C-X>\<C-U>"', 'E840:')

  " delete text when called for the second time
  func CompleteFunc2(findstart, base)
    if a:findstart == 1
      return col('.') - 1
    endif
    normal dd
    return ['a', 'b']
  endfunc
  set completefunc=CompleteFunc2
  call setline(1, ['', 'abcd', ''])
  call assert_fails('exe "normal 2G$a\<C-X>\<C-U>"', 'E578:')

  " Jump to a different window from the complete function
  " TODO: The following test causes an ASAN failure. Once this issue is
  " addressed, enable the following test.
  "func! CompleteFunc(findstart, base)
  "  if a:findstart == 1
  "    return col('.') - 1
  "  endif
  "  wincmd p
  "  return ['a', 'b']
  "endfunc
  "set completefunc=CompleteFunc
  "new
  "call assert_fails('exe "normal a\<C-X>\<C-U>"', 'E839:')
  "close!

  set completefunc&
  delfunc CompleteFunc
  delfunc CompleteFunc2
  close!
endfunc

" Test for returning non-string values from 'completefunc'
func Test_completefunc_invalid_data()
  new
  func! CompleteFunc(findstart, base)
    if a:findstart == 1
      return col('.') - 1
    endif
    return [{}, '', 'moon']
  endfunc
  set completefunc=CompleteFunc
  exe "normal i\<C-X>\<C-U>"
  call assert_equal('moon', getline(1))
  set completefunc&
  close!
endfunc

" Test for errors in using complete() function
func Test_complete_func_error()
  call assert_fails('call complete(1, ["a"])', 'E785:')
  func ListColors()
    call complete(col('.'), "blue")
  endfunc
  call assert_fails('exe "normal i\<C-R>=ListColors()\<CR>"', 'E474:')
  func ListMonths()
    call complete(col('.'), test_null_list())
  endfunc
  call assert_fails('exe "normal i\<C-R>=ListMonths()\<CR>"', 'E474:')
  delfunc ListColors
  delfunc ListMonths
  call assert_fails('call complete_info({})', 'E714:')
  call assert_equal([], complete_info(['items']).items)
endfunc

" Test for completing words following a completed word in a line
func Test_complete_wrapscan()
  " complete words from another buffer
  new
  call setline(1, ['one two', 'three four'])
  new
  setlocal complete=w
  call feedkeys("itw\<C-N>\<C-X>\<C-N>\<C-X>\<C-N>\<C-X>\<C-N>", 'xt')
  call assert_equal('two three four', getline(1))
  close!
  " complete words from the current buffer
  setlocal complete=.
  %d
  call setline(1, ['one two', ''])
  call cursor(2, 1)
  call feedkeys("ion\<C-N>\<C-X>\<C-N>\<C-X>\<C-N>\<C-X>\<C-N>", 'xt')
  call assert_equal('one two one two', getline(2))
  close!
endfunc

" Test for completing special characters
func Test_complete_special_chars()
  new
  call setline(1, 'int .*[-\^$ func float')
  call feedkeys("oin\<C-X>\<C-P>\<C-X>\<C-P>\<C-X>\<C-P>", 'xt')
  call assert_equal('int .*[-\^$ func float', getline(2))
  close!
endfunc

" Test for completion when text is wrapped across lines.
func Test_complete_across_line()
  new
  call setline(1, ['red green blue', 'one two three'])
  setlocal textwidth=20
  exe "normal 2G$a re\<C-X>\<C-P>\<C-X>\<C-P>\<C-X>\<C-P>\<C-X>\<C-P>"
  call assert_equal(['one two three red', 'green blue one'], getline(2, '$'))
  close!
endfunc

" Test for using CTRL-L to add one character when completing matching
func Test_complete_add_onechar()
  new
  call setline(1, ['wool', 'woodwork'])
  call feedkeys("Gowoo\<C-P>\<C-P>\<C-P>\<C-L>f", 'xt')
  call assert_equal('woof', getline(3))

  " use 'ignorecase' and backspace to erase characters from the prefix string
  " and then add letters using CTRL-L
  %d
  set ignorecase backspace=2
  setlocal complete=.
  call setline(1, ['workhorse', 'workload'])
  normal Go
  exe "normal aWOR\<C-P>\<bs>\<bs>\<bs>\<bs>\<bs>\<bs>\<C-L>r\<C-L>\<C-L>"
  call assert_equal('workh', getline(3))
  set ignorecase& backspace&
  close!
endfunc

" Test insert completion with 'cindent' (adjust the indent)
func Test_complete_with_cindent()
  new
  setlocal cindent
  call setline(1, ['if (i == 1)', "    j = 2;"])
  exe "normal Go{\<CR>i\<C-X>\<C-L>\<C-X>\<C-L>\<CR>}"
  call assert_equal(['{', "\tif (i == 1)", "\t\tj = 2;", '}'], getline(3, '$'))

  %d
  call setline(1, ['when while', '{', ''])
  setlocal cinkeys+==while
  exe "normal Giwh\<C-P> "
  call assert_equal("\twhile ", getline('$'))
  close!
endfunc

" Test for <CTRL-X> <CTRL-V> completion. Complete commands and functions
func Test_complete_cmdline()
  new
  exe "normal icaddb\<C-X>\<C-V>"
  call assert_equal('caddbuffer', getline(1))
  exe "normal ocall getqf\<C-X>\<C-V>"
  call assert_equal('call getqflist(', getline(2))
  exe "normal oabcxyz(\<C-X>\<C-V>"
  call assert_equal('abcxyz(', getline(3))
  close!
endfunc

func Test_issue_7021()
  CheckMSWindows

  let orig_shellslash = &shellslash
  set noshellslash

  set completeslash=slash
  call assert_false(expand('~') =~ '/')

  let &shellslash = orig_shellslash
  set completeslash=
endfunc

" vim: shiftwidth=2 sts=2 expandtab
