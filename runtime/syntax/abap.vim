" Vim ABAP syntax file
"    Language: SAP - ABAP/R4
"    Revision: 1.0
"  Maintainer: Marius Piedallu van Wyk <marius@e.co.za>
" Last Change: 2006 Apr 13

" For version  < 6.0: Clear all syntax items
" For version >= 6.0: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Always ignore case
syn case ignore

" Symbol Operators
syn match   abapSymbolOperator  "[+\-/=<>$]"
syn match   abapSymbolOperator  "\*"
syn match   abapSymbolOperator  "[<>]="
syn match   abapSymbolOperator  "<>"
syn match   abapSymbolOperator  "\*\*"
syn match   abapSymbolOperator  "[()]"
syn match   abapSymbolOperator  "[:,\.]"

" Literals
syn region  abapString matchgroup=abapString start="'" end="'" contains=abapStringEscape
syn match   abapStringEscape contained "''"

syn match   abapNumber  "-\=\<\d\+\>"
syn region  abapHex     matchgroup=abapHex start="X'" end="'"

if version >= 600
  setlocal iskeyword=-,48-57,_,A-Z,a-z
else
  set iskeyword=-,48-57,_,A-Z,a-z
endif

" ABAP statements
syn keyword abapStatement ADD ADD-CORRESPONDING ASSIGN AT AUTHORITY-CHECK
syn keyword abapStatement BACK BREAK-POINT
syn keyword abapStatement CALL CASE CHECK CLEAR CLOSE CNT COLLECT COMMIT COMMUNICATION COMPUTE CONCATENATE CONDENSE CONSTANTS CONTINUE CONTROLS CONVERT CREATE CURRENCY
syn keyword abapStatement DATA DEFINE DELETE DESCRIBE DETAIL DIVIDE DIVIDE-CORRESPONDING DO
syn keyword abapStatement EDITOR-CALL ELSE ELSEIF END-OF-DEFINITION END-OF-PAGE END-OF-SELECTION ENDAT ENDCASE ENDDO ENDEXEC ENDFORM ENDFUNCTION ENDIF ENDIFEND ENDLOOP ENDMODULE ENDON ENDPROVIDE ENDSELECT ENDWHILE EXEC EXPORT EXPORTING EXTRACT
syn keyword abapStatement FETCH FIELD-GROUPS FIELD-SYMBOLS FIELDS FORM FORMAT FREE FUNCTION FUNCTION-POOL
syn keyword abapStatement GENERATE GET
syn keyword abapStatement HIDE
syn keyword abapStatement IF IMPORT IMPORTING INDEX INFOTYPES INITIALIZATION INPUT INSERT
syn keyword abapStatement LEAVE LIKE LOAD LOCAL LOOP
syn keyword abapStatement MESSAGE MODIFY MODULE MOVE MOVE-CORRESPONDING MULTIPLY MULTIPLY-CORRESPONDING
syn keyword abapStatement NEW-LINE NEW-PAGE NEW-SECTION
syn keyword abapStatement ON OVERLAY
syn keyword abapStatement PACK PARAMETERS PERFORM POSITION PRINT-CONTROL PROGRAM PROVIDE PUT
syn keyword abapStatement RAISE RANGES READ RECEIVE REFRESH REJECT REPLACE REPORT RESERVE RESTORE ROLLBACK RP-PROVIDE-FROM-LAST
syn keyword abapStatement SCAN SCROLL SEARCH SELECT SELECT-OPTIONS SELECTION-SCREEN SET SHIFT SKIP SORT SPLIT START-OF-SELECTION STATICS STOP SUBMIT SUBTRACT SUBTRACT-CORRESPONDING SUM SUMMARY SUPPRESS SYNTAX-CHECK SYNTAX-TRACE
syn keyword abapStatement TABLES TOP-OF-PAGE TRANSFER TRANSLATE TYPE TYPE-POOL TYPE-POOLS TYPES
syn keyword abapStatement UNPACK UPDATE
syn keyword abapStatement WHEN WHILE WINDOW WRITE

" More statemets
syn keyword abapStatement OCCURS STRUCTURE OBJECT PROPERTY
syn keyword abapStatement CASTING APPEND RAISING VALUE COLOR
syn keyword abapStatement LINE-SIZE LINE-COUNT MESSAGE-ID
syn keyword abapStatement CHANGING EXCEPTIONS DEFAULT CHECKBOX COMMENT
syn keyword abapStatement ID NUMBER FOR DISPLAY-MODE TITLE OUTPUT

" More multi-word statements
syn match   abapStatement "\(\W\|^\)\(WITH\W\+\(HEADER\W\+LINE\|FRAME\|KEY\)\|WITH\)\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)NO\W\+STANDARD\W\+PAGE\W\+HEADING\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)\(EXIT\W\+FROM\W\+STEP\W\+LOOP\|EXIT\)\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)\(BEGIN\W\+OF\W\+\(BLOCK\|LINE\)\|BEGIN\W\+OF\)\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)\(END\W\+OF\W\+\(BLOCK\|LINE\)\|END\W\+OF\)\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)IS\W\+INITIAL\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)NO\W\+INTERVALS\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)SEPARATED\W\+BY\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)\(USING\W\+\(EDIT\W\+MASK\)\|USING\)\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)\(WHERE\W\+\(LINE\)\)\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)RADIOBUTTON\W\+GROUP\(\W\|$\)"ms=s+1,me=e-1
syn match   abapStatement "\(\W\|^\)REF\W\+TO\(\W\|$\)"ms=s+1,me=e-1

" Special ABAP specific tables:
syn match   abapSpecial       "\(\W\|^\)\(sy\|\(p\|pa\)\d\d\d\d\|t\d\d\d.\|innnn\)\(\W\|$\)"ms=s+1,me=e-1
syn match   abapSpecialTables "\(sy\|\(p\|pa\)\d\d\d\d\|t\d\d\d.\|innnn\)-"me=e-1 contained
syn match   abapSpecial       "\(\W\|^\)\w\+-\(\w\+-\w\+\|\w\+\)"ms=s+1 contains=abapSpecialTables

" Pointer
syn match   abapSpecial  "<\w\+>"

" Abap constants:
syn keyword abapSpecial  TRUE FALSE NULL SPACE

" Includes
syn region abapInclude   start="include" end="." contains=abapComment

" Types
syn keyword abapTypes    c n i p f d t x

" Atritmitic operators
syn keyword abapOperator abs sign ceil floor trunc frac acos asin atan cos sin tan
syn keyword abapOperator cosh sinh tanh exp log log10 sqrt

" String operators
syn keyword abapOperator strlen xstrlen charlen numofchar dbmaxlen

" Table operators
syn keyword abapOperator lines

" Table operators (SELECT operators)
syn keyword abapOperator INTO FROM WHERE GROUP BY HAVING ORDER BY SINGLE
syn keyword abapOperator APPENDING CORRESPONDING FIELDS OF TABLE
syn keyword abapOperator LEFT RIGHT OUTER INNER JOIN AS CLIENT SPECIFIED BYPASSING BUFFER UP TO ROWS CONNECTING
syn keyword abapOperator EQ NE LT LE GT GE NOT AND OR XOR IN LIKE BETWEEN

" An error? Not strictly... but cannot think of reason this is intended.
syn match   abapError    "\.\."

" Comments
syn region  abapComment  start="^\*" end="$" contains=abapTodo
syn match   abapComment  "\".*" contains=abapTodo
syn keyword abapTodo     contained TODO NOTE

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_abap_syntax_inits")
  if version < 508
    let did_abap_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink abapError          Error
  HiLink abapComment        Comment
  HiLink abapInclude        Include
  HiLink abapSpecial        Special
  HiLink abapSpecialTables  PreProc
  HiLink abapSymbolOperator abapOperator
  HiLink abapOperator       Operator
  HiLink abapStatement      Statement
  HiLink abapString         String
  HiLink abapFloat          Float
  HiLink abapNumber         Number
  HiLink abapHex            Number

  delcommand HiLink
endif

let b:current_syntax = "abap"

" vim: ts=8 sw=2

