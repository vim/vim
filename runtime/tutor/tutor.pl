===============================================================================
=    W i t a j   w   t u t o r i a l u   V I M - a      -    Wersja  1.5.     =
===============================================================================

     Vim to potê¿ny edytor, który posiada wiele poleceñ, zbyt du¿o by
     wyja¶niæ je wszystkie w tym tutorialu. Ten przewodnik ma nauczyæ
     Ciê pos³ugiwaæ siê wystarczaj±co wieloma komendami by¶ móg³ ³atwo
     u¿ywaæ Vim-a jako edytora ogólnego przeznaczenia.

     Czas potrzebny na ukoñczenie tutoriala to 25 do 30 minut i zale¿y
     od tego jak wiele czasu spêdzisz na eksperymentowaniu.  Polecenia
     wykonywane w czasie lekcji zmodyfikuj± tekst. Zrób wcze¶niej kopiê
     tego pliku do æwiczeñ (je¶li zacz±³e¶ komend± "vimtutor" to ju¿
     pracujesz na kopii).

     Wa¿ne jest, by¶ pamiêta³, ¿e przewodnik ten zosta³ zaprojektowany do
     nauki poprzez æwiczenia. To oznacza, ¿e musisz wykonywaæ polecenia
     by nauczyæ siê ich prawid³owo. Je¶li bêdziesz jedynie czyta³ tekst
     szybko zapomnisz wiele poleceñ!

     Teraz upewnij siê, ¿e nie masz wci¶niêtego CapsLocka i wciskaj  j
     tak d³ugo dopóki Lekcja 1.1. nie wype³ni ca³kowicie ekranu.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      Lekcja 1.1.: PORUSZANIE SIÊ KURSOREM

       ** By wykonaæ ruch kursorem, wci¶nij h, j, k, l jak pokazano. **

	       ^
	       k		      Wskazówka:  h jest po lewej
	  < h	  l >				  l jest po prawej
	       j				  j wygl±da jak strza³ka w dó³
	       v
  1. Poruszaj kursorem dopóki nie bêdziesz pewien, ¿e pamiêtasz polecenia.

  2. Trzymaj  j  tak d³ugo a¿ bêdzie siê powtarza³.
---> Teraz wiesz jak doj¶æ do nastêpnej lekcji.

  3. U¿ywaj±c strza³ki w dó³ przejd¼ do nastêpnej lekcji.

Uwaga: Je¶li nie jeste¶ pewien czego¶ co wpisa³e¶, wci¶nij <ESC> by wróciæ do
       trybu Normal. Wtedy powtórz polecenie.

Uwaga: Klawisze kursora tak¿e powinny dzia³aæ, ale u¿ywaj±c  hjkl  bêdziesz
       w stanie poruszaæ siê o wiele szybciej jak siê tylko przyzwyczaisz.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		 Lekcja 1.2.: WCHODZENIE I WYCHODZENIE Z VIM-a

 !! UWAGA: Przed wykonaniem jakiegokolwiek polecenia przeczytaj ca³± lekcjê.!!

  1. Wci¶nij <ESC> (aby upewniæ siê, ¿e jeste¶ w trybie Normal).
  2. Wpisz:			:q!<ENTER>.

---> To spowoduje wyj¶cie z edytora BEZ zapamiêtywania zmian jakie
     zd±¿y³e¶ zrobiæ. Je¶li chcesz zapamiêtaæ zmiany i wyj¶æ
     wpisz:			:wq<ENTER>

  3. Kiedy widzisz znak zachêty pow³oki wpisz komendê, ¿eby wróciæ
     do tutoriala.
     Powinienie¶ wpisaæ:	vimtutor<ENTER>
     Normalnie u¿y³by¶:		vim tutor<ENTER>

---> 'vim' oznacza edytor vim, 'tutor' jest plikem, który chcia³by¶
     edytowaæ.

  4. Je¶li chcesz zapamiêtaæ polecenia, wykonaj kroki 1. do 3. aby
     wyj¶æ i wróciæ do edytora. Potem przenie¶ siê do Lekcji 1.3.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    Lekcja 1.3.: EDYCJA TEKSTU - KASOWANIE


	** W trybie Normal wci¶nij x aby usun±æ znak pod kursorem. **

  1. Przenie¶ kursor do linii poni¿ej oznaczonej --->.

  2. By poprawiæ b³êdy, naprowad¼ kursor na znak do usuniêcia.

  3. Wci¶nij  x  aby usun±æ niechciany znak.

  4. Powtarzaj kroki 2. do 4. dopóki zdanie nie jest poprawne.

---> Kkrowa prrzeskoczy³a prrzez ksiiê¿ycc.

  5. Teraz kiedy zdanie jest poprawione przejd¼ do Lekcji 1.4.

UWAGA: Ucz siê przez æwiczenie, nie wkuwanie.




~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	       Lekcja 1.4.: EDYCJA TEKSTU - INSERT (wprowadzanie)


	      ** W trybie Normal wci¶nij  i  aby wstawiæ tekst. **

  1. Przenie¶ kursor do pierwszej linii poni¿ej oznaczonej --->.

  2. Aby poprawiæ pierwszy wiersz, ustaw kursor na pierwszym znaku PO tym
     gdzie tekst ma byæ wstawiony.

  3. Wci¶nij  i  a nastêpnie wpisz konieczne poprawki.

  4. Po poprawieniu b³êdu wci¶nij <ESC> by wróciæ do trybu Normal.
     Powtarzaj kroki 2. do 4. aby poprawiæ ca³e zdanie.

---> W tej brkje trochê .
---> W tej linii brakuje trochê tekstu.

  5. Kiedy czujesz siê swobodnie wstawiaj±c tekst przejd¼ do
     podsumowania poni¿ej.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			     LEKCJA 1. PODSUMOWANIE

  1. Poruszasz kursorem u¿ywaj±c "strza³ek" i klawiszy  hjkl .
       h (w lewo)	 j (w dó³)	 k (do góry)		l (w prawo)

  2. By wej¶æ do Vim-a (od znaku zachêty) wpisz:
			    vim NAZWA_PLIKU<ENTER>

  3. By wyj¶æ z Vim-a wpisz:
			    <ESC> :q!<ENTER>  by usun±c wszystkie zmiany.
	     LUB:	    <ESC> :wq<ENTER>  by zmiany zachowaæ.

  4. By usun±æ znak pod kursorem w trybie Normal:  x

  5. By wstawiæ tekst przed kursorem w trybie Normal:
			    i	  type in text	      <ESC>

UWAGA: Wci¶niêcie <ESC> przeniesie Ciê z powrotem do trybu Normal
lub odwo³a niechciane lub czê¶ciowo wprowadzone polecenia.

Teraz mo¿emy kontynuowaæ i przej¶æ do Lekcji 2.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		  Lekcja 2.1.: POLECENIE DELETE (usuwanie)


	       ** Wpisz  dw  by usun±c tekst do koñca wyrazu. **

  1. Wci¶nij  <ESC>  by upewniæ siê, ¿e jeste¶ w trybie Normal.

  2. Przenie¶ kursor do linii poni¿ej oznaczonej --->.

  3. Przesuñ kursor na pocz±tek wyrazu, które chcesz usun±æ.

  4. Wpisz   dw   by usun±c wyraz.

  UWAGA: Litery  dw  bêd± siê pojawiaæ na dole ekranu w miarê
	 wpisywania.  Je¶li wpisa³e¶ co¶ ¼le wci¶nij <ESC> i zacznij od
	 pocz±tku.

---> Jest tu parê papier wyrazów, które kamieñ nie nale¿± do no¿yce tego zdania.

  5. Powtarzaj kroki 3. i 4. dopóki zdanie nie bêdzie poprawne, potem
  przejd¼ do Lekcji 2.2.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    Lekcja 2.2.: WIÊCEJ POLECEÑ USUWAJ¡CYCH


	      ** Wpisz	d$  aby usun±æ tekst do koñca linii. **

  1. Wci¶nij  <ESC>  aby siê upewniæ, ¿e jeste¶ w trybie Normal.

  2. Przenie¶ kursor do linii poni¿ej oznaczonej --->.

  3. Przenie¶ kursor do koñca poprawnego zdania (PO pierwszej  . ).

  4. Wpisz  d$  aby usun±æ resztê linii.

---> Kto¶ wpisa³ koniec tego zdania dwukrotnie. zdania dwukrotnie.


  5. Przejd¼ do Lekcji 2.3. by zrozumieæ co siê sta³o.





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lekcja 2.3.: O POLECENIACH I OBIEKTACH


  Format dla polecenia usuwaj±cego  d  jest taki:

	    [liczba]  d  obiekt      LUB      d  [liczba]  obiekt

  Gdzie:
   liczba - ile razy polecenie ma byæ wykonane (opcjonalne, domy¶lna=1).
   d      - polecenie usuwaj±ce.
   obiekt - na czym polecenie bêdzie wykonywane (lista poni¿ej).

  Krótka lista obiektów:
    w - od kursora do koñca wyrazu w³±czaj±c spacjê.
    e - od kursora do koñca wyrazu NIE w³±czaj±c spacji.
    $ - od kursora do koñca linii.

UWAGA: Dla ciekawskich, wybieranie obiektu w trybie Normal bez polecania
       polecenia przeniesie kursor tak jak opisano w li¶cie obiektów.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		   Lekcja 2.4.: WYJ¡TEK DO 'POLECENIE-OBIEKT'


		    ** Wpisz  dd   by usun±c ca³± liniê. **

  Z powodu czêsto¶ci usuwania ca³ych linii, projektanci Vim-a zdecydowali, ¿e
  bêdzie ³atwiej wpisaæ dwa razy pod rz±d d aby usun±æ liniê.

  1. Przenie¶ kursor do zdania poni¿ej.
  2. Wpisz  dd  aby usun±c wiersz.
  3. Teraz przenie¶ siê do czwartego wiersza.
  4. Wpisz  2dd  (pamiêtaj  liczba-polecenie-obiekt) aby usun±c dwia wiersze.

      1)  Ró¿e s± czerwone,
      2)  B³oto jest fajne,
      3)  Fio³ki s± niebieskie,
      4)  Mam samochód,
      5)  Zegar podaje czas,
      6)  Cukier jest s³odki,
      7)  I ty te¿.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      Lekcja 2.5.: POLECENIE UNDO (cofnij)


	  ** Wci¶nij  u  aby cofn±æ skutki ostatniego polecenia.
		 U za¶, by cofn±æ skutki dla ca³ej linii. **

  1. Przenie¶ kursor do zdania poni¿ej oznaczonego ---> i umie¶æ go na
     pierwszym b³êdzie.
  2. Wpisz  x  aby usun±æ pierwszy niechciany znak.
  3. Teraz wci¶nij  u  aby cofn±æ skutki ostatniego polecenia.
  4. Tym razem popraw wszystkie b³êdy w linii u¿ywaj±c polecenia  x .
  5. Teraz wci¶nij wielkie U aby przywróciæ liniê do oryginalnego stanu.
  6. Teraz wci¶nij  u  kilka razy by cofn±æ  U  i poprzednie polecenia.
  7. Teraz wpsz CTRL-R (trzymaj równocze¶nie wci¶niête klawisze CTRL i R)
     kilka razy, by cofn±æ cofniêcia.

---> Poopraw blêdyyy w teej liniii i zaamiieñ je prrzez coofnij.

  8. To s± bardzo po¿yteczne polecenia.

     Przejd¼ teraz do podsumowania Lekcji 2.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			     LEKCJA 2. PODSUMOWANIE

  1. By usun±æ znaki od kursora do koñca wyrazu wpisz:   dw

  2. By usun±æ znaki od kursora do koñca linii wpisz:    d$

  3. By usun±æ ca³± liniê:    dd

  4. Format polecenia w trybie Normal:

    [liczba]  polecenie  obiekt      LUB      polecenie  [liczba]  obiekt

  Gdzie:
   liczba    - ile razy polecenie ma byæ wykonane
   polecenie - to co trzeba zrobiæ (np.  d  dla usuwania)
   obiekt    - na czym polecenie bêdzie wykonywane, takie jak  w  (wyraz),
	       $  (do koñca linii), etc.

  5. By cofn±æ poprzednie polecenie, wpisz:	  u (ma³e u)
     By cofn±æ wszystkie zmiany w linii wpisz:	  U (wielkie U)
     By cofn±æ cofniêcia wpisz:			  CTRL-R

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 Lekcja 3.1.: POLECENIE PUT (wstaw)


	  ** Wpisz  p  by wstawiæ ostatnie usuniêcia po kursorze. **

  1. Przenie¶ kursor do pierwszej linii w zestawie poni¿ej.

  2. Wpisz  dd  aby usun±æ liniê i przechowaæ j± w buforze Vim-a.

  3. Przenie¶ kursor do linii POWY¯EJ tej gdzie usuniêta linia powinna
     siê znajdowaæ.

  4. W trybie Normal, wci¶nij  p  by wstawiæ liniê.

  5. Powtaj kroki 2. do 4. a¿ znajd± siê w odpowiednim porz±dku.

     d) Jak dwa anio³ki.
     b) Na dole fio³ki,
     c) A my siê kochamy,
     a) Na górze ró¿e,


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    Lekcja 3.2.: POLECENIE REPLACE (zast±p)


       ** Wpisz  r  a nastêpnie znak by zast±piæ znak pod kursorem. **

  1. Przenie¶ kursor do pierwszej linii poni¿ej oznaczonej --->

  2. Ustaw kursor na pierwszym b³êdzie.

  3. Wpisz  r  a potem znak jaki powinien zast±piæ b³êdny.

  4. Powtarzaj kroki 2. i 3. dopóki pierwsza linia nie bêdzie poprawna.

--->  Kjedy ten wiersz bi³ wstókiwany kto¶ wcizn±³ perê z³ych klawirzy!
--->  Kiedy ten wiersz by³ wstukiwany kto¶ wcisn±³ parê z³ych klawiszy!

  5. Teraz czas na Lekcjê 3.3.


UWAGA: Pamiêtaj by uczyæ siê æwicz±c, a nie pamiêciowo.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lekcja 3.3.: POLECENIE CHANGE (zmieñ)

     ** By zmieniæ ca³e s³owo lub jego czê¶æ wpisz  cw  (change word). **

  1. Przenie¶ kursor do pierwszej linii poni¿ej oznaczonej --->.

  2. Umie¶æ kursor na  u  w lunos.

  3. Wpisz  cw  i popraw wyraz (w tym wypadku wstaw 'inia').

  4. Wci¶nij <ESC> i przejd¼ do nastêpnego b³êdu (pierwszy znak, który ma
     ulec zmianie).

  5. Powtarzaj kroki 3. i 4. dopóki pierwsze zdanie nie bêdzie takie same
     jak drugie.

---> Ta lunos ma pire s³ów, które t¿ina zbnic u¿ifajonc pcmazu zmieñ.
---> Ta linia ma parê s³ów, które trzeba zmieniæ u¿ywaj±c polecenia zmieñ.

  Zauwa¿, ¿e  cw  nie tylko zamienia wyraz, ale tak¿e zmienia tryb na
  Insert (wprowadzanie).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      Lekcja 3.4.: WIÊCEJ ZMIAN U¯YWAJ¡C c


       ** Polecenie change u¿ywa takich samych obiektów jak delete. **

  1. Polecenie change dzia³a tak samo jak delete. Format wygl±da tak:

	    [liczba]  c  obiekt      LUB      c  [liczba]  obiekt

  2. Obiekty s± tak¿e takie same, np.:  w  (wyraz),  $  (koniec linii), etc.

  3. Przenie¶ siê do pierwszej linii poni¿ej oznaczonej --->

  4. Ustaw kursor na pierwszym b³êdzie.

  5. Wpisz  c$ , popraw koniec wiersza i wci¶nij <ESC>.

---> Koniec tego wiersza musi byæ poprawiony aby wygl±dal tak jak drugi.
---> Koniec tego wiersza musi byæ poprawiony u¿ywaj±c polecenia  c$ .



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			     LEKCJA 3. PODSUMOWANIE


  1. Aby wstawiæ tekst, który zosta³ wcze¶niej usuniêty wci¶nij  p . To
     polecenie wstawia skasowany tekst PO kursorze (je¶li ca³a linia
     zosta³a usuniêta, zostanie ona umieszczona w linii poni¿ej kursora).

  2. By zamieniæ znak pod kursorem wci¶nij  r  a potem znak, który ma zast±piæ
     oryginalny.

  3. Polecenie change pozwala Ci na zast±pienie wyszczególnionego obiektu
     od kursora do koñca obiektu. Np. wpisz  cw  aby zamieniæ tekst od
     kursora do koñca wyrazu,  c$  aby zmieniæ tekst do koñca linii.

  4. Format do polecenia change (zmieñ):

	    [liczba]  c  obiekt      LUB      c  [liczba]  obiekt

     Teraz przejd¼ do nastêpnej lekcji.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		   Lekcja 4.1.: LOKALIZACJA ORAZ STATUS PLIKU

       ** Naci¶nij CTRL-G aby zobaczyæ swoje po³o¿enie w pliku i status
	  pliku. Naci¶nij SHIFT-G aby przej¶æ do linii w pliku. **

  UWAGA: Przeczytaj ca³± lekcjê zanim wykonasz jakie¶ polecenia!!!

  1. Przytrzymaj klawisz CTRL i wci¶nij  g . Na dole strony pojawi siê pasek
     statusu z nazw± pliku i numerem linii, w której jeste¶. Zapamiêtaj numer
     linii dla potrzeb kroku 3.

  2. Wci¶nij SHIFT-G aby przej¶æ na koniec pliku.

  3. Wpisz numer linii, w której by³e¶ a potem SHIFT-G.  To przeniesie Ciê
     z powrotem do linii, w której by³e¶ kiedy wcisn±³e¶ CTRL-G (kiedy
     wpisujesz numery NIE pojawiaj± siê one na ekranie).

  4. Je¶li czujesz siê wystarczaj±co pewnie, wykonaj kroki 1-3.




~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 Lekcja 4.2.: POLECENIE SZUKAJ


	     ** Wpisz  /  a nastêpnie wyra¿enie aby je znale¼æ. **

  1. W trybie Normal wpisz  / . Zauwa¿, ¿e znak ten, oraz kursor pojawi±
     siê na dole ekranu tak samo jak polecenie  : .

  2. Teraz wpisz  b³ond<ENTER> .  To jest s³owo, którego chcesz szukaæ.

  3. By szukaæ tej samej frazy ponownie, po prostu wci¶nij  n .
     Aby szukaæ tej frazy w przeciwnym, kierunku wci¶nij SHIFT-N.

  4. Je¶li chcesz szukaæ frazy do ty³u, u¿yj polecenia  ?  zamiast  / .

---> Kiedy polecenie 'szukaj' osi±gnie koniec pliku, przeszukiwanie
     zacznie siê od pocz±tku pliku.

  'b³ond' to nie jest metoda by przeliterowaæ b³±d; 'b³ond' to b³±d.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		Lekcja 4.3.: W POSZUKIWANIU PARUJ¡CYCH NAWIASÓW


		** Wpisz  %  by znale¼æ pasuj±cy ),], lub } . **

  1. Umie¶æ kursor na którym¶ z (, [, lub { w linii poni¿ej oznaczonej --->.

  2. Teraz wpisz znak  % .

  3. Kursor powinien siê znale¼æ na paruj±cym nawiasie.

  4. Wci¶nij  %  aby przenie¶æ kursor z powrotem do paruj±cego nawiasu.

---> To ( jest linia testowa z (, [, ] i {, } . ))

UWAGA: Ta funkcja jest bardzo u¿yteczna w debuggowaniu programu
       z niesparowanymi nawiasami!





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		  Lekcja 4.4.: INNA METODA POPRAWIANIA B£ÊDÓW


	 ** Wpisz  :s/stary/nowy/g  aby zamieniæ 'stary' na 'nowy'. **

  1. Przenie¶ kursor do linii poni¿ej oznaczonej --->.

  2. Wpisz  :s/czaas/czas<ENTER> .  Zauwa¿, ¿e to polecenie zmienia
     tylko pierwsze wyst±pienie 'czaas' w linii.

  3. Teraz wpisz  :s/czaas/czas/g  oznacza zamianê (substytucjê)
     globalnie w ca³ej linii.  Zmienia wszystkie wyst±pienia w linii.

---> Najlepszy czaas na zobaczenie naj³adniejszych kwiatów to czaas wiosny.

  4. Aby zmieniæ wszystkie wyst±pienia ³añcucha znaków pomiêdzy dwoma liniami,
     wpisz: :#,#s/stare/nowe/g gdzie #,# s± numerami dwóch linii.
     Wpisz  :%s/stare/nowe/g   by zmieniæ wszystkie wyst±pienia w ca³ym pliku.




~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			     LEKCJA 4. PODSUMOWANIE


  1. CTRL-G  poka¿e Twoj± pozycjê w pliku i status pliku.  SHIFT-G przenosi
     Ciê do koñca pliku.  SHIFT-G poprzedzony liczb± przenosi Ciê do linii
     o tym numerze.

  2. Wpisanie  /  a nastêpnie ³añcucha znaków szuka ³añcucha DO PRZODU.
     Wpisanie  ?  a nastêpnie ³añcucha znaków szuka ³añcucha DO TY£U.
     Po wyszukiwaniu wci¶nij  n  by znale¼æ nastêpne wyst±pienie szukanej
     frazy tym samym kierunku lub Shift-N by szukaæ w kierunku przeciwnym.

  3. Wpisanie  %  gdy kursor znajduje siê na (,),[,],{, lub } lokalizuje
     paruj±cy znak.

  4. By zamieniæ pierwszy stary na nowy w linii wpisz      :s/stary/nowy
     By zamieniæ wszystkie stary na nowy w linii wpisz     :s/stary/nowy/g
     By zamieniæ frazy pomiêdzy dwoma liniami # wpisz      :#,#s/stary/nowy/g
     By zamieniæ wszystkie wyst±pienia w pliku wpisz       :%s/stary/nowy/g
     By Vim prosi³ Ciê o potwierdzienie dodaj 'c'	   :%s/stary/nowy/gc


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		 Lekcja 5.1.: JAK WYKONAÆ POLECENIA ZEWNÊTRZNE


	** Wpisz  :!  a nastêpnie zewnêtrzne polecenie by je wykonaæ. **

  1. Wpisz znajome polecenie  :  by ustawiæ kursor na dole ekranu. To pozwala
     na wprowadzenie polecenia.

  2. Teraz wstaw  !  (wykrzyknik). To umo¿liwi Ci wykonanie dowolnego
     zewnêtrznego polecenia pow³oki.

  3. Jako przyk³ad wpisz  ls  za  !  a nastêpnie wci¶nij <ENTER>. To polecenie
     poka¿e spis plików w Twoim katalogu, tak jakby¶ by³ przy znaku zachêty
     pow³oki. Mo¿esz te¿ u¿yæ  :!dir  je¶li  ls  nie dzia³a.

---> Uwaga:  W ten sposób mo¿na wykonaæ wszystkie polecenia pow³oki.
---> Uwaga:  Wszystkie polecenia  :  musz± byæ zakoñczone <ENTER>.





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    Lekcja 5.2.: WIÊCEJ O ZAPISYWANIU PLIKÓW


	    ** By zachowaæ zmiany w pliku wpisz :w NAZWA_PLIKU . **

  1. Wpisz  :!dir  lub  :!ls  by zobaczyæ spis plików w katalogu.
     Ju¿ wiesz, ¿e musisz wcisn±æ <ENTER> po tym.

  2. Wybierz nazwê pliku jaka jeszcze nie istnieje, np. TEST.

  3. Teraz wpisz:   :w TEST   (gdzie TEST jest nazw± pliku jak± wybra³e¶.)

  4. To polecenie zapamiêta ca³y plik (Vim Tutor) pod nazw± TEST.
     By to sprawdziæ wpisz  :!dir , ¿eby znowu zobaczyæ listê plików.

---> Zauwa¿, ¿e gdyby¶ teraz wyszed³ z Vim-a, a nastêpnie wszed³ ponownie
     komend±  vim TEST , plik by³by dok³adn± kopi± tutoriala kiedy go
     zapisywa³e¶.

  5. Teraz usuñ plik wpisuj±c:		   :!rm TEST


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		Lekcja 5.3.: SELEKTYWNE POLECENIE WRITE (zapisz)


	    ** By zachowaæ czê¶æ pliku wpisz  :#,# w NAZWA_PLIKU **

  1. Jeszcze raz wpisz  :!dir  lub  :!ls  by uzyskaæ listê plików
     w katalogu i wybierz odpowiedni± nazwê tak± jak TEST.

  2. Przenie¶ kursor na góre tej strony i wci¶nij CTRL-G by uzyskaæ
     numer linii. ZAPAMIÊTAJ TÊ LICZBÊ!

  3. Teraz przenie¶ siê na dó³ strony i wpisz  CTRL-G znowu.  ZAPAMIÊTAJ
     NUMER TAK¯E TEJ LINII!

  4. By zachowaæ JEDYNIE czê¶æ pliku wpisz  :#,# w TEST   gdzie  #,# to
     dwie liczby jakie zapamiêta³e¶ (góra, dó³ ekranu), a TEST to nazwa
     Twojego pliku.

  5. Ponownie sprawd¼ czy ten plik tam jest ( :!dir ), ale NIE usuwaj go.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		   Lekcja 5.4.: WSTAWIANIE I £¡CZENIE PLIKÓW


	    ** By wstawiæ zawarto¶æ pliku wpisz   :r NAZWA_PLIKU **

  1. Wpisz  :!dir  by siê upewniæ, ze Twój plik TEST zosta³ poprawnie
     zapamiêtany.

  2. Umie¶æ kursor na górze strony.

UWAGA: Po wykonaniu kroku 3. ponownie zobaczysz Lekcjê 5.3. Potem przejd¼
       do DO£U by zobaczyæ ponownie tê lekcjê.

  3. Teraz wczytaj plik TEST u¿ywaj±c polecenia  :r TEST , gdzie TEST
     jest nazw± pliku.

UWAGA: Plik, który wczytujesz jest wstawiany tam gdzie by³ kursor.

  4. By sprawdziæ czy plik zosta³ wczytany cofnij kursor i zobacz, ¿e
     teraz s± dwie kopie Lekcji 5.3., orygina³ i kopia z pliku.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			     LEKCJA 5. PODSUMOWANIE


  1.  :!polecenie wykonuje polecenie zewnêtrzne.

      U¿ytecznymi przyk³adami s±:

	  :!dir  -  pokazuje spis plików w katalogu.

	  :!rm NAZWA_PLIKU  -  usuwa plik NAZWA_PLIKU.

  2.  :w NAZWA_PLIKU  zapisuje obecny plik Vim-a na dysk z nazw± NAZWA_PLIKU.

  3.  :#,#w NAZWA_PLIKU  zapisuje linie od # do # w pliku NAZWA_PLIKU.

  4.  :r NAZWA_PLIKU  wczytuje z dysku plik NAZWA_PLIKU i wstawia go do
      bie¿±cego pliku po kursorze.





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      Lekcja 6.1.: POLECENIE OPEN (otwórz)


      ** Wpisz  o  by otworzyæ liniê poni¿ej kursora i przenie¶æ siê do
	 trybu Insert (wprowadzanie). **

  1. Przenie¶ kursor do linii poni¿ej oznaczonej --->.

  2. Wpisz  o  (ma³e) by otworzyæ liniê PONI¯EJ kursora i przenie¶æ siê
     do trybu Insert (wprowadzanie).

  3. Teraz przepisz liniê oznaczon± ---> i wci¶nij <ESC> by wyj¶æ
     z trybu Insert (wprowadzanie).

---> Po wci¶niêciu  o  kursor znajdzie siê w otwartej linii w trybie
     Insert (wprowadzanie).

  4. By otworzyæ liniê POWY¯EJ kursora wci¶nij wielkie  O  zamiast ma³ego
     o . Wypróbuj to na linii poni¿ej.

 Otwórz liniê powy¿ej wciskaj±c SHIFT-O gdy kursor bêdzie na tej linii.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     Lekcja 6.2.: POLECENIE APPEND (dodaj)


		  ** Wpisz  a  by dodaæ tekst ZA kursorem. **

  1. Przenie¶ kursor do koñca pierwszej linii poni¿ej oznaczonej --->
     Zrób to wciskaj±c  $  w trybie Normal.

  2. Wpisz  a  (ma³e) aby dodaæ tekst ZA znakiem pod kursorem.
     Wielkie A dodaje tekst na koñcu linii.

Uwaga: To oszczêdza wpisania: ostatni znak ( $ ),  i , tekst do dodania,
       <ESC>, strza³ka w prawo i ostatecznie  x , tylko po to by dodaæ
       tekst na koñcu linii.

  3. Teraz dokoñcz pierwsz± liniê. Zauwa¿ tak¿e, ¿e  append  (dodaj)
     dzia³a tak samo jak tryb Insert (wprowadzanie) z wyj±tkiem tego gdzie
     tekst jest wstawiany.

---> Ta linia pozwoli Ci æwiczyæ
---> Ta linia pozwoli Ci æwiczyæ dodawanie tekstu do koñca linii.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		   Lekcja 6.3.: INNA WERSJA REPLACE (zamiana)


	   ** Wpisz wielkie  R  by zamieniæ wiêcej ni¿ jeden znak. **

  1. Przenie¶ kursor do pierwszej linii poni¿ej oznaczonej --->.

  2. Umie¶æ kursor na pocz±tku pierwszego wyrazu, który rózni siê od
     drugiej linii oznaczonej ---> (wyraz 'ostatni').

  3. Teraz wpisz  R  i zamieñ resztê tekstu w pierwszej linii przez
     nadpisanie nad starym tekstem tak aby pierwsza linia brzmia³a tak samo
     jak druga.

---> To make the first line the same as the last on this page use the keys.
---> To make the first line the same as the second, type R and the new text.

  4. Zauwa¿, ¿e kiedy wci¶niesz <ESC> aby wyj¶æ niezmieniony tekst
     pozostaje.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 Lekcja 6.4.: USTAWIANIE OPCJI


** Ustawianie opcji tak by szukaj lub substytucja ignorowa³y wielko¶æ liter **

  1. Szukaj 'ignore' wpisuj±c (w trybie Normal):
     /ignore
     Powtórz szukanie kilka razy naciskaj±c klawisz  n .

  2. Ustaw opcjê 'ic' (Ignore case -- ignoruj wielko¶æ liter) poprzez
     wpisanie:		:set ic

  3. Teraz szukaj 'ignore' ponownie wciskuj±c: n
     Powtórz szukanie kilka razy naciskaj±c klawisz  n .

  4. Ustaw opcje 'hlsearch' i 'incsearch':
     :set hls is

  5. Teraz wprowad¼ polecenie szukaj ponownie i zobacz co siê zdarzy:
     /ignore


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			     LEKCJA 6. PODSUMOWANIE


  1. Wpisanie  o  otwiera liniê PONI¯EJ kursora i umieszcza kursor
     w otwartej linii w trybie Insert (wprowadzanie).
     Wpisanie wielkiego  O  otwiera liniê POWY¯EJ linii, w której
     znajduje siê kursor.

  2. Wpisz  a  by wstawiæ tekst ZA znakiem na, którym jest kursor.
     Wpisanie wielkiego  A  automatycznie dodaje tekst na koñcu linii.

  3. Wpisanie wielkiego  R  wprowadza w tryb Replace (zamiana) dopóki
     nie zostanie wci¶niêty <ESC>.

  4. Wpisanie ":set xxx" ustawia opcjê "xxx".







~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		       LEKCJA 7. POLECENIA POMOCY ON-LINE


		      ** U¿ycie systemu pomocy on-line **

  Vim posiada bardzo dobry system pomocy on-line. By zacz±æ spróbuj jednej
  z trzech mo¿liwo¶ci:

	- wci¶nij klawisz <HELP> (je¶li takowy posiadasz)
	- wci¶nij klawisz <F1> (je¶li takowy posiadasz)
	- wpisz   :help<ENTER>

  Wpisz   :q<ENTER>   by zamkn±c okno pomocy.

  Mo¿esz te¿ znale¼æ pomoc na ka¿dy temat podaj±c argument polecenia ":help".
  Spróbuj tych (nie zapomnij wcisn±æ <ENTER>):

  :help w
  :help c_<T
  :help insert-index
  :help user-manual

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LEKCJA 8. TWORZENIE SKRYPTU STARTOWEGO

			** W³±czanie mo¿liwo¶ci Vim-a **

  Vim ma o wiele wiêcej mo¿liwo¶ci ni¿ Vi, ale wiêkszo¶æ z nich jest domy¶lnie
  wy³±czona. Je¶li chcesz w³±czyæ te mo¿liwo¶ci na starcie musisz utworzyæ
  plik "vimrc".

  1. Pocz±tek edycji pliku "vimrc" zale¿y od Twojego systemu:
     :edit ~/.vimrc	     dla Unixa
     :edit $VIM/_vimrc       dla MS-Windows

  2. Teraz wczytaj przyk³adowy plik "vimrc":
     :read $VIMRUNTIME/vimrc_example.vim

  3. Zapisz plik:
     :write

  Nastêpnym razem gdy zaczniesz pracê w Vimie bêdzie on u¿ywaæ pod¶wietlania
  sk³adni. Mo¿esz dodaæ wszystkie swoje ulubione ustawienia do tego pliku
  "vimrc".

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Tutaj siê koñczy tutorial Vim-a. Zosta³ on pomy¶lany tak aby daæ krótki
  przegl±d jego mo¿liwo¶ci, wystarczaj±cy by¶ móg³ go u¿ywaæ. Jest on
  daleki od kompletno¶ci poniewa¿ Vim ma o wiele, wiele wiêcej poleceñ.

  Dla dalszej nauki rekomendujemy ksi±¿kê:
	Vim - Vi Improved - autor Steve Oualline
	Wydawca: New Riders
  Pierwsza ksi±zka ca³kowicie po¶wiêcona Vim-owi. U¿yteczna zw³aszcza dla
  pocz±tkuj±cych. Zawiera wiele przyk³adów i ilustracji.
  Zobacz http://iccf-holland.org./click5.html

  Ta ksi±¿ka jest starsza i bardziej o Vi ni¿ o Vim-ie, ale tak¿e warta
  polecenia:
	Learning the Vi Editor - autor Linda Lamb
	Wydawca: O'Reilly & Associates Inc.
  To dobra ksi±¿ka by dowiedzieæ siê niemal wszystkiego co chcia³by¶ zrobiæ
  z Vi. Szósta edycja zawiera te¿ informacje o Vim-ie.

  Po polsku wydano:
	Edytor vi. Leksykon kieszonkowy - autor Arnold Robbins
	Wydawca: Helion 2001 (O'Reilly).
	ISBN: 83-7197-472-8
	http://helion.pl/ksiazki/vilek.htm
  Jest to ksi±¿eczka zawieraj±ca spis poleceñ vi i jego najwa¿niejszych
  klonów (miêdzy innymi Vim-a).

	Edytor vi - autorzy Linda Lamb i Arnold Robbins
	Wydawca: Helion 2001 (O'Reilly) - wg 6 ang. wydania
	ISBN: 83-7197-539-2
	http://helion.pl/ksiazki/viedyt.htm
  Rozszerzona wersja Learning the Vi Editor w polskim t³umaczeniu.

  Ten tutorial zosta³ napisany przez Michaela C. Pierce'a i Roberta K. Ware'a,
  Colorado School of Mines korzystaj±c z pomocy Charlesa Smitha,
  Colorado State University.
  E-mail: bware@mines.colorado.edu.

  Zmodyfikowane dla Vim-a przez Brama Moolenaara.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Przet³umaczone przez Miko³aja Machowskiego,
  Sierpieñ 2001,
  rev. Marzec 2002
  Wszelkie uwagi proszê kierowaæ na: mikmach@wp.pl

