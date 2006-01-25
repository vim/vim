" Vim script to clean the ll.xxxxx.add files of commented out entries
" Author:	Antonio Colombo, Bram Moolenaar
" Last Update:	2006 Jan 19

" Time in seconds after last time an ll.xxxxx.add file was updated
" Default is one hour.
if !exists("g:spell_clean_limit")
  let g:spell_clean_limit = 60 * 60
endif

" Loop over all the runtime/spell/*.add files.
for s:fname in split(globpath(&rtp, "spell/*.add"), "\n")
  if filewritable(s:fname) && localtime() - getftime(s:fname) > g:spell_clean_limit
    silent exe "split " . escape(s:fname, ' \')
    echo "Processing" s:fname
    silent! g/^#/d
    silent update
    close
  endif
endfor

echo "Done"
