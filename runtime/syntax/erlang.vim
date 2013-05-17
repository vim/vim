" Vim syntax file
" Language:     Erlang (http://www.erlang.org)
" Maintainer:   Csaba Hoch <csaba.hoch@gmail.com>
" Former Maintainer:  Kreąimir Marľić (Kresimir Marzic) <kmarzic@fly.srk.fer.hr>
" Last Update:  2013-Mar-07
" License:      Vim license
" URL:          https://github.com/hcs42/vim-erlang

" Customization:
"
" There are two optional sets of highlighting:
"
" 1. The BIFs (built-in functions) are highlighted by default. To disable
"    this, put the following line in your vimrc:
"
"      let g:erlang_highlight_bifs = 0
"
" 2. To enable highlighting some special atoms, put this in your vimrc:
"
"      let g:erlang_highlight_special_atoms = 1

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
    syntax clear
elseif exists("b:current_syntax")
    finish
endif

" Case sensitive
syn case match

if version >= 600
  setlocal iskeyword+=$,@-@
endif

" Comments
syn match erlangComment             '%.*$' contains=erlangCommentAnnotation,erlangTodo
syn match erlangCommentAnnotation   ' \@<=@\%(clear\|docfile\|end\|headerfile\|todo\|TODO\|type\|author\|copyright\|doc\|reference\|see\|since\|title\|version\|deprecated\|hidden\|private\|equiv\|spec\|throws\)' contained
syn match erlangCommentAnnotation   /`[^']*'/ contained
syn keyword erlangTodo              TODO FIXME XXX contained

" Numbers (minimum base is 2, maximum is 36.)
syn match   erlangNumberInteger     '\<\d\+\>'
syn match   erlangNumberInteger     '\<\%([2-9]\|[12]\d\|3[0-6]\)\+#[[:alnum:]]\+\>'
syn match   erlangNumberFloat       '\<\d\+\.\d\+\%([eE][+-]\=\d\+\)\=\>'

" Strings, atoms, characters
syn region  erlangString            start=/"/ end=/"/ contains=erlangStringModifier
syn region  erlangQuotedAtom        start=/'/ end=/'/ contains=erlangQuotedAtomModifier
syn match   erlangStringModifier     '\~\a\|\\\%(\o\{1,3}\|x\x\x\|x{\x\+}\|\^.\|.\)' contained
syn match   erlangQuotedAtomModifier '\~\a\|\\\%(\o\{1,3}\|x\x\x\|x{\x\+}\|\^.\|.\)' contained
syn match   erlangModifier           '\$\%([^\\]\|\\\%(\o\{1,3}\|x\x\x\|x{\x\+}\|\^.\|.\)\)'

" Operators
syn match   erlangOperator          '==\|=:=\|/=\|=/=\|<\|=<\|>\|>=\|++\|--\|=\|!\|<-\|+\|-\|\*\|\/'
syn keyword erlangOperator          div rem or xor bor bxor bsl bsr and band not bnot andalso orelse

" Separators
syn match erlangSeparator           '(\|)\|{\|}\|\[\|]\||\|||\|;\|,\|?\|#'
syn match erlangRightArrow          '->'

" Functions call
syn match   erlangFCall             '\<\%(\a[[:alnum:]@]*\s*\.\s*\)*\a[[:alnum:]@]*\s*:\s*\a[[:alnum:]@]*\>'

" Constants and Directives
syn match   erlangDirective         '-\%(behaviour\|behavior\|compile\|define\|else\|endif\|export\|file\|ifdef\|ifndef\|import\|include_lib\|include\|module\|record\|undef\|author\|copyright\|doc\|vsn\|on_load\|export_type\)\>'

" Keywords
syn keyword erlangKeyword           after begin case catch cond end fun if let of query receive when try
syn keyword erlangExtra             true false


if !exists("g:erlang_highlight_bifs") || g:erlang_highlight_bifs == 1

  " build-in-functions (BIFs)
  syn keyword erlangBIF        abs alive apply atom_to_binary atom_to_list binary_part binary_to_atom binary_to_existing_atom binary_to_float binary_to_integer bitstring_to_list binary_to_list binary_to_term bit_size byte_size check_old_code check_process_code concat_binary date delete_module demonitor disconnect_node element erase error exit float float_to_binary float_to_list garbage_collect get get_keys group_leader halt hd integer_to_binary integer_to_list iolist_to_binary iolist_size is_alive is_atom is_binary is_bitstring is_boolean is_float is_function is_integer is_list is_number is_pid is_port is_process_alive is_record is_reference is_tuple length link list_to_atom list_to_binary list_to_bitstring list_to_existing_atom list_to_float list_to_integer list_to_pid list_to_tuple load_module make_ref max min module_loaded monitor monitor_node node nodes now open_port pid_to_list port_close port_command port_connect pre_loaded process_flag process_flag process_info process purge_module put register registered round self setelement size spawn spawn_link spawn_monitor spawn_opt split_binary statistics term_to_binary throw time tl trunc tuple_size tuple_to_list unlink unregister whereis

endif


if exists("g:erlang_highlight_special_atoms") && g:erlang_highlight_special_atoms == 1

  " Processes
  syn keyword erlangProcess    creation current_function dictionary
  syn keyword erlangProcess    group_leader heap_size high initial_call
  syn keyword erlangProcess    linked low memory_in_use message_queue
  syn keyword erlangProcess    net_kernel node normal priority
  syn keyword erlangProcess    reductions registered_name runnable
  syn keyword erlangProcess    running stack_trace status timer
  syn keyword erlangProcess    trap_exit waiting

  " Ports
  syn keyword erlangPort       command count_in count_out creation in
  syn keyword erlangPort       in_format linked node out owner packeting

  " Nodes
  syn keyword erlangNode       atom_tables communicating creation
  syn keyword erlangNode       current_gc current_reductions current_runtime
  syn keyword erlangNode       current_wall_clock distribution_port
  syn keyword erlangNode       entry_points error_handler friends
  syn keyword erlangNode       garbage_collection magic_cookie magic_cookies
  syn keyword erlangNode       module_table monitored_nodes name next_ref
  syn keyword erlangNode       ports preloaded processes reductions
  syn keyword erlangNode       ref_state registry runtime wall_clock

  " Reserved
  syn keyword erlangReserved   apply_lambda module_info module_lambdas
  syn keyword erlangReserved   record record_index record_info

  " Extras
  syn keyword erlangExtra      badarg nocookie

  " Signals
  syn keyword erlangSignal     badsig kill killed exit normal
endif

" Sync at the beginning of functions: if this is not used, multiline string
" are not always recognized
syn sync match erlangSync grouphere NONE "^[a-z]\s*("

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists ("did_erlang_inits")
  if version < 508
    let did_erlang_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " erlang_characters
  HiLink erlangComment Comment
  HiLink erlangCommentAnnotation Special
  HiLink erlangTodo Todo
  HiLink erlangSeparator Normal
  HiLink erlangOperator Operator
  HiLink erlangRightArrow Operator

  HiLink erlangStartString String
  HiLink erlangString String
  HiLink erlangStringModifier Special

  HiLink erlangStartQuotedAtom Type
  HiLink erlangQuotedAtom Type
  HiLink erlangQuotedAtomModifier Special

  HiLink erlangNumberInteger Number
  HiLink erlangNumberFloat Float
  HiLink erlangNumberHex Number

  HiLink erlangModifier Special

  " erlang_functions
  HiLink erlangFCall Function
  HiLink erlangBIF Function

  " erlang_keywords
  HiLink erlangDirective Type
  HiLink erlangKeyword Keyword
  HiLink erlangProcess Special
  HiLink erlangPort Special
  HiLink erlangNode Special
  HiLink erlangReserved Statement
  HiLink erlangExtra Statement
  HiLink erlangSignal Statement

  delcommand HiLink
endif


let b:current_syntax = "erlang"

" vim: sw=2 et
