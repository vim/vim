CheckFeature image

source util/check.vim

func Test_image_add_basic()
  set imageprotocol=.*:sixel

  " RGB: 2x2 pixels, 3 bytes each.
  let blob = 0zff000000ff000000ffffffff
  let id = image_add(#{data: blob, width: 2, height: 2})
  call assert_true(id > 0)

  let info = image_info(id)
  call assert_equal(1, len(info))
  call assert_equal(id, info[0].id)
  call assert_equal(2, info[0].width)
  call assert_equal(2, info[0].height)
  call assert_equal(0, info[0].alpha)
  call assert_equal('rgb', info[0].format)
  call assert_equal(blob, info[0].data)

  call image_discard(id)
  call assert_fails('call image_info(id)', 'E1587:')

  set imageprotocol&
endfunc

func Test_image_add_rgba()
  set imageprotocol=.*:sixel

  let blob = 0zff0000ff00ff00ffff0000ff00ffffff
  let id = image_add(#{data: blob, width: 2, height: 2})
  call assert_true(id > 0)

  let info = image_info(id)
  call assert_equal(1, info[0].alpha)
  call assert_equal('rgba', info[0].format)

  call image_discard(id)

  set imageprotocol&
endfunc

func Test_image_add_invalid_args()
  set imageprotocol=.*:sixel

  " Missing required keys.
  call assert_fails('call image_add(#{width: 2, height: 2})', 'E474:')
  call assert_fails('call image_add(#{data: 0z00, height: 2})', 'E474:')
  call assert_fails('call image_add(#{data: 0z00, width: 2})', 'E474:')

  " Zero / negative dimensions.
  call assert_fails('call image_add(#{data: 0z00, width: 0, height: 2})',
        \ 'E474:')
  call assert_fails('call image_add(#{data: 0z00, width: -1, height: 2})',
        \ 'E474:')

  " Data length matches neither RGB nor RGBA size.
  call assert_fails(
        \ 'call image_add(#{data: 0z0011, width: 2, height: 2})',
        \ 'E475:')

  set imageprotocol&
endfunc

func Test_image_discard_invalid_id()
  set imageprotocol=.*:sixel

  call assert_fails('call image_discard(99999999)', 'E1587:')

  set imageprotocol&
endfunc

" Test image_info()
func Test_image_info_all_images()
  set imageprotocol=.*:sixel

  let blob = repeat([0, 0, 0], 4)->list2blob()
  let id1 = image_add(#{data: blob, width: 2, height: 2})
  let id2 = image_add(#{data: blob, width: 2, height: 2})

  let info = image_info()
  let ids = info->mapnew({_, v -> v.id})
  call assert_true(index(ids, id1) >= 0)
  call assert_true(index(ids, id2) >= 0)

  call image_discard(id1)
  call image_discard(id2)

  " Discarded images (state made private) should no longer show up.
  let info = image_info()
  let ids = info->mapnew({_, v -> v.id})
  call assert_true(index(ids, id1) < 0)
  call assert_true(index(ids, id2) < 0)

  set imageprotocol&
endfunc

" Test that images still referenced by a popup is not freed when image_discard()
" is called on the id.
func Test_image_add_refcount_via_popup()
  set imageprotocol=.*:sixel

  let blob = repeat([255, 0, 0], 4 * 4)->list2blob()
  let id = image_add(#{data: blob, width: 4, height: 4})

  let winid = popup_create('', #{image: #{id: id}, line: 1, col: 1})
  call assert_equal(1, len(image_info(id)))
  call image_discard(id)

  call popup_close(winid)
  call assert_fails('call image_info(id)', 'E1587:')

  set imageprotocol&
endfunc

" Test that images created directly by popup windows cannot be accessed anywhere
func Test_image_popup_private()
  set imageprotocol=.*:sixel

  let blob = repeat([255, 0, 0], 4 * 4)->list2blob()
  let winid = popup_create('', #{image:
        \ #{data: blob, width: 4, height: 4}, line: 1, col: 1})

  call assert_equal(0, len(image_info()))

  call popup_close(winid)

  set imageprotocol&
endfunc

" Test that image backend can be changed at runtime
func Test_imageprotocol_switch()
  CheckNotGui

  set imageprotocol=.*:sixel

  let blob = repeat([255, 0, 0], 4 * 4)->list2blob()
  let id = image_add(#{data: blob, width: 4, height: 4})
  let winid = popup_create('', #{image: #{id: id}, line: 1, col: 1})

  let &imageprotocol = $'{&term}:none'
  call assert_equal('none', v:imagebackend)

  let &imageprotocol = $'{&term}:kitty'
  call assert_equal('kitty', v:imagebackend)

  let &imageprotocol = $'{&term}:sixel'
  call assert_equal('sixel', v:imagebackend)

  let &imageprotocol = $'{&term}:none'
  call assert_equal('none', v:imagebackend)

  let &imageprotocol = $'{&term}:sixel'
  call assert_equal('sixel', v:imagebackend)

  let &imageprotocol = $'{&term}:kitty'
  call assert_equal('kitty', v:imagebackend)

  call image_discard(id)
  call popup_close(winid)
  set imageprotocol&
endfunc

" Test that image backend is updated when switching from terminal to gui. Must
" be last because it runs ":gui".
func Test_zz1_imageprotocol_switch_gui()
  CheckNotGui
  CheckCanRunGui
  CheckFeature gui_gtk

  set imageprotocol=.*:sixel

  let blob = repeat([255, 0, 0], 4 * 4)->list2blob()
  let id = image_add(#{data: blob, width: 4, height: 4})
  let winid = popup_create('', #{image: #{id: id}, line: 1, col: 1})

  call assert_equal('sixel', v:imagebackend)

  gui -f

  call assert_equal('gui', v:imagebackend)

  call image_discard(id)
  call popup_close(winid)
  set imageprotocol&
endfunc

" The kitty image backend cannot be tested in a terminal window: it does not
" show the escape sequences Vim writes and does not answer the probe for the
" kitty graphics protocol.  Run Vim on a pty as a job instead and look at what
" it writes there.

" a=t: transmit the pixels of an image.
let s:kitty_transmit = "\<Esc>_Ga=t,"
" a=p: place a transmitted image on the screen.
let s:kitty_place = "\<Esc>_Ga=p,"
" a=d,d=i: delete the placement of an image, the terminal keeps the pixels.
let s:kitty_delete = "\<Esc>_Ga=d,d=i,"

" The start of a sixel image, a DCS with the "q" command.
let s:sixel_start_pat = "\<Esc>P[0-9;]*q"
" The tab line of the second tab page, drawn after ":tabedit".
let s:tabline = "[No Name] "

" The script for the Vim on the pty: create a popup with an image.
let s:image_popup_script =<< trim END
  set imageprotocol=.*:kitty
  let img = repeat([0xff, 0, 0], 16 * 32)->list2blob()
  call popup_create('', #{image: #{data: img, width: 16, height: 32}})
  redraw
END

" The same with the sixel backend.
let s:image_popup_script_sixel =<< trim END
  set imageprotocol=.*:sixel
  let img = repeat([0xff, 0, 0], 16 * 32)->list2blob()
  call popup_create('', #{image: #{data: img, width: 16, height: 32}})
  redraw
END

" Collect what the Vim on the pty writes in s:pty_out.
func s:PtyOutput(job, msg)
  let s:pty_out ..= a:msg
endfunc

" Start Vim on a pty with the script written to XpopupImageTab.
func s:StartVimWithImageOnPty()
  let s:pty_out = ''
  return job_start(GetVimCommandCleanTerm() .. ' -S XpopupImageTab', #{
        \ pty: 1,
        \ out_mode: 'raw',
        \ env: #{TERM: 'xterm', LINES: '24', COLUMNS: '80'},
        \ out_cb: {job, msg -> s:PtyOutput(job, msg)},
        \ })
endfunc

" Wait for the Vim on the pty to write "seq" after the first "start" bytes.
func s:WaitForPtyOutput(seq, start)
  call WaitForAssert({-> assert_notequal(-1,
        \ stridx(s:pty_out, a:seq, a:start))})
endfunc

func Test_popup_image_kitty_leave_tabpage()
  CheckUnix
  CheckFeature job
  CheckFeature image_popup

  call writefile(s:image_popup_script, 'XpopupImageTab', 'D')
  let job = s:StartVimWithImageOnPty()
  try
    call s:WaitForPtyOutput(s:kitty_transmit, 0)
    call s:WaitForPtyOutput(s:kitty_place, 0)

    " Leaving the tab page deletes the placement.
    let start = len(s:pty_out)
    call ch_sendraw(job, ":tabedit\<CR>")
    call s:WaitForPtyOutput(s:kitty_delete, start)

    " Entering the tab page places the image again without transmitting it.
    let start = len(s:pty_out)
    call ch_sendraw(job, ":tabnext\<CR>")
    call s:WaitForPtyOutput(s:kitty_place, start)
    call assert_equal(-1, stridx(s:pty_out, s:kitty_transmit, start))
  finally
    call job_stop(job, 'kill')
    call WaitForAssert({-> assert_equal('dead', job_status(job))})
  endtry
endfunc

func Test_popup_image_kitty_covered()
  CheckUnix
  CheckFeature job
  CheckFeature image_popup

  let lines =<< trim END
    set imageprotocol=.*:kitty
    let img = repeat([0xff, 0, 0], 64 * 64)->list2blob()
    call popup_create('', #{image: #{data: img, width: 64, height: 64},
          \ line: 2, col: 2, zindex: 50})
    let g:cover = popup_create(['xx', 'xx'], #{line: 3, col: 6, zindex: 100})
    redraw
  END
  call writefile(lines, 'XpopupImageTab', 'D')
  let job = s:StartVimWithImageOnPty()
  try
    call s:WaitForPtyOutput(',p=4,', 0)
    " The row above the cover, the parts left and right of it, the row below.
    for seq in [',p=1,x=0,y=0,w=64,h=16,', ',p=2,x=0,y=16,w=32,h=32,',
          \ ',p=3,x=48,y=16,w=16,h=32,', ',p=4,x=0,y=48,w=64,h=16,']
      call assert_notequal(-1, stridx(s:pty_out, seq), seq)
    endfor

    " Without the cover one placement is enough, the others are deleted.  The
    " screen may be drawn once more before that, with the four placements
    " again: wait for the deletion of the last one.
    let start = len(s:pty_out)
    call ch_sendraw(job, ":call popup_close(g:cover)\<CR>")
    call WaitForAssert({-> assert_match(s:kitty_delete .. 'i=\d\+,p=4,',
          \ s:pty_out[start :])})
    call assert_notequal(-1, stridx(s:pty_out, ',p=1,x=0,y=0,w=64,h=64,',
          \ start))
    for p in [2, 3, 4]
      call assert_match(s:kitty_delete .. 'i=\d\+,p=' .. p .. ',',
            \ s:pty_out[start :])
    endfor
  finally
    call job_stop(job, 'kill')
    call WaitForAssert({-> assert_equal('dead', job_status(job))})
  endtry
endfunc

func Test_popup_image_sixel_leave_tabpage()
  CheckUnix
  CheckFeature job
  CheckFeature image_popup

  call writefile(s:image_popup_script_sixel, 'XpopupImageTab', 'D')
  let job = s:StartVimWithImageOnPty()
  try
    call WaitForAssert({-> assert_match(s:sixel_start_pat, s:pty_out)})

    " Sixel pixels cannot be deleted, the cells they cover are drawn over
    " instead.  Leaving the tab page does not draw the image again.
    let start = len(s:pty_out)
    call ch_sendraw(job, ":tabedit\<CR>")
    call s:WaitForPtyOutput(s:tabline, start)
    let mid = len(s:pty_out)
    call assert_equal(-1, match(s:pty_out[start : mid], s:sixel_start_pat))

    " Entering the tab page draws it again.
    call ch_sendraw(job, ":tabnext\<CR>")
    call WaitForAssert({-> assert_notequal(-1,
          \ match(s:pty_out[mid :], s:sixel_start_pat))})
  finally
    call job_stop(job, 'kill')
    call WaitForAssert({-> assert_equal('dead', job_status(job))})
  endtry
endfunc

func Test_popup_image_sixel_screen_shrinks()
  CheckUnix
  CheckFeature job
  CheckFeature image_popup

  " The image covers cells that are outside a smaller screen.
  let lines =<< trim END
    set imageprotocol=.*:sixel
    let img = repeat([0xff, 0, 0], 400 * 400)->list2blob()
    call popup_create('', #{image: #{data: img, width: 400, height: 400},
          \ line: 3, col: 20})
    redraw
  END
  call writefile(lines, 'XpopupImageTab', 'D')
  let job = s:StartVimWithImageOnPty()
  try
    call WaitForAssert({-> assert_match(s:sixel_start_pat, s:pty_out)})

    " Clearing the smaller screen must not look at the cells the image had
    " on the bigger one.
    let start = len(s:pty_out)
    call ch_sendraw(job, ":set columns=40 lines=10\<CR>")
    call WaitForAssert({-> assert_match(s:sixel_start_pat, s:pty_out[start :])})
    call assert_equal('run', job_status(job))
  finally
    call job_stop(job, 'kill')
    call WaitForAssert({-> assert_equal('dead', job_status(job))})
  endtry
endfunc

" vim: shiftwidth=2 sts=2 expandtab
