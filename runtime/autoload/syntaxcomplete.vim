" Vim completion script
" Language:    All languages, uses existing syntax highlighting rules
" Maintainer:  David Fishburn <fishburn@ianywhere.com>
" Version:     2.0
" Last Change: Fri May 05 2006 10:34:57 PM
" Usage:       For detailed help, ":help ft-syntax-omni" 

" Set completion with CTRL-X CTRL-O to autoloaded function.
" This check is in place in case this script is
" sourced directly instead of using the autoload feature. 
if exists('+omnifunc')
    " Do not set the option if already set since this
    " results in an E117 warning.
    if &omnifunc == ""
        setlocal omnifunc=syntaxcomplete#Complete
    endif
endif

if exists('g:loaded_syntax_completion')
    finish 
endif
let g:loaded_syntax_completion = 20

" Set ignorecase to the ftplugin standard
if !exists('g:omni_syntax_ignorecase')
    let g:omni_syntax_ignorecase = &ignorecase
endif

" This script will build a completion list based on the syntax
" elements defined by the files in $VIMRUNTIME/syntax.
let s:syn_remove_words = 'match,matchgroup=,contains,'.
            \ 'links to,start=,end=,nextgroup='

let s:cache_name = []
let s:cache_list = []
let s:prepended  = ''

" This function is used for the 'omnifunc' option.
function! syntaxcomplete#Complete(findstart, base)

    if a:findstart
        " Locate the start of the item, including "."
        let line = getline('.')
        let start = col('.') - 1
        let lastword = -1
        while start > 0
            if line[start - 1] =~ '\w'
                let start -= 1
            elseif line[start - 1] =~ '\.'
                " The user must be specifying a column name
                if lastword == -1
                    let lastword = start
                endif
                let start -= 1
                let b:sql_compl_type = 'column'
            else
                break
            endif
        endwhile

        " Return the column of the last word, which is going to be changed.
        " Remember the text that comes before it in s:prepended.
        if lastword == -1
            let s:prepended = ''
            return start
        endif
        let s:prepended = strpart(line, start, lastword - start)
        return lastword
    endif

    let base = s:prepended . a:base

    let filetype = substitute(&filetype, '\.', '_', 'g')
    let list_idx = index(s:cache_name, filetype, 0, &ignorecase)
    if list_idx > -1
        let compl_list = s:cache_list[list_idx]
    else
        let compl_list   = OmniSyntaxList()
        let s:cache_name = add( s:cache_name,  filetype )
        let s:cache_list = add( s:cache_list,  compl_list )
    endif

    " Return list of matches.

    if base =~ '\w'
        let compstr    = join(compl_list, ' ')
        let expr       = (g:omni_syntax_ignorecase==0?'\C':'').'\<\%('.base.'\)\@!\w\+\s*'
        let compstr    = substitute(compstr, expr, '', 'g')
        let compl_list = split(compstr, '\s\+')
    endif

    return compl_list
endfunc

function! OmniSyntaxList()
    " Default to returning a dictionary, if use_dictionary is set to 0
    " a list will be returned.
    " let use_dictionary = 1
    " if a:0 > 0 && a:1 != ''
    "     let use_dictionary = a:1
    " endif

    let saveL = @l
    
    " Loop through all the syntax groupnames, and build a
    " syntax file which contains these names.  This can 
    " work generically for any filetype that does not already
    " have a plugin defined.
    " This ASSUMES the syntax groupname BEGINS with the name
    " of the filetype.  From my casual viewing of the vim7\syntax 
    " directory.
    redir @l
    silent! exec 'syntax list '
    redir END

    let syntax_full = "\n".@l
    let @l = saveL

    if syntax_full =~ 'E28' 
                \ || syntax_full =~ 'E411'
                \ || syntax_full =~ 'E415'
                \ || syntax_full =~ 'No Syntax items'
        return []
    endif

    let filetype = substitute(&filetype, '\.', '_', 'g')

    " Default the include group to include the requested syntax group
    let syntax_group_include_{filetype} = ''
    " Check if there are any overrides specified for this filetype
    if exists('g:omni_syntax_group_include_'.filetype)
        let syntax_group_include_{filetype} =
                    \ substitute( g:omni_syntax_group_include_{filetype},'\s\+','','g') 
        if syntax_group_include_{filetype} =~ '\w'
            let syntax_group_include_{filetype} = 
                        \ substitute( syntax_group_include_{filetype}, 
                        \ '\s*,\s*', '\\|', 'g'
                        \ )
        endif
    endif

    " Default the exclude group to nothing
    let syntax_group_exclude_{filetype} = ''
    " Check if there are any overrides specified for this filetype
    if exists('g:omni_syntax_group_exclude_'.filetype)
        let syntax_group_exclude_{filetype} =
                    \ substitute( g:omni_syntax_group_exclude_{filetype},'\s\+','','g') 
        if syntax_group_exclude_{filetype} =~ '\w' 
            let syntax_group_exclude_{filetype} = 
                        \ substitute( syntax_group_exclude_{filetype}, 
                        \ '\s*,\s*', '\\|', 'g'
                        \ )
        endif
    endif

    " Sometimes filetypes can be composite names, like c.doxygen
    " Loop through each individual part looking for the syntax
    " items specific to each individual filetype.
    let syn_list = ''
    let ftindex  = 0
    let ftindex  = match(&filetype, '\w\+', ftindex)

    while ftindex > -1
        let ft_part_name = matchstr( &filetype, '\w\+', ftindex )

        " Syntax rules can contain items for more than just the current 
        " filetype.  They can contain additional items added by the user
        " via autocmds or their vimrc.
        " Some syntax files can be combined (html, php, jsp).
        " We want only items that begin with the filetype we are interested in.
        let next_group_regex = '\n' .
                    \ '\zs'.ft_part_name.'\w\+\ze'.
                    \ '\s\+xxx\s\+' 
        let index    = 0
        let index    = match(syntax_full, next_group_regex, index)

        while index > -1
            let group_name = matchstr( syntax_full, '\w\+', index )

            let get_syn_list = 1
            " if syntax_group_include_{&filetype} == ''
            "     if syntax_group_exclude_{&filetype} != ''
            "         if '\<'.syntax_group_exclude_{&filetype}.'\>' =~ '\<'.group_name.'\>'
            "             let get_syn_list = 0
            "         endif
            "     endif
            " else
            "     if '\<'.syntax_group_include_{&filetype}.'\>' !~ '\<'.group_name.'\>'
            "         let get_syn_list = 0
            "     endif
            " endif
            if syntax_group_exclude_{filetype} != ''
                if '\<'.syntax_group_exclude_{filetype}.'\>' =~ '\<'.group_name.'\>'
                    let get_syn_list = 0
                endif
            endif
        
            if get_syn_list == 1
                if syntax_group_include_{filetype} != ''
                    if '\<'.syntax_group_include_{filetype}.'\>' !~ '\<'.group_name.'\>'
                        let get_syn_list = 0
                    endif
                endif
            endif

            if get_syn_list == 1
                " Pass in the full syntax listing, plus the group name we 
                " are interested in.
                let extra_syn_list = s:SyntaxCSyntaxGroupItems(group_name, syntax_full)

                " if !empty(extra_syn_list)
                "     for elem in extra_syn_list
                "         let item = {'word':elem, 'kind':'t', 'info':group_name}
                "         let compl_list += [item]
                "     endfor
                " endif

                let syn_list = syn_list . extra_syn_list . "\n"
            endif

            let index = index + strlen(group_name)
            let index = match(syntax_full, next_group_regex, index)
        endwhile

        let ftindex  = ftindex + len(ft_part_name)
        let ftindex  = match( &filetype, '\w\+', ftindex )
    endwhile

    " Convert the string to a List and sort it.
    let compl_list = sort(split(syn_list))

    if &filetype == 'vim'
        let short_compl_list = []
        for i in range(len(compl_list))
            if i == len(compl_list)-1
                let next = i
            else
                let next = i + 1
            endif
            if  compl_list[next] !~ '^'.compl_list[i].'.$'
                let short_compl_list += [compl_list[i]]
            endif
        endfor

        return short_compl_list
    else
        return compl_list
    endif
endfunction

function! s:SyntaxCSyntaxGroupItems( group_name, syntax_full )

    let syn_list = ""

    " From the full syntax listing, strip out the portion for the
    " request group.
    " Query:
    "     \n           - must begin with a newline
    "     a:group_name - the group name we are interested in
    "     \s\+xxx\s\+  - group names are always followed by xxx
    "     \zs          - start the match
    "     .\{-}        - everything ...
    "     \ze          - end the match
    "     \n\w         - at the first newline starting with a character
    let syntax_group = matchstr(a:syntax_full, 
                \ "\n".a:group_name.'\s\+xxx\s\+\zs.\{-}\ze'."\n".'\w'
                \ )

    if syntax_group != ""
        " let syn_list = substitute( @l, '^.*xxx\s*\%(contained\s*\)\?', "", '' )
        " let syn_list = substitute( @l, '^.*xxx\s*', "", '' )

        " We only want the words for the lines begining with
        " containedin, but there could be other items.
        
        " Tried to remove all lines that do not begin with contained
        " but this does not work in all cases since you can have
        "    contained nextgroup=...
        " So this will strip off the ending of lines with known
        " keywords.
        let syn_list = substitute( 
                    \    syntax_group, '\<\('.
                    \    substitute(
                    \      escape(s:syn_remove_words, '\\/.*$^~[]')
                    \      , ',', '\\|', 'g'
                    \    ).
                    \    '\).\{-}\%($\|'."\n".'\)'
                    \    , "\n", 'g' 
                    \  )

        " Now strip off the newline + blank space + contained
        let syn_list = substitute( 
                    \    syn_list, '\%(^\|\n\)\@<=\s*\<\(contained\)'
                    \    , "", 'g' 
                    \ )

        " There are a number of items which have non-word characters in
        " them, *'T_F1'*.  vim.vim is one such file.
        " This will replace non-word characters with spaces.
        let syn_list = substitute( syn_list, '[^0-9A-Za-z_ ]', ' ', 'g' )
    else
        let syn_list = ''
    endif

    return syn_list
endfunction

