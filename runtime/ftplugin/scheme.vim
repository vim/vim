" Vim filetype plugin
" Language:      Scheme
" Maintainer:    Sergey Khorev <sergey.khorev@gmail.com>
" URL:		 http://iamphet.nm.ru/vim
" Original author:    Dorai Sitaram <ds26@gte.com>
" Original URL:		 http://www.ccs.neu.edu/~dorai/vimplugins/vimplugins.html
" Last Change:   Nov 22, 2004

runtime! ftplugin/lisp.vim ftplugin/lisp_*.vim ftplugin/lisp/*.vim

if exists("b:is_mzscheme") || exists("is_mzscheme")
    " improve indenting
    setl iskeyword+=#,%,^
    setl lispwords+=module,parameterize,let-values,let*-values,letrec-values
    setl lispwords+=define-values,opt-lambda,case-lambda,syntax-rules,with-syntax,syntax-case
    setl lispwords+=define-signature,unit,unit/sig,compund-unit/sig,define-values/invoke-unit/sig
endif

if exists("b:is_chicken") || exists("is_chicken")
    " improve indenting
    setl iskeyword+=#,%,^
    setl lispwords+=let-optionals,let-optionals*,declare
    setl lispwords+=let-values,let*-values,letrec-values
    setl lispwords+=define-values,opt-lambda,case-lambda,syntax-rules,with-syntax,syntax-case
    setl lispwords+=cond-expand,and-let*,foreign-lambda,foreign-lambda*
endif
