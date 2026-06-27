" Vim :append command
" VIM_TEST_SETUP let g:vimsyn_folding = "d"
" VIM_TEST_SETUP setl fdc=2 fdl=999 fdm=syntax

" NOTE: legacy Vim script only


append
.

append!
.

append | line0
.

append! | line0
.

append
line1
line2
.

append!
line1
line2
.

append " comment
line1
line2
.

append! " comment
line1
line2
.

append | line0
line1
line2
.

append! | line0
line1
line2
.

append | " line0 of input text
line1
line2
.

append! | " line0 of input text
line1
line2
.

append | echo "line0 of input text"
line0
line2
.

append! | echo "line0 of input text"
line1
line2
.


" empty first line


append

.

append!

.

append

line2
.

append!

line2
.

append " comment

line2
.

append! " comment

line2
.

append | line0

line2
.

append! | line0

line2
.

append | " line0 of input text

line2
.

append! | " line0 of input text

line2
.

append | echo "line0 of input text"

line2
.

append! | echo "line0 of input text"

line2
.


" no early termination
" NOTE: 'autoindent' end marker not supported

append
. 
 .
 . 
.

function Foo()
  append
. 
 .
 . 
.

  append!
. 
 .
 . 
.

  append " comment
. 
 .
 . 
.

  append! " comment
. 
 .
 . 
.

  append | line0
. 
 .
 . 
.

  append! | line0
. 
 .
 . 
.

  append
    endfunction
.
  append!
    ndfunction
.
  append " comment
    endfunction
.
  append! " comment
    endfunction
.
  append | line0
    endfunction
.
  append! | line0
    endfuncion
.
endfunction


