" Vim syntax file
"    Language: ColdFusion
"  Maintainer: Toby Woodwark (toby.woodwark+vim@gmail.com)
" Last Change: 2005 Nov 25
"   Filenames: *.cfc *.cfm
"     Version: Macromedia ColdFusion MX 7
"       Usage: Note that ColdFusion has its own comment syntax
"              i.e. <!--- --->

" For version 5.x, clear all syntax items.
" For version 6.x+, quit if a syntax file is already loaded.
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Use all the stuff from the HTML syntax file.
" TODO remove this; CFML is not a superset of HTML
if version < 600
  source <sfile>:p:h/html.vim
else
  runtime! syntax/html.vim
endif

syn sync fromstart
syn sync maxlines=200
syn case ignore

" Scopes and keywords.
syn keyword cfScope contained cgi cffile request caller this thistag cfcatch variables application server session client form url attributes arguments
syn keyword cfBool contained yes no true false

" Operator strings.
" Not exhaustive, since there are longhand equivalents.
syn keyword cfOperator contained xor eqv and or lt le lte gt ge gte eq neq not is mod contains
syn match cfOperatorMatch contained "[\+\-\*\/\\\^\&][\+\-\*\/\\\^\&]\@!"
syn cluster cfOperatorCluster contains=cfOperator,cfOperatorMatch

" Tag names.
syn keyword cfTagName contained cfabort cfapplet cfapplication cfargument cfassociate cfbreak cfcache
syn keyword cfTagName contained cfcalendar cfcase cfcatch cfchart cfchartdata cfchartseries cfcol cfcollection
syn keyword cfTagName contained cfcomponent cfcontent cfcookie cfdefaultcase cfdirectory cfdocument
syn keyword cfTagName contained cfdocumentitem cfdocumentsection cfdump cfelse cfelseif cferror cfexecute
syn keyword cfTagName contained cfexit cffile cfflush cfform cfformgroup cfformitem cfftp cffunction cfgrid
syn keyword cfTagName contained cfgridcolumn cfgridrow cfgridupdate cfheader cfhtmlhead cfhttp cfhttpparam cfif
syn keyword cfTagName contained cfimport cfinclude cfindex cfinput cfinsert cfinvoke cfinvokeargument
syn keyword cfTagName contained cfldap cflocation cflock cflog cflogin cfloginuser cflogout cfloop cfmail
syn keyword cfTagName contained cfmailparam cfmailpart cfmodule cfNTauthenticate cfobject cfobjectcache
syn keyword cfTagName contained cfoutput cfparam cfpop cfprocessingdirective cfprocparam cfprocresult
syn keyword cfTagName contained cfproperty cfquery cfqueryparam cfregistry cfreport cfreportparam cfrethrow
syn keyword cfTagName contained cfreturn cfsavecontent cfschedule cfscript cfsearch cfselect cfset cfsetting
syn keyword cfTagName contained cfsilent cfslider cfstoredproc cfswitch cftable cftextarea cfthrow cftimer
syn keyword cfTagName contained cftrace cftransaction cftree cftreeitem cftry cfupdate cfwddx cfxml 

" Tag parameters.
syn keyword cfArg contained abort accept access accessible action addnewline addtoken addtoken agentname
syn keyword cfArg contained align appendkey appletsource application applicationtimeout applicationtoken
syn keyword cfArg contained archive argumentcollection arguments asciiextensionlist attachmentpath
syn keyword cfArg contained attributecollection attributes attributes autowidth backgroundcolor
syn keyword cfArg contained backgroundvisible basetag bcc bgcolor bind bindingname blockfactor body bold
syn keyword cfArg contained border branch cachedafter cachedwithin casesensitive categories category
syn keyword cfArg contained categorytree cc cfsqltype charset chartheight chartwidth checked class
syn keyword cfArg contained clientmanagement clientstorage codebase colheaderalign colheaderbold
syn keyword cfArg contained colheaderfont colheaderfontsize colheaderitalic colheaders colheadertextcolor
syn keyword cfArg contained collection colorlist colspacing columns completepath component condition
syn keyword cfArg contained connection contentid context contextbytes contexthighlightbegin
syn keyword cfArg contained contexthighlightend contextpassages cookiedomain criteria custom1 custom2
syn keyword cfArg contained custom3 custom4 data dataalign databackgroundcolor datacollection
syn keyword cfArg contained datalabelstyle datasource date daynames dbname dbserver dbtype dbvarname debug
syn keyword cfArg contained default delete deletebutton deletefile delimiter delimiters description
syn keyword cfArg contained destination detail directory disabled display displayname disposition dn domain
syn keyword cfArg contained enablecab enablecfoutputonly enabled encoded encryption enctype enddate
syn keyword cfArg contained endrange endrow endtime entry errorcode exception existing expand expires
syn keyword cfArg contained expireurl expression extendedinfo extends extensions external failifexists
syn keyword cfArg contained failto file filefield filename filter firstdayofweek firstrowasheaders font
syn keyword cfArg contained fontbold fontembed fontitalic fontsize foregroundcolor format formfields
syn keyword cfArg contained formula from generateuniquefilenames getasbinary grid griddataalign gridlines
syn keyword cfArg contained groovecolor group groupcasesensitive header headeralign headerbold headerfont
syn keyword cfArg contained headerfontsize headeritalic headerlines headertextcolor height highlighthref
syn keyword cfArg contained hint href hrefkey hscroll hspace htmltable id idletimeout img imgopen imgstyle
syn keyword cfArg contained index inline input insert insertbutton interval isolation italic item
syn keyword cfArg contained itemcolumn key keyonly label labelformat language list listgroups locale
syn keyword cfArg contained localfile log loginstorage lookandfeel mailerid mailto marginbottom marginleft
syn keyword cfArg contained marginright marginright margintop markersize markerstyle mask maxlength maxrows
syn keyword cfArg contained message messagenumber method mimeattach mimetype mode modifytype monthnames
syn keyword cfArg contained multipart multiple name namecomplict nameconflict namespace new newdirectory
syn keyword cfArg contained notsupported null numberformat object omit onchange onclick onerror onkeydown
syn keyword cfArg contained onkeyup onload onmousedown onmouseup onreset onsubmit onvalidate operation
syn keyword cfArg contained orderby orientation output outputfile overwrite ownerpassword pageencoding
syn keyword cfArg contained pageheight pagetype pagewidth paintstyle param_1 param_2 param_3 param_4
syn keyword cfArg contained param_5 parent passive passthrough password path pattern permissions picturebar
syn keyword cfArg contained pieslicestyle port porttypename prefix preloader preservedata previouscriteria
syn keyword cfArg contained procedure protocol provider providerdsn proxybypass proxypassword proxyport
syn keyword cfArg contained proxyserver proxyuser publish query queryasroot queryposition range rebind
syn keyword cfArg contained recurse redirect referral refreshlabel remotefile replyto report requesttimeout
syn keyword cfArg contained required reset resolveurl result resultset retrycount returnasbinary returncode
syn keyword cfArg contained returntype returnvariable roles rowheaderalign rowheaderbold rowheaderfont
syn keyword cfArg contained rowheaderfontsize rowheaderitalic rowheaders rowheadertextcolor rowheaderwidth
syn keyword cfArg contained rowheight scale scalefrom scaleto scope scriptprotect scriptsrc secure
syn keyword cfArg contained securitycontext select selectcolor selected selecteddate selectedindex
syn keyword cfArg contained selectmode separator seriescolor serieslabel seriesplacement server serviceport
syn keyword cfArg contained serviceportname sessionmanagement sessiontimeout setclientcookies setcookie
syn keyword cfArg contained setdomaincookies show3d showborder showdebugoutput showerror showlegend
syn keyword cfArg contained showmarkers showxgridlines showygridlines size skin sort sortascendingbutton
syn keyword cfArg contained sortcontrol sortdescendingbutton sortxaxis source spoolenable sql src start
syn keyword cfArg contained startdate startrange startrow starttime status statuscode statustext step
syn keyword cfArg contained stoponerror style subject suggestions suppresswhitespace tablename tableowner
syn keyword cfArg contained tablequalifier taglib target task template text textcolor textqualifier
syn keyword cfArg contained thread throwonerror throwonfailure throwontimeout time timeout timespan tipbgcolor tipstyle
syn keyword cfArg contained title to tooltip top toplevelvariable transfermode type uid unit url urlpath
syn keyword cfArg contained useragent username userpassword usetimezoneinfo validate validateat value
syn keyword cfArg contained valuecolumn values valuesdelimiter valuesdisplay var variable vertical visible
syn keyword cfArg contained vscroll vspace webservice width wmode wraptext wsdlfile xaxistitle xaxistype
syn keyword cfArg contained xoffset yaxistitle yaxistype yoffset

" ColdFusion Functions.
syn keyword cfFunctionName contained Abs GetFunctionList Max ACos GetGatewayHelper Mid AddSOAPRequestHeader
syn keyword cfFunctionName contained GetHttpRequestData Min AddSOAPResponseHeader GetHttpTimeString Minute
syn keyword cfFunctionName contained ArrayAppend GetLocale Month ArrayAvg GetLocaleDisplayName MonthAsString
syn keyword cfFunctionName contained ArrayClear GetMetaData Now ArrayDeleteAt GetMetricData NumberFormat
syn keyword cfFunctionName contained ArrayInsertAt GetPageContext ParagraphFormat ArrayIsEmpty GetProfileSections
syn keyword cfFunctionName contained ParseDateTime ArrayLen GetProfileString Pi ArrayMax GetSOAPRequest
syn keyword cfFunctionName contained PreserveSingleQuotes ArrayMin GetSOAPRequestHeader Quarter ArrayNew
syn keyword cfFunctionName contained GetSOAPResponse QueryAddColumn ArrayPrepend GetSOAPResponseHeader QueryAddRow
syn keyword cfFunctionName contained ArrayResize GetTempDirectory QueryNew ArraySet GetTempFile QuerySetCell
syn keyword cfFunctionName contained ArraySort GetTickCount QuotedValueList ArraySum GetTimeZoneInfo Rand ArraySwap
syn keyword cfFunctionName contained GetToken Randomize ArrayToList Hash RandRange Asc Hour REFind ASin
syn keyword cfFunctionName contained HTMLCodeFormat REFindNoCase Atn HTMLEditFormat ReleaseComObject BinaryDecode
syn keyword cfFunctionName contained IIf RemoveChars BinaryEncode IncrementValue RepeatString BitAnd InputBaseN
syn keyword cfFunctionName contained Replace BitMaskClear Insert ReplaceList BitMaskRead Int ReplaceNoCase
syn keyword cfFunctionName contained BitMaskSet IsArray REReplace BitNot IsBinary REReplaceNoCase BitOr IsBoolean
syn keyword cfFunctionName contained Reverse BitSHLN IsCustomFunction Right BitSHRN IsDate RJustify BitXor
syn keyword cfFunctionName contained IsDebugMode Round Ceiling IsDefined RTrim CharsetDecode IsLeapYear Second
syn keyword cfFunctionName contained CharsetEncode IsNumeric SendGatewayMessage Chr IsNumericDate SetEncoding
syn keyword cfFunctionName contained CJustify IsObject SetLocale Compare IsQuery SetProfileString CompareNoCase
syn keyword cfFunctionName contained IsSimpleValue SetVariable Cos IsSOAPRequest Sgn CreateDate IsStruct Sin
syn keyword cfFunctionName contained CreateDateTime IsUserInRole SpanExcluding CreateObject IsValid SpanIncluding
syn keyword cfFunctionName contained CreateODBCDate IsWDDX Sqr CreateODBCDateTime IsXML StripCR CreateODBCTime
syn keyword cfFunctionName contained IsXmlAttribute StructAppend CreateTime IsXmlDoc StructClear CreateTimeSpan
syn keyword cfFunctionName contained IsXmlElem StructCopy CreateUUID IsXmlNode StructCount DateAdd IsXmlRoot
syn keyword cfFunctionName contained StructDelete DateCompare JavaCast StructFind DateConvert JSStringFormat
syn keyword cfFunctionName contained StructFindKey DateDiff LCase StructFindValue DateFormat Left StructGet
syn keyword cfFunctionName contained DatePart Len StructInsert Day ListAppend StructIsEmpty DayOfWeek
syn keyword cfFunctionName contained ListChangeDelims StructKeyArray DayOfWeekAsString ListContains StructKeyExists
syn keyword cfFunctionName contained DayOfYear ListContainsNoCase StructKeyList DaysInMonth ListDeleteAt StructNew
syn keyword cfFunctionName contained DaysInYear ListFind StructSort DE ListFindNoCase StructUpdate DecimalFormat
syn keyword cfFunctionName contained ListFirst Tan DecrementValue ListGetAt TimeFormat Decrypt ListInsertAt
syn keyword cfFunctionName contained ToBase64 DeleteClientVariable ListLast ToBinary DirectoryExists ListLen
syn keyword cfFunctionName contained ToScript DollarFormat ListPrepend ToString Duplicate ListQualify Trim Encrypt
syn keyword cfFunctionName contained ListRest UCase Evaluate ListSetAt URLDecode Exp ListSort URLEncodedFormat
syn keyword cfFunctionName contained ExpandPath ListToArray URLSessionFormat FileExists ListValueCount Val Find
syn keyword cfFunctionName contained ListValueCountNoCase ValueList FindNoCase LJustify Week FindOneOf Log Wrap
syn keyword cfFunctionName contained FirstDayOfMonth Log10 WriteOutput Fix LSCurrencyFormat XmlChildPos FormatBaseN
syn keyword cfFunctionName contained LSDateFormat XmlElemNew GetTempDirectory LSEuroCurrencyFormat XmlFormat
syn keyword cfFunctionName contained GetAuthUser LSIsCurrency XmlGetNodeType GetBaseTagData LSIsDate XmlNew
syn keyword cfFunctionName contained GetBaseTagList LSIsNumeric XmlParse GetBaseTemplatePath LSNumberFormat
syn keyword cfFunctionName contained XmlSearch GetClientVariablesList LSParseCurrency XmlTransform
syn keyword cfFunctionName contained GetCurrentTemplatePath LSParseDateTime XmlValidate GetDirectoryFromPath
syn keyword cfFunctionName contained LSParseEuroCurrency Year GetEncoding LSParseNumber YesNoFormat GetException
syn keyword cfFunctionName contained LSTimeFormat GetFileFromPath LTrim 

syn cluster htmlTagNameCluster add=cfTagName
syn cluster htmlArgCluster add=cfArg,cfHashRegion,cfScope
syn cluster htmlPreproc add=cfHashRegion

syn cluster cfExpressionCluster contains=cfFunctionName,cfScope,@cfOperatorCluster,cfScriptStringD,cfScriptStringS,cfScriptNumber,cfBool

" Evaluation; skip strings ( this helps with cases like nested IIf() )
syn region cfHashRegion start=+#+ skip=+"[^"]*"\|'[^']*'+ end=+#+ contains=@cfExpressionCluster,cfScriptParenError

" <cfset>, <cfif>, <cfelseif>, <cfreturn> are analogous to hashmarks (implicit evaluation) and has 'var'
syn region cfSetRegion start="<cfset " start="<cfreturn " start="<cfelseif " start="<cfif " end='>' keepend contains=@cfExpressionCluster,cfSetLHSRegion,cfSetTagEnd,cfScriptType
syn region cfSetLHSRegion contained start="<cfreturn" start="<cfelseif" start="<cfif" start="<cfset" end=" " keepend contains=cfTagName,htmlTag
syn match  cfSetTagEnd contained '>'

" CF comments: similar to SGML comments
syn region  cfComment     start='<!---' end='--->' keepend contains=cfCommentTodo
syn keyword cfCommentTodo contained TODO FIXME XXX TBD WTF 

" CFscript 
syn match   cfScriptLineComment      contained "\/\/.*$" contains=cfCommentTodo
syn region  cfScriptComment	     contained start="/\*"  end="\*/" contains=cfCommentTodo
" in CF, quotes are escaped by doubling
syn region  cfScriptStringD	     contained start=+"+  skip=+\\\\\|""+  end=+"+  extend contains=@htmlPreproc,cfHashRegion
syn region  cfScriptStringS	     contained start=+'+  skip=+\\\\\|''+  end=+'+  extend contains=@htmlPreproc,cfHashRegion
syn match   cfScriptNumber	     contained "-\=\<\d\+L\=\>"
syn keyword cfScriptConditional      contained if else
syn keyword cfScriptRepeat	     contained while for in
syn keyword cfScriptBranch	     contained break switch case try catch continue
syn keyword cfScriptFunction	     contained function
syn keyword cfScriptType	     contained var
syn match   cfScriptBraces	     contained "[{}]"
syn keyword cfScriptStatement        contained return

syn cluster cfScriptCluster contains=cfScriptParen,cfScriptLineComment,cfScriptComment,cfScriptStringD,cfScriptStringS,cfScriptFunction,cfScriptNumber,cfScriptRegexpString,cfScriptBoolean,cfScriptBraces,cfHashRegion,cfFunctionName,cfScope,@cfOperatorCluster,cfScriptConditional,cfScriptRepeat,cfScriptBranch,cfScriptType,@cfExpressionCluster,cfScriptStatement

" Errors caused by wrong parenthesis; skip strings
syn region  cfScriptParen       contained transparent skip=+"[^"]*"\|'[^']*'+ start=+(+ end=+)+ contains=@cfScriptCluster
syn match   cfScrParenError 	contained +)+

syn region cfscriptBlock matchgroup=NONE start="<cfscript>"  end="<\/cfscript>"me=s-1 keepend contains=@cfScriptCluster,cfscriptTag,cfScrParenError
syn region  cfscriptTag contained start='<cfscript' end='>' keepend contains=cfTagName,htmlTag

" Define the default highlighting.
if version >= 508 || !exists("did_cf_syn_inits")
  if version < 508
    let did_cf_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink cfTagName 		Statement
  HiLink cfArg 			Type
  HiLink cfFunctionName 	Function
  HiLink cfHashRegion 		PreProc
  HiLink cfComment 		Comment
  HiLink cfCommentTodo 		Todo
  HiLink cfOperator		Operator
  HiLink cfOperatorMatch	Operator
  HiLink cfScope		Title
  HiLink cfBool			Constant

  HiLink cfscriptBlock 		Special
  HiLink cfscriptTag 		htmlTag
  HiLink cfSetRegion 		PreProc
  HiLink cfSetLHSRegion 	htmlTag
  HiLink cfSetTagEnd		htmlTag

  HiLink cfScriptLineComment	Comment
  HiLink cfScriptComment	Comment
  HiLink cfScriptStringS	String
  HiLink cfScriptStringD	String
  HiLink cfScriptNumber	     	cfScriptValue
  HiLink cfScriptConditional	Conditional
  HiLink cfScriptRepeat	     	Repeat
  HiLink cfScriptBranch	     	Conditional
  HiLink cfScriptType		Type
  HiLink cfScriptStatement	Statement
  HiLink cfScriptBraces	     	Function
  HiLink cfScriptFunction    	Function
  HiLink cfScriptError	     	Error
  HiLink cfScrParenError	cfScriptError

  delcommand HiLink
endif

let b:current_syntax = "cf"

" vim: ts=8 sw=2
