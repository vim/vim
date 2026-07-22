vim9script
# Vim9 heredoc
# VIM_TEST_SETUP let g:vimsyn_folding = "h"
# VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


var foo =<< END
line1
line2
END

  var foo =<< END
line1
line2
END

var [foo, bar] =<< EOS
line1
line2
EOS

  var [foo, bar] =<< EOS
line1
line2
EOS

var [foo,
      \ bar] =<< EOS
line1
line2
EOS

  var [foo,
	\ bar] =<< EOS
line1
line2
EOS

# assignment

foo =<< END
line1
line2
END

  foo =<< END
line1
line2
END

g:foo =<< END
line1
line2
END

  g:foo =<< END
line1
line2
END

[foo, bar] =<< EOS
line1
line2
EOS

  [foo, bar] =<< EOS
line1
line2
EOS

[foo,
      \ bar] =<< EOS
line1
line2
EOS

  [foo,
	\ bar] =<< EOS
line1
line2
EOS

[g:foo, g:bar] =<< EOS
line1
line2
EOS

  [g:foo, g:bar] =<< EOS
line1
line2
EOS

[g:foo,
      \ g:bar] =<< EOS
line1
line2
EOS

  [g:foo,
	\ g:bar] =<< EOS
line1
line2
EOS


# typed

var foo: list<string> =<< END
line1
line2
END

var [foo: string, bar: string] =<< END
line1
line2
END

# assignment

# trim

var foo =<< trim END
  line1
  line2
END

  var foo =<< trim END
    line1
    line2
  END


# interpolation

var foo =<< eval END
line{1 + 0}
line{1 + 1}
END

  var foo =<< eval END
line{1 + 0}
line{1 + 1}
END

var foo =<< trim eval END
  line{1 + 0}
  line{1 + 1}
END

  var foo =<< trim eval END
    line{1 + 0}
    line{1 + 1}
  END

# no interpolation (escaped { and })

var foo =<< eval END
line{{1 + 0}}
line{{1 + 1}}
END

  var foo =<< eval END
line{{1 + 0}}
line{{1 + 1}}
END

var foo =<< trim eval END
  line{{1 + 0}}
  line{{1 + 1}}
END

  var foo =<< trim eval END
    line{{1 + 0}}
    line{{1 + 1}}
  END


# no interpolation

var foo =<< END
line{1 + 0}
line{1 + 1}
END

  var foo =<< END
line{1 + 0}
line{1 + 1}
END

var foo =<< trim END
  line{1 + 0}
  line{1 + 1}
END

  var foo =<< trim END
    line{1 + 0}
    line{1 + 1}
  END


# end marker must not be followed by whitespace

# assert_equal(foo, ["END "])
var foo =<< END
END 
END

# assert_equal(foo, [" END "])
var foo =<< END
 END 
END

# assert_equal(foo, ["END "])
var foo =<< trim END
  END 
END

# assert_equal(foo, ["END "])
  var foo =<< trim END
    END 
  END


# end marker must be vertically aligned with :var (if preceded by whitespace)

# assert_equal(foo, ["END"])
var foo =<< trim END
  END
END

  # assert_equal(foo, ["END"])
  var foo =<< trim END
    END
  END

# assert_equal(foo, ["END "])
var foo =<< trim END
END 
END

  # assert_equal(foo, ["END"])
  var foo =<< trim END
    END
  END

  # assert_equal(foo, ["END "])
  var foo =<< trim END
    END 
  END

  # assert_equal(foo, ["END"])
  var foo =<< trim END
     END
  END

  # assert_equal(foo, ["END "])
  var foo =<< trim END
     END 
  END

  # assert_equal(foo, ["END "])
  var foo =<< trim END
END 
END

  # assert_equal(foo, ["END"])
  var foo =<< trim END
 END
END

  # assert_equal(foo, ["END"])
  var foo =<< trim END
   END
END


# end markers

var foo =<< !@#$%^&*()_+
line1
line2
!@#$%^&*()_+

var foo =<< 0!@#$%^&*()_+
line1
line2
0!@#$%^&*()_+

var foo =<< A!@#$%^&*()_+
line1
line2
A!@#$%^&*()_+

# error - leading lowercase character
var foo =<< a!@#$%^&*()_+
line1
line2
a!@#$%^&*()_+

