" Vim syntax file
" Language:	phtml PHP 2.0
" Maintainer:	Lutz Eymers <ixtab@polzin.com>
" URL:		http://www.isp.de/data/phtml.vim
" Email:	Subject: send syntax_vim.tgz
" Last change:	2003 May 11
"
" Options	phtml_sql_query = 1 for SQL syntax highligthing inside strings
"		phtml_minlines = x     to sync at least x lines backwards

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if !exists("main_syntax")
  let main_syntax = 'phtml'
endif

if version < 600
  so <sfile>:p:h/html.vim
else
  runtime! syntax/html.vim
  unlet b:current_syntax
endif

syn cluster htmlPreproc add=phtmlRegionInsideHtmlTags

if exists( "phtml_sql_query")
  if phtml_sql_query == 1
    syn include @phtmlSql <sfile>:p:h/sql.vim
    unlet b:current_syntax
  endif
endif
syn cluster phtmlSql remove=sqlString,sqlComment

syn case match

" Env Variables
syn keyword phtmlEnvVar SERVER_SOFTWARE SERVER_NAME SERVER_URL GATEWAY_INTERFACE   contained
syn keyword phtmlEnvVar SERVER_PROTOCOL SERVER_PORT REQUEST_METHOD PATH_INFO  contained
syn keyword phtmlEnvVar PATH_TRANSLATED SCRIPT_NAME QUERY_STRING REMOTE_HOST contained
syn keyword phtmlEnvVar REMOTE_ADDR AUTH_TYPE REMOTE_USER CONTEN_TYPE  contained
syn keyword phtmlEnvVar CONTENT_LENGTH HTTPS HTTPS_KEYSIZE HTTPS_SECRETKEYSIZE  contained
syn keyword phtmlEnvVar HTTP_ACCECT HTTP_USER_AGENT HTTP_IF_MODIFIED_SINCE  contained
syn keyword phtmlEnvVar HTTP_FROM HTTP_REFERER contained
syn keyword phtmlEnvVar PHP_SELF contained

syn case ignore

" Internal Variables
syn keyword phtmlIntVar phperrmsg php_self contained

" Comment
syn region phtmlComment		start="/\*" end="\*/"  contained contains=phtmlTodo

" Function names
syn keyword phtmlFunctions  Abs Ada_Close Ada_Connect Ada_Exec Ada_FetchRow contained
syn keyword phtmlFunctions  Ada_FieldName Ada_FieldNum Ada_FieldType contained
syn keyword phtmlFunctions  Ada_FreeResult Ada_NumFields Ada_NumRows Ada_Result contained
syn keyword phtmlFunctions  Ada_ResultAll AddSlashes ASort BinDec Ceil ChDir contained
syn keyword phtmlFunctions  AdaGrp ChMod ChOwn Chop Chr ClearStack ClearStatCache contained
syn keyword phtmlFunctions  closeDir CloseLog Cos Count Crypt Date dbList  contained
syn keyword phtmlFunctions  dbmClose dbmDelete dbmExists dbmFetch dbmFirstKey contained
syn keyword phtmlFunctions  dbmInsert dbmNextKey dbmOpen dbmReplace DecBin DecHex contained
syn keyword phtmlFunctions  DecOct doubleval Echo End ereg eregi ereg_replace contained
syn keyword phtmlFunctions  eregi_replace EscapeShellCmd Eval Exec Exit Exp contained
syn keyword phtmlFunctions  fclose feof fgets fgetss File fileAtime fileCtime contained
syn keyword phtmlFunctions  fileGroup fileInode fileMtime fileOwner filePerms contained
syn keyword phtmlFunctions  fileSize fileType Floor Flush fopen fputs FPassThru contained
syn keyword phtmlFunctions  fseek fsockopen ftell getAccDir GetEnv getHostByName contained
syn keyword phtmlFunctions  getHostByAddr GetImageSize getLastAcess contained
syn keyword phtmlFunctions  getLastbrowser getLastEmail getLastHost getLastMod contained
syn keyword phtmlFunctions  getLastref getLogDir getMyInode getMyPid getMyUid contained
syn keyword phtmlFunctions  getRandMax getStartLogging getToday getTotal GetType contained
syn keyword phtmlFunctions  gmDate Header HexDec HtmlSpecialChars ImageArc contained
syn keyword phtmlFunctions  ImageChar ImageCharUp IamgeColorAllocate  contained
syn keyword phtmlFunctions  ImageColorTransparent ImageCopyResized ImageCreate contained
syn keyword phtmlFunctions  ImageCreateFromGif ImageDestroy ImageFill contained
syn keyword phtmlFunctions  ImageFilledPolygon ImageFilledRectangle contained
syn keyword phtmlFunctions  ImageFillToBorder ImageGif ImageInterlace ImageLine contained
syn keyword phtmlFunctions  ImagePolygon ImageRectangle ImageSetPixel  contained
syn keyword phtmlFunctions  ImageString ImageStringUp ImageSX ImageSY Include contained
syn keyword phtmlFunctions  InitSyslog intval IsSet Key Link LinkInfo Log Log10 contained
syn keyword phtmlFunctions  LosAs Mail Max Md5 mi_Close mi_Connect mi_DBname contained
syn keyword phtmlFunctions  mi_Exec mi_FieldName mi_FieldNum mi_NumFields contained
syn keyword phtmlFunctions  mi_NumRows mi_Result Microtime Min MkDir MkTime msql contained
syn keyword phtmlFunctions  msql_connect msql_CreateDB msql_dbName msql_DropDB contained
syn keyword phtmlFunctions  msqlFieldFlags msql_FieldLen msql_FieldName contained
syn keyword phtmlFunctions  msql_FieldType msql_FreeResult msql_ListDBs contained
syn keyword phtmlFunctions  msql_Listfields msql_ListTables msql_NumFields contained
syn keyword phtmlFunctions  msql_NumRows msql_RegCase msql_Result msql_TableName contained
syn keyword phtmlFunctions  mysql mysql_affected_rows mysql_close mysql_connect contained
syn keyword phtmlFunctions  mysql_CreateDB mysql_dbName mysqlDropDB  contained
syn keyword phtmlFunctions  mysql_FieldFlags mysql_FieldLen mysql_FieldName contained
syn keyword phtmlFunctions  mysql_FieldType mysql_FreeResult mysql_insert_id contained
syn keyword phtmlFunctions  mysql_listDBs mysql_Listfields mysql_ListTables contained
syn keyword phtmlFunctions  mysql_NumFields mysql_NumRows mysql_Result  contained
syn keyword phtmlFunctions  mysql_TableName Next OctDec openDir OpenLog  contained
syn keyword phtmlFunctions  Ora_Bind Ora_Close Ora_Commit Ora_CommitOff contained
syn keyword phtmlFunctions  Ora_CommitOn Ora_Exec Ora_Fetch Ora_GetColumn contained
syn keyword phtmlFunctions  Ora_Logoff Ora_Logon Ora_Parse Ora_Rollback Ord  contained
syn keyword phtmlFunctions  Parse_str PassThru pclose pg_Close pg_Connect contained
syn keyword phtmlFunctions  pg_DBname pg_ErrorMessage pg_Exec pg_FieldName contained
syn keyword phtmlFunctions  pg_FieldPrtLen pg_FieldNum pg_FieldSize  contained
syn keyword phtmlFunctions  pg_FieldType pg_FreeResult pg_GetLastOid pg_Host contained
syn keyword phtmlFunctions  pg_NumFields pg_NumRows pg_Options pg_Port  contained
syn keyword phtmlFunctions  pg_Result pg_tty phpInfo phpVersion popen pos pow contained
syn keyword phtmlFunctions  Prev PutEnv QuoteMeta Rand readDir ReadFile ReadLink contained
syn keyword phtmlFunctions  reg_Match reg_replace reg_Search Rename Reset return  contained
syn keyword phtmlFunctions  rewind rewindDir RmDir rSort SetCookie SetErrorReporting contained
syn keyword phtmlFunctions  SetLogging SetShowInfo SetType shl shr Sin Sleep contained
syn keyword phtmlFunctions  Solid_Close Solid_Connect Solid_Exec Solid_FetchRow contained
syn keyword phtmlFunctions  Solid_FieldName Solid_FieldNum Solid_FreeResult  contained
syn keyword phtmlFunctions  Solid_NumFields Solid_NumRows Solid_Result Sort contained
syn keyword phtmlFunctions  Spundtex Sprintf Sqrt Srand strchr strtr  contained
syn keyword phtmlFunctions  StripSlashes strlen strchr strstr strtok strtolower contained
syn keyword phtmlFunctions  strtoupper strval substr sybSQL_CheckConnect contained
syn keyword phtmlFunctions  sybSQL_DBUSE sybSQL_Connect sybSQL_Exit contained
syn keyword phtmlFunctions  sybSQL_Fieldname sybSQL_GetField sybSQL_IsRow  contained
syn keyword phtmlFunctions  sybSQL_NextRow sybSQL_NumFields sybSQL_NumRows contained
syn keyword phtmlFunctions  sybSQL_Query sybSQL_Result sybSQL_Result sybSQL_Seek contained
syn keyword phtmlFunctions  Symlink syslog System Tan TempNam Time Umask UniqId contained
syn keyword phtmlFunctions  Unlink Unset UrlDecode UrlEncode USleep Virtual contained
syn keyword phtmlFunctions  SecureVar contained

" Conditional
syn keyword phtmlConditional  if else elseif endif switch endswitch contained

" Repeat
syn keyword phtmlRepeat  while endwhile contained

" Repeat
syn keyword phtmlLabel  case default contained

" Statement
syn keyword phtmlStatement  break return continue exit contained

" Operator
syn match phtmlOperator  "[-=+%^&|*!]" contained
syn match phtmlOperator  "[-+*/%^&|]=" contained
syn match phtmlOperator  "/[^*]"me=e-1 contained
syn match phtmlOperator  "\$" contained
syn match phtmlRelation  "&&" contained
syn match phtmlRelation  "||" contained
syn match phtmlRelation  "[!=<>]=" contained
syn match phtmlRelation  "[<>]" contained

" Identifier
syn match  phtmlIdentifier "$\h\w*" contained contains=phtmlEnvVar,phtmlIntVar,phtmlOperator


" Include
syn keyword phtmlInclude  include contained

" Definesag
syn keyword phtmlDefine  Function contained

" String
syn region phtmlString keepend matchgroup=None start=+"+ skip=+\\\\\|\\"+  end=+"+ contains=phtmlIdentifier,phtmlSpecialChar,@phtmlSql contained

" Number
syn match phtmlNumber  "-\=\<\d\+\>" contained

" Float
syn match phtmlFloat  "\(-\=\<\d+\|-\=\)\.\d\+\>" contained

" SpecialChar
syn match phtmlSpecialChar "\\[abcfnrtyv\\]" contained
syn match phtmlSpecialChar "\\\d\{3}" contained contains=phtmlOctalError
syn match phtmlSpecialChar "\\x[0-9a-fA-F]\{2}" contained

syn match phtmlOctalError "[89]" contained


syn match phtmlParentError "[)}\]]" contained

" Todo
syn keyword phtmlTodo TODO Todo todo contained

" Parents
syn cluster phtmlInside contains=phtmlComment,phtmlFunctions,phtmlIdentifier,phtmlConditional,phtmlRepeat,phtmlLabel,phtmlStatement,phtmlOperator,phtmlRelation,phtmlString,phtmlNumber,phtmlFloat,phtmlSpecialChar,phtmlParent,phtmlParentError,phtmlInclude

syn cluster phtmlTop contains=@phtmlInside,phtmlInclude,phtmlDefine,phtmlParentError,phtmlTodo
syn region phtmlParent	matchgroup=Delimiter start="(" end=")" contained contains=@phtmlInside
syn region phtmlParent	matchgroup=Delimiter start="{" end="}" contained contains=@phtmlInside
syn region phtmlParent	matchgroup=Delimiter start="\[" end="\]" contained contains=@phtmlInside

syn region phtmlRegion keepend matchgroup=Delimiter start="<?" skip=+(.*>.*)\|".\{-}>.\{-}"\|/\*.\{-}>.\{-}\*/+ end=">" contains=@phtmlTop
syn region phtmlRegionInsideHtmlTags keepend matchgroup=Delimiter start="<?" skip=+(.*>.*)\|/\*.\{-}>.\{-}\*/+ end=">" contains=@phtmlTop contained

" sync
if exists("phtml_minlines")
  exec "syn sync minlines=" . phtml_minlines
else
  syn sync minlines=100
endif

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_phtml_syn_inits")
  if version < 508
    let did_phtml_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink phtmlComment		Comment
  HiLink phtmlString		String
  HiLink phtmlNumber		Number
  HiLink phtmlFloat		Float
  HiLink phtmlIdentifier	Identifier
  HiLink phtmlIntVar		Identifier
  HiLink phtmlEnvVar		Identifier
  HiLink phtmlFunctions		Function
  HiLink phtmlRepeat		Repeat
  HiLink phtmlConditional	Conditional
  HiLink phtmlLabel		Label
  HiLink phtmlStatement		Statement
  HiLink phtmlType		Type
  HiLink phtmlInclude		Include
  HiLink phtmlDefine		Define
  HiLink phtmlSpecialChar	SpecialChar
  HiLink phtmlParentError	Error
  HiLink phtmlOctalError	Error
  HiLink phtmlTodo		Todo
  HiLink phtmlOperator		Operator
  HiLink phtmlRelation		Operator

  delcommand HiLink
endif

let b:current_syntax = "phtml"

if main_syntax == 'phtml'
  unlet main_syntax
endif

" vim: ts=8
