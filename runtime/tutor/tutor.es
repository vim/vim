===============================================================================
=     B i e n v e n i d o   a l   t u t o r   d e   V I M  -  VersiÃ³n 1.7     =
===============================================================================

     Vim es un editor muy potente que dispone de muchos comandos, demasiados
     para ser explicados en un tutor como Ã©ste. Este tutor estÃ¡ diseÃ±ado
     para describir suficientes comandos para que usted sea capaz de
     aprender fÃ¡cilmente a usar Vim como un editor de propÃ³sito general.

     El tiempo necesario para completar el tutor es aproximadamente de 30
     minutos, dependiendo de cuÃ¡nto tiempo se dedique a la experimentaciÃ³n.

     Los comandos de estas lecciones modificarÃ¡n el texto. Haga una copia de
     este fichero para practicar (con Â«vimtutorÂ» esto ya es una copia).

     Es importante recordar que este tutor estÃ¡ pensado para enseÃ±ar con
     la prÃ¡ctica. Esto significa que es necesario ejecutar los comandos
     para aprenderlos adecuadamente. Si Ãºnicamente lee el texto, Â¡se le
     olvidarÃ¡n los comandos.

     Ahora, asegÃºrese de que la tecla de bloqueo de mayÃºsculas NO estÃ¡
     activada y pulse la tecla	j  lo suficiente para mover el cursor
     de forma que la LecciÃ³n 1.1 ocupe completamente la pantalla.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LecciÃ³n 1.1: MOVER EL CURSOR

 ** Para mover el cursor, pulse las teclas h,j,k,l de la forma indicada. **
      ^
      k       IndicaciÃ³n: La tecla h estÃ¡ a la izquierda y lo mueve a la izquierda.
 < h	 l >		  La tecla l estÃ¡ a la derecha y lo mueve a la derecha.
      j			  La tecla j parece una flecha que apunta hacia abajo.
      v

  1. Mueva el cursor por la pantalla hasta que se sienta cÃ³modo con ello.

  2. Mantenga pulsada la tecla (j) hasta que se repita Â«automÃ¡gicamenteÂ».
     Ahora ya sabe como llegar a la lecciÃ³n siguiente.

  3. Utilizando la tecla abajo, vaya a la lecciÃ³n 1.2.

NOTA: Si alguna vez no estÃ¡ seguro sobre algo que ha tecleado, pulse <ESC>
      para situarse en modo Normal. Luego vuelva a teclear la orden que deseaba.

NOTA: Las teclas de movimiento del cursor tambiÃ©n funcionan. Pero usando
      hjkl podrÃ¡ moverse mucho mÃ¡s rÃ¡pido una vez que se acostumbre a ello.
      Â¡De verdad!

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    LecciÃ³n 1.2: SALIR DE VIM

  Â¡Â¡ NOTA: Antes de ejecutar alguno de los siguientes pasos lea primero
	   la lecciÃ³n entera!!

  1. Pulse la tecla <ESC> (para asegurarse de que estÃ¡ en modo Normal).

  2. Escriba:  :q! <INTRO>
     Esto provoca la salida del editor DESCARTANDO cualquier cambio que haya hecho.

  3. Regrese aquÃ­ ejecutando el comando que le trajo a este tutor.
     Ãste puede haber sido:   vimtutor <INTRO>

  4. Si ha memorizado estos pasos y se siente con confianza, ejecute los
     pasos 1 a 3 para salir y volver a entrar al editor. 

NOTA:  :q! <INTRO> descarta cualquier cambio que haya realizado.
       En prÃ³ximas lecciones aprenderÃ¡ cÃ³mo guardar los cambios en un archivo.

  5. Mueva el cursor hasta la LecciÃ³n 1.3.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		   LecciÃ³n 1.3: EDITAR TEXTO - BORRAR

  ** Pulse  x  para eliminar el caracter bajo el cursor. **

  1. Mueva el cursor a la lÃ­nea de abajo seÃ±alada con --->.

  2. Para corregir los errores, mueva el cursor hasta que estÃ© sobre el
     caracter que va a ser borrado.

  3. Pulse la tecla  x	para eliminar el carÃ¡cter no deseado.

  4. Repita los pasos 2 a 4 hasta que la frase sea la correcta.

---> La vvaca saltÃ³Ã³ soobree laa luuuuna.

  5. Ahora que la lÃ­nea esta correcta, continÃºe con la LecciÃ³n 1.4.

NOTA: A medida que vaya avanzando en este tutor no intente memorizar,
      aprenda practicando.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		   LecciÃ³n 1.4: EDITAR TEXTO - BORRAR

         ** Pulse  i  para insertar texto. **

  1. Mueva el cursor a la primera lÃ­nea de abajo seÃ±alada con --->.

  2. Para hacer que la primera lÃ­nea sea igual que la segunda, mueva el
     cursor hasta que estÃ© sobre el caracter ANTES del cual el texto va a ser
     insertado.

  3. Pulse  i  y escriba los caracteres a aÃ±adir.

  4. A medida que sea corregido cada error pulse <ESC> para volver al modo
     Normal. Repita los pasos 2 a 4 para corregir la frase.

---> Flta texto en esta .
---> Falta algo de texto en esta lÃ­nea.

  5. Cuando se sienta cÃ³modo insertando texto pase vaya a la lecciÃ³n 1.5.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LecciÃ³n 1.5: EDITAR TEXTO - AÃADIR


			** Pulse  A  para aÃ±adir texto. **

  1. Mueva el cursor a la primera lÃ­nea inferior marcada con --->.
     No importa sobre quÃ© caracter estÃ¡ el cursor en esta lÃ­nea.

  2. Pulse  A  y escriba el texto necesario.

  3. Cuando el texto haya sido aÃ±adido pulse <ESC> para volver al modo Normal.

  4. Mueva el cursor a la segunda lÃ­nea marcada con ---> y repita los
     pasos 2 y 3 para corregir esta frase.

---> Falta algÃºn texto en es
     Falta algÃºn texto en esta lÃ­nea.
---> TambiÃ©n falta alg
     TambiÃ©n falta algÃºn texto aquÃ­.

  5. Cuando se sienta cÃ³modo aÃ±adiendo texto pase a la lecciÃ³n 1.6.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LecciÃ³n 1.6: EDITAR UN ARCHIVO

		    ** Use  :wq  para guardar un archivo y salir **

 !! NOTA: Antes de ejecutar los siguientes pasos, lea la lecciÃ³n entera!!

  1.  Si tiene acceso a otra terminal, haga lo siguiente en ella.
      Si no es asÃ­, salga de este tutor como hizo en la lecciÃ³n 1.2:  :q!

  2. En el sÃ­mbolo del sistema escriba este comando:  vim archivo.txt <INTRO>
     'vim' es el comando para arrancar el editor Vim, 'archivo.txt'
     es el nombre del archivo que quiere editar
     Utilice el nombre de un archivo que pueda cambiar.

  3. Inserte y elimine texto como ya aprendiÃ³ en las lecciones anteriores.

  4. Guarde el archivo con los cambios y salga de Vim con:  :wq <INTRO>

  5. Si ha salido de vimtutor en el paso 1 reinicie vimtutor y baje hasta
     el siguiente sumario.

  6. DespuÃ©s de leer los pasos anteriores y haberlos entendido: hÃ¡galos.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			    RESUMEN DE LA LECCIÃN 1


  1. El cursor se mueve utilizando las teclas de las flechas o las teclas hjkl.
	 h (izquierda)	   j (abajo)	  k (arriba)	  l (derecha)

  2. Para acceder a Vim desde el sÃ­mbolo del sistema escriba:
     vim NOMBREARCHIVO <INTRO>

  3. Para salir de Vim escriba: <ESC> :q! <INTRO> para eliminar todos
     los cambios.
     O escriba:  <ESC>  :wq  <INTRO> para guardar los cambios.

  4. Para borrar un caracter bajo el cursor en modo Normal pulse:  x

  5. Para insertar o aÃ±adir texto escriba:
     i  escriba el texto a insertar <ESC> inserta el texto antes del cursor
	 A  escriba el texto a aÃ±adir <ESC> aÃ±ade texto al final de la lÃ­nea

NOTA: Pulsando <ESC> se vuelve al modo Normal o cancela una orden no deseada
      o incompleta.

Ahora continÃºe con la LecciÃ³n 2.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LecciÃ³n 2.1:  COMANDOS PARA BORRAR


          ** Escriba dw para borrar una palabra **


  1. Pulse <ESC> para asegurarse de que estÃ¡ en el modo Normal.

  2. Mueva el cursor a la lÃ­nea inferior seÃ±alada con --->.

  3. Mueva el cursor al comienzo de una palabra que desee borrar.

  4. Pulse   dw   para hacer que la palabra desaparezca.

  NOTA: La letra  d  aparecerÃ¡ en la Ãºltima lÃ­nea inferior derecha 
    de la pantalla mientras la escribe. Vim estÃ¡ esperando que escriba  w .
    Si ve otro caracter que no sea  d  escribiÃ³ algo mal, pulse <ESC> y
    comience de nuevo.

---> Hay algunas palabras pÃ¡salo bien que no pertenecen papel a esta frase.

  5. Repita los pasos 3 y 4 hasta que la frase sea correcta y pase a la
     lecciÃ³n 2.2.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    LecciÃ³n 2.2: MÃS COMANDOS PARA BORRAR


	  ** Escriba  d$  para borrar hasta el final de la lÃ­nea. **

  1. Pulse  <ESC>  para asegurarse de que estÃ¡ en el modo Normal.

  2. Mueva el cursor a la lÃ­nea inferior seÃ±alada con --->.

  3. Mueva el cursor al final de la lÃ­nea correcta (DESPUÃS del primer . ).

  4. Escriba  d$  para borrar hasta el final de la lÃ­nea.

---> Alguien ha escrito el final de esta lÃ­nea dos veces. esta lÃ­nea dos veces.

  5. Pase a la lecciÃ³n 2.3 para entender quÃ© estÃ¡ pasando.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    LecciÃ³n 2.3: SOBRE OPERADORES Y MOVIMIENTOS


  Muchos comandos que cambian texto estÃ¡n compuestos por un operador y un
  movimiento.
  El formato para eliminar un comando con el operador de borrado  d  es el
  siguiente:

    d   movimiento

  Donde:
    d          - es el operador para borrar.
    movimiento - es sobre lo que el comando va a operar (lista inferior).

  Una lista resumida de movimientos:
   w - hasta el comienzo de la siguiente palabra, EXCLUYENDO su primer
       caracter.
   e - hasta el final de la palabra actual, INCLUYENDO su primer caracter.
   $ - hasta el finalde la lÃ­nea, INCLUYENDO el Ãºltimo caracter.

NOTA: Pulsando Ãºnicamente el movimiento estando en el modo Normal sin un
      operador, moverÃ¡ el cursor como se especifica en la lista anterior.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		  LecciÃ³n 2.4: UTILIZAR UN CONTADOR PARA UN MOVIMIENTO


   ** Al escribir un nÃºmero antes de un movimiento, lo repite esas veces. **

  1. Mueva el cursor al comienzo de la lÃ­nea marcada con --->.

  2. Escriba 2w  para mover el cursor dos palabras hacia adelante.

  3. Escriba  3e  para mover el cursor al final de la tercera palabra hacia
     adelante.

  4. Escriba  0  (cero) para colocar el cursor al inicio de la lÃ­nea.

  5. Repita el paso 2 y 3 con diferentes nÃºmeros.

---> Esto es solo una lÃ­nea con palabra donde poder moverse.

  6. Pase a la lecciÃ³n 2.5.




~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LecciÃ³n 2.5: UTILIZAR UN CONTADOR PARA BORRAR MAS


   ** Al escribir un nÃºmero con un operador lo repite esas veces. **

  En combinaciÃ³n con el operador de borrado y el movimiento mencionado
  anteriormente, aÃ±ada un contador antes del movimiento para eliminar mÃ¡s:
	 d   nÃºmero   movimiento

  1. Mueva el cursos a la primera palabra en MAYÃSCULAS en la lÃ­nea
     marcada con --->.

  2. Escriba  d2w  para eliminar las dos palabras en MAYÃSCULAS.

  3. Repita los pasos 1 y 2 con diferentes contadores para eliminar
     las siguientes palabras en MAYÃSCULAS con un comando.

--->  esta ABC DE serie FGHI JK LMN OP de palabras ha sido Q RS TUV limpiada.





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 LecciÃ³n 2.6: OPERACIÃN EN LÃNEAS


		   ** Escriba  dd   para eliminar una lÃ­nea completa. **

  Debido a la frecuencia con que se elimina una lÃ­nea completa, los
  diseÃ±adores de Vi, decidieron que serÃ­a mÃ¡s sencillo simplemente escribir
  dos letras d para eliminar una lÃ­nea.

  1. Mueva el cursor a la segunda lÃ­nea del pÃ¡rrafo inferior.
  2. Escriba  dd  para eliminar la lÃ­nea.
  3. Ahora muÃ©vase a la cuarta lÃ­nea.
  4. Escriba   2dd   para eliminar dos lÃ­neas a la vez.

--->  1)  Las rosas son rojas,
--->  2)  El barro es divertido,
--->  3)  Las violetas son azules,
--->  4)  Tengo un coche,
--->  5)  Los relojes dan la hora,
--->  6)  El azÃºcar es dulce
--->  7)  Y tambiÃ©n lo eres tÃº.

La duplicaciÃ³n para borrar lÃ­neas tambiÃ©n funcionan con los operadores
mencionados anteriormente.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LecciÃ³n 2.7: EL MANDATO DESHACER


   ** Pulse  u	para deshacer los Ãºltimos comandos,
	     U	para deshacer una lÃ­nea entera.       **

  1. Mueva el cursor a la lÃ­nea inferior seÃ±alada con ---> y sitÃºelo bajo el
     primer error.
  2. Pulse  x  para borrar el primer caracter no deseado.
  3. Pulse ahora  u  para deshacer el Ãºltimo comando ejecutado.
  4. Ahora corrija todos los errores de la lÃ­nea usando el comando  x.
  5. Pulse ahora  U  mayÃºscula para devolver la lÃ­nea a su estado original.
  6. Pulse ahora  u  unas pocas veces para deshacer lo hecho por  U  y los
     comandos previos.
  7. Ahora pulse CTRL-R (mantenga pulsada la tecla CTRL y pulse R) unas
     cuantas veces para volver a ejecutar los comandos (deshacer lo deshecho).

---> Corrrija los errores dee esttta lÃ­nea y vuuelva a ponerlos coon deshacer.

  8. Estos son unos comandos muy Ãºtiles. Ahora vayamos al resumen de la
     lecciÃ³n 2.




~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			    RESUMEN DE LA LECCIÃN 2

  1. Para borrar desde el cursor hasta siguiente palabra pulse:	     dw
  2. Para borrar desde el cursor hasta el final de la palabra pulse: de
  3. Para borrar desde el cursor hasta el final de una lÃ­nea pulse:	 d$
  4. Para borrar una lÃ­nea entera pulse:                             dd

  5. Para repetir un movimiento precÃ©dalo con un nÃºmero:  2w
  6. El formato para un comando de cambio es:
               operador  [nÃºmero]  movimiento
     donde:
       comando    - es lo que hay que hacer, por ejemplo,  d  para borrar
       [nÃºmero]   - es un nÃºmero opcional para repetir el movimiento
       movimiento - se mueve sobre el texto sobre el que operar, como
		            w (palabra), $ (hasta el final de la lÃ­nea), etc.
  7. Para moverse al inicio de la lÃ­nea utilice un cero:  0

  8. Para deshacer acciones previas pulse:		         u (u minÃºscula)
     Para deshacer todos los cambios de una lÃ­nea pulse: U (U mayÃºscula)
     Para deshacer lo deshecho pulse:			         CTRL-R


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 LecciÃ³n 3.1: EL MANDATO Â«PUTÂ» (poner)

  ** Pulse p para poner lo Ãºltimo que ha borrado despuÃ©s del cursor. **

  1. Mueva el cursor al final de la lista de abajo.

  2. Escriba  dd  para borrar la lÃ­nea y almacenarla en el buffer de Vim.

  3. Mueva el cursor a la lÃ­nea que debe quedar por debajo de la
     lÃ­nea a mover.

  4. Estando en mod Normal, pulse   p	para restituir la lÃ­nea borrada.

  5. Repita los pasos 2 a 4 para poner todas las lÃ­neas en el orden correcto.

     d) Â¿Puedes aprenderla tÃº?
     b) Las violetas son azules,
     c) La inteligencia se aprende,
     a) Las rosas son rojas,

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		       LecciÃ³n 3.2: EL MANDATO Â«REPLACEÂ» (remplazar)


  ** Pulse  r  y un carÃ¡cter para sustituir el carÃ¡cter sobre el cursor. **


  1. Mueva el cursor a la primera lÃ­nea de abajo seÃ±alada con --->.

  2. Mueva el cursor para situarlo bajo el primer error.

  3. Pulse   r	 y el carÃ¡cter que debe sustituir al errÃ³neo.

  4. Repita los pasos 2 y 3 hasta que la primera lÃ­nea estÃ© corregida.

---> Â¡Cuendo esta lÃ­nea fue rscrita alguien pulso algunas teclas equibocadas!
---> Â¡Cuando esta lÃ­nea fue escrita alguien pulsÃ³ algunas teclas equivocadas!






~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			LecciÃ³n 3.3: EL MANDATO Â«CHANGEÂ» (cambiar)


     ** Para cambiar parte de una palabra o toda ella escriba  cw . **


  1. Mueva el cursor a la primera lÃ­nea de abajo seÃ±alada con --->.

  2. SitÃºe el cursor en la u de lubrs.

  3. Escriba  cw  y corrija la palabra (en este caso, escriba 'Ã­nea').

  4. Pulse <ESC> y mueva el cursor al error siguiente (el primer carÃ¡cter
     que deba cambiarse).

  5. Repita los pasos 3 y 4 hasta que la primera frase sea igual a la segunda.

---> Esta lubrs tiene unas pocas pskavtad que corregir usem el mandato change.
---> Esta lÃ­nea tiene unas pocas palabras que corregir usando el mandato change.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		      LecciÃ³n 3.4: MÃS CAMBIOS USANDO c

   ** El mandato change se utiliza con los mismos objetos que delete. **

  1. El mandato change funciona de la misma forma que delete. El formato es:

       [nÃºmero]   c   objeto	   O	    c	[nÃºmero]   objeto

  2. Los objetos son tambiÃ©m los mismos, tales como  w (palabra), $ (fin de
     la lÃ­nea), etc.

  3. Mueva el cursor a la primera lÃ­nea de abajo seÃ±alada con --->.

  4. Mueva el cursor al primer error.

  5. Escriba  c$  para hacer que el resto de la lÃ­nea sea como la segunda
     y pulse <ESC>.

---> El final de esta lÃ­nea necesita alguna ayuda para que sea como la segunda.
---> El final de esta lÃ­nea necesita ser corregido usando el mandato  c$.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			    RESUMEN DE LA LECCIÃN 3


  1. Para sustituir texto que ha sido borrado, pulse  p . Esto Pone el texto
     borrado DESPUÃS del cursor (si lo que se ha borrado es una lÃ­nea se
     situarÃ¡ sobre la lÃ­nea que estÃ¡ sobre el cursor).

  2. Para sustituir el carÃ¡cter bajo el cursor, pulse	r   y luego el
     carÃ¡cter que sustituirÃ¡ al original.

  3. El mandato change le permite cambiar el objeto especificado desde la
     posiciÃ³n del cursor hasta el final del objeto; e.g. Pulse	cw  para
     cambiar desde el cursor hasta el final de la palabra, c$  para cambiar
     hasta el final de la lÃ­nea.

  4. El formato para change es:

	 [nÃºmero]   c	objeto	      O		c   [nÃºmero]   objeto

  Pase ahora a la lecciÃ³n siguiente.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	       LecciÃ³n 4.1: SITUACIÃN EN EL FICHERO Y SU ESTADO


 ** Pulse CTRL-g para mostrar su situaciÃ³n en el fichero y su estado.
    Pulse MAYU-G para moverse a una determinada lÃ­nea del fichero. **

  Nota: Â¡Â¡Lea esta lecciÃ³n entera antes de ejecutar alguno de los pasos!!


  1. Mantenga pulsada la tecla Ctrl y pulse  g . Aparece una lÃ­nea de estado
     al final de la pantalla con el nombre del fichero y la lÃ­nea en la que
     estÃ¡ situado. Recuerde el nÃºmero de la lÃ­nea para el Paso 3.

  2. Pulse Mayu-G para ir al final del fichero.

  3. Escriba el nÃºmero de la lÃ­nea en la que estaba y despÃºes Mayu-G. Esto
     le volverÃ¡ a la lÃ­nea en la que estaba cuando pulsÃ³ Ctrl-g.
     (Cuando escriba los nÃºmeros NO se mostrarÃ¡n en la pantalla).

  4. Si se siente confiado en poder hacer esto ejecute los pasos 1 a 3.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			LecciÃ³n 4.2: EL MANDATO Â«SEARCHÂ» (buscar)

     ** Escriba  /  seguido de una frase para buscar la frase. **

  1. En modo Normal pulse el carÃ¡cter  / . FÃ­jese que tanto el carÃ¡cter  /
     como el cursor aparecen en la Ãºltima lÃ­nea de la pantalla, lo mismo
     que el mandato  : .

  2. Escriba ahora   errroor   <INTRO>. Esta es la palabra que quiere buscar.

  3. Para repetir la bÃºsqueda, simplemente pulse  n .
     Para busacar la misma frase en la direcciÃ³n opuesta, pulse Mayu-N .

  4. Si quiere buscar una frase en la direcciÃ³n opuesta (hacia arriba),
     utilice el mandato  ?  en lugar de  / .

---> Cuando la bÃºsqueda alcanza el final del fichero continuarÃ¡ desde el
     principio.

  Â«errroorÂ» no es la forma de deletrear error; errroor es un error.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	       LecciÃ³n 4.3: BÃSQUEDA PARA COMPROBAR PARÃNTESIS

   ** Pulse %  para encontrar el parÃ©ntesis correspondiente a ),] o } . **


  1. SitÃºe el cursor en cualquiera de los caracteres ), ] o } en la lÃ­nea de
     abajo seÃ±alada con --->.

  2. Pulse ahora el carÃ¡cter  %  .

  3. El cursor deberÃ­a situarse en el parÃ©ntesis (, corchete [ o llave {
     correspondiente.

  4. Pulse  %  para mover de nuevo el cursor al parÃ©ntesis, corchete o llave
     correspondiente.

---> Esto ( es una lÃ­nea de prueba con (, [, ], {, y } en ella. )).

Nota: Â¡Esto es muy Ãºtil en la detecciÃ³n de errores en un programa con
      parÃ©ntesis, corchetes o llaves disparejos.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		  LecciÃ³n 4.4: UNA FORMA DE CAMBIAR ERRORES


    ** Escriba	:s/viejo/nuevo/g para sustituir 'viejo' por 'nuevo'. **


  1. Mueva el cursor a la lÃ­nea de abajo seÃ±alada con --->.

  2. Escriba  :s/laas/las/  <INTRO> . Tenga en cuenta que este mandato cambia
     sÃ³lo la primera apariciÃ³n en la lÃ­nea de la expresiÃ³n a cambiar.

---> Laas mejores Ã©pocas para ver laas flores son laas primaveras.

  4. Para cambiar todas las apariciones de una expresiÃ³n ente dos lÃ­neas
     escriba   :#,#s/viejo/nuevo/g   donde #,# son los nÃºmeros de las dos
     lÃ­neas. Escriba   :%s/viejo/nuevo/g   para hacer los cambios en todo
     el fichero.





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			    RESUMEN DE LA LECCIÃN 4


  1. Ctrl-g  muestra la posiciÃ³n del cursor en el fichero y su estado.
     Mayu-G mueve el cursor al final del fichero. Un nÃºmero de lÃ­nea
     seguido de Mayu-G mueve el cursor a la lÃ­nea con ese nÃºmero.

  2. Pulsando  /  seguido de una frase busca la frase hacia ADELANTE.
     Pulsando  ?  seguido de una frase busca la frase hacia ATRÃS.
     DespuÃ©s de una bÃºsqueda pulse  n  para encontrar la apariciÃ³n
     siguiente en la misma direcciÃ³n.

  3. Pulsando  %  cuando el cursor esta sobre (,), [,], { o } localiza
     la pareja correspondiente.

  4. Para cambiar viejo por nuevo en una lÃ­nea pulse	      :s/viejo/nuevo
     Para cambiar todos los viejo por nuevo en una lÃ­nea pulse :s/viejo/nuevo/g
     Para cambiar frases entre dos nÃºmeros de lÃ­neas pulse  :#,#s/viejo/nuevo/g
     Para cambiar viejo por nuevo en todo el fichero pulse  :%s/viejo/nuevo/g
     Para pedir confirmaciÃ³n en cada caso aÃ±ada  'c'	    :%s/viejo/nuevo/gc


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		LecciÃ³n 5.1: CÃMO EJECUTAR UN MANDATO EXTERNO


  ** Escriba  :!  seguido de un mandato externo para ejecutar ese mandato. **


  1. Escriba el conocido mandato  :  para situar el cursor al final de la
     pantalla. Esto le permitirÃ¡ introducir un mandato.

  2. Ahora escriba el carÃ¡cter ! (signo de admiraciÃ³n). Esto le permitirÃ¡
     ejecutar cualquier mandato del sistema.

  3. Como ejemplo escriba   ls	 despuÃ©s del ! y luego pulse <INTRO>. Esto
     le mostrarÃ¡ una lista de su directorio, igual que si estuviera en el
     sÃ­mbolo del sistema. Si  ls  no funciona utilice	!:dir	.

--->Nota: De esta manera es posible ejecutar cualquier mandato externo.

--->Nota: Todos los mandatos   :   deben finalizarse pulsando <INTRO>.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LecciÃ³n 5.2: MÃS SOBRE GUARDAR FICHEROS


     ** Para guardar los cambios hechos en un fichero,
	escriba  :w NOMBRE_DE_FICHERO. **


  1. Escriba  :!dir  o	:!ls  para ver una lista de su directorio.
     Ya sabe que debe pulsar <INTRO> despuÃ©s de ello.

  2. Elija un nombre de fichero que todavÃ­a no exista, como TEST.

  3. Ahora escriba   :w TEST  (donde TEST es el nombre de fichero elegido).

  4. Esta acciÃ³n guarda todo el fichero  (Vim Tutor)  bajo el nombre TEST.
     Para comprobarlo escriba	:!dir	de nuevo y vea su directorio.

---> Tenga en cuenta que si sale de Vim y  entra de nuevo con el nombre de
     fichero TEST, el fichero serÃ­a una copia exacta del tutor cuando lo
     ha guardado.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	       LecciÃ³n 5.3: UN MANDATO DE ESCRITURA SELECTIVO

   ** Para guardar parte del fichero escriba   :#,# NOMBRE_DEL_FICHERO **


  1. Escriba de nuevo, una vez mÃ¡s,  :!dir  o  :!ls  para obtener una lista
     de su directorio y elija nombre de fichero adecuado, como TEST.

  2. Mueva el cursor al principio de la pantalla y pulse  Ctrl-g  para saber
     el nÃºmero de la lÃ­nea correspondiente. Â¡RECUERDE ESTE NÃMERO!

  3. Ahora mueva el cursor a la Ãºltima lÃ­nea de la pantalla y pulse Ctrl-g
     de nuevo. Â¡RECUERDE TAMBIÃN ESTE NÃMERO!

  4. Para guardar SOLAMENTE una parte de un fichero, escriba  :#,# w TEST
     donde #,# son los nÃºmeros que usted ha recordado (primera lÃ­nea,
     Ãºltima lÃ­nea) y TEST es su nombre de dichero.

  5. De nuevo, vea que el fichero esta ahÃ­ con	:!dir  pero NO lo borre.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		LecciÃ³n 5.4: RECUPERANDO Y MEZCLANDO FICHEROS

 ** Para insertar el contenido de un fichero escriba :r NOMBRE_DEL_FICHERO **

  1. Escriba   :!dir   para asegurarse de que su fichero TEST del ejercicio
     anterior estÃ¡ presente.

  2. Situe el cursor al principio de esta pantalla.

NOTA: DespuÃ©s de ejecutar el paso 3 se verÃ¡ la LecciÃ³n 5.3. Luego muÃ©vase
      hacia ABAJO para ver esta lecciÃ³n de nuevo.

  3. Ahora recupere el fichero TEST utilizando el mandato  :r TEST  donde
     TEST es el nombre del fichero.

NOTA: El fichero recuperado se sitÃºa a partir de la posiciÃ³n del cursor.

  4. Para verificar que el fichero ha sido recuperado, mueva el cursor hacia
     arriba y vea que hay dos copias de la LecciÃ³n 5.3, la original y la
     versiÃ³n del fichero.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			   RESUMEN DE LA LECCIÃN 5


  1.  :!mandato  ejecuta un mandato externo.

      Algunos ejemplos Ãºtiles son:
	  :!dir - muestra el contenido de un directorio.
	  :!del NOMBRE_DE_FICHERO  -  borra el fichero NOMBRE_DE FICHERO.

  2.  :#,#w NOMBRE_DE _FICHERO  guarda desde la lÃ­nea # hasta la # en el
     fichero NOMBRE_DE_FICHERO.

  3.  :r NOMBRE_DE _FICHERO  recupera el fichero del disco NOMBRE_DE FICHERO
     y lo inserta en el fichero en curso a partir de la posiciÃ³n del cursor.







~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 LecciÃ³n 6.1: EL MANDATO Â«OPENÂ» (abrir)


	 ** Pulse  o  para abrir una lÃ­nea debajo del cursor
	    y situarle en modo Insert **


  1. Mueva el cursor a la lÃ­nea de abajo seÃ±alada con --->.

  2. Pulse  o (minÃºscula) para abrir una lÃ­nea por DEBAJO del cursor
     y situarle en modo Insert.

  3. Ahora copie la lÃ­nea seÃ±alada con ---> y pulse <ESC> para salir del
     modo Insert.

---> Luego de pulsar  o  el cursor se sitÃºa en la lÃ­nea abierta en modo Insert.

  4. Para abrir una lÃ­nea por encima del cursor, simplemente pulse una O
     mayÃºscula, en lugar de una o minÃºscula. Pruebe este en la lÃ­nea siguiente.
Abra una lÃ­nea sobre Ã©sta pulsando Mayu-O cuando el cursor estÃ¡ en esta lÃ­nea.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			LecciÃ³n 6.2: EL MANDATO Â«APPENDÂ» (aÃ±adir)

	 ** Pulse  a  para insertar texto DESPUÃS del cursor. **


  1. Mueva el cursor al final de la primera lÃ­nea de abajo seÃ±alada con --->
     pulsando  $  en modo Normal.

  2. Escriba una  a  (minÃºscula) para aÃ±adir texto DESPUÃS del carÃ¡cter
     que estÃ¡ sobre el cursor. (A mayÃºscula aÃ±ade texto al final de la lÃ­nea).

Nota: Â¡Esto evita el pulsar  i , el Ãºltimo carÃ¡cter, el texto a insertar,
      <ESC>, cursor a la derecha y, finalmente, x , sÃ³lo para aÃ±adir algo
      al final de una lÃ­nea!

  3. Complete ahora la primera lÃ­nea. NÃ³tese que append es exactamente lo
     mismo que modo Insert, excepto por el lugar donde se inserta el texto.

---> Esta lÃ­nea le permitirÃ¡ praticar
---> Esta lÃ­nea le permitirÃ¡ praticar el aÃ±adido de texto al final de una lÃ­nea.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		     LecciÃ³n 6.3: OTRA VERSIÃN DE Â«REPLACEÂ» (remplazar)

    ** Pulse una  R  mayÃºscula para sustituir mÃ¡s de un carÃ¡cter. **


  1. Mueva el cursor a la primera lÃ­nea de abajo seÃ±alada con --->.

  2. SitÃºe el cursor al comienzo de la primera palabra que sea diferente
     de las de la segunda lÃ­nea marcada con ---> (la palabra 'anterior').

  3. Ahora pulse  R  y sustituya el resto del texto de la primera lÃ­nea
     escribiendo sobre el viejo texto para que la primera lÃ­nea sea igual
     que la segunda.

---> Para hacer que esta lÃ­nea sea igual que la anterior use las teclas.
---> Para hacer que esta lÃ­nea sea igual que la siguiente escriba R y el texto.

  4. NÃ³tese que cuando pulse <ESC> para salir, el texto no alterado permanece.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			 LecciÃ³n 6.4: FIJAR OPCIONES

 ** Fijar una opciÃ³n de forma que una bÃºsqueda o sustituciÃ³n ignore la caja **
  (Para el concepto de caja de una letra, vÃ©ase la nota al final del fichero)


  1. Busque 'ignorar' introduciendo:
     /ignorar
     Repita varias veces la bÃºsque pulsando la tecla n

  2. Fije la opciÃ³n 'ic' (Ignorar la caja de la letra) escribiendo:
     :set ic

  3. Ahora busque 'ignorar' de nuevo pulsando n
     Repita la bÃºsqueda varias veces mÃ¡s pulsando la tecla n

  4. Fije las opciones 'hlsearch' y 'insearch':
     :set hls is

  5. Ahora introduzca la orden de bÃºsqueda otra vez, y vea quÃ© pasa:
     /ignore

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			   RESUMEN DE LA LECCIÃN 6


  1. Pulsando  o  abre una lÃ­nea por DEBAJO del cursor y sitÃºa el cursor en
     la lÃ­nea abierta en modo Insert.
     Pulsando una O mayÃºscula se abre una lÃ­nea SOBRE la que estÃ¡ el cursor.

  2. Pulse una	a  para insertar texto DESPUÃS del carÃ¡cter sobre el cursor.
     Pulsando una  A  mayÃºscula aÃ±ade automÃ¡ticamente texto al final de la
     lÃ­nea.

  3. Pulsando una  R  mayÃºscula se entra en modo Replace hasta que, para salir,
     se pulse <ESC>.

  4. Escribiendo Â«:set xxxÂ» fija la opciÃ³n Â«xxxÂ»







~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		  LecciÃ³n 7: MANDATOS PARA LA AYUDA EN LÃNEA

		 ** Utilice el sistema de ayuda en lÃ­nea **


  Vim dispone de un sistema de ayuda en lÃ­nea. Para activarlo, pruebe una
  de estas tres formas:
	- pulse la tecla <AYUDA> (si dispone de ella)
	- pulse la tecla <F1> (si dispone de ella)
	- escriba   :help <INTRO>

  Escriba   :q <INTRO>	 para cerrar la ventana de ayuda.

  Puede encontrar ayuda en casi cualquier tema aÃ±adiendo un argumento al
  mandato Â«:helpÂ» mandato. Pruebe Ã©stos:

  :help w <INTRO>
  :help c_<T <INTRO>
  :help insert-index <INTRO>


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  AquÃ­ concluye el tutor de Vim. EstÃ¡ pensado para dar una visiÃ³n breve del
  editor Vim, lo suficiente para permitirle usar el editor de forma bastante
  sencilla. EstÃ¡ muy lejos de estar completo pues Vim tiene muchÃ­simos mÃ¡s
  mandatos.

  Para lecturas y estudios posteriores se recomienda el libro:
	Learning the Vi Editor - por Linda Lamb
	Editorial: O'Reilly & Associates Inc.
  Es un buen libro para llegar a saber casi todo lo que desee hacer con Vi.
  La sexta ediciÃ³n incluye tambiÃ©n informaciÃ³n sobre Vim.

  Este tutorial ha sido escrito por Michael C. Pierce y Robert K. Ware,
  Colorado School of Mines utilizando ideas suministradas por Charles Smith,
  Colorado State University.
  E-mail: bware@mines.colorado.edu.

  Modificado para Vim por Bram Moolenaar.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Traducido del inglÃ©s por:

  Eduardo F. Amatria
  Correo electrÃ³nico: eferna1@platea.pntic.mec.es

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
