" Tests for solarized colorscheme

func Test_colorscheme()
  let colorscheme_saved = exists('g:colors_name') ? g:colors_name : 'default'

  colorscheme solarized
  redraw!
  sleep 200m
  call assert_equal('solarized', g:colors_name)
  exec 'colorscheme' colorscheme_saved
  redraw!
endfunc
