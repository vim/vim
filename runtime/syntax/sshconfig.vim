" Vim syntax file
" Language: OpenSSH client configuration file (ssh_config)
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
syn match sshconfigComment "#.*$" contains=sshconfigTodo
syn keyword sshconfigTodo TODO FIXME NOT contained

" Constants
syn keyword sshconfigYesNo yes no ask
syn keyword sshconfigYesNo any auto
syn keyword sshconfigCipher aes128-cbc 3des-cbc blowfish-cbc cast128-cbc
syn keyword sshconfigCipher aes192-cbc aes256-cbc aes128-ctr aes256-ctr
syn keyword sshconfigCipher arcfour arcfour128 arcfour256 cast128-cbc
syn keyword sshconfigMAC hmac-md5 hmac-sha1 hmac-ripemd160 hmac-sha1-96
syn keyword sshconfigMAC hmac-md5-96
syn match sshconfigMAC "\<umac-64@openssh\.com\>"
syn keyword sshconfigHostKeyAlg ssh-rsa ssh-dss
syn keyword sshconfigPreferredAuth hostbased publickey password
syn keyword sshconfigPreferredAuth keyboard-interactive
syn keyword sshconfigLogLevel QUIET FATAL ERROR INFO VERBOSE
syn keyword sshconfigLogLevel DEBUG DEBUG1 DEBUG2 DEBUG3
syn keyword sshconfigSysLogFacility DAEMON USER AUTH AUTHPRIV LOCAL0 LOCAL1
syn keyword sshconfigSysLogFacility LOCAL2 LOCAL3 LOCAL4 LOCAL5 LOCAL6 LOCAL7
syn match sshconfigVar "%[rhpldun]\>"
syn match sshconfigSpecial "[*?]"
syn match sshconfigNumber "\d\+"
syn match sshconfigHostPort "\<\(\d\{1,3}\.\)\{3}\d\{1,3}\(:\d\+\)\?\>"
syn match sshconfigHostPort "\<\([-a-zA-Z0-9]\+\.\)\+[-a-zA-Z0-9]\{2,}\(:\d\+\)\?\>"
syn match sshconfigHostPort "\<\(\x\{,4}:\)\+\x\{,4}[:/]\d\+\>"

" Keywords
syn keyword sshconfigHostSect Host
syn keyword sshconfigKeyword AddressFamily
syn keyword sshconfigKeyword BatchMode BindAddress
syn keyword sshconfigKeyword ChallengeResponseAuthentication CheckHostIP
syn keyword sshconfigKeyword Cipher Ciphers ClearAllForwardings
syn keyword sshconfigKeyword Compression CompressionLevel ConnectTimeout
syn keyword sshconfigKeyword ConnectionAttempts ControlMaster
syn keyword sshconfigKeyword ControlPath DynamicForward
syn keyword sshconfigKeyword EnableSSHKeysign EscapeChar ExitOnForwardFailure
syn keyword sshconfigKeyword ForwardAgent ForwardX11
syn keyword sshconfigKeyword ForwardX11Trusted
syn keyword sshconfigKeyword GSSAPIAuthentication
syn keyword sshconfigKeyword GSSAPIDelegateCredentials GatewayPorts
syn keyword sshconfigKeyword GlobalKnownHostsFile
syn keyword sshconfigKeyword HostKeyAlgorithms HashKnownHosts
syn keyword sshconfigKeyword HostKeyAlias HostName HostbasedAuthentication
syn keyword sshconfigKeyword IdentitiesOnly IdentityFile
syn keyword sshconfigKeyword KbdInteractiveAuthentication KbdInteractiveDevices
syn keyword sshconfigKeyword LocalCommand LocalForward LogLevel
syn keyword sshconfigKeyword MACs
syn keyword sshconfigKeyword NoHostAuthenticationForLocalhost
syn keyword sshconfigKeyword NumberOfPasswordPrompts
syn keyword sshconfigKeyword PasswordAuthentication PermitLocalCommand
syn keyword sshconfigKeyword Port PreferredAuthentications Protocol
syn keyword sshconfigKeyword ProxyCommand PubkeyAuthentication
syn keyword sshconfigKeyword PermitLocalCommand
syn keyword sshconfigKeyword RSAAuthentication RemoteForward RekeyLimit
syn keyword sshconfigKeyword RhostsRSAAuthentication
syn keyword sshconfigKeyword SendEnv ServerAliveCountMax ServerAliveInterval
syn keyword sshconfigKeyword SmartcardDevice StrictHostKeyChecking
syn keyword sshconfigKeyword Tunnel TunnelDevice
syn keyword sshconfigKeyword TCPKeepAlive UsePrivilegedPort User
syn keyword sshconfigKeyword UserKnownHostsFile
syn keyword sshconfigKeyword VerifyHostKeyDNS VisualHostKey
syn keyword sshconfigKeyword XAuthLocation

" Define the default highlighting
if version >= 508 || !exists("did_sshconfig_syntax_inits")
  if version < 508
    let did_sshconfig_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink sshconfigComment        Comment
  HiLink sshconfigTodo           Todo
  HiLink sshconfigHostPort       sshconfigConstant
  HiLink sshconfigNumber         sshconfigConstant
  HiLink sshconfigConstant       Constant
  HiLink sshconfigYesNo          sshconfigEnum
  HiLink sshconfigCipher         sshconfigEnum
  HiLink sshconfigMAC            sshconfigEnum
  HiLink sshconfigHostKeyAlg     sshconfigEnum
  HiLink sshconfigLogLevel       sshconfigEnum
  HiLink sshconfigSysLogFacility sshconfigEnum
  HiLink sshconfigPreferredAuth  sshconfigEnum
  HiLink sshconfigVar            sshconfigEnum
  HiLink sshconfigEnum           Identifier
  HiLink sshconfigSpecial        Special
  HiLink sshconfigKeyword        Keyword
  HiLink sshconfigHostSect       Type
  delcommand HiLink
endif

let b:current_syntax = "sshconfig"
