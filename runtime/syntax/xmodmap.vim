" Vim syntax file
" Language:	    xmodmap definition file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/xmodmap/
" Latest Revision:  2004-05-22
" arch-tag:	    8c37ed41-655a-479d-8050-e15dc6770338

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" comments
syn region  xmodmapComment	display oneline matchgroup=xmodmapComment start=/^!/ end=/$/ contains=xmodmapTodo

" todo
syn keyword xmodmapTodo		contained TODO FIXME XXX NOTE

" numbers
syn case ignore
syn match   xmodmapInt		display "\<\d\+\>"
syn match   xmodmapHex		display "\<0x\x\+\>"
syn match   xmodmapOctal	display "\<0\o\+\>"
syn match   xmodmapOctalError	display "\<0\o*[89]\d*"
syn case match

" keysyms (taken from <X11/keysymdef.h>)
syn keyword xmodmapKeySym	VoidSymbol BackSpace Tab Linefeed Clear Return Pause Scroll_Lock Sys_Req Escape Delete Multi_key Codeinput SingleCandidate MultipleCandidate
syn keyword xmodmapKeySym	PreviousCandidate Kanji Muhenkan Henkan_Mode Henkan Romaji Hiragana Katakana Hiragana_Katakana Zenkaku Hankaku Zenkaku_Hankaku Touroku Massyo Kana_Lock
syn keyword xmodmapKeySym	Kana_Shift Eisu_Shift Eisu_toggle Kanji_Bangou Zen_Koho Mae_Koho Home Left Up Right Down Prior Page_Up Next Page_Down
syn keyword xmodmapKeySym	End Begin Select Print Execute Insert Undo Redo Menu Find Cancel Help Break Mode_switch script_switch
syn keyword xmodmapKeySym	Num_Lock KP_Space KP_Tab KP_Enter KP_F1 KP_F2 KP_F3 KP_F4 KP_Home KP_Left KP_Up KP_Right KP_Down KP_Prior KP_Page_Up
syn keyword xmodmapKeySym	KP_Next KP_Page_Down KP_End KP_Begin KP_Insert KP_Delete KP_Equal KP_Multiply KP_Add KP_Separator KP_Subtract KP_Decimal KP_Divide KP_0 KP_1
syn keyword xmodmapKeySym	KP_2 KP_3 KP_4 KP_5 KP_6 KP_7 KP_8 KP_9 F1 F2 F3 F4 F5 F6 F7
syn keyword xmodmapKeySym	F8 F9 F10 F11 L1 F12 L2 F13 L3 F14 L4 F15 L5 F16 L6
syn keyword xmodmapKeySym	F17 L7 F18 L8 F19 L9 F20 L10 F21 R1 F22 R2 F23 R3 F24
syn keyword xmodmapKeySym	R4 F25 R5 F26 R6 F27 R7 F28 R8 F29 R9 F30 R10 F31 R11
syn keyword xmodmapKeySym	F32 R12 F33 R13 F34 R14 F35 R15 Shift_L Shift_R Control_L Control_R Caps_Lock Shift_Lock Meta_L
syn keyword xmodmapKeySym	Meta_R Alt_L Alt_R Super_L Super_R Hyper_L Hyper_R ISO_Lock ISO_Level2_Latch ISO_Level3_Shift ISO_Level3_Latch ISO_Level3_Lock ISO_Group_Shift ISO_Group_Latch ISO_Group_Lock
syn keyword xmodmapKeySym	ISO_Next_Group ISO_Next_Group_Lock ISO_Prev_Group ISO_Prev_Group_Lock ISO_First_Group ISO_First_Group_Lock ISO_Last_Group ISO_Last_Group_Lock ISO_Left_Tab ISO_Move_Line_Up ISO_Move_Line_Down ISO_Partial_Line_Up ISO_Partial_Line_Down ISO_Partial_Space_Left ISO_Partial_Space_Right
syn keyword xmodmapKeySym	ISO_Set_Margin_Left ISO_Set_Margin_Right ISO_Release_Margin_Left ISO_Release_Margin_Right ISO_Release_Both_Margins ISO_Fast_Cursor_Left ISO_Fast_Cursor_Right ISO_Fast_Cursor_Up ISO_Fast_Cursor_Down ISO_Continuous_Underline ISO_Discontinuous_Underline ISO_Emphasize ISO_Center_Object ISO_Enter dead_grave
syn keyword xmodmapKeySym	dead_acute dead_circumflex dead_tilde dead_macron dead_breve dead_abovedot dead_diaeresis dead_abovering dead_doubleacute dead_caron dead_cedilla dead_ogonek dead_iota dead_voiced_sound dead_semivoiced_sound
syn keyword xmodmapKeySym	dead_belowdot dead_hook dead_horn First_Virtual_Screen Prev_Virtual_Screen Next_Virtual_Screen Last_Virtual_Screen Terminate_Server AccessX_Enable AccessX_Feedback_Enable RepeatKeys_Enable SlowKeys_Enable BounceKeys_Enable StickyKeys_Enable MouseKeys_Enable
syn keyword xmodmapKeySym	MouseKeys_Accel_Enable Overlay1_Enable Overlay2_Enable AudibleBell_Enable Pointer_Left Pointer_Right Pointer_Up Pointer_Down Pointer_UpLeft Pointer_UpRight Pointer_DownLeft Pointer_DownRight Pointer_Button_Dflt Pointer_Button1 Pointer_Button2
syn keyword xmodmapKeySym	Pointer_Button3 Pointer_Button4 Pointer_Button5 Pointer_DblClick_Dflt Pointer_DblClick1 Pointer_DblClick2 Pointer_DblClick3 Pointer_DblClick4 Pointer_DblClick5 Pointer_Drag_Dflt Pointer_Drag1 Pointer_Drag2 Pointer_Drag3 Pointer_Drag4 Pointer_Drag5
syn keyword xmodmapKeySym	Pointer_EnableKeys Pointer_Accelerate Pointer_DfltBtnNext Pointer_DfltBtnPrev 3270_Duplicate 3270_FieldMark 3270_Right2 3270_Left2 3270_BackTab 3270_EraseEOF 3270_EraseInput 3270_Reset 3270_Quit 3270_PA1 3270_PA2
syn keyword xmodmapKeySym	3270_PA3 3270_Test 3270_Attn 3270_CursorBlink 3270_AltCursor 3270_KeyClick 3270_Jump 3270_Ident 3270_Rule 3270_Copy 3270_Play 3270_Setup 3270_Record 3270_ChangeScreen 3270_DeleteWord
syn keyword xmodmapKeySym	3270_ExSelect 3270_CursorSelect 3270_PrintScreen 3270_Enter space exclam quotedbl numbersign dollar percent ampersand apostrophe quoteright parenleft parenright
syn keyword xmodmapKeySym	asterisk plus comma minus period slash 0 1 2 3 4 5 6 7 8
syn keyword xmodmapKeySym	9 colon semicolon less equal greater question at A B C D E F G
syn keyword xmodmapKeySym	H I J K L M N O P Q R S T U V
syn keyword xmodmapKeySym	W X Y Z bracketleft backslash bracketright asciicircum underscore grave quoteleft a b c d
syn keyword xmodmapKeySym	e f g h i j k l m n o p q r s
syn keyword xmodmapKeySym	t u v w x y z braceleft bar braceright asciitilde nobreakspace exclamdown cent sterling
syn keyword xmodmapKeySym	currency yen brokenbar section diaeresis copyright ordfeminine guillemotleft notsign hyphen registered macron degree plusminus twosuperior
syn keyword xmodmapKeySym	threesuperior acute mu paragraph periodcentered cedilla onesuperior masculine guillemotright onequarter onehalf threequarters questiondown Agrave Aacute
syn keyword xmodmapKeySym	Acircumflex Atilde Adiaeresis Aring AE Ccedilla Egrave Eacute Ecircumflex Ediaeresis Igrave Iacute Icircumflex Idiaeresis ETH
syn keyword xmodmapKeySym	Eth Ntilde Ograve Oacute Ocircumflex Otilde Odiaeresis multiply Ooblique Oslash Ugrave Uacute Ucircumflex Udiaeresis Yacute
syn keyword xmodmapKeySym	THORN Thorn ssharp agrave aacute acircumflex atilde adiaeresis aring ae ccedilla egrave eacute ecircumflex ediaeresis
syn keyword xmodmapKeySym	igrave iacute icircumflex idiaeresis eth ntilde ograve oacute ocircumflex otilde odiaeresis division oslash ooblique ugrave
syn keyword xmodmapKeySym	uacute ucircumflex udiaeresis yacute thorn ydiaeresis Aogonek breve Lstroke Lcaron Sacute Scaron Scedilla Tcaron Zacute
syn keyword xmodmapKeySym	Zcaron Zabovedot aogonek ogonek lstroke lcaron sacute caron scaron scedilla tcaron zacute doubleacute zcaron zabovedot
syn keyword xmodmapKeySym	Racute Abreve Lacute Cacute Ccaron Eogonek Ecaron Dcaron Dstroke Nacute Ncaron Odoubleacute Rcaron Uring Udoubleacute
syn keyword xmodmapKeySym	Tcedilla racute abreve lacute cacute ccaron eogonek ecaron dcaron dstroke nacute ncaron odoubleacute udoubleacute rcaron
syn keyword xmodmapKeySym	uring tcedilla abovedot Hstroke Hcircumflex Iabovedot Gbreve Jcircumflex hstroke hcircumflex idotless gbreve jcircumflex Cabovedot Ccircumflex
syn keyword xmodmapKeySym	Gabovedot Gcircumflex Ubreve Scircumflex cabovedot ccircumflex gabovedot gcircumflex ubreve scircumflex kra kappa Rcedilla Itilde Lcedilla
syn keyword xmodmapKeySym	Emacron Gcedilla Tslash rcedilla itilde lcedilla emacron gcedilla tslash ENG eng Amacron Iogonek Eabovedot Imacron
syn keyword xmodmapKeySym	Ncedilla Omacron Kcedilla Uogonek Utilde Umacron amacron iogonek eabovedot imacron ncedilla omacron kcedilla uogonek utilde
syn keyword xmodmapKeySym	umacron Babovedot babovedot Dabovedot Wgrave Wacute dabovedot Ygrave Fabovedot fabovedot Mabovedot mabovedot Pabovedot wgrave pabovedot
syn keyword xmodmapKeySym	wacute Sabovedot ygrave Wdiaeresis wdiaeresis sabovedot Wcircumflex Tabovedot Ycircumflex wcircumflex tabovedot ycircumflex OE oe Ydiaeresis
syn keyword xmodmapKeySym	overline kana_fullstop kana_openingbracket kana_closingbracket kana_comma kana_conjunctive kana_middledot kana_WO kana_a kana_i kana_u kana_e kana_o kana_ya kana_yu
syn keyword xmodmapKeySym	kana_yo kana_tsu kana_tu prolongedsound kana_A kana_I kana_U kana_E kana_O kana_KA kana_KI kana_KU kana_KE kana_KO kana_SA
syn keyword xmodmapKeySym	kana_SHI kana_SU kana_SE kana_SO kana_TA kana_CHI kana_TI kana_TSU kana_TU kana_TE kana_TO kana_NA kana_NI kana_NU kana_NE
syn keyword xmodmapKeySym	kana_NO kana_HA kana_HI kana_FU kana_HU kana_HE kana_HO kana_MA kana_MI kana_MU kana_ME kana_MO kana_YA kana_YU kana_YO
syn keyword xmodmapKeySym	kana_RA kana_RI kana_RU kana_RE kana_RO kana_WA kana_N voicedsound semivoicedsound kana_switch Farsi_0 Farsi_1 Farsi_2 Farsi_3 Farsi_4
syn keyword xmodmapKeySym	Farsi_5 Farsi_6 Farsi_7 Farsi_8 Farsi_9 Arabic_percent Arabic_superscript_alef Arabic_tteh Arabic_peh Arabic_tcheh Arabic_ddal Arabic_rreh Arabic_comma Arabic_fullstop Arabic_0
syn keyword xmodmapKeySym	Arabic_1 Arabic_2 Arabic_3 Arabic_4 Arabic_5 Arabic_6 Arabic_7 Arabic_8 Arabic_9 Arabic_semicolon Arabic_question_mark Arabic_hamza Arabic_maddaonalef Arabic_hamzaonalef Arabic_hamzaonwaw
syn keyword xmodmapKeySym	Arabic_hamzaunderalef Arabic_hamzaonyeh Arabic_alef Arabic_beh Arabic_tehmarbuta Arabic_teh Arabic_theh Arabic_jeem Arabic_hah Arabic_khah Arabic_dal Arabic_thal Arabic_ra Arabic_zain Arabic_seen
syn keyword xmodmapKeySym	Arabic_sheen Arabic_sad Arabic_dad Arabic_tah Arabic_zah Arabic_ain Arabic_ghain Arabic_tatweel Arabic_feh Arabic_qaf Arabic_kaf Arabic_lam Arabic_meem Arabic_noon Arabic_ha
syn keyword xmodmapKeySym	Arabic_heh Arabic_waw Arabic_alefmaksura Arabic_yeh Arabic_fathatan Arabic_dammatan Arabic_kasratan Arabic_fatha Arabic_damma Arabic_kasra Arabic_shadda Arabic_sukun Arabic_madda_above Arabic_hamza_above Arabic_hamza_below
syn keyword xmodmapKeySym	Arabic_jeh Arabic_veh Arabic_keheh Arabic_gaf Arabic_noon_ghunna Arabic_heh_doachashmee Farsi_yeh Arabic_farsi_yeh Arabic_yeh_baree Arabic_heh_goal Arabic_switch Cyrillic_GHE_bar Cyrillic_ghe_bar Cyrillic_ZHE_descender Cyrillic_zhe_descender
syn keyword xmodmapKeySym	Cyrillic_KA_descender Cyrillic_ka_descender Cyrillic_KA_vertstroke Cyrillic_ka_vertstroke Cyrillic_EN_descender Cyrillic_en_descender Cyrillic_U_straight Cyrillic_u_straight Cyrillic_U_straight_bar Cyrillic_u_straight_bar Cyrillic_HA_descender Cyrillic_ha_descender Cyrillic_CHE_descender Cyrillic_che_descender Cyrillic_CHE_vertstroke
syn keyword xmodmapKeySym	Cyrillic_che_vertstroke Cyrillic_SHHA Cyrillic_shha Cyrillic_SCHWA Cyrillic_schwa Cyrillic_I_macron Cyrillic_i_macron Cyrillic_O_bar Cyrillic_o_bar Cyrillic_U_macron Cyrillic_u_macron Serbian_dje Macedonia_gje Cyrillic_io Ukrainian_ie
syn keyword xmodmapKeySym	Ukranian_je Macedonia_dse Ukrainian_i Ukranian_i Ukrainian_yi Ukranian_yi Cyrillic_je Serbian_je Cyrillic_lje Serbian_lje Cyrillic_nje Serbian_nje Serbian_tshe Macedonia_kje Ukrainian_ghe_with_upturn
syn keyword xmodmapKeySym	Byelorussian_shortu Cyrillic_dzhe Serbian_dze numerosign Serbian_DJE Macedonia_GJE Cyrillic_IO Ukrainian_IE Ukranian_JE Macedonia_DSE Ukrainian_I Ukranian_I Ukrainian_YI Ukranian_YI Cyrillic_JE
syn keyword xmodmapKeySym	Serbian_JE Cyrillic_LJE Serbian_LJE Cyrillic_NJE Serbian_NJE Serbian_TSHE Macedonia_KJE Ukrainian_GHE_WITH_UPTURN Byelorussian_SHORTU Cyrillic_DZHE Serbian_DZE Cyrillic_yu Cyrillic_a Cyrillic_be Cyrillic_tse
syn keyword xmodmapKeySym	Cyrillic_de Cyrillic_ie Cyrillic_ef Cyrillic_ghe Cyrillic_ha Cyrillic_i Cyrillic_shorti Cyrillic_ka Cyrillic_el Cyrillic_em Cyrillic_en Cyrillic_o Cyrillic_pe Cyrillic_ya Cyrillic_er
syn keyword xmodmapKeySym	Cyrillic_es Cyrillic_te Cyrillic_u Cyrillic_zhe Cyrillic_ve Cyrillic_softsign Cyrillic_yeru Cyrillic_ze Cyrillic_sha Cyrillic_e Cyrillic_shcha Cyrillic_che Cyrillic_hardsign Cyrillic_YU Cyrillic_A
syn keyword xmodmapKeySym	Cyrillic_BE Cyrillic_TSE Cyrillic_DE Cyrillic_IE Cyrillic_EF Cyrillic_GHE Cyrillic_HA Cyrillic_I Cyrillic_SHORTI Cyrillic_KA Cyrillic_EL Cyrillic_EM Cyrillic_EN Cyrillic_O Cyrillic_PE
syn keyword xmodmapKeySym	Cyrillic_YA Cyrillic_ER Cyrillic_ES Cyrillic_TE Cyrillic_U Cyrillic_ZHE Cyrillic_VE Cyrillic_SOFTSIGN Cyrillic_YERU Cyrillic_ZE Cyrillic_SHA Cyrillic_E Cyrillic_SHCHA Cyrillic_CHE Cyrillic_HARDSIGN
syn keyword xmodmapKeySym	Greek_ALPHAaccent Greek_EPSILONaccent Greek_ETAaccent Greek_IOTAaccent Greek_IOTAdieresis Greek_IOTAdiaeresis Greek_OMICRONaccent Greek_UPSILONaccent Greek_UPSILONdieresis Greek_OMEGAaccent Greek_accentdieresis Greek_horizbar Greek_alphaaccent Greek_epsilonaccent Greek_etaaccent
syn keyword xmodmapKeySym	Greek_iotaaccent Greek_iotadieresis Greek_iotaaccentdieresis Greek_omicronaccent Greek_upsilonaccent Greek_upsilondieresis Greek_upsilonaccentdieresis Greek_omegaaccent Greek_ALPHA Greek_BETA Greek_GAMMA Greek_DELTA Greek_EPSILON Greek_ZETA Greek_ETA
syn keyword xmodmapKeySym	Greek_THETA Greek_IOTA Greek_KAPPA Greek_LAMDA Greek_LAMBDA Greek_MU Greek_NU Greek_XI Greek_OMICRON Greek_PI Greek_RHO Greek_SIGMA Greek_TAU Greek_UPSILON Greek_PHI
syn keyword xmodmapKeySym	Greek_CHI Greek_PSI Greek_OMEGA Greek_alpha Greek_beta Greek_gamma Greek_delta Greek_epsilon Greek_zeta Greek_eta Greek_theta Greek_iota Greek_kappa Greek_lamda Greek_lambda
syn keyword xmodmapKeySym	Greek_mu Greek_nu Greek_xi Greek_omicron Greek_pi Greek_rho Greek_sigma Greek_finalsmallsigma Greek_tau Greek_upsilon Greek_phi Greek_chi Greek_psi Greek_omega Greek_switch
syn keyword xmodmapKeySym	leftradical topleftradical horizconnector topintegral botintegral vertconnector topleftsqbracket botleftsqbracket toprightsqbracket botrightsqbracket topleftparens botleftparens toprightparens botrightparens leftmiddlecurlybrace
syn keyword xmodmapKeySym	rightmiddlecurlybrace topleftsummation botleftsummation topvertsummationconnector botvertsummationconnector toprightsummation botrightsummation rightmiddlesummation lessthanequal notequal greaterthanequal integral therefore variation infinity
syn keyword xmodmapKeySym	nabla approximate similarequal ifonlyif implies identical radical includedin includes intersection union logicaland logicalor partialderivative function
syn keyword xmodmapKeySym	leftarrow uparrow rightarrow downarrow blank soliddiamond checkerboard ht ff cr lf nl vt lowrightcorner uprightcorner
syn keyword xmodmapKeySym	upleftcorner lowleftcorner crossinglines horizlinescan1 horizlinescan3 horizlinescan5 horizlinescan7 horizlinescan9 leftt rightt bott topt vertbar emspace enspace
syn keyword xmodmapKeySym	em3space em4space digitspace punctspace thinspace hairspace emdash endash signifblank ellipsis doubbaselinedot onethird twothirds onefifth twofifths
syn keyword xmodmapKeySym	threefifths fourfifths onesixth fivesixths careof figdash leftanglebracket decimalpoint rightanglebracket marker oneeighth threeeighths fiveeighths seveneighths trademark
syn keyword xmodmapKeySym	signaturemark trademarkincircle leftopentriangle rightopentriangle emopencircle emopenrectangle leftsinglequotemark rightsinglequotemark leftdoublequotemark rightdoublequotemark prescription minutes seconds latincross hexagram
syn keyword xmodmapKeySym	filledrectbullet filledlefttribullet filledrighttribullet emfilledcircle emfilledrect enopencircbullet enopensquarebullet openrectbullet opentribulletup opentribulletdown openstar enfilledcircbullet enfilledsqbullet filledtribulletup filledtribulletdown
syn keyword xmodmapKeySym	leftpointer rightpointer club diamond heart maltesecross dagger doubledagger checkmark ballotcross musicalsharp musicalflat malesymbol femalesymbol telephone
syn keyword xmodmapKeySym	telephonerecorder phonographcopyright caret singlelowquotemark doublelowquotemark cursor leftcaret rightcaret downcaret upcaret overbar downtack upshoe downstile underbar
syn keyword xmodmapKeySym	jot quad uptack circle upstile downshoe rightshoe leftshoe lefttack righttack hebrew_doublelowline hebrew_aleph hebrew_bet hebrew_beth hebrew_gimel
syn keyword xmodmapKeySym	hebrew_gimmel hebrew_dalet hebrew_daleth hebrew_he hebrew_waw hebrew_zain hebrew_zayin hebrew_chet hebrew_het hebrew_tet hebrew_teth hebrew_yod hebrew_finalkaph hebrew_kaph hebrew_lamed
syn keyword xmodmapKeySym	hebrew_finalmem hebrew_mem hebrew_finalnun hebrew_nun hebrew_samech hebrew_samekh hebrew_ayin hebrew_finalpe hebrew_pe hebrew_finalzade hebrew_finalzadi hebrew_zade hebrew_zadi hebrew_qoph hebrew_kuf
syn keyword xmodmapKeySym	hebrew_resh hebrew_shin hebrew_taw hebrew_taf Hebrew_switch Thai_kokai Thai_khokhai Thai_khokhuat Thai_khokhwai Thai_khokhon Thai_khorakhang Thai_ngongu Thai_chochan Thai_choching Thai_chochang
syn keyword xmodmapKeySym	Thai_soso Thai_chochoe Thai_yoying Thai_dochada Thai_topatak Thai_thothan Thai_thonangmontho Thai_thophuthao Thai_nonen Thai_dodek Thai_totao Thai_thothung Thai_thothahan Thai_thothong Thai_nonu
syn keyword xmodmapKeySym	Thai_bobaimai Thai_popla Thai_phophung Thai_fofa Thai_phophan Thai_fofan Thai_phosamphao Thai_moma Thai_yoyak Thai_rorua Thai_ru Thai_loling Thai_lu Thai_wowaen Thai_sosala
syn keyword xmodmapKeySym	Thai_sorusi Thai_sosua Thai_hohip Thai_lochula Thai_oang Thai_honokhuk Thai_paiyannoi Thai_saraa Thai_maihanakat Thai_saraaa Thai_saraam Thai_sarai Thai_saraii Thai_saraue Thai_sarauee
syn keyword xmodmapKeySym	Thai_sarau Thai_sarauu Thai_phinthu Thai_maihanakat_maitho Thai_baht Thai_sarae Thai_saraae Thai_sarao Thai_saraaimaimuan Thai_saraaimaimalai Thai_lakkhangyao Thai_maiyamok Thai_maitaikhu Thai_maiek Thai_maitho
syn keyword xmodmapKeySym	Thai_maitri Thai_maichattawa Thai_thanthakhat Thai_nikhahit Thai_leksun Thai_leknung Thai_leksong Thai_leksam Thai_leksi Thai_lekha Thai_lekhok Thai_lekchet Thai_lekpaet Thai_lekkao Hangul
syn keyword xmodmapKeySym	Hangul_Start Hangul_End Hangul_Hanja Hangul_Jamo Hangul_Romaja Hangul_Codeinput Hangul_Jeonja Hangul_Banja Hangul_PreHanja Hangul_PostHanja Hangul_SingleCandidate Hangul_MultipleCandidate Hangul_PreviousCandidate Hangul_Special Hangul_switch
syn keyword xmodmapKeySym	Hangul_Kiyeog Hangul_SsangKiyeog Hangul_KiyeogSios Hangul_Nieun Hangul_NieunJieuj Hangul_NieunHieuh Hangul_Dikeud Hangul_SsangDikeud Hangul_Rieul Hangul_RieulKiyeog Hangul_RieulMieum Hangul_RieulPieub Hangul_RieulSios Hangul_RieulTieut Hangul_RieulPhieuf
syn keyword xmodmapKeySym	Hangul_RieulHieuh Hangul_Mieum Hangul_Pieub Hangul_SsangPieub Hangul_PieubSios Hangul_Sios Hangul_SsangSios Hangul_Ieung Hangul_Jieuj Hangul_SsangJieuj Hangul_Cieuc Hangul_Khieuq Hangul_Tieut Hangul_Phieuf Hangul_Hieuh
syn keyword xmodmapKeySym	Hangul_A Hangul_AE Hangul_YA Hangul_YAE Hangul_EO Hangul_E Hangul_YEO Hangul_YE Hangul_O Hangul_WA Hangul_WAE Hangul_OE Hangul_YO Hangul_U Hangul_WEO
syn keyword xmodmapKeySym	Hangul_WE Hangul_WI Hangul_YU Hangul_EU Hangul_YI Hangul_I Hangul_J_Kiyeog Hangul_J_SsangKiyeog Hangul_J_KiyeogSios Hangul_J_Nieun Hangul_J_NieunJieuj Hangul_J_NieunHieuh Hangul_J_Dikeud Hangul_J_Rieul Hangul_J_RieulKiyeog
syn keyword xmodmapKeySym	Hangul_J_RieulMieum Hangul_J_RieulPieub Hangul_J_RieulSios Hangul_J_RieulTieut Hangul_J_RieulPhieuf Hangul_J_RieulHieuh Hangul_J_Mieum Hangul_J_Pieub Hangul_J_PieubSios Hangul_J_Sios Hangul_J_SsangSios Hangul_J_Ieung Hangul_J_Jieuj Hangul_J_Cieuc Hangul_J_Khieuq
syn keyword xmodmapKeySym	Hangul_J_Tieut Hangul_J_Phieuf Hangul_J_Hieuh Hangul_RieulYeorinHieuh Hangul_SunkyeongeumMieum Hangul_SunkyeongeumPieub Hangul_PanSios Hangul_KkogjiDalrinIeung Hangul_SunkyeongeumPhieuf Hangul_YeorinHieuh Hangul_AraeA Hangul_AraeAE Hangul_J_PanSios Hangul_J_KkogjiDalrinIeung Hangul_J_YeorinHieuh
syn keyword xmodmapKeySym	Korean_Won Armenian_eternity Armenian_ligature_ew Armenian_full_stop Armenian_verjaket Armenian_parenright Armenian_parenleft Armenian_guillemotright Armenian_guillemotleft Armenian_em_dash Armenian_dot Armenian_mijaket Armenian_separation_mark Armenian_but Armenian_comma
syn keyword xmodmapKeySym	Armenian_en_dash Armenian_hyphen Armenian_yentamna Armenian_ellipsis Armenian_exclam Armenian_amanak Armenian_accent Armenian_shesht Armenian_question Armenian_paruyk Armenian_AYB Armenian_ayb Armenian_BEN Armenian_ben Armenian_GIM
syn keyword xmodmapKeySym	Armenian_gim Armenian_DA Armenian_da Armenian_YECH Armenian_yech Armenian_ZA Armenian_za Armenian_E Armenian_e Armenian_AT Armenian_at Armenian_TO Armenian_to Armenian_ZHE Armenian_zhe
syn keyword xmodmapKeySym	Armenian_INI Armenian_ini Armenian_LYUN Armenian_lyun Armenian_KHE Armenian_khe Armenian_TSA Armenian_tsa Armenian_KEN Armenian_ken Armenian_HO Armenian_ho Armenian_DZA Armenian_dza Armenian_GHAT
syn keyword xmodmapKeySym	Armenian_ghat Armenian_TCHE Armenian_tche Armenian_MEN Armenian_men Armenian_HI Armenian_hi Armenian_NU Armenian_nu Armenian_SHA Armenian_sha Armenian_VO Armenian_vo Armenian_CHA Armenian_cha
syn keyword xmodmapKeySym	Armenian_PE Armenian_pe Armenian_JE Armenian_je Armenian_RA Armenian_ra Armenian_SE Armenian_se Armenian_VEV Armenian_vev Armenian_TYUN Armenian_tyun Armenian_RE Armenian_re Armenian_TSO
syn keyword xmodmapKeySym	Armenian_tso Armenian_VYUN Armenian_vyun Armenian_PYUR Armenian_pyur Armenian_KE Armenian_ke Armenian_O Armenian_o Armenian_FE Armenian_fe Armenian_apostrophe Armenian_section_sign Georgian_an Georgian_ban
syn keyword xmodmapKeySym	Georgian_gan Georgian_don Georgian_en Georgian_vin Georgian_zen Georgian_tan Georgian_in Georgian_kan Georgian_las Georgian_man Georgian_nar Georgian_on Georgian_par Georgian_zhar Georgian_rae
syn keyword xmodmapKeySym	Georgian_san Georgian_tar Georgian_un Georgian_phar Georgian_khar Georgian_ghan Georgian_qar Georgian_shin Georgian_chin Georgian_can Georgian_jil Georgian_cil Georgian_char Georgian_xan Georgian_jhan
syn keyword xmodmapKeySym	Georgian_hae Georgian_he Georgian_hie Georgian_we Georgian_har Georgian_hoe Georgian_fi Ccedillaabovedot Xabovedot Qabovedot Ibreve IE UO Zstroke Gcaron
syn keyword xmodmapKeySym	Obarred ccedillaabovedot xabovedot Ocaron qabovedot ibreve ie uo zstroke gcaron ocaron obarred SCHWA schwa Lbelowdot
syn keyword xmodmapKeySym	Lstrokebelowdot lbelowdot lstrokebelowdot Gtilde gtilde Abelowdot abelowdot Ahook ahook Acircumflexacute acircumflexacute Acircumflexgrave acircumflexgrave Acircumflexhook acircumflexhook
syn keyword xmodmapKeySym	Acircumflextilde acircumflextilde Acircumflexbelowdot acircumflexbelowdot Abreveacute abreveacute Abrevegrave abrevegrave Abrevehook abrevehook Abrevetilde abrevetilde Abrevebelowdot abrevebelowdot Ebelowdot
syn keyword xmodmapKeySym	ebelowdot Ehook ehook Etilde etilde Ecircumflexacute ecircumflexacute Ecircumflexgrave ecircumflexgrave Ecircumflexhook ecircumflexhook Ecircumflextilde ecircumflextilde Ecircumflexbelowdot ecircumflexbelowdot
syn keyword xmodmapKeySym	Ihook ihook Ibelowdot ibelowdot Obelowdot obelowdot Ohook ohook Ocircumflexacute ocircumflexacute Ocircumflexgrave ocircumflexgrave Ocircumflexhook ocircumflexhook Ocircumflextilde
syn keyword xmodmapKeySym	ocircumflextilde Ocircumflexbelowdot ocircumflexbelowdot Ohornacute ohornacute Ohorngrave ohorngrave Ohornhook ohornhook Ohorntilde ohorntilde Ohornbelowdot ohornbelowdot Ubelowdot ubelowdot
syn keyword xmodmapKeySym	Uhook uhook Uhornacute uhornacute Uhorngrave uhorngrave Uhornhook uhornhook Uhorntilde uhorntilde Uhornbelowdot uhornbelowdot Ybelowdot ybelowdot Yhook
syn keyword xmodmapKeySym	yhook Ytilde ytilde Ohorn ohorn Uhorn uhorn combining_tilde combining_grave combining_acute combining_hook combining_belowdot EcuSign ColonSign CruzeiroSign
syn keyword xmodmapKeySym	FFrancSign LiraSign MillSign NairaSign PesetaSign RupeeSign WonSign NewSheqelSign DongSign EuroSign
syn match   xmodmapKeySym	"\<[A-Za-z]\>"

" keywords
syn keyword xmodmapKeyword	keycode keysym clear add remove pointer

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_xmodmap_syn_inits")
  if version < 508
    let did_xmodmap_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink xmodmapComment	    Comment
  HiLink xmodmapTodo	    Todo
  HiLink xmodmapInt	    Number
  HiLink xmodmapHex	    Number
  HiLink xmodmapOctal	    Number
  HiLink xmodmapOctalError  Error
  HiLink xmodmapKeySym	    Constant
  HiLink xmodmapKeyword	    Keyword
  delcommand HiLink
endif

let b:current_syntax = "xmodmap"

" vim: set sts=2 sw=2:
