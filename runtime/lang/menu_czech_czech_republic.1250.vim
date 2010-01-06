" Menu Translations:	Czech for MS-Windows
" Maintainer:		Jiri Brezina <brzj@seznam.cz>
" vim:set foldmethod=marker:
" $Revision: 1.3 $
" $Date: 2005/12/19 22:13:30 $

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1

scriptencoding cp1250

" {{{ File menu
menutrans &File				&Soubor
menutrans &Open\.\.\.<Tab>:e		&Otevøít\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	Otevøít\ v\ no&vém\ oknì\.\.\.<Tab>:sp
menutrans &New<Tab>:enew		&Nový<Tab>:enew
menutrans &Close<Tab>:close		&Zavøít<Tab>:close
menutrans &Save<Tab>:w			&Uložit<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	Uložit\ &jako\.\.\.<Tab>:sav
menutrans Split\ &Diff\ with\.\.\.	Rozdìlit\ okno\ -\ &Diff\.\.\.
menutrans Split\ Patched\ &By\.\.\.	Rozdìlit\ okno\ -\ &Patch\.\.\.
menutrans &Print			&Tisk
menutrans Sa&ve-Exit<Tab>:wqa		U&ložit\ -\ Konec<Tab>:wqa
menutrans E&xit<Tab>:qa			&Konec<Tab>:qa
" }}}

" {{{ Edit menu
menutrans &Edit				Úpr&avy
menutrans &Undo<Tab>u			&Zpìt<Tab>u
menutrans &Redo<Tab>^R			Z&rušit\ vrácení<Tab>^R
menutrans Rep&eat<Tab>\.		&Opakovat<Tab>\.
menutrans Cu&t<Tab>"+x			&Vyøíznout<Tab>"+x
menutrans &Copy<Tab>"+y			&Kopírovat<Tab>"+y
menutrans &Paste<Tab>"+gP		V&ložit<Tab>"+gP
menutrans Put\ &Before<Tab>[p		Vložit\ &pøed<Tab>[p
menutrans Put\ &After<Tab>]p		Vloži&t\ za<Tab>]p
menutrans &Delete<Tab>x			&Smazat<Tab>x
menutrans &Select\ All<Tab>ggVG		Vy&brat\ vše<Tab>ggVG
menutrans &Find\.\.\.			&Hledat\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.	&Nahradit\.\.\.
menutrans Options\.\.\.			Volb&y\.\.\.
menutrans Settings\ &Window		Nastav&ení\ okna
	" {{{2 Edit -1
menutrans &Global\ Settings				&Globální\ nastavení
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!	&Pøepnout\ zvýraznìní\ vzoru<Tab>:set\ hls!
menutrans Toggle\ &Ignore-case<Tab>:set\ ic!		Pøepnout\ ignorování\ &VERZÁLEK<Tab>:set\ ic!
menutrans Toggle\ &Showmatch<Tab>:set\ sm!		Pøepnout\ &Showmatch\ \{\(\[\])\}<Tab>:set\ sm!
menutrans &Context\ lines				Zobrazit\ konte&xt\ kurzoru
menutrans &Virtual\ Edit				Virtuální\ p&ozice\ kurzoru
	menutrans Never						Nikdy
	menutrans Block\ Selection				Výbìr\ Bloku
	menutrans Insert\ mode					Insert\ mód
	menutrans Block\ and\ Insert				Blok\ a\ Insert
	menutrans Always					Vždycky
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!		Pøepnout\ Insert\ mó&d<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatible<Tab>:set\ cp!		Pøepnout\ kompatibilní\ režim\ s\ 'vi'<Tab>:set\ cp!
menutrans Search\ &Path\.\.\.				Nastavit\ &cestu\ k\ prohledávání\.\.\.
menutrans Ta&g\ Files\.\.\.				Ta&g\ soubory\.\.\.
menutrans Toggle\ &Toolbar				Pøepnout\ &Toolbar
menutrans Toggle\ &Bottom\ Scrollbar			Pø&epnout\ dolní\ rolovací\ lištu
menutrans Toggle\ &Left\ Scrollbar			Pøepnout\ &levou\ rolovací\ lištu
menutrans Toggle\ &Right\ Scrollbar			Pøepnout\ p&ravou\ rolovací\ lištu
	" {{{2 Edit -2
menutrans F&ile\ Settings				Nastavení\ so&uboru
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	Pøepnout\ èíslování\ øá&dkù<Tab>:set\ nu!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		Pøepnout\ &List\ mód<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrap<Tab>:set\ wrap!		Pøepnout\ zala&mování\ øádkù<Tab>:set\ wrap!
menutrans Toggle\ W&rap\ at\ word<Tab>:set\ lbr!	Pøepnout\ zl&om\ ve\ slovì<Tab>:set\ lbr!
menutrans Toggle\ &expand-tab<Tab>:set\ et!		Pøepnout\ &expand-tab<Tab>:set\ et!
menutrans Toggle\ &auto-indent<Tab>:set\ ai!		Pøepnout\ &auto-indent<Tab>:set\ ai!
menutrans Toggle\ &C-indenting<Tab>:set\ cin!		Pøepnout\ &C-indenting<Tab>:set\ cin!
menutrans &Shiftwidth					Nastav&it\ šíøku\ od&sazení
menutrans Soft\ &Tabstop				Nastavit\ Soft\ &Tabstop
menutrans Te&xt\ Width\.\.\.				Šíøka\ te&xtu\.\.\.
menutrans &File\ Format\.\.\.				&Formát\ souboru\.\.\.
	" {{{2 Edit -3
menutrans C&olor\ Scheme		Barevné\ s&chéma
menutrans &Keymap			Klávesová\ m&apa
menutrans Select\ Fo&nt\.\.\.		Vybrat\ pís&mo\.\.\.
" }}}1

" {{{ Programming menu
menutrans &Tools			Nást&roje
menutrans &Jump\ to\ this\ tag<Tab>g^]	&Skoèit\ na\ tag<Tab>g^]
menutrans Jump\ &back<Tab>^T		Skoèit\ &zpìt<Tab>^T
menutrans Build\ &Tags\ File		&Vytvoøit\ soubor\ tagù

menutrans &Spelling			&Kontrola\ pravopisu
menutrans &Spell\ Check\ On		Kontrola\ pravopisu\ &zapnuta
menutrans Spell\ Check\ &Off		Kontrola\ pravopisu\ &vypnuta
menutrans To\ Next\ error<Tab>]s	&Další\ chyba<Tab>]s
menutrans To\ Previous\ error<Tab>[s	&Pøedchozí\ chyba<Tab>[s
menutrans Suggest\ Corrections<Tab>z?	&Návrh\ oprav<Tab>z?
menutrans Repeat\ correction<Tab>:spellrepall	Zopakovat\ &opravu<Tab>:spellrepall
menutrans Set\ language\ to\ "en"	Nastav\ jazyk\ na\ "en"
menutrans Set\ language\ to\ "en_au"	Nastav\ jazyk\ na\ "en_au"
menutrans Set\ language\ to\ "en_ca"	Nastav\ jazyk\ na\ "en_ca"
menutrans Set\ language\ to\ "en_gb"	Nastav\ jazyk\ na\ "en_gb"
menutrans Set\ language\ to\ "en_nz"	Nastav\ jazyk\ na\ "en_nz"
menutrans Set\ language\ to\ "en_us"	Nastav\ jazyk\ na\ "en_us"
menutrans Set\ language\ to\ "cz"	Nastav\ jazyk\ na\ "cz"
menutrans Set\ language\ to\ "cs_cz"	Nastav\ jazyk\ na\ "cs_cz"
menutrans &Find\ More\ Languages	Nalézt\ další\ &jazyky

menutrans &Folding			&Foldy
menutrans &Enable/Disable\ folds<Tab>zi &Ano/Ne<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv	&Zobrazit\ øádek\ kurzoru<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx		Zo&brazit\ pouze\ øádek\ kurzoru\ <Tab>zMzx
menutrans C&lose\ more\ folds<Tab>zm	&Vyjmout\ jednu\ úroveò\ foldù<Tab>zm
menutrans &Close\ all\ folds<Tab>zM	Zavøí&t\ všechny\ foldy<Tab>zM
menutrans O&pen\ more\ folds<Tab>zr	Pøidat\ jedn&u\ úroveò\ foldù<Tab>zr
menutrans &Open\ all\ folds<Tab>zR	&Otevøít\ všechny\ foldy<Tab>zR
menutrans Fold\ Met&hod			Metoda\ &skládání
	"menutrans M&anual			&Ruènì
	"menutrans I&ndent			&Odsazení
	"menutrans E&xpression			&Výraz
	"menutrans S&yntax			&Syntax
	"menutrans &Diff			&Diff
	"menutrans Ma&rker			Ma&rker
menutrans Create\ &Fold<Tab>zf		Vytvoøit\ &fold<Tab>zf
menutrans &Delete\ Fold<Tab>zd		Vymazat\ fol&d<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD	V&ymazat\ všechny\ foldy<Tab>zD
menutrans Fold\ col&umn\ width		Sloupec\ zob&razení\ foldù

menutrans &Update			&Obnovit
menutrans &Get\ Block			&Sejmout\ Blok
menutrans &Put\ Block			&Vložit\ Blok
menutrans &Make<Tab>:make		&Make<Tab>:make
menutrans &List\ Errors<Tab>:cl		Výpis\ &chyb<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!	Výp&is\ zpráv<Tab>:cl!
menutrans &Next\ Error<Tab>:cn		Další\ ch&yba<Tab>:cn
menutrans &Previous\ Error<Tab>:cp	&Pøedchozí\ chyba<Tab>:cp
menutrans &Older\ List<Tab>:cold	Sta&rší\ seznam<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew	N&ovìjší\ seznam<Tab>:cnew
menutrans Error\ &Window		Chybové\ o&kno
menutrans SeT\ Compiler			Nas&tavení\ kompilátoru
menutrans &Update<Tab>:cwin		O&bnovit<Tab>:cwin
menutrans &Open<Tab>:copen		&Otevøít<Tab>:copen
menutrans &Close<Tab>:cclose		&Zavøít<Tab>:cclose
menutrans &Set\ Compiler		N&astavit\ kompilátor

menutrans &Convert\ to\ HEX<Tab>:%!xxd	Pøevést\ do\ šestnáctkového\ formát&u<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!xxd\ -r Pø&evést\ zpìt<Tab>:%!xxd\ -r
" }}}

" {{{ Syntax menu
menutrans &Syntax		Synta&xe
menutrans Set\ '&syntax'\ only	Nastavit\ pouze\ 'synta&x'
menutrans Set\ '&filetype'\ too	Nastavit\ také\ '&filetype'
menutrans &Off			&Vypnout
menutrans &Manual		&Ruènì
menutrans A&utomatic		A&utomaticky
menutrans on/off\ for\ &This\ file	&Pøepnout\ (pro\ tento\ soubor)
menutrans o&ff\ (this\ file)	vyp&nout\ (pro\ tento\ soubor)
menutrans Co&lor\ test		Test\ &barev
menutrans &Highlight\ test	&Test\ zvýrazòování
menutrans &Convert\ to\ HTML	Pøevést\ &do\ HTML
menutrans &Show\ filetypes\ in\ menu	&Zobrazit\ výbìr\ možností
" }}}

" {{{ Menu Buffers
menutrans &Buffers		&Buffery
menutrans &Refresh\ menu	&Obnovit\ menu
menutrans &Delete		Z&rušit
menutrans &Alternate		&Zmìnit
menutrans &Next			&Další
menutrans &Previous		&Pøedchozí
menutrans [No\ File]		[Žádný\ soubor]
" }}}

" {{{ Menu Window
menutrans &Window			&Okna
menutrans &New<Tab>^Wn			&Nové<Tab>^Wn
menutrans S&plit<Tab>^Ws		&Rozdìlit<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^	Ro&zdìlit\ na\ #<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv	Rozdìlit\ &vertikálnì<Tab>^Wv
menutrans Split\ File\ E&xplorer	Rozdìlit\ -\ File\ E&xplorer
menutrans Move\ &To			&Pøesun
menutrans &Top<Tab>^WK			&Nahoru<Tab>^WK
menutrans &Bottom<Tab>^WJ		&Dolu<Tab>^WJ
menutrans &Left\ side<Tab>^WH		&Vlevo<Tab>^WH
menutrans &Right\ side<Tab>^WL		Vp&ravo<Tab>^WL

menutrans &Close<Tab>^Wc		Zavøí&t<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo	Zavøít\ &ostatní<Tab>^Wo
menutrans Ne&xt<Tab>^Ww			&Další<Tab>^Ww
menutrans P&revious<Tab>^WW		&Pøedchozí<Tab>^WW
menutrans &Equal\ Size<Tab>^W=		&Stejná\ výška<Tab>^W=
menutrans &Max\ Height<Tab>^W_		Maximální\ výš&ka<Tab>^W_
menutrans M&in\ Height<Tab>^W1_		M&inimální\ výška<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|		&Maximální\ šíøka<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|		Minimální\ šíøk&a<Tab>^W1\|
menutrans Rotate\ &Up<Tab>^WR		Rotovat\ na&horu<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		Rotovat\ &dolù<Tab>^Wr

" {{{ Help menu
menutrans &Help			&Nápovìda
menutrans &Overview<Tab><F1>	&Pøehled<Tab><F1>
menutrans &User\ Manual		&Uživatelský\ Manuál
menutrans &How-to\ links	Ho&wto
menutrans &GUI			&Grafické\ rozhraní
menutrans &Credits		&Autoøi
menutrans Co&pying		&Licenèní\ politika
menutrans &Sponsor/Register	Sponzorování/&Registrace
menutrans &Find\.\.\.		&Hledat\.\.\.
menutrans O&rphans		O&siøelé\ dìti
menutrans &Version		&Verze
menutrans &About		&O\ aplikaci
" }}}

" {{{ The popup menu
menutrans &Undo			&Zpìt
menutrans Cu&t			&Vyøíznout
menutrans &Copy			&Kopírovat
menutrans &Paste		&Vložit
menutrans &Delete		&Smazat
menutrans Select\ Blockwise	Vybrat\ blokovì
menutrans Select\ &Word		Vybrat\ &slovo
menutrans Select\ &Line		Vybrat\ &øádek
menutrans Select\ &Block	Vybrat\ &blok
menutrans Select\ &All		Vybrat\ &vše
" }}}

" {{{ The GUI toolbar
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		Otevøít soubor
    tmenu ToolBar.Save		Uložit soubor
    tmenu ToolBar.SaveAll		Uložit všechny soubory
    tmenu ToolBar.Print		Tisk
    tmenu ToolBar.Undo		Zpìt
    tmenu ToolBar.Redo		Zrušit vrácení
    tmenu ToolBar.Cut		Vyøíznout
    tmenu ToolBar.Copy		Kopírovat
    tmenu ToolBar.Paste		Vložit
    tmenu ToolBar.Find		Hledat...
    tmenu ToolBar.FindNext	Hledat další
    tmenu ToolBar.FindPrev	Hledat pøedchozí
    tmenu ToolBar.Replace		Nahradit...
    if 0	" disabled; These are in the Windows menu
      tmenu ToolBar.New		Nové okno
      tmenu ToolBar.WinSplit	Rozdìlit okno
      tmenu ToolBar.WinMax		Maximalizovat okno
      tmenu ToolBar.WinMin		Minimalizovat okno
      tmenu ToolBar.WinClose	Zavøít okno
    endif
    tmenu ToolBar.LoadSesn	Naèíst sezení
    tmenu ToolBar.SaveSesn	Uložit sezení
    tmenu ToolBar.RunScript	Spustit skript
    tmenu ToolBar.Make		Spustit make
    tmenu ToolBar.Shell		Spustit shell
    tmenu ToolBar.RunCtags	Spustit ctags
    tmenu ToolBar.TagJump		Skoèit na tag pod kurzorem
    tmenu ToolBar.Help		Nápovìda
    tmenu ToolBar.FindHelp	Hledat nápovìdu k...
  endfun
endif
" }}}
