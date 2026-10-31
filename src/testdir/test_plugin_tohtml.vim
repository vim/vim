" Tests for Vim :TOhtml

func s:setup_basic(src_name)
  let lines =<< trim END
    #include <stdio.h>
    #include <stdlib.h>

    int isprime(int n)
    {
      if (n <= 1)
        return 0;

      for (int i = 2; i <= n / 2; i++)
        if (n % i == 0)
          return 0;

      return 1;
    }

    int main(int argc, char *argv[])
    {
      int n = 7;

      printf("%d is %s prime\n", n, isprime(n) ? "a" : "not a");

      return 0;
    }
  END
  call writefile(lines, a:src_name)
  exe 'edit ' . a:src_name
  TOhtml
  write
endfunc

func s:cleanup_basic(src_name)
  call delete(a:src_name)
  call delete(a:src_name . ".html")
endfunc

source $VIMRUNTIME/plugin/tohtml.vim

func Test_tohtml_basic()
  let src_name = "Test_tohtml_basic.c"
  call s:setup_basic(src_name)
  let expected = readfile("samples/" . src_name . ".html")
  let actual = readfile(src_name . ".html")
  call assert_equal(expected[0:3], actual[0:3])
  " Ignore the title
  call assert_equal(expected[5:11], actual[5:11])
  " Ignore pre and body css
  call assert_equal(expected[14:], actual[14:])
  call s:cleanup_basic(src_name)
endfunc

func Test_tohtml_basic_no_css()
  let g:html_use_css = 0
  let src_name = "Test_tohtml_basic_no_css.c"
  call s:setup_basic(src_name)
  let expected = readfile("samples/" . src_name . ".html")
  let actual = readfile(src_name . ".html")
  call assert_equal(expected[0:3], actual[0:3])
  " Ignore the title
  call assert_equal(expected[5:10], actual[5:10])
  " Ignore body's inline css
  call assert_equal(expected[12:], actual[12:])
  call s:cleanup_basic(src_name)
  unlet g:html_use_css
endfunc

" Return the converted lines between <pre> and </pre> in the current buffer.
func s:get_pre_lines()
  let lines = getline(1, '$')
  return lines[index(lines, "<pre id='vimCodeElement'>") + 1 :
        \ index(lines, '</pre>') - 1]
endfunc

func s:setup_textprop()
  new
  call setline(1, ['foo bar baz', 'one two', 'three'])
  syn match TestSyn /bar\|two/
  hi TestSyn cterm=italic gui=italic
  hi TestProp cterm=bold gui=bold
  hi TestPropHigh cterm=underline gui=underline
  call prop_type_add('test_prop',
        \ #{bufnr: bufnr(), highlight: 'TestProp'})
  call prop_type_add('test_high',
        \ #{bufnr: bufnr(), highlight: 'TestPropHigh', priority: 10})
  call prop_type_add('test_nocomb',
        \ #{bufnr: bufnr(), highlight: 'TestProp', combine: v:false})
  " overlapping properties, partly on a syntax item
  call prop_add(1, 1, #{length: 7, type: 'test_prop'})
  call prop_add(1, 5, #{length: 7, type: 'test_high'})
  " replaces the syntax highlighting, continues in the next line
  call prop_add(2, 5, #{end_lnum: 3, end_col: 3, type: 'test_nocomb'})
  let g:html_no_progress = 1
endfunc

func s:cleanup_textprop()
  unlet g:html_no_progress
  bwipe!
  bwipe!
  hi clear TestSyn
  hi clear TestProp
  hi clear TestPropHigh
endfunc

func Test_tohtml_textprop()
  CheckFeature textprop
  call s:setup_textprop()
  TOhtml
  let expected = [
        \ '<span class="TestProp">foo </span>'
        \ .. '<span class="TestSyn"><span class="TestProp">'
        \ .. '<span class="TestPropHigh">bar</span></span></span>'
        \ .. '<span class="TestPropHigh"> baz</span>',
        \ 'one <span class="TestProp">two</span>',
        \ '<span class="TestProp">th</span>ree',
        \ ]
  call assert_equal(expected, s:get_pre_lines())
  call s:cleanup_textprop()
endfunc

func Test_tohtml_textprop_no_css()
  CheckFeature textprop
  call s:setup_textprop()
  let g:html_use_css = 0
  TOhtml
  let actual = getline(1, '$')
  let idx = match(actual, '^<font face=')
  let expected = [
        \ '<b>foo&nbsp;</b><i><b><u>bar</u></b></i><u>&nbsp;baz</u><br>',
        \ 'one&nbsp;<b>two</b><br>',
        \ '<b>th</b>ree<br>',
        \ ]
  call assert_equal(expected, actual[idx + 1 : idx + 3])
  unlet g:html_use_css
  call s:cleanup_textprop()
endfunc

func Test_tohtml_ignore_textprop()
  CheckFeature textprop
  call s:setup_textprop()
  let g:html_ignore_textprop = 1
  TOhtml
  let expected = [
        \ 'foo <span class="TestSyn">bar</span> baz',
        \ 'one <span class="TestSyn">two</span>',
        \ 'three',
        \ ]
  call assert_equal(expected, s:get_pre_lines())
  unlet g:html_ignore_textprop
  call s:cleanup_textprop()
endfunc

func s:setup_virtual_text()
  new
  call setline(1, ['let x = 1', "a\tb", ''])
  hi TestProp cterm=bold gui=bold
  call prop_type_add('test_prop', #{bufnr: bufnr(), highlight: 'TestProp'})
  call prop_add(1, 5, #{type: 'test_prop', text: ': num'})
  call prop_add(1, 0,
        \ #{type: 'test_prop', text: 'after', text_padding_left: 2})
  call prop_add(1, 0, #{type: 'test_prop', text: 'R', text_align: 'right'})
  call prop_add(1, 0, #{type: 'test_prop', text: 'above', text_align: 'above'})
  call prop_add(1, 0,
        \ #{type: 'test_prop', text: 'below <b>', text_align: 'below'})
  " the tab is displayed after the virtual text
  call prop_add(2, 2, #{type: 'test_prop', text: 'XYZ'})
  call prop_add(3, 0, #{type: 'test_prop', text: 'empty'})
  let g:html_no_progress = 1
endfunc

func Test_tohtml_virtual_text()
  CheckFeature textprop
  call s:setup_virtual_text()
  let g:html_number_lines = 1
  TOhtml
  let expected = [
        \ '<span data-vtext-content="  " class="LineNr"></span>'
        \ .. '<span data-vtext-content="above" class="TestProp"></span>'
        \ .. '<span data-vtext-content="&#0010;"></span>'
        \ .. '<span id="L1" class="LineNr">1 </span>let '
        \ .. '<span data-vtext-content="&#0058; num" class="TestProp"></span>'
        \ .. 'x = 1<span data-vtext-content="  "></span>'
        \ .. '<span data-vtext-content="after" class="TestProp"></span>'
        \ .. '<span data-vtext-content="' .. repeat(' ', &columns - 24)
        \ .. '"></span><span data-vtext-content="R" class="TestProp"></span>'
        \ .. '<span data-vtext-content="&#0010;"></span>'
        \ .. '<span data-vtext-content="  " class="LineNr"></span>'
        \ .. '<span data-vtext-content="below &lt;b&gt;" class="TestProp">'
        \ .. '</span>',
        \ '<span id="L2" class="LineNr">2 </span>a'
        \ .. '<span data-vtext-content="XYZ" class="TestProp"></span>    b',
        \ '<span id="L3" class="LineNr">3 </span>'
        \ .. '<span data-vtext-content="empty" class="TestProp"></span>',
        \ ]
  call assert_equal(expected, s:get_pre_lines())
  call assert_notequal(-1,
        \ index(getline(1, '$'), '[data-vtext-content] { user-select: none; }'))
  unlet g:html_number_lines
  call s:cleanup_textprop()
endfunc

" Without a style sheet virtual text is normal text.
func Test_tohtml_virtual_text_no_css()
  CheckFeature textprop
  call s:setup_virtual_text()
  let g:html_use_css = 0
  TOhtml
  let actual = getline(1, '$')
  let idx = match(actual, '^<font face=')
  let expected = [
        \ '<b>above</b><br>',
        \ 'let&nbsp;<b>: num</b>x = 1&nbsp;&nbsp;<b>after</b><b>R</b><br>',
        \ '<b>below &lt;b&gt;</b><br>',
        \ 'a<b>XYZ</b>&nbsp;&nbsp;&nbsp;&nbsp;b<br>',
        \ '<b>empty</b><br>',
        \ ]
  call assert_equal(expected, actual[idx + 1 : idx + 5])
  unlet g:html_use_css
  call s:cleanup_textprop()
endfunc

" The diff highlighting is applied on top of the text property highlighting.
func Test_tohtml_textprop_diff()
  CheckFeature textprop
  CheckFeature diff
  new
  call setline(1, ['aaa bbb', 'same'])
  diffthis
  vnew
  call setline(1, ['aaa ccc', 'same'])
  diffthis
  hi TestProp cterm=bold gui=bold
  call prop_type_add('test_prop', #{bufnr: bufnr(), highlight: 'TestProp'})
  call prop_add(1, 1, #{length: 6, type: 'test_prop'})
  let g:html_no_progress = 1
  TOhtml
  let lines = getline(1, '$')
  let idx = match(lines, '^<span class="TestProp">')
  call assert_match('^<span class="TestProp"><span class="DiffChange">'
        \ .. 'aaa </span></span><span class="TestProp">'
        \ .. '<span class="DiffText">cc</span></span>'
        \ .. '<span class="DiffText">c</span>', lines[idx])
  unlet g:html_no_progress
  %bwipe!
  hi clear TestProp
endfunc

" With "override" the text property highlighting is applied again on top of
" the diff highlighting.
func Test_tohtml_textprop_diff_override()
  CheckFeature textprop
  CheckFeature diff
  new
  call setline(1, ['aaa bbb', 'same'])
  diffthis
  vnew
  call setline(1, ['aaa ccc', 'same'])
  diffthis
  hi TestProp cterm=bold gui=bold
  call prop_type_add('test_prop',
        \ #{bufnr: bufnr(), highlight: 'TestProp', override: v:true})
  call prop_add(1, 1, #{length: 6, type: 'test_prop'})
  let g:html_no_progress = 1
  TOhtml
  let lines = getline(1, '$')
  let idx = match(lines, '^<span class="TestProp">')
  call assert_match('^<span class="TestProp"><span class="DiffChange">'
        \ .. '<span class="TestProp">aaa </span></span></span>'
        \ .. '<span class="TestProp"><span class="DiffText">'
        \ .. '<span class="TestProp">cc</span></span></span>'
        \ .. '<span class="DiffText">c</span>', lines[idx])
  unlet g:html_no_progress
  %bwipe!
  hi clear TestProp
endfunc

" vim: shiftwidth=2 sts=2 expandtab
