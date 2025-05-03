" Vim :tcl, :tcldo and :tclfile commands
" VIM_TEST_SETUP let g:vimsyn_folding = "ft"
" VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


tcl << EOF
puts "Tcl script"
EOF
 
  tcl << trim EOF
    puts "Tcl script"
  EOF
 
tcl <<
puts "Tcl script"
.

  tcl << trim
    puts "Tcl script"
  .

function Foo()
  tcl << trim EOF
    puts "Tcl script in :func"
  EOF
endfunction | call Foo()

def Bar()
  tcl << trim EOF
    puts "Tcl script in :def"
  EOF
enddef | call Bar()

tcl puts "Tcl statement";
      "\ comment
      \ puts "Tcl statement again"

tcldo puts "Tcl statement";
      "\ comment
      \ puts "Tcl statement again"

tclfile foo.tcl

