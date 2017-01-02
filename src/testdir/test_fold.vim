" Test for folding

function! Test_address_fold()
  new
  call setline(1, ['int FuncName() {/*{{{*/', 1, 2, 3, 4, 5, '}/*}}}*/',
	      \ 'after fold 1', 'after fold 2', 'after fold 3'])
  setl fen fdm=marker
  " The next ccommands should all copy the same part of the buffer,
  " regardless of the adressing type, since the part to be copied
  " is folded away
  :1y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/'], getreg(0,1,1))
  :.y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/'], getreg(0,1,1))
  :.+y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/'], getreg(0,1,1))
  :.,.y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/'], getreg(0,1,1))
  :sil .1,.y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/'], getreg(0,1,1))
  " use silent to make E493 go away
  :sil .+,.y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/'], getreg(0,1,1))
  :,y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/'], getreg(0,1,1))
  :,+y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/','after fold 1'], getreg(0,1,1))
  " using .+3 as second address should copy the whole folded line + the next 3
  " lines
  :.,+3y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/',
	      \ 'after fold 1', 'after fold 2', 'after fold 3'], getreg(0,1,1))
  :sil .,-2y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3', '4', '5', '}/*}}}*/'], getreg(0,1,1))

  " now test again with folding disabled
  set nofoldenable
  :1y
  call assert_equal(['int FuncName() {/*{{{*/'], getreg(0,1,1))
  :.y
  call assert_equal(['int FuncName() {/*{{{*/'], getreg(0,1,1))
  :.+y
  call assert_equal(['1'], getreg(0,1,1))
  :.,.y
  call assert_equal(['int FuncName() {/*{{{*/'], getreg(0,1,1))
  " use silent to make E493 go away
  :sil .1,.y
  call assert_equal(['int FuncName() {/*{{{*/', '1'], getreg(0,1,1))
  " use silent to make E493 go away
  :sil .+,.y
  call assert_equal(['int FuncName() {/*{{{*/', '1'], getreg(0,1,1))
  :,y
  call assert_equal(['int FuncName() {/*{{{*/'], getreg(0,1,1))
  :,+y
  call assert_equal(['int FuncName() {/*{{{*/', '1'], getreg(0,1,1))
  " using .+3 as second address should copy the whole folded line + the next 3
  " lines
  :.,+3y
  call assert_equal(['int FuncName() {/*{{{*/', '1', '2', '3'], getreg(0,1,1))
  :7
  :sil .,-2y
  call assert_equal(['4', '5', '}/*}}}*/'], getreg(0,1,1))

  quit!
endfunction

function! Test_indent_fold()
    new
    call setline(1, ['', 'a', '    b', '    c'])
    setl fen fdm=indent
    2
    norm! >>
    let a=map(range(1,4), 'foldclosed(v:val)')
    call assert_equal([-1,-1,-1,-1], a)
endfu

function! Test_indent_fold()
    new
    call setline(1, ['', 'a', '    b', '    c'])
    setl fen fdm=indent
    2
    norm! >>
    let a=map(range(1,4), 'foldclosed(v:val)')
    call assert_equal([-1,-1,-1,-1], a)
    bw!
endfu

function! Test_indent_fold2()
    new
    call setline(1, ['', '{{{', '}}}', '{{{', '}}}'])
    setl fen fdm=marker
    2
    norm! >>
    let a=map(range(1,5), 'foldclosed(v:val)')
    call assert_equal([-1,-1,-1,4,4], a)
    bw!
endfu
