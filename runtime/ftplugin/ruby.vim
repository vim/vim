" Vim filetype plugin
" Language:		Ruby
" Maintainer:		Gavin Sinclair <gsinclair at gmail.com>
" Last Change:		2010 Mar 15
" URL:			http://vim-ruby.rubyforge.org
" Anon CVS:		See above site
" Release Coordinator:  Doug Kearns <dougkearns@gmail.com>
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

if has("gui_running") && !has("gui_win32")
  setlocal keywordprg=ri\ -T
else
  setlocal keywordprg=ri
endif

" Matchit support
if exists("loaded_matchit") && !exists("b:match_words")
  let b:match_ignorecase = 0

  let b:match_words =
	\ '\<\%(if\|unless\|case\|while\|until\|for\|do\|class\|module\|def\|begin\)\>=\@!' .
	\ ':' .
	\ '\<\%(else\|elsif\|ensure\|when\|rescue\|break\|redo\|next\|retry\)\>' .
	\ ':' .
	\ '\<end\>' .
	\ ',{:},\[:\],(:)'

  let b:match_skip =
	\ "synIDattr(synID(line('.'),col('.'),0),'name') =~ '" .
	\ "\\<ruby\\%(String\\|StringDelimiter\\|ASCIICode\\|Escape\\|" .
	\ "Interpolation\\|NoInterpolation\\|Comment\\|Documentation\\|" .
	\ "ConditionalModifier\\|RepeatModifier\\|OptionalDo\\|" .
	\ "Function\\|BlockArgument\\|KeywordAsMethod\\|ClassVariable\\|" .
	\ "InstanceVariable\\|GlobalVariable\\|Symbol\\)\\>'"
endif

setlocal formatoptions-=t formatoptions+=croql

setlocal include=^\\s*\\<\\(load\\\|\w*require\\)\\>
setlocal includeexpr=substitute(substitute(v:fname,'::','/','g'),'$','.rb','')
setlocal suffixesadd=.rb

if exists("&ofu") && has("ruby")
  setlocal omnifunc=rubycomplete#Complete
endif

" To activate, :set ballooneval
if has('balloon_eval') && exists('+balloonexpr')
  setlocal balloonexpr=RubyBalloonexpr()
endif


" TODO:
"setlocal define=^\\s*def

setlocal comments=:#
setlocal commentstring=#\ %s

if !exists("s:ruby_path")
  if exists("g:ruby_path")
    let s:ruby_path = g:ruby_path
  elseif has("ruby") && has("win32")
    ruby VIM::command( 'let s:ruby_path = "%s"' % ($: + begin; require %q{rubygems}; Gem.all_load_paths.sort.uniq; rescue LoadError; []; end).join(%q{,}) )
    let s:ruby_path = '.,' . substitute(s:ruby_path, '\%(^\|,\)\.\%(,\|$\)', ',,', '')
  elseif executable("ruby")
    let s:code = "print ($: + begin; require %q{rubygems}; Gem.all_load_paths.sort.uniq; rescue LoadError; []; end).join(%q{,})"
    if &shellxquote == "'"
      let s:ruby_path = system('ruby -e "' . s:code . '"')
    else
      let s:ruby_path = system("ruby -e '" . s:code . "'")
    endif
    let s:ruby_path = '.,' . substitute(s:ruby_path, '\%(^\|,\)\.\%(,\|$\)', ',,', '')
  else
    " If we can't call ruby to get its path, just default to using the
    " current directory and the directory of the current file.
    let s:ruby_path = ".,,"
  endif
endif

let &l:path = s:ruby_path

if has("gui_win32") && !exists("b:browsefilter")
  let b:browsefilter = "Ruby Source Files (*.rb)\t*.rb\n" .
                     \ "All Files (*.*)\t*.*\n"
endif

let b:undo_ftplugin = "setl fo< inc< inex< sua< def< com< cms< path< kp<"
      \."| unlet! b:browsefilter b:match_ignorecase b:match_words b:match_skip"
      \."| if exists('&ofu') && has('ruby') | setl ofu< | endif"
      \."| if has('balloon_eval') && exists('+bexpr') | setl bexpr< | endif"

if !exists("g:no_plugin_maps") && !exists("g:no_ruby_maps")

  nnoremap <silent> <buffer> [m :<C-U>call <SID>searchsyn('\<def\>','rubyDefine','b','n')<CR>
  nnoremap <silent> <buffer> ]m :<C-U>call <SID>searchsyn('\<def\>','rubyDefine','','n')<CR>
  nnoremap <silent> <buffer> [M :<C-U>call <SID>searchsyn('\<end\>','rubyDefine','b','n')<CR>
  nnoremap <silent> <buffer> ]M :<C-U>call <SID>searchsyn('\<end\>','rubyDefine','','n')<CR>
  xnoremap <silent> <buffer> [m :<C-U>call <SID>searchsyn('\<def\>','rubyDefine','b','v')<CR>
  xnoremap <silent> <buffer> ]m :<C-U>call <SID>searchsyn('\<def\>','rubyDefine','','v')<CR>
  xnoremap <silent> <buffer> [M :<C-U>call <SID>searchsyn('\<end\>','rubyDefine','b','v')<CR>
  xnoremap <silent> <buffer> ]M :<C-U>call <SID>searchsyn('\<end\>','rubyDefine','','v')<CR>

  nnoremap <silent> <buffer> [[ :<C-U>call <SID>searchsyn('\<\%(class\<Bar>module\)\>','rubyModule\<Bar>rubyClass','b','n')<CR>
  nnoremap <silent> <buffer> ]] :<C-U>call <SID>searchsyn('\<\%(class\<Bar>module\)\>','rubyModule\<Bar>rubyClass','','n')<CR>
  nnoremap <silent> <buffer> [] :<C-U>call <SID>searchsyn('\<end\>','rubyModule\<Bar>rubyClass','b','n')<CR>
  nnoremap <silent> <buffer> ][ :<C-U>call <SID>searchsyn('\<end\>','rubyModule\<Bar>rubyClass','','n')<CR>
  xnoremap <silent> <buffer> [[ :<C-U>call <SID>searchsyn('\<\%(class\<Bar>module\)\>','rubyModule\<Bar>rubyClass','b','v')<CR>
  xnoremap <silent> <buffer> ]] :<C-U>call <SID>searchsyn('\<\%(class\<Bar>module\)\>','rubyModule\<Bar>rubyClass','','v')<CR>
  xnoremap <silent> <buffer> [] :<C-U>call <SID>searchsyn('\<end\>','rubyModule\<Bar>rubyClass','b','v')<CR>
  xnoremap <silent> <buffer> ][ :<C-U>call <SID>searchsyn('\<end\>','rubyModule\<Bar>rubyClass','','v')<CR>

  let b:undo_ftplugin = b:undo_ftplugin
        \."| sil! exe 'unmap <buffer> [[' | sil! exe 'unmap <buffer> ]]' | sil! exe 'unmap <buffer> []' | sil! exe 'unmap <buffer> ]['"
        \."| sil! exe 'unmap <buffer> [m' | sil! exe 'unmap <buffer> ]m' | sil! exe 'unmap <buffer> [M' | sil! exe 'unmap <buffer> ]M'"

  if maparg("\<C-]>",'n') == ''
    nnoremap <silent> <buffer> <C-]>       :<C-U>exe  v:count1."tag <C-R>=RubyCursorIdentifier()<CR>"<CR>
    nnoremap <silent> <buffer> g<C-]>      :<C-U>exe         "tjump <C-R>=RubyCursorIdentifier()<CR>"<CR>
    nnoremap <silent> <buffer> g]          :<C-U>exe       "tselect <C-R>=RubyCursorIdentifier()<CR>"<CR>
    nnoremap <silent> <buffer> <C-W>]      :<C-U>exe v:count1."stag <C-R>=RubyCursorIdentifier()<CR>"<CR>
    nnoremap <silent> <buffer> <C-W><C-]>  :<C-U>exe v:count1."stag <C-R>=RubyCursorIdentifier()<CR>"<CR>
    nnoremap <silent> <buffer> <C-W>g<C-]> :<C-U>exe        "stjump <C-R>=RubyCursorIdentifier()<CR>"<CR>
    nnoremap <silent> <buffer> <C-W>g]     :<C-U>exe      "stselect <C-R>=RubyCursorIdentifier()<CR>"<CR>
    nnoremap <silent> <buffer> <C-W>}      :<C-U>exe          "ptag <C-R>=RubyCursorIdentifier()<CR>"<CR>
    nnoremap <silent> <buffer> <C-W>g}     :<C-U>exe        "ptjump <C-R>=RubyCursorIdentifier()<CR>"<CR>
    let b:undo_ftplugin = b:undo_ftplugin
          \."| sil! exe 'nunmap <buffer> <C-]>'| sil! exe 'nunmap <buffer> g<C-]>'| sil! exe 'nunmap <buffer> g]'"
          \."| sil! exe 'nunmap <buffer> <C-W>]'| sil! exe 'nunmap <buffer> <C-W><C-]>'"
          \."| sil! exe 'nunmap <buffer> <C-W>g<C-]>'| sil! exe 'nunmap <buffer> <C-W>g]'"
          \."| sil! exe 'nunmap <buffer> <C-W>}'| sil! exe 'nunmap <buffer> <C-W>g}'"
  endif
endif

let &cpo = s:cpo_save
unlet s:cpo_save

if exists("g:did_ruby_ftplugin_functions")
  finish
endif
let g:did_ruby_ftplugin_functions = 1

function! RubyBalloonexpr()
  if !exists('s:ri_found')
    let s:ri_found = executable('ri')
  endif
  if s:ri_found
    let line = getline(v:beval_lnum)
    let b = matchstr(strpart(line,0,v:beval_col),'\%(\w\|[:.]\)*$')
    let a = substitute(matchstr(strpart(line,v:beval_col),'^\w*\%([?!]\|\s*=\)\?'),'\s\+','','g')
    let str = b.a
    let before = strpart(line,0,v:beval_col-strlen(b))
    let after  = strpart(line,v:beval_col+strlen(a))
    if str =~ '^\.'
      let str = substitute(str,'^\.','#','g')
      if before =~ '\]\s*$'
        let str = 'Array'.str
      elseif before =~ '}\s*$'
        " False positives from blocks here
        let str = 'Hash'.str
      elseif before =~ "[\"'`]\\s*$" || before =~ '\$\d\+\s*$'
        let str = 'String'.str
      elseif before =~ '\$\d\+\.\d\+\s*$'
        let str = 'Float'.str
      elseif before =~ '\$\d\+\s*$'
        let str = 'Integer'.str
      elseif before =~ '/\s*$'
        let str = 'Regexp'.str
      else
        let str = substitute(str,'^#','.','')
      endif
    endif
    let str = substitute(str,'.*\.\s*to_f\s*\.\s*','Float#','')
    let str = substitute(str,'.*\.\s*to_i\%(nt\)\=\s*\.\s*','Integer#','')
    let str = substitute(str,'.*\.\s*to_s\%(tr\)\=\s*\.\s*','String#','')
    let str = substitute(str,'.*\.\s*to_sym\s*\.\s*','Symbol#','')
    let str = substitute(str,'.*\.\s*to_a\%(ry\)\=\s*\.\s*','Array#','')
    let str = substitute(str,'.*\.\s*to_proc\s*\.\s*','Proc#','')
    if str !~ '^\w'
      return ''
    endif
    silent! let res = substitute(system("ri -f simple -T \"".str.'"'),'\n$','','')
    if res =~ '^Nothing known about' || res =~ '^Bad argument:' || res =~ '^More than one method'
      return ''
    endif
    return res
  else
    return ""
  endif
endfunction

function! s:searchsyn(pattern,syn,flags,mode)
    norm! m'
    if a:mode ==# 'v'
      norm! gv
    endif
    let i = 0
    let cnt = v:count ? v:count : 1
    while i < cnt
        let i = i + 1
        let line = line('.')
        let col  = col('.')
        let pos = search(a:pattern,'W'.a:flags)
        while pos != 0 && s:synname() !~# a:syn
            let pos = search(a:pattern,'W'.a:flags)
        endwhile
        if pos == 0
            call cursor(line,col)
            return
        endif
    endwhile
endfunction

function! s:synname()
    return synIDattr(synID(line('.'),col('.'),0),'name')
endfunction

function! RubyCursorIdentifier()
  let asciicode    = '\%(\w\|[]})\"'."'".']\)\@<!\%(?\%(\\M-\\C-\|\\C-\\M-\|\\M-\\c\|\\c\\M-\|\\c\|\\C-\|\\M-\)\=\%(\\\o\{1,3}\|\\x\x\{1,2}\|\\\=\S\)\)'
  let number       = '\%(\%(\w\|[]})\"'."'".']\s*\)\@<!-\)\=\%(\<[[:digit:]_]\+\%(\.[[:digit:]_]\+\)\=\%([Ee][[:digit:]_]\+\)\=\>\|\<0[xXbBoOdD][[:xdigit:]_]\+\>\)\|'.asciicode
  let operator     = '\%(\[\]\|<<\|<=>\|[!<>]=\=\|===\=\|[!=]\~\|>>\|\*\*\|\.\.\.\=\|=>\|[~^&|*/%+-]\)'
  let method       = '\%(\<[_a-zA-Z]\w*\>\%([?!]\|\s*=>\@!\)\=\)'
  let global       = '$\%([!$&"'."'".'*+,./:;<=>?@\`~]\|-\=\w\+\>\)'
  let symbolizable = '\%(\%(@@\=\)\w\+\>\|'.global.'\|'.method.'\|'.operator.'\)'
  let pattern      = '\C\s*\%('.number.'\|\%(:\@<!:\)\='.symbolizable.'\)'
  let [lnum, col]  = searchpos(pattern,'bcn',line('.'))
  let raw          = matchstr(getline('.')[col-1 : ],pattern)
  let stripped     = substitute(substitute(raw,'\s\+=$','=',''),'^\s*:\=','','')
  return stripped == '' ? expand("<cword>") : stripped
endfunction

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

" vim: nowrap sw=2 sts=2 ts=8:
