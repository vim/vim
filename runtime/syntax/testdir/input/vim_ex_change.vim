" Vim :change command
" VIM_TEST_SETUP let g:vimsyn_folding = "d"
" VIM_TEST_SETUP setl fdc=2 fdl=999 fdm=syntax

" NOTE: legacy Vim script only


change
.

change!
.

change | line0
.

change! | line0
.

change
line1
line2
.

change!
line1
line2
.

change " comment
line1
line2
.

change! " comment
line1
line2
.

change | line0
line1
line2
.

change! | line0
line1
line2
.

change | " line0 of input text
line1
line2
.

change! | " line0 of input text
line1
line2
.

change | echo "line0 of input text"
line0
line2
.

change! | echo "line0 of input text"
line1
line2
.


" empty first line


change

.

change!

.

change

line2
.

change!

line2
.

change " comment

line2
.

change! " comment

line2
.

change | line0

line2
.

change! | line0

line2
.

change | " line0 of input text

line2
.

change! | " line0 of input text

line2
.

change | echo "line0 of input text"

line2
.

change! | echo "line0 of input text"

line2
.


" no early termination
" NOTE: 'autoindent' end marker not supported

change
. 
 .
 . 
.

function Foo()
  change
. 
 .
 . 
.

  change!
. 
 .
 . 
.

  change " comment
. 
 .
 . 
.

  change! " comment
. 
 .
 . 
.

  change | line0
. 
 .
 . 
.

  change! | line0
. 
 .
 . 
.

  change
    endfunction
.
  change!
    ndfunction
.
  change " comment
    endfunction
.
  change! " comment
    endfunction
.
  change | line0
    endfunction
.
  change! | line0
    endfuncion
.
endfunction


