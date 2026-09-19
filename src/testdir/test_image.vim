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

" vim: shiftwidth=2 sts=2 expandtab
