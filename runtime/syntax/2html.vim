" Vim syntax support file
" Maintainer: Bram Moolenaar <Bram@vim.org>
" Last Change: 2009 Jul 14
"	       (modified by David Ne\v{c}as (Yeti) <yeti@physics.muni.cz>)
"	       (XHTML support by Panagiotis Issaris <takis@lumumba.luc.ac.be>)
"	       (made w3 compliant by Edd Barrett <vext01@gmail.com>)
"	       (added html_font. Edd Barrett <vext01@gmail.com>)
"	       (dynamic folding by Ben Fritz <fritzophrenic@gmail.com>)

" Transform a file into HTML, using the current syntax highlighting.

" this file uses line continuations
let s:cpo_sav = &cpo
set cpo-=C

" Number lines when explicitely requested or when `number' is set
if exists("html_number_lines")
  let s:numblines = html_number_lines
else
  let s:numblines = &number
endif

" Font
if exists("html_font")
  let s:htmlfont = html_font . ", monospace"
else
  let s:htmlfont = "monospace"
endif

" make copies of the user-defined settings that we may overrule
if exists("html_dynamic_folds")
  let s:html_dynamic_folds = 1
endif
if exists("html_hover_unfold")
  let s:html_hover_unfold = 1
endif
if exists("html_use_css")
  let s:html_use_css = 1
endif

" hover opening implies dynamic folding
if exists("s:html_hover_unfold")
  let s:html_dynamic_folds = 1
endif

" dynamic folding with no foldcolumn implies hover opens
if exists("s:html_dynamic_folds") && exists("html_no_foldcolumn")
  let s:html_hover_unfold = 1
endif

" ignore folding overrides dynamic folding
if exists("html_ignore_folding") && exists("s:html_dynamic_folds")
  unlet s:html_dynamic_folds
endif

" dynamic folding implies css
if exists("s:html_dynamic_folds")
  let s:html_use_css = 1
endif

" When not in gui we can only guess the colors.
if has("gui_running")
  let s:whatterm = "gui"
else
  let s:whatterm = "cterm"
  if &t_Co == 8
    let s:cterm_color = {0: "#808080", 1: "#ff6060", 2: "#00ff00", 3: "#ffff00", 4: "#8080ff", 5: "#ff40ff", 6: "#00ffff", 7: "#ffffff"}
  else
    let s:cterm_color = {0: "#000000", 1: "#c00000", 2: "#008000", 3: "#804000", 4: "#0000c0", 5: "#c000c0", 6: "#008080", 7: "#c0c0c0", 8: "#808080", 9: "#ff6060", 10: "#00ff00", 11: "#ffff00", 12: "#8080ff", 13: "#ff40ff", 14: "#00ffff", 15: "#ffffff"}

    " Colors for 88 and 256 come from xterm.
    if &t_Co == 88
      call extend(s:cterm_color, {16: "#000000", 17: "#00008b", 18: "#0000cd", 19: "#0000ff", 20: "#008b00", 21: "#008b8b", 22: "#008bcd", 23: "#008bff", 24: "#00cd00", 25: "#00cd8b", 26: "#00cdcd", 27: "#00cdff", 28: "#00ff00", 29: "#00ff8b", 30: "#00ffcd", 31: "#00ffff", 32: "#8b0000", 33: "#8b008b", 34: "#8b00cd", 35: "#8b00ff", 36: "#8b8b00", 37: "#8b8b8b", 38: "#8b8bcd", 39: "#8b8bff", 40: "#8bcd00", 41: "#8bcd8b", 42: "#8bcdcd", 43: "#8bcdff", 44: "#8bff00", 45: "#8bff8b", 46: "#8bffcd", 47: "#8bffff", 48: "#cd0000", 49: "#cd008b", 50: "#cd00cd", 51: "#cd00ff", 52: "#cd8b00", 53: "#cd8b8b", 54: "#cd8bcd", 55: "#cd8bff", 56: "#cdcd00", 57: "#cdcd8b", 58: "#cdcdcd", 59: "#cdcdff", 60: "#cdff00", 61: "#cdff8b", 62: "#cdffcd", 63: "#cdffff", 64: "#ff0000"})
      call extend(s:cterm_color, {65: "#ff008b", 66: "#ff00cd", 67: "#ff00ff", 68: "#ff8b00", 69: "#ff8b8b", 70: "#ff8bcd", 71: "#ff8bff", 72: "#ffcd00", 73: "#ffcd8b", 74: "#ffcdcd", 75: "#ffcdff", 76: "#ffff00", 77: "#ffff8b", 78: "#ffffcd", 79: "#ffffff", 80: "#2e2e2e", 81: "#5c5c5c", 82: "#737373", 83: "#8b8b8b", 84: "#a2a2a2", 85: "#b9b9b9", 86: "#d0d0d0", 87: "#e7e7e7"})
    elseif &t_Co == 256
      call extend(s:cterm_color, {16: "#000000", 17: "#00005f", 18: "#000087", 19: "#0000af", 20: "#0000d7", 21: "#0000ff", 22: "#005f00", 23: "#005f5f", 24: "#005f87", 25: "#005faf", 26: "#005fd7", 27: "#005fff", 28: "#008700", 29: "#00875f", 30: "#008787", 31: "#0087af", 32: "#0087d7", 33: "#0087ff", 34: "#00af00", 35: "#00af5f", 36: "#00af87", 37: "#00afaf", 38: "#00afd7", 39: "#00afff", 40: "#00d700", 41: "#00d75f", 42: "#00d787", 43: "#00d7af", 44: "#00d7d7", 45: "#00d7ff", 46: "#00ff00", 47: "#00ff5f", 48: "#00ff87", 49: "#00ffaf", 50: "#00ffd7", 51: "#00ffff", 52: "#5f0000", 53: "#5f005f", 54: "#5f0087", 55: "#5f00af", 56: "#5f00d7", 57: "#5f00ff", 58: "#5f5f00", 59: "#5f5f5f", 60: "#5f5f87", 61: "#5f5faf", 62: "#5f5fd7", 63: "#5f5fff", 64: "#5f8700"})
      call extend(s:cterm_color, {65: "#5f875f", 66: "#5f8787", 67: "#5f87af", 68: "#5f87d7", 69: "#5f87ff", 70: "#5faf00", 71: "#5faf5f", 72: "#5faf87", 73: "#5fafaf", 74: "#5fafd7", 75: "#5fafff", 76: "#5fd700", 77: "#5fd75f", 78: "#5fd787", 79: "#5fd7af", 80: "#5fd7d7", 81: "#5fd7ff", 82: "#5fff00", 83: "#5fff5f", 84: "#5fff87", 85: "#5fffaf", 86: "#5fffd7", 87: "#5fffff", 88: "#870000", 89: "#87005f", 90: "#870087", 91: "#8700af", 92: "#8700d7", 93: "#8700ff", 94: "#875f00", 95: "#875f5f", 96: "#875f87", 97: "#875faf", 98: "#875fd7", 99: "#875fff", 100: "#878700", 101: "#87875f", 102: "#878787", 103: "#8787af", 104: "#8787d7", 105: "#8787ff", 106: "#87af00", 107: "#87af5f", 108: "#87af87", 109: "#87afaf", 110: "#87afd7", 111: "#87afff", 112: "#87d700"})
      call extend(s:cterm_color, {113: "#87d75f", 114: "#87d787", 115: "#87d7af", 116: "#87d7d7", 117: "#87d7ff", 118: "#87ff00", 119: "#87ff5f", 120: "#87ff87", 121: "#87ffaf", 122: "#87ffd7", 123: "#87ffff", 124: "#af0000", 125: "#af005f", 126: "#af0087", 127: "#af00af", 128: "#af00d7", 129: "#af00ff", 130: "#af5f00", 131: "#af5f5f", 132: "#af5f87", 133: "#af5faf", 134: "#af5fd7", 135: "#af5fff", 136: "#af8700", 137: "#af875f", 138: "#af8787", 139: "#af87af", 140: "#af87d7", 141: "#af87ff", 142: "#afaf00", 143: "#afaf5f", 144: "#afaf87", 145: "#afafaf", 146: "#afafd7", 147: "#afafff", 148: "#afd700", 149: "#afd75f", 150: "#afd787", 151: "#afd7af", 152: "#afd7d7", 153: "#afd7ff", 154: "#afff00", 155: "#afff5f", 156: "#afff87", 157: "#afffaf", 158: "#afffd7"})
      call extend(s:cterm_color, {159: "#afffff", 160: "#d70000", 161: "#d7005f", 162: "#d70087", 163: "#d700af", 164: "#d700d7", 165: "#d700ff", 166: "#d75f00", 167: "#d75f5f", 168: "#d75f87", 169: "#d75faf", 170: "#d75fd7", 171: "#d75fff", 172: "#d78700", 173: "#d7875f", 174: "#d78787", 175: "#d787af", 176: "#d787d7", 177: "#d787ff", 178: "#d7af00", 179: "#d7af5f", 180: "#d7af87", 181: "#d7afaf", 182: "#d7afd7", 183: "#d7afff", 184: "#d7d700", 185: "#d7d75f", 186: "#d7d787", 187: "#d7d7af", 188: "#d7d7d7", 189: "#d7d7ff", 190: "#d7ff00", 191: "#d7ff5f", 192: "#d7ff87", 193: "#d7ffaf", 194: "#d7ffd7", 195: "#d7ffff", 196: "#ff0000", 197: "#ff005f", 198: "#ff0087", 199: "#ff00af", 200: "#ff00d7", 201: "#ff00ff", 202: "#ff5f00", 203: "#ff5f5f", 204: "#ff5f87"})
      call extend(s:cterm_color, {205: "#ff5faf", 206: "#ff5fd7", 207: "#ff5fff", 208: "#ff8700", 209: "#ff875f", 210: "#ff8787", 211: "#ff87af", 212: "#ff87d7", 213: "#ff87ff", 214: "#ffaf00", 215: "#ffaf5f", 216: "#ffaf87", 217: "#ffafaf", 218: "#ffafd7", 219: "#ffafff", 220: "#ffd700", 221: "#ffd75f", 222: "#ffd787", 223: "#ffd7af", 224: "#ffd7d7", 225: "#ffd7ff", 226: "#ffff00", 227: "#ffff5f", 228: "#ffff87", 229: "#ffffaf", 230: "#ffffd7", 231: "#ffffff", 232: "#080808", 233: "#121212", 234: "#1c1c1c", 235: "#262626", 236: "#303030", 237: "#3a3a3a", 238: "#444444", 239: "#4e4e4e", 240: "#585858", 241: "#626262", 242: "#6c6c6c", 243: "#767676", 244: "#808080", 245: "#8a8a8a", 246: "#949494", 247: "#9e9e9e", 248: "#a8a8a8", 249: "#b2b2b2", 250: "#bcbcbc", 251: "#c6c6c6", 252: "#d0d0d0", 253: "#dadada", 254: "#e4e4e4", 255: "#eeeeee"})
    endif
  endif
endif

" Return good color specification: in GUI no transformation is done, in
" terminal return RGB values of known colors and empty string for unknown
if s:whatterm == "gui"
  function! s:HtmlColor(color)
    return a:color
  endfun
else
  function! s:HtmlColor(color)
    if has_key(s:cterm_color, a:color)
      return s:cterm_color[a:color]
    else
      return ""
    endif
  endfun
endif

if !exists("s:html_use_css")
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

if exists("s:html_dynamic_folds")

  " compares two folds as stored in our list of folds
  " A fold is "less" than another if it starts at an earlier line number,
  " or ends at a later line number, ties broken by fold level
  function! s:FoldCompare(f1, f2)
    if a:f1.firstline != a:f2.firstline
      " put it before if it starts earlier
      return a:f1.firstline - a:f2.firstline
    elseif a:f1.lastline != a:f2.lastline
      " put it before if it ends later
      return a:f2.lastline - a:f1.lastline
    else
      " if folds begin and end on the same lines, put lowest fold level first
      return a:f1.level - a:f2.level
    endif
  endfunction

endif

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
  elseif s:vim_encoding == 'sjis' || s:vim_encoding == 'cp932'
    let s:html_encoding = 'Shift_JIS'
  elseif s:vim_encoding == 'big5' || s:vim_encoding == 'cp950'
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
  let s:tag_close = ' />'
else
  let s:tag_close = '>'
endif

" Cache html_no_pre in case we have to turn it on for non-css mode
if exists("html_no_pre")
  let s:old_html_no_pre = html_no_pre
endif

if !exists("s:html_use_css")
  " Can't put font tags in <pre>
  let html_no_pre=1
endif

let s:HtmlSpace = ' '
let s:LeadingSpace = ' '
let s:HtmlEndline = ''
if exists("html_no_pre")
  let s:HtmlEndline = '<br' . s:tag_close
  let s:LeadingSpace = '&nbsp;'
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

if exists("s:html_use_css")
  if exists("s:html_dynamic_folds")
    if exists("s:html_hover_unfold")
      " if we are doing hover_unfold, use css 2 with css 1 fallback for IE6
      exe "normal! a".
	  \ "<style type=\"text/css\">\n<!--\n".
	  \ ".FoldColumn { text-decoration: none; white-space: pre; }\n\n".
	  \ "body * { margin: 0; padding: 0; }\n".
	  \ "\n".
	  \ ".open-fold   > .Folded { display: none;  }\n".
	  \ ".open-fold   > .fulltext { display: inline; }\n".
	  \ ".closed-fold > .fulltext { display: none;  }\n".
	  \ ".closed-fold > .Folded { display: inline; }\n".
	  \ "\n".
	  \ ".open-fold   > .toggle-open   { display: none;   }\n".
	  \ ".open-fold   > .toggle-closed { display: inline; }\n".
	  \ ".closed-fold > .toggle-open   { display: inline; }\n".
	  \ ".closed-fold > .toggle-closed { display: none;   }\n"
      exe "normal! a\n/* opening a fold while hovering won't be supported by IE6 and other\n".
	  \ "similar browsers, but it should fail gracefully. */\n".
	  \ ".closed-fold:hover > .fulltext { display: inline; }\n".
	  \ ".closed-fold:hover > .toggle-filler { display: none; }\n".
	  \ ".closed-fold:hover > .Folded { display: none; }\n"
      exe "normal! a-->\n</style>\n"
      exe "normal! a<!--[if lt IE 7]>".
	  \ "<style type=\"text/css\">\n".
	  \ ".open-fold   .Folded      { display: none; }\n".
	  \ ".open-fold   .fulltext      { display: inline; }\n".
	  \ ".open-fold   .toggle-open   { display: none; }\n".
	  \ ".closed-fold .toggle-closed { display: inline; }\n".
	  \ "\n".
	  \ ".closed-fold .fulltext      { display: none; }\n".
	  \ ".closed-fold .Folded      { display: inline; }\n".
	  \ ".closed-fold .toggle-open   { display: inline; }\n".
	  \ ".closed-fold .toggle-closed { display: none; }\n".
	  \ "</style>\n".
	  \ "<![endif]-->\n"
    else
      " if we aren't doing hover_unfold, use CSS 1 only
      exe "normal! a<style type=\"text/css\">\n<!--\n".
	    \ ".FoldColumn { text-decoration: none; white-space: pre; }\n\n".
	    \ ".open-fold   .Folded      { display: none; }\n".
	    \ ".open-fold   .fulltext      { display: inline; }\n".
	    \ ".open-fold   .toggle-open   { display: none; }\n".
	    \ ".closed-fold .toggle-closed { display: inline; }\n".
	    \ "\n".
	    \ ".closed-fold .fulltext      { display: none; }\n".
	    \ ".closed-fold .Folded      { display: inline; }\n".
	    \ ".closed-fold .toggle-open   { display: inline; }\n".
	    \ ".closed-fold .toggle-closed { display: none; }\n".
	    \ "-->\n</style>\n"
    endif
  else
    " if we aren't doing any dynamic folding, no need for any special rules
    exe "normal! a<style type=\"text/css\">\n<!--\n-->\n</style>\n\e"
  endif
endif

" insert javascript to toggle folds open and closed
if exists("s:html_dynamic_folds")
  exe "normal! a\n".
	\ "<script type='text/javascript'>\n".
	\ "<!--\n".
	\ "function toggleFold(objID)\n".
	\ "{\n".
	\ "  var fold;\n".
	\ "  fold = document.getElementById(objID);\n".
	\ "  if(fold.className == 'closed-fold')\n".
	\ "  {\n".
	\ "    fold.className = 'open-fold';\n".
	\ "  }\n".
	\ "  else if (fold.className == 'open-fold')\n".
	\ "  {\n".
	\ "    fold.className = 'closed-fold';\n".
	\ "  }\n".
	\ "}\n".
	\ "-->\n".
	\ "</script>\n\e"
endif

if exists("html_no_pre")
  exe "normal! a</head>\n<body>\n\e"
else
  exe "normal! a</head>\n<body>\n<pre>\n\e"
endif

exe s:orgwin . "wincmd w"

" List of all id's
let s:idlist = ","

" First do some preprocessing for dynamic folding. Do this for the entire file
" so we don't accidentally start within a closed fold or something.
let s:allfolds = []

if exists("s:html_dynamic_folds")
  let s:lnum = 1
  let s:end = line('$')
  " save the fold text and set it to the default so we can find fold levels
  let s:foldtext_save = &foldtext
  set foldtext&

  " we will set the foldcolumn in the html to the greater of the maximum fold
  " level and the current foldcolumn setting
  let s:foldcolumn = &foldcolumn

  " get all info needed to describe currently closed folds
  while s:lnum < s:end
    if foldclosed(s:lnum) == s:lnum
      " default fold text has '+-' and then a number of dashes equal to fold
      " level, so subtract 2 from index of first non-dash after the dashes
      " in order to get the fold level of the current fold
      let s:level = match(foldtextresult(s:lnum), '+-*\zs[^-]') - 2
      if s:level+1 > s:foldcolumn
	let s:foldcolumn = s:level+1
      endif
      " store fold info for later use
      let s:newfold = {'firstline': s:lnum, 'lastline': foldclosedend(s:lnum), 'level': s:level,'type': "closed-fold"}
      call add(s:allfolds, s:newfold)
      " open the fold so we can find any contained folds
      execute s:lnum."foldopen"
    else
      let s:lnum = s:lnum + 1
    endif
  endwhile

  " close all folds to get info for originally open folds
  silent! %foldclose!
  let s:lnum = 1

  " the originally open folds will be all folds we encounter that aren't
  " already in the list of closed folds
  while s:lnum < s:end
    if foldclosed(s:lnum) == s:lnum
      " default fold text has '+-' and then a number of dashes equal to fold
      " level, so subtract 2 from index of first non-dash after the dashes
      " in order to get the fold level of the current fold
      let s:level = match(foldtextresult(s:lnum), '+-*\zs[^-]') - 2
      if s:level+1 > s:foldcolumn
	let s:foldcolumn = s:level+1
      endif
      let s:newfold = {'firstline': s:lnum, 'lastline': foldclosedend(s:lnum), 'level': s:level,'type': "closed-fold"}
      " only add the fold if we don't already have it
      if empty(s:allfolds) || index(s:allfolds, s:newfold) == -1
	let s:newfold.type = "open-fold"
	call add(s:allfolds, s:newfold)
      endif
      " open the fold so we can find any contained folds
      execute s:lnum."foldopen"
    else
      let s:lnum = s:lnum + 1
    endif
  endwhile

  " sort the folds so that we only ever need to look at the first item in the
  " list of folds
  call sort(s:allfolds, "s:FoldCompare")

  let &foldtext = s:foldtext_save
  unlet s:foldtext_save

  " close all folds again so we can get the fold text as we go
  silent! %foldclose! 
endif

" Now loop over all lines in the original text to convert to html.
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

" stack to keep track of all the folds containing the current line
let s:foldstack = []

if s:numblines
  let s:margin = strlen(s:end) + 1
else
  let s:margin = 0
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

let s:foldId = 0

while s:lnum <= s:end

  " If there are filler lines for diff mode, show these above the line.
  let s:filler = diff_filler(s:lnum)
  if s:filler > 0
    let s:n = s:filler
    while s:n > 0
      let s:new = repeat(s:difffillchar, 3)

      if s:n > 2 && s:n < s:filler && !exists("html_whole_filler")
	let s:new = s:new . " " . s:filler . " inserted lines "
	let s:n = 2
      endif

      if !exists("html_no_pre")
	" HTML line wrapping is off--go ahead and fill to the margin
	let s:new = s:new . repeat(s:difffillchar, &columns - strlen(s:new) - s:margin)
      else
	let s:new = s:new . repeat(s:difffillchar, 3)
      endif

      let s:new = s:HtmlFormat(s:new, "DiffDelete")
      if s:numblines
	" Indent if line numbering is on; must be after escaping.
	let s:new = repeat(s:LeadingSpace, s:margin) . s:new
      endif
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
    let s:numcol = repeat(' ', s:margin - 1 - strlen(s:lnum)) . s:lnum . ' '
  else
    let s:numcol = ""
  endif

  let s:new = ""

  if has('folding') && !exists('html_ignore_folding') && foldclosed(s:lnum) > -1 && !exists('s:html_dynamic_folds')
    "
    " This is the beginning of a folded block (with no dynamic folding)
    "
    let s:new = s:numcol . foldtextresult(s:lnum)
    if !exists("html_no_pre")
      " HTML line wrapping is off--go ahead and fill to the margin
      let s:new = s:new . repeat(s:foldfillchar, &columns - strlen(s:new))
    endif

    let s:new = s:HtmlFormat(s:new, "Folded")

    " Skip to the end of the fold
    let s:lnum = foldclosedend(s:lnum)

  else
    "
    " A line that is not folded, or doing dynamic folding.
    "
    let s:line = getline(s:lnum)
    let s:len = strlen(s:line)

    if exists("s:html_dynamic_folds")
      " First insert a closing for any open folds that end on this line
      while !empty(s:foldstack) && get(s:foldstack,0).lastline == s:lnum-1
	let s:new = s:new."</span></span>"
	call remove(s:foldstack, 0)
      endwhile

      " Now insert an opening any new folds that start on this line
      let s:firstfold = 1
      while !empty(s:allfolds) && get(s:allfolds,0).firstline == s:lnum
	let s:foldId = s:foldId + 1
	let s:new = s:new . "<span id='fold".s:foldId."' class='".s:allfolds[0].type."'>"

	" Unless disabled, add a fold column for the opening line of a fold.
	"
	" Note that dynamic folds require using css so we just use css to take
	" care of the leading spaces rather than using &nbsp; in the case of
	" html_no_pre to make it easier
	if !exists("html_no_foldcolumn")
	  " add fold column that can open the new fold
	  if s:allfolds[0].level > 1 && s:firstfold
	    let s:new = s:new . "<a class='toggle-open FoldColumn' href='javascript:toggleFold(\"fold".s:foldstack[0].id."\")'>"
	    let s:new = s:new . repeat('|', s:allfolds[0].level - 1) . "</a>"
	  endif
	  let s:new = s:new . "<a class='toggle-open FoldColumn' href='javascript:toggleFold(\"fold".s:foldId."\")'>+</a>"
	  let s:new = s:new . "<a class='toggle-open "
	  " If this is not the last fold we're opening on this line, we need
	  " to keep the filler spaces hidden if the fold is opened by mouse
	  " hover. If it is the last fold to open in the line, we shouldn't hide
	  " them, so don't apply the toggle-filler class.
	  if get(s:allfolds, 1, {'firstline': 0}).firstline == s:lnum
	    let s:new = s:new . "toggle-filler "
	  endif
	  let s:new = s:new . "FoldColumn' href='javascript:toggleFold(\"fold".s:foldId."\")'>"
	  let s:new = s:new . repeat(" ", s:foldcolumn - s:allfolds[0].level) . "</a>"

	  " add fold column that can close the new fold
	  let s:new = s:new . "<a class='toggle-closed FoldColumn' href='javascript:toggleFold(\"fold".s:foldId."\")'>"
	  if s:firstfold
	    let s:new = s:new . repeat('|', s:allfolds[0].level - 1)
	  endif
	  let s:new = s:new . "-"
	  " only add spaces if we aren't opening another fold on the same line
	  if get(s:allfolds, 1, {'firstline': 0}).firstline != s:lnum
	    let s:new = s:new . repeat(" ", s:foldcolumn - s:allfolds[0].level)
	  endif
	  let s:new = s:new . "</a>"
	  let s:firstfold = 0
	endif

	" add fold text, moving the span ending to the next line so collapsing
	" of folds works correctly
	let s:new = s:new . substitute(s:HtmlFormat(s:numcol . foldtextresult(s:lnum), "Folded"), '</span>', s:HtmlEndline.'\r\0', '')
	let s:new = s:new . "<span class='fulltext'>"

	" open the fold now that we have the fold text to allow retrieval of
	" fold text for subsequent folds
	execute s:lnum."foldopen"
	call insert(s:foldstack, remove(s:allfolds,0))
	let s:foldstack[0].id = s:foldId
      endwhile

      " Unless disabled, add a fold column for other lines.
      "
      " Note that dynamic folds require using css so we just use css to take
      " care of the leading spaces rather than using &nbsp; in the case of
      " html_no_pre to make it easier
      if !exists("html_no_foldcolumn")
	if empty(s:foldstack)
	  " add the empty foldcolumn for unfolded lines
	  let s:new = s:new . s:HtmlFormat(repeat(' ', s:foldcolumn), "FoldColumn")
	else
	  " add the fold column for folds not on the opening line
	  if get(s:foldstack, 0).firstline < s:lnum
	    let s:new = s:new . "<a class='FoldColumn' href='javascript:toggleFold(\"fold".s:foldstack[0].id."\")'>"
	    let s:new = s:new . repeat('|', s:foldstack[0].level)
	    let s:new = s:new . repeat(' ', s:foldcolumn - s:foldstack[0].level) . "</a>"
	  endif
	endif
      endif
    endif

    " Now continue with the unfolded line text
    if s:numblines
      let s:new = s:new . s:HtmlFormat(s:numcol, "lnr")
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
	  let s:line = s:line . repeat(' ', &columns - virtcol([s:lnum, s:len]) - s:margin)
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
      let s:offset = 0
      let s:idx = stridx(s:expandedtab, "\t")
      while s:idx >= 0
	if has("multi_byte_encoding")
	  if s:startcol + s:idx == 1
	    let s:i = &ts
	  else
	    if s:idx == 0
	      let s:prevc = matchstr(s:line, '.\%' . (s:startcol + s:idx + s:offset) . 'c')
	    else
	      let s:prevc = matchstr(s:expandedtab, '.\%' . (s:idx + 1) . 'c')
	    endif
	    let s:vcol = virtcol([s:lnum, s:startcol + s:idx + s:offset - len(s:prevc)])
	    let s:i = &ts - (s:vcol % &ts)
	  endif
	  let s:offset -= s:i - 1
	else
	  let s:i = &ts - ((s:idx + s:startcol - 1) % &ts)
	endif
	let s:expandedtab = substitute(s:expandedtab, '\t', repeat(' ', s:i), '')
	let s:idx = stridx(s:expandedtab, "\t")
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

if exists("s:html_dynamic_folds")
  " finish off any open folds
  while !empty(s:foldstack)
    exe "normal! a</span></span>"
    call remove(s:foldstack, 0)
  endwhile

  " add fold column to the style list if not already there
  let s:id = hlID('FoldColumn')
  if stridx(s:idlist, "," . s:id . ",") == -1
    let s:idlist = s:idlist . s:id . ","
  endif
endif

" Close off the font tag that encapsulates the whole <body>
if !exists("s:html_use_css")
  exe "normal! a</font>\e"
endif

if exists("html_no_pre")
  exe "normal! a</body>\n</html>\e"
else
  exe "normal! a</pre>\n</body>\n</html>\e"
endif


" Now, when we finally know which, we define the colors and styles
if exists("s:html_use_css")
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
if exists("s:html_use_css")
  if exists("html_no_pre")
    execute "normal! A\nbody { color: " . s:fgc . "; background-color: " . s:bgc . "; font-family: ". s:htmlfont ."; }\e"
  else
    execute "normal! A\npre { font-family: ". s:htmlfont ."; color: " . s:fgc . "; background-color: " . s:bgc . "; }\e"
    yank
    put
    execute "normal! ^cwbody\e"
  endif
else
  execute '%s:<body>:<body bgcolor="' . s:bgc . '" text="' . s:fgc . '"><font face="'. s:htmlfont .'">'
endif

" Line numbering attributes
if s:numblines
  if exists("s:html_use_css")
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
    if exists("s:html_use_css")
      execute "normal! A\n." . s:id_name . " { " . s:attr . "}"
    else
      execute '%s+<span class="' . s:id_name . '">\([^<]*\)</span>+' . s:HtmlOpening(s:id) . '\1' . s:HtmlClosing(s:id) . '+g'
    endif
  else
    execute '%s+<span class="' . s:id_name . '">\([^<]*\)</span>+\1+ge'
    if exists("s:html_use_css")
      1;/<style type="text/+1
    endif
  endif
endwhile

" Add hyperlinks
%s+\(https\=://\S\{-}\)\(\([.,;:}]\=\(\s\|$\)\)\|[\\"'<>]\|&gt;\|&lt;\|&quot;\)+<a href="\1">\1</a>\2+ge

" The DTD
if exists("use_xhtml")
  exe "normal! gg$a\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\e"
else
  exe "normal! gg0i<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n\e"
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

" Reset old <pre> settings
if exists("s:old_html_no_pre")
  let html_no_pre = s:old_html_no_pre
  unlet s:old_html_no_pre
elseif exists("html_no_pre")
  unlet html_no_pre
endif

" Save a little bit of memory (worth doing?)
unlet s:htmlfont
unlet s:old_et s:old_paste s:old_icon s:old_report s:old_title s:old_search
unlet s:whatterm s:idlist s:lnum s:end s:margin s:fgc s:bgc s:old_magic
unlet! s:col s:id s:attr s:len s:line s:new s:expandedtab s:numblines
unlet! s:orgwin s:newwin s:orgbufnr s:idx s:i s:offset
if !v:profiling
  delfunc s:HtmlColor
  delfunc s:HtmlFormat
  delfunc s:CSS1
  if !exists("s:html_use_css")
    delfunc s:HtmlOpening
    delfunc s:HtmlClosing
  endif
endif
silent! unlet s:diffattr s:difffillchar s:foldfillchar s:HtmlSpace s:LeadingSpace s:HtmlEndline s:firstfold s:foldcolumn
unlet s:foldstack s:allfolds s:foldId s:numcol

if exists("s:html_dynamic_folds")
  delfunc s:FoldCompare
endif

silent! unlet s:html_dynamic_folds s:html_hover_unfold s:html_use_css

let &cpo = s:cpo_sav
unlet s:cpo_sav

" vim: noet sw=2 sts=2
