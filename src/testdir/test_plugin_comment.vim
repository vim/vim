" Test for the comment package

packadd comment

func Test_basic_comment()
  let lines =<< trim END
    vim9script

    def Hello()
      echo "Hello"
    enddef
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=vim

  normal gcc
  normal 2jgcip

  let result = getline(1, '$')
  call assert_equal(["# vim9script", "", "# def Hello()", '#   echo "Hello"', "# enddef"], result)
endfunc

func Test_basic_uncomment()
  let lines =<< trim END
    vim9script

    # def Hello()
    #   echo "Hello"
    # enddef
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=vim

  normal gcc
  normal 2jgcip

  let result = getline(1, '$')
  call assert_equal(["# vim9script", "", "def Hello()", '  echo "Hello"', "enddef"], result)
endfunc

func Test_backward_slash_uncomment()
  " Note this test depends on 'commentstring' setting in nroff ftplugin
  let lines =<< trim END
    .\" .TL Test backward slash uncomment
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=nroff

  normal gcc

  let result = getline(1, '$')
  call assert_equal([".TL Test backward slash uncomment"], result)
endfunc

func Test_caseinsensitive_uncomment()
  let lines =<< trim END
      rem echo "Hello"
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=dosbatch

  normal gcc

  let result = getline(1, '$')
  call assert_equal(['echo "Hello"'], result)
endfunc

func Test_bothends_comment()
  let lines =<< trim END
    int main() {}
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=c

  normal gcc

  let result = getline(1, '$')
  call assert_equal(["/* int main() {} */"], result)
endfunc

func Test_bothends_uncomment()
  let lines =<< trim END
    /* int main() { */
    /*   return 0; */
    /* } */
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=c

  normal gcip

  let result = getline(1, '$')
  call assert_equal(["int main() {", "  return 0;", "}"], result)
endfunc

func Test_mixed_comment()
  let lines =<< trim END
    for x in range(10):
      # print(x)
      # print(x*x)
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=python

  normal gcG

  let result = getline(1, '$')
  call assert_equal(["# for x in range(10):", "#   # print(x)", "#   # print(x*x)"], result)
endfunc

func Test_mixed_comment2()
  let lines =<< trim END
    # for x in range(10):
      print(x)
      # print(x*x)
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=python

  normal gcG

  let result = getline(1, '$')
  call assert_equal(["# # for x in range(10):", "#   print(x)", "#   # print(x*x)"], result)
endfunc

func Test_mixed_indent_comment()
  let lines = ["int main() {", "\tif 1 {", "\t  return 0;", "\t}", "    return 1;", "}"]

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=c

  normal gcip

  let result = getline(1, '$')
  call assert_equal(["/* int main() { */", "\t/* if 1 { */", "\t  /* return 0; */",  "\t/* } */", "    /* return 1; */", "/* } */"], result)
endfunc

func Test_buffer_first_col_comment()
  let lines =<< trim END
    def Hello():
      print("Hello")
      pass
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=python
  let b:comment_first_col = 1

  normal jgcc

  let result = getline(1, '$')
  call assert_equal(["def Hello():", '#   print("Hello")', "  pass"], result)
endfunc

func Test_global_first_col_comment()
  let lines =<< trim END
    def Hello():
      print("Hello")
      pass
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=python
  let g:comment_first_col = 1

  normal jgcj

  unlet g:comment_first_col

  let result = getline(1, '$')
  call assert_equal(["def Hello():", '#   print("Hello")', "#   pass"], result)
endfunc

func Test_textobj_icomment()
  let lines =<< trim END
    for x in range(10):
      print(x) # printing stuff
      # print(x*x)
      #print(x*x*x)
      print(x*x*x*x) # printing stuff
      print(x*x*x*x*x) # printing stuff
      # print(x*x)
      #print(x*x*x)

      print(x*x*x*x*x)
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=python

  normal dic..

  let result = getline(1, '$')
  call assert_equal(["for x in range(10):", "  print(x) ", "  print(x*x*x*x) ", "  print(x*x*x*x*x) ", "", "  print(x*x*x*x*x)"], result)
endfunc

func Test_textobj_icomment2()
  let lines =<< trim END
    #include <stdio.h>

    int main() {
        printf("hello"); /* hello world */ printf(" world\n");
        /* if 1 {
            return 1;
        }*/

        return 0;
    }
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal dic.

  let result = getline(1, '$')
  call assert_equal(["#include <stdio.h>", "", "int main() {", "    printf(\"hello\");  printf(\" world\\n\");", "    ", "", "    return 0;", "}"], result)
endfunc

func Test_textobj_icomment3()
  let lines =<< trim END
    #include <stdio.h>

    int main() {
        printf("hello");/*hello world*/printf(" world\n");
        return 0;
    }
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal jjjdic

  let result = getline(1, '$')
  call assert_equal(["#include <stdio.h>", "", "int main() {", "    printf(\"hello\");printf(\" world\\n\");",  "    return 0;", "}"], result)
endfunc

func Test_textobj_acomment()
  let lines =<< trim END
    for x in range(10):
      print(x) # printing stuff
      # print(x*x)
      #print(x*x*x)
      print(x*x*x*x) # printing stuff
      print(x*x*x*x*x) # printing stuff
      # print(x*x)
      #print(x*x*x)

      print(x*x*x*x*x)
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=python

  normal dac..

  let result = getline(1, '$')
  call assert_equal(["for x in range(10):", "  print(x)", "  print(x*x*x*x)", "  print(x*x*x*x*x)", "", "  print(x*x*x*x*x)"], result)
endfunc

func Test_textobj_acomment2()
  let lines =<< trim END
    #include <stdio.h>

    int main() {
        printf("hello"); /* hello world */ printf(" world\n");
        /* if 1 {
            return 1;
        }*/

        return 0;
    }
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal dac.

  let result = getline(1, '$')
  call assert_equal(["#include <stdio.h>", "", "int main() {", "    printf(\"hello\");printf(\" world\\n\");", "    return 0;", "}"], result)
endfunc

func Test_textobj_acomment3()
  let lines =<< trim END
    #include <stdio.h>

    int main() {
        printf("hello");/*hello world*/printf(" world\n");
        return 0;
    }
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal jjjdac

  let result = getline(1, '$')
  call assert_equal(["#include <stdio.h>", "", "int main() {", "    printf(\"hello\");printf(\" world\\n\");",  "    return 0;", "}"], result)
endfunc

func Test_textobj_firstline_comment()
  let lines =<< trim END
    /*#include <stdio.h>*/

    int main() {}
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal dac

  let result = getline(1, '$')
  call assert_equal(["int main() {}"], result)
endfunc

func Test_textobj_noleading_space_comment()
  let lines =<< trim END
    int main() {// main start
    }/* main end */
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal dacdic

  let result = getline(1, '$')
  call assert_equal(["int main() {", "}"], result)
endfunc

func Test_textobj_noleading_space_comment2()
  let lines =<< trim END
    int main() {// main start
    }    /* main end */
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal dac.

  let result = getline(1, '$')
  call assert_equal(["int main() {", "}"], result)
endfunc

func Test_textobj_trailing_spaces_comment()
  let lines = ['# print("hello")   ', '# print("world")   ', "#", 'print("!")']

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=python

  normal jdac

  let result = getline(1, '$')
  call assert_equal(['print("!")'], result)
endfunc

func Test_textobj_trailing_spaces_last_comment()
  let lines = ['# print("hello")   ', '# print("world")   ', "#", '', '']

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=python

  normal jdac

  let result = getline(1, '$')
  call assert_equal([''], result)
endfunc

func Test_textobj_last_line_empty_comment()
  let lines =<< trim END
    # print("hello")
    #
    #
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=python

  normal dac

  let result = getline(1, '$')
  call assert_equal([''], result)
endfunc

func Test_textobj_cursor_on_leading_space_comment()
  let lines =<< trim END
    int main() {
        // multiple comments
        // cursor is between them
    }
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal jjdac

  let result = getline(1, '$')
  call assert_equal(["int main() {", "}"], result)
endfunc

func Test_textobj_conseq_comment()
  let lines =<< trim END
    int main() {
        printf("hello"); // hello
        // world
        printf("world");
    }
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal dac

  let result = getline(1, '$')
  call assert_equal(["int main() {", "    printf(\"hello\");", "    printf(\"world\");", "}"], result)
endfunc

func Test_textobj_conseq_comment2()
  let lines =<< trim END
    int main() {
        printf("hello"); // hello

        // world
        printf("world");
    }
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=c

  normal dac

  let result = getline(1, '$')
  call assert_equal(["int main() {", "    printf(\"hello\");", "", "    // world", "    printf(\"world\");", "}"], result)
endfunc

func Test_inline_comment()
  let lines =<< trim END
    echo "Hello" This should be a comment
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=vim

  normal fTgC

  let result = getline(1, '$')
  call assert_equal(['echo "Hello" " This should be a comment'], result)
endfunc

func Test_inline_uncomment()
  let lines =<< trim END
    echo "Hello" " This should be a comment
  END

  enew
  call setline(1, lines)
  filetype plugin on
  set ft=vim

  normal $F"gC

  let result = getline(1, '$')
  call assert_equal(['echo "Hello" This should be a comment'], result)
endfunc

func Test_textobj_selection_exclusive_inline_comment()
  let lines =<< trim END
    print("Hello") # selection exclusive
  END

  enew
  call setline(1, lines)
  filetype plugin on
  syntax on
  set ft=python

  normal dac

  let result = getline(1, '$')
  call assert_equal(['print("Hello")'], result)
endfunc
