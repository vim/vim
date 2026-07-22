vim9script
# Vim9 :echo commands
# VIM_TEST_SETUP hi link vimVar Identifier


echo         # comment
echo "Foo"   # comment
echo foo[0]  # comment
echo Foo()   # comment
echo "Foo" | # comment

def Foo()
  echo         # comment
  echo "Foo"   # comment
  echo foo[0]  # comment
  echo Foo()   # comment
  echo "Foo" | # comment
enddef


echo "4"
  # comment
  .. "2" # comment

echo "4" ..
  # comment
  "2" # comment

echo "4" # comment
  # comment
  .. "2" # comment

echo "4" .. # comment
  # comment
  "2" # comment

