" Vim script to clean the ll.xxxxx.add files of commented out entries
" Author:	Antonio Colombo, Bram Moolenaar
" Last Update:	2006 Jan 19

" Time in seconds after last time an ll.xxxxx.add file was updated
" Default is one second.
" If you invoke this script often set it to something bigger, e.g. 60 * 60
" (one hour)
if !exists("g:spell_clean_limit")
  let g:spell_clean_limit = 1
endif

" Loop over all the runtime/spell/*.add files.
" Delete all comment lines, except the ones starting with ##.
for s:fname in split(globpath(&rtp, "spell/*.add"), "\n")
  if filewritable(s:fname) && localtime() - getftime(s:fname) > g:spell_clean_limit
    silent exe "tab split " . escape(s:fname, ' \')
    echo "Processing" s:fname
    silent! g/^#[^#]/d
    silent update
    close
  endif
endfor

echo "Done"
