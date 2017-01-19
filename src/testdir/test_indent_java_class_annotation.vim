" Tests for indentation of a java class with multiple class annotations
function! Test_indent_class_annotation_after_package()
  new
  call append(0, ["package com.foo;",
      	\ "",
      	\ "@SomeClassAnnotation",
      	\ "@SomeOtherAnnotation(true)",
      	\ "public class Bar {",
        \ "}"])
  set autoindent
  filetype plugin indent on
  set filetype=java
  exe "normal! gg=G\<CR>"
  call assert_equal("@SomeClassAnnotation", getline(3))
  enew! | close
endfunction

function! Test_indent_class_annotation_after_import()
  new
  call append(0, ["package com.foo;",
      	\ "",
      	\ "import java.util.String;",
      	\ "",
      	\ "@SomeClassAnnotation",
      	\ "@SomeOtherAnnotation(true)",
      	\ "public class Bar {",
        \ "}"])
  set autoindent
  filetype plugin indent on
  set filetype=java
  exe "normal! gg=G\<CR>"
  call assert_equal("@SomeClassAnnotation", getline(5))
  enew! | close
endfunction
