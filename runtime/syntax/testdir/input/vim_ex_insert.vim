" Vim :insert command
" VIM_TEST_SETUP let g:vimsyn_folding = "d"
" VIM_TEST_SETUP setl fdc=2 fdl=999 fdm=syntax

" NOTE: legacy Vim script only


insert
.

insert!
.

insert | line0
.

insert! | line0
.

insert
line1
line2
.

insert!
line1
line2
.

insert " comment
line1
line2
.

insert! " comment
line1
line2
.

insert | line0
line1
line2
.

insert! | line0
line1
line2
.

insert | " line0 of input text
line1
line2
.

insert! | " line0 of input text
line1
line2
.

insert | echo "line0 of input text"
line0
line2
.

insert! | echo "line0 of input text"
line1
line2
.


" empty first line


insert

.

insert!

.

insert

line2
.

insert!

line2
.

insert " comment

line2
.

insert! " comment

line2
.

insert | line0

line2
.

insert! | line0

line2
.

insert | " line0 of input text

line2
.

insert! | " line0 of input text

line2
.

insert | echo "line0 of input text"

line2
.

insert! | echo "line0 of input text"

line2
.


" no early termination
" NOTE: 'autoindent' end marker not supported

insert
. 
 .
 . 
.

function Foo()
  insert
. 
 .
 . 
.

  insert!
. 
 .
 . 
.

  insert " comment
. 
 .
 . 
.

  insert! " comment
. 
 .
 . 
.

  insert | line0
. 
 .
 . 
.

  insert! | line0
. 
 .
 . 
.

  insert
    endfunction
.
  insert!
    ndfunction
.
  insert " comment
    endfunction
.
  insert! " comment
    endfunction
.
  insert | line0
    endfunction
.
  insert! | line0
    endfuncion
.
endfunction


