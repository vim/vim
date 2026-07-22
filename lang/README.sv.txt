README.txt för version 9.1 av Vim: Vi IMproved.


VAD ÄR VIM?

Vim är en kraftigt förbättrad version av den gamla goda UNIX-editorn Vi.  Många nya
funktioner har lagts till: ångra på flera nivåer, syntaxmarkering, kommandoradshistorik
historik, onlinehjälp, stavningskontroll, filnamns komplettering, blockoperationer,
skriptspråk etc.  Det finns också ett grafiskt användargränssnitt (GUI) tillgängligt.
Vi-kompatibiliteten bibehålls dock, så de som har Vi "i fingrarna" kommer
känna sig som hemma.  Se "runtime/doc/vi_diff.txt" för skillnader jämfört med Vi.

Denna editor är mycket användbar för att redigera program och andra vanliga textfiler.
Alla kommandon ges med vanliga tangentbordstecken, så de som kan skriva
med tio fingrar kan arbeta mycket snabbt.  Dessutom kan funktionsknapparna
mappas till kommandon av användaren, och musen kan användas.

Vim syftar också till att tillhandahålla en (mestadels) POSIX-kompatibel vi-implementering när
kompileras med en minimal uppsättning funktioner (vanligtvis kallad vim.tiny), som används
av många Linux-distributioner som standardvi-redigerare.

Vim körs under MS-Windows (7, 8, 10, 11), macOS, Haiku, VMS och nästan alla
varianter av UNIX.  Det bör inte vara särskilt svårt att porta till andra system.
Äldre versioner av Vim körs på MS-DOS, MS-Windows 95/98/Me/NT/2000/XP/Vista,
Amiga DOS, Atari MiNT, BeOS, RISC OS och OS/2.  Dessa underhålls inte längre.


DISTRIBUTION

Du kan ofta använda din favoritpakethanterare för att installera Vim.  På Mac och
Linux är en liten version av Vim förinstallerad, men du behöver ändå installera Vim
om du vill ha fler funktioner.

Det finns separata distributioner för Unix, PC, Amiga och vissa andra system.
Denna README.txt-fil medföljer runtime-arkivet.  Den innehåller
dokumentation, syntaxfiler och andra filer som används vid körning.  För att köra
Vim måste du skaffa antingen ett av binärarkiven eller ett källarkiv.
Vilket du behöver beror på vilket system du vill köra det på och om du
vill eller måste kompilera det själv.  Se "https://www.vim.org/download.php" för
en översikt över de distributioner som för närvarande finns tillgängliga.

Några populära ställen att hämta den senaste versionen av Vim:
* Kolla in git-arkivet från github: https://github.com/vim/vim.
* Hämta källkoden som ett arkiv: https://github.com/vim/vim/tags.
* Hämta en Windows-körbar fil från vim-win32-installer-arkivet:
  https://github.com/vim/vim-win32-installer/releases.


KOMPILERING

Om du har skaffat en binär distribution behöver du inte kompilera Vim.  Om du
har skaffat en källkodsdistribution finns allt du behöver för att kompilera Vim i
katalogen "src".  Se src/INSTALL för instruktioner.


INSTALLATION

Se någon av dessa filer för systemspecifika instruktioner.  Antingen i
READMEdir-katalogen (i arkivet) eller i toppkatalogen (om du packar upp en
arkiv):

README_ami.txt        Amiga
README_unix.txt       Unix
README_dos.txt        MS-DOS och MS-Windows
README_mac.txt        Macintosh
README_haiku.txt      Haiku
README_vms.txt        VMS

Det finns andra README_*.txt-filer, beroende på vilken distribution du använde.


DOKUMENTATION

Vim-tutorn är en timmes lång utbildningskurs för nybörjare.  Ofta kan den
startas som "vimtutor".  Se ":help tutor" för mer information.

Det bästa är att använda ":help" i Vim.  Om du ännu inte har en körbar fil, läs
"runtime/doc/help.txt".  Den innehåller hänvisningar till andra dokumentationsfiler.
Användarhandboken läses som en bok och rekommenderas för att lära sig använda Vim.  Se
":help user-manual".


KOPIERING

Vim är Charityware.  Du kan använda och kopiera det så mycket du vill, men du
uppmuntras att göra en donation för att hjälpa föräldralösa barn i Uganda.  Läs filen
"runtime/doc/uganda.txt" för mer information (skriv ":help uganda" i Vim).

Sammanfattning av licensen: Det finns inga begränsningar för användning eller distribution av en
oförändrad kopia av Vim.  Delar av Vim får också distribueras, men licenstexten
texten måste alltid inkluderas.  För modifierade versioner gäller några begränsningar.
Licensen är GPL-kompatibel, du kan kompilera Vim med GPL-bibliotek och
distribuera det.


SPONSRING

Att fixa buggar och lägga till nya funktioner tar mycket tid och ansträngning.  För att visa
din uppskattning för arbetet och motivera utvecklarna att fortsätta arbeta med
Vim, skicka gärna en donation.

Pengarna du donerar kommer huvudsakligen att användas för att hjälpa barn i Uganda.  Se
"runtime/doc/uganda.txt".  Men samtidigt ökar donationerna
utvecklingsteamets motivation att fortsätta arbeta med Vim!

För den senaste informationen om sponsring, se Vims webbplats:
	https://www.vim.org/sponsor/


BIDRA

Om du vill hjälpa till att förbättra Vim, se filen CONTRIBUTING.md.


INFORMATION

Om du använder macOS kan du använda MacVim: https://macvim.org

De senaste nyheterna om Vim finns på Vims hemsida:
	https://www.vim.org/

Om du har problem, ta en titt på Vims dokumentation eller tips:
	https://www.vim.org/docs.php
	https://vim.fandom.com/wiki/Vim_Tips_Wiki

Om du fortfarande har problem eller andra frågor, använd någon av mailinglistorna
för att diskutera dem med Vim-användare och utvecklare:
	https://www.vim.org/maillist.php

Om inget annat fungerar, rapportera buggar direkt till vim-dev-maillistan:
	<vim-dev@vim.org>


HUVUDFÖRFATTARE

Det mesta av Vim har skapats av Bram Moolenaar <Bram@vim.org>, ":help Bram-Moolenaar"

Skicka övriga kommentarer, patchar, blommor och förslag till vim-dev
: <vim-dev@vim.org>
