" Vim syntax file
" Language:	Mutt setup files
" Maintainer:	Preben 'Peppe' Guldberg <peppe-vim@wielders.org>
" Contributor:	Gary Johnson <garyjohn@spk.agilent.com>
" Last Change:	27 May 2004

" This file covers mutt version 1.4.2.1i

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
syn match  muttrcString		"=\s*[^ #"']\+"lc=1 contains=muttrcEscape
syn region muttrcString		start=+"+ms=e skip=+\\"+ end=+"+ contains=muttrcEscape,muttrcSet,muttrcUnset,muttrcReset,muttrcToggle,muttrcMacro,muttrcCommand
syn region muttrcString		start=+'+ms=e skip=+\\'+ end=+'+ contains=muttrcEscape,muttrcSet,muttrcUnset,muttrcReset,muttrcToggle,muttrcMacro,muttrcCommand

syn match muttrcSpecial		+\(['"]\)!\1+

" Numbers and Quadoptions may be surrounded by " or '
syn match muttrcNumber		/=\s*\d\+/lc=1
syn match muttrcNumber		/=\s*"\d\+"/lc=1
syn match muttrcNumber		/=\s*'\d\+'/lc=1
syn match muttrcQuadopt		+=\s*\(ask-\)\=\(yes\|no\)+lc=1
syn match muttrcQuadopt		+=\s*"\(ask-\)\=\(yes\|no\)"+lc=1
syn match muttrcQuadopt		+=\s*'\(ask-\)\=\(yes\|no\)'+lc=1

" Now catch some email addresses and headers (purified version from mail.vim)
syn match muttrcEmail		"[a-zA-Z0-9._-]\+@[a-zA-Z0-9./-]\+"
syn match muttrcHeader		"\<\(From\|To\|Cc\|Bcc\|Reply-To\|Subject\|Return-Path\|Received\|Date\|Replied\|Attach\)\>:\="

syn match   muttrcKeySpecial	contained +\(\\[Cc'"]\|\^\|\\[01]\d\{2}\)+
syn match   muttrcKey		contained "\S\+"			contains=muttrcKeySpecial
syn region  muttrcKey		contained start=+"+ skip=+\\\\\|\\"+ end=+"+	contains=muttrcKeySpecial
syn region  muttrcKey		contained start=+'+ skip=+\\\\\|\\'+ end=+'+	contains=muttrcKeySpecial
syn match   muttrcKeyName	contained "\<f\(\d\|10\)\>"
syn match   muttrcKeyName	contained "\\[trne]"
syn match   muttrcKeyName	contained "<\(BackSpace\|Delete\|Down\|End\|Enter\|Esc\|Home\|Insert\|Left\|PageDown\|PageUp\|Return\|Right\|Space\|Tab\|Up\)>"

syn keyword muttrcVarBool	contained allow_8bit allow_ansi arrow_cursor ascii_chars askbcc askcc attach_split
syn keyword muttrcVarBool	contained auto_tag autoedit beep beep_new bounce_delivered check_new collapse_unread
syn keyword muttrcVarBool	contained confirmappend confirmcreate delete_untag digest_collapse duplicate_threads
syn keyword muttrcVarBool	contained edit_hdrs edit_headers encode_from envelope_from fast_reply fcc_attach
syn keyword muttrcVarBool	contained fcc_clear followup_to force_name forw_decode forw_decrypt forw_quote
syn keyword muttrcVarBool	contained forward_decode forward_decrypt forward_quote hdrs header help hidden_host
syn keyword muttrcVarBool	contained hide_limited hide_missing hide_top_limited hide_top_missing ignore_list_reply_to
syn keyword muttrcVarBool	contained imap_force_ssl imap_list_subscribed imap_passive imap_peek imap_servernoise
syn keyword muttrcVarBool	contained implicit_autoview keep_flagged mailcap_sanitize maildir_trash mark_old markers
syn keyword muttrcVarBool	contained menu_scroll meta_key metoo mh_purge mime_forward_decode pager_stop pgp_autoencrypt
syn keyword muttrcVarBool	contained pgp_autosign pgp_ignore_subkeys pgp_long_ids pgp_replyencrypt pgp_replysign
syn keyword muttrcVarBool	contained pgp_replysignencrypted pgp_retainable_sigs pgp_show_unusable pgp_strict_enc
syn keyword muttrcVarBool	contained pipe_decode pipe_split pop_auth_try_all pop_last print_decode print_split
syn keyword muttrcVarBool	contained prompt_after read_only reply_self resolve reverse_alias reverse_name
syn keyword muttrcVarBool	contained reverse_realname rfc2047_parameters save_address save_empty save_name score
syn keyword muttrcVarBool	contained sig_dashes sig_on_top smart_wrap sort_re ssl_use_sslv2 ssl_use_sslv3 ssl_use_tlsv1
syn keyword muttrcVarBool	contained ssl_usesystemcerts status_on_top strict_threads suspend text_flowed thorough_search
syn keyword muttrcVarBool	contained thread_received tilde uncollapse_jump use_8bitmime use_domain use_from use_ipv6
syn keyword muttrcVarBool	contained user_agent wait_key weed wrap_search write_bcc

syn keyword muttrcVarBool	contained noallow_8bit noallow_ansi noarrow_cursor noascii_chars noaskbcc noaskcc
syn keyword muttrcVarBool	contained noattach_split noauto_tag noautoedit nobeep nobeep_new nobounce_delivered
syn keyword muttrcVarBool	contained nocheck_new nocollapse_unread noconfirmappend noconfirmcreate nodelete_untag
syn keyword muttrcVarBool	contained nodigest_collapse noduplicate_threads noedit_hdrs noedit_headers noencode_from
syn keyword muttrcVarBool	contained noenvelope_from nofast_reply nofcc_attach nofcc_clear nofollowup_to noforce_name
syn keyword muttrcVarBool	contained noforw_decode noforw_decrypt noforw_quote noforward_decode noforward_decrypt
syn keyword muttrcVarBool	contained noforward_quote nohdrs noheader nohelp nohidden_host nohide_limited nohide_missing
syn keyword muttrcVarBool	contained nohide_top_limited nohide_top_missing noignore_list_reply_to noimap_force_ssl
syn keyword muttrcVarBool	contained noimap_list_subscribed noimap_passive noimap_peek noimap_servernoise
syn keyword muttrcVarBool	contained noimplicit_autoview nokeep_flagged nomailcap_sanitize nomaildir_trash nomark_old
syn keyword muttrcVarBool	contained nomarkers nomenu_scroll nometa_key nometoo nomh_purge nomime_forward_decode
syn keyword muttrcVarBool	contained nopager_stop nopgp_autoencrypt nopgp_autosign nopgp_ignore_subkeys nopgp_long_ids
syn keyword muttrcVarBool	contained nopgp_replyencrypt nopgp_replysign nopgp_replysignencrypted nopgp_retainable_sigs
syn keyword muttrcVarBool	contained nopgp_show_unusable nopgp_strict_enc nopipe_decode nopipe_split nopop_auth_try_all
syn keyword muttrcVarBool	contained nopop_last noprint_decode noprint_split noprompt_after noread_only noreply_self
syn keyword muttrcVarBool	contained noresolve noreverse_alias noreverse_name noreverse_realname norfc2047_parameters
syn keyword muttrcVarBool	contained nosave_address nosave_empty nosave_name noscore nosig_dashes nosig_on_top
syn keyword muttrcVarBool	contained nosmart_wrap nosort_re nossl_use_sslv2 nossl_use_sslv3 nossl_use_tlsv1
syn keyword muttrcVarBool	contained nossl_usesystemcerts nostatus_on_top nostrict_threads nosuspend notext_flowed
syn keyword muttrcVarBool	contained nothorough_search nothread_received notilde nouncollapse_jump nouse_8bitmime
syn keyword muttrcVarBool	contained nouse_domain nouse_from nouse_ipv6 nouser_agent nowait_key noweed nowrap_search
syn keyword muttrcVarBool	contained nowrite_bcc

syn keyword muttrcVarBool	contained invallow_8bit invallow_ansi invarrow_cursor invascii_chars invaskbcc invaskcc
syn keyword muttrcVarBool	contained invattach_split invauto_tag invautoedit invbeep invbeep_new invbounce_delivered
syn keyword muttrcVarBool	contained invcheck_new invcollapse_unread invconfirmappend invconfirmcreate invdelete_untag
syn keyword muttrcVarBool	contained invdigest_collapse invduplicate_threads invedit_hdrs invedit_headers invencode_from
syn keyword muttrcVarBool	contained invenvelope_from invfast_reply invfcc_attach invfcc_clear invfollowup_to invforce_name
syn keyword muttrcVarBool	contained invforw_decode invforw_decrypt invforw_quote invforward_decode invforward_decrypt
syn keyword muttrcVarBool	contained invforward_quote invhdrs invheader invhelp invhidden_host invhide_limited
syn keyword muttrcVarBool	contained invhide_missing invhide_top_limited invhide_top_missing invignore_list_reply_to
syn keyword muttrcVarBool	contained invimap_force_ssl invimap_list_subscribed invimap_passive invimap_peek
syn keyword muttrcVarBool	contained invimap_servernoise invimplicit_autoview invkeep_flagged invmailcap_sanitize
syn keyword muttrcVarBool	contained invmaildir_trash invmark_old invmarkers invmenu_scroll invmeta_key invmetoo
syn keyword muttrcVarBool	contained invmh_purge invmime_forward_decode invpager_stop invpgp_autoencrypt invpgp_autosign
syn keyword muttrcVarBool	contained invpgp_ignore_subkeys invpgp_long_ids invpgp_replyencrypt invpgp_replysign
syn keyword muttrcVarBool	contained invpgp_replysignencrypted invpgp_retainable_sigs invpgp_show_unusable invpgp_strict_enc
syn keyword muttrcVarBool	contained invpipe_decode invpipe_split invpop_auth_try_all invpop_last invprint_decode
syn keyword muttrcVarBool	contained invprint_split invprompt_after invread_only invreply_self invresolve invreverse_alias
syn keyword muttrcVarBool	contained invreverse_name invreverse_realname invrfc2047_parameters invsave_address invsave_empty
syn keyword muttrcVarBool	contained invsave_name invscore invsig_dashes invsig_on_top invsmart_wrap invsort_re
syn keyword muttrcVarBool	contained invssl_use_sslv2 invssl_use_sslv3 invssl_use_tlsv1 invssl_usesystemcerts
syn keyword muttrcVarBool	contained invstatus_on_top invstrict_threads invsuspend invtext_flowed invthorough_search
syn keyword muttrcVarBool	contained invthread_received invtilde invuncollapse_jump invuse_8bitmime invuse_domain invuse_from
syn keyword muttrcVarBool	contained invuse_ipv6 invuser_agent invwait_key invweed invwrap_search invwrite_bcc

syn keyword muttrcVarQuad	contained abort_nosubject abort_unmodified copy delete honor_followup_to include mime_forward
syn keyword muttrcVarQuad	contained mime_forward_rest mime_fwd move pgp_create_traditional pgp_verify_sig pop_delete
syn keyword muttrcVarQuad	contained pop_reconnect postpone print quit recall reply_to ssl_starttls

syn keyword muttrcVarQuad	contained noabort_nosubject noabort_unmodified nocopy nodelete nohonor_followup_to
syn keyword muttrcVarQuad	contained noinclude nomime_forward nocontained nomime_forward_rest nomime_fwd nomove
syn keyword muttrcVarQuad	contained nopgp_create_traditional nopgp_verify_sig nopop_delete nopop_reconnect
syn keyword muttrcVarQuad	contained nopostpone noprint noquit norecall noreply_to nossl_starttls

syn keyword muttrcVarQuad	contained invabort_nosubject invabort_unmodified invcopy invdelete invhonor_followup_to
syn keyword muttrcVarQuad	contained invinclude invmime_forward invcontained invmime_forward_rest invmime_fwd invmove
syn keyword muttrcVarQuad	contained invpgp_create_traditional invpgp_verify_sig invpop_delete invpop_reconnect
syn keyword muttrcVarQuad	contained invpostpone invprint invquit invrecall invreply_to invssl_starttls

syn keyword muttrcVarNum	contained connect_timeout history imap_keepalive mail_check pager_context pager_index_lines
syn keyword muttrcVarNum	contained pgp_timeout pop_checkinterval read_inc score_threshold_delete score_threshold_flag
syn keyword muttrcVarNum	contained score_threshold_read sendmail_wait sleep_time timeout wrapmargin write_inc

syn keyword muttrcVarStr	contained alias_file alias_format alternates attach_format attach_sep attribution certificate_file
syn keyword muttrcVarStr	contained charset compose_format date_format default_hook display_filter dotlock_program dsn_notify
syn keyword muttrcVarStr	contained dsn_return editor entropy_file escape folder folder_format forw_format forward_format
syn keyword muttrcVarStr	contained from gecos_mask hdr_format hostname imap_authenticators imap_delim_chars
syn keyword muttrcVarStr	contained imap_home_namespace imap_pass imap_user indent_str indent_string index_format ispell
syn keyword muttrcVarStr	contained locale mailcap_path mask mbox mbox_type message_format mh_seq_flagged mh_seq_replied
syn keyword muttrcVarStr	contained mh_seq_unseen mix_entry_format mixmaster msg_format pager pager_format
syn keyword muttrcVarStr	contained pgp_clearsign_command pgp_decode_command pgp_decrypt_command pgp_encrypt_only_command
syn keyword muttrcVarStr	contained pgp_encrypt_sign_command pgp_entry_format pgp_export_command pgp_getkeys_command
syn keyword muttrcVarStr	contained pgp_good_sign pgp_import_command pgp_list_pubring_command pgp_list_secring_command
syn keyword muttrcVarStr	contained pgp_sign_as pgp_sign_command pgp_sort_keys pgp_verify_command pgp_verify_key_command
syn keyword muttrcVarStr	contained pipe_sep pop_authenticators pop_host pop_pass pop_user post_indent_str post_indent_string
syn keyword muttrcVarStr	contained postponed preconnect print_cmd print_command query_command quote_regexp realname record
syn keyword muttrcVarStr	contained reply_regexp send_charset sendmail shell signature simple_search smileys sort sort_alias
syn keyword muttrcVarStr	contained sort_aux sort_browser spoolfile status_chars status_format tmpdir to_chars tunnel visual

syn keyword muttrcMenu		contained alias attach browser compose editor index pager postpone pgp mix query generic

syn keyword muttrcCommand	account-hook auto_view alternative_order charset-hook uncolor exec fcc-hook fcc-save-hook
syn keyword muttrcCommand	folder-hook hdr_order iconv-hook ignore lists mailboxes message-hook mbox-hook my_hdr
syn keyword muttrcCommand	pgp-hook push save-hook score send-hook source subscribe unalias unauto_view unhdr_order
syn keyword muttrcCommand	unhook unignore unlists unmono unmy_hdr unscore unsubscribe

syn keyword muttrcSet		set     skipwhite nextgroup=muttrcVar.*
syn keyword muttrcUnset		unset   skipwhite nextgroup=muttrcVar.*
syn keyword muttrcReset		reset   skipwhite nextgroup=muttrcVar.*
syn keyword muttrcToggle	toggle  skipwhite nextgroup=muttrcVar.*

syn keyword muttrcBind		contained bind		skipwhite nextgroup=muttrcMenu
syn match   muttrcBindLine	"^\s*bind\s\+\S\+"	skipwhite nextgroup=muttrcKey\(Name\)\= contains=muttrcBind

syn keyword muttrcMacro		contained macro		skipwhite nextgroup=muttrcMenu
syn match   muttrcMacroLine	"^\s*macro\s\+\S\+"	skipwhite nextgroup=muttrcKey\(Name\)\= contains=muttrcMacro

syn keyword muttrcAlias		contained alias
syn match   muttrcAliasLine	"^\s*alias\s\+\S\+" contains=muttrcAlias

" Colour definitions takes object, foreground and background arguments (regexps excluded).
syn keyword muttrcColorField	contained attachment body bold error hdrdefault header index
syn keyword muttrcColorField	contained indicator markers message normal quoted search signature
syn keyword muttrcColorField	contained status tilde tree underline
syn match   muttrcColorField	contained "\<quoted\d\=\>"
syn keyword muttrcColorFG	contained black blue cyan default green magenta red white yellow
syn keyword muttrcColorFG	contained brightblack brightblue brightcyan brightdefault brightgreen
syn keyword muttrcColorFG	contained brightmagenta brightred brightwhite brightyellow
syn match   muttrcColorFG	contained "\<\(bright\)\=color\d\{1,2}\>"
syn keyword muttrcColorBG	contained black blue cyan default green magenta red white yellow
syn match   muttrcColorBG	contained "\<color\d\{1,2}\>"
" Now for the match
syn keyword muttrcColor		contained color			skipwhite nextgroup=muttrcColorField
syn match   muttrcColorInit	contained "^\s*color\s\+\S\+"	skipwhite nextgroup=muttrcColorFG contains=muttrcColor
syn match   muttrcColorLine	"^\s*color\s\+\S\+\s\+\S"	skipwhite nextgroup=muttrcColorBG contains=muttrcColorInit

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
  HiLink muttrcString		String
  HiLink muttrcSpecial		Special
  HiLink muttrcNumber		Number
  HiLink muttrcQuadopt		Boolean
  HiLink muttrcEmail		Special
  HiLink muttrcHeader		Type
  HiLink muttrcKeySpecial	SpecialChar
  HiLink muttrcKey		Type
  HiLink muttrcKeyName		Macro
  HiLink muttrcVarBool		Identifier
  HiLink muttrcVarQuad		Identifier
  HiLink muttrcVarNum		Identifier
  HiLink muttrcVarStr		Identifier
  HiLink muttrcMenu		Identifier
  HiLink muttrcCommand		Keyword
  HiLink muttrcSet		muttrcCommand
  HiLink muttrcUnset		muttrcCommand
  HiLink muttrcReset		muttrcCommand
  HiLink muttrcToggle		muttrcCommand
  HiLink muttrcBind		muttrcCommand
  HiLink muttrcMacro		muttrcCommand
  HiLink muttrcAlias		muttrcCommand
  HiLink muttrcAliasLine	Identifier
  HiLink muttrcColorField	Identifier
  HiLink muttrcColorFG		String
  HiLink muttrcColorBG		muttrcColorFG
  HiLink muttrcColor		muttrcCommand
  HiLink muttrcMonoAttrib	muttrcColorFG
  HiLink muttrcMono		muttrcCommand

  delcommand HiLink
endif

let b:current_syntax = "muttrc"

"EOF	vim: ts=8 noet tw=100 sw=8 sts=0
