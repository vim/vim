" NetrwFileHandlers: contains various extension-based file handlers for
"                    netrw's browsers' x command ("eXecute launcher")
" Author:   Charles E. Campbell, Jr.
" Date:     Aug 31, 2004
" Version:  3

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
  let page = substitute(a:pagefile, '^', 'file://', '')

  if executable("mozilla")
    exe "!mozilla \"" . page . '"'
  elseif executable("netscape")
    exe "!netscape \"" . page . '"'
  else
    return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_htm: handles html when the user hits "x" when the {{{1
"                        cursor is atop a *.htm file
fun! NetrwFileHandler_htm(pagefile)
  let page = substitute(a:pagefile, '^', 'file://', '')

  if executable("mozilla")
    exe "!mozilla \"" . page . '"'
  elseif executable("netscape")
    exe "!netscape \"" . page . '"'
  else
    return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_jpg: {{{1
fun! NetrwFileHandler_jpg(jpgfile)
  if executable("gimp")
    exe "silent! !gimp -s " . a:jpgfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
    exe "!" . expand("$SystemRoot") . "/SYSTEM32/MSPAINT \"" . a:jpgfile . '"'
  else
    return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_gif: {{{1
fun! NetrwFileHandler_gif(giffile)
  if executable("gimp")
   exe "silent! !gimp -s " . a:giffile
  elseif executable(expand("$SystemRoot") . "/SYSTEM32/MSPAINT.EXE")
   exe "silent! !" . expand("$SystemRoot") . "/SYSTEM32/MSPAINT \"" . a:giffile . '"'
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_png: {{{1
fun! NetrwFileHandler_png(pngfile)
  if executable("gimp")
   exe "silent! !gimp -s " . a:pngfile
  elseif executable(expand("$SystemRoot") . "/SYSTEM32/MSPAINT.EXE")
   exe "silent! !" . expand("$SystemRoot") . "/SYSTEM32/MSPAINT \"" . a:pngfile . '"'
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_pnm: {{{1
fun! NetrwFileHandler_pnm(pnmfile)
  if executable("gimp")
   exe "silent! !gimp -s " . a:pnmfile
  elseif executable(expand("$SystemRoot") . "/SYSTEM32/MSPAINT.EXE")
   exe "silent! !" . expand("$SystemRoot") . "/SYSTEM32/MSPAINT \"" . a:pnmfile . '"'
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_bmp: visualize bmp files {{{1
fun! NetrwFileHandler_bmp(bmpfile)
  if executable("gimp")
   exe "silent! !gimp -s " . a:bmpfile
  elseif executable(expand("$SystemRoot")."/SYSTEM32/MSPAINT.EXE")
   exe "silent! !" . expand("$SystemRoot") . "/SYSTEM32/MSPAINT \"" . a:bmpfile . '"'
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_pdf: visualize pdf files {{{1
fun! NetrwFileHandler_pdf(pdf)
  if executable("acroread")
   exe 'silent! !acroread "' . a:pdf . '"'
  elseif executable("gs")
   exe 'silent! !gs "' . a:pdf . '"'
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_sxw: visualize sxw files {{{1
fun! NetrwFileHandler_sxw(sxw)
  if executable("gs")
   exe 'silent! !gs "' . a:sxw . '"'
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_doc: visualize doc files {{{1
fun! NetrwFileHandler_doc(doc)
  if executable("oowriter")
   exe 'silent! !oowriter "' . a:doc . '"'
   redraw!
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_sxw: visualize sxw files {{{1
fun! NetrwFileHandler_sxw(sxw)
  if executable("oowriter")
   exe 'silent! !oowriter "' . a:sxw . '"'
   redraw!
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_xls: visualize xls files {{{1
fun! NetrwFileHandler_xls(xls)
  if executable("oocalc")
   exe 'silent! !oocalc "' . a:xls . '"'
   redraw!
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_ps: handles PostScript files {{{1
fun! NetrwFileHandler_ps(ps)
  if executable("gs")
   exe "silent! !gs " . a:ps
   redraw!
  elseif executable("ghostscript")
   exe "silent! !ghostscript " . a:ps
   redraw!
  elseif executable("ghostscript")
   exe "silent! !ghostscript " . a:ps
   redraw!
  elseif executable("gswin32")
   exe "silent! !gswin32 \"" . a:ps . '"'
   redraw!
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_eps: handles encapsulated PostScript files {{{1
fun! NetrwFileHandler_eps(eps)
  if executable("gs")
   exe "silent! !gs " . a:eps
   redraw!
  elseif executable("ghostscript")
   exe "silent! !ghostscript " . a:eps
   redraw!
  elseif executable("ghostscript")
   exe "silent! !ghostscript " . a:eps
   redraw!
  elseif executable("gswin32")
   exe "silent! !gswin32 \"" . a:eps . '"'
   redraw!
  else
   return 0
  endif
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_fig: handles xfig files {{{1
fun! NetrwFileHandler_fig(fig)
  if executable("xfig")
   exe "silent! !xfig " . a:fig
   redraw!
  else
   return 0
  endif

  return 1
endfun

" ---------------------------------------------------------------------
" NetrwFileHandler_obj: handles tgif's obj files {{{1
fun! NetrwFileHandler_obj(obj)
  if has("unix") && executable("tgif")
   exe "silent! !tgif " . a:obj
   redraw!
  else
   return 0
  endif

  return 1
endfun


" ---------------------------------------------------------------------
"  vim: fdm=marker
