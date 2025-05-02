" Vim :python, :pydo and :pyfile commands
" VIM_TEST_SETUP let g:vimsyn_folding = "fP"
" VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


python << EOF
print("Python script")
EOF
 
  python << trim EOF
    print("Python script")
  EOF
 
python <<
print("Python script")
.

  python << trim
    print("Python script")
  .

function Foo()
  python << trim EOF
    print("Python script in :func")
  EOF
endfunction | call Foo()

def Bar()
  python << trim EOF
    print("Python script in :def")
  EOF
enddef | call Bar()

python print("Python statement");
      "\ comment
      \ print("Python statement again")

pydo print("Python statement");
      "\ comment
      \ print("Python statement again")

pyfile foo.py

