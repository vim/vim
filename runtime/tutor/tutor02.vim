" Vim tutor support file
" Author: Eduardo F. Amatria <eferna1@platea.pntic.mec.es>
" Maintainer: Bram Moolenaar
" Last Change:	2019 Nov 11

" This Vim script is used for detecting if a translation of the
" tutor file exist, i.e., a tutor.xx file, where xx is the language.
" If the translation does not exist, or no extension is given,
" it defaults to the English version.

" It is invoked by the vimtutor shell script.

" 1. Build the extension of the file, if any:
let s:ext = ""
if strlen($xx) > 1
  let s:ext = "." . $xx
else
  let s:lang = ""
  " Check that a potential value has at least two letters.
  " Ignore "1043" and "C".
  if exists("v:lang") && v:lang =~ '\a\a'
    let s:lang = v:lang
  elseif $LC_ALL =~ '\a\a'
    let s:lang = $LC_ALL
  elseif $LC_MESSAGES =~ '\a\a' || $LC_MESSAGES ==# "C"
    " LC_MESSAGES=C can be used to explicitly ask for English messages while
    " keeping LANG non-English; don't set s:lang then.
    if $LC_MESSAGES =~ '\a\a'
      let s:lang = $LC_MESSAGES
    endif
  elseif $LANG =~ '\a\a'
    let s:lang = $LANG
  endif
  if s:lang != ""
    " Remove "@euro" (ignoring case), it may be at the end
    let s:lang = substitute(s:lang, '\c@euro', '', '')
    " On MS-Windows it may be German_Germany.1252 or Polish_Poland.1250.  How
    " about other languages?
"   if s:lang =~ "German"
"     let s:ext = ".de"
"   elseif s:lang =~ "Polish"
"     let s:ext = ".pl"
"   elseif s:lang =~ "Slovak"
"     let s:ext = ".sk"
"   elseif s:lang =~ "Serbian"
"     let s:ext = ".sr"
"   elseif s:lang =~ "Czech"
"     let s:ext = ".cs"
"   elseif s:lang =~ "Dutch"
"     let s:ext = ".nl"
"   elseif s:lang =~ "Bulgarian"
"     let s:ext = ".bg"
"   else
"     let s:ext = "." . strpart(s:lang, 0, 2)
"   endif
    let s:ext = "." . strpart(s:lang, 0, 2)
  endif
endif

if s:ext =~? '\.en'
  let s:ext = ""
endif

" If 'encoding' is utf-8 s:ext must end in utf-8.
if &enc == 'utf-8' && s:ext !~ '\.utf-8'
  let s:ext .= '.utf-8'
endif

" 2. Build the name of the file:
let s:tutorfile = "/tutor/tutor02"
let s:tutorxx = $VIMRUNTIME . s:tutorfile . s:ext

" 3. Finding the file:
if filereadable(s:tutorxx)
  let $TUTOR = s:tutorxx
elseif s:ext !~ '\.utf-8' && filereadable(s:tutorxx . ".utf-8")
  " Fallback to utf-8 if available.
  let $TUTOR = s:tutorxx . ".utf-8"
else
  let $TUTOR = $VIMRUNTIME . s:tutorfile
  echo "The file " . s:tutorxx . " does not exist.\n"
  echo "Copying English version: " . $TUTOR
  4sleep
endif

" 4. Making the copy and exiting Vim:
e $TUTOR
wq! $TUTORCOPY
