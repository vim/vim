" Vim :syntax command


" :syn-case

syntax case
syntax case match
syntax case ignore

" :syn-conceal

syntax conceal
syntax conceal on
syntax conceal off

" :syn-foldlevel

syntax foldlevel
syntax foldlevel start
syntax foldlevel minimum

" :syn-iskeyword

syntax iskeyword
syntax iskeyword clear
syntax iskeyword @,48-57,192-255,$,_

" :syn-list

syntax list
syntax list Foo
syntax list @Bar

" :syn-spell

syntax spell
syntax spell toplevel
syntax spell notoplevel
syntax spell default

" :syn-sync-ccomment

syntax sync ccomment
syntax sync ccomment Foo
syntax sync ccomment minlines=42 maxlines=42 linebreaks=1 linecont "pattern"
syntax sync ccomment Foo minlines=42 maxlines=42 linebreaks=1 linecont "pattern"

" :syn-sync-fromstart

syntax sync fromstart
syntax sync fromstart minlines=42 maxlines=42 linebreaks=1 linecont "pattern"

" :syn-sync-linebreaks

syntax sync linebreaks=1
syntax sync linebreaks=1 minlines=42 maxlines=42 linecont "pattern"

" :syn-sync-linecont

syntax sync linecont "pattern"
syntax sync linecont "pattern" minlines=42 maxlines=42 linebreaks=1

" :syn-sync-(min|max)lines

syntax sync minlines=42 maxlines=42
syntax sync minlines=42 maxlines=42 linebreaks=1 linecont "pattern"
" obsolete
syntax sync lines=42

" :syn-sync sync patterns

syntax sync match testSyncMatch grouphere  testFoo "pattern"
syntax sync match testSyncMatch groupthere testBar "pattern"
syntax sync match testSyncMatch grouphere  NONE "pattern"
syntax sync match testSyncMatch groupthere NONE "pattern"

" :syn-sync skip groups

syn sync match testMatch "pattern"
syn sync region testRegion start="start-pattern" skip="skip-pattern" end="end-pattern"

" :syn-sync-clear

syntax sync clear
syntax sync clear Foo

" tail comments are not explicitly supported by :syntax, see :help :comment
syn match testMatch "pattern" contained " tail comment
" NOTE: comments not currently supported
syn keyword testKeyword keyword contained " tail comment
syn region testRegion start="start-pattern" skip="skip-pattern" end="end-pattern" contained " tail comment

" Multiline commands

syn keyword testKeyword
      "\ OPTIONS
      "\ conceal option
      \ conceal
      "\ cchar option
      \ cchar=&
      "\ contained option
      \ contained
      "\ containedin option
      \ containedin=testContainer
      "\ nextgroup option
      \ nextgroup=testNext0,@testCluster
      "\ transparent option
      \ transparent
      "\ skipwhite option
      \ skipwhite
      "\ skipempty option
      \ skipempty
      "\ skipnl option
      \ skipnl
      "\ KEYWORDS LIST
      "\ keyword 1
      \ keyword1
      "\ keyword 2
      \ keyword2
      "\ keyword 3
      \ keyword3

syn match testMatch
      "\ MATCH PATTERN
      "\ pattern start
      \ /
      "\ part 1 description
      \pat1a .* pat1b
      "\ part 2 description
      \pat2a .* pat2b
      "\ part 3 description
      \pat3a .* pat3b
      "\ pattern end
      \/
      "\ OPTIONS
      "\ conceal option
      \ conceal
      "\ cchar option
      \ cchar=&
      "\ contained option
      \ contained
      "\ containedin option
      \ containedin=testContainer
      "\ nextgroup option
      \ nextgroup=testNext0,@testCluster
      "\ transparent option
      \ transparent
      "\ skipwhite option
      \ skipwhite
      "\ skipempty option
      \ skipempty
      "\ skipnl option
      \ skipnl
      "\ contains option
      \ contains=testContained1,testContained2
      "\ fold option
      \ fold
      "\ display option
      \ display
      "\ extend option
      \ extend
      "\ excludenl option
      \ excludenl
      "\ keepend option
      \ keepend

syn region testRegion
      "\ OPTIONS
      "\ start option
      \ start="start-pattern"
      "\ skip option
      \ skip="skip-pattern"
      "\ end option
      \ end="end-pattern"
      "\ conceal option
      \ conceal
      "\ cchar option
      \ cchar=&
      "\ contained option
      \ contained
      "\ containedin option
      \ containedin=testContainer
      "\ nextgroup option
      \ nextgroup=testNext0,@testCluster
      "\ transparent option
      \ transparent
      "\ skipwhite option
      \ skipwhite
      "\ skipempty option
      \ skipempty
      "\ skipnl option
      \ skipnl
      "\ contains option
      \ contains=testContained1,testContained2
      "\ oneline option
      \ oneline
      "\ fold option
      \ fold
      "\ display option
      \ display
      "\ extend option
      \ extend
      "\ concealends option
      \ concealends
      "\ excludenl option
      \ excludenl
      "\ keepend option
      \ keepend

syn cluster testCluster
      "\ OPTIONS
      "\ contains option
      \ contains=testContained1,testContained2,testContained3

syn cluster testCluster
      "\ OPTIONS
      "\ add option
      \ add=testAdd
      "\ remove option
      \ remove=testRemove


" multiline group list

syn keyword testNext0 keyword
syn keyword testNext1 keyword
syn keyword testNext2 keyword
syn keyword testNext3 keyword
syn keyword testNext4 keyword
syn keyword testNext5 keyword
syn keyword testNext6 keyword
syn keyword testNext7 keyword
syn keyword testNext8 keyword
syn keyword testNext9 keyword

syn keyword testKeyword
      "\ nextgroup option
      \ nextgroup=
      "\ a comment
      \ testNext0 , testNext1 , 
      "\ a comment
      \ testNext[2-8].* , 
      "\ a comment
      \ testNext9 , @testCluster skipwhite
      "\ KEYWORDS LIST
      \ keyword4
      \ keyword5
      \ keyword6


" leaking contained groups

" Example: runtime/syntax/zsh.vim
"   "cluster" should not be highlighted outside of :syntax commands

function! s:ContainedGroup()
  " ...
  for cluster in ['markdownHighlight_zsh', 'zsh']
    " ...
  endfor
  " ...
endfunction

" early termination of vimSynRegion

syn region testRegion
      "\ | does not end the args region
      "\ start="foo\|bar"
      \ start="start"
      \ end="end"

