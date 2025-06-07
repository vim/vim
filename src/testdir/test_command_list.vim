" Test for listing user commands.

func Test_command_list_0()
  " Check space padding of attribute and name in command list
  set vbs&
  command! ShortCommand echo "ShortCommand"
  command! VeryMuchLongerCommand echo "VeryMuchLongerCommand"

  redi @"> | com | redi END
  pu

  let bl = matchbufline(bufnr('%'), "^    ShortCommand      0", 1, '$')
  call assert_false(bl == [])
  let bl = matchbufline(bufnr('%'), "^    VeryMuchLongerCommand 0", 1, '$')
  call assert_false(bl == [])

  bwipe!
  delcommand ShortCommand
  delcommand VeryMuchLongerCommand
endfunc
