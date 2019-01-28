func Test_cjk_linebreak()
  call setline('.', '这是一个测试，试试 CJK 行禁则补丁。')
  set textwidth=12 formatoptions=croqn2mB1j
  normal gqq
  call assert_equal('这是一个测试，', getline(1))
endfunc
