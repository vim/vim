" Vim :ruby, :rubydo and :rubyfile commands
" VIM_TEST_SETUP let g:vimsyn_folding = "fr"
" VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


ruby << EOF
puts "Ruby script"
EOF
 
  ruby << trim EOF
    puts "Ruby script"
  EOF
 
ruby <<
puts "Ruby script"
.

  ruby << trim
    puts "Ruby script"
  .

function Foo()
  ruby << trim EOF
    puts "Ruby script in :func"
  EOF
endfunction | call Foo()

def Bar()
  ruby << trim EOF
    puts "Ruby script in :def"
  EOF
enddef | call Bar()

ruby puts "Ruby statement";
      "\ comment
      \ puts "Ruby statement again"

rubydo puts "Ruby statement";
      "\ comment
      \ puts "Ruby statement again"

rubyfile foo.rb

