" Vim syntax file
" Language:	    elinks(1) configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/
" Latest Revision:  2004-05-22
" arch-tag:	    74eaff55-cdb5-4d31-805b-9627eb6535f1

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
SetIsk @,48-57,_,-
delcommand SetIsk

" Todo
syn keyword elinksTodo	    contained TODO FIXME XXX NOTE

" Comments
syn region  elinksComment   matchgroup=elinksComment start='#' end='$' contains=elinksTodo

" Numbers
syn match   elinksNumber    '\<\d\+\>'

" Strings
syn region  elinksString    matchgroup=elinksString start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=@elinksColor

" Keywords
syn keyword elinksKeyword   set bind

" Options
syn keyword elinksPrefix    bookmarks
syn keyword elinksOptions   file_format

syn keyword elinksPrefix    config
syn keyword elinksOptions   comments indentation saving_style i18n
syn keyword elinksOptions   saving_style_w show_template

syn keyword elinksPrefix    connection ssl client_cert
syn keyword elinksOptions   enable file cert_verify async_dns max_connections
syn keyword elinksOptions   max_connections_to_host receive_timeout retries
syn keyword elinksOptions   unrestartable_receive_timeout

syn keyword elinksPrefix    cookies
syn keyword elinksOptions   accept_policy max_age paranoid_security save resave

syn keyword elinksPrefix    document browse accesskey forms images links
syn keyword elinksPrefix    active_link colors search cache codepage colors
syn keyword elinksPrefix    format memory download dump history global html
syn keyword elinksPrefix    plain
syn keyword elinksOptions   auto_follow priority auto_submit confirm_submit
syn keyword elinksOptions   input_size show_formhist file_tags
syn keyword elinksOptions   image_link_tagging image_link_prefix
syn keyword elinksOptions   image_link_suffix show_as_links show_any_as_links
syn keyword elinksOptions   background text enable_color bold invert underline
syn keyword elinksOptions   color_dirs numbering use_tabindex
syn keyword elinksOptions   number_keys_select_link wraparound case regex
syn keyword elinksOptions   show_hit_top_bottom wraparound show_not_found
syn keyword elinksOptions   margin_width refresh minimum_refresh_time
syn keyword elinksOptions   scroll_margin scroll_step table_move_order size
syn keyword elinksOptions   size cache_redirects ignore_cache_control assume
syn keyword elinksOptions   force_assumed text background link vlink dirs
syn keyword elinksOptions   allow_dark_on_black ensure_contrast
syn keyword elinksOptions   use_document_colors directory set_original_time
syn keyword elinksOptions   overwrite notify_bell codepage width enable
syn keyword elinksOptions   max_items display_type write_interval
syn keyword elinksOptions   keep_unhistory display_frames display_tables
syn keyword elinksOptions   expand_table_columns display_subs display_sups
syn keyword elinksOptions   link_display underline_links wrap_nbsp
syn keyword elinksOptions   display_links compress_empty_lines

syn keyword elinksPrefix    mime extension handler mailcap mimetypes type
syn keyword elinksOptions   ask block program enable path ask description
syn keyword elinksOptions   prioritize enable path default_type

syn keyword elinksPrefix    protocol file cgi ftp proxy http bugs proxy
syn keyword elinksPrefix    referer https proxy rewrite dumb smart
syn keyword elinksOptions   path policy allow_special_files show_hidden_files
syn keyword elinksOptions   try_encoding_extensions host anon_passwd use_pasv
syn keyword elinksOptions   use_epsv accept_charset allow_blacklist
syn keyword elinksOptions   broken_302_redirect post_no_keepalive http10 host
syn keyword elinksOptions   user passwd policy fake accept_language
syn keyword elinksOptions   accept_ui_language trace user_agent host
syn keyword elinksOptions   enable-dumb enable-smart

syn keyword elinksPrefix    terminal
syn keyword elinksOptions   type m11_hack utf_8_io restrict_852 block_cursor
syn keyword elinksOptions   colors transparency underline charset

syn keyword elinksPrefix    ui colors color mainmenu normal selected hotkey
syn keyword elinksPrefix    menu marked hotkey frame dialog generic frame
syn keyword elinksPrefix    scrollbar scrollbar-selected title text checkbox
syn keyword elinksPrefix    checkbox-label button button-selected field
syn keyword elinksPrefix    field-text meter shadow title title-bar title-text
syn keyword elinksPrefix    status status-bar status-text tabs unvisited normal
syn keyword elinksPrefix    loading separator searched mono
syn keyword elinksOptions   text background

syn keyword elinksPrefix    ui dialogs leds sessions tabs timer
syn keyword elinksOptions   listbox_min_height shadows underline_hotkeys enable
syn keyword elinksOptions   auto_save auto_restore auto_save_foldername
syn keyword elinksOptions   homepage show_bar wraparound confirm_close enable
syn keyword elinksOptions   duration action language show_status_bar
syn keyword elinksOptions   show_title_bar startup_goto_dialog success_msgbox
syn keyword elinksOptions   window_title

syn keyword elinksOptions   secure_file_saving

" Colors
syn cluster elinksColor contains=elinksColorBlack,elinksColorDarkRed,elinksColorDarkGreen,elinksColorDarkYellow,elinksColorDarkBlue,elinksColorDarkMagenta,elinksColorDarkCyan,elinksColorGray,elinksColorDarkGray,elinksColorRed,elinksColorGreen,elinksColorYellow,elinksColorBlue,elinksColorMagenta,elinksColorCyan,elinksColorWhite

syn keyword elinksColorBlack	    black contained
syn keyword elinksColorDarkRed	    darkred sandybrown maroon crimson firebrick contained
syn keyword elinksColorDarkGreen    darkgreen darkolivegreen darkseagreen contained
syn keyword elinksColorDarkGreen    forestgreen mediumspringgreen seagreen contained
syn keyword elinksColorDarkYellow   brown blanchedalmond chocolate darkorange contained
syn keyword elinksColorDarkYellow   darkgoldenrod orange rosybrown saddlebrown contained
syn keyword elinksColorDarkYellow   peru olive olivedrab sienna contained
syn keyword elinksColorDarkBlue	    darkblue cadetblue cornflowerblue contained
syn keyword elinksColorDarkBlue	    darkslateblue deepskyblue midnightblue contained
syn keyword elinksColorDarkBlue	    royalblue steelblue navy contained
syn keyword elinksColorDarkMagenta  darkmagenta mediumorchid mediumpurple contained
syn keyword elinksColorDarkMagenta  mediumslateblue slateblue deeppink hotpink contained
syn keyword elinksColorDarkMagenta  darkorchid orchid purple indigo contained
syn keyword elinksColorDarkCyan	    darkcyan mediumaquamarine mediumturquoise contained
syn keyword elinksColorDarkCyan	    darkturquoise teal contained
syn keyword elinksColorGray	    silver dimgray lightslategray slategray contained
syn keyword elinksColorGray	    lightgrey burlywood plum tan thistle contained

syn keyword elinksColorDarkGray	    gray darkgray darkslategray darksalmon contained
syn keyword elinksColorRed	    red indianred orangered tomato lightsalmon contained
syn keyword elinksColorRed	    salmon coral lightcoral contained
syn keyword elinksColorGreen	    green greenyellow lawngreen lightgreen contained
syn keyword elinksColorGreen	    lightseagreen limegreen mediumseagreen contained
syn keyword elinksColorGreen	    springgreen yellowgreen palegreen lime contained
syn keyword elinksColorGreen	    chartreuse contained
syn keyword elinksColorYellow	    yellow beige darkkhaki lightgoldenrodyellow contained
syn keyword elinksColorYellow	    palegoldenrod gold goldenrod khaki contained
syn keyword elinksColorYellow	    lightyellow contained
syn keyword elinksColorBlue	    blue aliceblue aqua aquamarine azure contained
syn keyword elinksColorBlue	    dodgerblue lightblue lightskyblue contained
syn keyword elinksColorBlue	    lightsteelblue mediumblue contained
syn keyword elinksColorMagenta	    magenta darkviolet blueviolet lightpink contained
syn keyword elinksColorMagenta	    mediumvioletred palevioletred violet pink contained
syn keyword elinksColorMagenta	    fuchsia contained
syn keyword elinksColorCyan	    cyan lightcyan powderblue skyblue turquoise contained
syn keyword elinksColorCyan	    paleturquoise contained
syn keyword elinksColorWhite	    white antiquewhite floralwhite ghostwhite contained
syn keyword elinksColorWhite	    navajowhite whitesmoke linen lemonchiffon contained
syn keyword elinksColorWhite	    cornsilk lavender lavenderblush seashell contained
syn keyword elinksColorWhite	    mistyrose ivory papayawhip bisque gainsboro contained
syn keyword elinksColorWhite	    honeydew mintcream moccasin oldlace contained
syn keyword elinksColorWhite	    peachpuff snow wheat contained

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_elinks_syn_inits")
  if version < 508
    let did_elinks_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
    command -nargs=+ HiDef hi <args>
  else
    command -nargs=+ HiLink hi def link <args>
    command -nargs=+ HiDef hi def <args>
  endif

  HiLink elinksTodo		Todo
  HiLink elinksComment		Comment
  HiLink elinksNumber		Number
  HiLink elinksString		String
  HiLink elinksKeyword		Keyword
  HiLink elinksPrefix		Identifier
  HiLink elinksOptions		Identifier
  HiDef  elinksColorBlack	ctermfg=Black	    guifg=Black
  HiDef  elinksColorDarkRed	ctermfg=DarkRed	    guifg=DarkRed
  HiDef  elinksColorDarkGreen	ctermfg=DarkGreen   guifg=DarkGreen
  HiDef  elinksColorDarkYellow	ctermfg=DarkYellow  guifg=DarkYellow
  HiDef  elinksColorDarkBlue	ctermfg=DarkBlue    guifg=DarkBlue
  HiDef  elinksColorDarkMagenta	ctermfg=DarkMagenta guifg=DarkMagenta
  HiDef  elinksColorDarkCyan	ctermfg=DarkCyan    guifg=DarkCyan
  HiDef  elinksColorGray	ctermfg=Gray	    guifg=Gray
  HiDef  elinksColorDarkGray	ctermfg=DarkGray    guifg=DarkGray
  HiDef  elinksColorRed		ctermfg=Red	    guifg=Red
  HiDef  elinksColorGreen	ctermfg=Green	    guifg=Green
  HiDef  elinksColorYellow	ctermfg=Yellow	    guifg=Yellow
  HiDef  elinksColorBlue	ctermfg=Blue	    guifg=Blue
  HiDef  elinksColorMagenta	ctermfg=Magenta	    guifg=Magenta
  HiDef  elinksColorCyan	ctermfg=Cyan	    guifg=Cyan
  HiDef  elinksColorWhite	ctermfg=White	    guifg=White

  delcommand HiLink
  delcommand HiDef
endif

let b:current_syntax = "elinks"

" vim: set sts=2 sw=2:
