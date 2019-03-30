" Tests for 'listchars' display with 'list' and :list

source view_util.vim

func Test_listchars()
  enew!
  set ff=unix
  set list

  set listchars+=tab:>-,space:.,trail:<
  call append(0, [
	      \ '	aa	',
	      \ '  bb	  ',
	      \ '   cccc	 ',
	      \ 'dd        ee  	',
	      \ ' '
	      \ ])
  let expected = [
	      \ '>-------aa>-----$',
	      \ '..bb>---<<$',
	      \ '...cccc><$',
	      \ 'dd........ee<<>-$',
	      \ '<$'
	      \ ]
  redraw!
  for i in range(1, 5)
    call cursor(i, 1)
    call assert_equal([expected[i - 1]], ScreenLines(i, virtcol('$')))
  endfor

  set listchars-=trail:<
  let expected = [
	      \ '>-------aa>-----$',
	      \ '..bb>---..$',
	      \ '...cccc>.$',
	      \ 'dd........ee..>-$',
	      \ '.$'
	      \ ]
  redraw!
  for i in range(1, 5)
    call cursor(i, 1)
    call assert_equal([expected[i - 1]], ScreenLines(i, virtcol('$')))
  endfor

  " tab with 3rd character.
  set listchars-=tab:>-
  set listchars+=tab:<=>,trail:-
  let expected = [
	      \ '<======>aa<====>$',
	      \ '..bb<==>--$',
	      \ '...cccc>-$',
	      \ 'dd........ee--<>$',
	      \ '-$'
	      \ ]
  redraw!
  for i in range(1, 5)
    call cursor(i, 1)
    call assert_equal([expected[i - 1]], ScreenLines(i, virtcol('$')))
  endfor

  set listchars-=trail:-
  let expected = [
	      \ '<======>aa<====>$',
	      \ '..bb<==>..$',
	      \ '...cccc>.$',
	      \ 'dd........ee..<>$',
	      \ '.$'
	      \ ]
  redraw!
  for i in range(1, 5)
    call cursor(i, 1)
    call assert_equal([expected[i - 1]], ScreenLines(i, virtcol('$')))
  endfor

  set listchars-=tab:<=>
  set listchars+=tab:>-
  set listchars+=trail:<
  set nolist
  normal ggdG
  call append(0, [
	      \ '  fff	  ',
	      \ '	gg	',
	      \ '     h	',
	      \ 'iii    	  ',
	      \ ])
  let l = split(execute("%list"), "\n")
  call assert_equal([
	      \ '..fff>--<<$',
	      \ '>-------gg>-----$',
	      \ '.....h>-$',
	      \ 'iii<<<<><<$', '$'], l)


  " test nbsp
  normal ggdG
  set listchars=nbsp:X,trail:Y
  set list
  " Non-breaking space
  let nbsp = nr2char(0xa0)
  call append(0, [ ">".nbsp."<" ])

  let expected = '>X< '

  redraw!
  call cursor(1, 1)
  call assert_equal([expected], ScreenLines(1, virtcol('$')))

  set listchars=nbsp:X
  redraw!
  call cursor(1, 1)
  call assert_equal([expected], ScreenLines(1, virtcol('$')))

  enew!
  set listchars& ff&
endfunc

func Test_listchars_composing()
  enew!
  let oldencoding=&encoding
  set encoding=utf-8
  set ff=unix
  set list

  set listchars=eol:$,space:_
  call append(0, [
        \ "  \u3099	 \u309A"
        \ ])
  let expected = [
        \ "_ \u3099^I \u309A$"
        \ ]
  redraw!
  call cursor(1, 1)
  let got = ScreenLines(1, virtcol('$'))
  bw!
  call assert_equal(expected, got)
  let &encoding=oldencoding
  set listchars& ff&
endfunction
