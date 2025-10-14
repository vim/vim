" Vim tutor support file
" Author:	Eduardo F. Amatria <eferna1@platea.pntic.mec.es>
" Maintainer:	The·Vim·Project·<https://github.com/vim/vim>
" Last Change:	2025 Jun 20

" This Vim script is used for detecting if a translation of the
" tutor file exist, i.e., a tutor.xx file, where xx is the language.
" If the translation does not exist, or no extension is given,
" it defaults to the English version.

" It is invoked by the vimtutor shell script.

" 1. Build the extension of the file, if any:
let s:ext = ""
if strlen($xx) > 1
  let s:ext = "." .. $xx
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
    if s:lang =~ "German"
      let s:ext = ".de"
    elseif s:lang =~ "Polish"
      let s:ext = ".pl"
    elseif s:lang =~ "Slovak"
      let s:ext = ".sk"
    elseif s:lang =~ "Serbian"
      let s:ext = ".sr"
    elseif s:lang =~ "Czech"
      let s:ext = ".cs"
    elseif s:lang =~ "Dutch"
      let s:ext = ".nl"
    elseif s:lang =~ "Bulgarian"
      let s:ext = ".bg"
    else
      let s:ext = "." .. strpart(s:lang, 0, 2)
    endif
  endif
endif

" Somehow ".ge" (Germany) is sometimes used for ".de" (Deutsch).
if s:ext =~? '\.ge'
  let s:ext = ".de"
endif

if s:ext =~? '\.en'
  let s:ext = ""
endif

" Choose between Chinese (Simplified) and Chinese (Traditional)
" based on the language, suggested by Alick Zhao.
if s:ext =~? '\.zh'
  if s:ext =~? 'zh_tw' || (exists("s:lang") && s:lang =~? 'zh_tw')
    let s:ext = ".zh_tw"
  else
    let s:ext = ".zh_cn"
  endif
endif

" 2. Build the name of the file and chapter
let s:chapter = exists("$CHAPTER") ? $CHAPTER : 1

let s:tutorfile = "/tutor/tutor" .. s:chapter
let s:tutorxx = $VIMRUNTIME .. s:tutorfile .. s:ext

" 3. Finding the file:
if filereadable(s:tutorxx)
  let $TUTOR = s:tutorxx
else
  let $TUTOR = $VIMRUNTIME .. s:tutorfile
  echo "The file " .. s:tutorxx .. " does not exist.\n"
  echo "Copying English version: " .. $TUTOR
  4sleep
endif

" 4. Making the copy and exiting Vim:
e $TUTOR
wq! $TUTORCOPY
