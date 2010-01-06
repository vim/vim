" Vim syntax file
" Language: OpenSSH server configuration file (sshd_config)
" Maintainer: David Necas (Yeti) <yeti@physics.muni.cz>
" Last Change: 2009-07-09

" Setup
if version >= 600
  if exists("b:current_syntax")
    finish
  endif
else
  syntax clear
endif

if version >= 600
  setlocal iskeyword=_,-,a-z,A-Z,48-57
else
  set iskeyword=_,-,a-z,A-Z,48-57
endif

syn case ignore

" Comments
syn match sshdconfigComment "#.*$" contains=sshdconfigTodo
syn keyword sshdconfigTodo TODO FIXME NOT contained

" Constants
syn keyword sshdconfigYesNo yes no none
syn keyword sshdconfigAddressFamily any inet inet6
syn keyword sshdconfigCipher aes128-cbc 3des-cbc blowfish-cbc cast128-cbc
syn keyword sshdconfigCipher aes192-cbc aes256-cbc aes128-ctr aes256-ctr
syn keyword sshdconfigCipher arcfour arcfour128 arcfour256 cast128-cbc
syn keyword sshdconfigMAC hmac-md5 hmac-sha1 hmac-ripemd160 hmac-sha1-96
syn keyword sshdconfigMAC hmac-md5-96
syn match sshdconfigMAC "\<umac-64@openssh\.com\>"
syn keyword sshdconfigRootLogin without-password forced-commands-only
syn keyword sshdconfigLogLevel QUIET FATAL ERROR INFO VERBOSE
syn keyword sshdconfigLogLevel DEBUG DEBUG1 DEBUG2 DEBUG3
syn keyword sshdconfigSysLogFacility DAEMON USER AUTH AUTHPRIV LOCAL0 LOCAL1
syn keyword sshdconfigSysLogFacility LOCAL2 LOCAL3 LOCAL4 LOCAL5 LOCAL6 LOCAL7
syn match sshdconfigSpecial "[*?]"
syn match sshdconfigNumber "\d\+"
syn match sshdconfigHostPort "\<\(\d\{1,3}\.\)\{3}\d\{1,3}\(:\d\+\)\?\>"
syn match sshdconfigHostPort "\<\([-a-zA-Z0-9]\+\.\)\+[-a-zA-Z0-9]\{2,}\(:\d\+\)\?\>"
" FIXME: this matches quite a few things which are NOT valid IPv6 addresses
syn match sshdconfigHostPort "\<\(\x\{,4}:\)\+\x\{,4}:\d\+\>"
syn match sshdconfigTime "\<\(\d\+[sSmMhHdDwW]\)\+\>"

" Keywords
syn keyword sshdconfigMatch Host User Group Address
syn keyword sshdconfigKeyword AcceptEnv AddressFamily AllowAgentForwarding
syn keyword sshdconfigKeyword AllowGroups AllowTcpForwarding
syn keyword sshdconfigKeyword AllowUsers AuthorizedKeysFile
syn keyword sshdconfigKeyword Banner
syn keyword sshdconfigKeyword ChallengeResponseAuthentication ChrootDirectory
syn keyword sshdconfigKeyword Ciphers ClientAliveCountMax
syn keyword sshdconfigKeyword ClientAliveInterval Compression
syn keyword sshdconfigKeyword DenyGroups DenyUsers
syn keyword sshdconfigKeyword ForceCommand
syn keyword sshdconfigKeyword GatewayPorts GSSAPIAuthentication
syn keyword sshdconfigKeyword GSSAPICleanupCredentials
syn keyword sshdconfigKeyword HostbasedAuthentication HostKey
syn keyword sshdconfigKeyword IgnoreRhosts IgnoreUserKnownHosts
syn keyword sshdconfigKeyword KerberosAuthentication KerberosGetAFSToken
syn keyword sshdconfigKeyword KerberosOrLocalPasswd KerberosTicketCleanup
syn keyword sshdconfigKeyword KeyRegenerationInterval
syn keyword sshdconfigKeyword ListenAddress LoginGraceTime LogLevel
syn keyword sshdconfigKeyword MACs Match MaxAuthTries MaxSessions MaxStartups
syn keyword sshdconfigKeyword PasswordAuthentication PermitEmptyPasswords
syn keyword sshdconfigKeyword PermitRootLogin PermitOpen PermitTunnel
syn keyword sshdconfigKeyword PermitUserEnvironment PidFile Port
syn keyword sshdconfigKeyword PrintLastLog PrintMotd Protocol
syn keyword sshdconfigKeyword PubkeyAuthentication
syn keyword sshdconfigKeyword RhostsRSAAuthentication RSAAuthentication
syn keyword sshdconfigKeyword ServerKeyBits ShowPatchLevel StrictModes
syn keyword sshdconfigKeyword Subsystem SyslogFacility
syn keyword sshdconfigKeyword TCPKeepAlive
syn keyword sshdconfigKeyword UseDNS UseLogin UsePAM UsePrivilegeSeparation
syn keyword sshdconfigKeyword X11DisplayOffset X11Forwarding
syn keyword sshdconfigKeyword X11UseLocalhost XAuthLocation

" Define the default highlighting
if version >= 508 || !exists("did_sshdconfig_syntax_inits")
  if version < 508
    let did_sshdconfig_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink sshdconfigComment        Comment
  HiLink sshdconfigTodo           Todo
  HiLink sshdconfigHostPort       sshdconfigConstant
  HiLink sshdconfigTime           sshdconfigConstant
  HiLink sshdconfigNumber         sshdconfigConstant
  HiLink sshdconfigConstant       Constant
  HiLink sshdconfigYesNo          sshdconfigEnum
  HiLink sshdconfigAddressFamily  sshdconfigEnum
  HiLink sshdconfigCipher         sshdconfigEnum
  HiLink sshdconfigMAC            sshdconfigEnum
  HiLink sshdconfigRootLogin      sshdconfigEnum
  HiLink sshdconfigLogLevel       sshdconfigEnum
  HiLink sshdconfigSysLogFacility sshdconfigEnum
  HiLink sshdconfigEnum           Function
  HiLink sshdconfigSpecial        Special
  HiLink sshdconfigKeyword        Keyword
  HiLink sshdconfigMatch          Type
  delcommand HiLink
endif

let b:current_syntax = "sshdconfig"
