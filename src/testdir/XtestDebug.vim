vim9script
def g:SomeFunc()
  echo "here"
  echo "and"
  echo "there"
  breakadd func 2 LocalFunc
  LocalFunc()
enddef

def LocalFunc()
  echo "first"
  echo "second"
  breakadd func LegacyFunc
  LegacyFunc()
enddef

func LegacyFunc()
  echo "legone"
  echo "legtwo"
endfunc

breakadd func 2 g:SomeFunc
