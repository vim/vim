"=============================================================================
" File: explorer.vim
" Author: M A Aziz Ahmed (aziz@acorn-networks.com - doesn't work)
" Last Change:	2004 May 13
" Version: 2.5 + changes
" Additions by Mark Waggoner (waggoner@aracnet.com) et al.
"-----------------------------------------------------------------------------
" This file implements a file explorer.
"-----------------------------------------------------------------------------
" Normally, this file will reside in the plugins directory and be
" automatically sourced.  If not, you must manually source this file
" using :source explorer.vim
"
" To use it, just edit a directory (vi dirname) or type :Explore to
" launch the file explorer in the current window, or :Sexplore to split
" the current window and launch explorer there.
"
" If the current buffer is modified, the window is always split.
"
" It is also possible to delete files and rename files within explorer.
" See :help file-explorer for more details
"
"-----------------------------------------------------------------------------
" Update history removed, it's not very interesting.
" Contributors were: Doug Potts, Bram Moolenaar, Thomas Köhler
"=============================================================================

" Has this already been loaded?
if exists("loaded_explorer")
  finish
endif
let loaded_explorer=1

" Line continuation used here
let s:cpo_save = &cpo
set cpo&vim

"---
" Default settings for global configuration variables

" Split vertically instead of horizontally?
if !exists("g:explVertical")
  let g:explVertical=0
endif

" How big to make the window? Set to "" to avoid resizing
if !exists("g:explWinSize")
  let g:explWinSize=15
endif

" When opening a new file/directory, split below current window (or
" above)?  1 = below, 0 = to above
if !exists("g:explSplitBelow")
  let g:explSplitBelow = &splitbelow
endif

" Split to right of current window (or to left)?
" 1 = to right, 0 = to left
if !exists("g:explSplitRight")
  let g:explSplitRight = &splitright
endif

" Start the first explorer window...
" Defaults to be the same as explSplitBelow
if !exists("g:explStartBelow")
  let g:explStartBelow = g:explSplitBelow
endif

" Start the first explorer window...
" Defaults to be the same as explSplitRight
if !exists("g:explStartRight")
  let g:explStartRight = g:explSplitRight
endif

" Show detailed help?
if !exists("g:explDetailedHelp")
  let g:explDetailedHelp=0
endif

" Show file size and dates?
if !exists("g:explDetailedList")
  let g:explDetailedList=0
endif

" Format for the date
if !exists("g:explDateFormat")
  let g:explDateFormat="%d %b %Y %H:%M"
endif

" Files to hide
if !exists("g:explHideFiles")
  let g:explHideFiles=''
endif

" Field to sort by
if !exists("g:explSortBy")
  let g:explSortBy='name'
endif

" Segregate directories? 1, 0, or -1
if !exists("g:explDirsFirst")
  let g:explDirsFirst=1
endif

" Segregate items in suffixes option? 1, 0, or -1
if !exists("g:explSuffixesLast")
  let g:explSuffixesLast=1
endif

" Include separator lines between directories, files, and suffixes?
if !exists("g:explUseSeparators")
  let g:explUseSeparators=0
endif

" Execute file handler
if !exists("g:explFileHandler")
  if has("win32")
    " for Win32 use rundll32
    function! s:explFileHandlerWin32(fn)
      exec 'silent !start rundll32 url.dll,FileProtocolHandler "'
		\ . escape(a:fn, '%#') . '"'
    endfunction
    let g:explFileHandler = "<SID>explFileHandlerWin32"

  elseif has("unix")
    " for KDE use kfmclient, for GNUME use gnome-open
    if executable("kfmclient")
      let g:explFileHandlerCmd = "kfmclient exec"
    elseif executable("gnome-open")
      let g:explFileHandlerCmd = "gnome-open"
    else
      let g:explFileHandlerCmd = ""
    endif
    if g:explFileHandlerCmd != ""
      function! s:explFileHandlerUnix(fn)
	if &shellredir =~ "%s"
	  let redir = substitute(&shellredir, "%s", "/dev/null", "")
	else
	  let redir = &shellredir . "/dev/null"
	endif
	" Need to escape % and # but not spaces.
	exec "silent !" . g:explFileHandlerCmd . " '" . escape(a:fn, '%#') . "'" . redir
      endfunction
      let g:explFileHandler = "<SID>explFileHandlerUnix"
    endif
  endif
endif

"---
" script variables - these are the same across all
" explorer windows

" characters that must be escaped for a regular expression
let s:escregexp = '/*^$.~\'

" characters that must be escaped for filenames
if has("dos16") || has("dos32") || has("win16") || has("win32") || has("os2")
  let s:escfilename = ' %#'
else
  let s:escfilename = ' \%#[]'
endif


" A line to use for separating sections
let s:separator='"---------------------------------------------------'

"---
" Create commands

if !exists(':Explore')
  command -n=? -complete=dir Explore :call s:StartExplorer(0, '<a>')
endif
if !exists(':Sexplore')
  command -n=? -complete=dir Sexplore :call s:StartExplorer(1, '<a>')
endif

"---
" Start the explorer using the preferences from the global variables
"
function! s:StartExplorer(split, start_dir)
  let startcmd = "edit"
  if a:start_dir != ""
    let fname=a:start_dir
  else
    let fname = expand("%:p:h")
  endif
  if fname == ""
    let fname = getcwd()
  endif

  " Create a variable to use if splitting vertically
  let splitMode = ""
  if g:explVertical == 1
    let splitMode = "vertical"
  endif

  " Save the user's settings for splitbelow and splitright
  let savesplitbelow = &splitbelow
  let savesplitright = &splitright

  if a:split || &modified
    let startcmd = splitMode . " " . g:explWinSize . "new " . fname
    let &splitbelow = g:explStartBelow
    let &splitright = g:explStartRight
  else
    let startcmd = "edit " . fname
  endif
  silent execute startcmd
  let &splitbelow = savesplitbelow
  let &splitright = savesplitright
endfunction

"---
" This is the main entry for 'editing' a directory
"
function! s:EditDir()
  " Get out of here right away if this isn't a directory!
  let name = expand("%")
  if name == ""
    let name = expand("%:p")
  endif
  if !isdirectory(name)
    return
  endif

  " Turn off the swapfile, set the buffer type so that it won't get
  " written, and so that it will get deleted when it gets hidden.
  setlocal noreadonly modifiable
  setlocal noswapfile
  setlocal buftype=nowrite
  setlocal bufhidden=delete
  " Don't wrap around long lines
  setlocal nowrap

  " No need for any insertmode abbreviations, since we don't allow
  " insertions anyway!
  iabc <buffer>

  " Long or short listing?  Use the global variable the first time
  " explorer is called, after that use the script variable as set by
  " the interactive user.
  if exists("s:longlist")
    let w:longlist = s:longlist
  else
    let w:longlist = g:explDetailedList
  endif

  " Show keyboard shortcuts?
  if exists("s:longhelp")
    let w:longhelp = s:longhelp
  else
    let w:longhelp = g:explDetailedHelp
  endif

  " Set the sort based on the global variables the first time.  If you
  " later change the sort order, it will be retained in the s:sortby
  " variable for the next time you open explorer
  let w:sortdirection=1
  let w:sortdirlabel = ""
  let w:sorttype = ""
  if exists("s:sortby")
    let sortby=s:sortby
  else
    let sortby=g:explSortBy
  endif
  if sortby =~ "reverse"
    let w:sortdirection=-1
    let w:sortdirlabel = "reverse "
  endif
  if sortby =~ "date"
    let w:sorttype = "date"
  elseif sortby =~ "size"
    let w:sorttype = "size"
  else
    let w:sorttype = "name"
  endif
  call s:SetSuffixesLast()

  " If directory is already loaded, don't open it again!
  if line('$') > 1
    setlocal readonly nomodifiable
    return
  endif

  " Get the complete path to the directory to look at with a slash at
  " the end.  This also removes "/../" and "/./" things.
  let b:completePath = s:Path(expand("%:p"))

  " Add a slash at the end
  if b:completePath !~ '/$'
    let b:completePath = b:completePath . '/'
  endif

  " escape special characters for exec commands
  let b:completePathEsc = escape(b:completePath, s:escfilename)
  let b:parentDirEsc = substitute(b:completePathEsc, '/[^/]*/$', '/', 'g')

  " Set up syntax highlighting
  " Something wrong with the evaluation of the conditional though...
  if has("syntax") && exists("g:syntax_on") && !has("syntax_items")
    syn match browseSynopsis    "^\"[ -].*"
    syn match browseDirectory   "[^\"].*/ "
    syn match browseDirectory   "[^\"].*/$"
    syn match browseCurDir      "^\"= .*$"
    syn match browseSortBy      "^\" Sorted by .*$"  contains=browseSuffixInfo
    syn match browseSuffixInfo  "(.*)$"  contained
    syn match browseFilter      "^\" Not Showing:.*$"
    syn match browseFiletime    "««\d\+$"
    exec('syn match browseSuffixes    "' . b:suffixesHighlight . '"')

    "hi def link browseSynopsis    PreProc
    hi def link browseSynopsis    Special
    hi def link browseDirectory   Directory
    hi def link browseCurDir      Statement
    hi def link browseSortBy      String
    hi def link browseSuffixInfo  Type
    hi def link browseFilter      String
    hi def link browseFiletime    Ignore
    hi def link browseSuffixes    Type
  endif

  " Set filter for hiding files
  let b:filterFormula=substitute(g:explHideFiles, '\([^\\]\),', '\1\\|', 'g')
  if b:filterFormula != ''
    let b:filtering="\nNot showing: " . b:filterFormula
  else
    let b:filtering=""
  endif

  " Show the files
  call s:ShowDirectory()

  " Set up mappings for this buffer
  let cpo_save = &cpo
  set cpo&vim
  nnoremap <buffer> <cr> :call <SID>EditEntry("","edit")<cr>
  nnoremap <buffer> -    :exec ("silent e "  . b:parentDirEsc)<cr>
  if exists("g:explFileHandler")
    nnoremap <buffer> x    :call <SID>ExecuteEntry()<cr>
  endif
  nnoremap <buffer> o    :call <SID>OpenEntry()<cr>
  nnoremap <buffer> O    :call <SID>OpenEntryPrevWindow()<cr>
  nnoremap <buffer> p    :call <SID>EditEntry("","pedit")<cr>
  nnoremap <buffer> ?    :call <SID>ToggleHelp()<cr>
  nnoremap <buffer> a    :call <SID>ShowAllFiles()<cr>
  nnoremap <buffer> R    :call <SID>RenameFile()<cr>
  nnoremap <buffer> D    :. call <SID>DeleteFile()<cr>
  vnoremap <buffer> D    :call <SID>DeleteFile()<cr>
  nnoremap <buffer> i    :call <SID>ToggleLongList()<cr>
  nnoremap <buffer> s    :call <SID>SortSelect()<cr>
  nnoremap <buffer> r    :call <SID>SortReverse()<cr>
  nnoremap <buffer> c    :exec "cd ".b:completePathEsc<cr>
  nnoremap <buffer> <2-leftmouse> :call <SID>DoubleClick()<cr>
  if exists("*ExplorerCustomMap")
    call ExplorerCustomMap()
  endif
  let &cpo = cpo_save

  " prevent the buffer from being modified
  setlocal readonly nomodifiable
endfunction

"---
" Determine the number of windows open to this buffer number.
" Care of Yegappan Lakshman.  Thanks!
fun! s:BufInWindows(bnum)
  let cnt = 0
  let winnum = 1
  while 1
    let bufnum = winbufnr(winnum)
    if bufnum < 0
      break
    endif
    if bufnum == a:bnum
      let cnt = cnt + 1
    endif
    let winnum = winnum + 1
  endwhile

  return cnt
endfunction

" If this is the only window, open file in a new window
" Otherwise, open file in the most recently visited window
"
function! s:OpenEntryPrevWindow()
  " Figure out if there are any other windows
  let n = winnr()
  wincmd p
  " No other window?  Then open a new one
  if n == winnr()
    call s:OpenEntry()
  " Other windows exist
  else
    " Check if the previous buffer is modified - ask if they want to save!
    " Was it modified, and is it the only window open to this file
    if &modified && s:BufInWindows(winbufnr(winnr())) < 2
      let bufname = bufname(winbufnr(winnr()))

      let action=confirm("Save Changes in " . bufname . "?","&Yes\n&No\n&Cancel")
      " Yes - try to save - if there is an error, cancel
      if action == 1
	let v:errmsg = ""
	silent w
	if v:errmsg != ""
	  echoerr "Unable to write buffer!"
	  wincmd p
	  return
	endif
      " No, abandon changes
      elseif action == 2
	set nomodified
	echomsg "Warning, abandoning changes in " . bufname
      " Cancel (or any other result), don't do the open
      else
	wincmd p
	return
      endif
    endif
    wincmd p
    call s:EditEntry("wincmd p","edit")
  endif
endfunction


"---
" Open a file or directory in a new window.
" Use g:explSplitBelow and g:explSplitRight to decide where to put the
" split window, and resize the original explorer window if it is
" larger than g:explWinSize
"
function! s:OpenEntry()
  " Are we on a line with a file name?
  let l = getline(".")
  if l =~ '^"'
    return
  endif

  " Copy window settings to script settings
  let s:sortby=w:sortdirlabel . w:sorttype
  let s:longhelp = w:longhelp
  let s:longlist = w:longlist

  " Get the window number of the explorer window
  let n = winnr()

  " Save the user's settings for splitbelow and splitright
  let savesplitbelow=&splitbelow
  let savesplitright=&splitright

  " Figure out how to do the split based on the user's preferences.
  " We want to split to the (left,right,top,bottom) of the explorer
  " window, but we want to extract the screen real-estate from the
  " window next to the explorer if possible.
  "
  " 'there' will be set to a command to move from the split window
  " back to the explorer window
  "
  " 'back' will be set to a command to move from the explorer window
  " back to the newly split window
  "
  " 'right' and 'below' will be set to the settings needed for
  " splitbelow and splitright IF the explorer is the only window.
  "
  if g:explVertical
    if g:explSplitRight
      let there="wincmd h"
      let back ="wincmd l"
      let right=1
      let below=0
    else
      let there="wincmd l"
      let back ="wincmd h"
      let right=0
      let below=0
    endif
  else
    if g:explSplitBelow
      let there="wincmd k"
      let back ="wincmd j"
      let right=0
      let below=1
    else
      let there="wincmd j"
      let back ="wincmd k"
      let right=0
      let below=0
    endif
  endif

  " Get the file name
  let fn=s:GetFullFileName()

  " Attempt to go to adjacent window
  exec(back)
  " If no adjacent window, set splitright and splitbelow appropriately
  if n == winnr()
    let &splitright=right
    let &splitbelow=below
  else
    " found adjacent window - invert split direction
    let &splitright=!right
    let &splitbelow=!below
  endif

  " Create a variable to use if splitting vertically
  let splitMode = ""
  if g:explVertical == 1
    let splitMode = "vertical"
  endif

  " Is it a directory?  If so, get a real path to it instead of
  " relative path.  This also removes "/../" and "/./" things.
  if isdirectory(fn)
    let fn = fnamemodify(fn, ":p")
  endif

  " Open the new window
  exec("silent " . splitMode." sp " . escape(fn,s:escfilename))

  " resize the explorer window if it is larger than the requested size
  exec(there)
  if g:explWinSize =~ '[0-9]\+' && winheight("") > g:explWinSize
    exec("silent ".splitMode." resize ".g:explWinSize)
  endif
  exec(back)

  " Restore splitmode settings
  let &splitbelow=savesplitbelow
  let &splitright=savesplitright

endfunction

function! s:ExecuteEntry()
  " Are we on a line with a file name?
  let l = getline(".")
  if l =~ '^"'
    return
  endif

  " Get the file name
  let fn = s:GetFullFileName()
  if has("win32") && fn =~ '^//'
    let fn = substitute(fn, '/', '\\', 'g')
  endif
  exec "call " . g:explFileHandler . "(fn)"
endfunction

"---
" Double click with the mouse
"
function s:DoubleClick()
  if expand("<cfile>") =~ '[\\/]$'
    call s:EditEntry("","edit")		" directory: open in this window
  else
    call s:OpenEntryPrevWindow()	" file: open in another window
  endif
endfun

"---
" Open file or directory in the same window as the explorer is
" currently in
"
function! s:EditEntry(movefirst,editcmd)
  " Are we on a line with a file name?
  let l = getline(".")
  if l =~ '^"'
    return
  endif

  " Copy window settings to script settings
  let s:sortby=w:sortdirlabel . w:sorttype
  let s:longhelp = w:longhelp
  let s:longlist = w:longlist

  " Get the file name
  let fn = s:GetFullFileName()
  if isdirectory(fn)
    " This removes "/../" and "/./" things.
    let fn = fnamemodify(fn, ":p")
  endif

  " Move to desired window if needed
  exec(a:movefirst)
  " Edit the file/dir
  exec(a:editcmd . " " . escape(fn,s:escfilename))
endfunction


"---
" Create a regular expression out of the suffixes option for sorting
" and set a string to indicate whether we are sorting with the
" suffixes at the end (or the beginning)
"
function! s:SetSuffixesLast()
  let b:suffixesRegexp = '\(' . substitute(escape(&suffixes,s:escregexp),',','\\|','g') . '\)$'
  let b:suffixesHighlight = '^[^"].*\(' . substitute(escape(&suffixes,s:escregexp),',','\\|','g') . '\)\( \|$\)'
  if has("fname_case")
    let b:suffixesRegexp = '\C' . b:suffixesRegexp
    let b:suffixesHighlight = '\C' . b:suffixesHighlight
  else
    let b:suffixesRegexp = '\c' . b:suffixesRegexp
    let b:suffixesHighlight = '\c' . b:suffixesHighlight
  endif
  if g:explSuffixesLast > 0 && &suffixes != ""
    let b:suffixeslast=" (" . &suffixes . " at end of list)"
  elseif g:explSuffixesLast < 0 && &suffixes != ""
    let b:suffixeslast=" (" . &suffixes . " at start of list)"
  else
    let b:suffixeslast=" ('suffixes' mixed with files)"
  endif
endfunction

"---
" Show the header and contents of the directory
"
function! s:ShowDirectory()
  "Delete all lines
  1,$d _
  " Prevent a report of our actions from showing up
  let oldRep=&report
  let save_sc = &sc
  set report=10000 nosc

  " Add the header
  call s:AddHeader()
  $d _

  " Display the files

  " Get a list of all the files
  let files = s:Path(glob(b:completePathEsc . "*"))
  if files != "" && files !~ "\n$"
    let files = files . "\n"
  endif

  " Add the dot files now, making sure "." is not included!
  let files = files . substitute(s:Path(glob(b:completePathEsc . ".*")), "[^\n]*/./\\=\n", '' , '')
  if files != "" && files !~ "\n$"
    let files = files . "\n"
  endif

  " Are there any files left after filtering?
  if files != ""
    normal! mt
    put =files
    let b:maxFileLen = 0
    0
    /^"=/+1,$g/^/call s:MarkDirs()
    normal! `t
    call s:AddFileInfo()
  endif

  normal! zz

  " Move to first directory in the listing
  0
  /^"=/+1

  " Do the sort
  call s:SortListing("Loaded contents of ".b:completePath.". ")

  " Move to first directory in the listing
  0
  /^"=/+1

  let &report=oldRep
  let &sc = save_sc

endfunction

"---
" Mark which items are directories - called once for each file name
" must be used only when size/date is not displayed
"
function! s:MarkDirs()
  let oldRep=&report
  set report=1000
  "Remove slashes if added
  s;/$;;e
  "Removes all the leading slashes and adds slashes at the end of directories
  s;^.*\\\([^\\]*\)$;\1;e
  s;^.*/\([^/]*\)$;\1;e
  "normal! ^
  let currLine=getline(".")
  if isdirectory(b:completePath . currLine)
    s;$;/;
    let fileLen=strlen(currLine)+1
  else
    let fileLen=strlen(currLine)
    if (b:filterFormula!="") && (currLine =~ b:filterFormula)
      " Don't show the file if it is to be filtered.
      d _
    endif
  endif
  if fileLen > b:maxFileLen
    let b:maxFileLen=fileLen
  endif
  let &report=oldRep
endfunction

"---
" Make sure a path has proper form
"
function! s:Path(p)
  if has("dos16") || has("dos32") || has("win16") || has("win32") || has("os2")
    return substitute(a:p,'\\','/','g')
  else
    return a:p
  endif
endfunction

"---
" Extract the file name from a line in several different forms
"
function! s:GetFullFileNameEsc()
    return s:EscapeFilename(s:GetFullFileName())
endfunction

function! s:GetFileNameEsc()
    return s:EscapeFilename(s:GetFileName())
endfunction

function! s:EscapeFilename(name)
    return escape(a:name,s:escfilename)
endfunction


function! s:GetFullFileName()
  return s:ExtractFullFileName(getline("."))
endfunction

function! s:GetFileName()
  return s:ExtractFileName(getline("."))
endfunction

function! s:ExtractFullFileName(line)
      let fn=s:ExtractFileName(a:line)
      if fn == '/'
	return b:completePath
      else
	return b:completePath . s:ExtractFileName(a:line)
      endif
endfunction

function! s:ExtractFileName(line)
  return substitute(strpart(a:line,0,b:maxFileLen),'\s\+$','','')
endfunction

"---
" Get the size of the file
function! s:ExtractFileSize(line)
  if (w:longlist==0)
    return getfsize(s:ExtractFileName(a:line))
  else
    return strpart(a:line,b:maxFileLen+2,b:maxFileSizeLen)
  endif
endfunction

"---
" Get the date of the file as a number
function! s:ExtractFileDate(line)
  if w:longlist==0
    return getftime(s:ExtractFileName(a:line))
  else
    return strpart(matchstr(strpart(a:line,b:maxFileLen+b:maxFileSizeLen+4),"««.*"),2) + 0
  endif
endfunction


"---
" Add the header with help information
"
function! s:AddHeader()
    let save_f=@f
    1
    if w:longhelp==1
      let @f="\" <enter> : open file or directory\n"
	   \."\" o : open new window for file/directory\n"
	   \."\" O : open file in previously visited window\n"
	   \."\" p : preview the file\n"
      if exists("g:explFileHandler")
	let @f=@f."\" x : execute file or directory\n"
      endif
      let @f=@f
	   \."\" i : toggle size/date listing\n"
	   \."\" s : select sort field    r : reverse sort\n"
	   \."\" - : go up one level      c : cd to this dir\n"
	   \."\" R : rename file          D : delete file\n"
	   \."\" :help file-explorer for detailed help\n"
    else
      let @f="\" Press ? for keyboard shortcuts\n"
    endif
    let @f=@f."\" Sorted by ".w:sortdirlabel.w:sorttype.b:suffixeslast.b:filtering."\n"
    let @f=@f."\"= ".b:completePath."\n"
    put! f
    let @f=save_f
endfunction


"---
" Show the size and date for each file
"
function! s:AddFileInfo()
  let save_sc = &sc
  set nosc

  " Mark our starting point
  normal! mt

  call s:RemoveSeparators()

  " Remove all info
  0
  /^"=/+1,$g/^/call setline(line("."),s:GetFileName())

  " Add info if requested
  if w:longlist==1
    " Add file size and calculate maximum length of file size field
    let b:maxFileSizeLen = 0
    0
    /^"=/+1,$g/^/let fn=s:GetFullFileName() |
		   \let fileSize=getfsize(fn) |
		   \let fileSizeLen=strlen(fileSize) |
		   \if fileSizeLen > b:maxFileSizeLen |
		   \  let b:maxFileSizeLen = fileSizeLen |
		   \endif |
		   \exec "normal! ".(b:maxFileLen-strlen(getline("."))+2)."A \<esc>" |
		   \exec 's/$/'.fileSize.'/'

    " Right justify the file sizes and
    " add file modification date
    0
    /^"=/+1,$g/^/let fn=s:GetFullFileName() |
		   \exec "normal! A \<esc>$b".(b:maxFileLen+b:maxFileSizeLen-strlen(getline("."))+3)."i \<esc>\"_x" |
		   \exec 's/$/ '.escape(s:FileModDate(fn), '/').'/'
    setlocal nomodified
  endif

  call s:AddSeparators()

  " return to start
  normal! `t

  let &sc = save_sc
endfunction


"----
" Get the modification time for a file
"
function! s:FileModDate(name)
  let filetime=getftime(a:name)
  if filetime > 0
    return strftime(g:explDateFormat,filetime) . " ««" . filetime
  else
    return ""
  endif
endfunction

"---
" Delete a file or files
"
function! s:DeleteFile() range
  let oldRep = &report
  let &report = 1000

  let filesDeleted = 0
  let stopDel = 0
  let delAll = 0
  let currLine = a:firstline
  let lastLine = a:lastline
  setlocal noreadonly modifiable

  while ((currLine <= lastLine) && (stopDel==0))
    exec(currLine)
    let fileName=s:GetFullFileName()
    if isdirectory(fileName)
      echo fileName." : Directory deletion not supported yet"
      let currLine = currLine + 1
    else
      if delAll == 0
	let sure=input("Delete ".fileName." (y/n/a/q)? ")
	if sure=="a"
	  let delAll = 1
	endif
      endif
      if (sure=="y") || (sure=="a")
	let success=delete(fileName)
	if success!=0
	  exec (" ")
	  echo "\nCannot delete ".fileName
	  let currLine = currLine + 1
	else
	  d _
	  let filesDeleted = filesDeleted + 1
	  let lastLine = lastLine - 1
	endif
      elseif sure=="q"
	let stopDel = 1
      elseif sure=="n"
	let currLine = currLine + 1
      endif
    endif
  endwhile
  echo "\n".filesDeleted." files deleted"
  let &report = oldRep
  setlocal nomodified
  setlocal readonly nomodifiable
endfunction

"---
" Rename a file
"
function! s:RenameFile()
  let fileName=s:GetFullFileName()
  setlocal noreadonly modifiable
  if isdirectory(fileName)
    echo "Directory renaming not supported yet"
  elseif filereadable(fileName)
    let altName=input("Rename ".fileName." to : ")
    echo " "
    if altName==""
      setlocal readonly nomodifiable
      return
    endif
    let success=rename(fileName, b:completePath.altName)
    if success!=0
      echo "Cannot rename ".fileName. " to ".altName
    else
      echo "Renamed ".fileName." to ".altName
      let oldRep=&report
      set report=1000
      e!
      let &report=oldRep
    endif
  endif
  setlocal nomodified
  setlocal readonly nomodifiable
endfunction

"---
" Toggle between short and long help
"
function! s:ToggleHelp()
  if exists("w:longhelp") && w:longhelp==0
    let w:longhelp=1
    let s:longhelp=1
  else
    let w:longhelp=0
    let s:longhelp=0
  endif
  " Allow modification
  setlocal noreadonly modifiable
  call s:UpdateHeader()
  " Disallow modification
  setlocal readonly nomodifiable
endfunction

"---
" Update the header
"
function! s:UpdateHeader()
  let oldRep=&report
  set report=10000
  " Save position
  normal! mt
  " Remove old header
  0
  1,/^"=/ d _
  " Add new header
  call s:AddHeader()
  " Go back where we came from if possible
  0
  if line("'t") != 0
    normal! `t
  endif

  let &report=oldRep
  setlocal nomodified
endfunction

"---
" Toggle long vs. short listing
"
function! s:ToggleLongList()
  setlocal noreadonly modifiable
  if exists("w:longlist") && w:longlist==1
    let w:longlist=0
    let s:longlist=0
  else
    let w:longlist=1
    let s:longlist=1
  endif
  call s:AddFileInfo()
  setlocal readonly nomodifiable
endfunction

"---
" Show all files - remove filtering
"
function! s:ShowAllFiles()
  setlocal noreadonly modifiable
  let b:filterFormula=""
  let b:filtering=""
  call s:ShowDirectory()
  setlocal readonly nomodifiable
endfunction

"---
" Figure out what section we are in
"
function! s:GetSection()
  let fn=s:GetFileName()
  let section="file"
  if fn =~ '/$'
    let section="directory"
  elseif fn =~ b:suffixesRegexp
    let section="suffixes"
  endif
  return section
endfunction

"---
" Remove section separators
"
function! s:RemoveSeparators()
  if !g:explUseSeparators
    return
  endif
  0
  silent! exec '/^"=/+1,$g/^' . s:separator . "/d _"
endfunction

"---
" Add section separators
"   between directories and files if they are separated
"   between files and 'suffixes' files if they are separated
function! s:AddSeparators()
  if !g:explUseSeparators
    return
  endif
  0
  /^"=/+1
  let lastsec=s:GetSection()
  +1
  .,$g/^/let sec=s:GetSection() |
	       \if g:explDirsFirst != 0 && sec != lastsec &&
	       \   (lastsec == "directory" || sec == "directory") |
	       \  exec "normal! I" . s:separator . "\n\<esc>" |
	       \elseif g:explSuffixesLast != 0 && sec != lastsec &&
	       \   (lastsec == "suffixes" || sec == "suffixes") |
	       \  exec "normal! I" . s:separator . "\n\<esc>" |
	       \endif |
	       \let lastsec=sec
endfunction

"---
" General string comparison function
"
function! s:StrCmp(line1, line2, direction)
  if a:line1 < a:line2
    return -a:direction
  elseif a:line1 > a:line2
    return a:direction
  else
    return 0
  endif
endfunction

"---
" Function for use with Sort(), to compare the file names
" Default sort is to put in alphabetical order, but with all directory
" names before all file names
"
function! s:FileNameCmp(line1, line2, direction)
  let f1=s:ExtractFileName(a:line1)
  let f2=s:ExtractFileName(a:line2)

  " Put directory names before file names
  if (g:explDirsFirst != 0) && (f1 =~ '\/$') && (f2 !~ '\/$')
    return -g:explDirsFirst
  elseif (g:explDirsFirst != 0) && (f1 !~ '\/$') && (f2 =~ '\/$')
    return g:explDirsFirst
  elseif (g:explSuffixesLast != 0) && (f1 =~ b:suffixesRegexp) && (f2 !~ b:suffixesRegexp)
    return g:explSuffixesLast
  elseif (g:explSuffixesLast != 0) && (f1 !~ b:suffixesRegexp) && (f2 =~ b:suffixesRegexp)
    return -g:explSuffixesLast
  else
    return s:StrCmp(substitute(f1, "/$", "", ""), substitute(f2, "/$", "", ""), a:direction)
  endif

endfunction

"---
" Function for use with Sort(), to compare the file modification dates
" Default sort is to put NEWEST files first.  Reverse will put oldest
" files first
"
function! s:FileDateCmp(line1, line2, direction)
  let f1=s:ExtractFileName(a:line1)
  let f2=s:ExtractFileName(a:line2)
  let t1=s:ExtractFileDate(a:line1)
  let t2=s:ExtractFileDate(a:line2)

  " Put directory names before file names
  if (g:explDirsFirst != 0) && (f1 =~ '\/$') && (f2 !~ '\/$')
    return -g:explDirsFirst
  elseif (g:explDirsFirst != 0) && (f1 !~ '\/$') && (f2 =~ '\/$')
    return g:explDirsFirst
  elseif (g:explSuffixesLast != 0) && (f1 =~ b:suffixesRegexp) && (f2 !~ b:suffixesRegexp)
    return g:explSuffixesLast
  elseif (g:explSuffixesLast != 0) && (f1 !~ b:suffixesRegexp) && (f2 =~ b:suffixesRegexp)
    return -g:explSuffixesLast
  elseif t1 > t2
    return -a:direction
  elseif t1 < t2
    return a:direction
  else
    return s:StrCmp(substitute(f1, "/$", "", ""), substitute(f2, "/$", "", ""), 1)
  endif
endfunction

"---
" Function for use with Sort(), to compare the file sizes
" Default sort is to put largest files first.  Reverse will put
" smallest files first
"
function! s:FileSizeCmp(line1, line2, direction)
  let f1=s:ExtractFileName(a:line1)
  let f2=s:ExtractFileName(a:line2)
  let s1=s:ExtractFileSize(a:line1)
  let s2=s:ExtractFileSize(a:line2)

  if (g:explDirsFirst != 0) && (f1 =~ '\/$') && (f2 !~ '\/$')
    return -g:explDirsFirst
  elseif (g:explDirsFirst != 0) && (f1 !~ '\/$') && (f2 =~ '\/$')
    return g:explDirsFirst
  elseif (g:explSuffixesLast != 0) && (f1 =~ b:suffixesRegexp) && (f2 !~ b:suffixesRegexp)
    return g:explSuffixesLast
  elseif (g:explSuffixesLast != 0) && (f1 !~ b:suffixesRegexp) && (f2 =~ b:suffixesRegexp)
    return -g:explSuffixesLast
  elseif s1 > s2
    return -a:direction
  elseif s1 < s2
    return a:direction
  else
    return s:StrCmp(substitute(f1, "/$", "", ""), substitute(f2, "/$", "", ""), 1)
  endif
endfunction

"---
" Sort lines.  SortR() is called recursively.
"
function! s:SortR(start, end, cmp, direction)

  " Bottom of the recursion if start reaches end
  if a:start >= a:end
    return
  endif
  "
  let partition = a:start - 1
  let middle = partition
  let partStr = getline((a:start + a:end) / 2)
  let i = a:start
  while (i <= a:end)
    let str = getline(i)
    exec "let result = " . a:cmp . "(str, partStr, " . a:direction . ")"
    if result <= 0
      " Need to put it before the partition.  Swap lines i and partition.
      let partition = partition + 1
      if result == 0
	let middle = partition
      endif
      if i != partition
	let str2 = getline(partition)
	call setline(i, str2)
	call setline(partition, str)
      endif
    endif
    let i = i + 1
  endwhile

  " Now we have a pointer to the "middle" element, as far as partitioning
  " goes, which could be anywhere before the partition.  Make sure it is at
  " the end of the partition.
  if middle != partition
    let str = getline(middle)
    let str2 = getline(partition)
    call setline(middle, str2)
    call setline(partition, str)
  endif
  call s:SortR(a:start, partition - 1, a:cmp,a:direction)
  call s:SortR(partition + 1, a:end, a:cmp,a:direction)
endfunction

"---
" To Sort a range of lines, pass the range to Sort() along with the name of a
" function that will compare two lines.
"
function! s:Sort(cmp,direction) range
  call s:SortR(a:firstline, a:lastline, a:cmp, a:direction)
endfunction

"---
" Reverse the current sort order
"
function! s:SortReverse()
  if exists("w:sortdirection") && w:sortdirection == -1
    let w:sortdirection = 1
    let w:sortdirlabel  = ""
  else
    let w:sortdirection = -1
    let w:sortdirlabel  = "reverse "
  endif
  let s:sortby=w:sortdirlabel . w:sorttype
  call s:SortListing("")
endfunction

"---
" Toggle through the different sort orders
"
function! s:SortSelect()
  " Select the next sort option
  if !exists("w:sorttype")
    let w:sorttype="name"
  elseif w:sorttype == "name"
    let w:sorttype="size"
  elseif w:sorttype == "size"
    let w:sorttype="date"
  else
    let w:sorttype="name"
  endif
  let s:sortby=w:sortdirlabel . w:sorttype
  call s:SortListing("")
endfunction

"---
" Sort the file listing
"
function! s:SortListing(msg)
    " Save the line we start on so we can go back there when done
    " sorting
    let startline = getline(".")
    let col=col(".")
    let lin=line(".")

    " Allow modification
    setlocal noreadonly modifiable

    " Send a message about what we're doing
    " Don't really need this - it can cause hit return prompts
"   echo a:msg . "Sorting by" . w:sortdirlabel . w:sorttype

    " Create a regular expression out of the suffixes option in case
    " we need it.
    call s:SetSuffixesLast()

    " Remove section separators
    call s:RemoveSeparators()

    " Do the sort
    0
    if w:sorttype == "size"
      /^"=/+1,$call s:Sort("s:FileSizeCmp",w:sortdirection)
    elseif w:sorttype == "date"
      /^"=/+1,$call s:Sort("s:FileDateCmp",w:sortdirection)
    else
      /^"=/+1,$call s:Sort("s:FileNameCmp",w:sortdirection)
    endif

    " Replace the header with updated information
    call s:UpdateHeader()

    " Restore section separators
    call s:AddSeparators()

    " Return to the position we started on
    0
    if search('\m^'.escape(startline,s:escregexp),'W') <= 0
      execute lin
    endif
    execute "normal!" col . "|"

    " Disallow modification
    setlocal nomodified
    setlocal readonly nomodifiable

endfunction

"---
" Setup for editing directories after starting up by going to each window.
" Required for "vim -o filename dirname"
"
function! s:EditAll()
  if winbufnr(2) == -1
    return
  endif
  let cmd = winrestcmd()
  let curwin = winnr()
  while 1
    wincmd w
    if winnr() == curwin
      break
    endif
    call s:EditDir()
  endwhile
  exe cmd
endfunction

"---
" Set up the autocommand to allow directories to be edited
"
augroup fileExplorer
  au!
  " Fill the window when entering the buffer; ":edit dir".
  au BufEnter * call s:EditDir()
  " Set the window variables after a split; ":split".
  au WinEnter * if !exists("w:sortdirection") | call s:EditDir() | endif
  " Fill the windows after Vim has started up.
  au VimEnter * call s:EditAll()
augroup end

" restore 'cpo'
let &cpo = s:cpo_save
unlet s:cpo_save
