" NetrwFileHandlers: contains various extension-based file handlers for
"               netrw's browser
" Author:	Charles E. Campbell, Jr.
" Date:		Jul 02, 2004
" Version:	2
" ---------------------------------------------------------------------

" NetrwFileHandler_html: handles html when the user hits "x" when the
"                        cursor is atop a *.html file
fun! NetrwFileHandler_html(webpage)
"  call Dfunc("NetrwFileHandler_html(".a:webpage.")")

  let host= substitute(a:webpage,'^\w\+://\%(\w*@\)\=\(\w\+\)/.*$','\1','e')
"  call Decho("host<".host.">")

  if host == hostname() || host == substitute(hostname(),'\..*$','','e')
   let page= substitute(a:webpage,'^\w\+://\%(\w*@\)\=\(\w\+\)/','file://\1/'.expand("$HOME").'/','e')
  else
   let page= substitute(a:webpage,'^\w\+://\%(\w*@\)\=\(\w\+\)/','http://\1/','e')
  endif

  if executable("mozilla")
"  call Decho("executing !mozilla ".page)
   exe "!mozilla ".page
  elseif executable("netscape")
"  call Decho("executing !netscape ".page)
   exe "!netscape ".page
  endif

"  call Dret("NetrwFileHandler_html")
endfun

" ---------------------------------------------------------------------
