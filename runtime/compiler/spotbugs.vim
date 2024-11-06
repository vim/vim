" Vim compiler file
" Compiler: Spotbugs (Java static checker; needs javac compiled classes)
" Maintainer:   @Konfekt
" Last Change:  2024 nov 6

if exists("current_compiler") | finish | endif
let current_compiler = "spotbugs"

let s:cpo_save = &cpo
set cpo&vim

" Unfortunately Spotbugs does not output absolute paths, so you need to
" pass the directory of the files being checked as `-sourcepath` parameter.
" The auxpath and glob tries to include all dependent classes of that of
" the current buffer. See https://github.com/spotbugs/spotbugs/issues/856
silent exe 'CompilerSet makeprg=spotbugs'..(has('win32')?'.bat':'')..
  \ escape(' -textui -emacs '..
    \ get(b:, 'spotbugs_params', get(g:, 'spotbugs_params', '-workHard -experimental'))..
    \ ' -sourcepath %:p:h:S -auxclasspath %:p:h:S '..
    \ ' %:p:r:S.class '..join(map(glob(expand('%:p:r')..'\$*.class', 1, 1), 'shellescape(v:val)'), ' '),
  \ ' ')
" Emacs expects doubled line numbers
CompilerSet errorformat=%f:%l:%*[0-9]\ %m,%f:-%*[0-9]:-%*[0-9]\ %m

let &cpo = s:cpo_save
unlet s:cpo_save

