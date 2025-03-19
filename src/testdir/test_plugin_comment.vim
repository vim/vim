source check.vim
source term_util.vim

func Test_basic_comment()
  CheckScreendump
  let lines =<< trim END
    vim9script

    def Hello()
      echo "Hello"
    enddef
  END

  let input_file = "test_basic_comment_input.vim"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd comment" ' .. input_file, {})

  call term_sendkeys(buf, "gcc")
  call term_sendkeys(buf, "2jgcip")
  let output_file = "comment_basic_test.vim"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)

  let result = readfile(output_file)

  call assert_equal(["# vim9script", "", "# def Hello()", '#   echo "Hello"', "# enddef"], result)
endfunc


func Test_basic_uncomment()
  CheckScreendump
  let lines =<< trim END
    vim9script

    # def Hello()
    #   echo "Hello"
    # enddef
  END

  let input_file = "test_basic_uncomment_input.vim"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd comment" ' .. input_file, {})

  call term_sendkeys(buf, "gcc")
  call term_sendkeys(buf, "2jgcip")
  let output_file = "uncomment_basic_test.vim"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)

  let result = readfile(output_file)

 call assert_equal(["# vim9script", "", "def Hello()", '  echo "Hello"', "enddef"], result)
endfunc

func Test_bothends_comment()
  CheckScreendump
  let lines =<< trim END
    int main() {}
  END

  let input_file = "test_bothends_comment_input.c"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd comment" ' .. input_file, {})

  call term_sendkeys(buf, "gcc")
  let output_file = "comment_bothends_test.c"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)

  let result = readfile(output_file)

  call assert_equal(["/* int main() {} */"], result)
endfunc

func Test_bothends_uncomment()
  CheckScreendump
  let lines =<< trim END
    /* int main() { */
    /*   return 0; */
    /* } */
  END

  let input_file = "test_bothends_uncomment_input.c"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd comment" ' .. input_file, {})

  call term_sendkeys(buf, "gcip")
  let output_file = "uncomment_bothends_test.c"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)

  let result = readfile(output_file)

  call assert_equal(["int main() {", "  return 0;", "}"], result)
endfunc


func Test_mixed_comment()
  CheckScreendump
  let lines =<< trim END
    for x in range(10):
      # print(x)
      # print(x*x)
  END

  let input_file = "test_mixed_comment_input.py"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd comment" ' .. input_file, {})

  call term_sendkeys(buf, "gcG")
  let output_file = "comment_mixed_test.py"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)

  let result = readfile(output_file)

  call assert_equal(["# for x in range(10):", "#   # print(x)", "#   # print(x*x)"], result)
endfunc

func Test_mixed_comment2()
  CheckScreendump
  let lines =<< trim END
    # for x in range(10):
      print(x)
      # print(x*x)
  END

  let input_file = "test_mixed_comment_input2.py"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd comment" ' .. input_file, {})

  call term_sendkeys(buf, "gcG")
  let output_file = "comment_mixed_test2.py"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)

  let result = readfile(output_file)

  call assert_equal(["# # for x in range(10):", "#   print(x)", "#   # print(x*x)"], result)
endfunc

func Test_mixed_indent_comment()
  CheckScreendump
  let lines = ["int main() {", "\tif 1 {", "\t  return 0;", "\t}", "    return 1;", "}"]

  let input_file = "test_mixed_indent_comment_input.c"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd comment" ' .. input_file, {})

  call term_sendkeys(buf, "gcip")
  let output_file = "comment_mixed_indent_test.c"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)

  let result = readfile(output_file)

  call assert_equal(["/* int main() { */", "\t/* if 1 { */", "\t  /* return 0; */",  "\t/* } */", "    /* return 1; */", "/* } */"], result)
endfunc
