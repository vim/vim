" Lynx syntax file
" Filename:	lynx.vim
" Language:	Lynx configuration file ( lynx.cfg )
" Maintainer:	Doug Kearns <djkea2@gus.gscit.monash.edu.au>
" URL:		http://gus.gscit.monash.edu.au/~djkea2/vim/syntax/lynx.vim
" Last Change:	2004 Nov 27

" TODO: more intelligent and complete argument highlighting

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn match   lynxLeadingWS  "^\s*" transparent nextgroup=lynxOption

syn match   lynxComment    "\(^\|\s\+\)#.*$" contains=lynxTodo

syn keyword lynxTodo       TODO NOTE FIXME XXX contained

syn match   lynxDelimiter  ":" contained nextgroup=lynxBoolean,lynxNumber

syn case ignore
syn keyword lynxBoolean    TRUE FALSE contained
syn case match

syn match   lynxNumber     "-\=\<\d\+\>" contained

syn case ignore
syn keyword lynxOption ACCEPT_ALL_COOKIES ALERTSECS ALWAYS_RESUBMIT_POSTS ALWAYS_TRUSTED_EXEC ASSUME_CHARSET ASSUMED_COLOR  contained nextgroup=lynxDelimiter
syn keyword lynxOption ASSUMED_DOC_CHARSET_CHOICE ASSUME_LOCAL_CHARSET ASSUME_UNREC_CHARSET AUTO_UNCACHE_DIRLISTS	    contained nextgroup=lynxDelimiter
syn keyword lynxOption BIBP_BIBHOST BIBP_GLOBAL_SERVER BLOCK_MULTI_BOOKMARKS BOLD_H1 BOLD_HEADERS BOLD_NAME_ANCHORS	    contained nextgroup=lynxDelimiter
syn keyword lynxOption CASE_SENSITIVE_ALWAYS_ON CHARACTER_SET CHARSETS_DIRECTORY CHARSET_SWITCH_RULES CHECKMAIL		    contained nextgroup=lynxDelimiter
syn keyword lynxOption COLLAPSE_BR_TAGS COLOR CONNECT_TIMEOUT COOKIE_ACCEPT_DOMAINS COOKIE_FILE				    contained nextgroup=lynxDelimiter
syn keyword lynxOption COOKIE_LOOSE_INVALID_DOMAINS COOKIE_QUERY_INVALID_DOMAINS COOKIE_REJECT_DOMAINS COOKIE_SAVE_FILE     contained nextgroup=lynxDelimiter
syn keyword lynxOption COOKIE_STRICT_INVALID_DOMAINS CSO_PROXY CSWING_PATH DEFAULT_BOOKMARK_FILE DEFAULT_CACHE_SIZE	    contained nextgroup=lynxDelimiter
syn keyword lynxOption DEFAULT_EDITOR DEFAULT_INDEX_FILE DEFAULT_KEYPAD_MODE DEFAULT_KEYPAD_MODE_IS_NUMBERS_AS_ARROWS	    contained nextgroup=lynxDelimiter
syn keyword lynxOption DEFAULT_USER_MODE DEFAULT_VIRTUAL_MEMORY_SIZE DIRED_MENU DISPLAY_CHARSET_CHOICE DOWNLOADER	    contained nextgroup=lynxDelimiter
syn keyword lynxOption EMACS_KEYS_ALWAYS_ON ENABLE_LYNXRC ENABLE_SCROLLBACK EXTERNAL FINGER_PROXY FOCUS_WINDOW		    contained nextgroup=lynxDelimiter
syn keyword lynxOption FORCE_8BIT_TOUPPER FORCE_EMPTY_HREFLESS_A FORCE_SSL_COOKIES_SECURE FORMS_OPTIONS FTP_PASSIVE	    contained nextgroup=lynxDelimiter
syn keyword lynxOption FTP_PROXY GLOBAL_EXTENSION_MAP GLOBAL_MAILCAP GOPHER_PROXY GOTOBUFFER HELPFILE HIDDEN_LINK_MARKER    contained nextgroup=lynxDelimiter
syn keyword lynxOption HISTORICAL_COMMENTS HTMLSRC_ATTRNAME_XFORM HTMLSRC_TAGNAME_XFORM HTTP_PROXY HTTPS_PROXY INCLUDE	    contained nextgroup=lynxDelimiter
syn keyword lynxOption INFOSECS JUMPBUFFER JUMPFILE JUMP_PROMPT JUSTIFY JUSTIFY_MAX_VOID_PERCENT KEYBOARD_LAYOUT KEYMAP     contained nextgroup=lynxDelimiter
syn keyword lynxOption LEFTARROW_IN_TEXTFIELD_PROMPT LIST_FORMAT LIST_NEWS_DATES LIST_NEWS_NUMBERS LOCAL_DOMAIN		    contained nextgroup=lynxDelimiter
syn keyword lynxOption LOCAL_EXECUTION_LINKS_ALWAYS_ON LOCAL_EXECUTION_LINKS_ON_BUT_NOT_REMOTE LOCALHOST_ALIAS		    contained nextgroup=lynxDelimiter
syn keyword lynxOption LYNXCGI_DOCUMENT_ROOT LYNXCGI_ENVIRONMENT LYNX_HOST_NAME LYNX_SIG_FILE MAIL_ADRS			    contained nextgroup=lynxDelimiter
syn keyword lynxOption MAIL_SYSTEM_ERROR_LOGGING MAKE_LINKS_FOR_ALL_IMAGES MAKE_PSEUDO_ALTS_FOR_INLINES MESSAGESECS	    contained nextgroup=lynxDelimiter
syn keyword lynxOption MINIMAL_COMMENTS MULTI_BOOKMARK_SUPPORT NCR_IN_BOOKMARKS NEWS_CHUNK_SIZE NEWS_MAX_CHUNK		    contained nextgroup=lynxDelimiter
syn keyword lynxOption NEWS_POSTING NEWSPOST_PROXY NEWS_PROXY NEWSREPLY_PROXY NNTP_PROXY NNTPSERVER NO_DOT_FILES	    contained nextgroup=lynxDelimiter
syn keyword lynxOption NO_FILE_REFERER NO_FORCED_CORE_DUMP NO_FROM_HEADER NO_ISMAP_IF_USEMAP NONRESTARTING_SIGWINCH	    contained nextgroup=lynxDelimiter
syn keyword lynxOption NO_PROXY NO_REFERER_HEADER NO_TABLE_CENTER OUTGOING_MAIL_CHARSET PARTIAL PARTIAL_THRES		    contained nextgroup=lynxDelimiter
syn keyword lynxOption PERSISTENT_COOKIES PERSONAL_EXTENSION_MAP PERSONAL_MAILCAP PREFERRED_CHARSET PREFERRED_LANGUAGE	    contained nextgroup=lynxDelimiter
syn keyword lynxOption PREPEND_BASE_TO_SOURCE PREPEND_CHARSET_TO_SOURCE PRETTYSRC PRETTYSRC_SPEC			    contained nextgroup=lynxDelimiter
syn keyword lynxOption PRETTYSRC_VIEW_NO_ANCHOR_NUMBERING PRINTER QUIT_DEFAULT_YES REFERER_WITH_QUERY REUSE_TEMPFILES	    contained nextgroup=lynxDelimiter
syn keyword lynxOption RULE RULESFILE SAVE_SPACE SCAN_FOR_BURIED_NEWS_REFS SCROLLBAR SCROLLBAR_ARROW SEEK_FRAG_AREA_IN_CUR  contained nextgroup=lynxDelimiter
syn keyword lynxOption SEEK_FRAG_MAP_IN_CUR SET_COOKIES SHOW_CURSOR SHOW_KB_RATE SNEWSPOST_PROXY SNEWS_PROXY		    contained nextgroup=lynxDelimiter
syn keyword lynxOption SNEWSREPLY_PROXY SOFT_DQUOTES SOURCE_CACHE SOURCE_CACHE_FOR_ABORTED STARTFILE STRIP_DOTDOT_URLS	    contained nextgroup=lynxDelimiter
syn keyword lynxOption SUBSTITUTE_UNDERSCORES SUFFIX SUFFIX_ORDER SYSTEM_EDITOR SYSTEM_MAIL SYSTEM_MAIL_FLAGS TAGSOUP	    contained nextgroup=lynxDelimiter
syn keyword lynxOption TEXTFIELDS_NEED_ACTIVATION TIMEOUT TRIM_INPUT_FIELDS TRUSTED_EXEC TRUSTED_LYNXCGI UPLOADER	    contained nextgroup=lynxDelimiter
syn keyword lynxOption URL_DOMAIN_PREFIXES URL_DOMAIN_SUFFIXES USE_FIXED_RECORDS USE_MOUSE USE_SELECT_POPUPS VERBOSE_IMAGES contained nextgroup=lynxDelimiter
syn keyword lynxOption VIEWER VI_KEYS_ALWAYS_ON WAIS_PROXY XLOADIMAGE_COMMAND						    contained nextgroup=lynxDelimiter
syn case match

" NOTE: set this if you want the cfg2html.pl formatting directives to be highlighted
if exists("lynx_formatting_directives")
  syn match lynxFormatDir  "^\.\(h1\|h2\)\s.*$"
  syn match lynxFormatDir  "^\.\(ex\|nf\)\(\s\+\d\+\)\=$"
  syn match lynxFormatDir  "^\.fi$"
endif

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_lynx_syn_inits")
  if version < 508
    let did_lynx_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink lynxBoolean    Boolean
  HiLink lynxComment    Comment
  HiLink lynxDelimiter  Special
  HiLink lynxFormatDir  Special
  HiLink lynxNumber     Number
  HiLink lynxOption     Identifier
  HiLink lynxTodo       Todo

  delcommand HiLink
endif

let b:current_syntax = "lynx"

" vim: ts=8
