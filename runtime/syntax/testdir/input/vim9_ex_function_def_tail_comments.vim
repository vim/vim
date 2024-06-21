vim9script

# Vim9 :function and :def tail comments
# VIM_TEST_SETUP unlet! g:vimsyn_folding

fun Test1() abort		" fun
  return 1
endfun				" endfun

def Test2(): number		# def
  return 2
enddef				# enddef

fun Test3() abort		" fun
  fun s:DoTest3() abort		" fun
    return 3
  endfun			" endfun
  return s:DoTest3()
endfun				" endfun

def Test4(): number		# def
  def DoTest4(): number		# def
    return 4
  enddef			# enddef
  return DoTest4()
enddef				# enddef

def Test5(): number		# def
  fun DoTest5() abort		" fun
    return 5
  endfun			" endfun
  return DoTest5()
enddef				# enddef

fun Test6() abort		" fun
  def s:DoTest6(): number	# def
    return 6
  enddef			# enddef
  return s:DoTest6()
endfun				" endfun

for d in range(1, 6)
  exec $'echo Test{d}()'
endfor
