" Menu Translations:	German / Deutsch
" Maintainer:		Johannes Zellner <johannes@zellner.org>
" Originally By:	Marcin Dalecki <dalecki@cs.net.pl>
" Last Change:	Sat, 20 Apr 2002 19:02:42 CEST
" vim:set foldmethod=marker tabstop=8:

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1

" The translations below are in latin1, but they work for cp1252 and
" iso-8859-15 without conversion as well.
if &enc != "cp1252" && &enc != "iso-8859-15"
  scriptencoding latin1
endif

" {{{ FILE / DATEI
menutrans &File				&Datei
menutrans &Open\.\.\.<Tab>:e		&Öffnen\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	In\ geteiltem\ &Fenster\ Öffnen\.\.\.<Tab>:sp
menutrans &New<Tab>:enew		&Neue\ Datei<Tab>:enew
menutrans &Close<Tab>:close		S&chließen<Tab>:close
menutrans &Save<Tab>:w			&Speichern<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	Speichern\ &Als\.\.\.<Tab>:sav
menutrans &Print			&Drucken
menutrans Sa&ve-Exit<Tab>:wqa		Speichern\ und\ Be&enden<Tab>:wqa
menutrans E&xit<Tab>:qa			&Beenden<Tab>:qa

if has("diff")
    menutrans Split\ &Diff\ with\.\.\.	D&ifferenz\ in\ geteiltem\ Fenster\ mit\.\.\.
    menutrans Split\ Patched\ &By\.\.\.	&Patch\ in\ geteiltem\ Fenster\ mit\.\.\.
endif
" }}} FILE / DATEI

" {{{ EDIT / EDITIEREN
menutrans &Edit				&Editieren
menutrans &Undo<Tab>u			Z&urück<Tab>u
menutrans &Redo<Tab>^R			Vo&r<Tab>^R
menutrans Rep&eat<Tab>\.		&Wiederholen<Tab>\.
menutrans Cu&t<Tab>"+x			&Ausschneiden<Tab>"+x
menutrans &Copy<Tab>"+y			&Kopieren<Tab>"+y
menutrans &Paste<Tab>"+gP		Ein&fügen<Tab>"+gP
menutrans Put\ &Before<Tab>[p		Da&vor\ Einfügen<Tab>[p
menutrans Put\ &After<Tab>]p		Da&nach\ Einfügen<Tab>]p
menutrans &Select\ all<Tab>ggVG		Alles\ &Markieren<Tab>ggVG
menutrans &Find\.\.\.			&Suchen\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.	Suchen\ und\ &Ersetzen\.\.\.

" [-- SETTINGS --]
" XXX &E would conflict with 'Suchen\ und\ &Ersetzen', see above
menutrans Settings\ &Window				E&instellungen\.\.\.
menutrans &Global\ Settings				&Globale\ Einstellungen

menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!	&Hervorhebungen\ ein-\ und\ ausschalten<Tab>:set\ hls!
menutrans Toggle\ &Ignore-case<Tab>:set\ ic!		Großschreibung\ &ignorieren\ oder\ benutzen<Tab>:set\ ic!
menutrans Toggle\ &Showmatch<Tab>:set\ sm!		Anzeige\ des\ passenden\ &Symbols\ ein-\ und\ ausschalten<Tab>:set\ sm!

menutrans &Context\ lines				&Zusammenhang

menutrans &Virtual\ Edit				&Virtueller\ Editier-Modus
menutrans Never						Nie
menutrans Block\ Selection				Block-Auswahl
menutrans Insert\ mode					Einfüge-Modus
menutrans Block\ and\ Insert				Block-\ und\ Einfüge-Modus
menutrans Always					Immer
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!		Einfüge-&Modus\ ein-\ und\ ausschalten<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatible<Tab>:set\ cp!		Vi-Kompatibilität\ ein-\ und\ ausschalten<Tab>:set\ cp!

menutrans Search\ &Path\.\.\.				Such-&Pfad\.\.\.
menutrans Ta&g\ Files\.\.\.				Ta&g-Dateien\.\.\.

menutrans Toggle\ &Toolbar				Werkzeugleiste\ ein-\ und\ ausschalten
menutrans Toggle\ &Bottom\ Scrollbar			Unteren\ Rollbalken\ ein-\ und\ ausschalten
menutrans Toggle\ &Left\ Scrollbar			Linken\ Rollbalken\ ein-\ und\ ausschalten
menutrans Toggle\ &Right\ Scrollbar			Rechten\ Rollbalken\ ein-\ und\ ausschalten

" Edit/File Settings
menutrans F&ile\ Settings				&Datei-Einstellungen

" Boolean options
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	Anzeige\ der\ Zeilen&nummer\ ein-\ und\ ausschalten<Tab>:set\ nu!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		&List-Modus\ ein-\ und\ ausschalten<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrap<Tab>:set\ wrap!		&Zeilenumbruch\ ein-\ und\ ausschalten<Tab>:set\ wrap!
menutrans Toggle\ W&rap\ at\ word<Tab>:set\ lbr!	Umbruch\ an\ &Wortgrenzen\ ein-\ und\ ausschalten<Tab>:set\ lbr!
menutrans Toggle\ &expand-tab<Tab>:set\ et!		&Erweiterung\ von\ Tabulatoren\ ein-\ und\ ausschalten<Tab>:set\ et!
menutrans Toggle\ &auto-indent<Tab>:set\ ai!		&Automatische\ Einrückung\ ein-\ und\ ausschalten<Tab>:set\ ai!
menutrans Toggle\ &C-indenting<Tab>:set\ cin!		&C-Einrückung\ ein-\ und\ ausschalten<Tab>:set\ cin!

" other options
menutrans &Shiftwidth					&Schiebeweite
menutrans Soft\ &Tabstop				&Tabulator
menutrans Te&xt\ Width\.\.\.				Te&xt\ Breite\.\.\.
menutrans &File\ Format\.\.\.				&Datei\ Format\.\.\.
menutrans C&olor\ Scheme				F&arbschema\.\.\.
menutrans &Keymap					&Tastatur-Belegung
" }}} EDIT / EDITIEREN

" {{{  TOOLS / WERKZEUGE
if has("folding")
  menutrans &Folding					Fa&ltung
  " open close folds
  menutrans &Enable/Disable\ folds<Tab>zi		&Ein-\ und\ ausschalten<Tab>zi
  menutrans &View\ Cursor\ Line<Tab>zv			Momentane\ &Position\ anzeigen<Tab>zv
  menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx		&Ausschließlich\ momentane\ Position\ anzeigen<Tab>zMzx
  menutrans C&lose\ more\ folds<Tab>zm			Faltungen\ &schließen<Tab>zm
  menutrans &Close\ all\ folds<Tab>zM			Alle\ Faltungen\ schließen<Tab>zM
  menutrans O&pen\ more\ folds<Tab>zr			Faltungen\ &öffnen<Tab>zr
  menutrans &Open\ all\ folds<Tab>zR			Alle\ Faltungen\ öffnen<Tab>zR
  " fold method
  menutrans Fold\ Met&hod				Faltungs-&Methode
  menutrans M&anual					&Manuell
  menutrans I&ndent					&Einrückungen
  menutrans E&xpression					&Ausdruck
  menutrans S&yntax					&Syntax
  menutrans &Diff					&Differenz
  menutrans Ma&rker					Ma&rkierungen
  " create and delete folds
  " TODO accelerators
  menutrans Create\ &Fold<Tab>zf			Faltung\ Erzeugen<Tab>zf
  menutrans &Delete\ Fold<Tab>zd			Faltung\ Löschen<Tab>zd
  menutrans Delete\ &All\ Folds<Tab>zD			Alle\ Faltungen\ Löschen<Tab>zD
  " moving around in folds
  menutrans Fold\ column\ &width			&Breite\ der\ Faltungs-Spalte
endif  " has folding

if has("diff")
  menutrans &Diff					&Differenz
  menutrans &Update					&Aktualisieren
  menutrans &Get\ Block					Block\ &Einfügen
  menutrans &Put\ Block					Block\ &Übertragen
endif

menutrans &Tools					&Werkzeuge
menutrans &Jump\ to\ this\ tag<Tab>g^]			&Springe\ zum\ Tag<Tab>g^]
menutrans Jump\ &back<Tab>^T				Springe\ &Zurück<Tab>^T
menutrans Build\ &Tags\ File				Erstelle\ &Tags\ Datei
menutrans &Make<Tab>:make				&Erstellen<Tab>:make
menutrans &List\ Errors<Tab>:cl				&Fehler\ Anzeigen<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!			&Hinweise\ Anzeigen<Tab>:cl!
menutrans &Next\ Error<Tab>:cn				Zum\ &Nächsten\ Fehler<Tab>:cn
menutrans &Previous\ Error<Tab>:cp			Zum\ &Vorherigen\ Fehler<Tab>:cp
menutrans &Older\ List<Tab>:cold			&Ältere\ Liste<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew			&Neuere\ Liste<Tab>:cnew

menutrans Error\ &Window				Feh&ler-Fenster
menutrans &Set\ Compiler				&Compiler
menutrans &Update<Tab>:cwin				&Aktualisieren<Tab>:cwin
menutrans &Open<Tab>:copen				&Öffnen<Tab>:copen
menutrans &Close<Tab>:cclose				&Schließen<Tab>:cclose

menutrans &Convert\ to\ HEX<Tab>:%!xxd			Nach\ HE&X\ konvertieren<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!xxd\ -r			Zurück\ konvertieren<Tab>:%!xxd\ -r
" }}}  TOOLS / WERKZEUGE

" {{{ SYNTAX / SYNTAX
menutrans &Syntax				&Syntax
menutrans Set\ '&syntax'\ only			Nur\ '&syntax'\ Setzen
menutrans Set\ '&filetype'\ too			Auch\ '&filetype'\ Setzen
menutrans &Off					&Aus
menutrans &Manual				&Manuell
menutrans A&utomatic				A&utomatisch
menutrans on/off\ for\ &This\ file		An/Aus (diese\ &Datei)
menutrans Co&lor\ test				Test\ der\ Farben
menutrans &Highlight\ test			Test\ der\ Un&terstreichungen
menutrans &Convert\ to\ HTML			Konvertieren\ nach\ &HTML
" }}} SYNTAX / SYNTAX

" {{{ BUFFERS / PUFFER
menutrans &Buffers					&Puffer
menutrans &Refresh\ menu				&Aktualisieren
menutrans Delete					Löschen
menutrans &Alternate					&Wechseln
menutrans &Next						&Nächster
menutrans &Previous					&Vorheriger
menutrans [No\ File]					[Keine\ Datei]
" }}} BUFFERS / PUFFER

" {{{ WINDOW / ANSICHT
menutrans &Window			&Ansicht
menutrans &New<Tab>^Wn			&Neu<Tab>^Wn
menutrans S&plit<Tab>^Ws		Aufs&palten<Tab>^Ws
menutrans Split\ &Vertically<Tab>^Wv	&Vertikal\ Aufspalten<Tab>^Ws
menutrans Split\ File\ E&xplorer	Ver&zeichnis
menutrans Sp&lit\ To\ #<Tab>^W^^	Aufspa&lten\ in\ #<Tab>^W^^
menutrans &Close<Tab>^Wc		&Schließen<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo	&Andere\ Schließen<Tab>^Wo
menutrans Ne&xt<Tab>^Ww			N&ächstes<Tab>^Ww
menutrans P&revious<Tab>^WW		Vor&heriges<Tab>^WW
menutrans &Equal\ Size<Tab>^W=		&Gleiche\ Höhen<Tab>^W=
menutrans &Max\ Height<Tab>^W_		&Maximale\ Höhe<Tab>^W_
menutrans M&in\ Height<Tab>^W1_		M&inimale\ Höhe<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|		Maximale\ &Breite<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|		Minimale\ Brei&te<Tab>^W1\|
menutrans Move\ &To			V&erschiebe\ nach
menutrans &Top<Tab>^WK			&Oben<Tab>^WK
menutrans &Bottom<Tab>^WJ		&Unten<Tab>^WJ
menutrans &Left\ side<Tab>^WH		&Links<Tab>^WH
menutrans &Right\ side<Tab>^WL		&Rechts<Tab>^WL
menutrans Rotate\ &Up<Tab>^WR		Rotiere\ nach\ &oben<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		Rotiere\ nach\ &unten<Tab>^Wr
menutrans Select\ Fo&nt\.\.\.		Auswahl\ der\ Schriftart\.\.\.
" }}} WINDOW / ANSICHT

" {{{ HELP / HILFE
menutrans &Help			&Hilfe
menutrans &Overview<Tab><F1>	&Überblick<Tab><F1>
menutrans &User\ Manual		&Handbuch
menutrans &How-to\ links	How-to\ &Index
menutrans &GUI			&Graphische\ Oberfläche
menutrans &Credits		&Autoren
menutrans Co&pying		&Urheberrecht
menutrans O&rphans		&Waisen
menutrans &Find\.\.\.		&Suchen\.\.\.	" conflicts with Edit.Find
menutrans &Version		&Version
menutrans &About		&Titelseite
" }}} HELP / HILFE

" {{{ POPUP
menutrans &Undo				&Zurück
menutrans Cu&t				Aus&schneiden
menutrans &Copy				&Kopieren
menutrans &Paste			&Einfügen
menutrans &Delete			&Löschen
menutrans Select\ Blockwise		Auswahl\ Blockartig
menutrans Select\ &Word			Auswahl\ des\ &Wortes
menutrans Select\ &Line			Auswahl\ der\ &Zeile
menutrans Select\ &Block		Auswahl\ des\ &Blocks
menutrans Select\ &All			&Alles\ Auswählen
" }}} POPUP

" {{{ TOOLBAR
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		Datei Öffnen
    tmenu ToolBar.Save		Datei Speichern
    tmenu ToolBar.SaveAll	Alle Dateien Speichern
    tmenu ToolBar.Print		Drucken
    tmenu ToolBar.Undo		Zurück
    tmenu ToolBar.Redo		Wiederholen
    tmenu ToolBar.Cut		Ausschneiden
    tmenu ToolBar.Copy		Kopieren
    tmenu ToolBar.Paste		Einfügen
    tmenu ToolBar.Find		Suchen...
    tmenu ToolBar.FindNext	Suche Nächsten
    tmenu ToolBar.FindPrev	Suche Vorherigen
    tmenu ToolBar.Replace	Suchen und Ersetzen...
    if 0	" disabled; These are in the Windows menu
      tmenu ToolBar.New		Neue Ansicht
      tmenu ToolBar.WinSplit	Ansicht Aufspalten
      tmenu ToolBar.WinMax	Ansicht Maximale Höhen
      tmenu ToolBar.WinMin	Ansicht Minimale Höhen
      tmenu ToolBar.WinClose	Ansicht Schließen
    endif
    tmenu ToolBar.LoadSesn	Sitzung Laden
    tmenu ToolBar.SaveSesn	Sitzung Speichern
    tmenu ToolBar.RunScript	Vim-Skript Ausführen
    tmenu ToolBar.Make		Erstellen
    tmenu ToolBar.Shell		Shell Starten
    tmenu ToolBar.RunCtags	Erstelle Tags Datei
    tmenu ToolBar.TagJump	Springe zum Tag
    tmenu ToolBar.Help		Hilfe!
    tmenu ToolBar.FindHelp	Hilfe Durchsuchen...
  endfun
endif
" }}} TOOLBAR

" {{{ DIALOG TEXTS
let g:menutrans_no_file = "[Keine Datei]"
let g:menutrans_help_dialog = "Geben Sie einen Befehl oder ein Wort ein, für das Sie Hilfe benötigen:\n\nVerwenden Sie i_ für Eingabe ('input') Befehle (z.B.: i_CTRL-X)\nVerwenden Sie c_ für Befehls-Zeilen ('command-line') Befehle (z.B.: c_<Del>)\nVerwenden Sie ' für Options-Namen (z.B.: 'shiftwidth')"
let g:menutrans_path_dialog = "Geben Sie Such-Pfade für Dateien ein.\nTrennen Sie die Verzeichnis-Namen durch Kommata."
let g:menutrans_tags_dialog = "Geben Sie die Namen der 'tag'-Dateien ein.\nTrennen Sie die Namen durch Kommata."
let g:menutrans_textwidth_dialog = "Geben Sie eine neue Text-Breite ein (oder 0, um die Formatierung abzuschalten)"
let g:menutrans_fileformat_dialog = "Wählen Sie ein Datei-Format aus"
" }}}
