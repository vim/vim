" Vim syntax support file
" Maintainer: Bram Moolenaar <Bram@vim.org>
" Last Change: 2006 Apr 30
"	       (modified by David Ne\v{c}as (Yeti) <yeti@physics.muni.cz>)
"	       (XHTML support by Panagiotis Issaris <takis@lumumba.luc.ac.be>)

" Transform a file into HTML, using the current syntax highlighting.

" Number lines when explicitely requested or when `number' is set
if exists("html_number_lines")
  let s:numblines = html_number_lines
else
  let s:numblines = &number
endif

" When not in gui we can only guess the colors.
if has("gui_running")
  let s:whatterm = "gui"
else
  let s:whatterm = "cterm"
  if &t_Co == 8
    let s:cterm_color0  = "#808080"
    let s:cterm_color1  = "#ff6060"
    let s:cterm_color2  = "#00ff00"
    let s:cterm_color3  = "#ffff00"
    let s:cterm_color4  = "#8080ff"
    let s:cterm_color5  = "#ff40ff"
    let s:cterm_color6  = "#00ffff"
    let s:cterm_color7  = "#ffffff"
  else
    let s:cterm_color0  = "#000000"
    let s:cterm_color1  = "#c00000"
    let s:cterm_color2  = "#008000"
    let s:cterm_color3  = "#804000"
    let s:cterm_color4  = "#0000c0"
    let s:cterm_color5  = "#c000c0"
    let s:cterm_color6  = "#008080"
    let s:cterm_color7  = "#c0c0c0"
    let s:cterm_color8  = "#808080"
    let s:cterm_color9  = "#ff6060"
    let s:cterm_color10 = "#00ff00"
    let s:cterm_color11 = "#ffff00"
    let s:cterm_color12 = "#8080ff"
    let s:cterm_color13 = "#ff40ff"
    let s:cterm_color14 = "#00ffff"
    let s:cterm_color15 = "#ffffff"
  endif
endif

" Return good color specification: in GUI no transformation is done, in
" terminal return RGB values of known colors and empty string on unknown
if s:whatterm == "gui"
  function! s:HtmlColor(color)
    return a:color
  endfun
else
  function! s:HtmlColor(color)
    if exists("s:cterm_color" . a:color)
      execute "return s:cterm_color" . a:color
    else
      return ""
    endif
  endfun
endif

if !exists("html_use_css")
  " Return opening HTML tag for given highlight id
  function! s:HtmlOpening(id)
    let a = ""
    if synIDattr(a:id, "inverse")
      " For inverse, we always must set both colors (and exchange them)
      let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
      let a = a . '<span style="background-color: ' . ( x != "" ? x : s:fgc ) . '">'
      let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
      let a = a . '<font color="' . ( x != "" ? x : s:bgc ) . '">'
    else
      let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
      if x != "" | let a = a . '<span style="background-color: ' . x . '">' | endif
      let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
      if x != "" | let a = a . '<font color="' . x . '">' | endif
    endif
    if synIDattr(a:id, "bold") | let a = a . "<b>" | endif
    if synIDattr(a:id, "italic") | let a = a . "<i>" | endif
    if synIDattr(a:id, "underline") | let a = a . "<u>" | endif
    return a
  endfun

  " Return closing HTML tag for given highlight id
  function s:HtmlClosing(id)
    let a = ""
    if synIDattr(a:id, "underline") | let a = a . "</u>" | endif
    if synIDattr(a:id, "italic") | let a = a . "</i>" | endif
    if synIDattr(a:id, "bold") | let a = a . "</b>" | endif
    if synIDattr(a:id, "inverse")
      let a = a . '</font></span>'
    else
      let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
      if x != "" | let a = a . '</font>' | endif
      let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
      if x != "" | let a = a . '</span>' | endif
    endif
    return a
  endfun
endif

" Return HTML valid characters enclosed in a span of class style_name with
" unprintable characters expanded and double spaces replaced as necessary.
function! s:HtmlFormat(text, style_name)
  " Replace unprintable characters
  let formatted = strtrans(a:text)

  " Replace the reserved html characters
  let formatted = substitute(substitute(substitute(substitute(substitute(formatted, '&', '\&amp;', 'g'), '<', '\&lt;', 'g'), '>', '\&gt;', 'g'), '"', '\&quot;', 'g'), "\x0c", '<hr class="PAGE-BREAK">', 'g')

  " Replace double spaces and leading spaces
  if ' ' != s:HtmlSpace
    let formatted = substitute(formatted, '  ', s:HtmlSpace . s:HtmlSpace, 'g')
    let formatted = substitute(formatted, '^ ', s:HtmlSpace, 'g')
  endif

  " Enclose in a span of class style_name
  let formatted = '<span class="' . a:style_name . '">' . formatted . '</span>'

  " Add the class to class list if it's not there yet
  let s:id = hlID(a:style_name)
  if stridx(s:idlist, "," . s:id . ",") == -1
    let s:idlist = s:idlist . s:id . ","
  endif

  return formatted
endfun

" Return CSS style describing given highlight id (can be empty)
function! s:CSS1(id)
  let a = ""
  if synIDattr(a:id, "inverse")
    " For inverse, we always must set both colors (and exchange them)
    let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
    let a = a . "color: " . ( x != "" ? x : s:bgc ) . "; "
    let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
    let a = a . "background-color: " . ( x != "" ? x : s:fgc ) . "; "
  else
    let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
    if x != "" | let a = a . "color: " . x . "; " | endif
    let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
    if x != "" | let a = a . "background-color: " . x . "; " | endif
  endif
  if synIDattr(a:id, "bold") | let a = a . "font-weight: bold; " | endif
  if synIDattr(a:id, "italic") | let a = a . "font-style: italic; " | endif
  if synIDattr(a:id, "underline") | let a = a . "text-decoration: underline; " | endif
  return a
endfun

" Figure out proper MIME charset from the 'encoding' option.
if exists("html_use_encoding")
  let s:html_encoding = html_use_encoding
else
  let s:vim_encoding = &encoding
  if s:vim_encoding =~ '^8bit\|^2byte'
    let s:vim_encoding = substitute(s:vim_encoding, '^8bit-\|^2byte-', '', '')
  endif
  if s:vim_encoding == 'latin1'
    let s:html_encoding = 'iso-8859-1'
  elseif s:vim_encoding =~ "^cp12"
    let s:html_encoding = substitute(s:vim_encoding, 'cp', 'windows-', '')
  elseif s:vim_encoding == 'sjis'
    let s:html_encoding = 'Shift_JIS'
  elseif s:vim_encoding == 'big5'
    let s:html_encoding = "Big5"
  elseif s:vim_encoding == 'euc-cn'
    let s:html_encoding = 'GB_2312-80'
  elseif s:vim_encoding == 'euc-tw'
    let s:html_encoding = ""
  elseif s:vim_encoding =~ '^euc\|^iso\|^koi'
    let s:html_encoding = substitute(s:vim_encoding, '.*', '\U\0', '')
  elseif s:vim_encoding == 'cp949'
    let s:html_encoding = 'KS_C_5601-1987'
  elseif s:vim_encoding == 'cp936'
    let s:html_encoding = 'GBK'
  elseif s:vim_encoding =~ '^ucs\|^utf'
    let s:html_encoding = 'UTF-8'
  else
    let s:html_encoding = ""
  endif
endif


" Set some options to make it work faster.
" Don't report changes for :substitute, there will be many of them.
let s:old_title = &title
let s:old_icon = &icon
let s:old_et = &l:et
let s:old_report = &report
let s:old_search = @/
set notitle noicon
setlocal et
set report=1000000

" Split window to create a buffer with the HTML file.
let s:orgbufnr = winbufnr(0)
if expand("%") == ""
  new Untitled.html
else
  new %.html
endif
let s:newwin = winnr()
let s:orgwin = bufwinnr(s:orgbufnr)

set modifiable
%d
let s:old_paste = &paste
set paste
let s:old_magic = &magic
set magic

if exists("use_xhtml")
  if s:html_encoding != ""
    exe "normal!  a<?xml version=\"1.0\" encoding=\"" . s:html_encoding . "\"?>\n\e"
  else
    exe "normal! a<?xml version=\"1.0\"?>\n\e"
  endif
  let s:tag_close = '/>'
else
  let s:tag_close = '>'
endif

let s:HtmlSpace = ' '
let s:LeadingSpace = ' '
let s:HtmlEndline = ''
if exists("html_no_pre")
  let s:HtmlEndline = '<br' . s:tag_close
  if exists("use_xhtml")
    let s:LeadingSpace = '&#x20;'
  else
    let s:LeadingSpace = '&nbsp;'
  endif
  let s:HtmlSpace = '\' . s:LeadingSpace
endif

" HTML header, with the title and generator ;-). Left free space for the CSS,
" to be filled at the end.
exe "normal! a<html>\n\e"
exe "normal! a<head>\n<title>" . expand("%:p:~") . "</title>\n\e"
exe "normal! a<meta name=\"Generator\" content=\"Vim/" . v:version/100 . "." . v:version %100 . '"' . s:tag_close . "\n\e"
if s:html_encoding != ""
  exe "normal! a<meta http-equiv=\"content-type\" content=\"text/html; charset=" . s:html_encoding . '"' . s:tag_close . "\n\e"
endif
if exists("html_use_css")
  exe "normal! a<style type=\"text/css\">\n<!--\n-->\n</style>\n\e"
endif
if exists("html_no_pre")
  if exists("use_xhtml")
    exe "normal! a</head>\n<body>\n<p>\n\e"
  else
    exe "normal! a</head>\n<body>\n\e"
  endif
else
  if exists("use_xhtml")
    exe "normal! a</head>\n<body>\n<p>\n<pre>\n\e"
  else
    exe "normal! a</head>\n<body>\n<pre>\n\e"
  endif
endif

exe s:orgwin . "wincmd w"

" List of all id's
let s:idlist = ","

" Loop over all lines in the original text.
" Use html_start_line and html_end_line if they are set.
if exists("html_start_line")
  let s:lnum = html_start_line
  if s:lnum < 1 || s:lnum > line("$")
    let s:lnum = 1
  endif
else
  let s:lnum = 1
endif
if exists("html_end_line")
  let s:end = html_end_line
  if s:end < s:lnum || s:end > line("$")
    let s:end = line("$")
  endif
else
  let s:end = line("$")
endif

if has('folding') && !exists('html_ignore_folding')
  let s:foldfillchar = &fillchars[matchend(&fillchars, 'fold:')]
  if s:foldfillchar == ''
    let s:foldfillchar = '-'
  endif
endif
let s:difffillchar = &fillchars[matchend(&fillchars, 'diff:')]
if s:difffillchar == ''
  let s:difffillchar = '-'
endif


while s:lnum <= s:end

  " If there are filler lines for diff mode, show these above the line.
  let s:filler = diff_filler(s:lnum)
  if s:filler > 0
    let s:n = s:filler
    while s:n > 0
      if s:numblines
	" Indent if line numbering is on
	let s:new = repeat(s:LeadingSpace, strlen(s:end) + 1) . repeat(s:difffillchar, 3)
      else
	let s:new = repeat(s:difffillchar, 3)
      endif

      if s:n > 2 && s:n < s:filler && !exists("html_whole_filler")
	let s:new = s:new . " " . s:filler . " inserted lines "
	let s:n = 2
      endif

      if !exists("html_no_pre")
	" HTML line wrapping is off--go ahead and fill to the margin
	let s:new = s:new . repeat(s:difffillchar, &columns - strlen(s:new))
      endif

      let s:new = s:HtmlFormat(s:new, "DiffDelete")
      exe s:newwin . "wincmd w"
      exe "normal! a" . s:new . s:HtmlEndline . "\n\e"
      exe s:orgwin . "wincmd w"

      let s:n = s:n - 1
    endwhile
    unlet s:n
  endif
  unlet s:filler

  " Start the line with the line number.
  if s:numblines
    let s:new = repeat(' ', strlen(s:end) - strlen(s:lnum)) . s:lnum . ' '
  else
    let s:new = ""
  endif

  if has('folding') && !exists('html_ignore_folding') && foldclosed(s:lnum) > -1
    "
    " This is the beginning of a folded block
    "
    let s:new = s:new . foldtextresult(s:lnum)
    if !exists("html_no_pre")
      " HTML line wrapping is off--go ahead and fill to the margin
      let s:new = s:new . repeat(s:foldfillchar, &columns - strlen(s:new))
    endif

    let s:new = s:HtmlFormat(s:new, "Folded")

    " Skip to the end of the fold
    let s:lnum = foldclosedend(s:lnum)

  else
    "
    " A line that is not folded.
    "
    let s:line = getline(s:lnum)

    let s:len = strlen(s:line)

    if s:numblines
      let s:new = s:HtmlFormat(s:new, "lnr")
    endif

    " Get the diff attribute, if any.
    let s:diffattr = diff_hlID(s:lnum, 1)

    " Loop over each character in the line
    let s:col = 1
    while s:col <= s:len || (s:col == 1 && s:diffattr)
      let s:startcol = s:col " The start column for processing text
      if s:diffattr
	let s:id = diff_hlID(s:lnum, s:col)
	let s:col = s:col + 1
	" Speed loop (it's small - that's the trick)
	" Go along till we find a change in hlID
	while s:col <= s:len && s:id == diff_hlID(s:lnum, s:col) | let s:col = s:col + 1 | endwhile
	if s:len < &columns && !exists("html_no_pre")
	  " Add spaces at the end to mark the changed line.
	  let s:line = s:line . repeat(' ', &columns - s:len)
	  let s:len = &columns
	endif
      else
	let s:id = synID(s:lnum, s:col, 1)
	let s:col = s:col + 1
	" Speed loop (it's small - that's the trick)
	" Go along till we find a change in synID
	while s:col <= s:len && s:id == synID(s:lnum, s:col, 1) | let s:col = s:col + 1 | endwhile
      endif

      " Expand tabs
      let s:expandedtab = strpart(s:line, s:startcol - 1, s:col - s:startcol)
      let idx = stridx(s:expandedtab, "\t")
      while idx >= 0
	let i = &ts - ((idx + s:startcol - 1) % &ts)
	let s:expandedtab = substitute(s:expandedtab, '\t', repeat(' ', i), '')
	let idx = stridx(s:expandedtab, "\t")
      endwhile

      " Output the text with the same synID, with class set to {s:id_name}
      let s:id = synIDtrans(s:id)
      let s:id_name = synIDattr(s:id, "name", s:whatterm)
      let s:new = s:new . s:HtmlFormat(s:expandedtab,  s:id_name)
    endwhile
  endif

  exe s:newwin . "wincmd w"
  exe "normal! a" . s:new . s:HtmlEndline . "\n\e"
  exe s:orgwin . "wincmd w"
  let s:lnum = s:lnum + 1
endwhile
" Finish with the last line
exe s:newwin . "wincmd w"
if exists("html_no_pre")
  if exists("use_xhtml")
    exe "normal! a</p>\n</body>\n</html>\e"
  else
    exe "normal! a</body>\n</html>\e"
  endif
else
  if exists("use_xhtml")
    exe "normal! a</pre>\n</p>\n</body>\n</html>\e"
  else
    exe "normal! a</pre>\n</body>\n</html>\e"
  endif
endif


" Now, when we finally know which, we define the colors and styles
if exists("html_use_css")
  1;/<style type="text/+1
endif

" Find out the background and foreground color.
let s:fgc = s:HtmlColor(synIDattr(hlID("Normal"), "fg#", s:whatterm))
let s:bgc = s:HtmlColor(synIDattr(hlID("Normal"), "bg#", s:whatterm))
if s:fgc == ""
  let s:fgc = ( &background == "dark" ? "#ffffff" : "#000000" )
endif
if s:bgc == ""
  let s:bgc = ( &background == "dark" ? "#000000" : "#ffffff" )
endif

" Normal/global attributes
" For Netscape 4, set <body> attributes too, though, strictly speaking, it's
" incorrect.
if exists("html_use_css")
  if exists("html_no_pre")
    execute "normal! A\nbody { color: " . s:fgc . "; background-color: " . s:bgc . "; font-family: Courier, monospace; }\e"
  else
    execute "normal! A\npre { color: " . s:fgc . "; background-color: " . s:bgc . "; }\e"
    yank
    put
    execute "normal! ^cwbody\e"
  endif
else
  if exists("html_no_pre")
    execute '%s:<body>:<body ' . 'bgcolor="' . s:bgc . '" text="' . s:fgc . '" style="font-family\: Courier, monospace;">'
  else
    execute '%s:<body>:<body ' . 'bgcolor="' . s:bgc . '" text="' . s:fgc . '">'
  endif
endif

" Line numbering attributes
if s:numblines
  if exists("html_use_css")
    execute "normal! A\n.lnr { " . s:CSS1(hlID("LineNr")) . "}\e"
  else
    execute '%s+^<span class="lnr">\([^<]*\)</span>+' . s:HtmlOpening(hlID("LineNr")) . '\1' . s:HtmlClosing(hlID("LineNr")) . '+g'
  endif
endif

" Gather attributes for all other classes
let s:idlist = strpart(s:idlist, 1)
while s:idlist != ""
  let s:attr = ""
  let s:col = stridx(s:idlist, ",")
  let s:id = strpart(s:idlist, 0, s:col)
  let s:idlist = strpart(s:idlist, s:col + 1)
  let s:attr = s:CSS1(s:id)
  let s:id_name = synIDattr(s:id, "name", s:whatterm)
  " If the class has some attributes, export the style, otherwise DELETE all
  " its occurences to make the HTML shorter
  if s:attr != ""
    if exists("html_use_css")
      execute "normal! A\n." . s:id_name . " { " . s:attr . "}"
    else
      execute '%s+<span class="' . s:id_name . '">\([^<]*\)</span>+' . s:HtmlOpening(s:id) . '\1' . s:HtmlClosing(s:id) . '+g'
    endif
  else
    execute '%s+<span class="' . s:id_name . '">\([^<]*\)</span>+\1+g'
    if exists("html_use_css")
      1;/<style type="text/+1
    endif
  endif
endwhile

" Add hyperlinks
%s+\(https\=://\S\{-}\)\(\([.,;:}]\=\(\s\|$\)\)\|[\\"'<>]\|&gt;\|&lt;\|&quot;\)+<a href="\1">\1</a>\2+ge

" The DTD
if exists("html_use_css")
  if exists("use_xhtml")
    exe "normal! gg$a\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\e"
  else
    exe "normal! gg0i<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n\e"
  endif
endif

if exists("use_xhtml")
  exe "normal! gg/<html/e\na xmlns=\"http://www.w3.org/1999/xhtml\"\e"
endif

" Cleanup
%s:\s\+$::e

" Restore old settings
let &report = s:old_report
let &title = s:old_title
let &icon = s:old_icon
let &paste = s:old_paste
let &magic = s:old_magic
let @/ = s:old_search
exe s:orgwin . "wincmd w"
let &l:et = s:old_et
exe s:newwin . "wincmd w"

" Save a little bit of memory (worth doing?)
unlet s:old_et s:old_paste s:old_icon s:old_report s:old_title s:old_search
unlet s:whatterm s:idlist s:lnum s:end s:fgc s:bgc s:old_magic
unlet! s:col s:id s:attr s:len s:line s:new s:expandedtab s:numblines
unlet s:orgwin s:newwin s:orgbufnr
if !v:profiling
  delfunc s:HtmlColor
  delfunc s:HtmlFormat
  delfunc s:CSS1
  if !exists("html_use_css")
    delfunc s:HtmlOpening
    delfunc s:HtmlClosing
  endif
endif
silent! unlet s:diffattr s:difffillchar s:foldfillchar s:HtmlSpace s:LeadingSpace s:HtmlEndline
