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

" vim: shiftwidth=2 sts=2 expandtab
