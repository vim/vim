" NetrwFileHandlers: contains various extension-based file handlers for
"                    netrw's browsers' x command
" Author:	Charles E. Campbell, Jr.
" Date:		Jul 06, 2004
" Version:	3
" ---------------------------------------------------------------------

" NetrwFileHandler_html: handles html when the user hits "x" when the
"                        cursor is atop a *.html file
fun! NetrwFileHandler_html(pagefile)
"  call Dfunc("NetrwFileHandler_html(".a:pagefile.")")

  let page= substitute(a:pagefile,'^','file://','')

  if executable("mozilla")
"   call Decho("executing !mozilla ".page)
   exe "!mozilla ".page
  elseif executable("netscape")
"   call Decho("executing !netscape ".page)
   exe "!netscape ".page
  else
"   call Dret("NetrwFileHandler_html 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_html 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_htm: handles html when the user hits "x" when the
"                        cursor is atop a *.htm file
fun! NetrwFileHandler_htm(pagefile)
"  call Dfunc("NetrwFileHandler_htm(".a:pagefile.")")

  let page= substitute(a:pagefile,'^','file://','')

  if executable("mozilla")
"   call Decho("executing !mozilla ".page)
   exe "!mozilla ".page
  elseif executable("netscape")
"   call Decho("executing !netscape ".page)
   exe "!netscape ".page
  else
"   call Dret("NetrwFileHandler_htm 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_htm 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_jpg:
fun! NetrwFileHandler_jpg(jpgfile)
"  call Dfunc("NetrwFileHandler_jpg(jpgfile<".a:jpgfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:jpgfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT ".a:jpgfile
  else
"   call Dret("NetrwFileHandler_jpg 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_jpg 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_gif:
fun! NetrwFileHandler_gif(giffile)
"  call Dfunc("NetrwFileHandler_gif(giffile<".a:giffile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:giffile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT ".a:giffile
  else
"   call Dret("NetrwFileHandler_gif 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_gif 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_pnm:
fun! NetrwFileHandler_pnm(pnmfile)
"  call Dfunc("NetrwFileHandler_pnm(pnmfile<".a:pnmfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:pnmfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT ".a:pnmfile
  else
"   call Dret("NetrwFileHandler_pnm 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_pnm 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_bmp:
fun! NetrwFileHandler_bmp(bmpfile)
"  call Dfunc("NetrwFileHandler_bmp(bmpfile<".a:bmpfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:bmpfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT ".a:bmpfile
  else
"   call Dret("NetrwFileHandler_bmp 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_bmp 1")
  return 1
endfun

" ---------------------------------------------------------------------
