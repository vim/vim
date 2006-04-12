" Menu Translations:	Italian / Italiano
" Maintainer:		Antonio Colombo <azc100@gmail.com>
"			Vlad Sandrini <vlad.gently@gmail.com>
" Last Change:	2006 Apr 03

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1

scriptencoding iso-8859-1

" Help / Aiuto
menut &Help			&Aiuto
menut &Overview<Tab><F1>	&Panoramica<Tab><F1>
menut &User\ Manual		Manuale\ &Utente
menut &How-to\ links	Co&me\.\.\.
"menut &GUI			&GUI
menut &Credits		Cr&editi
menut Co&pying		C&opie
menut &Sponsor/Register &Sponsor/Registrazione
menut O&rphans		O&rfani
"menut &Find\.\.\.		&Cerca\.\.\.
"NOTA: fa conflitto con 'cerca' nel menu 'modifica'
menut &Version		&Versione
menut &About		&Intro

let g:menutrans_help_dialog = "Batti un comando o una parola per cercare aiuto:\n\nPremetti i_ per comandi in modalità Input (ad.es.: i_CTRL-X)\nPremetti c_ per comandi che editano la linea-comandi (ad.es.: c_<Del>)\nPremetti ' per un nome di opzione (ad.es.: 'shiftwidth')"

" File menu
"menut &File				&File
"
menut &Open\.\.\.<Tab>:e		&Apri\.\.\.<Tab>:e
menut Sp&lit-Open\.\.\.<Tab>:sp	A&pri\ nuova\ Finestra\.\.\.<Tab>:sp
menut Open\ Tab\.\.\.<Tab>:tabnew Apri\ nuova\ &Linguetta\.\.\.<Tab>:tabnew
menut &New<Tab>:enew		&Nuovo<Tab>:enew
menut &Close<Tab>:close		&Chiudi<Tab>:close
menut &Save<Tab>:w			&Salva<Tab>:w
menut Save\ &As\.\.\.<Tab>:sav	Salva\ &con\ nome\.\.\.<Tab>:sav

if has("diff")
    menut Split\ &Diff\ with\.\.\.	Finestra\ &Differenza\ con\.\.\.
    menut Split\ Patched\ &By\.\.\.	Finestra\ Patc&h\ da\.\.\.
endif

menut &Print			S&tampa
menut Sa&ve-Exit<Tab>:wqa		Sa&lva\ ed\ esci<Tab>:wqa
menut E&xit<Tab>:qa			&Esci<Tab>:qa

" Edit / Modifica

menut &Edit				&Modifica
menut &Undo<Tab>u			&Annulla<Tab>u
menut &Redo<Tab>^R			&Ripristina<Tab>^R
menut Rep&eat<Tab>\.		Ri&peti<Tab>\.
menut Cu&t<Tab>"+x			&Taglia<Tab>"+x
menut &Copy<Tab>"+y			&Copia<Tab>"+y
menut &Paste<Tab>"+gP		&Incolla<Tab>"+gP
menut Put\ &Before<Tab>[p		&Metti\ davanti<Tab>[p
menut Put\ &After<Tab>]p		M&etti\ dietro<Tab>]p
menut &Delete<Tab>x			Cance&lla<Tab>x
menut &Select\ all<Tab>ggVG		Seleziona\ &tutto<Tab>ggVG
menut &Select\ All<Tab>ggVG		Seleziona\ &tutto<Tab>ggVG
menut &Find\.\.\.			&Cerca\.\.\.
menut Find\ and\ Rep&lace\.\.\.	&Sostituisci\.\.\.
menut Settings\ &Window		&Finestra\ Impostazioni
menut Startup\ &Settings	Impostazioni\ di\ &Avvio
menut &Global\ Settings		Impostazioni\ &Globali
menut Question			Domanda

" Edit / Modifica Impostazioni Globali
menut &Global\ Settings	Impostazioni\ &Globali
menut Toggle\ Pattern\ &Highlight<Tab>:set\ hls!	&Evidenzia\ ricerche\ Sì/No<Tab>:set\ hls!
menut Toggle\ &Ignore-case<Tab>:set\ ic!		&Ignora\ maiusc\.-minusc\.\ Sì/No<Tab>:set\ ic!
menut Toggle\ &Showmatch<Tab>:set\ sm!	Indica\ &Corrispondente\ Sì/No<Tab>:set\ sm!

menut &Context\ lines	&Linee\ di\ contesto
menut &Virtual\ Edit		&Edit\ Virtuale

menut Never		Mai
menut Block\ Selection		Selezione\ Blocco
menut Insert\ mode	Modalità\ Inserimento
menut Block\ and\ Insert	Selezione\ Blocco+Inserimento
menut Always		Sempre

menut Toggle\ Insert\ &Mode<Tab>:set\ im!	&Modalità\ Inserimento\ Sì/No<Tab>:set\ im!
menut Toggle\ Vi\ C&ompatible<Tab>:set\ cp!	C&ompatibile\ Vi\ Sì/No<Tab>:set\ cp!
menut Search\ &Path\.\.\.	&Percorso\ di\ ricerca\.\.\.
menut Ta&g\ Files\.\.\.		File\ Ta&g\.\.\.
"
" Opzioni GUI
menut Toggle\ &Toolbar	Barra\ S&trumenti\ Sì/No
menut Toggle\ &Bottom\ Scrollbar	Barra\ Scorrimento\ in\ &fondo\ Sì/No
menut Toggle\ &Left\ Scrollbar	Barra\ Scorrimento\ a\ &sinistra\ Sì/No
menut Toggle\ &Right\ Scrollbar	Barra\ Scorrimento\ a\ &destra\ Sì/No

let g:menutrans_path_dialog = "Batti percorso di ricerca per i file.\nSepara fra loro i nomi di directory con una virgola."
let g:menutrans_tags_dialog = "Batti nome dei file di tag.\nSepara fra loro i nomi di directory con una virgola."

" Edit / Impostazioni File
menut F&ile\ Settings	&Impostazioni\ File

" Boolean options
menut Toggle\ Line\ &Numbering<Tab>:set\ nu!	&Numerazione\ \ Sì/No<Tab>:set\ nu!
menut Toggle\ &List\ Mode<Tab>:set\ list!		Modalità\ &Lista\ Sì/No<Tab>:set\ list!
menut Toggle\ Line\ &Wrap<Tab>:set\ wrap!		Linee\ &Continuate\ Sì/No<Tab>:set\ wrap!
menut Toggle\ W&rap\ at\ word<Tab>:set\ lbr!	A\ capo\ alla\ &parola\ Sì/No<Tab>:set\ lbr!
menut Toggle\ &expand-tab<Tab>:set\ et!		&Espandi\ Tabulazione\ Sì/No<Tab>:set\ et!
menut Toggle\ &auto-indent<Tab>:set\ ai!	Indentazione\ &automatica\ Sì/No<Tab>:set ai!
menut Toggle\ &C-indenting<Tab>:set\ cin!	Indentazione\ stile\ &C\ Sì/No<Tab>:set cin!

" altre opzioni
menut &Shiftwidth			&Spazi\ Rientranza
menut Soft\ &Tabstop		&Tabulazione\ Software
menut Te&xt\ Width\.\.\.		Lunghe&zza\ Riga\.\.\.
menut &File\ Format\.\.\.	Formato\ &File\.\.\.

let g:menutrans_textwidth_dialog = "Batti nuova lunghezza linea (0 per inibire la formattazione): "

let g:menutrans_fileformat_dialog = "Scegli formato con cui scrivere il file"

menut C&olor\ Scheme		Schema\ C&olori

menut default		normale
menut DEFAULT		NORMALE
menut evening		notturno
menut EVENING		NOTTURNO
menut morning		diurno
menut MORNING		DIURNO
menut shine		brillante
menut SHINE		BRILLANTE
menut peachpuff		pesca
menut PEACHPUF		PESCA

menut &Keymap				&Mappa\ tastiera

menut None		nessuna
menut accents		accenti
menut ACCENTS		ACCENTI
menut hebrew		ebraico
menut HEBREW		EBRAICO
menut hebrew_iso-8859-8 ebraico_iso-8859-8
menut hebrew_cp1255	ebraico_cp1255
menut hebrew_utf-8	ebraico_utf-8
menut hebrewp_iso-8859-8 ebraico_p_iso-8859-8
menut HEBREW-		EBRAICO+
menut hebrewp		EBRAICOP
menut HEBREWP		EBRAICOP
menut russian-jcuken	cirillico-jcuken
menut russian-jcukenwin	cirillico-jcuken-win
menut RUSSIAN		CIRILLICO
menut RUSSIAN-		CIRILLICO-

menut Select\ Fo&nt\.\.\.		Scegli\ &Font\.\.\.

" Menu strumenti programmazione
menut &Tools			&Strumenti

menut &Jump\ to\ this\ tag<Tab>g^]	&Vai\ a\ questa\ Tag<Tab>g^]
menut Jump\ &back<Tab>^T		Torna\ &indietro<Tab>^T
menut Build\ &Tags\ File		Costruisci\ File\ &Tags\

" Menu ortografia / Spelling
menut &Spelling			&Ortografia

menut &Spell\ Check\ On			Attiva\ &Controllo\ ortografico
menut Spell\ Check\ &Off		&Disattiva\ controllo\ ortografico
menut To\ &Next\ error<Tab>]s		Errore\ &Seguente<tab>]s
menut To\ &Previous\ error<Tab>[s	Errore\ &Precedente<tab>[s
menut Suggest\ &Corrections<Tab>z=	&Suggerimenti<Tab>z=
menut &Repeat\ correction<Tab>:spellrepall	&Ripeti\ correzione<Tab>:spellrepall
menut Set\ language\ to\ "en"		Imposta\ lingua\ a\ "en"
menut Set\ language\ to\ "en_au"	Imposta\ lingua\ a\ "en_au"
menut Set\ language\ to\ "en_ca"	Imposta\ lingua\ a\ "en_ca"
menut Set\ language\ to\ "en_gb"	Imposta\ lingua\ a\ "en_gb"
menut Set\ language\ to\ "en_nz"	Imposta\ lingua\ a\ "en_nz"
menut Set\ language\ to\ "en_us"	Imposta\ lingua\ a\ "en_us"
menut Set\ language\ to\ "it"		Imposta\ lingua\ a\ "it"
menut Set\ language\ to\ "it_it"	Imposta\ lingua\ a\ "it_it"
menut Set\ language\ to\ "it_ch"	Imposta\ lingua\ a\ "it_ch"
menut &Find\ More\ Languages		&Trova\ altre\ lingue

" Menu piegature / Fold
if has("folding")
  menut &Folding					&Piegature
  " apri e chiudi piegature
  menut &Enable/Disable\ folds<Tab>zi		Pi&egature\ Sì/No<Tab>zi
  menut &View\ Cursor\ Line<Tab>zv			&Vedi\ linea\ col\ Cursore<Tab>zv
  menut Vie&w\ Cursor\ Line\ only<Tab>zMzx		Vedi\ &solo\ linea\ col\ Cursore<Tab>zMzx
  menut C&lose\ more\ folds<Tab>zm			C&hiudi\ più\ Piegature<Tab>zm
  menut &Close\ all\ folds<Tab>zM			&Chiudi\ tutte\ le\ Piegature<Tab>zM
  menut O&pen\ more\ folds<Tab>zr			A&pri\ più\ Piegature<Tab>zr
  menut &Open\ all\ folds<Tab>zR			&Apri\ tutte\ le\ Piegature<Tab>zR
  " metodo piegatura
  menut Fold\ Met&hod				Meto&do\ Piegatura
  menut M&anual					&Manuale
  menut I&ndent					&Nidificazione
  menut E&xpression					&Espressione\ Reg\.
  menut S&yntax					&Sintassi
  menut &Diff					&Differenza
  menut Ma&rker					Mar&catura
  " crea e cancella piegature
  menut Create\ &Fold<Tab>zf			Crea\ &Piegatura<Tab>zf
  menut &Delete\ Fold<Tab>zd			&Leva\ Piegatura<Tab>zd
  menut Delete\ &All\ Folds<Tab>zD			Leva\ &tutte\ le\ Piegature<Tab>zD
  " movimenti all'interno delle piegature
  menut Fold\ col&umn\ width			Larghezza\ piegat&ure\ in\ colonne
endif  " has folding

if has("diff")
  menut &Diff					&Differenza
  "
  menut &Update					&Aggiorna
  menut &Get\ Block					&Importa\ Differenze
  menut &Put\ Block					&Esporta\ Differenze
endif  " has diff

menut &Make<Tab>:make		Esegui\ &Make<Tab>:make

menut &List\ Errors<Tab>:cl		Lista\ &Errori<Tab>:cl
menut L&ist\ Messages<Tab>:cl!	Lista\ &Messaggi<Tab>:cl!
menut &Next\ Error<Tab>:cn		Errore\ S&uccessivo<Tab>:cn
menut &Previous\ Error<Tab>:cp	Errore\ &Precedente<Tab>:cp
menut &Older\ List<Tab>:cold	Lista\ men&o\ recente<Tab>:cold
menut N&ewer\ List<Tab>:cnew	Lista\ più\ rece&nte<Tab>:cnew

menut Error\ &Window		&Finestra\ errori

menut &Update<Tab>:cwin		A&ggiorna<Tab>:cwin
menut &Open<Tab>:copen		&Apri<Tab>:copen
menut &Close<Tab>:cclose	&Chiudi<Tab>:cclose

menut &Convert\ to\ HEX<Tab>:%!xxd	&Converti\ a\ Esadecimale<Tab>:%!xxd
menut Conve&rt\ back<Tab>:%!xxd\ -r	Conve&rti\ da\ Esadecimale<Tab>:%!xxd\ -r

menut &SeT\ Compiler		Impo&sta\ Compilatore

" Buffers / Buffer
menut &Buffers		&Buffer

menut &Refresh\ menu				A&ggiorna\ menu
menut &Delete		&Elimina
menut &Alternate		&Alternato
menut &Next			&Successivo
menut &Previous		&Precedente
menut [No\ File]		[Nessun\ File]
" Syntax / Sintassi
menut &Syntax		&Sintassi
menut &Show\ filetypes\ in\ menu	Mo&stra\ tipi\ di\ file\ nel\ menu
menut Set\ '&syntax'\ only	Attiva\ solo\ \ '&syntax'
menut Set\ '&filetype'\ too	Attiva\ anche\ '&filetype'
menut &Off			&Disattiva
menut &Manual		&Manuale
menut A&utomatic		A&utomatico
menut on/off\ for\ &This\ file	Attiva\ Sì/No\ su\ ques&to\ file
menut Co&lor\ test		Test\ &Colori
menut &Highlight\ test	Test\ &Evidenziamento
menut &Convert\ to\ HTML	Converti\ ad\ &HTML

let g:menutrans_no_file = "[Senza nome]"

" Window / Finestra
menut &Window			&Finestra

menut &New<Tab>^Wn			&Nuova<Tab>^Wn
menut S&plit<Tab>^Ws		&Dividi\ lo\ schermo<Tab>^Ws
menut Sp&lit\ To\ #<Tab>^W^^	D&ividi\ verso\ #<Tab>^W^^
menut Split\ &Vertically<Tab>^Wv	Di&vidi\ verticalmente<Tab>^Wv
menut Split\ File\ E&xplorer	Aggiungi\ finestra\ E&xplorer

menut &Close<Tab>^Wc		&Chiudi<Tab>^Wc
menut Close\ &Other(s)<Tab>^Wo	C&hiudi\ altra(e)<Tab>^Wo

menut Move\ &To			&Muovi\ verso

menut &Top<Tab>^WK			&Cima<Tab>^WK
menut &Bottom<Tab>^WJ		&Fondo<Tab>^WJ
menut &Left\ side<Tab>^WH		Lato\ &Sinistro<Tab>^WH
menut &Right\ side<Tab>^WL		Lato\ &Destro<Tab>^WL
menut Rotate\ &Up<Tab>^WR		Ruota\ verso\ l'&alto<Tab>^WR
menut Rotate\ &Down<Tab>^Wr		Ruota\ verso\ il\ &basso<Tab>^Wr

menut &Equal\ Size<Tab>^W=		&Uguale\ ampiezza<Tab>^W=
menut &Max\ Height<Tab>^W_		&Altezza\ massima<Tab>^W_
menut M&in\ Height<Tab>^W1_		A&ltezza\ minima<Tab>^W1_
menut Max\ &Width<Tab>^W\|		Larghezza\ massima<Tab>^W\|
menut Min\ Widt&h<Tab>^W1\|		Larghezza\ minima<Tab>^W1\|

" The popup menu
menut &Undo			&Annulla
menut Cu&t			&Taglia
menut &Copy			&Copia
menut &Paste		&Incolla
menut &Delete		&Elimina

menut Select\ Blockwise	Seleziona\ in\ blocco
menut Select\ &Word		Seleziona\ &Parola
menut Select\ &Line		Seleziona\ &Linea
menut Select\ &Block	Seleziona\ &Blocco
menut Select\ &All		Seleziona\ T&utto

" The GUI Toolbar / Barra Strumenti
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		Apri
    tmenu ToolBar.Save		Salva
    tmenu ToolBar.SaveAll	Salva Tutto
    tmenu ToolBar.Print		Stampa
    tmenu ToolBar.Undo		Annulla
    tmenu ToolBar.Redo		Ripristina
    tmenu ToolBar.Cut		Taglia
    tmenu ToolBar.Copy		Copia
    tmenu ToolBar.Paste		Incolla

    if !has("gui_athena")
      tmenu ToolBar.Find	Cerca
      tmenu ToolBar.FindNext	Cerca Successivo
      tmenu ToolBar.FindPrev	Cerca Precedente
      tmenu ToolBar.Replace	Sostituisci
    endif

if 0	" disabled; These are in the Windows menu
      tmenu ToolBar.New		Nuova finestra
      tmenu ToolBar.WinSplit	Dividi finestra
      tmenu ToolBar.WinMax	Massima ampiezza
      tmenu ToolBar.WinMin	Minima ampiezza
      tmenu ToolBar.WinVSplit	Dividi verticalmente
      tmenu ToolBar.WinMaxWidth	Massima larghezza
      tmenu ToolBar.WinMinWidth	Minima larghezza
      tmenu ToolBar.WinClose	Chiudi finestra
endif

    tmenu ToolBar.LoadSesn	Carica Sessione
    tmenu ToolBar.SaveSesn	Salva Sessione
    tmenu ToolBar.RunScript	Esegui Script
    tmenu ToolBar.Make		Make
    tmenu ToolBar.Shell		Shell
    tmenu ToolBar.RunCtags	Esegui Ctags
    tmenu ToolBar.TagJump	Vai a Tag
    tmenu ToolBar.Help		Aiuto
    tmenu ToolBar.FindHelp	Cerca in Aiuto
  endfun
endif

" vim: set sw=2 :
