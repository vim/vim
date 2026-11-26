" Vim :z command
" VIM_TEST_SETUP hi link vimZType Todo
" [range]z[!][type ...] [count] [flags]

z


" bang

z!


" type

z=
z.
z+
z-
z^

z++
z+++
z--
z---
z^^
z^^^


" count

z42
z 42


" flags

z#
z#lp

z l
z p
z #
z lp#


" bang type

z!=
z!.
z!+
z!-
z!^

z!++
z!+++
z!--
z!---
z!^^
z!^^^


" bang count

z!42
z! 42


" bang flags

z! l
z! p
z! #
z! lp#


" type count

z=42
z.42
z+42
z-42
z^42

z= 42
z. 42
z+ 42
z- 42
z^ 42

z++ 42
z+++ 42
z-- 42
z--- 42
z^^ 42
z^^^ 42

z++42
z+++42
z--42
z---42
z^^42
z^^^42


" type flags

z=lp#
z.lp#
z+lp#
z-lp#
z^lp#

z= lp#
z. lp#
z+ lp#
z- lp#
z^ lp#

z++ lp#
z+++ lp#
z-- lp#
z--- lp#
z^^ lp#
z^^^ lp#

z++lp#
z+++lp#
z--lp#
z---lp#
z^^lp#
z^^^lp#


" count flags

z42l
z42lp
z42#
z42lp#

z42 l
z42 lp
z42 #
z42 lp#

z 42 l
z 42 lp
z 42 #
z 42 lp#


" bang type count

z!=42
z!.42
z!+42
z!-42
z!^42

z!= 42
z!. 42
z!+ 42
z!- 42
z!^ 42

z!++ 42
z!+++ 42
z!-- 42
z!--- 42
z!^^ 42
z!^^^ 42

z!++42
z!+++42
z!--42
z!---42
z!^^42
z!^^^42


" bang type flags

z!=lp#
z!.lp#
z!+lp#
z!-lp#
z!^lp#

z!= lp#
z!. lp#
z!+ lp#
z!- lp#
z!^ lp#

z!++ lp#
z!+++ lp#
z!-- lp#
z!--- lp#
z!^^ lp#
z!^^^ lp#

z!++lp#
z!+++lp#
z!--lp#
z!---lp#
z!^^lp#
z!^^^lp#

" bang count flags

z!42l
z!42lp
z!42#
z!42lp#

z!42 l
z!42 lp
z!42 #
z!42 lp#

z! 42 l
z! 42 lp
z! 42 #
z! 42 lp#


" type count flags

z=42lp#
z.42lp#
z+42lp#
z-42lp#
z^42lp#

z= 42lp#
z. 42lp#
z+ 42lp#
z- 42lp#
z^ 42lp#

z=42 lp#
z.42 lp#
z+42 lp#
z-42 lp#
z^42 lp#

z= 42 lp#
z. 42 lp#
z+ 42 lp#
z- 42 lp#
z^ 42 lp#

z++42lp#
z+++42lp#
z--42lp#
z---42lp#
z^^42lp#
z^^^42lp#

z++ 42lp#
z+++ 42lp#
z-- 42lp#
z--- 42lp#
z^^ 42lp#
z^^^ 42lp#

z++42 lp#
z+++42 lp#
z--42 lp#
z---42 lp#
z^^42 lp#
z^^^42 lp#

z++ 42 lp#
z+++ 42 lp#
z-- 42 lp#
z--- 42 lp#
z^^ 42 lp#
z^^^ 42 lp#


" bang type count flags

z=42lp#
z.42lp#
z+42lp#
z-42lp#
z^42lp#

z= 42lp#
z. 42lp#
z+ 42lp#
z- 42lp#
z^ 42lp#

z=42 lp#
z.42 lp#
z+42 lp#
z-42 lp#
z^42 lp#

z= 42 lp#
z. 42 lp#
z+ 42 lp#
z- 42 lp#
z^ 42 lp#

z++42lp#
z+++42lp#
z--42lp#
z---42lp#
z^^42lp#
z^^^42lp#

z++ 42lp#
z+++ 42lp#
z-- 42lp#
z--- 42lp#
z^^ 42lp#
z^^^ 42lp#

z++42 lp#
z+++42 lp#
z--42 lp#
z---42 lp#
z^^42 lp#
z^^^42 lp#

z++ 42 lp#
z+++ 42 lp#
z-- 42 lp#
z--- 42 lp#
z^^ 42 lp#
z^^^ 42 lp#

z!=42lp#
z!.42lp#
z!+42lp#
z!-42lp#
z!^42lp#

z!= 42lp#
z!. 42lp#
z!+ 42lp#
z!- 42lp#
z!^ 42lp#

z!=42 lp#
z!.42 lp#
z!+42 lp#
z!-42 lp#
z!^42 lp#

z!= 42 lp#
z!. 42 lp#
z!+ 42 lp#
z!- 42 lp#
z!^ 42 lp#

z!++42lp#
z!+++42lp#
z!--42lp#
z!---42lp#
z!^^42lp#
z!^^^42lp#

z!++ 42lp#
z!+++ 42lp#
z!-- 42lp#
z!--- 42lp#
z!^^ 42lp#
z!^^^ 42lp#

z!++42 lp#
z!+++42 lp#
z!--42 lp#
z!---42 lp#
z!^^42 lp#
z!^^^42 lp#

z!++ 42 lp#
z!+++ 42 lp#
z!-- 42 lp#
z!--- 42 lp#
z!^^ 42 lp#
z!^^^ 42 lp#


" tail comments and trailing bar

z " comment
z | echo "..."
z! " comment
z! | echo "..."
z= " comment
z= | echo "..."
z. " comment
z. | echo "..."
z+ " comment
z+ | echo "..."
z- " comment
z- | echo "..."
z^ " comment
z^ | echo "..."
z42 " comment
z42 | echo "..."
z# " comment
z# | echo "..."
z # " comment
z # | echo "..."
z l " comment
z l | echo "..."
z p " comment
z p | echo "..."
z lp# " comment
z lp# | echo "..."

def Vim9Context()
  z=42

  # always an assignment
  z = 42
  Foo() | z = 42

  z # comment
  z | echo "..."
  z! # comment
  z! | echo "..."
  z= # comment
  z= | echo "..."
  # FIXME: after legacy/Vim9 split
  z. # comment
  z. | echo "..."
  z+ # comment
  z+ | echo "..."
  z- # comment
  z- | echo "..."
  z^ # comment
  z^ | echo "..."
  z42 # comment
  z42 | echo "..."
  z# # comment
  z# | echo "..."
  z l # comment
  z l | echo "..."
  z p # comment
  z p | echo "..."
  z lp# # comment
  z lp# | echo "..."
enddef


" Vim leading flags extension

z#=
z#.
z#+
z#-
z#^

z #=
z #.
z #+
z #-
z #^

z l=
z l.
z l+
z l-
z l^

z p=
z p.
z p+
z p-
z p^

z lp#=
z lp#.
z lp#+
z lp#-
z lp#^

z!#=
z!#.
z!#+
z!#-
z!#^

z! #=
z! #.
z! #+
z! #-
z! #^

z!l=
z!l.
z!l+
z!l-
z!l^

z! l=
z! l.
z! l+
z! l-
z! l^

z!p=
z!p.
z!p+
z!p-
z!p^

z! p=
z! p.
z! p+
z! p-
z! p^

z! lp#=
z! lp#.
z! lp#+
z! lp#-
z! lp#^

