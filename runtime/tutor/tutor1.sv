===============================================================================
= V ä l k o m m e n  t i l l  V I M - h a n d l e d n i n g e n  - Ver. 1.7  =
===============================================================================
=			    K A P I T E L   E T T			      =
===============================================================================

     Vim är en väldigt kraftfull redigerare som har många kommandon, alltför
     många att förklara i en handledning som denna. Den här handledningen är
     gjord för att beskriva tillräckligt många kommandon så att du enkelt ska
     kunna använda Vim som en redigerare för alla ändamål.
     Den beräknade tiden för att slutföra denna handledning är 30 minuter,
     beroende på hur mycket tid som läggs ned på experimentering.

     OBSERVERA:
     Kommandona i lektionerna kommer att modifiera texten. Gör en kopia av den
     här filen att öva på (om du startade "vimtutor" är det här redan en kopia).

     Det är viktigt att komma ihåg att den här handledningen är konstruerad
     att lära vid användning. Det betyder att du måste köra kommandona för att
     lära dig dem ordentligt. Om du bara läser texten så kommer du att glömma
     kommandona!
     Försäkra dig nu om att din Caps-Lock-tangent INTE är aktiv och tryck på
     j-tangenten tillräckligt många gånger för att förflytta markören så att
     Lektion 1.1.1 fyller skärmen helt.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			Lektion 1.1.1: FLYTTA MARKÖREN


   ** För att flytta markören, tryck på tangenterna h,j,k,l som indikerat. **
	     ^
	     k		Tips:  h-tangenten är till vänster och flyttar vänster.
       < h	 l >	       l-tangenten är till höger och flyttar höger.
	     j		       j-tangenten ser ut som en pil ned.
	     v
  1. Flytta runt markören på skärmen tills du känner dig bekväm.

  2. Håll ned tangenten (j) tills den repeterar.
     Nu vet du hur du tar dig till nästa lektion.

  3. Flytta till Lektion 1.1.2, med hjälp av ned-tangenten.

NOTERA: Om du är osäker på någonting du skrev, tryck <ESC> för att placera
	dig i Normalläge. Skriv sedan om kommandot du ville använda.

NOTERA: Piltangenterna borde också fungera. Men om du använder hjkl så kommer
	du att kunna flytta runt mycket snabbare, när du väl vant dig vid det.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			    Lektion 1.1.2: AVSLUTA VIM


  !! NOTERA: Innan du utför någon av punkterna nedan, läs hela lektionen!!

  1. Tryck <ESC>-tangenten (för att se till att du är i Normalläge).

  2. Skriv:	:q! <ENTER>.
     Detta avslutar redigeraren UTAN att spara några ändringar.

  3. Kom tillbaka hit genom att köra kommandot som tog dig till den här
     handledningen. Det kan vara:  vimtutor <ENTER>

  4. Om du har memorerat dessa steg och känner dig säker, kör steg 1 till 3
     för att avsluta och starta om redigeraren.

NOTERA: :q! <ENTER> kastar alla ändringar du gjort. Om några lektioner lär
	du dig hur du sparar ändringar till en fil.

  5. Flytta ned markören till Lektion 1.1.3.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lektion 1.1.3: TEXTREDIGERING - BORTTAGNING


	   ** Tryck  x  för att ta bort tecknet under markören. **

  1. Flytta markören till raden nedan markerad med --->.

  2. För att rätta felen, flytta markören tills den står på tecknet som ska
     tas bort.

  3. Tryck på  x-tangenten för att ta bort det oönskade tecknet.

  4. Upprepa steg 2 till 4 tills meningen är korrekt.

---> Kkon hoppadee övverr måånen.

  5. Nu när raden är korrekt, gå till Lektion 1.1.4.

NOTERA: När du går igenom den här handledningen, försök inte memorera, lär
	genom användning.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      Lektion 1.1.4: TEXTREDIGERING - INFOGNING


		    ** Tryck  i  för att infoga text. **

  1. Flytta markören till första raden nedan markerad med --->.

  2. För att göra första raden likadan som den andra, flytta markören till
     tecknet FÖRE vilket texten ska infogas.

  3. Tryck  i  och skriv in de nödvändiga tilläggen.

  4. När varje fel är rättat, tryck <ESC> för att återgå till Normalläge.
     Upprepa steg 2 till 4 för att rätta meningen.

---> Det saknas lit från den här .
---> Det saknas lite text från den här raden.

  5. När du känner dig bekväm med att infoga text, gå till Lektion 1.1.5.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lektion 1.1.5: TEXTREDIGERING - LÄGGA TILL


		    ** Tryck  A  för att lägga till text. **

  1. Flytta markören till första raden nedan markerad med --->.
     Det spelar ingen roll på vilket tecken markören är på den raden.

  2. Tryck  A  och skriv in de nödvändiga tilläggen.

  3. När texten har lagts till, tryck <ESC> för att återgå till Normalläge.

  4. Flytta markören till den andra raden markerad ---> och upprepa
     steg 2 och 3 för att rätta denna mening.

---> Det saknas lite text från de
     Det saknas lite text från den här raden.
---> Det saknas också lite tex
     Det saknas också lite text här.

  5. När du känner dig bekväm med att lägga till text, gå till Lektion 1.1.6.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lektion 1.1.6: REDIGERA EN FIL


		    ** Använd  :wq  för att spara en fil och avsluta. **

  !! NOTERA: Innan du utför någon av punkterna nedan, läs hela lektionen!!

  1.  Om du har tillgång till en annan terminal, gör följande där.
      Annars, avsluta denna handledning som i Lektion 1.1.2:  :q!

  2. Vid skalprompten, skriv detta kommando:  vim fil.txt <ENTER>
     'vim' är kommandot för att starta Vim-redigeraren, 'fil.txt' är namnet på
     filen du vill redigera. Använd ett namn på en fil som du kan ändra.

  3. Infoga och ta bort text som du lärt dig i tidigare lektioner.

  4. Spara filen med ändringar och avsluta Vim med:  :wq <ENTER>

  5. Om du avslutade vimtutor i steg 1, starta om vimtutor och flytta ned till
     följande sammanfattning.

  6. Efter att ha läst stegen ovan och förstått dem: gör det.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			       Lektion 1.1 SAMMANFATTNING


  1. Markören flyttas med antingen piltangenterna eller hjkl-tangenterna.
	 h (vänster)	j (ned)       k (upp)	    l (höger)

  2. För att starta Vim från skalprompten, skriv:  vim FILNAMN <ENTER>

  3. För att avsluta Vim, skriv:   <ESC>   :q!	<ENTER>  för att kasta ändringar.
	     ELLER skriv:	   <ESC>   :wq	<ENTER>  för att spara ändringar.

  4. För att ta bort tecknet vid markören, skriv:  x

  5. För att infoga eller lägga till text, skriv:
	 i   skriv infogad text   <ESC>		infoga före markören
	 A   skriv tillagd text   <ESC>         lägg till efter raden

NOTERA: Att trycka <ESC> placerar dig i Normalläge eller avbryter ett
	oönskat och delvis slutfört kommando.

Fortsätt nu med Lektion 1.2.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			Lektion 1.2.1: BORTTAGNINGSKOMMANDON


		       ** Skriv  dw  för att ta bort ett ord. **

  1. Tryck  <ESC>  för att se till att du är i Normalläge.

  2. Flytta markören till raden nedan markerad med --->.

  3. Flytta markören till början av ett ord som behöver tas bort.

  4. Skriv   dw	 för att få ordet att försvinna.

NOTERA: Bokstaven  d  kommer att synas på sista raden på skärmen när du
	skriver den. Vim väntar på att du ska skriva  w . Om du ser ett annat
	tecken än  d  skrev du fel; tryck <ESC> och börja om.

---> Det finns a några ord kul som inte hör hemma papper i den här meningen.

  5. Upprepa steg 3 och 4 tills meningen är korrekt och gå till Lektion 1.2.2.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      Lektion 1.2.2: FLER BORTTAGNINGSKOMMANDON


	   ** Skriv  d$  för att ta bort till slutet av raden. **

  1. Tryck  <ESC>  för att se till att du är i Normalläge.

  2. Flytta markören till raden nedan markerad med --->.

  3. Flytta markören till slutet av den korrekta raden (EFTER den första . ).

  4. Skriv    d$    för att ta bort till slutet av raden.

---> Någon skrev slutet av denna rad två gånger. slutet av denna rad två gånger.


  5. Gå vidare till Lektion 1.2.3 för att förstå vad som händer.





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lektion 1.2.3: OM OPERATORER OCH RÖRELSER


  Många kommandon som ändrar text är gjorda av en operator och en rörelse.
  Formatet för ett borttagningskommando med  d  borttagningsoperatorn är:

  	d   rörelse

  Där:
    d      - är borttagningsoperatorn.
    rörelse - är vad operatorn ska operera på (listad nedan).

  En kort lista med rörelser:
    w - till början av nästa ord, EXKLUSIVE dess första tecken.
    e - till slutet av nuvarande ord, INKLUSIVE det sista tecknet.
    $ - till slutet av raden, INKLUSIVE det sista tecknet.

  Alltså kommer  de  att ta bort från markören till slutet av ordet.

NOTERA: Att bara trycka rörelsen i Normalläge utan operator kommer att flytta
	markören som specificerat.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lektion 1.2.4: ANVÄNDA ANTAL FÖR EN RÖRELSE


   ** Att skriva ett nummer före en rörelse repeterar den så många gånger. **

  1. Flytta markören till början av raden nedan markerad med --->.

  2. Skriv  2w  för att flytta markören två ord framåt.

  3. Skriv  3e  för att flytta markören till slutet av det tredje ordet framåt.

  4. Skriv  0  (noll) för att flytta till början av raden.

  5. Upprepa steg 2 och 3 med olika nummer.

---> Detta är bara en rad med ord som du kan flytta runt i.

  6. Gå vidare till Lektion 1.2.5.




~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lektion 1.2.5: ANVÄNDA ANTAL FÖR ATT TA BORT MER


   ** Att skriva ett nummer med en operator repeterar den så många gånger. **

  I kombinationen av borttagningsoperatorn och en rörelse nämnd ovan kan du
  infoga ett antal före rörelsen för att ta bort mer:
	 d   antal   rörelse

  1. Flytta markören till det första VERSALA ordet på raden markerad med --->.

  2. Skriv  d2w  för att ta bort de två VERSALA orden.

  3. Upprepa steg 1 och 2 med olika antal för att ta bort de efterföljande
     VERSALA orden med ett kommando.

--->  denna ABC DE rad FGHI JK LMN OP av ord är Q RS TUV städad.





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 Lektion 1.2.6: OPERERA PÅ RADER


		   ** Skriv  dd   för att ta bort en hel rad. **

  På grund av hur vanligt det är att ta bort hela rader, beslutade Vis
  konstruktörer att det skulle vara enklare att bara skriva två d för att
  ta bort en rad.

  1. Flytta markören till den andra raden i frasen nedan.
  2. Skriv  dd  för att ta bort raden.
  3. Flytta nu till den fjärde raden.
  4. Skriv   2dd   för att ta bort två rader.

--->  1)  Rosor är röda,
--->  2)  Lera är kul,
--->  3)  Violer är blå,
--->  4)  Jag har en bil,
--->  5)  Klockor visar tid,
--->  6)  Socker är sött
--->  7)  Och det är du med.

Att dubbla för att operera på en rad fungerar också för operatorer nämnda nedan.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 Lektion 1.2.7: ÅNGRA-KOMMANDOT


   ** Tryck  u  för att ångra senaste kommandot,  U  för att fixa en hel rad. **

  1. Flytta markören till raden nedan markerad ---> och placera den på det
     första felet.
  2. Skriv  x  för att ta bort det första oönskade tecknet.
  3. Skriv nu  u  för att ångra det senast körda kommandot.
  4. Denna gång, rätta alla fel på raden med  x  kommandot.
  5. Skriv nu ett stort  U  för att återställa raden till dess ursprungliga tillstånd.
  6. Skriv nu  u  några gånger för att ångra  U  och föregående kommandon.
  7. Tryck nu CTRL-R (håll CTRL nedtryckt medan du trycker R) några gånger
     för att göra om kommandona (ångra ångringarna).

---> Rätta feelen på dennna rad ochh ersätt dem meed ångra.

  8. Dessa är mycket användbara kommandon. Gå nu vidare till Lektion 1.2 Sammanfattning.




~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			       Lektion 1.2 SAMMANFATTNING

  1. För att ta bort från markören till nästa ord, skriv:       dw
  2. För att ta bort från markören till slutet av ordet, skriv: de
  3. För att ta bort från markören till slutet av raden, skriv: d$
  4. För att ta bort en hel rad, skriv:                         dd

  5. För att repetera en rörelse, sätt ett nummer före den:   2w
  6. Formatet för ett ändringskommando är:
               operator   [antal]   rörelse
     där:
       operator - är vad som ska göras, såsom  d  för ta bort
       [antal]  - är ett valfritt antal för att repetera rörelsen
       rörelse  - rör sig över texten att operera på, såsom  w (ord),
		  e (slut på ord),  $ (slut på rad), osv.

  7. För att flytta till början av raden, använd noll:  0

  8. För att ångra tidigare handlingar, skriv:         u  (litet u)
     För att ångra alla ändringar på en rad, skriv:    U  (stort U)
     För att ångra ångringarna, skriv:                 CTRL-R

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 Lektion 1.3.1: KLISTRA IN-KOMMANDOT


       ** Skriv  p  för att klistra in tidigare borttagen text efter markören. **

  1. Flytta markören till den första raden nedan markerad med --->.

  2. Skriv  dd  för att ta bort raden och lagra den i ett Vim-register.

  3. Flytta markören till rad c), OVANFÖR där den borttagna raden ska vara.

  4. Skriv   p   för att lägga raden under markören.

  5. Upprepa steg 2 till 4 för att lägga alla rader i korrekt ordning.

---> d) Kan du också lära dig?
---> b) Violer är blå,
---> c) Intelligens är lärt,
---> a) Rosor är röda,



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		       Lektion 1.3.2: ERSÄTT-KOMMANDOT


       ** Skriv  rx  för att ersätta tecknet vid markören med  x . **

  1. Flytta markören till den första raden nedan markerad med --->.

  2. Flytta markören så att den är ovanpå det första felet.

  3. Skriv   r	och sedan tecknet som borde vara där.

  4. Upprepa steg 2 och 3 tills den första raden är lika som den andra.

--->  Näe danne rad skrevs in, tryckte någpn några felaktiga tangenter!
--->  När denna rad skrevs in, tryckte någon några felaktiga tangenter!

  5. Gå nu vidare till Lektion 1.3.3.

NOTERA: Kom ihåg att du bör lära dig genom att göra, inte genom att memorera.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			Lektion 1.3.3: ÄNDRA-OPERATORN


	   ** För att ändra till slutet av ett ord, skriv  ce . **

  1. Flytta markören till den första raden nedan markerad med --->.

  2. Placera markören på  u  i  kubw.

  3. Skriv  ce  och det korrekta ordet (i detta fall, skriv  ine ).

  4. Tryck <ESC> och flytta till nästa tecken som behöver ändras.

  5. Upprepa steg 3 och 4 tills den första meningen är likadan som den andra.

---> Denna kubw har några otf som mfpr ändras anef ändra-operatorn.
---> Denna rad har några ord som behöver ändras med ändra-operatorn.

Observera att  ce  tar bort ordet och placerar dig i Infogningsläge.
               cc  gör samma sak för hela raden.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		       Lektion 1.3.4: FLER ÄNDRINGAR MED c


     ** Ändra-operatorn används med samma rörelser som borttagning. **

  1. Ändra-operatorn fungerar på samma sätt som borttagning. Formatet är:

         c    [antal]   rörelse

  2. Rörelserna är desamma, såsom   w (ord) och  $ (slutet av raden).

  3. Flytta markören till den första raden nedan markerad med --->.

  4. Flytta markören till det första felet.

  5. Skriv  c$  och skriv resten av raden som den andra och tryck <ESC>.

---> Slutet av denna rad behöver hjälp för att bli som den andra.
---> Slutet av denna rad behöver rättas med c$ kommandot.

NOTERA: Du kan använda Backsteg-tangenten för att rätta misstag när du skriver.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			       Lektion 1.3 SAMMANFATTNING


  1. För att klistra tillbaka text som just tagits bort, skriv   p . Detta
     lägger den borttagna texten EFTER markören (om en rad togs bort hamnar
     den på raden under markören).

  2. För att ersätta tecknet under markören, skriv   r   och sedan tecknet
     du vill ha där.

  3. Ändra-operatorn låter dig ändra från markören till dit rörelsen tar dig.
     T.ex. Skriv  ce  för att ändra från markören till slutet av ordet,
     c$  för att ändra till slutet av raden.

  4. Formatet för ändra är:

	 c   [antal]   rörelse

Gå nu vidare till nästa lektion.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		  Lektion 1.4.1: MARKÖRPOSITION OCH FILSTATUS

  ** Skriv CTRL-G för att visa din position i filen och filstatus.
     Skriv  G  för att flytta till en rad i filen. **

  NOTERA: Läs hela denna lektion innan du utför något av stegen!!

  1. Håll ned Ctrl-tangenten och tryck  g . Vi kallar detta CTRL-G.
     Ett meddelande kommer att visas längst ned på sidan med filnamnet och
     positionen i filen. Kom ihåg radnumret för Steg 3.

NOTERA: Du kanske ser markörpositionen i nedre högra hörnet av skärmen.
	Detta händer när 'ruler'-alternativet är satt (se  :help 'ruler'  )

  2. Tryck  G  för att flytta dig till botten av filen.
     Skriv  gg  för att flytta dig till början av filen.

  3. Skriv numret på raden du var på och sedan  G . Detta tar dig tillbaka
     till raden du var på när du först tryckte CTRL-G.

  4. Om du känner dig säker på detta, utför steg 1 till 3.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			Lektion 1.4.2: SÖK-KOMMANDOT


     ** Skriv  /  följt av en fras för att söka efter frasen. **

  1. I Normalläge, skriv  /  tecknet. Lägg märke till att det och markören
     visas längst ned på skärmen som med  :  kommandot.

  2. Skriv nu 'feeeel' <ENTER>. Detta är ordet du vill söka efter.

  3. För att söka efter samma fras igen, skriv helt enkelt  n .
     För att söka efter samma fras i motsatt riktning, skriv  N .

  4. För att söka efter en fras baklänges, använd  ?  istället för  / .

  5. För att gå tillbaka dit du kom ifrån, tryck  CTRL-O  (håll Ctrl nedtryckt
     medan du trycker bokstaven o). Upprepa för att gå längre tillbaka. CTRL-I
     går framåt.

---> "feeeel" är inte hur man stavar feel; feeeel är ett fel.

NOTERA: När sökningen når slutet av filen fortsätter den från början, såvida
	inte 'wrapscan'-alternativet har återställts.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		   Lektion 1.4.3: SÖKA MATCHANDE PARENTESER


	      ** Skriv  %  för att hitta en matchande ), ] eller }. **

  1. Placera markören på någon (, [ eller { på raden nedan markerad med --->.

  2. Skriv nu  %  tecknet.

  3. Markören kommer att flytta till den matchande parentesen eller hakparentesen.

  4. Skriv  %  för att flytta markören till den andra matchande parentesen.

  5. Flytta markören till en annan (, ), [, ], { eller } och se vad  %  gör.

---> Detta ( är en testrad med (, [ ] och { } i den. ))


NOTERA: Detta är väldigt användbart vid felsökning av ett program med
	omatchade parenteser!



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      Lektion 1.4.4: ERSÄTT-KOMMANDOT


	** Skriv  :s/gammalt/nytt/g  för att ersätta 'gammalt' med 'nytt'. **

  1. Flytta markören till raden nedan markerad med --->.

  2. Skriv  :s/denn/den/ <ENTER> . Notera att detta kommando bara ändrar den
     första förekomsten av "denn" på raden.

  3. Skriv nu  :s/denn/den/g . Att lägga till  g  flaggan betyder att ersätta
     globalt på raden, och ändrar alla förekomster av "denn" på raden.

---> denn bästa tiden att se denn blomman är under denn sommarn.

  4. För att ändra varje förekomst av en teckensträng mellan två rader,
     skriv  :#,#s/gammalt/nytt/g  där #,# är radnumren på de två raderna för
			         intervallet där ersättningen ska göras.
     Skriv  :%s/gammalt/nytt/g  för att ändra varje förekomst i hela filen.
     Skriv  :%s/gammalt/nytt/gc för att hitta varje förekomst i hela filen,
			         med en prompt om du vill ersätta eller inte.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			       Lektion 1.4 SAMMANFATTNING


  1. CTRL-G  visar din position i filen och filstatus.
	     G  flyttar till slutet av filen.
     antal  G  flyttar till det radnumret.
	    gg  flyttar till första raden.

  2. Att skriva  /  följt av en fras söker FRAMÅT efter frasen.
     Att skriva  ?  följt av en fras söker BAKÅT efter frasen.
     Efter en sökning, skriv  n  för att hitta nästa förekomst i samma
     riktning eller  N  för att söka i motsatt riktning.
     CTRL-O tar dig tillbaka till äldre positioner, CTRL-I till nyare positioner.

  3. Att skriva  %  medan markören är på en (, ), [, ], { eller } hittar dess
     matchande par.

  4. För att ersätta ny för den första gammalt på en rad, skriv    :s/gammalt/nytt
     För att ersätta ny för alla gammalt på en rad, skriv          :s/gammalt/nytt/g
     För att ersätta fraser mellan två radnummer, skriv            :#,#s/gammalt/nytt/g
     För att ersätta alla förekomster i filen, skriv               :%s/gammalt/nytt/g
     För att få fråga om bekräftelse varje gång, lägg till 'c'     :%s/gammalt/nytt/gc

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		Lektion 1.5.1: HUR MAN KÖR ETT EXTERNT KOMMANDO


   ** Skriv  :!  följt av ett externt kommando för att köra det kommandot. **

  1. Skriv det välbekanta kommandot  :  för att placera markören längst ned
     på skärmen. Detta låter dig skriva ett kommandorads-kommando.

  2. Skriv nu  !  (utropstecken). Detta låter dig köra vilket externt
     skalkommando som helst.

  3. Som ett exempel, skriv   ls   efter ! och tryck sedan <ENTER>. Detta
     kommer att visa dig en lista över din katalog, precis som om du var vid
     en skalprompt. Eller använd  :!dir  om  ls  inte fungerar.

NOTERA: Det är möjligt att köra vilket externt kommando som helst på detta sätt,
	också med argument.

NOTERA: Alla  :  kommandon måste avslutas genom att trycka <ENTER>
	Från och med nu nämner vi inte alltid det.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      Lektion 1.5.2: MER OM ATT SPARA FILER


	** För att spara ändringarna i filen, skriv  :w FILNAMN . **

  1. Skriv  :!ls  eller  :!dir  för att få en lista över din katalog.
     Du vet redan att du måste trycka <ENTER> efter detta.

  2. Välj ett filnamn som inte finns ännu, såsom TEST.

  3. Skriv nu:	 :w TEST   (där TEST är filnamnet du valde.)

  4. Detta sparar hela filen (Vim Tutor) under namnet TEST.
     För att verifiera detta, skriv   :!ls   eller  :!dir  igen för att se
     din katalog.

NOTERA: Om du skulle avsluta Vim och starta igen med  vim TEST , skulle filen
	vara en exakt kopia av handledningen när du sparade den.

  5. Ta nu bort filen genom att skriva (MS-DOS):    :!del TEST
				   eller (Unix):    :!rm TEST


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    Lektion 1.5.3: VÄLJA TEXT ATT SPARA


	** För att spara del av filen, skriv  v  rörelse  :w FILNAMN **

  1. Flytta markören till denna rad.

  2. Tryck  v  och flytta markören till det femte objektet nedan. Lägg märke
     till att texten markeras.

  3. Tryck  :  tecknet. Längst ned på skärmen visas  :'<,'>  .

  4. Skriv  w TEST  , där TEST är ett filnamn som inte finns ännu. Verifiera
     att du ser  :'<,'>w TEST  innan du trycker <ENTER>.

  5. Vim kommer att skriva de markerade raderna till filen TEST. Använd  :!ls
     eller  :!dir  för att se den. Ta inte bort den ännu! Vi kommer att
     använda den i nästa lektion.

NOTERA: Att trycka  v  startar Visuell markering. Du kan flytta markören runt
	för att göra markeringen större eller mindre. Sedan kan du använda en
	operator för att göra något med texten. Till exempel,  d  tar bort texten.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		   Lektion 1.5.4: HÄMTA OCH SAMMANFOGA FILER


       ** För att infoga innehållet av en fil, skriv  :r FILNAMN  **

  1. Placera markören precis ovanför denna rad.

NOTERA: Efter att ha kört Steg 2 kommer du att se text från Lektion 1.5.3.
	Flytta sedan NED för att se denna lektion igen.

  2. Hämta nu din TEST-fil med kommandot   :r TEST   där TEST är namnet på
     filen du använde.
     Filen du hämtar placeras under markörens rad.

  3. För att verifiera att en fil hämtades, flytta markören tillbaka och lägg
     märke till att det nu finns två kopior av Lektion 1.5.3, originalet och
     filversionen.

NOTERA: Du kan också läsa utdata från ett externt kommando. Till exempel,
	:r !ls  läser utdata från ls kommandot och lägger det under markören.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			       Lektion 1.5 SAMMANFATTNING


  1.  :!kommando  kör ett externt kommando.

      Några användbara exempel är:
	 (MS-DOS)	  (Unix)
	  :!dir		   :!ls		   -  visar en kataloglista.
	  :!del FILNAMN	   :!rm FILNAMN    -  tar bort filen FILNAMN.

  2.  :w FILNAMN  sparar nuvarande Vim-fil till disk med namnet FILNAMN.

  3.  v  rörelse  :w FILNAMN  sparar de Visuellt markerade raderna i filen
      FILNAMN.

  4.  :r FILNAMN  hämtar diskfilen FILNAMN och lägger den under markörens
      position.

  5.  :r !ls  läser utdata från ls kommandot och lägger det under markörens
      position.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			Lektion 1.6.1: ÖPPNA-KOMMANDOT


	   ** Skriv  o  för att öppna en rad under markören
	      och placera dig i Infogningsläge. **

  1. Flytta markören till raden nedan markerad med --->.

  2. Skriv det lilla  o  för att öppna en rad UNDER markören och placera dig
     i Infogningsläge.

  3. Kopiera nu raden markerad ---> och tryck <ESC> för att avsluta
     Infogningsläge.

---> När du har tryckt  o  placeras markören på den öppna raden i Infogningsläge.

  4. För att öppna en rad OVANFÖR markören, skriv helt enkelt ett stort  O ,
     istället för ett litet  o . Prova detta på raden nedan.

---> Öppna en rad ovanför denna genom att skriva O medan markören är på denna rad.




~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			Lektion 1.6.2: LÄGG TILL-KOMMANDOT


	    ** Skriv  a  för att infoga text EFTER markören. **

  1. Flytta markören till början av raden nedan markerad med --->.

  2. Tryck  e  tills markören är på slutet av  ra .

  3. Skriv ett  a  (litet) för att lägga till text EFTER markören.

  4. Slutför ordet som på raden under det. Tryck <ESC> för att avsluta
     Infogningsläge.

  5. Använd  e  för att flytta till nästa ofullständiga ord och upprepa
     steg 3 och 4.

---> Denna ra låter dig öv på att läg till te i en rad.
---> Denna rad låter dig öva på att lägga till text i en rad.

NOTERA: a, i och A går alla till samma Infogningsläge, den enda skillnaden är
	var tecknen infogas.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    Lektion 1.6.3: ETT ANNAT SÄTT ATT ERSÄTTA


      ** Skriv ett stort  R  för att ersätta fler än ett tecken. **

  1. Flytta markören till den första raden nedan markerad med --->. Flytta
     markören till början av det första  xxx .

  2. Tryck nu  R  och skriv numret nedan det på den andra raden, så att det
     ersätter xxx .

  3. Tryck <ESC> för att lämna Ersättningsläge. Lägg märke till att resten
     av raden förblir oförändrad.

  4. Upprepa stegen för att ersätta det återstående xxx .

---> Att lägga 123 till xxx ger dig xxx.
---> Att lägga 123 till 456 ger dig 579.

NOTERA: Ersättningsläge är som Infogningsläge, men varje skrivet tecken tar
	bort ett existerande tecken.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    Lektion 1.6.4: KOPIERA OCH KLISTRA IN TEXT


  ** Använd  y  operatorn för att kopiera text och  p  för att klistra in den **

  1. Gå till raden nedan markerad med ---> och placera markören efter "a)".

  2. Starta Visuellt läge med  v  och flytta markören till precis före "första".

  3. Skriv  y  för att kopiera (yank) den markerade texten.

  4. Flytta markören till slutet av nästa rad:  j$

  5. Skriv  p  för att klistra in texten. Skriv sedan:  ett andra <ESC> .

  6. Använd Visuellt läge för att markera " objekt.", kopiera det med  y ,
     flytta till slutet av nästa rad med  j$  och klistra in texten där med  p .

--->  a) detta är det första objektet.
      b)

NOTERA: du kan också använda  y  som en operator;  yw  kopierar ett ord.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    Lektion 1.6.5: STÄLLA IN ALTERNATIV


  ** Ställ in ett alternativ så sökningar och ersättningar ignorerar skiftläge **

  1. Sök efter 'ignorera' genom att skriva:   /ignorera <ENTER>
     Upprepa flera gånger genom att trycka  n  tangenten.

  2. Ställ in 'ic' (Ignorera skiftläge) alternativet genom att skriva:  :set ic

  3. Sök nu efter 'ignorera' igen genom att trycka  n
     Lägg märke till att Ignorera och IGNORERA nu också hittas.

  4. Ställ in 'hlsearch' och 'incsearch' alternativen:  :set hls is

  5. Skriv nu in sökkommandot igen och se vad som händer:  /ignorera <ENTER>

  6. För att inaktivera skiftlägeskänslighet, skriv:  :set noic

NOTERA: För att ta bort markeringen av träffar, skriv:   :nohlsearch
NOTERA: Om du vill ignorera skiftläge för bara en sökning, använd  \c
	i frasen:  /ignorera\c  <ENTER>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			       Lektion 1.6 SAMMANFATTNING

  1. Skriv  o  för att öppna en rad UNDER markören och starta Infogningsläge.
     Skriv  O  för att öppna en rad OVANFÖR markören.

  2. Skriv  a  för att infoga text EFTER markören.
     Skriv  A  för att infoga text efter slutet av raden.

  3. Det stora  e  kommandot flyttar till slutet av ett ord.

  4. Operatorn  y  kopierar text,  p  klistrar in den.

  5. Att skriva ett stort  R  startar Ersättningsläge tills  <ESC>  trycks.

  6. Att skriva ":set xxx" ställer in alternativet "xxx". Några alternativ är:
  	'ic' 'ignorecase'	ignorera versaler/gemener vid sökning
	'is' 'incsearch'	visa delmatchningar för en sökfras
	'hls' 'hlsearch'	markera alla matchande fraser
     Du kan använda antingen det långa eller korta alternativnamnet.

  7. Sätt "no" före ett alternativnamn för att stänga av det:  :set noic

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			      Lektion 1.7: ONLINE-HJÄLP


		       ** Använd online-hjälpsystemet **

  Vim har ett omfattande online-hjälpsystem. För att komma igång, prova en
  av dessa tre:
	- tryck <HJÄLP>-tangenten (om du har en)
	- tryck <F1>-tangenten (om du har en)
	- skriv   :help <ENTER>

  Läs texten i hjälpfönstret för att ta reda på hur hjälpen fungerar.
  Skriv  CTRL-W CTRL-W   för att hoppa från ett fönster till ett annat.
  Skriv    :q <ENTER>    för att stänga hjälpfönstret.

  Du kan hitta hjälp om nästan allt genom att ge ett argument till
  ":help"-kommandot.  Prova dessa (glöm inte att trycka <ENTER>):

	:help w
	:help c_CTRL-D
	:help insert-index
	:help user-manual
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			  Lektion 1.8: SKAPA ETT STARTSKRIPT

			  ** Aktivera Vim-funktioner **

  Vim har många fler funktioner än Vi, men de flesta är inaktiverade som
  standard. För att börja använda fler funktioner behöver du skapa en
  "vimrc"-fil.

  1. Börja redigera "vimrc"-filen. Detta beror på ditt system:
	:e ~/.vimrc		för Unix
	:e ~/_vimrc		för MS-Windows

  2. Läs nu in exempel-"vimrc"-filens innehåll:
	:r $VIMRUNTIME/vimrc_example.vim

  3. Skriv filen med:
	:w

  Nästa gång du startar Vim kommer den att använda syntaxmarkering.
  Du kan lägga till alla dina föredragna inställningar i denna "vimrc"-fil.
  För mer information, skriv  :help vimrc-intro

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			       Lektion 1.9: SLUTSATS

  Detta var tänkt att ge en kort översikt av Vim-redigeraren, precis tillräckligt
  för att du ska kunna använda redigeraren ganska enkelt. Det är långt ifrån
  komplett eftersom Vim har många många fler kommandon. Läs användarhandboken
  härnäst: ":help user-manual".

  För vidare läsning och studier rekommenderas denna bok:
	Vim - Vi Improved - av Steve Oualline
	Förläggare: New Riders
  Den första boken helt tillägnad Vim. Särskilt användbar för nybörjare.
  Det finns många exempel och bilder.
  Se https://iccf-holland.org/click5.html

  Denna äldre bok handlar mer om Vi än Vim, men rekommenderas också:
	Learning the Vi Editor - av Linda Lamb
	Förläggare: O'Reilly & Associates Inc.
  Det är en bra bok för att lära sig nästan allt du vill göra med Vi.
  Den sjätte upplagan inkluderar också information om Vim.

  Denna handledning skrevs av Michael C. Pierce och Robert K. Ware,
  Colorado School of Mines med idéer av Charles Smith,
  Colorado State University.  E-post: bware@mines.colorado.edu.

  Modifierad för Vim av Bram Moolenaar.
  Svensk översättning av Johan Svedberg och Daniel Nylander.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
