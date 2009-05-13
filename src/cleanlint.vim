" Vim tool: Filter output of splint
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2009 May 13

" Usage: redirect output of "make lint" to a file, edit that file with Vim and
" :call CleanLint()
" This deletes irrelevant messages.  What remains might be valid warnings.

fun! CleanLint()
  g/Assignment of dev_t to __dev_t:/lockmarks d
  g/Assignment of __dev_t to dev_t:/lockmarks d
  g/Operands of == have incompatible types (__dev_t, dev_t): /lockmarks d
  g/Operands of == have incompatible types (char_u, int): /lockmarks d
  g/Assignment of char to char_u: /lockmarks d
  g/Assignment of unsigned int to int: /lockmarks d
  g/Assignment of int to unsigned int: /lockmarks d
  g/Assignment of unsigned int to long int: /lockmarks d
  g/Assignment of int to char_u: /lockmarks d
  g/Function .* expects arg . to be wint_t gets int: /lockmarks d
  g/Function .* expects arg . to be size_t gets int: /lockmarks d
  g/Initial value of .* is type char, expects char_u: /lockmarks d
  g/^ex_cmds.h:.* Function types are inconsistent. Parameter 1 is implicitly temp, but unqualified in assigned function:/lockmarks d
  g/^ex_docmd.c:.* nospec_str/lockmarks d
  g/^digraph.c.*Additional initialization errors for digraphdefault not reported/lockmarks d
  g/Function strncasecmp expects arg 3 to be int gets size_t: /lockmarks d
  g/^  Types are incompatible/lockmarks d
  g/ To ignore signs in type comparisons use +ignoresigns/lockmarks d
  g/ To allow arbitrary integral types to match any integral type, use +matchanyintegral./lockmarks d
  g/ To allow arbitrary integral types to match long unsigned, use +longintegral./lockmarks d
  g+ A variable is declared but never used. Use /.@unused@./ in front of declaration to suppress message.+lockmarks d
endfun
