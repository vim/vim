" Vim compiler file
" Compiler:         Rust Compiler
" Maintainer:       Chris Morgan <me@chrismorgan.info>
" Latest Revision:  2023-09-11
" For bugs, patches and license go to https://github.com/rust-lang/rust.vim

if exists("current_compiler")
    finish
endif
let current_compiler = "rustc"

" vint: -ProhibitAbbreviationOption
let s:save_cpo = &cpo
set cpo&vim
" vint: +ProhibitAbbreviationOption

if get(g:, 'rustc_makeprg_no_percent', 0)
    CompilerSet makeprg=rustc
else
    CompilerSet makeprg=rustc\ \%:S
endif

" New errorformat (after nightly 2016/08/10)
CompilerSet errorformat=
            \%-G,
            \%-Gerror:\ aborting\ %.%#,
            \%-Gerror:\ Could\ not\ compile\ %.%#,
            \%Eerror:\ %m,
            \%Eerror[E%n]:\ %m,
            \%Wwarning:\ %m,
            \%Inote:\ %m,
            \%C\ %#-->\ %f:%l:%c,
            \%E\ \ left:%m,%C\ right:%m\ %f:%l:%c,%Z

" Old errorformat (before nightly 2016/08/10)
CompilerSet errorformat+=
            \%f:%l:%c:\ %t%*[^:]:\ %m,
            \%f:%l:%c:\ %*\\d:%*\\d\ %t%*[^:]:\ %m,
            \%-G%f:%l\ %s,
            \%-G%*[\ ]^,
            \%-G%*[\ ]^%*[~],
            \%-G%*[\ ]...

" Compatibility with standard unix tools errorformat
CompilerSet errorformat+=
            \%f:%l:%c:%m

" vint: -ProhibitAbbreviationOption
let &cpo = s:save_cpo
unlet s:save_cpo
" vint: +ProhibitAbbreviationOption

" vim: set et sw=4 sts=4 ts=8:
