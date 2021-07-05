" Tests for ":namecolor"

func Test_colornames()
  let v:colornames['a redish white'] = '#ffeedd'
  highlight Normal guifg='a redish white'
endfunc

func Test_colornames_overwrite()
  highlight Normal guifg='rebecca purple' guibg='rebecca purple'
  let v:colornames['rebecca purple'] = '#550099'
  highlight Normal guifg='rebecca purple' guibg='rebecca purple'
endfunc

func Test_non_overwrite()
  call assert_fails("let v:colornames = {}", 'E46:')

  let v:colornames['x1'] = '#111111'
  call assert_equal(v:colornames['x1'], '#111111')
  unlet v:colornames['x1']
  call assert_fails("echo v:colornames['x1']")
endfunc

" vim: shiftwidth=2 sts=2 expandtab
