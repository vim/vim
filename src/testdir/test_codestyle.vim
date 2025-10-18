vim9script
# Tests for checking the source code style.

var list_of_c_files = []

def ReportError(fname: string, lnum: number, msg: string)
  if lnum > 0
    assert_report(fname .. ' line ' .. lnum .. ': ' .. msg)
  endif
enddef

def PerformCheck(fname: string, pattern: string, msg: string,
  Skip: func(string, string): bool = (_, _) => false)

  var prev_lnum = 1
  var lnum = 1
  while (lnum > 0)
    cursor(lnum, 1)
    lnum = search(pattern, 'W', 0, 0, () => Skip(fname, getline('.')))
    if (prev_lnum == lnum)
      break
    endif
    prev_lnum = lnum
    if (lnum > 0)
      ReportError(fname, lnum, msg)
    endif
  endwhile
enddef

def Get_C_source_files(): list<string>
  if empty(list_of_c_files)
    var list = glob('../*.[ch]', 0, 1) + ['../xxd/xxd.c']
    # Some files are auto-generated and may contain space errors, so skip those
    list_of_c_files = filter(list, (i, v) => v !~ 'dlldata.c\|if_ole.h\|iid_ole.c')
  endif
  return list_of_c_files
enddef

def g:Test_source_files()
  var Skip: func(string, string): bool

  for fname in Get_C_source_files()
    bwipe!
    g:ignoreSwapExists = 'e'
    exe 'edit ' .. fname

    PerformCheck(fname, ' \t', 'space before Tab')

    PerformCheck(fname, '\s$', 'trailing white space')

    # some files don't stick to the Vim style rules
    if fname =~ 'iscygpty.c'
      continue
    endif

    Skip = (_: string, line: string) =>
      line =~ 'condition) {'
        || line =~ 'vimglob_func'
        || line =~ '{"'
        || line =~ '{\d'
        || line =~ '{{{'

    PerformCheck(fname, ')\s*{', 'curly after closing paren', Skip)

    # Examples in comments use double quotes.
    # skip = "getline('.') =~ '\"'"
    Skip = (_: string, line: string) => line =~ '"'

    PerformCheck(fname, '}\s*else', 'curly before "else"', Skip)

    PerformCheck(fname, 'else\s*{', 'curly after "else"', Skip)

    PerformCheck(fname, '\<\(if\|while\|for\)(', 'missing white space after "if"/"while"/"for"', Skip)
  endfor

  bwipe!
enddef

def g:Test_test_files()
  for fname in glob('*.vim', 0, 1)
    g:ignoreSwapExists = 'e'
    exe 'edit ' .. fname

    # some files intentionally have misplaced white space
    if fname =~ 'test_cindent.vim' || fname =~ 'test_join.vim'
      continue
    endif

    # skip files that are known to have a space before a tab
    if fname !~ 'test_comments.vim'
        && fname !~ 'test_listchars.vim'
        && fname !~ 'test_visual.vim'
      cursor(1, 1)
      var skip = 'getline(".") =~ "codestyle: ignore"'
      var lnum = search(fname =~ 'test_regexp_latin' ? '[^á] \t' : ' \t', 'W', 0, 0, skip)
      ReportError('testdir/' .. fname, lnum, 'space before Tab')
    endif

    # skip files that are known to have trailing white space
    if fname !~ 'test_cmdline.vim'
            && fname !~ 'test_let.vim'
            && fname !~ 'test_tagjump.vim'
            && fname !~ 'test_vim9_cmd.vim'
            && fname !~ 'test_vim9_enum.vim'
      cursor(1, 1)
      var lnum = search(
          fname =~ 'test_vim9_assign.vim' ? '[^=]\s$'
          : fname =~ 'test_vim9_class.vim' ? '[^)]\s$'
          : fname =~ 'test_vim9_script.vim' ? '[^,:3]\s$'
          : fname =~ 'test_visual.vim' ? '[^/]\s$'
          : '[^\\]\s$')
      ReportError('testdir/' .. fname, lnum, 'trailing white space')
    endif
  endfor

  bwipe!
enddef

def g:Test_help_files()
  var Skip: func(string, string): bool

  for fpath in glob('../../runtime/doc/*.txt', 0, 1)
    g:ignoreSwapExists = 'e'
    exe 'edit ' .. fpath

    var fname = fnamemodify(fpath, ':t')

    # todo.txt is for developers, it's not need a strictly check
    # version*.txt is a history and large size, so it's not checked
    if fname == 'todo.txt' || fname =~ 'version.*\.txt'
      continue
    endif

    Skip = (file: string, line: string): bool =>
      # :help v_b_I_example
      file == 'visual.txt' && line =~# 'STRING  \tjkl'
      # :help 12.7
      || file == 'usr_12.txt' && line =~ '/ \t'
      # :help 27.6
      || file == 'usr_27.txt' && line =~ '\[^\? \t\]'

    PerformCheck(fname, ' \t', 'space before Tab', Skip)

    Skip = (file: string, line: string): bool =>
      # :help registers (two lines)
      file == 'change.txt' && line =~# 'drop registers "\*, "+ and "\~ $'
      # :help definitions (two lines)
      || file == 'intro.txt' && line =~# '\%(6\|11\). \~ $'
      # :help map-trailing-white
      || file == 'map.txt' && line =~# 'unmap @@ $'
      # :help 'list' and :help 'showbreak'
      || file == 'options.txt' && line =~# ':set \%(list lcs=tab:\\ \\\|showbreak=>\\\) $'
      # :help 12.7
      || file == 'usr_12.txt' && line =~ '^\t/ \t$'
      # :help 41.11
      || file == 'usr_41.txt' && line =~# 'map <F4> o#include  $'
      # :help autoformat
      || file == 'change.txt' && line =~# 'foobar bla $'

    PerformCheck(fname, '\s$', 'trailing white space', Skip)

    # TODO: Check for line over 78 columns
    # Skip = (_, line: string): bool =>
    #   synIDattr(synID(line('.'), col('.'), 1), 'name') =~# 'helpExample' || line =~# 'https\=://'
    # PerformCheck(fname, '\%>78v.', 'line over 78 columns', Skip)

  endfor

  bwipe!
enddef

def g:Test_indent_of_source_files()
  for fname in Get_C_source_files()
    execute 'tabnew ' .. fname
    if &expandtab
      continue
    endif
    for lnum in range(1, line('$'))
      var name: string = synIDattr(synID(lnum, 1, 0), 'name')
      if -1 == index(['cComment', 'cCommentStart'], name)
        var line: string = getline(lnum)
        var indent: string = matchstr(line, '^\s*')
        var tailing: string = matchstr(line, '\s*$')
        if !empty(indent)
          if indent !~# '^\t* \{0,7}$'
            ReportError('testdir/' .. fname, lnum, 'invalid indent')
          endif
        endif
        if !empty(tailing)
          ReportError('testdir/' .. fname, lnum, 'tailing spaces')
        endif
      endif
    endfor
    close
  endfor
enddef

# vim: shiftwidth=2 sts=2 expandtab nofoldenable
