" NetrwFileHandlers: contains various extension-based file handlers for
"                    netrw's browsers' x command ("eXecute launcher")
" Author:	Charles E. Campbell, Jr.
" Date:		Aug 31, 2004
" Version:	3

" ---------------------------------------------------------------------
" Prevent Reloading: {{{1
if exists("g:loaded_netrwfilehandlers") || &cp
 finish
endif
let g:loaded_netrwfilehandlers= "v3"

" ---------------------------------------------------------------------
" NetrwFileHandler_html: handles html when the user hits "x" when the {{{1
"                        cursor is atop a *.html file
fun! NetrwFileHandler_html(pagefile)
"  call Dfunc("NetrwFileHandler_html(".a:pagefile.")")

  let page= substitute(a:pagefile,'^','file://','')

  if executable("mozilla")
"   call Decho("executing !mozilla ".page)
   exe "!mozilla \"".page.'"'
  elseif executable("netscape")
"   call Decho("executing !netscape ".page)
   exe "!netscape \"".page.'"'
  else
"   call Dret("NetrwFileHandler_html 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_html 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_htm: handles html when the user hits "x" when the {{{1
"                        cursor is atop a *.htm file
fun! NetrwFileHandler_htm(pagefile)
"  call Dfunc("NetrwFileHandler_htm(".a:pagefile.")")

  let page= substitute(a:pagefile,'^','file://','')

  if executable("mozilla")
"   call Decho("executing !mozilla ".page)
   exe "!mozilla \"".page.'"'
  elseif executable("netscape")
"   call Decho("executing !netscape ".page)
   exe "!netscape \"".page.'"'
  else
"   call Dret("NetrwFileHandler_htm 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_htm 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_jpg: {{{1
fun! NetrwFileHandler_jpg(jpgfile)
"  call Dfunc("NetrwFileHandler_jpg(jpgfile<".a:jpgfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:jpgfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
"   call Decho("silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT ".escape(a:jpgfile," []|'"))
   exe "!".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:jpgfile.'"'
  else
"   call Dret("NetrwFileHandler_jpg 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_jpg 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_gif: {{{1
fun! NetrwFileHandler_gif(giffile)
"  call Dfunc("NetrwFileHandler_gif(giffile<".a:giffile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:giffile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:giffile.'"'
  else
"   call Dret("NetrwFileHandler_gif 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_gif 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_png: {{{1
fun! NetrwFileHandler_png(pngfile)
"  call Dfunc("NetrwFileHandler_png(pngfile<".a:pngfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:pngfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:pngfile.'"'
  else
"   call Dret("NetrwFileHandler_png 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_png 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_pnm: {{{1
fun! NetrwFileHandler_pnm(pnmfile)
"  call Dfunc("NetrwFileHandler_pnm(pnmfile<".a:pnmfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:pnmfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:pnmfile.'"'
  else
"   call Dret("NetrwFileHandler_pnm 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_pnm 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_bmp: visualize bmp files {{{1
fun! NetrwFileHandler_bmp(bmpfile)
"  call Dfunc("NetrwFileHandler_bmp(bmpfile<".a:bmpfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:bmpfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:bmpfile.'"'
  else
"   call Dret("NetrwFileHandler_bmp 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_bmp 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_pdf: visualize pdf files {{{1
fun! NetrwFileHandler_pdf(pdf)
"  " call Dfunc("NetrwFileHandler_pdf(pdf<".a:pdf.">)")
  if executable("gs")
   exe 'silent! !gs "'.a:pdf.'"'
  else
"   " call Dret("NetrwFileHandler_pdf 0")
   return 0
  endif

"  " call Dret("NetrwFileHandler_pdf 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_sxw: visualize sxw files {{{1
fun! NetrwFileHandler_sxw(sxw)
"  " call Dfunc("NetrwFileHandler_sxw(sxw<".a:sxw.">)")
  if executable("gs")
   exe 'silent! !gs "'.a:sxw.'"'
  else
"   " call Dret("NetrwFileHandler_sxw 0")
   return 0
  endif

"  " call Dret("NetrwFileHandler_sxw 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_doc: visualize doc files {{{1
fun! NetrwFileHandler_doc(doc)
"  " call Dfunc("NetrwFileHandler_doc(doc<".a:doc.">)")

  if executable("oowriter")
   exe 'silent! !oowriter "'.a:doc.'"'
   redraw!
  else
"   " call Dret("NetrwFileHandler_doc 0")
   return 0
  endif

"  " call Dret("NetrwFileHandler_doc 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_sxw: visualize sxw files {{{1
fun! NetrwFileHandler_sxw(sxw)
"  " call Dfunc("NetrwFileHandler_sxw(sxw<".a:sxw.">)")

  if executable("oowriter")
   exe 'silent! !oowriter "'.a:sxw.'"'
   redraw!
  else
"   " call Dret("NetrwFileHandler_sxw 0")
   return 0
  endif

"  " call Dret("NetrwFileHandler_sxw 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_xls: visualize xls files {{{1
fun! NetrwFileHandler_xls(xls)
"  " call Dfunc("NetrwFileHandler_xls(xls<".a:xls.">)")

  if executable("oocalc")
   exe 'silent! !oocalc "'.a:xls.'"'
   redraw!
  else
"   " call Dret("NetrwFileHandler_xls 0")
   return 0
  endif

"  " call Dret("NetrwFileHandler_xls 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_ps: handles PostScript files {{{1
fun! NetrwFileHandler_ps(ps)
"  call Dfunc("NetrwFileHandler_ps()")
  if executable("gs")
   exe "silent! !gs ".a:ps
   redraw!
  elseif executable("ghostscript")
   exe "silent! !ghostscript ".a:ps
   redraw!
  elseif executable("ghostscript")
   exe "silent! !ghostscript ".a:ps
   redraw!
  elseif executable("gswin32")
   exe "silent! !gswin32 \"".a:ps.'"'
   redraw!
  else
"   call Dret("NetrwFileHandler_ps 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_ps 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_eps: handles encapsulated PostScript files {{{1
fun! NetrwFileHandler_eps(eps)
"  call Dfunc("NetrwFileHandler_ps()")
  if executable("gs")
   exe "silent! !gs ".a:eps
   redraw!
  elseif executable("ghostscript")
   exe "silent! !ghostscript ".a:eps
   redraw!
  elseif executable("ghostscript")
   exe "silent! !ghostscript ".a:eps
   redraw!
  elseif executable("gswin32")
   exe "silent! !gswin32 \"".a:eps.'"'
   redraw!
  else
"   call Dret("NetrwFileHandler_ps 0")
   return 0
  endif
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_fig: handles xfig files {{{1
fun! NetrwFileHandler_fig(fig)
"  call Dfunc("NetrwFileHandler_fig()")
  if executable("xfig")
   exe "silent! !xfig ".a:fig
   redraw!
  else
"   call Dret("NetrwFileHandler_fig 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_fig 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_obj: handles tgif's obj files {{{1
fun! NetrwFileHandler_obj(obj)
"  call Dfunc("NetrwFileHandler_obj()")
  if has("unix") && executable("tgif")
   exe "silent! !tgif ".a:obj
   redraw!
  else
"   call Dret("NetrwFileHandler_obj 0")
   return 0
  endif

"  call Dret("NetrwFileHandler_obj 1")
  return 1
endfun


" ---------------------------------------------------------------------
"  vim: ts=4 fdm=marker
