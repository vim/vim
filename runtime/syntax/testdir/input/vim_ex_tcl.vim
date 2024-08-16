" Vim :tcl, :tcldo and :tclfile commands
" VIM_TEST_SETUP let g:vimsyn_embed = "t"
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


" :lua, :luado and :luafile

lua << trim EOF 
  print("Lua script")
EOF

lua print("Lua statement")
      "\ comment
      \ print("Lua statement again")

luado print("Lua statement")
      "\ comment
      \ print("Lua statement again")

luafile foo.lua


" :mzscheme and :mzfile

mzscheme << trim EOF 
  (display "MzScheme script")
EOF

mzscheme (display "MzScheme statement")
      "\ comment
      \ (display "MzScheme statement again")

mzfile foo.rkt


" :perl and :perldo

perl << trim EOF 
  print("Perl script\n")
EOF

perl print("Perl statement\n");
      "\ comment
      \ print("Perl statement again\n")

perldo print("Perl statement\n");
      "\ comment
      \ print("Perl statement again\n")


" :python, :pydo and :pyfile

python << trim EOF 
  print("Python script")
EOF

python print("Python statement");
      "\ comment
      \ print("Python statement again")

pydo print("Python statement");
      "\ comment
      \ print("Python statement again")

pyfile foo.py


" :python3, :py3do and :py3file

python3 << trim EOF 
  print("Python3 script")
EOF

python3 print("Python3 statement");
      "\ comment
      \ print("Python3 statement")

py3do print("Python3 statement");
      "\ comment
      \ print("Python3 statement")

py3file foo.py


" :pythonx, :pyxdo and :pyxfile

pythonx << trim EOF 
  print("PythonX script")
EOF

pythonx print("PythonX statement");
      "\ comment
      \ print("PythonX statement")

pyxdo print("PythonX statement");
      "\ comment
      \ print("PythonX statement")

pyxfile foo.py


" :ruby, :rubydo and :rubyfile

ruby << trim EOF 
  puts "Ruby script"
EOF

ruby puts "Ruby statement";
      "\ comment
      \ puts "Ruby statement again"

rubydo puts "Ruby statement";
      "\ comment
      \ puts "Ruby statement again"

rubyfile foo.rb

