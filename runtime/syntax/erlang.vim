" Vim syntax file
" Language:    erlang (ERicsson LANGuage)
"              http://www.erlang.se
"              http://www.erlang.org
" Maintainer:  Csaba Hoch <csaba.hoch@gmail.com>
" Former Maintainer:  Kreąimir Marľić (Kresimir Marzic) <kmarzic@fly.srk.fer.hr>
" Last update: 12-Mar-2008
" Filenames:   .erl


" There are three sets of highlighting in here:
" One is "erlang_characters", second is "erlang_functions" and third
" is "erlang_keywords".
" If you want to disable keywords highlighting, put in your .vimrc:
"       let erlang_keywords=1
" If you want to disable erlang BIF highlighting, put in your .vimrc
" this:
"       let erlang_functions=1
" If you want to disable special characters highlighting, put in
" your .vimrc:
"       let erlang_characters=1


" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
    syntax clear
elseif exists ("b:current_syntax")
    finish
endif


" Case sensitive
syn case match


if ! exists ("erlang_characters")

    " Basic elements
    syn match   erlangComment          "%.*$" contains=erlangAnnotation,erlangTodo
    syn match   erlangAnnotation       " \@<=@\%(clear\|docfile\|end\|headerfile\|todo\|TODO\|type\|author\|copyright\|doc\|reference\|see\|since\|title\|version\|deprecated\|hidden\|private\|equiv\|spec\|throws\)" contained
    syn match   erlangAnnotation       "`[^']*'" contained
    syn keyword erlangTodo             TODO FIXME XXX contained
    syn match   erlangModifier         "\~\a\|\\\a\|\\\\" contained
    syn match   erlangSpecialCharacter ":\|_\|@\|\\\|\"\|\."
    syn match   erlangSeparator        "(\|)\|{\|}\|\[\|]\||\|||\|;\|,\|?\|->\|#" contained
    syn region  erlangString           start=+"+ skip=+\\.+ end=+"+ contains=erlangModifier
    syn region  erlangAtom             start=+'+ skip=+\\'+ end=+'+

    " Operators
    syn match   erlangOperator         "+\|-\|\*\|\/"
    syn keyword erlangOperator         div rem or xor bor bxor bsl bsr
    syn keyword erlangOperator         and band not bnot
    syn match   erlangOperator         "==\|/=\|=:=\|=/=\|<\|=<\|>\|>="
    syn match   erlangOperator         "++\|--\|=\|!\|<-"

    " Numbers
    syn match   erlangNumberInteger    "\d\+" contains=erlangSeparator
    syn match   erlangNumberFloat1     "\d\+\.\d\+" contains=erlangSeparator
    syn match   erlangNumberFloat2     "\d\+\(\.\d\+\)\=[eE][+-]\=\d\+\(\.\d\+\)\=" contains=erlangSeparator
    syn match   erlangNumberFloat3     "\d\+[#]\x\+" contains=erlangSeparator
    syn match   erlangNumberHex        "$\x\+" contains=erlangSeparator

    " Ignore '_' and '-' in words
    syn match   erlangWord             "\h\+\w*"

    syn match   erlangChar             /\$./
endif

if ! exists ("erlang_functions")
    " Functions call
    syn match   erlangFCall      "\%(\w\+\s*\.\s*\)*\w\+\s*[:@]\s*\w\+"

    " build-in-functions (BIFs)
    syn keyword erlangBIF        abs alive apply atom_to_list
    syn keyword erlangBIF        binary_to_list binary_to_term
    syn keyword erlangBIF        concat_binary
    syn keyword erlangBIF        date disconnect_node
    syn keyword erlangBIF        element erase exit
    syn keyword erlangBIF        float float_to_list
    syn keyword erlangBIF        get get_keys group_leader
    syn keyword erlangBIF        halt hd
    syn keyword erlangBIF        integer_to_list is_alive
    syn keyword erlangBIF        length link list_to_atom list_to_binary
    syn keyword erlangBIF        list_to_float list_to_integer list_to_pid
    syn keyword erlangBIF        list_to_tuple load_module
    syn keyword erlangBIF        make_ref monitor_node
    syn keyword erlangBIF        node nodes now
    syn keyword erlangBIF        open_port
    syn keyword erlangBIF        pid_to_list process_flag
    syn keyword erlangBIF        process_info process put
    syn keyword erlangBIF        register registered round
    syn keyword erlangBIF        self setelement size spawn
    syn keyword erlangBIF        spawn_link split_binary statistics
    syn keyword erlangBIF        term_to_binary throw time tl trunc
    syn keyword erlangBIF        tuple_to_list
    syn keyword erlangBIF        unlink unregister
    syn keyword erlangBIF        whereis

    " Other BIFs
    syn keyword erlangBIF        atom binary constant function integer
    syn keyword erlangBIF        list number pid ports port_close port_info
    syn keyword erlangBIF        reference record

    " erlang:BIFs
    syn keyword erlangBIF        check_process_code delete_module
    syn keyword erlangBIF        get_cookie hash math module_loaded
    syn keyword erlangBIF        preloaded processes purge_module set_cookie
    syn keyword erlangBIF        set_node

    " functions of math library
    syn keyword erlangFunction   acos asin atan atan2 cos cosh exp
    syn keyword erlangFunction   log log10 pi pow power sin sinh sqrt
    syn keyword erlangFunction   tan tanh

    " Other functions
    syn keyword erlangFunction   call module_info parse_transform
    syn keyword erlangFunction   undefined_function

    " Modules
    syn keyword erlangModule     error_handler
endif

if ! exists ("erlang_keywords")
    " Constants and Directives
    syn match   erlangDirective  "-behaviour\|-behaviour"
    syn match   erlangDirective  "-compile\|-define\|-else\|-endif\|-export\|-file"
    syn match   erlangDirective  "-ifdef\|-ifndef\|-import\|-include_lib\|-include"
    syn match   erlangDirective  "-module\|-record\|-undef"

    syn match   erlangConstant   "-author\|-copyright\|-doc\|-vsn"

    " Keywords
    syn keyword erlangKeyword    after begin case catch
    syn keyword erlangKeyword    cond end fun if
    syn keyword erlangKeyword    let of query receive
    syn keyword erlangKeyword    when
    syn keyword erlangKeyword    try

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
    syn keyword erlangExtra      badarg nocookie false fun true

    " Signals
    syn keyword erlangSignal     badsig kill killed exit normal
endif



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
    HiLink erlangAnnotation Special
    HiLink erlangTodo Todo
    HiLink erlangSpecialCharacter Special
    HiLink erlangSeparator Normal
    HiLink erlangModifier Special
    HiLink erlangOperator Operator
    HiLink erlangString String
    HiLink erlangAtom Type

    HiLink erlangNumberInteger Number
    HiLink erlangNumberFloat1 Float
    HiLink erlangNumberFloat2 Float
    HiLink erlangNumberFloat3 Float
    HiLink erlangNumberFloat4 Float
    HiLink erlangNumberHex Number

    HiLink erlangWord Normal

    " erlang_functions
    HiLink erlangFCall Function
    HiLink erlangBIF Function
    HiLink erlangFunction Function
    HiLink erlangModuleFunction Function

    " erlang_keywords
    HiLink erlangDirective Type
    HiLink erlangConstant Type
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

