" Vim filetype plugin file
" Language: VHDL
" Maintainer:   R.Shankar (shankar at txc.stpn.soft.net)
" Last Change:  Tue Oct 8


" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

" Set 'formatoptions' to break comment lines but not other lines,
" and insert the comment leader when hitting <CR> or using "o".
"setlocal fo-=t fo+=croqlm1

" Set 'comments' to format dashed lists in comments.
"setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,://

" Format comments to be up to 78 characters long
setlocal tw=75

set cpo-=C

" Win32 can filter files in the browse dialog
"if has("gui_win32") && !exists("b:browsefilter")
"  let b:browsefilter = "Verilog Source Files (*.v)\t*.v\n" .
"   \ "All Files (*.*)\t*.*\n"
"endif

" Let the matchit plugin know what items can be matched.
if ! exists("b:match_words")  &&  exists("loaded_matchit")
  let b:match_ignorecase=1
  let s:notend = '\%(\<end\s\+\)\@<!'
  let b:match_words=
  \ s:notend . '\<if\>:\<elsif\>:\<else\>:\<end\>\s\+\<if\>,' .
  \ s:notend . '\<case\>:\<when\>:\<end\>\s\+\<case\>,' .
  \ s:notend . '\<process\>:\<end\>\s\+\<process\>'
endif
