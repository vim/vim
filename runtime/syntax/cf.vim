" Vim syntax file
"    Language: Cold Fusion
"  Maintainer: Jeff Lanzarotta (jefflanzarotta@yahoo.com)
"	  URL: http://lanzarotta.tripod.com/vim/syntax/cf.vim.zip
" Last Change: October 15, 2001
"	Usage: Since Cold Fusion has its own version of html comments,
"	       make sure that you put
"	       'let html_wrong_comments=1' in your _vimrc file.

" For version 5.x, clear all syntax items.
" For version 6.x, quit when a syntax file was already loaded.
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Use all the stuff from the original html syntax file.
if version < 600
  source <sfile>:p:h/html.vim
else
  runtime! syntax/html.vim
endif

" Tag names.
syn keyword cfTagName contained cfabort cfapplet cfapplication cfassociate
syn keyword cfTagName contained cfauthenticate cfbreak cfcache cfcol
syn keyword cfTagName contained cfcollection cfcontent cfcookie cfdirectory
syn keyword cfTagName contained cferror cfexit cffile cfform cfftp cfgrid
syn keyword cfTagName contained cfgridcolumn cfgridrow cfgridupdate cfheader
syn keyword cfTagName contained cfhtmlhead cfhttp cfhttpparam
syn keyword cfTagName contained cfif cfelseif cfelse
syn keyword cfTagName contained cfinclude cfindex cfinput cfinsert
syn keyword cfTagName contained cfldap cflocation cflock cfloop cfmail
syn keyword cfTagName contained cfmodule cfobject cfoutput cfparam cfpop
syn keyword cfTagName contained cfprocparam cfprocresult cfquery cfregistry
syn keyword cfTagName contained cfreport cfschedule cfscript cfsearch cfselect
syn keyword cfTagName contained cfset cfsetting cfslider cfstoredproc
syn keyword cfTagName contained cfswitch cfcase cfdefaultcase
syn keyword cfTagName contained cftable cftextinput cfthrow cftransaction
syn keyword cfTagName contained cftree cftreeitem
syn keyword cfTagName contained cftry cfcatch
syn keyword cfTagName contained cfupdate cfwddx

" Legal arguments.
syn keyword cfArg contained accept action addnewline addtoken agentname align
syn keyword cfArg contained appendkey applicationtimeout attachmentpath
syn keyword cfArg contained attributecollection attributes basetag bgcolor
syn keyword cfArg contained blockfactor body bold border branch cachedafter
syn keyword cfArg contained cachedwithin cc cfsqltype checked class clientmanagement
syn keyword cfArg contained clientstorage colheaderalign colheaderbold colheaderfont
syn keyword cfArg contained colheaderfontsize colheaderitalic colheaders collection
syn keyword cfArg contained colspacing columns completepath connection context
syn keyword cfArg contained criteria custom1 custom2 data dataalign datacollection
syn keyword cfArg contained datasource dbname dbserver dbtype dbvarname debug default
syn keyword cfArg contained delete deletebutton deletefile delimiter destination detail
syn keyword cfArg contained directory display dn domain enablecab enablecfoutputonly
syn keyword cfArg contained enctype enddate endtime entry errorcode expand expires
syn keyword cfArg contained expireurl expression extendedinfo extensions external
syn keyword cfArg contained file filefield filter font fontsize formfields formula
syn keyword cfArg contained from grid griddataalign gridlines groovecolor group header
syn keyword cfArg contained headeralign headerbold headerfont headerfontsize headeritalic
syn keyword cfArg contained headerlines height highlighthref href hrefkey hscroll hspace
syn keyword cfArg contained htmltable img imgopen imgstyle index input insert insertbutton
syn keyword cfArg contained interval isolation italic key keyonly label language mailerid
syn keyword cfArg contained mailto maxlength maxrows message messagenumber method
syn keyword cfArg contained mimeattach mode multiple name namecomplict newdirectory
syn keyword cfArg contained notsupported null numberformat onerror onsubmit onvalidate
syn keyword cfArg contained operation orderby output parrent passthrough password path
syn keyword cfArg contained picturebar port procedure protocol provider providerdsn
syn keyword cfArg contained proxybypass proxyserver publish query queryasroot range
syn keyword cfArg contained recurse refreshlabel report requesttimeout required reset
syn keyword cfArg contained resoleurl resultset retrycount returncode rowheaderalign
syn keyword cfArg contained rowheaderbold rowheaderfont rowheaderfontsize rowheaderitalic
syn keyword cfArg contained rowheaders rowheaderwidth rowheight scale scope secure
syn keyword cfArg contained securitycontext select selectcolor selected selectmode server
syn keyword cfArg contained sessionmanagement sessiontimeout setclientcookies setcookie
syn keyword cfArg contained showdebugoutput showerror size sort sortascendingbutton
syn keyword cfArg contained sortdescendingbutton source sql start startdate startrow starttime
syn keyword cfArg contained step stoponerror subject tablename tableowner tablequalifier
syn keyword cfArg contained target task template text textcolor textqualifier
syn keyword cfArg contained throwonfailure throwontimeout timeout title to toplevelvariable
syn keyword cfArg contained type url urlpath username usetimezoneinfo validate value
syn keyword cfArg contained variable vscroll vspace width

" Cold Fusion Functions.
syn keyword cfFunctionName contained Abs ArrayAppend ArrayAvg ArrayClear ArrayDeleteAt
syn keyword cfFunctionName contained ArrayInsertAt ArrayIsEmpty ArrayLen ArrayMax
syn keyword cfFunctionName contained ArrayMin ArrayNew ArrayPrepend ArrayResize ArraySet
syn keyword cfFunctionName contained ArraySort ArraySum ArraySwap ArrayToList Asc Atn
syn keyword cfFunctionName contained BitAnd BitMaskClear BitMaskRead BitMaskSet BitNot
syn keyword cfFunctionName contained BitOr BitSHLN BitSHRN BitXor CJustify Ceiling Chr
syn keyword cfFunctionName contained Compare CompareNoCase Cos CreateDate CreateDateTime
syn keyword cfFunctionName contained CreateODBCDate CreateODBCDateTime CreateODBCTime
syn keyword cfFunctionName contained CreateTime CreateTimeSpan DE DateAdd DateCompare DateDiff
syn keyword cfFunctionName contained DateFormat DatePart Day DayOfWeek DayOfWeekAsString
syn keyword cfFunctionName contained DayOfYear DaysInMonth DaysInYear DecimalFormat DecrementValue
syn keyword cfFunctionName contained Decrypt DeleteClientVariable DirectoryExists DollarFormat
syn keyword cfFunctionName contained Encrypt Evaluate Exp ExpandPath FileExists Find FindNoCase
syn keyword cfFunctionName contained FindOneOf FirstDayOfMonth Fix FormatBaseN GetBaseTagData
syn keyword cfFunctionName contained GetBaseTagList GetClientVariablesList GetDirectoryFromPath
syn keyword cfFunctionName contained GetFileFromPath GetLocale GetTempDirectory GetTempFile
syn keyword cfFunctionName contained GetTemplatePath GetTickCount GetToken HTMLCodeFormat
syn keyword cfFunctionName contained HTMLEditFormat Hour IIf IncrementValue InputBaseN Insert
syn keyword cfFunctionName contained Int IsArray IsAuthenticated IsAuthorized IsBoolean IsDate
syn keyword cfFunctionName contained IsDebugMode IsDefined IsLeapYear IsNumeric IsNumericDate
syn keyword cfFunctionName contained IsQuery IsSimpleValue IsStruct LCase LJustify LSCurrencyFormat
syn keyword cfFunctionName contained LSDateFormat LSIsCurrency LSIsDate LSIsNumeric LSNumberFormat
syn keyword cfFunctionName contained LSParseCurrency LSParseDateTime LSParseNumber LSTimeFormat
syn keyword cfFunctionName contained LTrim Left Len ListAppend ListChangeDelims ListContains
syn keyword cfFunctionName contained ListContainsNoCase ListDeleteAt ListFind ListFindNoCase ListFirst
syn keyword cfFunctionName contained ListGetAt ListInsertAt ListLast ListLen ListPrepend ListRest
syn keyword cfFunctionName contained ListSetAt ListToArray Log Log10 Max Mid Min Minute Month
syn keyword cfFunctionName contained MonthAsString Now NumberFormat ParagraphFormat ParameterExists
syn keyword cfFunctionName contained ParseDateTime Pi PreserveSingleQuotes Quarter QueryAddRow
syn keyword cfFunctionName contained QueryNew QuerySetCell QuotedValueList REFind REFindNoCase
syn keyword cfFunctionName contained REReplace REReplaceNoCase RJustify RTrim Rand RandRange
syn keyword cfFunctionName contained Randomize RemoveChars RepeatString Replace ReplaceList
syn keyword cfFunctionName contained ReplaceNoCase Reverse Right Round Second SetLocale SetVariable
syn keyword cfFunctionName contained Sgn Sin SpanExcluding SpanIncluding Sqr StripCR StructClear
syn keyword cfFunctionName contained StructCopy StructCount StructDelete StructFind StructInsert
syn keyword cfFunctionName contained StructIsEmpty StructKeyExists StructNew StructUpdate Tan
syn keyword cfFunctionName contained TimeFormat Trim UCase URLEncodedFormat Val ValueList Week
syn keyword cfFunctionName contained WriteOutput Year YesNoFormat

syn cluster htmlTagNameCluster add=cfTagName
syn cluster htmlArgCluster add=cfArg,cfFunctionName

syn region cfFunctionRegion start='#' end='#' contains=cfFunctionName

" Define the default highlighting.
" For version 5.x and earlier, only when not done already.
" For version 5.8 and later, only when and item doesn't have highlighting yet.
if version >= 508 || !exists("did_cf_syn_inits")
  if version < 508
    let did_cf_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink cfTagName Statement
  HiLink cfArg Type
  HiLink cfFunctionName Function

  delcommand HiLink
endif

let b:current_syntax = "cf"

" vim: ts=8 sw=2
