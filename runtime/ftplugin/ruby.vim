" Vim filetype plugin
" Language:	Ruby
" Maintainer:	Gavin Sinclair <gsinclair at soyabean.com.au>
" Info:         $Id$
" URL:          http://vim-ruby.sourceforge.net
" Anon CVS:     See above site
" Licence:      GPL (http://www.gnu.org)
" Disclaimer:
"    This program is distributed in the hope that it will be useful,
"    but WITHOUT ANY WARRANTY; without even the implied warranty of
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
"    GNU General Public License for more details.
" ----------------------------------------------------------------------------
"
" Original matchit support thanks to Ned Konz.  See his ftplugin/ruby.vim at
"   http://bike-nomad.com/vim/ruby.vim.
" ----------------------------------------------------------------------------

" Only do this when not done yet for this buffer
if (exists("b:did_ftplugin"))
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

" Matchit support
if exists("loaded_matchit") && !exists("b:match_words")
  let b:match_ignorecase = 0

 " TODO: improve optional do loops
 let b:match_words =
    \ '\%(' .
    \     '\%(\%(\.\|\:\:\)\s*\)\@<!\<\%(class\|module\|begin\|def\|case\|for\|do\)\>' .
    \   '\|' .
    \     '\%(\%(^\|\.\.\.\=\|[\,;=([<>~\*/%!&^|+-]\)\s*\)\@<=\%(if\|unless\|until\|while\)\>' .
    \ '\)' .
    \ ':' .
    \ '\%(' .
    \     '\%(\%(\.\|\:\:\)\s*\)\@<!\<\%(else\|elsif\|ensure\|when\)\>' .
    \   '\|' .
    \     '\%(\%(^\|;\)\s*\)\@<=\<rescue\>' .
    \ '\)' .
    \ ':' .
    \ '\%(\%(\.\|\:\:\)\s*\)\@<!\<end\>'

  let b:match_skip =
     \ "synIDattr(synID(line('.'),col('.'),0),'name') =~ '" .
     \ "\\<ruby\\%(String\\|StringDelimiter\\|ASCIICode\\|Interpolation\\|" .
     \ "NoInterpolation\\|Escape\\|Comment\\|Documentation\\)\\>'"

endif

setlocal formatoptions-=t formatoptions+=croql

setlocal include=^\\s*\\<\\(load\\\|\w*require\\)\\>
setlocal includeexpr=substitute(substitute(v:fname,'::','/','g'),'$','.rb','')
setlocal suffixesadd=.rb

" TODO:
"setlocal define=^\\s*def

setlocal comments=:#
setlocal commentstring=#\ %s

if !exists("s:rubypath")
  if executable("ruby")
    if &shellxquote == "'"
      let s:rubypath = system('ruby -e "puts (begin; require %q{rubygems}; Gem.all_load_paths; rescue LoadError; []; end + $:).join(%q{,})"')
    else
      let s:rubypath = system("ruby -e 'puts (begin; require %q{rubygems}; Gem.all_load_paths; rescue LoadError; []; end + $:).join(%q{,})'")
    endif
    let s:rubypath = substitute(s:rubypath,',.$',',,','')
  else
    " If we can't call ruby to get its path, just default to using the
    " current directory and the directory of the current file.
    let s:rubypath = ".,,"
  endif
endif

let &l:path = s:rubypath

if has("gui_win32") && !exists("b:browsefilter")
  let b:browsefilter = "Ruby Source Files (*.rb)\t*.rb\n" .
                     \ "All Files (*.*)\t*.*\n"
endif

let b:undo_ftplugin = "setl fo< inc< inex< sua< def< com< cms< path< "
      \ "| unlet! b:browsefilter b:match_ignorecase b:match_words b:match_skip"

let &cpo = s:cpo_save
unlet s:cpo_save

"
" Instructions for enabling "matchit" support:
"
" 1. Look for the latest "matchit" plugin at
"
"         http://www.vim.org/scripts/script.php?script_id=39
"
"    It is also packaged with Vim, in the $VIMRUNTIME/macros directory.
"
" 2. Copy "matchit.txt" into a "doc" directory (e.g. $HOME/.vim/doc).
"
" 3. Copy "matchit.vim" into a "plugin" directory (e.g. $HOME/.vim/plugin).
"
" 4. Ensure this file (ftplugin/ruby.vim) is installed.
"
" 5. Ensure you have this line in your $HOME/.vimrc:
"         filetype plugin on
"
" 6. Restart Vim and create the matchit documentation:
"
"         :helptags ~/.vim/doc
"
"    Now you can do ":help matchit", and you should be able to use "%" on Ruby
"    keywords.  Try ":echo b:match_words" to be sure.
"
" Thanks to Mark J. Reed for the instructions.  See ":help vimrc" for the
" locations of plugin directories, etc., as there are several options, and it
" differs on Windows.  Email gsinclair@soyabean.com.au if you need help.
"

" vim: nowrap sw=2 sts=2 ts=8 ff=unix:
