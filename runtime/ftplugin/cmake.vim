" Vim filetype plugin
" Language:    CMake
" Maintainer:  Keith Smiley <keithbsmiley@gmail.com>
" Last Change: 2017 Dec 24

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl commentstring<"

if exists("loaded_matchit")
  let b:match_words =  '^[^#]*\<if\>\s*(:^[^#]*\<else\>\s*(:^[^#]*\<elseif\>\s*(:^[^#]*\<endif\>\s*(\c'
  let b:match_words .= ',^[^#]*\<function\>\s*(:^[^#]*\<endfunction\>\s*(\c'
  let b:match_words .= ',^[^#]*\<foreach\>\s*(:^[^#]*\<endforeach\>\s*(\c'
  let b:match_words .= ',^[^#]*\<macro\>\s*(:^[^#]*\<endmacro\>\s*(\c'
  let b:match_words .= ',^[^#]*\<while\>\s*(:^[^#]*\<endwhile\>\s*(\c'
endif

setlocal commentstring=#\ %s
