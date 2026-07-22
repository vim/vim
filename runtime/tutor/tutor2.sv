===============================================================================
=    V ä l k o m m e n   t i l l   V I M - h a n d l e d n i n g e n          =
=                              -   Version 1.7                                =
===============================================================================
=			    K A P I T E L   T V Å			      =
===============================================================================

     Hic Sunt Dracones: om detta är din första kontakt med Vim och du
     avsåg att börja med introduktionskapitlet, skriv vänligen
     :q!<ENTER> och kör vimtutor för kapitel 1 istället.

     Den ungefärliga tiden för att slutföra detta kapitel är 8-10 minuter,
     beroende på hur mycket tid som läggs på experimentering.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lektion 2.1.1: BEMÄSTRA TEXTOBJEKT

   ** Arbeta på logiska textblock med precision med hjälp av textobjekt **

  1. Öva på ordoperationer:
     - Placera markören på valfritt ord i raden nedan
     - Skriv  diw  för att radera INRE ord (ord utan omgivande mellanslag)
     - Skriv  daw  för att radera ETT ORD (inklusive efterföljande mellanslag)
     - Prova med andra operatorer:  ciw  (ändra),  yiw  (kopiera),  gqiw  (formatera)

---> Öva på: "Vims", (text_objekt), och 'kraftfulla' ord här.

  2. Arbeta med innehåll inom parenteser:
     - Placera markören inuti något () {} [] <> par nedan
     - Skriv  di(  eller  dib  (radera inuti parentes)
     - Skriv  da(  eller  dab  (radera runt parenteser)
     - Prova samma med  i"/a"  för citattecken,  it/at  för HTML/XML-taggar

---> Testfall: {klamrar}, [hakparenteser], <vinkelparenteser>, och "citerade" objekt.

  3. Stycke- och meningsmanipulering:
     - Använd  dip  för att radera inre stycke (markören var som helst i stycket)
     - Använd  vap  för att visuellt markera hela stycket
     - Prova  das  för att radera en mening (fungerar mellan .!? skiljetecken)

  4. Avancerade kombinationer:
     - ciwnew<ESC>    - Ändra nuvarande ord till "new"
     - ciw"<CTRL-R>-"<ESC> - Omslut nuvarande ord med citattecken
     - gUit           - Gör HTML-tagginnehåll till versaler
     - va"p           - Markera citerad text och klistra över den

---> Slutövning: (Ändra "denna" text) genom att [tillämpa {olika} operationer]<

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lektion 2.1.2: NAMNGIVNA REGISTER


         ** Lagra två kopierade ord samtidigt och klistra sedan in dem **

  1. Flytta markören till raden nedan markerad --->

  2. Navigera till valfri punkt på ordet 'Edward' och skriv   "ayiw

MINNESREGEL: till register(") namngivet (a) (y)ank (kopiera) (i)nre (w)ord (ord)

  3. Navigera framåt till ordet 'kaka' (fk eller 3fc eller $2b eller /ka<ENTER>)
     och skriv   "byiw

  4. Navigera till valfri punkt på ordet 'Vince' och skriv   ciw<CTRL-R>a<ESC>

MINNESREGEL: (c)hange (ändra) (i)nre (w)ord (ord) med <innehåll från (r)egister> namngivet (a)

  5. Navigera till valfri punkt på ordet 'tårta' och skriv   ciw<CTRL-R>b<ESC>

--->  a) Edward kommer hädanefter att ansvara för kaka-ransonerna
      b) I denna egenskap kommer Vince ha ensam tårta-beslutanderätt

OBS: Radering fungerar också till register, dvs. "sdiw raderar ordet under
     markören till register s.

REFERENS: 	Register 	:h registers
		Namngivna register :h quotea
		Förflyttning 	:h motion.txt<ENTER> /inner<ENTER>
		CTRL-R		:h insert<ENTER> /CTRL-R<ENTER>

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lektion 2.1.3: UTTRYCKSREGISTRET


	     ** Infoga resultat av beräkningar direkt **

  1. Flytta markören till raden nedan markerad --->

  2. Navigera till valfri punkt på det angivna numret

  3. Skriv ciw<CTRL-R> följt av  =60*60*24<ENTER>

  4. På nästa rad, gå in i infogningsläge och lägg till dagens datum med
     <CTRL-R> följt av  =system('date')<ENTER>

OBS: Alla anrop till system är OS-beroende, t.ex. på Windows använd
      system('date /t')   eller  :r!date /t

---> Jag har glömt det exakta antalet sekunder på en dag, är det 84600?
     Dagens datum är:

OBS: samma sak kan uppnås med :pu=system('date')
      eller, med färre tangenttryckningar :r!date

REFERENS: 	Uttrycksregister 	:h quote=

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lektion 2.1.4: NUMRERADE REGISTER


	** Tryck  yy och dd för att se deras effekt på registren **

  1. Flytta markören till raden nedan markerad --->

  2. Kopiera noll-raden, sedan inspektera register med  :reg<ENTER>

  3. Radera rad 0. med "cdd, sedan inspektera register
     (Var förväntar du dig att rad 0 ska vara?)

  4. Fortsätt radera varje efterföljande rad, inspektera :reg medan du gör det

OBS: Du bör märka att gamla helradsraderingar flyttas nedåt i listan
      när nya helradsraderingar läggs till

  5. Nu (p)asta följande register i ordning; c, 7, 4, 8, 2. dvs. "7p

---> 0. Detta
     9. vinglar
     8. hemliga
     7. är
     6. på
     5. axel
     4. ett
     3. krig
     2. meddelande
     1. hyllning

OBS: Helradsraderingar (dd) lever mycket längre i de numrerade registren
      än helradskopieringar, eller raderingar med mindre förflyttningar

REFERENS: 	Numrerade register 	:h quote0

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lektion 2.1.5: SPECIALREGISTER

 ** Använd systemets urklipp och svarta hålet för avancerad redigering **

 Obs: Urklippsanvändning kräver X11/Wayland-bibliotek på Linux-system OCH
      en Vim byggd med "+clipboard" (vanligtvis en Huge-byggning). Kontrollera med
      ":version"  och ":echo has('clipboard_working')"

  1. Urklippsregister  +  och  *  :
     - "+y  - Kopiera till systemets urklipp (t.ex. "+yy för nuvarande rad)
     - "+p  - Klistra in från systemets urklipp
     - "* är primärmarkering på X11 (mittenklick), "+ är urklipp

---> Prova: "+yy sedan klistra in i ett annat program med Ctrl-V eller Cmd+V

  2. Svarta hålet-register  _  kastar text:
     - "_daw  - Radera ord utan att spara till något register
     - Användbart när du inte vill skriva över ditt standard " register
     - Observera att detta använder "ett Ord" textobjekt, introducerat i en
       tidigare lektion
     - "_dd   - Radera rad utan att spara
     - "_dap  - Radera stycke utan att spara
     - Kombinera med antal: 3"_dw

---> Öva: "_diw på valfritt ord för att radera det utan att påverka kopieringshistorik

  3. Kombinera med visuella markeringar:
     - Markera text med V sedan "+y
     - För att klistra in från urklipp i infogningsläge: <CTRL-R>+
     - Prova att öppna ett annat program och klistra in från urklipp

  4. Kom ihåg:
     - Urklippsregister fungerar mellan olika Vim-instanser
     - Urklippsregister fungerar inte alltid
     - Svarta hålet förhindrar oavsiktlig registerskrivning
     - Standard " register är fortfarande tillgängligt för normal kopiering/inklistring
     - Namngivna register (a-z) förblir privata för varje Vim-session

  5. Urklippsfelsökning:
     - Kontrollera stöd med :echo has('clipboard_working')
     - 1 betyder tillgängligt, 0 betyder inte inkompilerat
     - På Linux kan vim-gtk eller vim-x11 paket behövas
       (kontrollera :version utdata)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lektion 2.1.6: SKÖNHETEN MED MARKERINGAR

	           ** Kodapans aritmetik-undvikande **

OBS: ett vanligt problem vid kodning är att flytta runt stora kodblock.
      Följande teknik hjälper till att undvika radnummerberäkningar associerade
      med operationer som   "a147d   eller   :945,1091d a   eller ännu värre att använda
      i<CTRL-R> följt av   =1091-945<ENTER>   först

  1. Flytta markören till raden nedan markerad --->

  2. Gå till första raden av funktionen och markera den med   ma

OBS: exakt position på raden är INTE viktigt!

  3. Navigera till slutet av raden och sedan slutet av kodblocket
     med   $%

  4. Radera blocket till register a med   "ad'a

MINNESREGEL: till register(") namngivet (a) lägg (d)eletionen (raderingen) från markören till
          RADEN som innehåller markering(') (a)

  5. Klistra in blocket mellan BBB och CCC   "ap

OBS: öva denna operation flera gånger för att bli flytande   ma$%"ad'a

---> AAA
     function detBlevStortSnabbt() {
       if ( nagotArSant ) {
         gorDet()
       }
       // vår funktions taxonomi har ändrats och den
       // är inte längre alfabetiskt logisk på sin nuvarande plats

       // tänk dig hundratals rader kod

       // naivt kunde du navigera till början och slutet och spela in eller
       // komma ihåg varje radnummer
     }
     BBB
     CCC

OBS: markeringar och register delar inte namnrymd, därför är register a
      helt oberoende av markering a. Detta gäller inte register och
      makron.

REFERENS: 	Markeringar 	:h marks
		Markeringsförflyttningar :h mark-motions  (skillnad mellan ' och `)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lektion 2.1 SAMMANFATTNING

  1. Textobjekt ger precisionsredigering:
     - iw/aw - inre/runt ord
     - i[/a[ - inre/runt hakparentes
     - i"/a" - inre/runt citattecken
     - it/at - inre/runt tagg
     - ip/ap - inre/runt stycke
     - is/as - inre/runt mening

  2. För att lagra (kopiera, radera) text till, och hämta (klistra in) från, totalt
     26 register (a-z)
  3. Kopiera ett helt ord från var som helst inom ett ord:   yiw
  4. Ändra ett helt ord från var som helst inom ett ord:   ciw
  5. Infoga text direkt från register i infogningsläge:   <CTRL-R>a

  6. Infoga resultat av enkla aritmetiska operationer: <CTRL-R> följt av
     =60*60<ENTER>
     i infogningsläge
  7. Infoga resultat av systemanrop: <CTRL-R> följt av
     =system('ls -1')<ENTER>
     i infogningsläge

  8. Inspektera register med   :reg
  9. Lär dig slutdestinationen för helradsraderingar: dd i de numrerade
     registren, dvs. fallande från register 1 - 9. Uppskatta att hel-
     radsraderingar bevaras längre i de numrerade registren än någon
     annan operation
 10. Lär dig slutdestinationen för alla kopieringar i de numrerade registren och
     hur flyktiga de är

 11. Placera markeringar från kommandoläge   m[a-zA-Z0-9]
 12. Flytta radvis till en markering med   '

 13. Specialregister:
     - "+/"*  - Systemets urklipp (OS-beroende)
     - "_     - Svarta hålet (kasta raderad/kopierad text)
     - "=     - Uttrycksregister
     - "-     - Register för små raderingar

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Detta avslutar kapitel två av Vim-handledningen. Det är ett pågående arbete.

  Detta kapitel skrevs av Paul D. Parker och Christian Brabandt.
  Svensk översättning av Daniel Nylander.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
