" Vim insert commands

append
  line 1
  line 2
.

" trailing whitespace
append  
  line 1
  line 2
.

append!
  line 1
  line 2
.

" trailing whitespace
append!  
  line 1
  line 2
.

insert
  line 1
  line 2
.

" trailing whitespace
insert  
  line 1
  line 2
.

insert!
  line 1
  line 2
.

" trailing whitespace
insert!  
  line 1
  line 2
.

change
  line 1
  line 2
.

" trailing whitespace
change  
  line 1
  line 2
.

change!
  line 1
  line 2
.

" trailing whitespace
change!  
  line 1
  line 2
.


" currently unsupported

echo "Pre" | append | echo "Post"
  line 1
  line 2
.

echo "Pre"
      \ | append
      \ | echo "Post"
  line 1
  line 2
.
