" This ftplugin provides syntax highlighting for blocks of code surrounded
" with "source" or "syntaxhighlight" tags. These tags are used by the GeSHi
" mediawiki extension:
" https://www.mediawiki.org/wiki/Extension:SyntaxHighlight_GeSHi
"
" By default, only languages present in the current buffer are highlighted.
" You can customize this behavior by defining a few variables in your .vimrc:
"
"   g:loaded_mediawiki_filetype_highlighting
"       If set, highlighting in the "source" and "syntaxhighlight" tags will be
"       disabled.
"       Example:
"           let g:loaded_mediawiki_filetype_highlighting = 1
"       Default: unset
"
"   g:mediawiki_forced_wikilang
"       List of GeSHi languages for which highlighting should always be
"       loaded, even if there is no corresponding tag in the current buffer.
"       Example:
"           let g:mediawiki_forced_wikilang = ['sql', 'java', 'java5']
"       Default: []
"       Note: Forcing many languages can slow down the opening of mediawiki
"       files. If you often use various languages, it may be better to keep
"       the default values, and reload the buffer from time to time with :e
"
"   g:mediawiki_ignored_wikilang
"       List of GeSHi languages for which no syntax highlighting is desired.
"       If a language is both forced and ignored, it will be ignored.
"       Example:
"           let g:mediawiki_ignored_wikilang = ['html4strict', 'html5']
"       Default: []
"
"   g:mediawiki_wikilang_to_vim_overrides
"       Dictionary allowing to overrides the default language mappings.
"       The key of the dictionary is a GeSHi language, and the value is a Vim
"       filetype.
"       Example:
"           let g:mediawiki_wikilang_to_vim_overrides = {
"                   \ 'bash': 'zsh',
"                   \ 'new_geshi_language': 'foobar',
"                   \ }
"       Default: {}

if exists('g:loaded_mediawiki_filetype_highlighting')
    finish
endif
let g:loaded_mediawiki_filetype_highlighting = 1

" Set default values
if !exists('g:mediawiki_ignored_wikilang')
    let g:mediawiki_ignored_wikilang = []
endif
if !exists('g:mediawiki_forced_wikilang')
    let g:mediawiki_forced_wikilang = []
endif
if !exists('g:mediawiki_wikilang_to_vim_overrides')
    let g:mediawiki_wikilang_to_vim_overrides = {}
endif

augroup MediaWiki
    autocmd!
    autocmd Syntax mediawiki call mediawiki#PerformHighlighting()
augroup END

