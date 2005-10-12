" netrwFileHandlers: contains various extension-based file handlers for
"                    netrw's browsers' x command ("eXecute launcher")
" Author:	Charles E. Campbell, Jr.
" Date:		Oct 12, 2005
" Version:	7
" Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
"               Permission is hereby granted to use and distribute this code,
"               with or without modifications, provided that this copyright
"               notice is copied with it. Like anything else that's free,
"               netrwFileHandlers.vim is provided *as is* and comes with no
"               warranty of any kind, either expressed or implied. In no
"               event will the copyright holder be liable for any damages
"               resulting from the use of this software.
"
" Rom 6:23 (WEB) For the wages of sin is death, but the free gift of God {{{1
"                is eternal life in Christ Jesus our Lord.

" ---------------------------------------------------------------------
" Load Once: {{{1
if exists("g:loaded_netrwFileHandlers") || &cp
 finish
endif
let s:keepcpo= &cpo
set cpo&vim
let g:loaded_netrwFileHandlers= "v7"

" ---------------------------------------------------------------------
" netrwFileHandlers#Init: {{{1
"    This functions is here to allow a call to this function to autoload
"    the netrwFileHandlers.vim file
fun! netrwFileHandlers#Init()
"  call Dfunc("netrwFileHandlers#Init()")
"  call Dret("netrwFileHandlers#Init")
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_html: handles html when the user hits "x" when the {{{1
"                        cursor is atop a *.html file
fun! netrwFileHandlers#NFH_html(pagefile)
"  call Dfunc("netrwFileHandlers#NFH_html(".a:pagefile.")")

  let page= substitute(a:pagefile,'^','file://','')

  if executable("mozilla")
"   call Decho("executing !mozilla ".page)
   exe "!mozilla \"".page.'"'
  elseif executable("netscape")
"   call Decho("executing !netscape ".page)
   exe "!netscape \"".page.'"'
  else
"   call Dret("netrwFileHandlers#NFH_html 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_html 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_htm: handles html when the user hits "x" when the {{{1
"                        cursor is atop a *.htm file
fun! netrwFileHandlers#NFH_htm(pagefile)
"  call Dfunc("netrwFileHandlers#NFH_htm(".a:pagefile.")")

  let page= substitute(a:pagefile,'^','file://','')

  if executable("mozilla")
"   call Decho("executing !mozilla ".page)
   exe "!mozilla \"".page.'"'
  elseif executable("netscape")
"   call Decho("executing !netscape ".page)
   exe "!netscape \"".page.'"'
  else
"   call Dret("netrwFileHandlers#NFH_htm 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_htm 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_jpg: {{{1
fun! netrwFileHandlers#NFH_jpg(jpgfile)
"  call Dfunc("netrwFileHandlers#NFH_jpg(jpgfile<".a:jpgfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:jpgfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
"   call Decho("silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT ".escape(a:jpgfile," []|'"))
   exe "!".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:jpgfile.'"'
  else
"   call Dret("netrwFileHandlers#NFH_jpg 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_jpg 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_gif: {{{1
fun! netrwFileHandlers#NFH_gif(giffile)
"  call Dfunc("netrwFileHandlers#NFH_gif(giffile<".a:giffile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:giffile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:giffile.'"'
  else
"   call Dret("netrwFileHandlers#NFH_gif 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_gif 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_png: {{{1
fun! netrwFileHandlers#NFH_png(pngfile)
"  call Dfunc("netrwFileHandlers#NFH_png(pngfile<".a:pngfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:pngfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:pngfile.'"'
  else
"   call Dret("netrwFileHandlers#NFH_png 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_png 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_pnm: {{{1
fun! netrwFileHandlers#NFH_pnm(pnmfile)
"  call Dfunc("netrwFileHandlers#NFH_pnm(pnmfile<".a:pnmfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:pnmfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:pnmfile.'"'
  else
"   call Dret("netrwFileHandlers#NFH_pnm 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_pnm 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_bmp: visualize bmp files {{{1
fun! netrwFileHandlers#NFH_bmp(bmpfile)
"  call Dfunc("netrwFileHandlers#NFH_bmp(bmpfile<".a:bmpfile.">)")

  if executable("gimp")
   exe "silent! !gimp -s ".a:bmpfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !".expand("$SystemRoot")."/SYSTEM32/MSPAINT \"".a:bmpfile.'"'
  else
"   call Dret("netrwFileHandlers#NFH_bmp 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_bmp 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_pdf: visualize pdf files {{{1
fun! netrwFileHandlers#NFH_pdf(pdf)
"  " call Dfunc("netrwFileHandlers#NFH_pdf(pdf<".a:pdf.">)")
  if executable("gs")
   exe 'silent! !gs "'.a:pdf.'"'
  else
"   " call Dret("netrwFileHandlers#NFH_pdf 0")
   return 0
  endif

"  " call Dret("netrwFileHandlers#NFH_pdf 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_doc: visualize doc files {{{1
fun! netrwFileHandlers#NFH_doc(doc)
"  " call Dfunc("netrwFileHandlers#NFH_doc(doc<".a:doc.">)")

  if executable("oowriter")
   exe 'silent! !oowriter "'.a:doc.'"'
   redraw!
  else
"   " call Dret("netrwFileHandlers#NFH_doc 0")
   return 0
  endif

"  " call Dret("netrwFileHandlers#NFH_doc 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_sxw: visualize sxw files {{{1
fun! netrwFileHandlers#NFH_sxw(sxw)
"  " call Dfunc("netrwFileHandlers#NFH_sxw(sxw<".a:sxw.">)")

  if executable("oowriter")
   exe 'silent! !oowriter "'.a:sxw.'"'
   redraw!
  else
"   " call Dret("netrwFileHandlers#NFH_sxw 0")
   return 0
  endif

"  " call Dret("netrwFileHandlers#NFH_sxw 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_xls: visualize xls files {{{1
fun! netrwFileHandlers#NFH_xls(xls)
"  " call Dfunc("netrwFileHandlers#NFH_xls(xls<".a:xls.">)")

  if executable("oocalc")
   exe 'silent! !oocalc "'.a:xls.'"'
   redraw!
  else
"   " call Dret("netrwFileHandlers#NFH_xls 0")
   return 0
  endif

"  " call Dret("netrwFileHandlers#NFH_xls 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_ps: handles PostScript files {{{1
fun! netrwFileHandlers#NFH_ps(ps)
"  call Dfunc("netrwFileHandlers#NFH_ps()")
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
"   call Dret("netrwFileHandlers#NFH_ps 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_ps 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_eps: handles encapsulated PostScript files {{{1
fun! netrwFileHandlers#NFH_eps(eps)
"  call Dfunc("netrwFileHandlers#NFH_ps()")
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
"   call Dret("netrwFileHandlers#NFH_ps 0")
   return 0
  endif
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_fig: handles xfig files {{{1
fun! netrwFileHandlers#NFH_fig(fig)
"  call Dfunc("netrwFileHandlers#NFH_fig()")
  if executable("xfig")
   exe "silent! !xfig ".a:fig
   redraw!
  else
"   call Dret("netrwFileHandlers#NFH_fig 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_fig 1")
  return 1
endfun

" ---------------------------------------------------------------------
" netrwFileHandlers#NFH_obj: handles tgif's obj files {{{1
fun! netrwFileHandlers#NFH_obj(obj)
"  call Dfunc("netrwFileHandlers#NFH_obj()")
  if has("unix") && executable("tgif")
   exe "silent! !tgif ".a:obj
   redraw!
  else
"   call Dret("netrwFileHandlers#NFH_obj 0")
   return 0
  endif

"  call Dret("netrwFileHandlers#NFH_obj 1")
  return 1
endfun

let &cpo= s:keepcpo
" ---------------------------------------------------------------------
"  Modelines: {{{1
"  vim: ts=4 fdm=marker
