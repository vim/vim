" NetrwFileHandlers: contains various extension-based file handlers for
"               netrw's browser
" Author:	Charles E. Campbell, Jr.
" Date:		Jun 25, 2004
" Version:	1
" ---------------------------------------------------------------------

" NetrwFileHandler_html: handles html
fun! NetrwFileHandler_html(webpage)
"  call Dfunc("NetrwFileHandler_html(".a:webpage.")")

  let host= substitute(a:webpage,'^\w\+://\%(\w*@\)\=\(\w\+\)/.*$','\1','e')
"  call Decho("host<".host.">")

  if host == hostname() || host == substitute(hostname(),'\..*$','','e')
   let page= substitute(a:webpage,'^\w\+://\%(\w*@\)\=\(\w\+\)/','file://\1/'.expand("$HOME").'/','e')
  else
   let page= substitute(a:webpage,'^\w\+://\%(\w*@\)\=\(\w\+\)/','http://\1/','e')
  endif
"  call Decho("executing !mozilla ".page)
  exe "!mozilla ".page

"  call Dret("NetrwFileHandler_html")
endfun

" ---------------------------------------------------------------------
