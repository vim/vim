" (Varnish|Vinyl) Configuration Language
"
" https://vinyl-cache.org/docs/8.0/reference/vcl.html
" https://vinyl-cache.org/docs/8.0/reference/vmod_std.html
if exists('b:current_syntax') | finish | endif

syn keyword  vclKeyword   vcl import backend sub return if elif elsif elseif else new set unset
syn match    vclKeyword   /\v<include>%(\s+\+glob)?/
syn keyword  vclSpecial   true false now
syn region   vclString    start=/"/ end=/"/ end=/$/
syn region   vclString    start=/{"/ end=/"}/
syn match    vclNumber    /\v<\d+%(\.\d+)?%(ms|s|m|h|d|w|y)?>/
syn match    vclComment   /#.*/
syn match    vclComment   +//.*+
syn region   vclComment   start=+/\*+ end=+\*/+
syn region   vclAclBlock  start=/\<acl\>/ end=/}$/  contains=ALLBUT,vclKeyword
syn keyword  vclAcl       acl nextgroup=vclAclflag
syn match    vclAclflag   /\v\s+[a-zA-Z0-9_-]+\zs%(\s+%(\+log|\+table|-pedantic|-fold))+/
syn match    vclBuiltin   /\v<%(std\.)?%(
                            \ random|round|collect|querysort|toupper|tolower|strstr|fnmatch|fileread
                            \ |blobread|file_exists|healthy|port|duration|bytes|integer|ip|real|time
                            \ |strftime|log|syslog|timestamp|syntax|getenv|cache_req_body
                            \ |late_100_continue|set_ip_tos|rollback|ban|ban_error|now|timed_call
                          \ )\ze\s*\(/

hi def link  vclKeyword  Keyword
hi def link  vclAcl      Keyword
hi def link  vclAclFlag  Keyword
hi def link  vclSpecial  Boolean
hi def link  vclString   String
hi def link  vclNumber   Number
hi def link  vclTime     Number
hi def link  vclComment  Comment
hi def link  vclBuiltin  Function

let b:current_syntax = 'vcl'
