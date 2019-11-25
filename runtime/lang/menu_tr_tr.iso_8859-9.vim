" Menu Translations:	Turkish
" Maintainer:			Emir SARI <bitigchi@me.com>

if exists("did_menu_trans")
   finish
endif
let did_menu_trans = 1
let s:keepcpo= &cpo
set cpo&vim

scriptencoding iso-8859-9

" Top
menutrans &File				&Dosya
menutrans &Edit				DÃ¼&zen
menutrans &Tools			&AraÃ§lar
menutrans &Syntax			&SÃ¶zdizim
menutrans &Buffers			A&rabellekler
menutrans &Window			&Pencere
menutrans &Help				&YardÄ±m


" Help menu
menutrans &Overview<Tab><F1>	&Genel\ BakÄ±Å<Tab><F1>
menutrans &User\ Manual			&KullanÄ±m\ KÄ±lavuzu
menutrans &How-To\ Links		&NasÄ±l\ YapÄ±lÄ±r?
menutrans &Find\.\.\.			&Bul\.\.\.
"--------------------
menutrans &Credits			    &TeÅekkÃ¼rler
menutrans Co&pying			    &DaÄÄ±tÄ±m
menutrans &Sponsor/Register		&Sponsorluk/KayÄ±t
menutrans O&rphans			    &Yetimler
"--------------------
menutrans &Version			    SÃ¼rÃ¼m\ &Bilgisi
menutrans &About			    &HakkÄ±nda


" File menu
menutrans &Open\.\.\.<Tab>:e		    &AÃ§\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	    &Yeni\ BÃ¶lÃ¼mde\ AÃ§\.\.\.<Tab>:sp
menutrans Open\ Tab\.\.\.<Tab>:tabnew	S&ekme\ AÃ§\.\.\.<Tab>:tabnew
menutrans &New<Tab>:enew		        Yeni\ &Sekme<Tab>:enew
menutrans &Close<Tab>:close		        Ka&pat<Tab>:close
"--------------------
menutrans &Save<Tab>:w			        Ka&ydet<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	    &FarklÄ± Kaydet\.\.\.<Tab>:sav
"--------------------
menutrans Split\ &Diff\ With\.\.\.	    Ka&rÅÄ±laÅtÄ±r\.\.\.
menutrans Split\ Patched\ &By\.\.\.	    Ya&malar\ Dahil\ KarÅÄ±laÅtÄ±r\.\.\.
"--------------------
menutrans &Print			            Ya&zdÄ±r
menutrans Sa&ve-Exit<Tab>:wqa		    Kaydet\ &ve ÃÄ±k<Tab>:wqa
menutrans E&xit<Tab>:qa			        ÃÄ±&k<Tab>:qa


" Edit menu
menutrans &Undo<Tab>u			&Geri\ Al<Tab>u
menutrans &Redo<Tab>^R			&Yinele<Tab>^R
menutrans Rep&eat<Tab>\.		Son\ Komutu\ Y&inele<Tab>\.
"--------------------
menutrans Cu&t<Tab>"+x			&Kes<Tab>"+x
menutrans &Copy<Tab>"+y			K&opyala<Tab>"+y
menutrans &Paste<Tab>"+gP		Ya&pÄ±ÅtÄ±r<Tab>"+gP
menutrans Put\ &Before<Tab>[p	Ã&nÃ¼ne Koy<Tab>[p
menutrans Put\ &After<Tab>]p	A&rkasÄ±na Koy<Tab>]p
menutrans &Delete<Tab>x			Si&l<Tab>x
menutrans &Select\ All<Tab>ggVG	TÃ¼&mÃ¼nÃ¼\ SeÃ§<Tab>ggVG
"--------------------
" Athena GUI only
menutrans &Find<Tab>/			            &Bul<Tab>/
menutrans Find\ and\ Rep&lace<Tab>:%s	    Bul\ &ve\ DeÄiÅtir<Tab>:%s
" End Athena GUI only
menutrans &Find\.\.\.<Tab>/		            &Bul\.\.\.<Tab>/
menutrans Find\ and\ Rep&lace\.\.\.	        Bul\ ve\ &DeÄiÅtir\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.<Tab>:%s	Bul\ ve\ &DeÄiÅtir\.\.\.<Tab>:%s
menutrans Find\ and\ Rep&lace\.\.\.<Tab>:s	Bul\ ve\ &DeÄiÅtir\.\.\.<Tab>:s
"--------------------
menutrans Settings\ &Window		&Ayarlar\ Penceresi
menutrans Startup\ &Settings	BaÅlan&gÄ±Ã§\ AyarlarÄ±
menutrans &Global\ Settings		Ge&nel\ Ayarlar
menutrans F&ile\ Settings		&Dosya\ AyarlarÄ±
menutrans C&olor\ Scheme		&Renk\ DÃ¼zeni
menutrans &Keymap			    DÃ¼Äme\ &EÅlem
menutrans Select\ Fo&nt\.\.\.	Ya&zÄ±tipi SeÃ§\.\.\.
">>>----------------- Edit/Global settings
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!        Dizgi\ &VurgulamasÄ±nÄ±\ AÃ§/Kapat<Tab>:set\ hls!
menutrans Toggle\ &Ignoring\ Case<Tab>:set\ ic!		        BÃYÃK/kÃ¼Ã§Ã¼k\ Harf\ &DuyarsÄ±z\ AÃ§/Kapat<Tab>:set\ ic!
menutrans Toggle\ &Showing\ Matched\ Pairs<Tab>:set\ sm!	EÅ&leÅen\ Ä°kilileri\ AÃ§/Kapat<Tab>:set\ sm!
menutrans &Context\ Lines				                    Ä°&mleÃ§le\ Oynayan\ SatÄ±rlar
menutrans &Virtual\ Edit				                    &Sanal\ DÃ¼zenleme
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!		        Ekleme\ &Kipini\ AÃ§/Kapat<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatibility<Tab>:set\ cp!		    &Vi\ Uyumlu\ Kipi\ AÃ§/Kapat<Tab>:set\ cp!
menutrans Search\ &Path\.\.\.				                &Arama\ Yolu\.\.\.
menutrans Ta&g\ Files\.\.\.				                    &Etiket\ DosyalarÄ±\.\.\.
"
menutrans Toggle\ &Toolbar				    &AraÃ§\ ÃubuÄunu\ AÃ§/Kapat
menutrans Toggle\ &Bottom\ Scrollbar		A&lt\ KaydÄ±rma\ ÃubuÄunu\ AÃ§/Kapat
menutrans Toggle\ &Left\ Scrollbar			&Sol\ KaydÄ±rma\ ÃubuÄunu\ AÃ§/Kapat
menutrans Toggle\ &Right\ Scrollbar		    S&aÄ\ KaydÄ±rma\ ÃubuÄunu\ AÃ§/Kapat  	
">>>->>>------------- Edit/Global settings/Virtual edit
menutrans Never					KapalÄ±
menutrans Block\ Selection		Blok\ SeÃ§imi
menutrans Insert\ Mode			Ekleme\ Kipi
menutrans Block\ and\ Insert	Blok\ SeÃ§iminde\ ve\ Ekleme\ Kipinde
menutrans Always				Her\ Zaman\ AÃ§Ä±k
">>>----------------- Edit/File settings
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	        &SatÄ±r\ NumaralandÄ±rmayÄ±\ AÃ§/Kapat<Tab>:set\ nu!
menutrans Toggle\ Relati&ve\ Line\ Numbering<Tab>:set\ rnu!	&GÃ¶receli\ SatÄ±r\ NumaralandÄ±rmayÄ±\ AÃ§/Kapat<Tab>:set\ nru!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		        GÃ¶&rÃ¼nmeyen\ Karakterleri\ AÃ§/Kapat<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrapping<Tab>:set\ wrap!	        Sa&tÄ±r\ KaydÄ±rmayÄ±\ AÃ§/Kapat<Tab>:set\ wrap!
menutrans Toggle\ W&rapping\ at\ Word<Tab>:set\ lbr!	    SÃ¶&zcÃ¼k\ KaydÄ±rmayÄ±\ AÃ§/Kapat<Tab>:set\ lbr!
menutrans Toggle\ Tab\ &Expanding-tab<Tab>:set\ et!	        S&ekmeleri\ BoÅluklara\ DÃ¶nÃ¼ÅtÃ¼rmeyi\ AÃ§/Kapat<Tab>:set\ et!
menutrans Toggle\ &Auto\ Indenting<Tab>:set\ ai!	        &Otomatik\ Girintilemeyi\ AÃ§/Kapat<Tab>:set\ ai!
menutrans Toggle\ &C-Style\ Indenting<Tab>:set\ cin!	    &C\ TarzÄ±\ Girintilemeyi\ AÃ§/Kapat<Tab>:set\ cin!
">>>---
menutrans &Shiftwidth			&Girinti\ DÃ¼zeyi
menutrans Soft\ &Tabstop		&Sekme\ GeniÅliÄi
menutrans Te&xt\ Width\.\.\.	&Metin\ GeniÅliÄi\.\.\.
menutrans &File\ Format\.\.\.	&Dosya\ BiÃ§imi\.\.\.
"
"
"
" Tools menu
menutrans &Jump\ to\ This\ Tag<Tab>g^]		Å&u\ Etikete\ Atla<Tab>g^]
menutrans Jump\ &Back<Tab>^T				&Geri\ DÃ¶n<Tab>^T
menutrans Build\ &Tags\ File				&Etiket\ DosyasÄ±\ OluÅtur
"-------------------
menutrans &Folding					&KÄ±vÄ±rmalar
menutrans &Spelling					&YazÄ±m\ Denetimi
menutrans &Diff						&AyrÄ±mlar\ (diff)
"-------------------
menutrans &Make<Tab>:make						&Derle<Tab>:make
menutrans &List\ Errors<Tab>:cl					&HatalarÄ±\ Listele<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!				Ä°&letileri\ Listele<Tab>:cl!
menutrans &Next\ Error<Tab>:cn					Bir\ &Sonraki\ Hata<Tab>:cn
menutrans &Previous\ Error<Tab>:cp				Bir\ Ã&nceki\ Hata<Tab>:cp
menutrans &Older\ List<Tab>:cold				Daha\ &Eski\ Hatalar<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew				Daha\ &Yeni\ Hatalar<Tab>:cnew
menutrans Error\ &Window						Hatalar\ &Penceresi
menutrans Se&t\ Compiler						De&rleyici\ SeÃ§
menutrans Show\ Compiler\ Se&ttings\ in\ Menu	Derleyici\ AyarlarÄ±nÄ±\ MenÃ¼de\ &GÃ¶ster 
"-------------------
menutrans &Convert\ to\ HEX<Tab>:%!xxd			HEX'e\ DÃ¶&nÃ¼ÅtÃ¼r<Tab>:%!xxd
menutrans Conve&rt\ Back<Tab>:%!xxd\ -r			HEX'&ten\ DÃ¶nÃ¼ÅtÃ¼r<Tab>:%!xxd\ -r
">>>---------------- Tools/Spelling
menutrans &Spell\ Check\ On						YazÄ±m\ Denetimini\ &AÃ§
menutrans Spell\ Check\ &Off					YazÄ±m\ Denetimini\ &Kapat
menutrans To\ &Next\ Error<Tab>]s				Bir\ &Sonraki\ Hata<Tab>]s
menutrans To\ &Previous\ Error<Tab>[s			Bir\ Ã&nceki\ Hata<Tab>[s
menutrans Suggest\ &Corrections<Tab>z=			DÃ¼&zeltme\ Ãner<Tab>z=
menutrans &Repeat\ Correction<Tab>:spellrepall	DÃ¼zeltmeyi\ &Yinele<Tab>spellrepall
"-------------------
menutrans Set\ Language\ to\ "en"		Dili\ "en"\ yap
menutrans Set\ Language\ to\ "en_au"	Dili\ "en_au"\ yap
menutrans Set\ Language\ to\ "en_ca"	Dili\ "en_ca"\ yap
menutrans Set\ Language\ to\ "en_gb"	Dili\ "en_gb"\ yap
menutrans Set\ Language\ to\ "en_nz"	Dili\ "en_nz"\ yap
menutrans Set\ Language\ to\ "en_us"	Dili\ "en_us"\ yap
menutrans &Find\ More\ Languages		&BaÅka\ Diller\ Bul
let g:menutrans_set_lang_to =			'Dil YÃ¼kle'
"
"
" The Spelling popup menu
"
"
let g:menutrans_spell_change_ARG_to =			'DÃ¼zeltilecek:\ "%s"\ ->'
let g:menutrans_spell_add_ARG_to_word_list =	'"%s"\ sÃ¶zcÃ¼ÄÃ¼nÃ¼\ sÃ¶zlÃ¼Äe\ ekle'
let g:menutrans_spell_ignore_ARG =				'"%s"\ sÃ¶zcÃ¼ÄÃ¼nÃ¼\ yoksay'
">>>---------------- Folds
menutrans &Enable/Disable\ Folds<Tab>zi			&KÄ±vÄ±rmalarÄ±\ AÃ§/Kapat<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv			Ä°&mlecin\ OlduÄu\ SatÄ±rÄ±\ GÃ¶rÃ¼ntÃ¼le<Tab>zv
menutrans Vie&w\ Cursor\ Line\ Only<Tab>zMzx	Ya&lnÄ±zca\ Ä°mlecin\ OlduÄu\ SatÄ±rÄ±\ GÃ¶rÃ¼ntÃ¼le<Tab>zMzx
menutrans C&lose\ More\ Folds<Tab>zm			&Daha\ Fazla\ KÄ±vÄ±rma\ Kapat<Tab>zm
menutrans &Close\ All\ Folds<Tab>zM				BÃ¼tÃ¼n\ KÄ±&vÄ±rmalarÄ±\ Kapat<Tab>zM
menutrans &Open\ All\ Folds<Tab>zR				BÃ¼&tÃ¼n\ KÄ±vÄ±rmalarÄ±\ AÃ§<Tab>zR
menutrans O&pen\ More\ Folds<Tab>zr				D&aha\ Fazla\ KÄ±vÄ±rma\ AÃ§<Tab>zr
menutrans Fold\ Met&hod							KÄ±vÄ±&rma\ YÃ¶ntemi
menutrans Create\ &Fold<Tab>zf					KÄ±vÄ±rma\ &OluÅtur<Tab>zf
menutrans &Delete\ Fold<Tab>zd					KÄ±vÄ±rma\ &Sil<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD			TÃ¼&m\ KÄ±vÄ±rmalarÄ±\ Sil<Tab>zD
menutrans Fold\ col&umn\ Width					KÄ±vÄ±rma\ SÃ¼tunu\ &GeniÅliÄi
">>>->>>----------- Tools/Folds/Fold Method
menutrans M&anual		&El\ Ä°le
menutrans I&ndent		&Girinti
menutrans E&xpression	Ä°&fade
menutrans S&yntax		&SÃ¶zdizim
menutrans Ma&rker		Ä°&mleyici
">>>--------------- Tools/Diff
menutrans &Update		&GÃ¼ncelle
menutrans &Get\ Block	BloÄu\ &Al
menutrans &Put\ Block	BloÄu\ &Koy
">>>--------------- Tools/Diff/Error window
menutrans &Update<Tab>:cwin		&GÃ¼ncelle<Tab>:cwin
menutrans &Close<Tab>:cclose	&Kapat<Tab>:cclose
menutrans &Open<Tab>:copen		&AÃ§<Tab>:copen
"
"
" Syntax menu
"
menutrans &Show\ File\ Types\ in\ Menu		Dosya\ TÃ¼rlerini\ MenÃ¼de\ &GÃ¶ster
menutrans Set\ '&syntax'\ only				YalnÄ±zca\ 'syntax'\ &Ayarla
menutrans Set\ '&filetype'\ too				'filetype'\ Ä°Ã§in\ &de\ Ayarla
menutrans &Off								&Kapat
menutrans &Manual							&El\ Ä°le
menutrans A&utomatic						&Otomatik
menutrans On/Off\ for\ &This\ File			&Bu\ Dosya\ Ä°Ã§in\ AÃ§/Kapat
menutrans Co&lor\ Test						&Renk\ Testi
menutrans &Highlight\ Test					&Vurgulama\ Testi
menutrans &Convert\ to\ HTML				&HTML'ye\ DÃ¶nÃ¼ÅtÃ¼r
"
"
" Buffers menu
"
menutrans &Refresh\ menu	&MenÃ¼yÃ¼\ GÃ¼ncelle
menutrans Delete			&Sil
menutrans &Alternate		Ã&teki
menutrans &Next				So&nraki
menutrans &Previous			Ãn&ceki
menutrans [No\ File]		[Dosya\ Yok]
"
"
" Window menu
"
menutrans &New<Tab>^Wn					Yeni\ &Pencere<Tab>^Wn
menutrans S&plit<Tab>^Ws				Pencereyi\ &BÃ¶l<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^		Pencereyi\ BaÅkasÄ±na\ BÃ¶&l<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv	Pencereyi\ &Dikey\ Olarak\ BÃ¶l<Tab>^Wv
menutrans Split\ File\ E&xplorer		Yeni\ BÃ¶lÃ¼&mde\ Dosya\ Gezginini\ AÃ§
"
menutrans &Close<Tab>^Wc				Pen&cereyi\ Kapat<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo		DiÄer\ Pencerele&ri\ Kapat<Tab>^Wo
"
menutrans Move\ &To						&TaÅÄ±
menutrans Rotate\ &Up<Tab>^WR			&YukarÄ±\ TaÅÄ±<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr			&AÅaÄÄ±\ TaÅÄ±<Tab>^Wr
"
menutrans &Equal\ Size<Tab>^W=			&EÅit\ Boyut<Tab>^W=
menutrans &Max\ Height<Tab>^W_			E&n\ BÃ¼yÃ¼k\ YÃ¼kseklik<Tab>^W_
menutrans M&in\ Height<Tab>^W1_			En\ KÃ¼Ã§Ã¼k\ YÃ¼ksekl&ik<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|			En\ BÃ¼yÃ¼k\ Gen&iÅlik<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|			En\ KÃ¼Ã§Ã¼k\ GeniÅ&lik<Tab>^W1\|
">>>----------------- Window/Move To
menutrans &Top<Tab>^WK					&YukarÄ±<Tab>^WK
menutrans &Bottom<Tab>^WJ				&AÅaÄÄ±<Tab>^WJ
menutrans &Left\ Side<Tab>^WH			So&la<Tab>^WH
menutrans &Right\ Side<Tab>^WL			&SaÄa<Tab>^WL
"
"
" The popup menu
"
"
menutrans &Undo					&Geri\ Al
menutrans Cu&t					&Kes
menutrans &Copy					K&opyala
menutrans &Paste				&YapÄ±ÅtÄ±r
menutrans &Delete				&Sil
menutrans Select\ Blockwise		&Blok\ BiÃ§iminde\ SeÃ§
menutrans Select\ &Word			SÃ¶&zcÃ¼k\ SeÃ§
menutrans Select\ &Sentence		&TÃ¼mce\ SeÃ§
menutrans Select\ Pa&ragraph	&Paragraf\ SeÃ§
menutrans Select\ &Line			S&atÄ±r\ SeÃ§
menutrans Select\ &Block		Bl&ok\ SeÃ§
menutrans Select\ &All			TÃ¼mÃ¼&nÃ¼\ SeÃ§
"
" The GUI toolbar
"
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open			Dosya AÃ§
    tmenu ToolBar.Save			Dosya Kaydet
    tmenu ToolBar.SaveAll		TÃ¼m DosyalarÄ± Kaydet
    tmenu ToolBar.Print			YazdÄ±r
    tmenu ToolBar.Undo			Geri Al
    tmenu ToolBar.Redo			Yinele
    tmenu ToolBar.Cut			Kes
    tmenu ToolBar.Copy			Kopyala
    tmenu ToolBar.Paste			YapÄ±ÅtÄ±r
	tmenu ToolBar.Find			Bul...
    tmenu ToolBar.FindNext		Sonrakini Bul
    tmenu ToolBar.FindPrev		Ãncekini Bul
    tmenu ToolBar.Replace		Bul ve DeÄiÅtir...
    if 0	" disabled; These are in the Windows menu
      tmenu ToolBar.New			Yeni Pencere
      tmenu ToolBar.WinSplit	Pencereyi BÃ¶l
      tmenu ToolBar.WinMax		En BÃ¼yÃ¼k Pencere YÃ¼ksekliÄi
      tmenu ToolBar.WinMin		En KÃ¼Ã§Ã¼k Pencere YÃ¼ksekliÄi
      tmenu ToolBar.WinClose	Pencereyi Kapat
    endif
    tmenu ToolBar.LoadSesn		Oturum YÃ¼kle
    tmenu ToolBar.SaveSesn		Oturum Kaydet
    tmenu ToolBar.RunScript		Betik ÃalÄ±ÅtÄ±r
    tmenu ToolBar.Make			Derle
    tmenu ToolBar.Shell			Kabuk
    tmenu ToolBar.RunCtags		Etiket DosyasÄ± OluÅtur
    tmenu ToolBar.TagJump		Etikete Atla
    tmenu ToolBar.Help			YardÄ±m
    tmenu ToolBar.FindHelp		YardÄ±m Bul
  endfun
endif
"
"
" Dialog texts
"
" Find in help dialog
"
let g:menutrans_help_dialog = "YardÄ±m iÃ§in komut veya sÃ¶zcÃ¼k girin:\n\nEkleme Kipi komutlarÄ±nÄ± aramak iÃ§in i_ ekleyin (Ã¶rneÄin i_CTRL-X)\nNormal Kip komutlarÄ±nÄ± aramak iÃ§in _c ekleyin (Ã¶rneÄin Ñ_<Del>)\nSeÃ§enekler hakkÄ±nda yardÄ±m almak iÃ§in ' ekleyin (Ã¶rneÄin 'shiftwidth')"
"
" Searh path dialog
"
let g:menutrans_path_dialog = "Dosya aramasÄ± iÃ§in yol belirtin.\nDizin adlarÄ± virgÃ¼llerle ayrÄ±lÄ±r."
"
" Tag files dialog
"
let g:menutrans_tags_dialog = "Etiket dosyasÄ± adlarÄ± belirtin (virgÃ¼lle ayÄ±rarak).\n"
"
" Text width dialog
"
let g:menutrans_textwidth_dialog = "BiÃ§imlendirme iÃ§in metin geniÅliÄini belirtin.\nBiÃ§imlendirme iptali iÃ§in 0 girin."
"
" File format dialog
"
let g:menutrans_fileformat_dialog = "Dosya biÃ§imi seÃ§in"
let g:menutrans_fileformat_choices = "&Unix\n&Dos\n&Mac\nÄ°&ptal"
"
let menutrans_no_file = "[Dosya Yok]"

let &cpo = s:keepcpo
unlet s:keepcpo
