" Vim :runtime command


runtime       plugin/**/foo.vim
runtime START plugin/**/foo.vim
runtime OPT   plugin/**/foo.vim
runtime PACK  plugin/**/foo.vim
runtime ALL   plugin/**/foo.vim

runtime!       plugin/**/foo.vim
runtime! START plugin/**/foo.vim
runtime! OPT   plugin/**/foo.vim
runtime! PACK  plugin/**/foo.vim
runtime! ALL   plugin/**/foo.vim

runtime plugin/**/foo.vim
      "\ comment
      \ plugin/**/bar.vim

runtime plugin/**/foo.vim " comment
runtime plugin/**/foo.vim | echo "..."


def Vim9Context()
  runtime       plugin/**/foo.vim
  runtime START plugin/**/foo.vim
  runtime OPT   plugin/**/foo.vim
  runtime PACK  plugin/**/foo.vim
  runtime ALL   plugin/**/foo.vim

  runtime!       plugin/**/foo.vim
  runtime! START plugin/**/foo.vim
  runtime! OPT   plugin/**/foo.vim
  runtime! PACK  plugin/**/foo.vim
  runtime! ALL   plugin/**/foo.vim

  runtime plugin/**/foo.vim
	#\ comment
	\ plugin/**/bar.vim

  runtime plugin/**/foo.vim # comment
  runtime plugin/**/foo.vim | echo "..."
enddef

