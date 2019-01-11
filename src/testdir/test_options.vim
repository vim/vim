" Test for options

func Test_whichwrap()
  set whichwrap=b,s
  call assert_equal('b,s', &whichwrap)

  set whichwrap+=h,l
  call assert_equal('b,s,h,l', &whichwrap)

  set whichwrap+=h,l
  call assert_equal('b,s,h,l', &whichwrap)

  set whichwrap+=h,l
  call assert_equal('b,s,h,l', &whichwrap)

  set whichwrap=h,h
  call assert_equal('h', &whichwrap)

  set whichwrap=h,h,h
  call assert_equal('h', &whichwrap)

  set whichwrap&
endfunc

func Test_isfname()
  " This used to cause Vim to access uninitialized memory.
  set isfname=
  call assert_equal("~X", expand("~X"))
  set isfname&
endfunc

func Test_wildchar()
  " Empty 'wildchar' used to access invalid memory.
  call assert_fails('set wildchar=', 'E521:')
  call assert_fails('set wildchar=abc', 'E521:')
  set wildchar=<Esc>
  let a=execute('set wildchar?')
  call assert_equal("\n  wildchar=<Esc>", a)
  set wildchar=27
  let a=execute('set wildchar?')
  call assert_equal("\n  wildchar=<Esc>", a)
  set wildchar&
endfunc

func Test_options()
  let caught = 'ok'
  try
    options
  catch
    let caught = v:throwpoint . "\n" . v:exception
  endtry
  call assert_equal('ok', caught)

  " close option-window
  close
endfunc

func Test_path_keep_commas()
  " Test that changing 'path' keeps two commas.
  set path=foo,,bar
  set path-=bar
  set path+=bar
  call assert_equal('foo,,bar', &path)

  set path&
endfunc

func Test_signcolumn()
  if has('signs')
    call assert_equal("auto", &signcolumn)
    set signcolumn=yes
    set signcolumn=no
    call assert_fails('set signcolumn=nope')
  endif
endfunc

func Test_filetype_valid()
  if !has('autocmd')
    return
  endif
  set ft=valid_name
  call assert_equal("valid_name", &filetype)
  set ft=valid-name
  call assert_equal("valid-name", &filetype)

  call assert_fails(":set ft=wrong;name", "E474:")
  call assert_fails(":set ft=wrong\\\\name", "E474:")
  call assert_fails(":set ft=wrong\\|name", "E474:")
  call assert_fails(":set ft=wrong/name", "E474:")
  call assert_fails(":set ft=wrong\\\nname", "E474:")
  call assert_equal("valid-name", &filetype)

  exe "set ft=trunc\x00name"
  call assert_equal("trunc", &filetype)
endfunc

func Test_syntax_valid()
  if !has('syntax')
    return
  endif
  set syn=valid_name
  call assert_equal("valid_name", &syntax)
  set syn=valid-name
  call assert_equal("valid-name", &syntax)

  call assert_fails(":set syn=wrong;name", "E474:")
  call assert_fails(":set syn=wrong\\\\name", "E474:")
  call assert_fails(":set syn=wrong\\|name", "E474:")
  call assert_fails(":set syn=wrong/name", "E474:")
  call assert_fails(":set syn=wrong\\\nname", "E474:")
  call assert_equal("valid-name", &syntax)

  exe "set syn=trunc\x00name"
  call assert_equal("trunc", &syntax)
endfunc

func Test_keymap_valid()
  if !has('keymap')
    return
  endif
  call assert_fails(":set kmp=valid_name", "E544:")
  call assert_fails(":set kmp=valid_name", "valid_name")
  call assert_fails(":set kmp=valid-name", "E544:")
  call assert_fails(":set kmp=valid-name", "valid-name")

  call assert_fails(":set kmp=wrong;name", "E474:")
  call assert_fails(":set kmp=wrong\\\\name", "E474:")
  call assert_fails(":set kmp=wrong\\|name", "E474:")
  call assert_fails(":set kmp=wrong/name", "E474:")
  call assert_fails(":set kmp=wrong\\\nname", "E474:")

  call assert_fails(":set kmp=trunc\x00name", "E544:")
  call assert_fails(":set kmp=trunc\x00name", "trunc")
endfunc

func Check_dir_option(name)
  " Check that it's possible to set the option.
  exe 'set ' . a:name . '=/usr/share/dict/words'
  call assert_equal('/usr/share/dict/words', eval('&' . a:name))
  exe 'set ' . a:name . '=/usr/share/dict/words,/and/there'
  call assert_equal('/usr/share/dict/words,/and/there', eval('&' . a:name))
  exe 'set ' . a:name . '=/usr/share/dict\ words'
  call assert_equal('/usr/share/dict words', eval('&' . a:name))

  " Check rejecting weird characters.
  call assert_fails("set " . a:name . "=/not&there", "E474:")
  call assert_fails("set " . a:name . "=/not>there", "E474:")
  call assert_fails("set " . a:name . "=/not.*there", "E474:")
endfunc

func Test_cinkeys()
  " This used to cause invalid memory access
  set cindent cinkeys=0
  norm a
  set cindent& cinkeys&
endfunc

func Test_dictionary()
  call Check_dir_option('dictionary')
endfunc

func Test_thesaurus()
  call Check_dir_option('thesaurus')
endfun

func Test_complete()
  " Trailing single backslash used to cause invalid memory access.
  set complete=s\
  new
  call feedkeys("i\<C-N>\<Esc>", 'xt')
  bwipe!
  set complete&
endfun

func Test_set_completion()
  call feedkeys(":set di\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set dictionary diff diffexpr diffopt digraph directory display', @:)

  " Expand boolan options. When doing :set no<Tab>
  " vim displays the options names without "no" but completion uses "no...".
  call feedkeys(":set nodi\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set nodiff digraph', @:)

  call feedkeys(":set invdi\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set invdiff digraph', @:)

  " Expand abbreviation of options.
  call feedkeys(":set ts\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set tabstop thesaurus ttyscroll', @:)

  " Expand current value
  call feedkeys(":set fileencodings=\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set fileencodings=ucs-bom,utf-8,default,latin1', @:)

  call feedkeys(":set fileencodings:\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set fileencodings:ucs-bom,utf-8,default,latin1', @:)

  " Expand key codes.
  call feedkeys(":set <H\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set <Help> <Home>', @:)

  " Expand terminal options.
  call feedkeys(":set t_A\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set t_AB t_AF t_AL', @:)

  " Expand directories.
  call feedkeys(":set cdpath=./\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_match(' ./samples/ ', @:)
  call assert_notmatch(' ./small.vim ', @:)

  " Expand files and directories.
  call feedkeys(":set tags=./\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_match(' ./samples/.* ./small.vim', @:)

  call feedkeys(":set tags=./\\\\ dif\<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"set tags=./\\ diff diffexpr diffopt', @:)
endfunc

func Test_set_errors()
  call assert_fails('set scroll=-1', 'E49:')
  call assert_fails('set backupcopy=', 'E474:')
  call assert_fails('set regexpengine=3', 'E474:')
  call assert_fails('set history=10001', 'E474:')
  call assert_fails('set numberwidth=11', 'E474:')
  call assert_fails('set colorcolumn=-a')
  call assert_fails('set colorcolumn=a')
  call assert_fails('set colorcolumn=1,')
  call assert_fails('set cmdheight=-1', 'E487:')
  call assert_fails('set cmdwinheight=-1', 'E487:')
  if has('conceal')
    call assert_fails('set conceallevel=-1', 'E487:')
    call assert_fails('set conceallevel=4', 'E474:')
  endif
  call assert_fails('set helpheight=-1', 'E487:')
  call assert_fails('set history=-1', 'E487:')
  call assert_fails('set report=-1', 'E487:')
  call assert_fails('set shiftwidth=-1', 'E487:')
  call assert_fails('set sidescroll=-1', 'E487:')
  call assert_fails('set tabstop=-1', 'E487:')
  call assert_fails('set textwidth=-1', 'E487:')
  call assert_fails('set timeoutlen=-1', 'E487:')
  call assert_fails('set updatecount=-1', 'E487:')
  call assert_fails('set updatetime=-1', 'E487:')
  call assert_fails('set winheight=-1', 'E487:')
  call assert_fails('set tabstop!', 'E488:')
  call assert_fails('set xxx', 'E518:')
  call assert_fails('set beautify?', 'E519:')
  call assert_fails('set undolevels=x', 'E521:')
  call assert_fails('set tabstop=', 'E521:')
  call assert_fails('set comments=-', 'E524:')
  call assert_fails('set comments=a', 'E525:')
  call assert_fails('set foldmarker=x', 'E536:')
  call assert_fails('set commentstring=x', 'E537:')
  call assert_fails('set complete=x', 'E539:')
  call assert_fails('set statusline=%{', 'E540:')
  call assert_fails('set statusline=' . repeat("%p", 81), 'E541:')
  call assert_fails('set statusline=%(', 'E542:')
  if has('cursorshape')
    " This invalid value for 'guicursor' used to cause Vim to crash.
    call assert_fails('set guicursor=i-ci,r-cr:h', 'E545:')
    call assert_fails('set guicursor=i-ci', 'E545:')
    call assert_fails('set guicursor=x', 'E545:')
    call assert_fails('set guicursor=r-cr:horx', 'E548:')
    call assert_fails('set guicursor=r-cr:hor0', 'E549:')
  endif
  call assert_fails('set backupext=~ patchmode=~', 'E589:')
  call assert_fails('set winminheight=10 winheight=9', 'E591:')
  call assert_fails('set winminwidth=10 winwidth=9', 'E592:')
  call assert_fails("set showbreak=\x01", 'E595:')
  call assert_fails('set t_foo=', 'E846:')
endfunc

" Must be executed before other tests that set 'term'.
func Test_000_term_option_verbose()
  if has('gui_running')
    return
  endif
  let verb_cm = execute('verbose set t_cm')
  call assert_notmatch('Last set from', verb_cm)

  let term_save = &term
  set term=ansi
  let verb_cm = execute('verbose set t_cm')
  call assert_match('Last set from.*test_options.vim', verb_cm)
  let &term = term_save
endfunc

func Test_set_ttytype()
  if !has('gui_running') && has('unix')
    " Setting 'ttytype' used to cause a double-free when exiting vim and
    " when vim is compiled with -DEXITFREE.
    set ttytype=ansi
    call assert_equal('ansi', &ttytype)
    call assert_equal(&ttytype, &term)
    set ttytype=xterm
    call assert_equal('xterm', &ttytype)
    call assert_equal(&ttytype, &term)
    " "set ttytype=" gives E522 instead of E529
    " in travis on some builds. Why?  Catch both for now
    try
      set ttytype=
      call assert_report('set ttytype= did not fail')
    catch /E529\|E522/
    endtry

    " Some systems accept any terminal name and return dumb settings,
    " check for failure of finding the entry and for missing 'cm' entry.
    try
      set ttytype=xxx
      call assert_report('set ttytype=xxx did not fail')
    catch /E522\|E437/
    endtry

    set ttytype&
    call assert_equal(&ttytype, &term)
  endif
endfunc

func Test_set_all()
  set tw=75
  set iskeyword=a-z,A-Z
  set nosplitbelow
  let out = execute('set all')
  call assert_match('textwidth=75', out)
  call assert_match('iskeyword=a-z,A-Z', out)
  call assert_match('nosplitbelow', out)
  set tw& iskeyword& splitbelow&
endfunc

func Test_set_values()
  if filereadable('opt_test.vim')
    source opt_test.vim
  else
    throw 'Skipped: opt_test.vim does not exist'
  endif
endfunc

func ResetIndentexpr()
  set indentexpr=
endfunc

func Test_set_indentexpr()
  " this was causing usage of freed memory
  set indentexpr=ResetIndentexpr()
  new
  call feedkeys("i\<c-f>", 'x')
  call assert_equal('', &indentexpr)
  bwipe!
endfunc

func Test_backupskip()
  if has("mac")
    call assert_match('/private/tmp/\*', &bsk)
  elseif has("unix")
    call assert_match('/tmp/\*', &bsk)
  endif

  let bskvalue = substitute(&bsk, '\\', '/', 'g')
  for var in  ['$TEMPDIR', '$TMP', '$TEMP']
    if exists(var)
      let varvalue = substitute(expand(var), '\\', '/', 'g')
      call assert_match(varvalue . '/\=\*', bskvalue)
    endif
  endfor
endfunc

func Test_copy_winopt()
  set hidden

  " Test copy option from current buffer in window
  split
  enew
  setlocal numberwidth=5
  wincmd w
  call assert_equal(4,&numberwidth)
  bnext
  call assert_equal(5,&numberwidth)
  bw!
  call assert_equal(4,&numberwidth)

  " Test copy value from window that used to be display the buffer
  split
  enew
  setlocal numberwidth=6
  bnext
  wincmd w
  call assert_equal(4,&numberwidth)
  bnext
  call assert_equal(6,&numberwidth)
  bw!

  " Test that if buffer is current, don't use the stale cached value
  " from the last time the buffer was displayed.
  split
  enew
  setlocal numberwidth=7
  bnext
  bnext
  setlocal numberwidth=8
  wincmd w
  call assert_equal(4,&numberwidth)
  bnext
  call assert_equal(8,&numberwidth)
  bw!

  " Test value is not copied if window already has seen the buffer
  enew
  split
  setlocal numberwidth=9
  bnext
  setlocal numberwidth=10
  wincmd w
  call assert_equal(4,&numberwidth)
  bnext
  call assert_equal(4,&numberwidth)
  bw!

  set hidden&
endfunc

func Test_shortmess_F()
  new
  call assert_match('\[No Name\]', execute('file'))
  set shortmess+=F
  call assert_match('\[No Name\]', execute('file'))
  call assert_match('^\s*$', execute('file foo'))
  call assert_match('foo', execute('file'))
  set shortmess-=F
  call assert_match('bar', execute('file bar'))
  call assert_match('bar', execute('file'))
  set shortmess&
  bwipe
endfunc

func Test_shortmess_F2()
  e file1
  e file2
  call assert_match('file1', execute('bn', ''))
  call assert_match('file2', execute('bn', ''))
  set shortmess+=F
  call assert_true(empty(execute('bn', '')))
  call assert_true(empty(execute('bn', '')))
  set hidden
  call assert_true(empty(execute('bn', '')))
  call assert_true(empty(execute('bn', '')))
  set nohidden
  call assert_true(empty(execute('bn', '')))
  call assert_true(empty(execute('bn', '')))
  set shortmess&
  call assert_match('file1', execute('bn', ''))
  call assert_match('file2', execute('bn', ''))
  bwipe
  bwipe
endfunc
