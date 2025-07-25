vim9script
# Vim :new command and class constructors
# TODO: move to vim9_constructor and create new vim_ex_new and vim9_ex_new
#       tests


class Test
  def new()
  enddef
  def newOther()
  enddef
  def newyetanother()
  enddef
endclass

Test.new()
Test.newOther()
Test.newyetanother()
new
quit

