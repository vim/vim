" Vim :history command
" VIM_TEST_SETUP hi link vimHistoryRange Todo

history		1,9
history cmd	1,9
history :	1,9
history search	1,9
history /	1,9
history ?	1,9
history expr	1,9
history =	1,9
history input	1,9
history @	1,9
history debug	1,9
history >	1,9
history all	1,9

history  1
history -1
history  1,
history -1,
history   , 1
history   ,-1
history  1, 1
history  1,-1
history -1, 1
history -1,-1

history 1, 1
history 1 ,1
history 1 , 1


" tail comments and trailing bar

history         | echo "..."
history         " comment
history all     | echo "..."
history all     " comment
history 1,9     | echo "..."
history 1,9     " comment
history all 1,9 | echo "..."
history all 1,9 " comment


def Vim9Context()
  history		1,9
  history cmd		1,9
  history :		1,9
  history search	1,9
  history /		1,9
  history ?		1,9
  history expr		1,9
  # FIXME
  # history =		1,9
  history input		1,9
  history @		1,9
  history debug		1,9
  history >		1,9
  history all		1,9

  history  1
  history -1
  history  1,
  history -1,
  history   , 1
  history   ,-1
  history  1, 1
  history  1,-1
  history -1, 1
  history -1,-1

  history 1, 1
  history 1 ,1
  history 1 , 1


  # tail comments and trailing bar

  history         | echo "..."
  history         # comment
  history all     | echo "..."
  history all     # comment
  history 1,9     | echo "..."
  history 1,9     # comment
  history all 1,9 | echo "..."
  history all 1,9 # comment
enddef

