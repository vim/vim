README.txt per la versione 9.1 di Vim: VI Migliorato.


COS'È VIM?

Vim è una versione migliorata del classico programma di videoscrittura UNIX
Vi.  Molte nuove funzionalità sono state aggiunte: la possibilità di avere
multipli annullamenti di comando, l'evidenziazione sintattica, una storia dei
comandi immessi, file di aiuto facilmente consultabili, controlli ortografici,
completamento di nomi di file, operazioni su blocchi di dati, un linguaggio di
script, etc.  È anche disponibile una versione grafica (GUI).  Tuttavia è
possibile lavorare come se si stesse usando il Vi "classico".  Chi avesse Vi
"sulle dita" si troverà a suo agio.  Vedere il file "runtime/doc/vi_diff.txt"
[in inglese] per dettagli sulle differenze di Vim rispetto a Vi.

Questo editor è molto utile per modificare programmi e altri file di testo.
Tutti i comandi sono immessi usando i tasti presenti sulla tastiera, in modo
che chi è in grado di digitare usando tutte e dieci le dita può lavorare molto
velocemente.  Inoltre, i tasti funzione possono essere mappati per inserire
comandi dell'utente, ed è possibile usare il mouse.

Vim è disponibile in ambiente MS-Windows (7, 8, 10, 11), macOS, Haiku, VMS e
in quasi tutte le varianti di Unix.  L'adattamento a nuovi sistemi operativi
non dovrebbe essere molto difficile.
Precedenti versioni di Vim funzionano in ambiente MS-DOS, MS-Windows
95/98/Me/NT/2000/XP/Vista, Amiga DOS, Atari MiNT, BeOS, RISC OS e OS/2.
Tali versioni non sono più supportate.


DISTRIBUZIONE

Spesso è possibile usare il vostro Gestore applicazioni preferito per
installare Vim.  Negli ambienti Mac e Linux una versione base di Vim è inclusa
nel sistema operativo, ma può ancora essere necessario installare Vim se si
desiderano funzionalità ulteriori.

Ci sono distribuzioni separate di Vim per gli ambienti Unix, PC, Amiga e per
qualche altro sistema operativo.  Questo file README.txt è contenuto nelle
directory che contengono i file usati da Vim in fase di esecuzione.  Nelle
stesse directory sono presente la documentazione, i file usati per
l'evidenziazione sintattica e altri file usati da Vim in fase di esecuzione.
Per installare Vim occorre ottenere un archivio che contiene solo i file
eseguibili, o un archivio che permette di compilare Vim a partire dai file
sorgente.  Quale alternativa preferire dipende dal sistema su cui si vuole
usare Vim, e dal preferire (o meno) di compilarlo direttamente a partire dai
file sorgente.  Consultate "https://www.vim.org/download.php" per una
panoramica delle distribuzioni correntemente disponibili.

Alcuni siti da cui ottenere l'ultima versione di Vim:
* Consultare la repository git in github: https://github.com/vim/vim.
* Procurarsi il codice sorgente come archivio https://github.com/vim/vim/tags.
* Ottenere un file per installare Vim in ambiente Windows dalla repository
  vim-win32-installer:
  https://github.com/vim/vim-win32-installer/releases.


COMPILARE VIM

Se si è ottenuta una distribuzione di file eseguibili di Vim non è necessario
compilarlo.  Se si è ottenuta una distribuzione di file sorgente, tutto ciò
che serve per compilare Vim è nella directory "src".  Vedere src/INSTALL per
come procedere.


INSTALLAZIONE

Vedere uno dei file elencati più sotto per istruzioni riguardo a uno specifico
sistema operativo.  Tali file sono (nella repository git) nella directory
READMEdir oppure nella directory principale se si scompatta un archivio:

README_ami.txt		Amiga
README_unix.txt		Unix
README_dos.txt		MS-DOS e MS-Windows
README_mac.txt		Macintosh
README_haiku.txt	Haiku
README_vms.txt		VMS

Esistono altri file README_*.txt, a seconda della distribuzione in uso.


DOCUMENTAZIONE

Esiste un corso di introduzione a Vim per principianti, della durata di circa
un'ora.  Normalmente si può accedervi tramite il comando "vimtutor".  Vedere
":help tutor" per ulteriori informazioni.

Ma la cosa migliore è usare la documentazione disponibile in una sessione di
Vim, tramite il comando ":help".  Se ancora non si ha a disposizione Vim, si
può leggere il file "runtime/doc/help.txt".  Questo file contiene puntatori
agli altri file che costituiscono la documentazione.
All'interno della documentazione esiste anche uno User Manual (manuale utente)
che si legge come un libro ed è raccomandato per imparare a usare Vim.
Vedere ":help user-manual".  Il manuale utente è stato interamente tradotto in
italiano, ed è disponibile, vedere:
	https://www.vim.org/translations.php


COPIE

Vim è un Charityware (ossia eventuali offerte per il suo utilizzo vanno a
un'attività caritativa).  Vim può essere usato e copiato liberamente, senza
limitazioni, ma è incoraggiata un'offerta a favore di orfani ugandesi.  Si
prega di leggere il file "runtime/doc/uganda.txt" per dettagli su come fare
(il file si può visualizzare digitando ":help uganda" all'interno di Vim).

Sommario della licenza: Non ci sono restrizioni nell'uso e nella distribuzione
di una copia non modificata di Vim.  Parti di Vim possono anche essere
distribuite, ma il testo della licenza va sempre incluso.  Per versioni
modificate di Vim, valgono alcune limitazioni.  La licenza di Vim è
compatibile con la licenza GPL, è possibile compilare Vim utilizzando librerie
con licenza GPL e distribuirlo.


SPONSORIZZAZIONI

Correggere errori e aggiungere nuove funzionalità richiede tempo e fatica.
Per mostrare la vostra stima per quest'attività e per fornire motivazioni
agli sviluppatori perché continuino a lavorare su Vim, siete invitati a
fare una donazione.

Le somme donate saranno usate principalmente per aiutare dei bambini in
Uganda.  Vedere "runtime/doc/uganda.txt".  Allo stesso tempo, le donazioni
aumentano la motivazione del gruppo di sviluppo a continuare a lavorare su
Vim!

Informazioni più aggiornate sulla sponsorizzazione, possono essere trovate
sul sito Internet di Vim:
	https://www.vim.org/sponsor/


CONTRIBUIRE

Chi vuole contribuire a rendere Vim ancora migliore, può consultare
il file CONTRIBUTING.md (in inglese).


INFORMAZIONE

Se il vostro sistema operativo è macOS, potete usare MacVim:
	https://macvim.org

Le ultime notizie riguardo a Vim si possono trovare sulla pagina Internet di
Vim:
	https://www.vim.org/

Se avete problemi, è possibile consultare la documentazione Vim e i
suggerimenti su come utilizzarlo:
	https://www.vim.org/docs.php
	https://vim.fandom.com/wiki/Vim_Tips_Wiki

Se avete ancora problemi o qualsiasi altra richiesta, è possibile usare una
delle mailing list per discuterne con utenti e sviluppatori di Vim:
	https://www.vim.org/maillist.php

Se nient'altro funziona, potete riferire direttamente i problemi incontrati
alla mailing list degli sviluppatori, vim-dev:
	<vim-dev@vim.org>


AUTORE PRINCIPALE

La maggior parte di Vim è stata creata da Bram Moolenaar <Bram@vim.org>,
vedere ":help Bram-Moolenaar"

Spedire tutti gli altri commenti, modifiche al codice sorgente, fiori e
suggerimenti alla mailing list vim-dev:
	<vim-dev@vim.org>
