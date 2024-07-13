" Tests for Vim :TOhtml

source check.vim

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

" vim: shiftwidth=2 sts=2 expandtab
