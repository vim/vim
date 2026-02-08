" Menu Translations:    Swedish
" Maintainer:		Johan Svedberg <johan@svedberg.com>
" Last Change:		2020 Apr 22
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
menutrans &Help			&Hjälp
menutrans &Overview<Tab><F1>	Öv&ersikt<Tab><F1>
menutrans &User\ Manual		&Användarmanual
menutrans &How-to\ links	&Hur-göra-länkar
menutrans &Find\.\.\.		&Sök\.\.\.
menutrans &Credits		&Tack\ till
menutrans Co&pying		&Kopiering
menutrans &Sponsor/Register	&Sponsra/Registrera
menutrans O&rphans		&Föräldralösa\ barn
menutrans &Version		&Version
menutrans &About		&Om

" File menu

menutrans &File				&Arkiv
menutrans &Open\.\.\.<Tab>:e		Ö&ppna\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	Öppna\ i\ &delad\ vy\.\.\.<Tab>:sp
menutrans Open\ &Tab\.\.\.<Tab>:tabnew	Öppna\ &flik\.\.\.<Tab>:tabnew
menutrans &New<Tab>:enew		&Ny<Tab>:enew
menutrans &Close<Tab>:close		S&täng<Tab>:close
menutrans &Save<Tab>:w			&Spara<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	Spara\ so&m\.\.\.<Tab>:sav
menutrans Split\ &Diff\ with\.\.\.	Dela\ &diff\ med\.\.\.
menutrans Split\ Patched\ &By\.\.\.	Dela\ &lappad\ med\.\.\.
menutrans &Print			Skriv\ &ut
menutrans Sa&ve-Exit<Tab>:wqa		Spara\ &och\ avsluta<Tab>:wqa
menutrans E&xit<Tab>:qa			A&vsluta<Tab>:qa

" Edit menu
menutrans &Edit				R&edigera
menutrans &Undo<Tab>u			Å&ngra<Tab>u
menutrans &Redo<Tab>^R			&Gör\ om<Tab>^R
menutrans Rep&eat<Tab>\.		Upp&repa<Tab>\.
menutrans Cu&t<Tab>"+x			Klipp\ &ut<Tab>"+x
menutrans &Copy<Tab>"+y			&Kopiera<Tab>"+y
menutrans &Paste<Tab>"+gP		Klistra\ &in<Tab>"+gP
menutrans Put\ &Before<Tab>[p		Infoga\ &före<Tab>[p
menutrans Put\ &After<Tab>]p		Infoga\ &efter<Tab>]p
menutrans &Select\ All<Tab>ggVG		&Markera\ allt<Tab>ggVG
menutrans &Find\.\.\.			&Sök\.\.\.
menutrans &Find<Tab>/			&Sök<Tab>/
menutrans Find\ and\ Rep&lace\.\.\.	Sök\ och\ e&rsätt\.\.\.
menutrans Find\ and\ Rep&lace<Tab>:%s	Sök\ och\ e&rsätt<Tab>:%s
menutrans Find\ and\ Rep&lace		Sök\ och\ e&rsätt
menutrans Find\ and\ Rep&lace<Tab>:s	Sök\ och\ e&rsätt<Tab>:s
menutrans Settings\ &Window		In&ställningar
menutrans &Global\ Settings		Gl&obala\ inställningar
menutrans F&ile\ Settings		Fi&linställningar
menutrans C&olor\ Scheme		Färgs&chema
menutrans Show\ C&olor\ Schemes\ in\ Menu    Visa\ fär&gscheman\ i\ meny
menutrans &Keymap			&Tangentuppsättning
menutrans Show\ &Keymaps\ in\ Menu      Visa\ &tangentuppsättningar\ i\ meny
menutrans Startup\ &Settings		U&ppstartsinställningar

" Edit.Global Settings
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!	Växla\ &mönsterframhävning<Tab>:set\ hls!
menutrans Toggle\ &Ignoring\ Case<Tab>:set\ ic!		Växla\ &ignorering\ av\ skiftläge<Tab>:set\ ic!
menutrans Toggle\ &Showing\ Matched\ Pairs<Tab>:set\ sm!  Växla\ visning\ av\ &matchande\ par<Tab>:set\ sm!
menutrans &Context\ lines				Sa&mmanhangsrader
menutrans &Virtual\ Edit				&Virtuell\ redigering
menutrans Never						Aldrig
menutrans Block\ Selection				Blockval
menutrans Insert\ mode					Infogningsläge
menutrans Block\ and\ Insert				Block\ och\ infogning
menutrans Always					Alltid
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!		Växla\ &infogningsläge<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatibility<Tab>:set\ cp!		Växla\ Vi-k&ompabilitet<Tab>:set\ cp!
menutrans Search\ &Path\.\.\.				Sö&kväg\.\.\.
menutrans Ta&g\ Files\.\.\.				Tag&gfiler\.\.\.
menutrans Toggle\ &Toolbar				Växla\ v&erktygsrad
menutrans Toggle\ &Bottom\ Scrollbar			Vä&xla\ rullningslista\ i\ botten
menutrans Toggle\ &Left\ Scrollbar			Växla\ &vänster\ rullningslista
menutrans Toggle\ &Right\ Scrollbar			Växla\ &höger\ rullningslista
menutrans None						Ingen

" Edit.File Settings
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	Växla\ rad&numrering<Tab>:set\ nu!
menutrans Toggle\ Relati&ve\ Line\ Numbering<Tab>:set\ rnu!  Växla\ &relativ\ radnumrering<Tab>:set\ rnu!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		Växla\ &listläge<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrapping<Tab>:set\ wrap!		Växla\ radbr&ytning<Tab>:set\ wrap!
menutrans Toggle\ W&rapping\ at\ word<Tab>:set\ lbr!	Växla\ radbry&tning\ vid\ ord<tab>:set\ lbr!
menutrans Toggle\ Tab\ &Expanding<Tab>:set\ et!		Växla\ tab-e&xpandering<Tab>:set\ et!
menutrans Toggle\ &Auto\ Indenting<Tab>:set\ ai!		Växla\ &automatisk\ indentering<Tab>:set\ ai!
menutrans Toggle\ &C-Style\ Indenting<Tab>:set\ cin!		Växla\ &C-indentering<Tab>:set\ cin!
menutrans &Shiftwidth					&Shiftbredd
menutrans Soft\ &Tabstop				Mjuk\ &tab-stopp
menutrans Te&xt\ Width\.\.\.				Te&xtbredd\.\.\.
menutrans &File\ Format\.\.\.				&Filformat\.\.\.

" Tools menu
menutrans &Tools			Ver&ktyg
menutrans &Jump\ to\ this\ tag<Tab>g^]	&Hoppa\ till\ den\ här\ taggen<Tab>g^]
menutrans Jump\ &back<Tab>^T		Hoppa\ &tillbaka<Tab>^T
menutrans Build\ &Tags\ File		&Bygg\ taggfil
menutrans &Make<Tab>:make		&Bygg<Tab>:make
menutrans &List\ Errors<Tab>:cl		L&istfel<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!	&Listmeddelande<Tab>:cl!
menutrans &Next\ Error<Tab>:cn		&Nästa\ fel<Tab>:cn
menutrans &Previous\ Error<Tab>:cp	Före&gående\ fel<Tab>:cp
menutrans &Older\ List<Tab>:cold	Ä&ldre\ lista<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew	&Nyare\ lista<Tab>:cnew
menutrans Error\ &Window		Felfö&nster
menutrans &Update<Tab>:cwin		&Uppdatera<Tab>:cwin
menutrans &Open<Tab>:copen		Ö&ppna<Tab>:copen
menutrans &Close<Tab>:cclose		&Stäng<Tab>:cclose
menutrans &Convert\ to\ HEX<Tab>:%!xxd	&Konvertera\ till\ HEX<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!xxd\ -r	Konv&ertera\ tillbaka<Tab>:%!xxd\ -r
menutrans Se&T\ Compiler		Ställ\ in\ &kompilerare

" Tools.Spelling
menutrans &Spelling				&Stavning
menutrans &Spell\ Check\ On			&Stavningskontroll\ på
menutrans Spell\ Check\ &Off			Stavningskontroll\ a&v
menutrans To\ &Next\ error<Tab>]s		Till\ &nästa\ fel<Tab>]s
menutrans To\ &Previous\ error<Tab>[s		Till\ &föregående\ fel<Tab>[s
menutrans Suggest\ &Corrections<Tab>z=		Föreslå\ &korrigeringar<Tab>z=
menutrans &Repeat\ correction<Tab>:spellrepall	&Upprepa\ korrigering<Tab>:spellrepall

" Tools.Folding
menutrans &Enable/Disable\ folds<Tab>zi	Växla\ ve&ck<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv	Visa\ ma&rkörrad<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx	Vi&sa\ bara\ markörrad<Tab>zMzx
menutrans C&lose\ more\ folds<Tab>zm	Stäng\ f&ler\ veck<Tab>zm
menutrans &Close\ all\ folds<Tab>zM	S&täng\ alla\ veck<Tab>zM
menutrans O&pen\ more\ folds<Tab>zr	Ö&pp&na\ mer\ veck<Tab>zr
menutrans &Open\ all\ folds<Tab>zR	Öppna\ alla\ veck<Tab>zR
menutrans Fold\ Met&hod			Veckmet&od
menutrans M&anual			M&anuell
menutrans I&ndent			Indentering
menutrans E&xpression			&Uttryck
menutrans S&yntax			S&yntax
menutrans &Folding			Vi&kning
menutrans &Diff				&Diff
menutrans Ma&rker			Mar&kering
menutrans Create\ &Fold<Tab>zf		Skapa\ &veck<Tab>zf
menutrans &Delete\ Fold<Tab>zd		&Ta\ bort\ veck<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD	Ta\ bort\ &alla\ veck<Tab>zD
menutrans Fold\ col&umn\ width		Veckkol&umnsbredd

" Tools.Diff
menutrans &Update		&Uppdatera
menutrans &Get\ Block		&Hämta\ block
menutrans &Put\ Block		&Lämna\ block

" Names for buffer menu.
menutrans &Buffers		&Buffertar
menutrans &Refresh\ menu	&Uppdatera\ meny
menutrans &Delete		Ta\ &bort
menutrans &Alternate		&Alternativ
menutrans &Next			&Nästa
menutrans &Previous		&Föregående

" Window menu
menutrans &Window			&Fönster
menutrans &New<Tab>^Wn			&Nytt<Tab>^Wn
menutrans S&plit<Tab>^Ws		&Dela<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^	&Dela\ till\ #<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv	Dela\ &vertikalt<Tab>^Wv
menutrans Split\ File\ E&xplorer	Dela\ f&ilhanterare
menutrans &Close<Tab>^Wc		&Stäng<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo	&Stäng\ alla\ andra<Tab>^Wo
menutrans Ne&xt<Tab>^Ww			&Nästa<Tab>^Ww
menutrans P&revious<Tab>^WW		&Föregående<Tab>^WW
menutrans &Equal\ Size<Tab>^W=		&Samma\ storlek<Tab>^W=
menutrans &Max\ Height<Tab>^W_		&Maximal\ storlek<Tab>^W_
menutrans M&in\ Height<Tab>^W1_		M&inimal\ storlek<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|		Ma&ximal\ bredd<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|		Mi&nimal\ bredd<Tab>^W1\|
menutrans Move\ &To			Flytta\ &till
menutrans &Top<Tab>^WK			&Toppen<Tab>^WK
menutrans &Bottom<Tab>^WJ		&Botten<Tab>^WJ
menutrans &Left\ side<Tab>^WH		&Vänstra\ sidan<Tab>^WH
menutrans &Right\ side<Tab>^WL		&Högra\ sidan<Tab>^WL
menutrans Rotate\ &Up<Tab>^WR		Rotera\ &upp<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		Rotera\ &ned<Tab>^Wr
menutrans Select\ Fo&nt\.\.\.		Välj\ t&ypsnitt\.\.\.

" The popup menu
menutrans &Undo			Å&ngra
menutrans Cu&t			Klipp\ &ut
menutrans &Copy			&Kopiera
menutrans &Paste		Klistra\ &in
menutrans &Delete		&Ta\ bort
menutrans Select\ Blockwise	Markera\ blockvis
menutrans Select\ &Word		Markera\ &ord
menutrans Select\ &Line		Markera\ &rad
menutrans Select\ &Block	Markera\ &block
menutrans Select\ &All		Markera\ &allt

" The GUI toolbar (for Win32 or GTK)
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		Öppna fil
    tmenu ToolBar.Save		Spara aktuell fil
    tmenu ToolBar.SaveAll	Spara alla filer
    tmenu ToolBar.Print		Skriv ut
    tmenu ToolBar.Undo		Ångra
    tmenu ToolBar.Redo		Gör om
    tmenu ToolBar.Cut		Klipp ut
    tmenu ToolBar.Copy		Kopiera
    tmenu ToolBar.Paste		Klistra in
    tmenu ToolBar.Find		Sök...
    tmenu ToolBar.FindNext	Sök nästa
    tmenu ToolBar.FindPrev	Sök föregående
    tmenu ToolBar.Replace	Sök och ersätt...
    tmenu ToolBar.LoadSesn	Läs in session
    tmenu ToolBar.SaveSesn	Spara session
    tmenu ToolBar.RunScript	Kör ett Vim-skript
    tmenu ToolBar.Make		Bygg aktuellt projekt
    tmenu ToolBar.Shell		Öppna ett kommandoskal
    tmenu ToolBar.RunCtags	Kör Ctags
    tmenu ToolBar.TagJump	Hoppa till tagg under markör
    tmenu ToolBar.Help		Hjälp
    tmenu ToolBar.FindHelp	Sök i hjälp
  endfun
endif

" Syntax menu
menutrans &Syntax			&Syntax
menutrans &Show\ File\ Types\ in\ Menu	&Visa\ filtyper\ i\ meny
menutrans &Off				&Av
menutrans &Manual			&Manuellt
menutrans A&utomatic			A&utomatiskt
menutrans on/off\ for\ &This\ file	Av/På\ för\ a&ktuell\ fil
menutrans Co&lor\ test			Fär&gtest
menutrans &Highlight\ test		&Framhävningstest
menutrans &Convert\ to\ HTML		&Konvertera\ till\ HTML

" dialog texts
let menutrans_no_file = "[Ingen fil]"
let menutrans_help_dialog = "Skriv in ett kommando eller ord som du vill söka hjälp på:\n\nBörja med i_ för infogninglägeskommandon (t.ex. i_CTRL-X)\nBörja med c_ för kommandoradredigeringskommandon (t.ex. c_<Del>)\nBörja med ' för ett inställningsnamn (t.ex. 'shiftwidth')"
let g:menutrans_path_dialog = "Skriv in sökväg för filer.\nSeparera katalognamn med komma"
let g:menutrans_tags_dialog = "Skriv in namn på taggfiler.\nSeparera namn med komma."
let g:menutrans_textwidth_dialog = "Välj ny textbredd (0 för att förhindra formatering): "
let g:menutrans_fileformat_dialog = "Välj filformat som filen ska sparas med"
let g:menutrans_fileformat_choices = "&Unix\n&Dos\n&Mac\n&Avbryt"

let &cpo = s:keepcpo
unlet s:keepcpo
