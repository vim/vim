" Vim syntax file
" Language:         login.defs(5) configuration file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword logindefsTodo       contained TODO FIXME XXX NOTE

syn region  logindefsComment    display oneline start='^\s*#' end='$'
                                \ contains=logindefsTodo,@Spell

syn match   logindefsString     contained '[[:graph:]]\+'

syn match   logindefsPath       contained '[[:graph:]]\+'

syn match   logindefsPaths      contained '[[:graph:]]\+'
                                \ nextgroup=logindefsPathDelim

syn match   logindefsPathDelim  contained ':' nextgroup=logindefsPaths

syn keyword logindefsBoolean    contained yes no

syn match   logindefsDecimal    contained '\<\d\+\>'

syn match   logindefsOctal      contained display '\<0\o\+\>'
                                \ contains=logindefsOctalZero
syn match   logindefsOctalZero  contained display '\<0'
syn match   logindefsOctalError contained display '\<0\o*[89]\d*\>'

syn match   logindefsHex        contained display '\<0x\x\+\>'

syn cluster logindefsNumber     contains=logindefsDecimal,logindefsOctal,
                                \ logindefsOctalError,logindefsHex

syn match   logindefsBegin      display '^'
                                \ nextgroup=logindefsKeyword,logindefsComment
                                \ skipwhite

syn keyword logindefsKeyword    contained CHFN_AUTH CLOSE_SESSIONS CREATE_HOME
                                \ DEFAULT_HOME FAILLOG_ENAB LASTLOG_ENAB
                                \ LOG_OK_LOGINS LOG_UNKFAIL_ENAB MAIL_CHECK_ENAB
                                \ MD5_CRYPT_ENAB OBSCURE_CHECKS_ENAB
                                \ PASS_ALWAYS_WARN PORTTIME_CHECKS_ENAB
                                \ QUOTAS_ENAB SU_WHEEL_ONLY SYSLOG_SG_ENAB
                                \ SYSLOG_SU_ENAB USERGROUPS_ENAB
                                \ nextgroup=logindefsBoolean skipwhite

syn keyword logindefsKeyword    contained CHFN_RESTRICT CONSOLE CONSOLE_GROUPS
                                \ ENV_TZ ENV_HZ FAKE_SHELL SU_NAME LOGIN_STRING
                                \ NOLOGIN_STR TTYGROUP USERDEL_CMD
                                \ nextgroup=logindefsString skipwhite

syn keyword logindefsKeyword    contained ENVIRON_FILE FTMP_FILE HUSHLOGIN_FILE
                                \ ISSUE_FILE MAIL_DIR MAIL_FILE NOLOGINS_FILE
                                \ NOLOGINS_FILE TTYTYPE_FILE QMAIL_DIR
                                \ SULOG_FILE
                                \ nextgroup=logindefsPath skipwhite

syn keyword logindefsKeyword    contained CRACKLIB_DICTPATH ENV_PATH
                                \ ENV_ROOTPATH ENV_SUPATH MOTD_FILE
                                \ nextgroup=logindefsPaths skipwhite

syn keyword logindefsKeyword    contained ERASECHAR FAIL_DELAY GETPASS_ASTERISKS
                                \ GID_MAX GID_MIN KILLCHAR LOGIN_RETRIES
                                \ LOGIN_TIMEOUT PASS_CHANGE_TRIES PASS_MAX_DAYS
                                \ PASS_MAX_LEN PASS_MIN_DAYS PASS_MIN_LEN
                                \ PASS_WARN_AGE TTYPERM UID_MAX UID_MIN ULIMIT
                                \ UMASK
                                \ nextgroup=@logindefsNumber skipwhite

hi def link logindefsTodo       Todo
hi def link logindefsComment    Comment
hi def link logindefsString     String
hi def link logindefsPath       String
hi def link logindefsPaths      logindefsPath
hi def link logindefsPathDelim  Delimiter
hi def link logindefsBoolean    Boolean
hi def link logindefsDecimal    Number
hi def link logindefsOctal      Number
hi def link logindefsOctalZero  PreProc
hi def link logindefsOctalError Error
hi def link logindefsHex        Number
hi def link logindefsKeyword    Keyword

let b:current_syntax = "logindefs"

let &cpo = s:cpo_save
unlet s:cpo_save
