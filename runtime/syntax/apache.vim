" Vim syntax file
" This is a GENERATED FILE. Please always refer to source file at the URI below.
" Language: Apache configuration (httpd.conf, srm.conf, access.conf, .htaccess)
" Maintainer: David Ne\v{c}as (Yeti) <yeti@physics.muni.cz>
" Last Change: 2002-10-15
" URL: http://trific.ath.cx/Ftp/vim/syntax/apache.vim
" Note: define apache_version to your Apache version, e.g. "1.3", "2", "2.0.39"

" Setup
if version >= 600
	if exists("b:current_syntax")
		finish
	endif
else
	syntax clear
endif

if exists("apache_version")
	let s:av = apache_version
else
	let s:av = "1.3"
endif
let s:av = substitute(s:av, "[^.0-9]", "", "g")
let s:av = substitute(s:av, "^\\d\\+$", "\\0.999", "")
let s:av = substitute(s:av, "^\\d\\+\\.\\d\\+$", "\\0.999", "")
let s:av = substitute(s:av, "\\<\\d\\>", "0\\0", "g")
let s:av = substitute(s:av, "\\<\\d\\d\\>", "0\\0", "g")
let s:av = substitute(s:av, "[.]", "", "g")

syn case ignore

" Base constructs
syn match apacheComment "^\s*#.*$" contains=apacheFixme
if s:av >= "002000000"
	syn match apacheUserID "#-\?\d\+\>"
endif
syn case match
syn keyword apacheFixme FIXME TODO XXX NOT
syn case ignore
syn match apacheAnything "\s[^>]*" contained
syn match apacheError "\w\+" contained
syn region apacheString start=+"+ end=+"+ skip=+\\\\\|\\\"+

" Core and mpm
syn keyword apacheDeclaration AccessFileName AddDefaultCharset AllowOverride AuthName AuthType ContentDigest DefaultType DocumentRoot ErrorDocument ErrorLog HostNameLookups IdentityCheck Include KeepAlive KeepAliveTimeout LimitRequestBody LimitRequestFields LimitRequestFieldsize LimitRequestLine LogLevel MaxKeepAliveRequests NameVirtualHost Options Require RLimitCPU RLimitMEM RLimitNPROC Satisfy ScriptInterpreterSource ServerAdmin ServerAlias ServerName ServerPath ServerRoot ServerSignature ServerTokens TimeOut UseCanonicalName
if s:av < "002000000"
	syn keyword apacheDeclaration AccessConfig AddModule BindAddress BS2000Account ClearModuleList CoreDumpDirectory Group Listen ListenBacklog LockFile MaxClients MaxRequestsPerChild MaxSpareServers MinSpareServers PidFile Port ResourceConfig ScoreBoardFile SendBufferSize ServerType StartServers ThreadsPerChild ThreadStackSize User
endif
if s:av >= "002000000"
	syn keyword apacheDeclaration AcceptPathInfo CGIMapExtension EnableMMAP FileETag ForceType LimitXMLRequestBody SetHandler SetInputFilter SetOutputFilter
	syn keyword apacheOption INode MTime Size
endif
syn keyword apacheOption Any All On Off Double EMail DNS Min Minimal OS Prod ProductOnly Full
syn keyword apacheOption emerg alert crit error warn notice info debug
syn keyword apacheOption registry script inetd standalone
syn match apacheOptionOption "[+-]\?\<\(ExecCGI\|FollowSymLinks\|Includes\|IncludesNoExec\|Indexes\|MultiViews\|SymLinksIfOwnerMatch\)\>"
syn keyword apacheOption user group valid-user
syn case match
syn keyword apacheMethodOption GET POST PUT DELETE CONNECT OPTIONS TRACE PATCH PROPFIND PROPPATCH MKCOL COPY MOVE LOCK UNLOCK contained
syn case ignore
syn match apacheSection "<\/\=\(Directory\|DirectoryMatch\|Files\|FilesMatch\|IfModule\|IfDefine\|Location\|LocationMatch\|VirtualHost\)\+.*>" contains=apacheAnything
syn match apacheLimitSection "<\/\=\(Limit\|LimitExcept\)\+.*>" contains=apacheLimitSectionKeyword,apacheMethodOption,apacheError
syn keyword apacheLimitSectionKeyword Limit LimitExcept contained
syn match apacheAuthType "AuthType\s.*$" contains=apacheAuthTypeValue
syn keyword apacheAuthTypeValue Basic Digest
syn match apacheAllowOverride "AllowOverride\s.*$" contains=apacheAllowOverrideValue,apacheComment
syn keyword apacheAllowOverrideValue AuthConfig FileInfo Indexes Limit Options contained
if s:av >= "002000000"
	syn keyword apacheDeclaration CoreDumpDirectory Group Listen ListenBacklog LockFile MaxClients MaxMemFree MaxRequestsPerChild MaxSpareThreads MaxSpareThreadsPerChild MinSpareThreads NumServers PidFile ScoreBoardFile SendBufferSize ServerLimit StartServers StartThreads ThreadLimit ThreadsPerChild User
	syn keyword apacheDeclaration MaxThreads ThreadStackSize
	syn keyword apacheDeclaration AssignUserId ChildPerUserId
	syn keyword apacheDeclaration AcceptMutex MaxSpareServers MinSpareServers
	syn keyword apacheOption flock fcntl sysvsem pthread
endif

" Modules
syn match apacheAllowDeny "Allow\s\+from.*$" contains=apacheAllowDenyValue,apacheComment
syn match apacheAllowDeny "Deny\s\+from.*$" contains=apacheAllowDenyValue,apacheComment
syn keyword apacheAllowDenyValue All None contained
syn match apacheOrder "^\s*Order\s.*$" contains=apacheOrderValue,apacheComment
syn keyword apacheOrderValue Deny Allow contained
syn keyword apacheDeclaration Action Script
syn keyword apacheDeclaration Alias AliasMatch Redirect RedirectMatch RedirectTemp RedirectPermanent ScriptAlias ScriptAliasMatch
syn keyword apacheOption permanent temp seeother gone
syn keyword apacheDeclaration AuthAuthoritative AuthGroupFile AuthUserFile
syn keyword apacheDeclaration Anonymous Anonymous_Authoritative Anonymous_LogEmail Anonymous_MustGiveEmail Anonymous_NoUserID Anonymous_VerifyEmail
if s:av < "002000000"
	syn keyword apacheDeclaration AuthDBGroupFile AuthDBUserFile AuthDBAuthoritative
endif
syn keyword apacheDeclaration AuthDBMGroupFile AuthDBMUserFile AuthDBMAuthoritative
if s:av >= "002000000"
	syn keyword apacheDeclaration AuthDBMType
	syn keyword apacheOption default SDBM GDBM NDBM DB
endif
syn keyword apacheDeclaration AuthDigestAlgorithm AuthDigestDomain AuthDigestFile AuthDigestGroupFile AuthDigestNcCheck AuthDigestNonceFormat AuthDigestNonceLifetime AuthDigestQop
syn keyword apacheOption none auth auth-int MD5 MD5-sess
if s:av >= "002000000"
	syn keyword apacheDeclaration AuthLDAPAuthoritative AuthLDAPBindON AuthLDAPBindPassword AuthLDAPCompareDNOnServer AuthLDAPDereferenceAliases AuthLDAPEnabled AuthLDAPFrontPageHack AuthLDAPGroupAttribute AuthLDAPGroupAttributeIsDN AuthLDAPRemoteUserIsDN AuthLDAPStartTLS AuthLDAPUrl
	syn keyword apacheOption always never searching finding
endif
if s:av < "002000000"
	syn keyword apacheDeclaration FancyIndexing
endif
syn keyword apacheDeclaration AddAlt AddAltByEncoding AddAltByType AddDescription AddIcon AddIconByEncoding AddIconByType DefaultIcon HeaderName IndexIgnore IndexOptions IndexOrderDefault ReadmeName
syn keyword apacheOption DescriptionWidth FancyIndexing FoldersFirst IconHeight IconsAreLinks IconWidth NameWidth ScanHTMLTitles SuppressColumnSorting SuppressDescription SuppressHTMLPreamble SuppressLastModified SuppressSize TrackModified
syn keyword apacheOption Ascending Descending Name Date Size Description
if s:av >= "002000000"
	syn keyword apacheOption HTMLTable SupressIcon SupressRules VersionSort
endif
if s:av < "002000000"
	syn keyword apacheDeclaration BrowserMatch BrowserMatchNoCase
endif
if s:av >= "002000000"
	syn keyword apacheDeclaration CacheDefaultExpire CacheEnable CacheForceCompletion CacheIgnoreCacheControl CacheIgnoreNoLastMod CacheLastModifiedFactor CacheMaxExpire CacheMaxStreamingBuffer
endif
syn keyword apacheDeclaration MetaFiles MetaDir MetaSuffix
syn keyword apacheDeclaration ScriptLog ScriptLogLength ScriptLogBuffer
if s:av >= "002000000"
	syn keyword apacheDeclaration ScriptStock
	syn keyword apacheDeclaration CharsetDefault CharsetOptions CharsetSourceEnc
	syn keyword apacheOption DebugLevel ImplicitAdd NoImplicitAdd
endif
syn keyword apacheDeclaration Dav DavDepthInfinity DavLockDB DavMinTimeout
if s:av < "002000000"
	syn keyword apacheDeclaration Define
end
if s:av >= "002000000"
	syn keyword apacheDeclaration DeflateBufferSize DeflateFilterNote DeflateMemLevel DeflateWindowSize
endif
if s:av < "002000000"
	syn keyword apacheDeclaration AuthDigestFile
endif
syn keyword apacheDeclaration DirectoryIndex
if s:av >= "002000000"
	syn keyword apacheDeclaration ProtocolEcho
endif
syn keyword apacheDeclaration PassEnv SetEnv UnsetEnv
syn keyword apacheDeclaration Example
syn keyword apacheDeclaration ExpiresActive ExpiresByType ExpiresDefault
if s:av >= "002000000"
	syn keyword apacheDeclaration ExtFilterDefine ExtFilterOptions
	syn keyword apacheOption PreservesContentLength DebugLevel LogStderr NoLogStderr
	syn keyword apacheDeclaration CacheFile MMapFile
endif
syn keyword apacheDeclaration Header
if s:av >= "002000000"
	syn keyword apacheDeclaration RequestHeader
endif
syn keyword apacheOption set unset append add
syn keyword apacheDeclaration ImapMenu ImapDefault ImapBase
syn keyword apacheOption none formatted semiformatted unformatted
syn keyword apacheOption nocontent referer error map
syn keyword apacheDeclaration XBitHack
if s:av >= "002000000"
	syn keyword apacheDeclaration SSIEndTag SSIErrorMsg SSIStartTag SSITimeFormat SSIUndefinedEcho
endif
syn keyword apacheOption on off full
syn keyword apacheDeclaration AddModuleInfo
syn keyword apacheDeclaration ISAPIReadAheadBuffer ISAPILogNotSupported ISAPIAppendLogToErrors ISAPIAppendLogToQuery
if s:av >= "002000000"
	syn keyword apacheDeclaration ISAPICacheFile ISAIPFakeAsync
	syn keyword apacheDeclaration LDAPCacheEntries LDAPCacheTTL LDAPCertDBPath LDAPOpCacheEntries LDAPOpCacheTTL LDAPSharedCacheSize
endif
if s:av < "002000000"
	syn keyword apacheDeclaration AgentLog
endif
syn keyword apacheDeclaration CookieLog CustomLog LogFormat TransferLog
if s:av < "002000000"
	syn keyword apacheDeclaration RefererIgnore RefererLog
endif
if s:av >= "002000000"
endif
syn keyword apacheDeclaration AddCharset AddEncoding AddHandler AddLanguage AddType DefaultLanguage RemoveEncoding RemoveHandler RemoveType TypesConfig
if s:av < "002000000"
	syn keyword apacheDeclaration ForceType SetHandler
endif
if s:av >= "002000000"
	syn keyword apacheDeclaration AddInputFilter AddOutputFilter ModMimeUsePathInfo MultiviewsMatch RemoveInputFilter RemoveOutputFilter
endif
syn keyword apacheDeclaration MimeMagicFile
syn keyword apacheDeclaration MMapFile
syn keyword apacheDeclaration CacheNegotiatedDocs LanguagePriority
if s:av >= "002000000"
	syn keyword apacheDeclaration ForceLanguagePriority
endif
syn keyword apacheDeclaration PerlModule PerlRequire PerlTaintCheck PerlWarn
syn keyword apacheDeclaration PerlSetVar PerlSetEnv PerlPassEnv PerlSetupEnv
syn keyword apacheDeclaration PerlInitHandler PerlPostReadRequestHandler PerlHeaderParserHandler
syn keyword apacheDeclaration PerlTransHandler PerlAccessHandler PerlAuthenHandler PerlAuthzHandler
syn keyword apacheDeclaration PerlTypeHandler PerlFixupHandler PerlHandler PerlLogHandler
syn keyword apacheDeclaration PerlCleanupHandler PerlChildInitHandler PerlChildExitHandler
syn keyword apacheDeclaration PerlRestartHandler PerlDispatchHandler
syn keyword apacheDeclaration PerlFreshRestart PerlSendHeader
syn keyword apacheDeclaration php_value php_flag php_admin_value php_admin_flag
syn keyword apacheDeclaration AllowCONNECT NoProxy ProxyBlock ProxyDomain ProxyPass ProxyPassReverse ProxyReceiveBufferSize ProxyRemote ProxyRequests ProxyVia
if s:av < "002000000"
	syn keyword apacheDeclaration CacheRoot CacheSize CacheMaxExpire CacheDefaultExpire CacheLastModifiedFactor CacheGcInterval CacheDirLevels CacheDirLength CacheForceCompletion NoCache
	syn keyword apacheOption block
endif
if s:av >= "002000000"
	syn match apacheSection "<\/\=\(Proxy\|ProxyMatch\)\+.*>" contains=apacheAnything
	syn keyword apacheDeclaration ProxyErrorOverride ProxyIOBufferSize ProxyMaxForwards ProxyPreserveHost ProxyRemoteMatch ProxyTimeout
endif
syn keyword apacheDeclaration RewriteEngine RewriteOptions RewriteLog RewriteLogLevel RewriteLock RewriteMap RewriteBase RewriteCond RewriteRule
syn keyword apacheOption inherit
if s:av < "002000000"
	syn keyword apacheDeclaration RoamingAlias
endif
syn keyword apacheDeclaration BrowserMatch BrowserMatchNoCase SetEnvIf SetEnvIfNoCase
syn keyword apacheDeclaration LoadFile LoadModule
syn keyword apacheDeclaration CheckSpelling
syn keyword apacheDeclaration SSLCACertificateFile SSLCACertificatePath SSLCARevocationFile SSLCARevocationPath SSLCertificateChainFile SSLCertificateFile SSLCertificateKeyFile SSLCipherSuite SSLEngine SSLMutex SSLOptions SSLPassPhraseDialog SSLProtocol SSLRandomSeed SSLRequire SSLRequireSSL SSLSessionCache SSLSessionCacheTimeout SSLVerifyClient SSLVerifyDepth
if s:av < "002000000"
	syn keyword apacheDeclaration SSLLog SSLLogLevel
endif
if s:av >= "002000000"
	syn keyword apacheDeclaration SSLProxyCACertificateFile SSLProxyCACertificatePath SSLProxyCARevocationFile SSLProxyCARevocationPath SSLProxyCipherSuite SSLProxyEngine SSLProxyMachineCertificateFile SSLProxyMachineCertificatePath SSLProxyProtocol SSLProxyVerify SSLProxyVerifyDepth
endif
syn match apacheOption "[+-]\?\<\(StdEnvVars\|CompatEnvVars\|ExportCertData\|FakeBasicAuth\|StrictRequire\|OptRenegotiate\)\>"
syn keyword apacheOption builtin sem
syn match apacheOption "\(file\|exec\|egd\|dbm\|shm\):"
if s:av < "002000000"
	syn match apacheOption "[+-]\?\<\(SSLv2\|SSLv3\|TLSv1\)\>"
endif
if s:av >= "002000000"
	syn match apacheOption "[+-]\?\<\(SSLv2\|SSLv3\|TLSv1\|kRSA\|kHDr\|kDHd\|kEDH\|aNULL\|aRSA\|aDSS\|aRH\|eNULL\|DES\|3DES\|RC2\|RC4\|IDEA\|MD5\|SHA1\|SHA\|EXP\|EXPORT40\|EXPORT56\|LOW\|MEDIUM\|HIGH\|RSA\|DH\|EDH\|ADH\|DSS\|NULL\)\>"
endif
syn keyword apacheOption optional require optional_no_ca
syn keyword apacheDeclaration ExtendedStatus
if s:av >= "002000000"
	syn keyword apacheDeclaration SuexecUserGroup
endif
syn keyword apacheDeclaration UserDir
syn keyword apacheDeclaration CookieExpires CookieName CookieTracking
if s:av >= "002000000"
	syn keyword apacheDeclaration CookieDomain CookieStyle
	syn keyword apacheOption Netscape Cookie Cookie2 RFC2109 RFC2965
endif
syn keyword apacheDeclaration VirtualDocumentRoot VirtualDocumentRootIP VirtualScriptAlias VirtualScriptAliasIP

" Define the default highlighting
if version >= 508 || !exists("did_apache_syntax_inits")
	if version < 508
		let did_apache_syntax_inits = 1
		command -nargs=+ HiLink hi link <args>
	else
		command -nargs=+ HiLink hi def link <args>
	endif

	HiLink apacheAllowOverride apacheDeclaration
	HiLink apacheAllowOverrideValue apacheOption
	HiLink apacheAuthType apacheDeclaration
	HiLink apacheAuthTypeValue apacheOption
	HiLink apacheOptionOption apacheOption
	HiLink apacheDeclaration Function
	HiLink apacheAnything apacheOption
	HiLink apacheOption Number
	HiLink apacheComment Comment
	HiLink apacheFixme Todo
	HiLink apacheLimitSectionKeyword apacheLimitSection
	HiLink apacheLimitSection apacheSection
	HiLink apacheSection Label
	HiLink apacheMethodOption Type
	HiLink apacheAllowDeny Include
	HiLink apacheAllowDenyValue Identifier
	HiLink apacheOrder Special
	HiLink apacheOrderValue String
	HiLink apacheString String
	HiLink apacheError Error
	HiLink apacheUserID Number

	delcommand HiLink
endif

let b:current_syntax = "apache"
