" Menu Translations:	Turkish
" Maintainer:			Emir SARI <bitigchi@me.com>

if exists("did_menu_trans")
   finish
endif
let did_menu_trans = 1
let s:keepcpo= &cpo
set cpo&vim

scriptencoding latin1 

" Top
menutrans &File				&Dosya
menutrans &Edit				Dü&zen
menutrans &Tools			&Araçlar
menutrans &Syntax			&Sözdizim
menutrans &Buffers			A&rabellekler
menutrans &Window			&Pencere
menutrans &Help				&Yardim


" Help menu
menutrans &Overview<Tab><F1>	&Genel\ Bakis<Tab><F1>
menutrans &User\ Manual			&Kullanim\ Kilavuzu
menutrans &How-To\ Links		&Nasil\ Yapilir?
menutrans &Find\.\.\.			&Bul\.\.\.
"--------------------
menutrans &Credits			    &Tesekkürler
menutrans Co&pying			    &Dagitim
menutrans &Sponsor/Register		&Sponsorluk/Kayit
menutrans O&rphans			    &Yetimler
"--------------------
menutrans &Version			    Sürüm\ &Bilgisi
menutrans &About			    &Hakkinda


" File menu
menutrans &Open\.\.\.<Tab>:e		    &Aç\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	    &Yeni\ Bölümde\ Aç\.\.\.<Tab>:sp
menutrans Open\ Tab\.\.\.<Tab>:tabnew	S&ekme\ Aç\.\.\.<Tab>:tabnew
menutrans &New<Tab>:enew		        Yeni\ &Sekme<Tab>:enew
menutrans &Close<Tab>:close		        Ka&pat<Tab>:close
"--------------------
menutrans &Save<Tab>:w			        Ka&ydet<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	    &Farkli Kaydet\.\.\.<Tab>:sav
"--------------------
menutrans Split\ &Diff\ With\.\.\.	    Ka&rsilastir\.\.\.
menutrans Split\ Patched\ &By\.\.\.	    Ya&malar\ Dahil\ Karsilastir\.\.\.
"--------------------
menutrans &Print			            Ya&zdir
menutrans Sa&ve-Exit<Tab>:wqa		    Kaydet\ &ve Çik<Tab>:wqa
menutrans E&xit<Tab>:qa			        Çi&k<Tab>:qa


" Edit menu
menutrans &Undo<Tab>u			&Geri\ Al<Tab>u
menutrans &Redo<Tab>^R			&Yinele<Tab>^R
menutrans Rep&eat<Tab>\.		Son\ Komutu\ Y&inele<Tab>\.
"--------------------
menutrans Cu&t<Tab>"+x			&Kes<Tab>"+x
menutrans &Copy<Tab>"+y			K&opyala<Tab>"+y
menutrans &Paste<Tab>"+gP		Ya&pistir<Tab>"+gP
menutrans Put\ &Before<Tab>[p	Ö&nüne Koy<Tab>[p
menutrans Put\ &After<Tab>]p	A&rkasina Koy<Tab>]p
menutrans &Delete<Tab>x			Si&l<Tab>x
menutrans &Select\ All<Tab>ggVG	Tü&münü\ Seç<Tab>ggVG
"--------------------
" Athena GUI only
menutrans &Find<Tab>/			            &Bul<Tab>/
menutrans Find\ and\ Rep&lace<Tab>:%s	    Bul\ &ve\ Degistir<Tab>:%s
" End Athena GUI only
menutrans &Find\.\.\.<Tab>/		            &Bul\.\.\.<Tab>/
menutrans Find\ and\ Rep&lace\.\.\.	        Bul\ ve\ &Degistir\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.<Tab>:%s	Bul\ ve\ &Degistir\.\.\.<Tab>:%s
menutrans Find\ and\ Rep&lace\.\.\.<Tab>:s	Bul\ ve\ &Degistir\.\.\.<Tab>:s
"--------------------
menutrans Settings\ &Window		&Ayarlar\ Penceresi
menutrans Startup\ &Settings	Baslan&giç\ Ayarlari
menutrans &Global\ Settings		Ge&nel\ Ayarlar
menutrans F&ile\ Settings		&Dosya\ Ayarlari
menutrans C&olor\ Scheme		&Renk\ Düzeni
menutrans &Keymap			    Dügme\ &Eslem
menutrans Select\ Fo&nt\.\.\.	Ya&zitipi Seç\.\.\.
">>>----------------- Edit/Global settings
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!        Dizgi\ &Vurgulamasini\ Aç/Kapat<Tab>:set\ hls!
menutrans Toggle\ &Ignoring\ Case<Tab>:set\ ic!		        BÜYÜK/küçük\ Harf\ &Duyarsiz\ Aç/Kapat<Tab>:set\ ic!
menutrans Toggle\ &Showing\ Matched\ Pairs<Tab>:set\ sm!	Es&lesen\ Ikilileri\ Aç/Kapat<Tab>:set\ sm!
menutrans &Context\ Lines				                    I&mleçle\ Oynayan\ Satirlar
menutrans &Virtual\ Edit				                    &Sanal\ Düzenleme
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!		        Ekleme\ &Kipini\ Aç/Kapat<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatibility<Tab>:set\ cp!		    &Vi\ Uyumlu\ Kipi\ Aç/Kapat<Tab>:set\ cp!
menutrans Search\ &Path\.\.\.				                &Arama\ Yolu\.\.\.
menutrans Ta&g\ Files\.\.\.				                    &Etiket\ Dosyalari\.\.\.
"
menutrans Toggle\ &Toolbar				    &Araç\ Çubugunu\ Aç/Kapat
menutrans Toggle\ &Bottom\ Scrollbar		A&lt\ Kaydirma\ Çubugunu\ Aç/Kapat
menutrans Toggle\ &Left\ Scrollbar			&Sol\ Kaydirma\ Çubugunu\ Aç/Kapat
menutrans Toggle\ &Right\ Scrollbar		    S&ag\ Kaydirma\ Çubugunu\ Aç/Kapat  	
">>>->>>------------- Edit/Global settings/Virtual edit
menutrans Never					Kapali
menutrans Block\ Selection		Blok\ Seçimi
menutrans Insert\ Mode			Ekleme\ Kipi
menutrans Block\ and\ Insert	Blok\ Seçiminde\ ve\ Ekleme\ Kipinde
menutrans Always				Her\ Zaman\ Açik
">>>----------------- Edit/File settings
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	        &Satir\ Numaralandirmayi\ Aç/Kapat<Tab>:set\ nu!
menutrans Toggle\ Relati&ve\ Line\ Numbering<Tab>:set\ rnu!	&Göreceli\ Satir\ Numaralandirmayi\ Aç/Kapat<Tab>:set\ nru!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		        Gö&rünmeyen\ Karakterleri\ Aç/Kapat<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrapping<Tab>:set\ wrap!	        Sa&tir\ Kaydirmayi\ Aç/Kapat<Tab>:set\ wrap!
menutrans Toggle\ W&rapping\ at\ Word<Tab>:set\ lbr!	    Sö&zcük\ Kaydirmayi\ Aç/Kapat<Tab>:set\ lbr!
menutrans Toggle\ Tab\ &Expanding-tab<Tab>:set\ et!	        S&ekmeleri\ Bosluklara\ Dönüstürmeyi\ Aç/Kapat<Tab>:set\ et!
menutrans Toggle\ &Auto\ Indenting<Tab>:set\ ai!	        &Otomatik\ Girintilemeyi\ Aç/Kapat<Tab>:set\ ai!
menutrans Toggle\ &C-Style\ Indenting<Tab>:set\ cin!	    &C\ Tarzi\ Girintilemeyi\ Aç/Kapat<Tab>:set\ cin!
">>>---
menutrans &Shiftwidth			&Girinti\ Düzeyi
menutrans Soft\ &Tabstop		&Sekme\ Genisligi
menutrans Te&xt\ Width\.\.\.	&Metin\ Genisligi\.\.\.
menutrans &File\ Format\.\.\.	&Dosya\ Biçimi\.\.\.
"
"
"
" Tools menu
menutrans &Jump\ to\ This\ Tag<Tab>g^]		S&u\ Etikete\ Atla<Tab>g^]
menutrans Jump\ &Back<Tab>^T				&Geri\ Dön<Tab>^T
menutrans Build\ &Tags\ File				&Etiket\ Dosyasi\ Olustur
"-------------------
menutrans &Folding					&Kivirmalar
menutrans &Spelling					&Yazim\ Denetimi
menutrans &Diff						&Ayrimlar\ (diff)
"-------------------
menutrans &Make<Tab>:make						&Derle<Tab>:make
menutrans &List\ Errors<Tab>:cl					&Hatalari\ Listele<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!				I&letileri\ Listele<Tab>:cl!
menutrans &Next\ Error<Tab>:cn					Bir\ &Sonraki\ Hata<Tab>:cn
menutrans &Previous\ Error<Tab>:cp				Bir\ Ö&nceki\ Hata<Tab>:cp
menutrans &Older\ List<Tab>:cold				Daha\ &Eski\ Hatalar<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew				Daha\ &Yeni\ Hatalar<Tab>:cnew
menutrans Error\ &Window						Hatalar\ &Penceresi
menutrans Se&t\ Compiler						De&rleyici\ Seç
menutrans Show\ Compiler\ Se&ttings\ in\ Menu	Derleyici\ Ayarlarini\ Menüde\ &Göster 
"-------------------
menutrans &Convert\ to\ HEX<Tab>:%!xxd			HEX'e\ Dö&nüstür<Tab>:%!xxd
menutrans Conve&rt\ Back<Tab>:%!xxd\ -r			HEX'&ten\ Dönüstür<Tab>:%!xxd\ -r
">>>---------------- Tools/Spelling
menutrans &Spell\ Check\ On						Yazim\ Denetimini\ &Aç
menutrans Spell\ Check\ &Off					Yazim\ Denetimini\ &Kapat
menutrans To\ &Next\ Error<Tab>]s				Bir\ &Sonraki\ Hata<Tab>]s
menutrans To\ &Previous\ Error<Tab>[s			Bir\ Ö&nceki\ Hata<Tab>[s
menutrans Suggest\ &Corrections<Tab>z=			Dü&zeltme\ Öner<Tab>z=
menutrans &Repeat\ Correction<Tab>:spellrepall	Düzeltmeyi\ &Yinele<Tab>spellrepall
"-------------------
menutrans Set\ Language\ to\ "en"		Dili\ "en"\ yap
menutrans Set\ Language\ to\ "en_au"	Dili\ "en_au"\ yap
menutrans Set\ Language\ to\ "en_ca"	Dili\ "en_ca"\ yap
menutrans Set\ Language\ to\ "en_gb"	Dili\ "en_gb"\ yap
menutrans Set\ Language\ to\ "en_nz"	Dili\ "en_nz"\ yap
menutrans Set\ Language\ to\ "en_us"	Dili\ "en_us"\ yap
menutrans &Find\ More\ Languages		&Baska\ Diller\ Bul
let g:menutrans_set_lang_to =			'Dil Yükle'
"
"
" The Spelling popup menu
"
"
let g:menutrans_spell_change_ARG_to =			'Düzeltilecek:\ "%s"\ ->'
let g:menutrans_spell_add_ARG_to_word_list =	'"%s"\ sözcügünü\ sözlüge\ ekle'
let g:menutrans_spell_ignore_ARG =				'"%s"\ sözcügünü\ yoksay'
">>>---------------- Folds
menutrans &Enable/Disable\ Folds<Tab>zi			&Kivirmalari\ Aç/Kapat<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv			I&mlecin\ Oldugu\ Satiri\ Görüntüle<Tab>zv
menutrans Vie&w\ Cursor\ Line\ Only<Tab>zMzx	Ya&lnizca\ Imlecin\ Oldugu\ Satiri\ Görüntüle<Tab>zMzx
menutrans C&lose\ More\ Folds<Tab>zm			&Daha\ Fazla\ Kivirma\ Kapat<Tab>zm
menutrans &Close\ All\ Folds<Tab>zM				Bütün\ Ki&virmalari\ Kapat<Tab>zM
menutrans &Open\ All\ Folds<Tab>zR				Bü&tün\ Kivirmalari\ Aç<Tab>zR
menutrans O&pen\ More\ Folds<Tab>zr				D&aha\ Fazla\ Kivirma\ Aç<Tab>zr
menutrans Fold\ Met&hod							Kivi&rma\ Yöntemi
menutrans Create\ &Fold<Tab>zf					Kivirma\ &Olustur<Tab>zf
menutrans &Delete\ Fold<Tab>zd					Kivirma\ &Sil<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD			Tü&m\ Kivirmalari\ Sil<Tab>zD
menutrans Fold\ col&umn\ Width					Kivirma\ Sütunu\ &Genisligi
">>>->>>----------- Tools/Folds/Fold Method
menutrans M&anual		&El\ Ile
menutrans I&ndent		&Girinti
menutrans E&xpression	I&fade
menutrans S&yntax		&Sözdizim
menutrans Ma&rker		I&mleyici
">>>--------------- Tools/Diff
menutrans &Update		&Güncelle
menutrans &Get\ Block	Blogu\ &Al
menutrans &Put\ Block	Blogu\ &Koy
">>>--------------- Tools/Diff/Error window
menutrans &Update<Tab>:cwin		&Güncelle<Tab>:cwin
menutrans &Close<Tab>:cclose	&Kapat<Tab>:cclose
menutrans &Open<Tab>:copen		&Aç<Tab>:copen
"
"
" Syntax menu
"
menutrans &Show\ File\ Types\ in\ Menu		Dosya\ Türlerini\ Menüde\ &Göster
menutrans Set\ '&syntax'\ only				Yalnizca\ 'syntax'\ &Ayarla
menutrans Set\ '&filetype'\ too				'filetype'\ Için\ &de\ Ayarla
menutrans &Off								&Kapat
menutrans &Manual							&El\ Ile
menutrans A&utomatic						&Otomatik
menutrans On/Off\ for\ &This\ File			&Bu\ Dosya\ Için\ Aç/Kapat
menutrans Co&lor\ Test						&Renk\ Testi
menutrans &Highlight\ Test					&Vurgulama\ Testi
menutrans &Convert\ to\ HTML				&HTML'ye\ Dönüstür
"
"
" Buffers menu
"
menutrans &Refresh\ menu	&Menüyü\ Güncelle
menutrans Delete			&Sil
menutrans &Alternate		Ö&teki
menutrans &Next				So&nraki
menutrans &Previous			Ön&ceki
menutrans [No\ File]		[Dosya\ Yok]
"
"
" Window menu
"
menutrans &New<Tab>^Wn					Yeni\ &Pencere<Tab>^Wn
menutrans S&plit<Tab>^Ws				Pencereyi\ &Böl<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^		Pencereyi\ Baskasina\ Bö&l<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv	Pencereyi\ &Dikey\ Olarak\ Böl<Tab>^Wv
menutrans Split\ File\ E&xplorer		Yeni\ Bölü&mde\ Dosya\ Gezginini\ Aç
"
menutrans &Close<Tab>^Wc				Pen&cereyi\ Kapat<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo		Diger\ Pencerele&ri\ Kapat<Tab>^Wo
"
menutrans Move\ &To						&Tasi
menutrans Rotate\ &Up<Tab>^WR			&Yukari\ Tasi<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr			&Asagi\ Tasi<Tab>^Wr
"
menutrans &Equal\ Size<Tab>^W=			&Esit\ Boyut<Tab>^W=
menutrans &Max\ Height<Tab>^W_			E&n\ Büyük\ Yükseklik<Tab>^W_
menutrans M&in\ Height<Tab>^W1_			En\ Küçük\ Yüksekl&ik<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|			En\ Büyük\ Gen&islik<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|			En\ Küçük\ Genis&lik<Tab>^W1\|
">>>----------------- Window/Move To
menutrans &Top<Tab>^WK					&Yukari<Tab>^WK
menutrans &Bottom<Tab>^WJ				&Asagi<Tab>^WJ
menutrans &Left\ Side<Tab>^WH			So&la<Tab>^WH
menutrans &Right\ Side<Tab>^WL			&Saga<Tab>^WL
"
"
" The popup menu
"
"
menutrans &Undo					&Geri\ Al
menutrans Cu&t					&Kes
menutrans &Copy					K&opyala
menutrans &Paste				&Yapistir
menutrans &Delete				&Sil
menutrans Select\ Blockwise		&Blok\ Biçiminde\ Seç
menutrans Select\ &Word			Sö&zcük\ Seç
menutrans Select\ &Sentence		&Tümce\ Seç
menutrans Select\ Pa&ragraph	&Paragraf\ Seç
menutrans Select\ &Line			S&atir\ Seç
menutrans Select\ &Block		Bl&ok\ Seç
menutrans Select\ &All			Tümü&nü\ Seç
"
" The GUI toolbar
"
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open			Dosya Aç
    tmenu ToolBar.Save			Dosya Kaydet
    tmenu ToolBar.SaveAll		Tüm Dosyalari Kaydet
    tmenu ToolBar.Print			Yazdir
    tmenu ToolBar.Undo			Geri Al
    tmenu ToolBar.Redo			Yinele
    tmenu ToolBar.Cut			Kes
    tmenu ToolBar.Copy			Kopyala
    tmenu ToolBar.Paste			Yapistir
	tmenu ToolBar.Find			Bul...
    tmenu ToolBar.FindNext		Sonrakini Bul
    tmenu ToolBar.FindPrev		Öncekini Bul
    tmenu ToolBar.Replace		Bul ve Degistir...
    if 0	" disabled; These are in the Windows menu
      tmenu ToolBar.New			Yeni Pencere
      tmenu ToolBar.WinSplit	Pencereyi Böl
      tmenu ToolBar.WinMax		En Büyük Pencere Yüksekligi
      tmenu ToolBar.WinMin		En Küçük Pencere Yüksekligi
      tmenu ToolBar.WinClose	Pencereyi Kapat
    endif
    tmenu ToolBar.LoadSesn		Oturum Yükle
    tmenu ToolBar.SaveSesn		Oturum Kaydet
    tmenu ToolBar.RunScript		Betik Çalistir
    tmenu ToolBar.Make			Derle
    tmenu ToolBar.Shell			Kabuk
    tmenu ToolBar.RunCtags		Etiket Dosyasi Olustur
    tmenu ToolBar.TagJump		Etikete Atla
    tmenu ToolBar.Help			Yardim
    tmenu ToolBar.FindHelp		Yardim Bul
  endfun
endif
"
"
" Dialog texts
"
" Find in help dialog
"
let g:menutrans_help_dialog = "Yardim icin komut veya sozcuk girin:\n\nEkleme Kipi komutlarini aramak icin i_ ekleyin (ornegin i_CTRL-X)\nNormal Kip komutlarini aramak icin _c ekleyin (ornegin c_<Del>)\nSecenekler hakkinda yardim almak icin ' ekleyin (ornegin 'shiftwidth')"
"
"
" Searh path dialog
"
let g:menutrans_path_dialog = "Dosya aramasi için yol belirtin.\nDizin adlari virgüllerle ayrilir."
"
" Tag files dialog
"
let g:menutrans_tags_dialog = "Etiket dosyasi adlari belirtin (virgülle ayirarak).\n"
"
" Text width dialog
"
let g:menutrans_textwidth_dialog = "Biçimlendirme için metin genisligini belirtin.\nBiçimlendirme iptali için 0 girin."
"
" File format dialog
"
let g:menutrans_fileformat_dialog = "Dosya biçimi seçin"
let g:menutrans_fileformat_choices = "&Unix\n&Dos\n&Mac\nI&ptal"
"
let menutrans_no_file = "[Dosya Yok]"

let &cpo = s:keepcpo
unlet s:keepcpo
