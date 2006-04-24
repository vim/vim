" Vim syntax file
" Language:	Mutt setup files
" Original:	Preben 'Peppe' Guldberg <peppe-vim@wielders.org>
" Maintainer:	Kyle Wheeler <kyle-muttrc.vim@memoryhole.net>
" Last Change:	22 Apr 2006

" This file covers mutt version 1.5.11
" Included are also a few features from 1.4.2.1

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Set the keyword characters
if version < 600
  set isk=@,48-57,_,-
else
  setlocal isk=@,48-57,_,-
endif

syn match muttrcComment		"^#.*$"
syn match muttrcComment		"[^\\]#.*$"lc=1

" Escape sequences (back-tick and pipe goes here too)
syn match muttrcEscape		+\\[#tnr"'Cc]+
syn match muttrcEscape		+[`|]+

" The variables takes the following arguments
syn match  muttrcString		"=\s*[^ #"'`]\+"lc=1 contains=muttrcEscape
syn region muttrcString		start=+"+ms=e skip=+\\"+ end=+"+ contains=muttrcEscape,muttrcSet,muttrcUnset,muttrcReset,muttrcToggle,muttrcMacro,muttrcCommand,muttrcAction
syn region muttrcString		start=+'+ms=e skip=+\\'+ end=+'+ contains=muttrcEscape,muttrcSet,muttrcUnset,muttrcReset,muttrcToggle,muttrcMacro,muttrcCommand,muttrcAction

syn region muttrcShellString	matchgroup=muttrcEscape keepend start=+`+ skip=+\\`+ end=+`+ contains=muttrcVarStr,muttrcVarBool,muttrcVarQuad,muttrcVarNum,muttrcCommand,muttrcSet

syn match  muttrcRXChars	contained /[^\\][][.*?+]\+/hs=s+1
syn match  muttrcRXChars	contained /[][|()][.*?+]*/
syn match  muttrcRXChars	contained /'^/ms=s+1
syn match  muttrcRXChars	contained /$'/me=e-1
syn match  muttrcRXChars	contained /\\/
syn region muttrcRXString	contained start=+'+ skip=+\\'+ end=+'+ contains=muttrcRXChars
syn region muttrcRXString	contained start=+"+ skip=+\\"+ end=+"+ contains=muttrcRXChars

syn region muttrcRXPat		contained start=+'+ skip=+\\'+ end=+'\s*+ keepend skipwhite contains=muttrcRXString nextgroup=muttrcRXPat
syn region muttrcRXPat		contained start=+"+ skip=+\\"+ end=+"\s*+ keepend skipwhite contains=muttrcRXString nextgroup=muttrcRXPat
syn match muttrcRXPat		contained /[^-'"#!]\S\+\%(\s\|$\)/ skipwhite contains=muttrcRXChars nextgroup=muttrcRXPat
syn match muttrcRXDef 		contained "-rx\s\+" skipwhite nextgroup=muttrcRXPat

syn match muttrcSpecial		+\(['"]\)!\1+

" Numbers and Quadoptions may be surrounded by " or '
syn match muttrcNumber		/=\s*\d\+/lc=1
syn match muttrcNumber		/=\s*"\d\+"/lc=1
syn match muttrcNumber		/=\s*'\d\+'/lc=1
syn match muttrcQuadopt		+=\s*\%(ask-\)\=\%(yes\|no\)+lc=1
syn match muttrcQuadopt		+=\s*"\%(ask-\)\=\%(yes\|no\)"+lc=1
syn match muttrcQuadopt		+=\s*'\%(ask-\)\=\%(yes\|no\)'+lc=1

" Now catch some email addresses and headers (purified version from mail.vim)
syn match muttrcEmail		"[a-zA-Z0-9._-]\+@[a-zA-Z0-9./-]\+"
syn match muttrcHeader		"\<\%(From\|To\|C[Cc]\|B[Cc][Cc]\|Reply-To\|Subject\|Return-Path\|Received\|Date\|Replied\|Attach\)\>:\="

syn match   muttrcKeySpecial	contained +\%(\\[Cc'"]\|\^\|\\[01]\d\{2}\)+
syn match   muttrcKey		contained "\S\+"			contains=muttrcKeySpecial,muttrcKeyName
syn region  muttrcKey		contained start=+"+ skip=+\\\\\|\\"+ end=+"+	contains=muttrcKeySpecial,muttrcKeyName
syn region  muttrcKey		contained start=+'+ skip=+\\\\\|\\'+ end=+'+	contains=muttrcKeySpecial,muttrcKeyName
syn match   muttrcKeyName	contained "\<f\%(\d\|10\)\>"
syn match   muttrcKeyName	contained "\\[trne]"
syn match   muttrcKeyName	contained "\c<\%(BackSpace\|Delete\|Down\|End\|Enter\|Esc\|Home\|Insert\|Left\|PageDown\|PageUp\|Return\|Right\|Space\|Tab\|Up\)>"

syn keyword muttrcVarBool	contained allow_8bit allow_ansi arrow_cursor ascii_chars askbcc
syn keyword muttrcVarBool	contained askcc attach_split auto_tag autoedit beep beep_new
syn keyword muttrcVarBool	contained bounce_delivered braille_friendly check_new collapse_unread
syn keyword muttrcVarBool	contained confirmappend confirmcreate crypt_autoencrypt crypt_autopgp
syn keyword muttrcVarBool	contained crypt_autosign crypt_autosmime crypt_replyencrypt
syn keyword muttrcVarBool	contained crypt_replysign crypt_replysignencrypted crypt_timestamp
syn keyword muttrcVarBool	contained crypt_use_gpgme delete_untag digest_collapse duplicate_threads
syn keyword muttrcVarBool	contained edit_hdrs edit_headers encode_from envelope_from fast_reply
syn keyword muttrcVarBool	contained fcc_attach fcc_clear followup_to force_name forw_decode
syn keyword muttrcVarBool	contained forw_decrypt forw_quote forward_decode forward_decrypt
syn keyword muttrcVarBool	contained forward_quote hdrs header help hidden_host hide_limited
syn keyword muttrcVarBool	contained hide_missing hide_thread_subject hide_top_limited
syn keyword muttrcVarBool	contained hide_top_missing ignore_list_reply_to imap_check_subscribed
syn keyword muttrcVarBool	contained imap_list_subscribed imap_passive imap_peek imap_servernoise
syn keyword muttrcVarBool	contained implicit_autoview include_onlyfirst keep_flagged
syn keyword muttrcVarBool	contained mailcap_sanitize maildir_header_cache_verify maildir_trash
syn keyword muttrcVarBool	contained mark_old markers menu_move_off menu_scroll meta_key
syn keyword muttrcVarBool	contained metoo mh_purge mime_forward_decode narrow_tree pager_stop
syn keyword muttrcVarBool	contained pgp_auto_decode pgp_auto_traditional pgp_autoencrypt
syn keyword muttrcVarBool	contained pgp_autoinline pgp_autosign pgp_check_exit
syn keyword muttrcVarBool	contained pgp_create_traditional pgp_ignore_subkeys pgp_long_ids
syn keyword muttrcVarBool	contained pgp_replyencrypt pgp_replyinline pgp_replysign
syn keyword muttrcVarBool	contained pgp_replysignencrypted pgp_retainable_sigs pgp_show_unusable
syn keyword muttrcVarBool	contained pgp_strict_enc pgp_use_gpg_agent pipe_decode pipe_split
syn keyword muttrcVarBool	contained pop_auth_try_all pop_last print_decode print_split
syn keyword muttrcVarBool	contained prompt_after read_only reply_self resolve reverse_alias
syn keyword muttrcVarBool	contained reverse_name reverse_realname rfc2047_parameters save_address
syn keyword muttrcVarBool	contained save_empty save_name score sig_dashes sig_on_top
syn keyword muttrcVarBool	contained smart_wrap smime_ask_cert_label smime_decrypt_use_default_key
syn keyword muttrcVarBool	contained smime_is_default sort_re ssl_force_tls ssl_use_sslv2
syn keyword muttrcVarBool	contained ssl_use_sslv3 ssl_use_tlsv1 ssl_usesystemcerts status_on_top
syn keyword muttrcVarBool	contained strict_threads suspend text_flowed thorough_search
syn keyword muttrcVarBool	contained thread_received tilde uncollapse_jump use_8bitmime
syn keyword muttrcVarBool	contained use_domain use_envelope_from use_from use_idn use_ipv6
syn keyword muttrcVarBool	contained user_agent wait_key weed wrap_search write_bcc

syn keyword muttrcVarBool	contained noallow_8bit noallow_ansi noarrow_cursor noascii_chars noaskbcc
syn keyword muttrcVarBool	contained noaskcc noattach_split noauto_tag noautoedit nobeep nobeep_new
syn keyword muttrcVarBool	contained nobounce_delivered nobraille_friendly nocheck_new nocollapse_unread
syn keyword muttrcVarBool	contained noconfirmappend noconfirmcreate nocrypt_autoencrypt nocrypt_autopgp
syn keyword muttrcVarBool	contained nocrypt_autosign nocrypt_autosmime nocrypt_replyencrypt
syn keyword muttrcVarBool	contained nocrypt_replysign nocrypt_replysignencrypted nocrypt_timestamp
syn keyword muttrcVarBool	contained nocrypt_use_gpgme nodelete_untag nodigest_collapse noduplicate_threads
syn keyword muttrcVarBool	contained noedit_hdrs noedit_headers noencode_from noenvelope_from nofast_reply
syn keyword muttrcVarBool	contained nofcc_attach nofcc_clear nofollowup_to noforce_name noforw_decode
syn keyword muttrcVarBool	contained noforw_decrypt noforw_quote noforward_decode noforward_decrypt
syn keyword muttrcVarBool	contained noforward_quote nohdrs noheader nohelp nohidden_host nohide_limited
syn keyword muttrcVarBool	contained nohide_missing nohide_thread_subject nohide_top_limited
syn keyword muttrcVarBool	contained nohide_top_missing noignore_list_reply_to noimap_check_subscribed
syn keyword muttrcVarBool	contained noimap_list_subscribed noimap_passive noimap_peek noimap_servernoise
syn keyword muttrcVarBool	contained noimplicit_autoview noinclude_onlyfirst nokeep_flagged
syn keyword muttrcVarBool	contained nomailcap_sanitize nomaildir_header_cache_verify nomaildir_trash
syn keyword muttrcVarBool	contained nomark_old nomarkers nomenu_move_off nomenu_scroll nometa_key
syn keyword muttrcVarBool	contained nometoo nomh_purge nomime_forward_decode nonarrow_tree nopager_stop
syn keyword muttrcVarBool	contained nopgp_auto_decode nopgp_auto_traditional nopgp_autoencrypt
syn keyword muttrcVarBool	contained nopgp_autoinline nopgp_autosign nopgp_check_exit
syn keyword muttrcVarBool	contained nopgp_create_traditional nopgp_ignore_subkeys nopgp_long_ids
syn keyword muttrcVarBool	contained nopgp_replyencrypt nopgp_replyinline nopgp_replysign
syn keyword muttrcVarBool	contained nopgp_replysignencrypted nopgp_retainable_sigs nopgp_show_unusable
syn keyword muttrcVarBool	contained nopgp_strict_enc nopgp_use_gpg_agent nopipe_decode nopipe_split
syn keyword muttrcVarBool	contained nopop_auth_try_all nopop_last noprint_decode noprint_split
syn keyword muttrcVarBool	contained noprompt_after noread_only noreply_self noresolve noreverse_alias
syn keyword muttrcVarBool	contained noreverse_name noreverse_realname norfc2047_parameters nosave_address
syn keyword muttrcVarBool	contained nosave_empty nosave_name noscore nosig_dashes nosig_on_top
syn keyword muttrcVarBool	contained nosmart_wrap nosmime_ask_cert_label nosmime_decrypt_use_default_key
syn keyword muttrcVarBool	contained nosmime_is_default nosort_re nossl_force_tls nossl_use_sslv2
syn keyword muttrcVarBool	contained nossl_use_sslv3 nossl_use_tlsv1 nossl_usesystemcerts nostatus_on_top
syn keyword muttrcVarBool	contained nostrict_threads nosuspend notext_flowed nothorough_search
syn keyword muttrcVarBool	contained nothread_received notilde nouncollapse_jump nouse_8bitmime
syn keyword muttrcVarBool	contained nouse_domain nouse_envelope_from nouse_from nouse_idn nouse_ipv6
syn keyword muttrcVarBool	contained nouser_agent nowait_key noweed nowrap_search nowrite_bcc

syn keyword muttrcVarBool	contained invallow_8bit invallow_ansi invarrow_cursor invascii_chars invaskbcc
syn keyword muttrcVarBool	contained invaskcc invattach_split invauto_tag invautoedit invbeep invbeep_new
syn keyword muttrcVarBool	contained invbounce_delivered invbraille_friendly invcheck_new invcollapse_unread
syn keyword muttrcVarBool	contained invconfirmappend invconfirmcreate invcrypt_autoencrypt invcrypt_autopgp
syn keyword muttrcVarBool	contained invcrypt_autosign invcrypt_autosmime invcrypt_replyencrypt
syn keyword muttrcVarBool	contained invcrypt_replysign invcrypt_replysignencrypted invcrypt_timestamp
syn keyword muttrcVarBool	contained invcrypt_use_gpgme invdelete_untag invdigest_collapse invduplicate_threads
syn keyword muttrcVarBool	contained invedit_hdrs invedit_headers invencode_from invenvelope_from invfast_reply
syn keyword muttrcVarBool	contained invfcc_attach invfcc_clear invfollowup_to invforce_name invforw_decode
syn keyword muttrcVarBool	contained invforw_decrypt invforw_quote invforward_decode invforward_decrypt
syn keyword muttrcVarBool	contained invforward_quote invhdrs invheader invhelp invhidden_host invhide_limited
syn keyword muttrcVarBool	contained invhide_missing invhide_thread_subject invhide_top_limited
syn keyword muttrcVarBool	contained invhide_top_missing invignore_list_reply_to invimap_check_subscribed
syn keyword muttrcVarBool	contained invimap_list_subscribed invimap_passive invimap_peek invimap_servernoise
syn keyword muttrcVarBool	contained invimplicit_autoview invinclude_onlyfirst invkeep_flagged
syn keyword muttrcVarBool	contained invmailcap_sanitize invmaildir_header_cache_verify invmaildir_trash
syn keyword muttrcVarBool	contained invmark_old invmarkers invmenu_move_off invmenu_scroll invmeta_key
syn keyword muttrcVarBool	contained invmetoo invmh_purge invmime_forward_decode invnarrow_tree invpager_stop
syn keyword muttrcVarBool	contained invpgp_auto_decode invpgp_auto_traditional invpgp_autoencrypt
syn keyword muttrcVarBool	contained invpgp_autoinline invpgp_autosign invpgp_check_exit
syn keyword muttrcVarBool	contained invpgp_create_traditional invpgp_ignore_subkeys invpgp_long_ids
syn keyword muttrcVarBool	contained invpgp_replyencrypt invpgp_replyinline invpgp_replysign
syn keyword muttrcVarBool	contained invpgp_replysignencrypted invpgp_retainable_sigs invpgp_show_unusable
syn keyword muttrcVarBool	contained invpgp_strict_enc invpgp_use_gpg_agent invpipe_decode invpipe_split
syn keyword muttrcVarBool	contained invpop_auth_try_all invpop_last invprint_decode invprint_split
syn keyword muttrcVarBool	contained invprompt_after invread_only invreply_self invresolve invreverse_alias
syn keyword muttrcVarBool	contained invreverse_name invreverse_realname invrfc2047_parameters invsave_address
syn keyword muttrcVarBool	contained invsave_empty invsave_name invscore invsig_dashes invsig_on_top
syn keyword muttrcVarBool	contained invsmart_wrap invsmime_ask_cert_label invsmime_decrypt_use_default_key
syn keyword muttrcVarBool	contained invsmime_is_default invsort_re invssl_force_tls invssl_use_sslv2
syn keyword muttrcVarBool	contained invssl_use_sslv3 invssl_use_tlsv1 invssl_usesystemcerts invstatus_on_top
syn keyword muttrcVarBool	contained invstrict_threads invsuspend invtext_flowed invthorough_search
syn keyword muttrcVarBool	contained invthread_received invtilde invuncollapse_jump invuse_8bitmime
syn keyword muttrcVarBool	contained invuse_domain invuse_envelope_from invuse_from invuse_idn invuse_ipv6
syn keyword muttrcVarBool	contained invuser_agent invwait_key invweed invwrap_search invwrite_bcc

syn keyword muttrcVarQuad	contained abort_nosubject abort_unmodified bounce copy
syn keyword muttrcVarQuad	contained crypt_verify_sig delete forward_edit honor_followup_to
syn keyword muttrcVarQuad	contained include mime_forward mime_forward_rest mime_fwd move
syn keyword muttrcVarQuad	contained pgp_mime_auto pgp_verify_sig pop_delete pop_reconnect
syn keyword muttrcVarQuad	contained postpone print quit recall reply_to ssl_starttls

syn keyword muttrcVarQuad	contained noabort_nosubject noabort_unmodified nobounce nocopy
syn keyword muttrcVarQuad	contained nocrypt_verify_sig nodelete noforward_edit nohonor_followup_to
syn keyword muttrcVarQuad	contained noinclude nomime_forward nomime_forward_rest nomime_fwd nomove
syn keyword muttrcVarQuad	contained nopgp_mime_auto nopgp_verify_sig nopop_delete nopop_reconnect
syn keyword muttrcVarQuad	contained nopostpone noprint noquit norecall noreply_to nossl_starttls

syn keyword muttrcVarQuad	contained invabort_nosubject invabort_unmodified invbounce invcopy
syn keyword muttrcVarQuad	contained invcrypt_verify_sig invdelete invforward_edit invhonor_followup_to
syn keyword muttrcVarQuad	contained invinclude invmime_forward invmime_forward_rest invmime_fwd invmove
syn keyword muttrcVarQuad	contained invpgp_mime_auto invpgp_verify_sig invpop_delete invpop_reconnect
syn keyword muttrcVarQuad	contained invpostpone invprint invquit invrecall invreply_to invssl_starttls

syn keyword muttrcVarNum	contained connect_timeout history imap_keepalive mail_check menu_context net_inc
syn keyword muttrcVarNum	contained pager_context pager_index_lines pgp_timeout pop_checkinterval read_inc
syn keyword muttrcVarNum	contained score_threshold_delete score_threshold_flag score_threshold_read
syn keyword muttrcVarNum	contained sendmail_wait sleep_time smime_timeout ssl_min_dh_prime_bits timeout
syn keyword muttrcVarNum	contained wrapmargin write_inc

syn keyword muttrcVarStr	contained alias_file alias_format attach_format attach_sep attribution
syn keyword muttrcVarStr	contained certificate_file charset compose_format config_charset content_type
syn keyword muttrcVarStr	contained date_format default_hook display_filter dotlock_program dsn_notify
syn keyword muttrcVarStr	contained dsn_return editor entropy_file envelope_from_address escape folder
syn keyword muttrcVarStr	contained folder_format forw_format forward_format from gecos_mask hdr_format
syn keyword muttrcVarStr	contained header_cache header_cache_pagesize hostname imap_authenticators
syn keyword muttrcVarStr	contained imap_delim_chars imap_headers imap_home_namespace imap_login imap_pass
syn keyword muttrcVarStr	contained imap_user indent_str indent_string index_format ispell locale mailcap_path
syn keyword muttrcVarStr	contained mask mbox mbox_type message_format mh_seq_flagged mh_seq_replied
syn keyword muttrcVarStr	contained mh_seq_unseen mix_entry_format mixmaster msg_format pager pager_format
syn keyword muttrcVarStr	contained pgp_clearsign_command pgp_decode_command pgp_decrypt_command
syn keyword muttrcVarStr	contained pgp_encrypt_only_command pgp_encrypt_sign_command pgp_entry_format
syn keyword muttrcVarStr	contained pgp_export_command pgp_getkeys_command pgp_good_sign pgp_import_command
syn keyword muttrcVarStr	contained pgp_list_pubring_command pgp_list_secring_command pgp_sign_as
syn keyword muttrcVarStr	contained pgp_sign_command pgp_sort_keys pgp_verify_command pgp_verify_key_command
syn keyword muttrcVarStr	contained pipe_sep pop_authenticators pop_host pop_pass pop_user post_indent_str
syn keyword muttrcVarStr	contained post_indent_string postponed preconnect print_cmd print_command
syn keyword muttrcVarStr	contained query_command quote_regexp realname record reply_regexp send_charset
syn keyword muttrcVarStr	contained sendmail shell signature simple_search smileys smime_ca_location
syn keyword muttrcVarStr	contained smime_certificates smime_decrypt_command smime_default_key
syn keyword muttrcVarStr	contained smime_encrypt_command smime_encrypt_with smime_get_cert_command
syn keyword muttrcVarStr	contained smime_get_cert_email_command smime_get_signer_cert_command
syn keyword muttrcVarStr	contained smime_import_cert_command smime_keys smime_pk7out_command smime_sign_as
syn keyword muttrcVarStr	contained smime_sign_command smime_sign_opaque_command smime_verify_command
syn keyword muttrcVarStr	contained smime_verify_opaque_command sort sort_alias sort_aux sort_browser
syn keyword muttrcVarStr	contained spam_separator spoolfile ssl_ca_certificates_file ssl_client_cert
syn keyword muttrcVarStr	contained status_chars status_format tmpdir to_chars tunnel visual

" Present in 1.4.2.1 (pgp_create_traditional was a bool then)
syn keyword muttrcVarBool	contained imap_force_ssl imap_force_ssl noinvimap_force_ssl
"syn keyword muttrcVarQuad	contained pgp_create_traditional nopgp_create_traditional invpgp_create_traditional
syn keyword muttrcVarStr	contained alternates

syn keyword muttrcMenu		contained alias attach browser compose editor index pager postpone pgp mix query generic
syn match muttrcMenuList "\S\+" contained contains=muttrcMenu
syn match muttrcMenuCommas /,/ contained

syn keyword muttrcCommand	auto_view alternative_order charset-hook exec unalternative_order
syn keyword muttrcCommand	hdr_order iconv-hook ignore mailboxes my_hdr unmailboxes
syn keyword muttrcCommand	pgp-hook push score source unauto_view unhdr_order
syn keyword muttrcCommand	unhook unignore unmono unmy_hdr unscore
syn keyword muttrcCommand	mime_lookup unmime_lookup spam ungroup
syn keyword muttrcCommand	nospam unalternative_order

syn match muttrcAttachmentsMimeType contained "[*a-z0-9_-]\+/[*a-z0-9._-]\+\s*" skipwhite nextgroup=muttrcAttachmentsMimeType
syn match muttrcAttachmentsFlag contained "[+-]\%([AI]\|inline\|attachment\)\s\+" skipwhite nextgroup=muttrcAttachmentsMimeType
syn match muttrcAttachmentsLine "^\s*\%(un\)\?attachments\s\+" skipwhite nextgroup=muttrcAttachmentsFlag

syn match muttrcUnHighlightSpace contained "\%(\s\+\|\\$\)"

syn keyword muttrcListsKeyword	contained lists unlists
syn region muttrcListsLine	keepend start=+^\s*\%(un\)\?lists\s+ skip=+\\$+ end=+$+ contains=muttrcListsKeyword,muttrcRXPat,muttrcGroupDef,muttrcUnHighlightSpace,muttrcComment

syn keyword muttrcSubscribeKeyword	contained subscribe unsubscribe
syn region muttrcSubscribeLine 	keepend start=+^\s*\%(un\)\?subscribe\s+ skip=+\\$+ end=+$+ contains=muttrcSubscribeKeyword,muttrcRXPat,muttrcGroupDef,muttrcUnHighlightSpace,muttrcComment

syn keyword muttrcAlternateKeyword contained alternates unalternates
syn region muttrcAlternatesLine keepend start=+^\s*\%(un\)\?alternates\s+ skip=+\\$+ end=+$+ contains=muttrcAlternateKeyword,muttrcGroupDef,muttrcRXPat,muttrcUnHighlightSpace,muttrcComment

syn match muttrcVariable	"\$[a-zA-Z_-]\+"

syn match muttrcBadAction	contained "[^<>]\+" contains=muttrcEmail
syn match muttrcFunction	contained "\<\%(attach\|bounce\|copy\|delete\|display\|flag\|forward\|parent\|pipe\|postpone\|print\|recall\|resent\|save\|send\|tag\|undelete\)-message\>"
syn match muttrcFunction	contained "\<\%(delete\|next\|previous\|read\|tag\|undelete\)-thread\>"
syn match muttrcFunction	contained "\<\%(backward\|capitalize\|downcase\|forward\|kill\|upcase\)-word\>"
syn match muttrcFunction	contained "\<\%(delete\|filter\|first\|last\|next\|pipe\|previous\|print\|save\|select\|tag\|undelete\)-entry\>"
syn match muttrcFunction	contained "\<attach-\%(file\|key\)\>"
syn match muttrcFunction	contained "\<change-\%(dir\|folder\|folder-readonly\)\>"
syn match muttrcFunction	contained "\<check-\%(new\|traditional-pgp\)\>"
syn match muttrcFunction	contained "\<current-\%(bottom\|middle\|top\)\>"
syn match muttrcFunction	contained "\<decode-\%(copy\|save\)\>"
syn match muttrcFunction	contained "\<delete-\%(char\|pattern\|subthread\)\>"
syn match muttrcFunction	contained "\<display-\%(address\|toggle-weed\)\>"
syn match muttrcFunction	contained "\<edit\%(-\%(bcc\|cc\|description\|encoding\|fcc\|file\|from\|headers\|mime\|reply-to\|subject\|to\|type\)\)\?\>"
syn match muttrcFunction	contained "\<enter-\%(command\|mask\)\>"
syn match muttrcFunction	contained "\<half-\%(up\|down\)\>"
syn match muttrcFunction	contained "\<history-\%(up\|down\)\>"
syn match muttrcFunction	contained "\<kill-\%(eol\|eow\|line\)\>"
syn match muttrcFunction	contained "\<next-\%(line\|new\|page\|subthread\|undeleted\|unread\)\>"
syn match muttrcFunction	contained "\<previous-\%(line\|new\|page\|subthread\|undeleted\|unread\)\>"
syn match muttrcFunction	contained "\<search\%(-\%(next\|opposite\|reverse\|toggle\)\)\?\>"
syn match muttrcFunction	contained "\<show-\%(limit\|version\)\>"
syn match muttrcFunction	contained "\<sort-\%(mailbox\|reverse\)\>"
syn match muttrcFunction	contained "\<tag-\%(pattern\|prefix\)\>"
syn match muttrcFunction	contained "\<toggle-\%(mailboxes\|new\|quoted\|subscribed\|unlink\|write\)\>"
syn match muttrcFunction	contained "\<undelete-\%(pattern\|subthread\)\>"
syn match muttrcFunction	contained "\<collapse-\%(parts\|thread\|all\)\>"
syn match muttrcFunction	contained "\<view-\%(attach\|attachments\|file\|mailcap\|name\|text\)\>"
syn match muttrcFunction	contained "\<\%(backspace\|backward-char\|bol\|bottom\|bottom-page\|buffy-cycle\|clear-flag\|complete\%(-query\)\?\|copy-file\|create-alias\|detach-file\|eol\|exit\|extract-keys\|\%(imap-\)\?fetch-mail\|forget-passphrase\|forward-char\|group-reply\|help\|ispell\|jump\|limit\|list-reply\|mail\|mail-key\|mark-as-new\|middle-page\|new-mime\|pgp-menu\|query\|query-append\|quit\|quote-char\|read-subthread\|redraw-screen\|refresh\|rename-file\|reply\|select-new\|set-flag\|shell-escape\|skip-quoted\|sort\|subscribe\|sync-mailbox\|top\|top-page\|transpose-chars\|unsubscribe\|untag-pattern\|verify-key\|write-fcc\)\>"
syn match muttrcAction		contained "<[^>]\{-}>" contains=muttrcBadAction,muttrcFunction,muttrcKeyName

syn keyword muttrcSet		set     skipwhite nextgroup=muttrcVar.*
syn keyword muttrcUnset		unset   skipwhite nextgroup=muttrcVar.*
syn keyword muttrcReset		reset   skipwhite nextgroup=muttrcVar.*
syn keyword muttrcToggle	toggle  skipwhite nextgroup=muttrcVar.*

" First, functions that take regular expressions:
syn match  muttrcRXHookNot	contained /!\s*/ skipwhite nextgroup=muttrcRXString
syn match  muttrcRXHooks	/^\s*\%(account\|folder\)-hook\s\+/ skipwhite nextgroup=muttrcRXHookNot,muttrcRXString

" Now, functions that take patterns
syn match muttrcPatHookNot	contained /!\s*/ skipwhite nextgroup=muttrcPattern
syn match muttrcPatHooks	/^\s*\%(message\|mbox\|save\|fcc\%(-save\)\?\|send2\?\|reply\|crypt\)-hook\s\+/ nextgroup=muttrcPatHookNot,muttrcPattern

syn match muttrcBindFunction	contained /\S\+\%(\s\|$\)/ skipwhite contains=muttrcFunction
syn match muttrcBindFunctionNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcBindFunction,muttrcBindFunctionNL
syn match muttrcBindKey		contained /\S\+/ skipwhite contains=muttrcKey nextgroup=muttrcBindFunction,muttrcBindFunctionNL
syn match muttrcBindKeyNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcBindKey,muttrcBindKeyNL
syn match muttrcBindMenuList	contained /\S\+/ skipwhite contains=muttrcMenu,muttrcMenuCommas nextgroup=muttrcBindKey,muttrcBindKeyNL
syn match muttrcBindMenuListNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcBindMenuList,muttrcBindMenuListNL
syn match muttrcBind		/^\s*bind\s\?/ skipwhite nextgroup=muttrcBindMenuList,muttrcBindMenuListNL

syn match muttrcMacroKey	contained /\S\+/ skipwhite contains=muttrcKey
syn match muttrcMacroKeyNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcMacroKey,muttrcMacroKeyNL
syn match muttrcMacroMenuList	contained /\S\+/ skipwhite contains=muttrcMenu,muttrcMenuCommas nextgroup=muttrcMacroKey,muttrcMacroKeyNL
syn match muttrcMacroMenuListNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcMacroMenuList,muttrcMacroMenuListNL
syn match muttrcMacro		/^\s*macro\s\?/	skipwhite nextgroup=muttrcMacroMenuList,muttrcMacroMenuListNL

syn match muttrcAddrContent	contained "[a-zA-Z0-9._-]\+@[a-zA-Z0-9./-]\+\s*" skipwhite contains=muttrcEmail nextgroup=muttrcAddrContent
syn region muttrcAddrContent	contained start=+'+ end=+'\s*+ skip=+\\'+ skipwhite contains=muttrcEmail nextgroup=muttrcAddrContent
syn region muttrcAddrContent	contained start=+"+ end=+"\s*+ skip=+\\"+ skipwhite contains=muttrcEmail nextgroup=muttrcAddrContent
syn match muttrcAddrDef 	contained "-addr\s\+" skipwhite nextgroup=muttrcAddrContent

syn match muttrcGroupFlag	contained "-group"
syn region muttrcGroupDef	contained start="-group\s\+" skip="\\$" end="\s" skipwhite keepend contains=muttrcGroupFlag,muttrcUnHighlightSpace

syn keyword muttrcGroupKeyword	contained group ungroup
syn region muttrcGroupLine	keepend start=+^\s*\%(un\)\?group\s+ skip=+\\$+ end=+$+ contains=muttrcGroupKeyword,muttrcGroupDef,muttrcAddrDef,muttrcRXDef,muttrcUnHighlightSpace,muttrcComment

syn match muttrcAliasGroupName	contained /\w\+/ skipwhite nextgroup=muttrcAliasGroupDef,muttrcAliasAbbr,muttrcAliasNL
syn match muttrcAliasGroupDefNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcAliasGroupName,muttrcAliasGroupDefNL
syn match muttrcAliasGroupDef	contained /\s*-group/ skipwhite nextgroup=muttrcAliasGroupName,muttrcAliasGroupDefNL contains=muttrcGroupFlag
syn match muttrcAliasEmail	contained /\S\+@\S\+/ contains=muttrcEmail nextgroup=muttrcAliasName,muttrcAliasNameNL
syn match muttrcAliasEncEmail	contained /<[^>]\+>/ contains=muttrcEmail
syn match muttrcAliasEncEmailNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcAliasEncEmail,muttrcAliasEncEmailNL
syn match muttrcAliasNameNoParens contained /[^<(@]\+\s\+/ nextgroup=muttrcAliasEncEmail,muttrcAliasEncEmailNL
syn region muttrcAliasName	contained matchgroup=Type start=/(/ end=/)/ skipwhite
syn match muttrcAliasNameNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcAliasName,muttrcAliasNameNL
syn match muttrcAliasENNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcAliasEmail,muttrcAliasNameNoParens,muttrcAliasENNL
syn match muttrcAliasAbbr	contained /\s*[^- \t]\S\+/ skipwhite nextgroup=muttrcAliasEmail,muttrcAliasNameNoParens,muttrcAliasENNL
syn match muttrcAliasNL		contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcAliasGroupDef,muttrcAliasAbbr,muttrcAliasNL
syn match muttrcAlias		/^\s*alias\s/ skipwhite nextgroup=muttrcAliasGroupDef,muttrcAliasAbbr,muttrcAliasNL

syn match muttrcUnAliasAbbr	contained "\s*\w\+\s*" skipwhite nextgroup=muttrcUnAliasAbbr,muttrcUnAliasNL
syn match muttrcUnAliasNL	contained /\s*\\$/ skipwhite skipnl nextgroup=muttrcUnAliasAbbr,muttrcUnAliasNL
syn match muttrcUnAlias		/^\s*unalias\s\?/ nextgroup=muttrcUnAliasAbbr,muttrcUnAliasNL

syn match muttrcSimplePat contained "!\?\^\?[~][ADEFgGklNOpPQRSTuUvV=$]"
syn match muttrcSimplePat contained "!\?\^\?[~][mnXz]\s\+\%([<>-][0-9]\+\|[0-9]\+[-][0-9]*\)"
syn match muttrcSimplePat contained "!\?\^\?[~][dr]\s\+[0-9]\{2}\%(/[0-9]\{2}\%(/[0-9]\{2}\%([0-9]\{2}\)\)\?\)\?"
syn match muttrcSimplePat contained "!\?\^\?[~][bBcCefhHiLstxy]\s\+" nextgroup=muttrcSimplePatRXContainer
syn match muttrcSimplePat contained "!\?\^\?[%][bBcCefhHiLstxy]\s\+" nextgroup=muttrcSimplePatGroup
"syn match muttrcSimplePat contained /"[^~=%][^"]*/ contains=muttrcRXPat
"syn match muttrcSimplePat contained /'[^~=%][^']*/ contains=muttrcRXPat
syn match muttrcSimplePatGroup contained /[a-zA-Z0-9]\+/
syn region muttrcSimplePatRXContainer contained keepend start=+"+ end=+"+ skip=+\\"+ contains=muttrcRXPat
syn region muttrcSimplePatRXContainer contained keepend start=+'+ end=+'+ skip=+\\'+ contains=muttrcRXPat
syn match muttrcSimplePatRXContainer contained /\S\+/ contains=muttrcRXPat
syn match muttrcSimplePatMetas contained /[(|)]/

syn region muttrcPattern contained keepend start=+"+ skip=+\\"+ end=+"+ contains=muttrcPatternInner
syn region muttrcPattern contained keepend start=+'+ skip=+\\'+ end=+'+ contains=muttrcPatternInner
syn match muttrcPattern contained "[~][A-Za-z]" contains=muttrcSimplePat
syn region muttrcPatternInner contained keepend start=+"[~=%!(^]+ms=s+1 skip=+\\"+ end=+"+me=e-1 contains=muttrcSimplePat,muttrcUnHighlightSpace,muttrcSimplePatMetas
syn region muttrcPatternInner contained keepend start=+'[~=%!(^]+ms=s+1 skip=+\\'+ end=+'+me=e-1 contains=muttrcSimplePat,muttrcUnHighlightSpace,muttrcSimplePatMetas

" Colour definitions takes object, foreground and background arguments (regexps excluded).
syn match muttrcColorMatchCount	contained "[0-9]\+"
syn match muttrcColorMatchCountNL contained skipwhite skipnl "\s*\\$" nextgroup=muttrcColorMatchCount,muttrcColorMatchCountNL
syn region muttrcColorRXPat	contained start=+\s\+'+ skip=+\\'+ end=+'\s*+ keepend skipwhite contains=muttrcRXString nextgroup=muttrcColorMatchCount,muttrcColorMatchCountNL
syn region muttrcColorRXPat	contained start=+\s\+"+ skip=+\\"+ end=+"\s*+ keepend skipwhite contains=muttrcRXString nextgroup=muttrcColorMatchCount,muttrcColorMatchCountNL
syn keyword muttrcColorField	contained attachment body bold error hdrdefault header index indicator markers message normal quoted search signature status tilde tree underline
syn match   muttrcColorField	contained "\<quoted\d\=\>"
syn keyword muttrcColor	contained black blue cyan default green magenta red white yellow
syn keyword muttrcColor	contained brightblack brightblue brightcyan brightdefault brightgreen brightmagenta brightred brightwhite brightyellow
syn match   muttrcColor	contained "\<\%(bright\)\=color\d\{1,2}\>"
" Now for the structure of the color line
syn match muttrcColorRXNL	contained skipwhite skipnl "\s*\\$" nextgroup=muttrcColorRXPat,muttrcColorRXNL
syn match muttrcColorBG 	contained skipwhite /\s*[$]\?\w\+/ contains=muttrcColor,muttrcVariable,muttrcUnHighlightSpace nextgroup=muttrcColorRXPat,muttrcColorRXNL
syn match muttrcColorBGNL	contained skipwhite skipnl "\s*\\$" nextgroup=muttrcColorBG,muttrcColorBGNL
syn match muttrcColorFG 	contained skipwhite /\s*[$]\?\w\+/ contains=muttrcColor,muttrcVariable,muttrcUnHighlightSpace nextgroup=muttrcColorBG,muttrcColorBGNL
syn match muttrcColorFGNL	contained skipwhite skipnl "\s*\\$" nextgroup=muttrcColorFG,muttrcColorFGNL
syn match muttrcColorContext 	contained skipwhite /\s*[$]\?\w\+/ contains=muttrcColorField,muttrcVariable,muttrcUnHighlightSpace nextgroup=muttrcColorFG,muttrcColorFGNL
syn match muttrcColorNL 	contained skipwhite skipnl "\s*\\$" nextgroup=muttrcColorContext,muttrcColorNL
syn match muttrcColorKeyword	contained skipwhite /^\s*color\s\+/ nextgroup=muttrcColorContext,muttrcColorNL
syn region muttrcColorLine keepend start=/^\s*color\s\+\%(index\)\@!/ skip=+\\$+ end=+$+ contains=muttrcColorKeyword,muttrcComment,muttrcUnHighlightSpace
" Now for the structure of the color index line
syn match muttrcPatternNL	contained skipwhite skipnl "\s*\\$" nextgroup=muttrcPattern,muttrcPatternNL
syn match muttrcColorBGI	contained skipwhite /\s*[$]\?\w\+\s*/ contains=muttrcColor,muttrcVariable,muttrcUnHighlightSpace nextgroup=muttrcPattern,muttrcPatternNL
syn match muttrcColorBGNLI	contained skipwhite skipnl "\s*\\$" nextgroup=muttrcColorBGI,muttrcColorBGNLI
syn match muttrcColorFGI	contained skipwhite /\s*[$]\?\w\+/ contains=muttrcColor,muttrcVariable,muttrcUnHighlightSpace nextgroup=muttrcColorBGI,muttrcColorBGNLI
syn match muttrcColorFGNLI	contained skipwhite skipnl "\s*\\$" nextgroup=muttrcColorFGI,muttrcColorFGNLI
syn match muttrcColorContextI	contained skipwhite /\s*index/ contains=muttrcUnHighlightSpace nextgroup=muttrcColorFGI,muttrcColorFGNLI
syn match muttrcColorNLI	contained skipwhite skipnl "\s*\\$" nextgroup=muttrcColorContextI,muttrcColorNLI
syn match muttrcColorKeywordI	contained skipwhite /^\s*color\s\+/ nextgroup=muttrcColorContextI,muttrcColorNLI
syn region muttrcColorLine keepend start=/^\s*color\s\+index/ skip=+\\$+ end=+$+ contains=muttrcColorKeywordI,muttrcComment,muttrcUnHighlightSpace
" And now color's brother:
syn region muttrcUnColorPatterns contained skipwhite start=+\s*'+ end=+'+ skip=+\\'+ contains=muttrcPattern nextgroup=muttrcUnColorPatterns,muttrcUnColorPatNL
syn region muttrcUnColorPatterns contained skipwhite start=+\s*"+ end=+"+ skip=+\\"+ contains=muttrcPattern nextgroup=muttrcUnColorPatterns,muttrcUnColorPatNL
syn match muttrcUnColorPatterns contained skipwhite /\s*[^'"\s]\S\*/ contains=muttrcPattern nextgroup=muttrcUnColorPatterns,muttrcUnColorPatNL
syn match muttrcUnColorPatNL	contained skipwhite skipnl /\s*\\$/ nextgroup=muttrcUnColorPatterns,muttrcUnColorPatNL
syn match muttrcUnColorAll	contained skipwhite /[*]/
syn match muttrcUnColorAPNL	contained skipwhite skipnl /\s*\\$/ nextgroup=muttrcUnColorPatterns,muttrcUnColorAll,muttrcUnColorAPNL
syn match muttrcUnColorIndex	contained skipwhite /\s*index\s\+/ nextgroup=muttrcUnColorPatterns,muttrcUnColorAll,muttrcUnColorAPNL
syn match muttrcUnColorIndexNL	contained skipwhite skipnl /\s*\\$/ nextgroup=muttrcUnColorIndex,muttrcUnColorIndexNL
syn match muttrcUnColorKeyword	contained skipwhite /^\s*uncolor\s\+/ nextgroup=muttrcUnColorIndex,muttrcUnColorIndexNL
syn region muttrcUnColorLine keepend start=+^\s*uncolor\s+ skip=+\\$+ end=+$+ contains=muttrcUnColorKeyword,muttrcComment,muttrcUnHighlightSpace

" Mono are almost like color (ojects inherited from color)
syn keyword muttrcMonoAttrib	contained bold none normal reverse standout underline
syn keyword muttrcMono		contained mono		skipwhite nextgroup=muttrcColorField
syn match   muttrcMonoLine	"^\s*mono\s\+\S\+"	skipwhite nextgroup=muttrcMonoAttrib contains=muttrcMono

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_muttrc_syntax_inits")
  if version < 508
    let did_muttrc_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink muttrcComment		Comment
  HiLink muttrcEscape		SpecialChar
  HiLink muttrcRXChars		SpecialChar
  HiLink muttrcString		String
  HiLink muttrcRXString		String
  HiLink muttrcSpecial		Special
  HiLink muttrcGroupFlag	Type
  HiLink muttrcGroupDef		Macro
  HiLink muttrcAddrDef		muttrcGroupFlag
  HiLink muttrcRXDef		muttrcGroupFlag
  HiLink muttrcRXPat		String
  HiLink muttrcAliasGroupName	Macro
  HiLink muttrcAliasAbbr	Identifier
  HiLink muttrcUnAliasAbbr	Identifier
  HiLink muttrcAliasEncEmail	Identifier
  HiLink muttrcAliasParens	Type
  HiLink muttrcNumber		Number
  HiLink muttrcQuadopt		Boolean
  HiLink muttrcEmail		Special
  HiLink muttrcVariable		Special
  HiLink muttrcHeader		Type
  HiLink muttrcKeySpecial	SpecialChar
  HiLink muttrcKey		Type
  HiLink muttrcKeyName		SpecialChar
  HiLink muttrcVarBool		Identifier
  HiLink muttrcVarQuad		Identifier
  HiLink muttrcVarNum		Identifier
  HiLink muttrcVarStr		Identifier
  HiLink muttrcMenu		Identifier
  HiLink muttrcCommand		Keyword
  HiLink muttrcSet		Keyword
  HiLink muttrcUnset		muttrcCommand
  HiLink muttrcReset		muttrcCommand
  HiLink muttrcToggle		muttrcCommand
  HiLink muttrcBind		muttrcCommand
  HiLink muttrcMacro		muttrcCommand
  HiLink muttrcAlias		muttrcCommand
  HiLink muttrcUnAlias		muttrcCommand
  HiLink muttrcAction		Macro
  HiLink muttrcBadAction	Error
  HiLink muttrcBindFunction	Error
  HiLink muttrcBindMenuList	Error
  HiLink muttrcFunction		Macro
  HiLink muttrcGroupKeyword	muttrcCommand
  HiLink muttrcGroupLine	Error
  HiLink muttrcSubscribeKeyword	muttrcCommand
  HiLink muttrcSubscribeLine	Error
  HiLink muttrcListsKeyword	muttrcCommand
  HiLink muttrcListsLine	Error
  HiLink muttrcAlternateKeyword	muttrcCommand
  HiLink muttrcAlternatesLine	Error
  HiLink muttrcAttachmentsLine	muttrcCommand
  HiLink muttrcAttachmentsFlag	Type
  HiLink muttrcAttachmentsMimeType	String
  HiLink muttrcColorLine	Error
  HiLink muttrcColorContext	Error
  HiLink muttrcColorContextI	Identifier
  HiLink muttrcColorKeyword	muttrcCommand
  HiLink muttrcColorKeywordI	muttrcColorKeyword
  HiLink muttrcColorField	Identifier
  HiLink muttrcColor		String
  HiLink muttrcColorFG		Error
  HiLink muttrcColorFGI		Error
  HiLink muttrcColorBG		Error
  HiLink muttrcColorBGI		Error
  HiLink muttrcMonoAttrib	muttrcColor
  HiLink muttrcMono		muttrcCommand
  HiLink muttrcSimplePat	Identifier
  HiLink muttrcSimplePatGroup	Macro
  HiLink muttrcSimplePatMetas	Special
  HiLink muttrcPattern		Type
  HiLink muttrcPatternInner	Error
  HiLink muttrcUnColorLine	Error
  HiLink muttrcUnColorKeyword	muttrcCommand
  HiLink muttrcUnColorIndex	Identifier
  HiLink muttrcShellString	muttrcEscape
  HiLink muttrcRXHooks		muttrcCommand
  HiLink muttrcRXHookNot	Type
  HiLink muttrcPatHooks		muttrcCommand
  HiLink muttrcPatHookNot	Type

  delcommand HiLink
endif

let b:current_syntax = "muttrc"

"EOF	vim: ts=8 noet tw=100 sw=8 sts=0
