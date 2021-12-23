" Vim syntax file
" Language:    SQL, PL/SQL (Oracle 19c)
" Maintainer:  Christian Brabandt
" Repository:   https://github.com/chrisbra/vim-sqloracle-syntax
" License:      Vim
" Previous Maintainer: Paul Moore
" Last Change: 2021 April 28

" Changes:
" 02.04.2016: Support for when keyword
" 03.04.2016: Support for join related keywords
" 22.07.2016: Support Oracle Q-Quote-Syntax
" 25.07.2016: Support for Oracle N'-Quote syntax
" 22.06.2018: Remove skip part for sqlString (do not escape strings)
" (https://web.archive.org/web/20150922065035/https://mariadb.com/kb/en/sql-99/character-string-literals/)
" 28.04.2021: Support for Oracle 19c (Fahad AlHoshan)

if exists("b:current_syntax")
  finish
endif

" Make all Syntax Highlighting case insensitive.
syntax case ignore

" Oracle SQL, and PLSQL Strings:
syntax region oraString	matchgroup=oraQuote start=+n\="+                         end=+"+
syntax region oraString	matchgroup=oraQuote start=+n\='+                         end=+'+
syntax region oraString	matchgroup=oraQuote start=+n\=q'\z([^[:space:](<{]\)+    end=+\z1'+
syntax region oraString	matchgroup=oraQuote start=+n\=q'<+                       end=+>'+
syntax region oraString	matchgroup=oraQuote start=+n\=q'{+                       end=+}'+
syntax region oraString	matchgroup=oraQuote start=+n\=q'(+                       end=+)'+
syntax region oraString	matchgroup=oraQuote start=+n\=q'\[+                      end=+]'+

" Oracle SQL, and PLSQL Pseudocolumns:
syntax keyword oraPseudocolumn CONNECT_BY_ISCYCLE CONNECT_BY_ISLEAF LEVEL CURRVAL NEXTVAL VERSIONS_STARTSCN VERSIONS_STARTTIME VERSIONS_ENDSCN
syntax keyword oraPseudocolumn VERSIONS_ENDTIME VERSIONS_XID VERSIONS_OPERATION COLUMN_VALUE OBJECT_ID OBJECT_VALUE ROWNUM XMLDATA

" Oracle SQL, and PLSQL Operators:
syntax match oraOperator      "!\|\$\|%\|&\|(\|)\|*\|+\|,\|-\|\.\|/\|:\|;\|<\|=\|>\|?\|@\|\[\|\]\|\^\||\|{\|}"
syntax keyword oraOperator ALL AND ANY AS ASC BETWEEN BY COLLATE CONNECT_BY_ROOT CURRENT DESC DISTINCT ESCAPE EXCEPT EXISTS FOLLOWING IGNORE INTERSECT LIKE LIKE2 LIKE4 LIKEC MINUS
syntax keyword oraOperator MULTISET NOT OF OR OUT PRECEDING PRIOR RANGE RESPECT SIZE SIZE_T SOME UNBOUNDED UNION UNIQUE USING IN BOTH

" Oracle SQL, and PLSQL Todos:
syntax keyword oraTodo TODO FIXME XXX DEBUG NOTE contained

" Oracle SQL, and PLSQL Comments:
syntax region oraComment      start="/\*"  end="\*/"  contains=@Spell,oraTodo
syntax match oraComment       "--.*$"                 contains=@Spell,oraTodo
syntax match oraComment       "^rem.*$"               contains=@Spell,oraTodo

" Oracle SQL, and PLSQL Various errors:
syntax match oraError         "\<\w\+\ze("                    " Not a known function.
syntax match oraError         ",\ze\([[:space:]]*\(;\|)\)\)"  " Comma before a paren or semicolon.
syntax match oraError         "[[:space:]]\=$"                " Space at the end of a line.
" Comma before certain words.
syntax match   oraError        ",\ze\([[:space:]]*\(ASC\|DESC\|EXISTS\|FOR\|FROM\|GROUP BY\|INTO\|LIMIT\|ORDER\|TABLE\|USING\|WHERE\)\)"

" Oracle SQL, and PLSQL Numbers, and Special values:
syntax keyword oraValue FALSE NULL NULLS TRUE
syntax match oraValue         ":\w\+"                                         " Bind Variables.
syntax match oraNumber        "\(-\|+\)\=[[:digit:]]\+\(\.[[:digit:]]\+\)\="  " Decimal Numbers.
syntax match oraNumber        "0x[[:xdigit:]]\+"                          " Hexadecimal Numbers.

" Oracle SQL, and PLSQL Data Types:
syntax keyword oratype ANYDATA ANYDATASET ANYTYPE BFILE BINARY_DOUBLE BINARY_FLOAT BLOB BOOLEAN CHAR CHARACTER CLOB DATE DATETIME DBURITYPE DECIMAL INTEGER INTERVAL LONG NCHAR NCLOB NUMBER NUMERIC
syntax keyword oratyp  NVARCHAR2 RAW REAL REF ROWID SDO_GEOMETRY SDO_GEORASTER SDO_TOPO_GEOMETRY VARCHAR2 VARCHAR VARRAY XDBURITYPE XMLTYPE DOUBLE FLOAT HTTPURITYPE INT SMALLINT TIMESTAMP URITYPE UROWID
syntax match oratype          "\<VAR\(I\(A\(B\(L\(E\)\=\)\=\)\=\)\=\)\=\>"

" Oracle SQL, and PLSQL Keywords:
syntax keyword oraKeyword ACCESS ABORT ACCESSIBLE ADD ADMIN AGENT AGGREGATE APPLY ANALYTIC ARCHIVE ARCHIVED ARRAY AT ATTRIBUTE AUTHID
syntax keyword oraKeyword BACKUP BFILE_BASE BINARY BLOB_BASE BLOCK BODY BOUND BULK BYTE
syntax keyword oraKeyword CACHE_TEMP_TABLE CALL CALLING CASCADE CHARSET CHARSETFORM CHARSETID CHAR_BASE CHECK CLOB_BASE CLONE CLOSE CLUSTER CLUSTERS COLAUTH COLUMN COLUMNS
syntax keyword oraKeyword COLUMN_AUTHORIZATION_INDICATOR COLUMN_AUTH_INDICATOR COMMITTED COMPILED COMPRESS CONNECT CONSTANT CONSTRAINTS CONSTRUCTOR CONTAINER CONTEXT CONTINUE CRASH
syntax keyword oraKeyword CREDENTIAL CURSOR CURRENT CUSTOMDATUM
syntax keyword oraKeyword DANGLING DATA DATABASE DATE_BASE DAY DEFAULT DEFINE DETERMINISTIC DIMENSION DISCONNECT DIRECTORY DURATION
syntax keyword oraKeyword ELEMENT ELIMINATE_OUTER_JOIN ELSE ELSIF EMPTY EXCLUSIVE EXIT EXTERNAL
syntax match oraKeyword       "\<END[[:space:]]\+\(LOOP\|IF\)\=\>"
syntax keyword oraKeyword FOR FROM
syntax keyword oraKeyword GENERAL GOTO GROUP
syntax keyword oraKeyword HASH HAVING HEAP HIDDEN HOUR
syntax keyword oraKeyword IDENTIFIED IF IMMEDIATE INCLUDING INCREMENT INDEX INDEXES INDEX_RS INDICATOR INDICES INFINITE INITIAL INSTANTIABLE INTERFACE INTO INVALIDATE IS ISOLATION
syntax keyword oraKeyword JAVA KEY KEYSTORE
syntax keyword oraKeyword LANGUAGE LARGE LEADING LIBRARY LIMIT LIMITED LOCAL LOCKDOWN LOGICAL LOG LOGFILE LOOP
syntax keyword oraKeyword MAP MANAGED MANAGEMENT MATERIALIZED MAXARCHLOGS MAXEXTENTS MAXLEN MEMBER MERGE MINUTE MLSLABEL MODE MODIFY MONTH
syntax keyword oraKeyword NAME NAN NATIONAL NATIVE NESTED_TABLE_ID NESTED_TABLE_SET_REFS NEW NOCOMPRESS NOCOPY NOCPU_COSTING NODELAY NOPARALLEL_INDEX NOREWRITE NORMAL NOWAIT
syntax keyword oraKeyword NO_ELIMINATE_OUTER_JOIN NO_FILTERING NO_PQ_MAP NUMBER_BASE
syntax keyword oraKeyword OBJECT OCICOLL OCIDATE OCIDATETIME OCIDURATION OCIINTERVAL OCILOBLOCATOR OCINUMBER OCIRAW OCIREF OCIREFCURSOR OCIROWID OCISTRING OCITYPE OFFLINE OLD ON ONLINE ONLY OPAQUE
syntax keyword oraKeyword OPEN OPERATOR OPTION ORACLE ORADATA ORA_GET_ACLIDS ORA_GET_PRIVILEGES ORDER ORGANIZATION ORLANY ORLVARY OTHERS OVERLAPS OVERRIDING
syntax keyword oraKeyword PACKAGE PARALLEL PARALLEL_ENABLE PARAM PARAMETER PARAMETERS PARENT PARTITION PASCAL PCTFREE PERSISTABLE PIPE PIPELINED PLUGGABLE POLICY POLYMORPHIC PRAGMA PRECISION PRIVATE
syntax keyword oraKeyword PROCEDURE PUBLIC PROFILE PROMPT PRINT
syntax keyword oraKeyword RAISE READ RECOVER RECORD REFERENCE REFERENCING RELIES_ON RESOURCE RESULT RESULT_CACHE RETURN RETURNING REVERSE ROLE ROLES ROW ROWS
syntax keyword oraKeyword SAMPLE SAVE SB1 SB2 SB4 SCOPE SECOND SEGMENT SELF SEPARATE SEQUENCE SERIALIZABLE SESSION SET SHARE SHORT SID SPARSE SQL SQLCODE SQLDATA SQLNAME SQLSTATE
syntax keyword oraKeyword STANDARD STANDBY START STATIC STORED STRING STRUCT STYLE SUBMULTISET SUBPARTITION SUBSTITUTABLE SUBTYPE SUCCESSFUL SYNONYM SWITCH
syntax keyword oraKeyword TABAUTH TABLE TABLESPACE TDO TAG THE THEN TIME TIMEZONE_ABBR TIMEZONE_HOUR TIMEZONE_MINUTE TIMEZONE_REGION TO TRAILING TRANSACTION TRANSACTIONAL TRIGGER TRUSTED TYPE
syntax keyword oraKeyword UB1 UB2 UB4 UNDER UNPLUG UNSIGNED UNTRUSTED USE UNDEFINE
syntax keyword oraKeyword VALIDATE VALIST VALUES VARYING VIEW VIEWS VOID
syntax keyword oraKeyword WHEN WHENEVER WHERE WHILE WITH WORK WRAPPED WRITE
syntax keyword oraKeyword YEAR
syntax keyword oraKeyword ZONE

" Oracle SQL, and PLSQL Exceptions:
syntax match oraException     "\<EXCEPTIONS\=\>"

" Oracle SQL, and PLSQL Statements:
syntax keyword oraStatement ADMINISTER ANALYZE AUDIT CASE COMMENT COMMIT DELETE DROP EXECUTE EXPLAIN GRANT LOCK NOAUDIT RENAME REVOKE ROLLBACK SAVEPOINT TRUNCATE DECLARE SHUTDOWN STARTUP
syntax match oraStatement     "\<END\ze\([[:space:]]*;\)"
syntax match oraStatement     "\<SHOW\=\>"

" next ones are contained, so folding works.
syntax keyword oraStatement CREATE UPDATE ALTER SELECT INSERT BEGIN contained
" Setup Folding:
" this is a hack, to get certain statements folded.
" the keywords create/update/alter/select/insert need to
" have contained option.
syntax region oraFold         start='^[[:space:]]*\zs\c\(CREATE\|UPDATE\|ALTER\|SELECT\|INSERT\|BEGIN\)'    end=';$\|^$' transparent fold contains=ALL

" Oracle SQL, and PLSQL Functions:
" (Oracle 19c)
" Aggregate Functions
syntax keyword oraFunction APPROX_COUNT APPROX_COUNT_DISTINCT APPROX_COUNT_DISTINCT_AGG APPROX_COUNT_DISTINCT_DETAIL APPROX_MEDIAN APPROX_PERCENTILE APPROX_PERCENTILE_AGG APPROX_PERCENTILE_DETAIL
syntax keyword oraFunction APPROX_RANK APPROX_SUM AVG CLUSTER_DETAILS CLUSTER_DISTANCE CLUSTER_ID CLUSTER_PROBABILITY CLUSTER_SET COLLECT CORR CORR_K CORR_S COUNT COVAR_POP COVAR_SAMP CUBE_TABLE CUME_DIST CV
syntax keyword oraFunction DATAOBJ_TO_MAT_PARTITION DATAOBJ_TO_PARTITION DENSE_RANK DEREF FEATURE_DETAILS FEATURE_ID FEATURE_SET FEATURE_VALUE FIRST FIRST_VALUE GROUPING GROUPING_ID GROUP_ID ITERATION_NUMBER
syntax keyword oraFunction JSON_ARRAYAGG JSON_OBJECTAGG LAG LAST LAST_VALUE LEAD LISTAGG MAKE_REF MAX MEDIAN MIN NTH_VALUE NTILE PERCENTILE_CONT PERCENTILE_DISC PERCENT_RANK PREDICTION PREDICTION_COST PREDICTION_DETAILS
syntax keyword oraFunction PREDICTION_PROBABILITY PREDICTION_SET PRESENTNNV PRESENTV PREVIOUS RANK RATIO_TO_REPORT REF REFTOHEX REGR_AVGX REGR_AVGY REGR_COUNT REGR_INTERCEPT REGR_R2 REGR_SLOPE REGR_SXX REGR_SXY REGR_SYY
syntax keyword oraFunction ROW_NUMBER STATS_BINOMIAL_TEST STATS_CROSSTAB STATS_F_TEST STATS_KS_TEST STATS_MODE STATS_MW_TEST STATS_ONE_WAY_ANOVA STATS_T_TEST_INDEP STATS_T_TEST_INDEPU
syntax keyword oraFunction STATS_T_TEST_ONE STATS_T_TEST_PAIRED STATS_WSR_TEST STDDEV STDDEV_POP STDDEV_SAMP SUM SYS_OP_ZONE_ID SYS_XMLAGG TO_APPROX_COUNT_DISTINCT TO_APPROX_PERCENTILE VALUE VARIANCE VAR_POP VAR_SAMP
syntax keyword oraFunction XMLAGG

" Character Functions
syntax keyword oraFunction CHR CONCAT INITCAP LOWER LPAD LTRIM NCHR NLS_INITCAP NLS_LOWER NLS_UPPER NLSSORT REGEXP_REPLACE REGEXP_SUBSTR REPLACE RPAD RTRIM SOUNDEX SUBSTR TRANSLATE
syntax keyword oraFunction TRIM UPPER ASCII INSTR LENGTH REGEXP_COUNT REGEXP_INSTR NLS_CHARSET_DECL_LEN NLS_CHARSET_ID NLS_CHARSET_NAME

" Collation Functions
syntax keyword oraFunction COLLATION NLS_COLLATION_ID NLS_COLLATION_NAME

" Collection Functions
syntax keyword oraFunction CARDINALITY COLLECT POWERMULTISET POWERMULTISET_BY_CARDINALITY

" Comparison Functions
syntax keyword oraFunction GREATEST LEAST

" Conversion Functions
syntax keyword oraFunction ASCIISTR BIN_TO_NUM CAST CHARTOROWID COMPOSE CONVERT DECOMPOSE HEXTORAW NUMTODSINTERVAL NUMTOYMINTERVAL RAWTOHEX RAWTONHEX ROWIDTOCHAR ROWIDTONCHAR SCN_TO_TIMESTAMP
      \ TIMESTAMP_TO_SCN TO_BINARY_DOUBLE TO_BINARY_FLOAT TO_BLOB TO_CHAR TO_CLOB TO_DATE TO_DSINTERVAL TO_LOB TO_MULTI_BYTE TO_NCHAR TO_NCLOB TO_NUMBER TO_SINGLE_BYTE TO_TIMESTAMP TO_TIMESTAMP_TZ
      \ TO_YMINTERVAL TREAT UNISTR VALIDATE_CONVERSION

" DataMining Functions
syntax keyword oraFunction CLUSTER_DETAILS CLUSTER_DISTANCE CLUSTER_ID CLUSTER_PROBABILITY CLUSTER_SET FEATURE_COMPARE FEATURE_DETAILS FEATURE_ID FEATURE_SET FEATURE_VALUE ORA_DM_PARTITION_NAME
syntax keyword oraFunction PREDICTION PREDICTION_BOUNDS PREDICTION_COST PREDICTION_DETAILS PREDICTION_PROBABILITY PREDICTION_SET

" Datetime Functions
syntax keyword oraFunction ADD_MONTHS CURRENT_DATE CURRENT_TIMESTAMP DBTIMEZONE FROM_TZ LAST_DAY LOCALTIMESTAMP MONTHS_BETWEEN NEW_TIME NEXT_DAY ORA_DST_AFFECTED ORA_DST_CONVERT ORA_DST_ERROR
syntax keyword oraFunction SESSIONTIMEZONE SYS_EXTRACT_UTC SYSDATE SYSTIMESTAMP TZ_OFFSET

" Encoding and Decoding Functions
syntax keyword oraFunction DECODE DUMP ORA_HASH STANDARD_HASH VSIZE

" Environment and Identifier Functions
syntax keyword oraFunction CON_DBID_TO_ID CON_GUID_TO_ID CON_NAME_TO_ID CON_UID_TO_ID ORA_INVOKING_USER ORA_INVOKING_USERID SYS_CONTEXT SYS_GUID SYS_TYPEID UID USER USERENV

" NULL-Related Functions
syntax keyword oraFunction COALESCE LNNVL NULLIF NVL NVL2

" Numeric Functions
syntax keyword oraFunction ABS ACOS ASIN ATAN ATAN2 BITAND CEIL COS COSH EXP FLOOR LN LOG MOD POWER REMAINDER SIGN SIN SINH SQRT TAN TANH WIDTH_BUCKET

" Multipurpose Functions
syntax keyword oraFunction ROUND TRUNC SYS_CONNECT_BY_PATH EXTRACT NANVL
syntax match oraFunction      "SET\ze\([[:space:]]*(\)"
" Oracle DBMS functions.
syntax match oraFunction    "DBMS_\w\+\.\w\+\ze\([[:space:]]*(\)"

" Large object Functions
syntax keyword oraFunction BFILENAME EMPTY_BLOB EMPTY_CLOB

" JSON Functions
syntax keyword oraFunction JSON_QUERY JSON_TABLE JSON_VALUE JSON_ARRAY JSON_ARRAYAGG JSON_OBJECT JSON_OBJECTAGG JSON_DATAGUIDE

" XML Functions
syntax keyword oraFunction DEPTH EXISTSNODE EXTRACTVALUE PATH SYS_DBURIGEN SYS_XMLAGG SYS_XMLGEN XMLAGG XMLCAST XMLCDATA XMLCOLATTVAL XMLCOMMENT XMLCONCAT XMLDIFF XMLELEMENT XMLEXISTS XMLFOREST
syntax keyword oraFunction XMLISVALID XMLPARSE XMLPATCH XMLPI XMLQUERY XMLROOT XMLSEQUENCE XMLSERIALIZE XMLTABLE XMLTRANSFORM

" Stolen from sh.vim.
if !exists("sh_minlines")
  let sh_minlines = 200
endif
if !exists("sh_maxlines")
  let sh_maxlines = 2 * sh_minlines
endif
exec "syntax sync minlines=" . sh_minlines . " maxlines=" . sh_maxlines

" Define the default highlighting.
highlight default link oraString          String
highlight default link oraQuote           Delimiter
highlight default link oraComment         Comment
highlight default link oraPseudocolumn    Identifier
highlight default link oraOperator        Operator
highlight default link oraValue           Special
highlight default link oraNumber          Number
highlight default link oraType            Type
highlight default link oraKeyword         Keyword
highlight default link oraFunction        Function
highlight default link oraException       Exception
highlight default link oraStatement       Statement
highlight default link oraTodo            Todo
highlight default link oraError           Error

let b:current_syntax = "sql"
" vim: ts=8
