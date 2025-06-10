vim9script
# Vim9 blocks
# VIM_TESTSETUP set list listchars=tab:> ,trail:-,extends:>,precedes:<,nbsp:+


{
  var foo = 42
}


{
  {
    {
      {
        var foo = 42
      }
      var foo = 42
    }
    var foo = 42
  }
  var foo = 42
}


def Foo()
  {
    var foo = 42
    echo foo
  }
enddef


# start/end patterns

{             
  var foo = 42
}             

{ # comment
  var foo = 42
} # comment

echo "Foo" | { | echo "Bar"
  var foo = 42
} | echo "Baz"

# dictionary
{}->items()

