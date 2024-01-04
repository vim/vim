" Vim :syntax highlighting

syn keyword testKeyword
      \ conceal
      \ cchar=&
      \ contained
      \ containedin=testContainer
      \ nextgroup=testNext,@testCluster
      \ transparent
      \ skipwhite
      \ skipempty
      \ skipnl
      \ keyword1
      \ keyword2
      \ keyword3

syn match testMatch
      \ "pattern"
      \ conceal
      \ cchar=&
      \ contained
      \ containedin=testContainer
      \ nextgroup=testNext,@testCluster
      \ transparent
      \ skipwhite
      \ skipempty
      \ skipnl
      \ contains=testContained1,testContained2
      \ fold
      \ display
      \ extend
      \ excludenl
      \ keepend

syn region testRegion
      \ start="start-pattern"
      \ end="end-pattern"
      \ skip="skip-pattern"
      \ contained
      \ conceal
      \ cchar=&
      \ contained
      \ containedin=testContainer
      \ nextgroup=testNext,@testCluster
      \ transparent
      \ skipwhite
      \ skipempty
      \ skipnl
      \ contains=testContained1,testContained2
      \ oneline
      \ fold
      \ display
      \ extend
      \ concealends
      \ excludenl
      \ keepend

syn cluster testCluster
      \ contains=testContained1,testContained2,testContained3

syn cluster testCluster
      \ add=testAdd
      \ remove=testRemove


" check multiline group list
syn keyword testKeyword
      \ nextgroup=
      \ testNext , 
      \ testNext2 , 
      \ @testCluster 
      \ skipwhite
      \ keyword4
      \ keyword5
      \ keyword6
