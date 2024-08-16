" Vim :perl and :perldo commands
" VIM_TEST_SETUP let g:vimsyn_embed = "p"
" VIM_TEST_SETUP let g:vimsyn_folding = "fp"
" VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


perl << EOF
print("Perl script")
EOF
 
  perl << trim EOF
    print("Perl script")
  EOF
 
perl <<
print("Perl script")
.

  perl << trim
    print("Perl script")
  .

function Foo()
  perl << trim EOF
    print("Perl script in :func")
  EOF
endfunction | call Foo()

def Bar()
  perl << trim EOF
    print("Perl script in :def")
  EOF
enddef | call Bar()

perl print("Perl statement");
      "\ comment
      \ print("Perl statement again")

perldo print("Perl statement");
      "\ comment
      \ print("Perl statement again")


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


" :tcl, :tcldo and :tclfile

tcl << trim EOF 
  puts "TCL script"
EOF

tcl puts "TCL statement";
      "\ comment
      \ puts "TCL statement again"

tcldo puts "TCL statement";
      "\ comment
      \ puts "TCL statement again"

tclfile foo.tcl

