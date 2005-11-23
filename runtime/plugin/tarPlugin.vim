" tarPlugin.vim -- a Vim plugin for browsing tarfiles
" Copyright (c) 2002, Michael C. Toren <mct@toren.net>
" Distributed under the GNU General Public License.
"
" Updates are available from <http://michael.toren.net/code/>.  If you
" find this script useful, or have suggestions for improvements, please
" let me know.
" Also look there for further comments and documentation.
"
" This part only sets the autocommands.  The functions are in autoload/tar.vim.

augroup tar
  au!
  au BufReadCmd   tarfile:*	call tar#Read(expand("<amatch>"), 1)
  au FileReadCmd  tarfile:*	call tar#Read(expand("<amatch>"), 0)
  au BufWriteCmd  tarfile:*	call tar#Write(expand("<amatch>"))
  au FileWriteCmd tarfile:*	call tar#Write(expand("<amatch>"))

  if has("unix")
   au BufReadCmd   tarfile:*/*	call tar#Read(expand("<amatch>"), 1)
   au FileReadCmd  tarfile:*/*	call tar#Read(expand("<amatch>"), 0)
   au BufWriteCmd  tarfile:*/*	call tar#Write(expand("<amatch>"))
   au FileWriteCmd tarfile:*/*	call tar#Write(expand("<amatch>"))
  endif

  au BufReadCmd   *.tar		call tar#Browse(expand("<amatch>"))
  au BufReadCmd   *.tar.gz	call tar#Browse(expand("<amatch>"))
  au BufReadCmd   *.tar.bz2	call tar#Browse(expand("<amatch>"))
  au BufReadCmd   *.tar.Z	call tar#Browse(expand("<amatch>"))
  au BufReadCmd   *.tgz		call tar#Browse(expand("<amatch>"))
augroup END

" vim: ts=8
