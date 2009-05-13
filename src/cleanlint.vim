" Vim tool: Filter output of splint
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2009 May 05

" Usage: redirect output of "make lint" to a file, edit that file with Vim and
" :call CleanLint()
" This deletes irrelevant messages.  What remains might be valid warnings.

fun! CleanLint()
  g/^  Types are incompatible/lockmarks d
  g/Assignment of dev_t to __dev_t:/lockmarks d
  g/Assignment of __dev_t to dev_t:/lockmarks d
  g/Operands of == have incompatible types (__dev_t, dev_t): /lockmarks d
  g/Operands of == have incompatible types (unsigned int, int): /lockmarks d
  g/Assignment of char to char_u: /lockmarks d
  g/Assignment of unsigned int to int: /lockmarks d
  g/Assignment of colnr_T to int: /lockmarks d
  g/Assignment of int to char_u: /lockmarks d
  g/Function .* expects arg . to be wint_t gets int: /lockmarks d
  g/^digraph.c.*digraphdefault.*is type char, expects char_u:/lockmarks d
  g/^digraph.c.*Additional initialization errors for digraphdefault not reported/lockmarks d
  g/Function strncasecmp expects arg 3 to be int gets size_t: /lockmarks d
  g/ To ignore signs in type comparisons use +ignoresigns/lockmarks d
  g/ To allow arbitrary integral types to match any integral type, use +matchanyintegral./lockmarks d
  g/ To allow arbitrary integral types to match long unsigned, use +longintegral./lockmarks d
endfun
