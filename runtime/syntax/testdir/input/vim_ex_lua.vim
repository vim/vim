" Vim :lua, :luado and :luafile commands
" VIM_TEST_SETUP let g:vimsyn_folding = "fl"
" VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


lua << EOF
print("Lua script")
EOF
 
  lua << trim EOF
    print("Lua script")
  EOF
 
lua <<
print("Lua script")
.

  lua << trim
    print("Lua script")
  .

function Foo()
  lua << trim EOF
    print("Lua script in :func")
  EOF
endfunction | call Foo()

def Bar()
  lua << trim EOF
    print("Lua script in :def")
  EOF
enddef | call Bar()

lua print("Lua statement")
      "\ comment
      \ print("Lua statement again")

luado print("Lua statement")
      "\ comment
      \ print("Lua statement again")

luafile foo.lua

