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

if has("autocmd")
  augroup tar
    au!
    au BufReadCmd   tarfile:*	call tar#Read(expand("<afile>"), 1)
    au BufReadCmd   tarfile:*/*	call tar#Read(expand("<afile>"), 1)
    au FileReadCmd  tarfile:*	call tar#Read(expand("<afile>"), 0)
    au FileReadCmd  tarfile:*/*	call tar#Read(expand("<afile>"), 0)

    au BufWriteCmd  tarfile:*	call tar#Write(expand("<afile>"))
    au BufWriteCmd  tarfile:*/*	call tar#Write(expand("<afile>"))
    au FileWriteCmd tarfile:*	call tar#Write(expand("<afile>"))
    au FileWriteCmd tarfile:*/*	call tar#Write(expand("<afile>"))

    au BufReadCmd   *.tar	call tar#Browse(expand("<afile>"))
    au BufReadCmd   *.tar.gz	call tar#Browse(expand("<afile>"))
    au BufReadCmd   *.tar.bz2	call tar#Browse(expand("<afile>"))
    au BufReadCmd   *.tar.Z	call tar#Browse(expand("<afile>"))
    au BufReadCmd   *.tgz	call tar#Browse(expand("<afile>"))
  augroup END
endif

" vim: ts=8
