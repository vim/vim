" Menu Translations:	Italian / Italiano
" Maintainer:		Antonio Colombo <azc100@gmail.com>
" Last Change:	2025 Mar 13
" Original translations

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1
let s:cpo_save= &cpo
set cpo&vim

scriptencoding iso-8859-1

" Help / Aiuto
menut &Help			&Aiuto

menut &Overview<Tab><F1>	&Panoramica<Tab><F1>
menut &User\ Manual		Manuale\ &Utente
menut &How-To\ Links	Co&Me\.\.\.
menut &Find\.\.\.	&Cerca\.\.\.
menut &Credits		Cr&Editi
menut Co&pying		C&Opie
menut &Sponsor/Register	&Sponsor/Registrazione
menut O&rphans		O&Rfani
menut &Version		&Versione
menut &About		&Intro

let g:menutrans_help_dialog = "Batti un comando o una parola per cercare aiuto su:\n\nPremetti i_ per comandi in modo Input (p.es.: i_CTRL-X)\nPremetti c_ per comandi che editano la linea-comandi (p.es.: c_<Del>)\nPremetti ' per un nome di opzione (p.es.: 'shiftwidth')"

" File / File
menut &File				&File

menut &Open\.\.\.<Tab>:e		&Apri\.\.\.<Tab>:e
menut Sp&lit-Open\.\.\.<Tab>:sp		A&Pri\ nuova\ finestra\.\.\.<Tab>:sp
menut Open\ &Tab\.\.\.<Tab>:tabnew	Apri\ nuova\ &Linguetta\.\.\.<Tab>:tabnew
menut &New<Tab>:enew			&Nuovo<Tab>:enew
menut &Close<Tab>:close			&Chiudi<Tab>:close
menut &Save<Tab>:w			&Salva<Tab>:w
menut Save\ &As\.\.\.<Tab>:sav		Salva\ &Con\ nome\.\.\.<Tab>:sav
menut Split\ &Diff\ With\.\.\.		&Differenza\ con\.\.\.
menut Split\ Patched\ &By\.\.\.		Patc&H\ da\.\.\.
menut &Print				S&tampa
menut Sa&ve-Exit<Tab>:wqa		Sa&Lva\ ed\ esci<Tab>:wqa
menut E&xit<Tab>:qa			&Esci<Tab>:qa

" Edit / Modifica
menut &Edit				&Modifica

menut &Undo<Tab>u			&Annulla<Tab>u
menut &Redo<Tab>^R			&Ripristina<Tab>^R
menut Rep&eat<Tab>\.			Ri&Peti<Tab>\.
menut Cu&t<Tab>"+x			&Taglia<Tab>"+x
menut &Copy<Tab>"+y			&Copia<Tab>"+y
menut &Paste<Tab>"+gP			&Incolla<Tab>"+gP
menut Put\ &Before<Tab>[p		&Metti\ davanti<Tab>[p
menut Put\ &After<Tab>]p		M&Etti\ dietro<Tab>]p
menut &Delete<Tab>x			Cance&Lla<Tab>x
menut &Select\ All<Tab>ggVG		Seleziona\ &Tutto<Tab>ggVG
menut &Find\.\.\.			&Cerca\.\.\.
menut &Find<Tab>/			&Cerca<Tab>/
menut Find\ and\ Rep&lace\.\.\.		&Sostituisci\.\.\.
menut Find\ and\ Rep&lace<Tab>:%s	&Sostituisci<Tab>:%s
menut Find\ and\ Rep&lace<Tab>:s	&Sostituisci<Tab>:s
menut Settings\ &Window			&Finestra\ Impostazioni
menut Startup\ &Settings		Impostazioni\ di\ &Avvio
menut &Global\ Settings			Impostazioni\ &Globali

" Edit / Modifica / Impostazioni Globali

menut Toggle\ Pattern\ &Highlight<Tab>:set\ hls!	&Evidenzia\ ricerche\ Sì/No<Tab>:set\ hls!
menut Toggle\ &Ignoring\ Case<Tab>:set\ ic!		&Ignora\ maiusc\.-minusc\.\ Sì/No<Tab>:set\ ic!
menut Toggle\ &Showing\ Matched\ Pairs<Tab>:set\ sm!			Indica\ &Corrispondenze\ Sì/No<Tab>:set\ sm!

menut &Context\ Lines			&Linee\ di\ contesto
"menut &Context\ Lines.\ 1\		&Linee\ di\ contesto.\ 1\
"menut &Context\ Lines.\ 2\		&Linee\ di\ contesto.\ 2\
"menut &Context\ Lines.\ 3\		&Linee\ di\ contesto.\ 3\
"menut &Context\ Lines.\ 4\		&Linee\ di\ contesto.\ 4\
"menut &Context\ Lines.\ 5\		&Linee\ di\ contesto.\ 5\
"menut &Context\ Lines.\ 7\		&Linee\ di\ contesto.\ 7\
"menut &Context\ Lines.\ 10\		&Linee\ di\ contesto.\ 10\
"menut &Context\ Lines.\ 100\		&Linee\ di\ contesto.\ 100\

menut &Virtual\ Edit		&Edit\ virtuale
menut Never			Mai
menut Block\ Selection		Seleziona\ Blocco
menut Insert\ Mode		Modo\ Insert
menut Block\ and\ Insert	Selezione\ Blocco\ e\ Inserimento
menut Always			Sempre

menut Toggle\ Insert\ &Mode<Tab>:set\ im!	&Modo\ Insert\ Sì/No<Tab>:set\ im!
menut Toggle\ Vi\ C&ompatibility<Tab>:set\ cp!	C&Ompatibilità\ VI\ Sì/No<Tab>:set\ cp!
menut Search\ &Path\.\.\.	&Percorso\ di\ ricerca\.\.\.
menut Ta&g\ Files\.\.\.		File\ ta&G\.\.\.
menut Toggle\ &Toolbar			Barra\ s&Trumenti\ Sì/No
menut Toggle\ &Bottom\ Scrollbar	Barra\ scorrimento\ in\ &Fondo\ Sì/No
menut Toggle\ &Left\ Scrollbar		Barra\ scorrimento\ a\ &Sinistra\ Sì/No
menut Toggle\ &Right\ Scrollbar		Barra\ scorrimento\ a\ &Destra\ Sì/No

if has("toolbar")
   if exists("*Do_toolbar_tmenu")
      delfun Do_toolbar_tmenu
   endif
   fun Do_toolbar_tmenu()
      tmenu ToolBar.Open		Apri file
      tmenu ToolBar.Save		Salva file
      tmenu ToolBar.SaveAll		Salva tutti i file
      if has("printer") || has("unix")
         tmenu ToolBar.Print		Stampa
      endif
      tmenu ToolBar.Undo		Annulla
      tmenu ToolBar.Redo		Rifai
      tmenu ToolBar.Cut			Taglia
      tmenu ToolBar.Copy		Copia
      tmenu ToolBar.Paste		Incolla
      tmenu ToolBar.FindNext		Trova seguente
      tmenu ToolBar.FindPrev		Trova precedente
      tmenu ToolBar.Replace		Sostituisci
      tmenu ToolBar.LoadSesn		Carica sessione
      tmenu ToolBar.SaveSesn		Salva sessione
      tmenu ToolBar.RunScript		Esegui script
      tmenu ToolBar.Make		Esegui make
      tmenu ToolBar.RunCtags		Esegui ctags
      tmenu ToolBar.TagJump		Salta alla tag
      tmenu ToolBar.Help		Aiuto
      tmenu ToolBar.FindHelp		Trova aiuto...
   endfun
endif

let g:menutrans_path_dialog = "Batti percorso di ricerca per i file.\nSepara fra loro i nomi di directory con una virgola."
let g:menutrans_tags_dialog = "Batti nome dei file di tag.\nSepara fra loro i nomi di directory con una virgola."

" Edit / Impostazioni File
menut F&ile\ Settings	&Impostazioni\ file

" Boolean options
menut Toggle\ Line\ &Numbering<Tab>:set\ nu!		&Numerazione\ Sì/No<Tab>:set\ nu!
menut Toggle\ Relati&ve\ Line\ Numbering<Tab>:set\ rnu!	Numerazione\ relati&Va\ Sì/No<Tab>:set\ rnu!
menut Toggle\ &List\ Mode<Tab>:set\ list!		Modo\ &List\ Sì/No<Tab>:set\ list!
menut Toggle\ Line\ &Wrapping<Tab>:set\ wrap!		Linee\ &Continuate\ Sì/No<Tab>:set\ wrap!
menut Toggle\ W&rapping\ at\ Word<Tab>:set\ lbr!	A\ capo\ alla\ &Parola\ Sì/No<Tab>:set\ lbr!
menut Toggle\ Tab\ &Expanding<Tab>:set\ et!		&Espandi\ Tabulazione\ Sì/No<Tab>:set\ et!
menut Toggle\ &Auto\ Indenting<Tab>:set\ ai!		Indentazione\ &Automatica\ Sì/No<Tab>:set\ ai!
menut Toggle\ &C-Style\ Indenting<Tab>:set\ cin!	Indentazione\ stile\ &C\ Sì/No<Tab>:set\ cin!
menut &Shiftwidth					&Spazi\ rientranza
"menut &Shiftwidth.3					&Spazi\ rientranza.3
"menut &Shiftwidth.4					&Spazi\ rientranza.4
"menut &Shiftwidth.5					&Spazi\ rientranza.5
"menut &Shiftwidth.6					&Spazi\ rientranza.6
"menut &Shiftwidth.8					&Spazi\ rientranza.8
menut Soft\ &Tabstop					&Tabulazione\ software
"menut Soft\ &Tabstop.2					&Tabulazione\ software.2
"menut Soft\ &Tabstop.3					&Tabulazione\ software.3
"menut Soft\ &Tabstop.4					&Tabulazione\ software.4
"menut Soft\ &Tabstop.5					&Tabulazione\ software.5
"menut Soft\ &Tabstop.6					&Tabulazione\ software.6
"menut Soft\ &Tabstop.8					&Tabulazione\ software.8
menut Te&xt\ Width\.\.\.				Lunghe&Zza\ riga\.\.\.
menut &File\ Format\.\.\.				Formato\ &File\.\.\.

let g:menutrans_textwidth_dialog = "Batti nuova lunghezza linea (0 per inibire la formattazione): "
let g:menutrans_fileformat_dialog = "Scegli formato con cui scrivere il file"
let g:menutrans_fileformat_choices = " &Unix\n&Dos\n&Mac\n&Annullare "

menut Show\ C&olor\ Schemes\ in\ Menu	Mostra\ Schemi\ C&olore\ in\ Menù
menut C&olor\ Scheme		Schema\ c&Olori

menut blue		blù
menut darkblue		blù\ scuro
menut default		predefinito
menut delek		delek
menut desert		deserto
menut elflord		signore\ degli\ elfi
menut evening		sera
menut habamax		habamax
menut industry		industria
menut koehler		koehler
menut lunaperche	luna\ perché
menut morning		mattino
menut murphy		murphy
menut pablo		pablo
menut peachpuff		pesca
menut quiet		quieto
menut retrobox		retrobox
menut ron		ron
menut shine		brillante
menut sorbet		sorbetto
menut slate		ardesia
menut torte		torta
menut unokai		unokai
menut wildcharm		fascino\ selvaggio
menut zaibatsu		zaibatsu
menut zellner		zellner
menut BLUE		BLÙ
menut DARKBLUE		BLÙ\ SCURO
menut DEFAULT		DEFAULT
menut DELEK		DELEK
menut DESERT		DESERTO
menut ELFLORD		SIGNORE\ DEGLI\ ELFI
menut EVENING		SERA
menut HABAMAX		HABAMAX
menut INDUSTRY		INDUSTRIA
menut KOEHLER		KOEHLER
menut LUNAPERCHE	LUNA\ PERCHÉ
menut MORNING		MATTINO
menut MURPHY		MURPHY
menut PABLO		PABLO
menut PEACHPUFF		PESCA
menut QUIET		QUIETO
menut RETROBOX		RETROBOX
menut RON		RON
menut SHINE		BRILLANTE
menut SORBET		SORBETTO
menut SLATE		ARDESIA
menut TORTE		TORTA
menut UNOKAI		UNOKAI
menut WILDCHARM		FASCINO\ SELVAGGIO
menut ZAIBATSU		ZAIBATSU
menut ZELLNER		ZELLNER

menut Show\ &Keymaps\ in\ Menu	Mostra\ Ma&ppe\ tastiera\ in\ Menù
menut &Keymap			Ma&ppa\ tastiera

menut None			Nessuna
menut accents			accenti
menut arabic			arabo
menut armenian-eastern		armeno-orientale
menut armenian-western		armeno-occidentale
menut belarusian-jcuken		bielorusso-jcuken
menut bulgarian-bds		bulgaro-bds
menut bulgarian-phonetic	bulgaro-fonetico
menut canfr-win			franco-canadese-win
menut croatian			croato
menut czech			ceco
menut dvorak			tastiera-dvorak
menut esperanto			esperanto
menut french-azerty		francese-azerty
menut georgian-qwerty		georgiano-qwerty
menut german-qwertz		tedesco-qwertz
menut greek			greco
menut hebrew			ebraico
menut hebrewp			ebraico-fonetico
menut kana			kana
menut kazakh-jcuken		kazako-jcuken
menut korean			coreano
menut korean-dubeolsik		coreano-dubeolsik
menut lithuanian-baltic		lituano-baltico
menut magyar			ungherese
menut mongolian			mongolo
menut oldturkic-orkhon		turco-antico-orkhon
menut oldturkic-yenisei		turco-antico-yenisei
menut persian			persiano
menut persian-iranian		persiano-iraniano
menut pinyin			pinyin
menut polish-slash		polacco-slash
menut russian-dvorak		russo-dvorak
menut russian-jcuken		russo-jcuken
menut russian-jcukenmac		russo-jcukenmac
menut russian-jcukenwin		russo-jcukenwin
menut russian-jcukenwintype	russo-jcukenwintype
menut russian-typograph		russo-tipografico
menut russian-yawerty		russo-yawerty
menut serbian			serbo
menut serbian-latin		serbo-latino
menut sinhala			singalese
menut sinhala-phonetic		singalese-phonetic
menut slovak			slovacco
menut tamil			tamil
menut thaana			thaana
menut thaana-phonetic		thaana-fonetico
menut turkish-f			turco-f
menut turkish-q			turco-q
menut ukrainian-dvorak		ucraino-dvorak
menut ukrainian-jcuken		ucraino-jcuken
menut ukrainian-enhanced	ucraino-migliorato
menut vietnamese-telex		vietnamita-telex
menut vietnamese-viqr		vietnamita-viqr
menut vietnamese-vni		vietnamita-vni
menut ACCENTS			ACCENTI
menut ARABIC			ARABO
menut ARMENIAN-EASTERN		ARMENO-ORIENTALE
menut ARMENIAN-WESTERN		ARMENO-OCCIDENTALE
menut BELARUSIAN-JCUKEN		BIELORUSSO-JCUKEN
menut BULGARIAN-BDS		BULGARO-BDS
menut BULGARIAN-PHONETIC	BULGARO-FONETICO
menut CANFR-WIN			FRANCO-CANADESE-WIN
menut CROATIAN			CROATO
menut CZECH			CECO
menut DVORAK			TASTIERA-DVORAK
menut ESPERANTO			ESPERANTO
menut FRENCH-AZERTY		FRANCESE-AZERTY
menut GEORGIAN-QWERTY		GEORGIANO-QWERTY
menut GERMAN-QWERTZ		TEDESCO-QWERTZ
menut GREEK			GRECO
menut HEBREW			EBRAICO
menut HEBREWP			EBRAICO-FONETICO
menut KANA			KANA
menut KAZAKH-JCUKEN		KAZAKO-JCUKEN
menut KOREAN			COREANO
menut KOREAN-DUBEOLSIK		COREANO-DUBEOLSIK
menut LITHUANIAN-BALTIC		LITUANO-BALTICO
menut MAGYAR			UNGHERESE
menut MONGOLIAN			MONGOLO
menut OLDTURKIC-ORKHON		TURCO-ANTICO-ORKHON
menut OLDTURKIC-YENISEI		TURCO-ANTICO-YENISEI
menut PERSIAN			PERSIANO
menut PERSIAN-IRANIAN		PERSIANO-IRANIANO
menut PINYIN			PINYIN
menut POLISH-SLASH		POLACCO-SLASH
menut RUSSIAN-DVORAK		RUSSO-DVORAK
menut RUSSIAN-JCUKEN		RUSSO-JCUKEN
menut RUSSIAN-JCUKENMAC		RUSSO-JCUKENMAC
menut RUSSIAN-JCUKENWIN		RUSSO-JCUKENWIN
menut RUSSIAN-JCUKENWINTYPE	RUSSO-JCUKENWINTYPE
menut RUSSIAN-TYPOGRAPH		RUSSO-TIPOGRAFICO
menut RUSSIAN-YAWERTY		RUSSO-YAWERTY
menut SERBIAN			SERBO
menut SERBIAN-LATIN		SERBO-LATINO
menut SINHALA			SINGALESE
menut SINHALA-PHONETIC		SINGALESE-PHONETIC
menut SLOVAK			SLOVACCO
menut TAMIL			TAMIL
menut THAANA			THAANA
menut THAANA-PHONETIC		THAANA-FONETICO
menut TURKISH-F			TURCO-F
menut TURKISH-Q			TURCO-Q
menut UKRAINIAN-DVORAK		UCRAINO-DVORAK
menut UKRAINIAN-JCUKEN		UCRAINO-JCUKEN
menut UKRAINIAN-ENHANCED	UCRAINO-MIGLIORATO
menut VIETNAMESE-TELEX		VIETNAMITA-TELEX
menut VIETNAMESE-VIQR		VIETNAMITA-VIQR
menut VIETNAMESE-VNI		VIETNAMITA-VNI

menut Select\ Fo&nt\.\.\.		Scegli\ &Font\.\.\.

" Menù strumenti programmazione
menut &Tools				&Strumenti

menut &Jump\ to\ This\ Tag<Tab>g^]	&Vai\ a\ questa\ tag<Tab>g^]
menut Jump\ &Back<Tab>^T		Torna\ &Indietro<Tab>^T
menut Build\ &Tags\ File		Costruisci\ file\ &Tag\
" Menù ortografia / Spelling
menut &Spelling			&Ortografia

menut &Spell\ Check\ On			Attiva\ &Controllo\ ortografico
menut Spell\ Check\ &Off		&Disattiva\ controllo\ ortografico
menut To\ &Next\ Error<Tab>]s		Errore\ &Seguente<Tab>]s
menut To\ &Previous\ Error<Tab>[s	Errore\ &Precedente<Tab>[s
menut Suggest\ &Corrections<Tab>z=	&Suggerimenti<Tab>z=
menut &Repeat\ Correction<Tab>:spellrepall	&Ripeti\ correzione<Tab>:spellrepall
menut Set\ Language\ to\ "en"		Imposta\ lingua\ a\ "en"
menut Set\ Language\ to\ "en_au"	Imposta\ lingua\ a\ "en_au"
menut Set\ Language\ to\ "en_ca"	Imposta\ lingua\ a\ "en_ca"
menut Set\ Language\ to\ "en_gb"	Imposta\ lingua\ a\ "en_gb"
menut Set\ Language\ to\ "en_nz"	Imposta\ lingua\ a\ "en_nz"
menut Set\ Language\ to\ "en_us"	Imposta\ lingua\ a\ "en_us"
menut &Find\ More\ Languages		&Trova\ altre\ lingue

" Menù piegature / Fold
menut &Folding					&Piegature
" apri e chiudi piegature
menut &Enable/Disable\ Folds<Tab>zi		Pi&egature\ Sì/No<Tab>zi
menut &View\ Cursor\ Line<Tab>zv		&Vedi\ linea\ col\ Cursore<Tab>zv
menut Vie&w\ Cursor\ Line\ Only<Tab>zMzx	Vedi\ &Solo\ linea\ col\ Cursore<Tab>zMzx
menut C&lose\ More\ Folds<Tab>zm		C&Hiudi\ più\ piegature<Tab>zm
menut &Close\ All\ Folds<Tab>zM			&Chiudi\ tutte\ le\ piegature<Tab>zM
menut O&pen\ More\ Folds<Tab>zr			A&Pri\ più\ piegature<Tab>zr
menut &Open\ All\ Folds<Tab>zR			&Apri\ tutte\ le\ piegature<Tab>zR
" metodo piegatura
menut Fold\ Met&hod				Meto&Do\ piegatura
menut M&anual					&Manuale
menut I&ndent					&Nidificazione
menut E&xpression				&Espressione\ Reg\.
menut S&yntax					&Sintassi
menut &Diff					&Differenza
menut Ma&rker					Mar&Catura

" crea e cancella piegature
menut Create\ &Fold<Tab>zf			Crea\ &Piegatura<Tab>zf
menut &Delete\ Fold<Tab>zd			&Togli\ piegatura<Tab>zd
menut Delete\ &All\ Folds<Tab>zD		Togli\ &Tutte\ le\ piegature<Tab>zD
" movimenti all'interno delle piegature
menut Fold\ Col&umn\ Width			Larghezza\ piegat&Ure\ in\ colonne
"menut Fold\ Col&umn\ Width.\ &0\		Larghezza\ piegat&Ure\ in\ colonne.\ &0\
"menut Fold\ Col&umn\ Width.\ &2\		Larghezza\ piegat&Ure\ in\ colonne.\ &2\
"menut Fold\ Col&umn\ Width.\ &3\		Larghezza\ piegat&Ure\ in\ colonne.\ &3\
"menut Fold\ Col&umn\ Width.\ &4\		Larghezza\ piegat&Ure\ in\ colonne.\ &4\
"menut Fold\ Col&umn\ Width.\ &5\		Larghezza\ piegat&Ure\ in\ colonne.\ &5\
"menut Fold\ Col&umn\ Width.\ &6\		Larghezza\ piegat&Ure\ in\ colonne.\ &6\
"menut Fold\ Col&umn\ Width.\ &7\		Larghezza\ piegat&Ure\ in\ colonne.\ &7\
"menut Fold\ Col&umn\ Width.\ &8\		Larghezza\ piegat&Ure\ in\ colonne.\ &8\

menut &Diff					&Differenza
"
menut &Update					&Aggiorna
menut &Get\ Block				&Importa\ differenze
menut &Put\ Block				&Esporta\ differenze

menut &Make<Tab>:make		Esegui\ &Make<Tab>:make

menut &List\ Errors<Tab>:cl	Lista\ &Errori<Tab>:cl
menut L&ist\ Messages<Tab>:cl!	Lista\ &Messaggi<Tab>:cl!
menut &Next\ Error<Tab>:cn	Errore\ s&Uccessivo<Tab>:cn
menut &Previous\ Error<Tab>:cp	Errore\ &Precedente<Tab>:cp
menut &Older\ List<Tab>:cold	Lista\ men&O\ recente<Tab>:cold
menut N&ewer\ List<Tab>:cnew	Lista\ più\ rece&Nte<Tab>:cnew

menut Error\ &Window		&Finestra\ errori

menut &Update<Tab>:cwin		A&Ggiorna<Tab>:cwin
menut &Open<Tab>:copen		&Apri<Tab>:copen
menut &Close<Tab>:cclose	&Chiudi<Tab>:cclose

menut &Convert\ to\ HEX<Tab>:%!xxd	&Converti\ a\ esadecimale<Tab>:%!xxd
menut Conve&rt\ Back<Tab>:%!xxd\ -r	Conve&rti\ da\ esadecimale<Tab>:%!xxd\ -r

menut Se&t\ Compiler		Impo&Sta\ Compilatore
menut Show\ Compiler\ Se&ttings\ in\ Menu	Mostra\ Impos&Tazioni\ Compilatore\ nel\ Menù

" Buffers / Buffer
menut &Buffers			&Buffer

menut &Refresh\ menu		A&Ggiorna\ menù
menut &Delete			&Elimina
menut &Alternate		&Alternato
menut &Next			&Successivo
menut &Previous			&Precedente
"menut [No\ File]		[Nessun\ File]

" Syntax / Sintassi
menut &Syntax				&Sintassi

menut &Show\ File\ Types\ in\ Menu	Mo&Stra\ tipi\ di\ file\ nel\ menù
menut Set\ '&syntax'\ Only		&S\ Attiva\ solo\ \ 'syntax'
menut Set\ '&filetype'\ Too		&F\ Attiva\ anche\ 'filetype'
menut &Off				&Disattiva
menut &Manual				&Manuale
menut A&utomatic			A&Utomatico
menut On/Off\ for\ &This\ File		Attiva\ Sì/No\ su\ ques&To\ file
menut Co&lor\ Test			Test\ &Colori
menut &Highlight\ Test			Test\ &Evidenziamento
menut &Convert\ to\ HTML		Converti\ ad\ &HTML

let g:menutrans_set_lang_to = "Cambia linguaggio a"
let g:menutrans_no_file = "[Senza nome]"
let g:menutrans_spell_change_ARG = 'Cambia\ da\ "%s"\ a'
let g:menutrans_spell_add_ARG_to_word_list = 'Aggiungi\ "%s"\ alla\ Word\ List'
let g:menutrans_spell_ignore_ARG = 'Ignora\ "%s"'

" Window / Finestra
menut &Window				&Finestra

menut &New<Tab>^Wn			&Nuova<Tab>^Wn
menut S&plit<Tab>^Ws			&Dividi\ lo\ schermo<Tab>^Ws
menut Sp&lit\ To\ #<Tab>^W^^		D&Ividi\ verso\ #<Tab>^W^^
menut Split\ &Vertically<Tab>^Wv	Di&Vidi\ verticalmente<Tab>^Wv
menut Split\ File\ E&xplorer		Aggiungi\ finestra\ e&Xplorer
menut &Close<Tab>^Wc			&Chiudi<Tab>^Wc
menut Close\ &Other(s)<Tab>^Wo		C&Hiudi\ altra(e)<Tab>^Wo
menut Move\ &To				&Muovi\ verso

menut &Top<Tab>^WK			&Cima<Tab>^WK
menut &Bottom<Tab>^WJ			&Fondo<Tab>^WJ
menut &Left\ Side<Tab>^WH		Lato\ &Sinistro<Tab>^WH
menut &Right\ Side<Tab>^WL		Lato\ &Destro<Tab>^WL
menut Rotate\ &Up<Tab>^WR		Ruota\ verso\ l'&Alto<Tab>^WR
menut Rotate\ &Down<Tab>^Wr		Ruota\ verso\ il\ &Basso<Tab>^Wr
menut &Equal\ Size<Tab>^W=		&Uguale\ ampiezza<Tab>^W=
menut &Max\ Height<Tab>^W_		A&Ltezza\ massima<Tab>^W_
menut M&in\ Height<Tab>^W1_		Al&Tezza\ minima<Tab>^W1_
menut Max\ &Width<Tab>^W\|		Lar&Ghezza\ massima<Tab>^W\|
menut Min\ Widt&h<Tab>^W1\|		Larg&Hezza\ minima<Tab>^W1\|

" The popup menu
menut &Undo		&Annulla
menut Cu&t		&Taglia
menut &Copy		&Copia
menut &Paste		&Incolla
menut &Delete		&Elimina
menut Select\ Blockwise 	Seleziona\ Blocco
menut Select\ &Word		Seleziona\ &Parola
menut Select\ &Line		Seleziona\ &Riga
menut Select\ &Block 		Seleziona\ &Blocco
menut Select\ &All		Seleziona\ &Tutto
menut Select\ &Sentence		Seleziona\ &Frase
menut Select\ Pa&ragraph	Seleziona\ Para&Grafo

" The GUI Toolbar / Barra Strumenti
"menut Open		Apri
"menut Save		Salva
"menut SaveAll		Salva\ Tutto
"menut Print		Stampa
"menut Undo		Annulla
"menut Redo		Ripristina
"menut Cut		Taglia
"menut Copy		Copia
"menut Paste		Incolla
" -sep3-
"menut Find	Cerca
"menut FindNext	Cerca\ Successivo
"menut FindPrev	Cerca\ Precedente
"menut Replace	Sostituisci
" -sep4-
"menut New		Nuova\ finestra
"menut WinSplit		Dividi\ finestra
"menut WinMax		Massima\ ampiezza
"menut WinMin		Minima\ ampiezza
"menut WinVSplit		Dividi\ verticalmente
"menut WinMaxWidth	Massima\ larghezza
"menut WinMinWidth	Minima\ larghezza
"menut WinClose		Chiudi\ finestra
"menut LoadSesn		Carica\ Sessione
"menut SaveSesn		Salva\ Sessione
"menut RunScript		Esegui\ Script
"menut Make		Make
"menut Shell		Shell
"menut RunCtags		Esegui\ Ctags
"menut TagJump		Vai\ a\ Tag
"menut Help		Aiuto
"menut FindHelp		Cerca\ in\ Aiuto

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: set sw=2 :
