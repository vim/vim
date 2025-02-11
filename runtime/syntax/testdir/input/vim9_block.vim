vim9script

# Vim9 blocks


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

echo "foo" | {
  var foo = 42
}

