" Vim :perl and :perldo commands
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

