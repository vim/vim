
" Known mappings
let s:mediawiki_wikilang_to_vim = {
            \ 'abap': 'abap',
            \ 'ada': 'ada',
            \ 'apache': 'apache',
            \ 'apt_sources': 'debsources',
            \ 'asm': 'asm',
            \ 'autohotkey': 'autohotkey',
            \ 'autoit': 'autoit',
            \ 'awk': 'awk',
            \ 'bash': 'sh',
            \ 'bibtex': 'bib',
            \ 'c': 'c',
            \ 'chaiscript': 'chaiscript',
            \ 'clojure': 'clojure',
            \ 'cmake': 'cmake',
            \ 'cobol': 'cobol',
            \ 'cpp': 'cpp',
            \ 'csharp': 'cs',
            \ 'css': 'css',
            \ 'd': 'd',
            \ 'dcl': 'dcl',
            \ 'diff': 'diff',
            \ 'dot': 'dot',
            \ 'eiffel': 'eiffel',
            \ 'email': 'mail',
            \ 'erlang': 'erlang',
            \ 'falcon': 'falcon',
            \ 'freebasic': 'freebasic',
            \ 'gdb': 'gdb',
            \ 'gnuplot': 'gnuplot',
            \ 'groovy': 'groovy',
            \ 'haskell': 'haskell',
            \ 'html4strict': 'html',
            \ 'html5': 'html',
            \ 'icon': 'icon',
            \ 'idl': 'idl',
            \ 'ini': 'dosini',
            \ 'j': 'j',
            \ 'java': 'java',
            \ 'java5': 'java',
            \ 'javascript': 'javascript',
            \ 'kixtart': 'kix',
            \ 'latex': 'tex',
            \ 'lisp': 'lisp',
            \ 'logtalk': 'logtalk',
            \ 'lscript': 'lscript',
            \ 'lua': 'lua',
            \ 'make': 'make',
            \ 'matlab': 'matlab',
            \ 'mmix': 'mmix',
            \ 'modula2': 'modula2',
            \ 'modula3': 'modula3',
            \ 'mysql': 'mysql',
            \ 'nsis': 'nsis',
            \ 'objc': 'objc',
            \ 'ocaml': 'ocaml',
            \ 'oracle11': 'sql',
            \ 'oracle8': 'sql',
            \ 'pascal': 'pascal',
            \ 'perl': 'perl',
            \ 'perl6': 'perl6',
            \ 'pf': 'pf',
            \ 'php': 'php',
            \ 'pic16': 'pic',
            \ 'pike': 'pike',
            \ 'pli': 'pli',
            \ 'plsql': 'plsql',
            \ 'povray': 'pov',
            \ 'progress': 'progress',
            \ 'prolog': 'prolog',
            \ 'python': 'python',
            \ 'rebol': 'rebol',
            \ 'rexx': 'rexx',
            \ 'robots': 'robots',
            \ 'ruby': 'ruby',
            \ 'sas': 'sas',
            \ 'scheme': 'scheme',
            \ 'scilab': 'scilab',
            \ 'smalltalk': 'st',
            \ 'sql': 'sql',
            \ 'tcl': 'tcl',
            \ 'vb': 'vb',
            \ 'verilog': 'verilog',
            \ 'vhdl': 'vhdl',
            \ 'vim': 'vim',
            \ 'whitespace': 'whitespace',
            \ 'winbatch': 'winbatch',
            \ 'yaml': 'yaml'
            \ }

function! mediawiki#FindLanguagesInBuffer()
    let save_cursor = getpos('.')
    let languagesDict = {}
    call cursor('$', 1)
    let flags = 'w'
    while search('<\(source\|syntaxhighlight\)\s\+lang="', flags) > 0
        " Assumes there is only one match per line
        let lang = substitute(getline('.'), '.*<\(source\|syntaxhighlight\)\s\+lang="\(\w\+\)".*', '\2', '')
        let languagesDict[lang] = 1
        " Do not wrap search anymore
        let flags = 'W'
    endwhile
    call setpos('.', save_cursor)
    return keys(languagesDict)
endfunction

" Include the syntax file for the given filetype
function! mediawiki#IncludeSyntax(filetype, groupName)
    " Most syntax files no nothing if b:current_syntax is defined.
    " Make sure to unset it.
    if exists('b:current_syntax')
        let b:saved_current_syntax = b:current_syntax
        unlet b:current_syntax
    endif

    exe 'syntax include @' . a:groupName . ' syntax/' . a:filetype . '.vim'

    " Restore b:current_syntax
    if exists('b:saved_current_syntax')
        let b:current_syntax = b:saved_current_syntax
        unlet b:saved_current_syntax
    elseif exists('b:current_syntax')
        unlet b:current_syntax
    endif
endfunction

" Define the highlighted region. Must be called after IncludeSyntax()
function! mediawiki#DefineRegion(filetype, groupName, wikiLang)
    " <source> tag
    exe 'syntax region wiki_' . a:filetype . '_region ' .
                \ "start='<source lang=\"" . a:wikiLang . "\">' " .
                \ "end='</source>' ".
                \ "keepend contains=wikiSourceTag,wikiSourceEndTag,@" . a:groupName
    " <syntaxhighlight> tag
    exe 'syntax region wiki_' . a:filetype . '_region ' .
                \ "start='<syntaxhighlight lang=\"" . a:wikiLang . "\">' " .
                \ "end='</syntaxhighlight>' ".
                \ "keepend contains=wikiSyntaxHLTag,wikiSyntaxHLEndTag,@" . a:groupName
endfunction

" Perform highlighting for a given wiki language
function! mediawiki#HighlightWikiLang(wikiLang, filetype, alreadyIncludedFt)
    let groupName = a:filetype . '_group'

    " Include syntax file, if not yet included
    if !has_key(a:alreadyIncludedFt, a:filetype)
        call mediawiki#IncludeSyntax(a:filetype, groupName)
        let a:alreadyIncludedFt[a:filetype] = 1
    endif

    call mediawiki#DefineRegion(a:filetype, groupName, a:wikiLang)
endfunction

" Perform highlighting
function! mediawiki#PerformHighlighting()
    " Apply user overrides
    call extend(s:mediawiki_wikilang_to_vim, g:mediawiki_wikilang_to_vim_overrides)

    " Convert list into a dict
    let ignoredDict = {}
    for wikiLang in g:mediawiki_ignored_wikilang
        let ignoredDict[wikiLang] = 1
    endfor

    let alreadyIncludedFt = {}
    " Load languages
    for wikiLang in mediawiki#FindLanguagesInBuffer() + g:mediawiki_forced_wikilang
        if has_key(ignoredDict, wikiLang)
            continue
        endif

        " Get corresponding filetype
        if !has_key(s:mediawiki_wikilang_to_vim, wikiLang)
            let msg = 'Warning: no filetype mapped to wiki language "' . wikiLang . '"'
            echohl WarningMsg | echom msg | echohl None
            continue
        endif
        let ft = s:mediawiki_wikilang_to_vim[wikiLang]

        call mediawiki#HighlightWikiLang(wikiLang, ft, alreadyIncludedFt)
    endfor
endfunction

