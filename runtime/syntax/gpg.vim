" Vim syntax file
" Language:	    GnuPG Configuration File.
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/gpg/
" Latest Revision:  2004-05-06
" arch-tag:	    602305f7-d8ae-48ef-a68f-4d54f12af70a

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Set iskeyword since we need `-' (and potentially others) in keywords.
" For version 5.x: Set it globally
" For version 6.x: Set it locally
if version >= 600
  command -nargs=1 SetIsk setlocal iskeyword=<args>
else
  command -nargs=1 SetIsk set iskeyword=<args>
endif
SetIsk 48-57,65-90,97-122,-
delcommand SetIsk

" comments
syn region  gpgComment	    contained display oneline start="#" end="$" contains=gpgTodo,gpgID

" todo
syn keyword gpgTodo	    contained FIXME TODO XXX NOTE

" ids
syn match   gpgID	    contained display "\<\(0x\)\=\x\{8,}\>"

syn match   gpgBegin	    "^" skipwhite nextgroup=gpgComment,gpgOption,gpgCommand

" commands that take args
syn keyword gpgCommand      contained skipwhite nextgroup=gpgArg check-sigs decrypt decrypt-files delete-key delete-secret-and-public-key delete-secret-key edit-key encrypt-files export export-all export-ownertrust export-secret-keys export-secret-subkeys fast-import fingerprint gen-prime gen-random import import-ownertrust list-keys list-public-keys list-secret-keys list-sigs lsign-key nrsign-key print-md print-mds recv-keys search-keys send-keys sign-key verify verify-files
" commands that take no args
syn keyword gpgCommand      contained skipwhite nextgroup=gpgArgError check-trustdb clearsign desig-revoke detach-sign encrypt gen-key gen-revoke help list-packets rebuild-keydb-caches sign store symmetric update-trustdb version warranty

" options that take args
syn keyword gpgOption       contained skipwhite nextgroup=gpgArg attribute-fd cert-digest-algo charset cipher-algo command-fd comment completes-needed compress compress-algo debug default-cert-check-level default-key default-preference-list default-recipient digest-algo disable-cipher-algo disable-pubkey-algo encrypt-to exec-path export-options group homedir import-options keyring keyserver keyserver-options load-extension local-user logger-fd marginals-needed max-cert-depth notation-data options output override-session-key passphrase-fd personal-cipher-preferences personal-compress-preferences personal-digest-preferences photo-viewer recipient s2k-cipher-algo s2k-digest-algo s2k-mode secret-keyring set-filename set-policy-url status-fd trusted-key
" options that take no args
syn keyword gpgOption       contained skipwhite nextgroup=gpgArgError allow-freeform-uid allow-non-selfsigned-uid allow-secret-key-import always-trust armor ask-cert-expire ask-sig-expire auto-check-trustdb batch debug-all default-comment default-recipient-self dry-run emit-version emulate-md-encode-bug enable-special-filenames escape-from-lines expert fast-list-mode fixed-list-mode for-your-eyes-only force-mdc force-v3-sigs force-v4-certs gpg-agent-info ignore-crc-error ignore-mdc-error ignore-time-conflict ignore-valid-from interactive list-only lock-multiple lock-never lock-once merge-only no no-allow-non-selfsigned-uid no-armor no-ask-cert-expire no-ask-sig-expire no-auto-check-trustdb no-batch no-comment no-default-keyring no-default-recipient no-encrypt-to no-expensive-trust-checks no-expert no-for-your-eyes-only no-force-v3-sigs no-force-v4-certs no-greeting no-literal no-mdc-warning no-options no-permission-warning no-pgp2 no-pgp6 no-pgp7 no-random-seed-file no-secmem-warning no-show-notation no-show-photos no-show-policy-url no-sig-cache no-sig-create-check no-sk-comments no-tty no-utf8-strings no-verbose no-version not-dash-escaped openpgp pgp2 pgp6 pgp7 preserve-permissions quiet rfc1991 set-filesize show-keyring show-notation show-photos show-policy-url show-session-key simple-sk-checksum sk-comments skip-verify textmode throw-keyid try-all-secrets use-agent use-embedded-filename utf8-strings verbose with-colons with-fingerprint with-key-data yes

" arguments to commands and options
syn match   gpgArg          contained display "\S\+\(\s\+\S\+\)*" contains=gpgID
syn match   gpgArgError     contained display "\S\+\(\s\+\S\+\)*"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_gpg_syn_inits")
  if version < 508
    let did_gpg_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink gpgComment   Comment
  HiLink gpgTodo      Todo
  HiLink gpgID        Number
  HiLink gpgOption    Keyword
  HiLink gpgCommand   Error
  HiLink gpgArgError  Error
  delcommand HiLink
endif

let b:current_syntax = "gpg"

" vim: set sts=2 sw=2:
