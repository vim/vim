" Tests for ":namecolor"

func Test_namecolor()
  " basic test if ":namecolor" doesn't crash
  namecolor rgb=#ffffff name=white
  namecolor rgb=#ffeedd name='a redish white'
  namecolor rgb=#ffffff name=hyphenated-name
  namecolor rgb=#ffffff name=underscored_name

  call assert_equal(0, len(v:errors))
endfunc

func Test_namecolor_invalid_syntax()
  call assert_fails("namecolor rgb=aabbcc", "Syntax error in namecolor")
  call assert_fails("namecolor name='start of a col", "Broken quotes for name=")
  call assert_fails("namecolor name= rgb=", "Syntax error in namecolor")
endfunc

func Test_namecolor_required_args()
  call assert_fails('namecolor', 'Missing ')
  call assert_fails('namecolor name=red', "Missing 'rgb='")
  call assert_fails('namecolor rgb=#ff0000', "Missing 'name='")
endfunc

func Test_namecolor_highlight()
  namecolor rgb=#ffeedd name='a redish white'
  highlight Normal guifg='a redish white'
  call assert_equal(0, len(v:errors))
endfunc

func Test_doc_examples()
 namecolor rgb=#ffdab9 name='peach puff'
 namecolor name=PeachPuff rgb=#ffdab9
endfunc

" vim: shiftwidth=2 sts=2 expandtab
