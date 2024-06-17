" Menu Translations:	Afrikaas
" Maintainer:		Danie Roux <droux@tuks.co.za>
" Last Change:		2024 May 2
" Original translations

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1
let s:keepcpo= &cpo
set cpo&vim

" The translations below are in latin1, but they work for cp1252 and
" iso-8859-15 without conversion as well.
if &enc != "cp1252" && &enc != "iso-8859-15"
  scriptencoding latin1
endif


" Help menu
menutrans &Help			&Hulp
menutrans &Overview<Tab><F1>	&Oorsig<Tab><F1>
menutrans &How-to\ links	&How-to\ Indeks
"menutrans &GUI			&GUI
menutrans &Credits		&Met\ dank\ aan
menutrans Co&pying		&Kopiereg
menutrans &Find\.\.\.		&Soek\.\.\.
menutrans &Version		&Weergawe
menutrans &About		&Inleiding\ skerm

" File menu
menutrans &File				&Lêer
menutrans &Open\.\.\.<Tab>:e		Maak\ oop\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	Maak\ oop\ in\ nuwe\ &venster\.\.\.<Tab>:sp
menutrans Open\ &Tab\.\.\.<Tab>:tabnew	Maak\ oortjie\ oop\.\.\.<Tab>:tabnew
menutrans &New<Tab>:enew		&Nuut<Tab>:enew
menutrans &Close<Tab>:close		Maak\ &Toe<Tab>:close
menutrans &Save<Tab>:w			&Skryf<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	Skryf\ &as\.\.\.<Tab>:sav
menutrans &Print			&Druk
menutrans Sa&ve-Exit<Tab>:wqa		Skryf\ en\ verlaat<Tab>:wqa
menutrans E&xit<Tab>:qa			&Verlaat<Tab>:qa

" Edit menu
menutrans &Edit				&Wysig
menutrans &Undo<Tab>u			Terug<Tab>u
menutrans &Redo<Tab>^R			Voo&ruit<Tab>^R
menutrans Rep&eat<Tab>\.			&Herhaal<Tab>\.
menutrans Cu&t<Tab>"+x			&Knip<Tab>"+x
menutrans &Copy<Tab>"+y			&Kopiëer<Tab>"+y
menutrans &Paste<Tab>"+gP		Plak<Tab>"+gP
menutrans Put\ &Before<Tab>[p		Voeg\ &Voor\ in<Tab>[p
menutrans Put\ &After<Tab>]p		Voeg\ A&gter\ in<Tab>]p
menutrans &Select\ all<Tab>ggVG		Kies\ &Alles<Tab>ggVG
menutrans &Find\.\.\.			&Soek\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.			Soek\ en\ Vervang\.\.\.
menutrans Options\.\.\.			Opsies\.\.\.

" Programming menu
menutrans &Tools			&Gereedskap
menutrans &Jump\ to\ this\ tag<Tab>g^]	&Spring\ na\ Etiket<Tab>g^]
menutrans Jump\ &back<Tab>^T		Spring\ &Terug<Tab>^T
menutrans Build\ &Tags\ File		Genereer\ &Etiket\ Leêr
menutrans &Make<Tab>:make		Voer\ &Make\ uit<Tab>:make
menutrans &List\ Errors<Tab>:cl		&Foutlys<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!	&Boodskaplys<Tab>:cl!
menutrans &Next\ Error<Tab>:cn		Volgende\ Fout<Tab>:cn
menutrans &Previous\ Error<Tab>:cp	Vorige\ Fout<Tab>:cp
menutrans &Older\ List<Tab>:cold	&Ouer\ Lys<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew	&Nuwer\ Lys<Tab>:cnew
menutrans Error\ &Window		Foute\ Venster<Tab>:cwin
menutrans &Convert\ to\ HEX<Tab>:%!xxd	Verwissel\ na\ HEX<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!xxd\ -r	Verwissel\ terug<Tab>:%!xxd\ -r

" Names for buffer menu.
menutrans &Buffers	&Buffers
menutrans &Refresh\ menu	Verfris
menutrans Delete	Verwyder
menutrans Alternate	Vorige
menutrans [No\ Name]	[Geen\ Leêr]
menutrans &Next		Volgende
menutrans &Previous	Vorige

" Window menu
menutrans &Window			&Venster
menutrans &New<Tab>^Wn			&Nuut<Tab>^Wn
menutrans S&plit<Tab>^Ws		Ver&deel<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^	Verdeel\ N&a\ #<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv		Verdeel\ Vertikaal<Tab>^Wv
menutrans &Close<Tab>^Wc		&Maak\ toe<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo	Maak\ &Ander\ Toe<Tab>^Wo
menutrans Ne&xt<Tab>^Ww			Volgende<Tab>^Ww
menutrans P&revious<Tab>^WW		&Vorige<Tab>^WW
menutrans &Equal\ Size<Tab>^W=		&Gelyke\ hoogte<Tab>^W=
menutrans &Max\ Height<Tab>^W_		&Maksimale\ hoogte<Tab>^W_
menutrans M&in\ Height<Tab>^W1_		Mi&nimale\ hoogte<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|		Maksimale\ breedte<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|		Minimale\ breedte<Tab>^W1\|
menutrans Rotate\ &Up<Tab>^WR		Roteer\ na\ &bo<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		Roteer\ na\ &onder<Tab>^Wr
menutrans Select\ Fo&nt\.\.\.		Kies\ font\.\.\.

" The popup menu
menutrans &Undo			&Terug
menutrans Cu&t			Knip
menutrans &Copy			&Kopiëer
menutrans &Paste		&Plak
menutrans &Delete		&Verwyder
menutrans Select\ Blockwise	Kies\ per\ Blok
menutrans Select\ &Word		Kies\ een\ &Woord
menutrans Select\ &Line		Kies\ een\ &Reël
menutrans Select\ &Block	Kies\ een\ &Blok
menutrans Select\ &All		Kies\ &Alles

" The GUI toolbar
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		Maak leêr oop
    tmenu ToolBar.Save		Skryf leêr
    tmenu ToolBar.SaveAll	Skryf alle leêrs
    tmenu ToolBar.Print		Druk
    tmenu ToolBar.Undo		Terug
    tmenu ToolBar.Redo		Vooruit
    tmenu ToolBar.Cut		Knip
    tmenu ToolBar.Copy		Kopiëer
    tmenu ToolBar.Paste		Plak
    tmenu ToolBar.Find		Soek...
    tmenu ToolBar.FindNext	Soek volgende
    tmenu ToolBar.FindPrev	Soek vorige
    tmenu ToolBar.Replace	Soek en vervang...
    tmenu ToolBar.LoadSesn	Laai sessie
    tmenu ToolBar.SaveSesn	Stoor sessie
    tmenu ToolBar.RunScript	Voer vim skrip uit
    tmenu ToolBar.Make		Voer make uit
    tmenu ToolBar.Shell		Begin dop
    tmenu ToolBar.RunCtags	Genereer etikette
    tmenu ToolBar.TagJump	Spring na etiket
    tmenu ToolBar.Help		Hulp
    tmenu ToolBar.FindHelp	Soek hulp...
  endfun
endif

" Syntax menu
menutrans &Syntax		&Sintaks
menutrans Set\ 'syntax'\ only		Stel\ slegs\ 'syntax'
menutrans Set\ 'filetype'\ too	Verander\ 'filetype'\ ook
menutrans &Off			&Af
menutrans &Manual		&Met\ die\ hand
menutrans A&utomatic		O&utomaties
menutrans o&n\ (this\ file)		Aa&n\ (die\ leêr)
menutrans o&ff\ (this\ file)	&Af\ (die\ leêr)
menutrans Co&lor\ test		Toets\ die\ &kleure
menutrans &Highlight\ test	Toets\ die\ verligting
menutrans &Convert\ to\ HTML	Verwissel\ na\ HTML
menutrans Split\ &Diff\ with\.\.\.	Verdeel\ ewenaar\ met\.\.\.
menutrans Split\ Patched\ &By\.\.\.	Verdeel\ gelap\ deur\.\.\.
menutrans Settings\ &Window		Instellings\ venster\.\.\.
menutrans Startup\ &Settings		Opstart\ instellings
menutrans &Global\ Settings		Globale\ instellings
menutrans F&ile\ Settings		Lêer\ instellings
menutrans C&olor\ Scheme		Kleurskema
menutrans &Keymap			Sleutelkaart
">>>----------------- Edit/Global settings
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!		Wissel\ patroon\ hoogtepunt<Tab>:set\ hls!
menutrans Toggle\ &Ignoring\ Case<Tab>:set\ ic!			Wissel\ geval\ ignoreer<Tab>:set\ ic!
menutrans Toggle\ &Showing\ Matched\ Pairs<Tab>:set\ sm!	Wissel\ wat\ ooreenstemmende\ pare\ wys<Tab>:set\ sm!
menutrans &Context\ lines					Kontekslyne
menutrans &Virtual\ Edit					Virtuele\ wysiging
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!			Wissel\ invoegmodus<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatibility<Tab>:set\ cp!		Wissel\ Vi\ verenigbaarheid<Tab>:set\ cp!
menutrans Search\ &Path\.\.\.					Soek\ pad\.\.\.
menutrans Ta&g\ Files\.\.\.					Merk\ lêers\.\.\.
"
menutrans Toggle\ &Toolbar		Wissel\ nutsbalk
menutrans Toggle\ &Bottom\ Scrollbar	Wissel\ onderste\ skuifbalk
menutrans Toggle\ &Left\ Scrollbar	Wissel\ linker\ skuifbalk
menutrans Toggle\ &Right\ Scrollbar	Wissel\ regs\ skuifbalk
">>>->>>------------- Edit/Global settings/Virtual edit
menutrans Never				Nooit\ nie
menutrans Block\ Selection		Blokkeuse
menutrans Insert\ mode			Invoegmodus
menutrans Block\ and\ Insert		Blokkeer\ en\ plaas
menutrans Always			Altyd
">>>----------------- Edit/File settings
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!		Wissel\ lynnommering<Tab>:set\ nu!
menutrans Toggle\ relati&ve\ Line\ Numbering<Tab>:set\ rnu!	Wissel\ relatiewe\ lynnommering<Tab>:set\ nru!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!			Wissel\ lys\ modus<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrapping<Tab>:set\ wrap!		Wissel\ lyn\ wikkel<Tab>:set\ wrap!
menutrans Toggle\ W&rapping\ at\ word<Tab>:set\ lbr!		Wissel\ omvou\ by\ woord<Tab>:set\ lbr!
menutrans Toggle\ Tab\ &Expanding<Tab>:set\ et!			Wissel\ oortjie\ wat\ uitbrei<Tab>:set\ et!
menutrans Toggle\ &Auto\ Indenting<Tab>:set\ ai!		Wissel\ outomatiese\ inkeping<Tab>:set\ ai!
menutrans Toggle\ &C-Style\ Indenting<Tab>:set\ cin!		Wissel\ C-styl\ inkeping<Tab>:set\ cin!
">>>---
menutrans &Shiftwidth				Skuifwydte
menutrans Soft\ &Tabstop			Sagte\ tabstop
menutrans Te&xt\ Width\.\.\.			Teks\ breedte\.\.\.
menutrans &File\ Format\.\.\.			Lêerformaat\.\.\.
">>>---------------- Tools/Spelling
menutrans &Spell\ Check\ On			Speltoets\ aan
menutrans Spell\ Check\ &Off			Spelmerk\ af
menutrans To\ &Next\ error<Tab>]s		Na\ die\ volgende\ fout<Tab>]s
menutrans To\ &Previous\ error<Tab>[s		Om\ vorige\ fout<Tab>[s
menutrans Suggest\ &Corrections<Tab>z=		Stel\ regstellings\ voor<Tab>z=
menutrans &Repeat\ correction<Tab>:spellrepall	Herhaal\ regstelling<Tab>:spellrepall
"-------------------
menutrans Set\ language\ to\ "en"		Stel\ taal\ op\ "en"
menutrans Set\ language\ to\ "en_au"		Stel\ taal\ op\ "en_au"
menutrans Set\ language\ to\ "en_ca"		Stel\ taal\ op\ "en_ca"
menutrans Set\ language\ to\ "en_gb"		Stel\ taal\ op\ "en_gb"
menutrans Set\ language\ to\ "en_nz"		Stel\ taal\ op\ "en_nz"
menutrans Set\ language\ to\ "en_us"		Stel\ taal\ op\ "en_us"
menutrans &Find\ More\ Languages		Vind\ meer\ tale
let g:menutrans_set_lang_to =			'Stel taal op'
">>>---------------- Folds
menutrans &Enable/Disable\ folds<Tab>zi		Aktiveer/deaktiveer\ voue<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv		Bekyk\ wyserlyn<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx	Bekyk\ slegs\ wyserlyn<Tab>z\ Shift+M\ zx
menutrans C&lose\ more\ folds<Tab>zm		Maak\ nog\ voue\ toe<Tab>zm
menutrans &Close\ all\ folds<Tab>zM		Maak\ alle\ voue\ toe<Tab>z\ Shift+M
menutrans &Open\ all\ folds<Tab>zR		Maak\ alle\ voue\ oop<Tab>z\ Shift+R
menutrans O&pen\ more\ folds<Tab>zr		Maak\ meer\ voue\ oop<Tab>zr
menutrans Fold\ Met&hod				Vou\ metode
menutrans Create\ &Fold<Tab>zf			Skep\ vou<Tab>zf
menutrans &Delete\ Fold<Tab>zd			Vee\ vou\ uit<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD		Vee\ alle\ voue\ uit<Tab>z\ Shift+D
menutrans Fold\ col&umn\ width			Vou\ kolomwydte
">>>->>>----------- Tools/Folds/Fold Method
menutrans M&anual				Handleiding
menutrans I&ndent				Inkeep
menutrans E&xpression				Uitdrukking
menutrans S&yntax				Sintaksis
menutrans &Diff					Verskil
menutrans Ma&rker				Merker
">>>--------------- Sub of Tools/Diff
menutrans &Update				Opdateer
menutrans &Get\ Block				Kry\ blok
menutrans &Put\ Block				Sit\ blok
">>>--------------- Tools/Error window
menutrans &Update<Tab>:cwin			Opdateer<Tab>:cwin
menutrans &Close<Tab>:cclose			Naby<Tab>:cclose
menutrans &Open<Tab>:copen			Maak\ oop<Tab>:copen
menutrans Se&T\ Compiler			Stel\ samesteller
menutrans &Show\ File\ Types\ in\ menu		Wys\ lêertipes\ in\ kieslys
menutrans on/off\ for\ &This\ file		Aan/af\ vir\ hierdie\ lêer
menutrans Split\ File\ E&xplorer		Verdeel\ lêerverkenner
menutrans Move\ &To				Skuif\ na
">>>----------------- Submenu of Window/Move To
menutrans &Top<Tab>^WK				Top<Tab>Ctrl+W\ Shift+K
menutrans &Bottom<Tab>^WJ			Onderkant<Tab>Ctrl+W\ Shift+J
menutrans &Left\ side<Tab>^WH			Linkerkant<Tab>Ctrl+W\ Shift+H
menutrans &Right\ side<Tab>^WL			Regter\ kant<Tab>Ctrl+W\ Shift+L
menutrans &User\ Manual				Gebruikers\ gids
menutrans &Sponsor/Register			Borg/registreer
menutrans &Folding				Vou
menutrans &Spelling				&Spelling

let &cpo = s:keepcpo
unlet s:keepcpo
