" Vim :let heredoc command
" VIM_TEST_SETUP let g:vimsyn_folding = "h"
" VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


let foo =<< END
line1
line2
END

  let foo =<< END
line1
line2
END

let [foo, bar] =<< EOS
line1
line2
EOS

  let [foo, bar] =<< EOS
line1
line2
EOS

let [foo,
      \ bar] =<< EOS
line1
line2
EOS

  let [foo,
	\ bar] =<< EOS
line1
line2
EOS

let g:foo =<< END
line1
line2
END

  let g:foo =<< END
line1
line2
END

let [g:foo, g:bar] =<< EOS
line1
line2
EOS

  let [g:foo, g:bar] =<< EOS
line1
line2
EOS

let [g:foo,
      \ g:bar] =<< EOS
line1
line2
EOS

  let [g:foo,
	\ g:bar] =<< EOS
line1
line2
EOS


" Trim

let foo =<< trim END
  line1
  line2
END

  let foo =<< trim END
    line1
    line2
  END


" Interpolation

let foo =<< eval END
line{1 + 0}
line{1 + 1}
END

  let foo =<< eval END
line{1 + 0}
line{1 + 1}
END

let foo =<< trim eval END
  line{1 + 0}
  line{1 + 1}
END

  let foo =<< trim eval END
    line{1 + 0}
    line{1 + 1}
  END

" No interpolation (escaped { and })

let foo =<< eval END
line{{1 + 0}}
line{{1 + 1}}
END

  let foo =<< eval END
line{{1 + 0}}
line{{1 + 1}}
END

let foo =<< trim eval END
  line{{1 + 0}}
  line{{1 + 1}}
END

  let foo =<< trim eval END
    line{{1 + 0}}
    line{{1 + 1}}
  END


" No interpolation

let foo =<< END
line{1 + 0}
line{1 + 1}
END

  let foo =<< END
line{1 + 0}
line{1 + 1}
END

let foo =<< trim END
  line{1 + 0}
  line{1 + 1}
END

  let foo =<< trim END
    line{1 + 0}
    line{1 + 1}
  END


" End marker must not be followed by whitespace

" assert_equal(foo, ["END "])
let foo =<< END
END 
END

" assert_equal(foo, [" END "])
let foo =<< END
 END 
END

" assert_equal(foo, ["END "])
let foo =<< trim END
  END 
END

" assert_equal(foo, ["END "])
  let foo =<< trim END
    END 
  END


" end marker must be vertically aligned with :let (if preceded by whitespace)

" assert_equal(foo, ["END"])
let foo =<< trim END
  END
END

  " assert_equal(foo, ["END"])
  let foo =<< trim END
    END
  END

" assert_equal(foo, ["END "])
let foo =<< trim END
END 
END

  " assert_equal(foo, ["END"])
  let foo =<< trim END
    END
  END

  " assert_equal(foo, ["END "])
  let foo =<< trim END
    END 
  END

  " assert_equal(foo, ["END"])
  let foo =<< trim END
     END
  END

  " assert_equal(foo, ["END "])
  let foo =<< trim END
     END 
  END

  " assert_equal(foo, ["END "])
  let foo =<< trim END
END 
END

  " assert_equal(foo, ["END"])
  let foo =<< trim END
 END
END

  " assert_equal(foo, ["END"])
  let foo =<< trim END
   END
END


" End markers

let foo =<< !@#$%^&*()_+
line1
line2
!@#$%^&*()_+

let foo =<< 0!@#$%^&*()_+
line1
line2
0!@#$%^&*()_+

let foo =<< A!@#$%^&*()_+
line1
line2
A!@#$%^&*()_+

" error - leading lowercase character
let foo =<< a!@#$%^&*()_+
line1
line2
a!@#$%^&*()_+

