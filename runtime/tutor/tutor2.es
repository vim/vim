===============================================================================
=     B i e n v e n i d o   a l   t u t o r   d e   V I M  -  Versión 1.7     =
===============================================================================
=	                         CAPÍTULO DOS                                 =
===============================================================================

     Hic Sunt Dracones: si esta es tu primera vez usando vim y tienes la
     intención de aprovechar el capítulo de introducción, simplemente escribe
     :q!<ENTER> y ejecuta vimtutor para empezar por el Capítulo 1.

     El tiempo aproxiomado para completar este capítulo es de 8-10 minutos,
     dependiendo de cuanto tiempo dediques a experimentar.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lección 2.1.1: DOMINAR LOS OBJETOS DE TEXTO

** Operar en bloques de texto lógicos con precisión usando objetos de texto. **

  1. Practica las operaciones con palabras:
     - Situa el cursor en cualquier palabra de la línea inferior
     - Escribe  diw  para eliminar la palabra DENTRO donde está el cursor
       (la palabra sin el espacio circundante)
     - Escribe  daw  para eliminar UNA PALABRA
       (incluyendo espacios en blanco al final)
     - Prueba con otras operaciones:  ciw  (cambiar),  yiw  (copiar),
       gqiw  (formato)

---> Practica aquí: "Vim's", (text_object), y aquí palabras 'poderosas'.

  2. Trabaja con contenido entre paréntesis, corchetes o llaves:
     - Situa el cursor dentro de cualquier par de los símbolos () {} [] <>
     - Escribe  di(  o  dib  (eliminar dentro de los símbolos)
     - Escribe  da(  o  dab  (eliminar alrededor de los símbolos)
     - Prueba lo mismo con  i"/a"  para las comillas
     - Prueba con  it/at  para etiquetas HTML/XML

---> Ejemplos de prueba: con {llaves}, [corchetes], <ángulos> y "comillas".

  3. Manipulación de párrafos y frases:
     - Utiliza  dip  para eliminar el párrafo donde se encuentra el cursor
     - Utiliza  vap  para seleccionar visualmente el párrafo entero
     - Prueba  das  para eliminar una frase
       (funciona entre símbolos de puntuación .!?)

  4. Combinaciones avanzadas:
     - ciwnuevo<ESC>    - Cambiar la palabra actual por "nuevo"
     - yss"<ESC>        - Encerrar la línea completa entre comillas
                          (similar al comlemento vim-surround)
     - gUit             - Convertir a mayúsculas el contenido de la
			  etiqueta HTML donde esté el cursor
     - va"p             - Seleccionar el texto entre comillas y pegarlo sobre él

---> Ejercicio final: (Modificar "el" texto) al [aplicar {varias} operaciones]<

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lección 2.1.2: LOS REGISTROS NOMINALES


 ** Almacenar dos palabras copiadas de manera consecutiva y después pegarlas **

  1. Mueve el cursor a la línea inferior marcada con --->

  2. Situa el cursor en cualquier parte de la palabra 'Edward' y escribe "ayiw

REGLA NEMOTÉCNICA: dentro del registo (") llamado (a) (y)copia
                   (i)entera la (w)palabra

  3. Mueve el cursor a la palabra 'galletas' (ft o 2fg o $1b o /lle<ENTER>)
     y escribe   "byiw

  4. Situa el cursos en cualquier parte de la palabra 'Vince'
     y escribe  ciw<CTRL-R>a<ESC>

REGLA NEMOTÉCNICA: (c)ambia el (i)interior de la (w)palabra
                   por el <contenido del (r)egistro> llamado (a)

  5. Navega hasta cualquier punto de la palabra 'tarta'
     y escribe  ciw<CTRL-R>b<ESC>

--->  a) Edward se encargará de las raciones de galletas
      b) En esta función, Vince solo tentrá poderes sobre la tarta

NOTA: Eliminar también funciona dentro de los registros, por ejemplo:
      "sdiw  eliminará la palabra bajo el cursor en el registro s.

REFERENCIAS: 	Registros 		:h registers
		Registros nominales     :h quotea
		Movimiento		:h motion.txt<ENTER> /inner<ENTER>
		CTRL-R			:h insert<ENTER> /CTRL-R<ENTER>

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lección 2.1.3: EL REGISTRO DE EXPRESIÓN


     ** Insertar los resultados de los cálculos sobre la marcha **

  1. Situa el cursor sobre la línea inferior marcada con --->

  2. Navega hasta cualquier parte del número que se muestra

  3. Escribe ciw<CTRL-R> seguido por =60*60*24<ENTER>

  4. En la línea siguiente, entra en modo insertar y añade la fecha actual con
     <CTRL-R> seguido por  =system('date')<ENTER>

NOTA: Todas las llamadas al sistema dependen del sistema operativo utilizado.
      Por ejemplo en Windows hay que usar  system('date /t')   o  :r!date /t

---> He olvidado el número exacto de segundos en un días ¿son 84600?
     La fecha actual es:

NOTA: se puede obtener el mismo resultado con  :pu=system('date')
      o, usando menos pulsaciones de teclas  :r!date

REFERENCIA: 	Registro de expresión	:h quote=

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lección 2.1.4: LOS REGISTROS NUMERADOS


   ** Pulsa  yy  y  dd  para ser testigo de sus efectos en los registros **

  1. Situa el cursor sobre la línea inferior marcada con --->

  2. Copia la línea marcada con 0,
     después revisa los registros mediante :reg<ENTER>

  3. Elimina la línea 0 mediante  "cdd, después revisa los registros
     (¿Dónde esperas que esté la línea 0?)

  4. Continúa eliminado cada línea sucesivamente,
     inspencciona :reg mientras lo haces

NOTA: Deberías comprobar cómo las líneas completas eliminadas
      van bajando en la lista, cada vez que nuevas líneas eliminadas se añaden

  5. Ahora (p)ega los siguiente registros en orden; 0, 7, 4, 2, 8. Así: "7p

---> 0. Este
     9. tambaleante
     8. secreto
     7. es
     6. en
     5. eje
     4. un
     3. guerra
     2. mensaje
     1. tributo

NOTA: La eliminación completa de líneas (dd) persisten más en los registros
      numerados que las copias completas de lína o las eliminaciones que
      implican pequeños movimientos

REFERENCIA: 	Registros numerados	:h quote0

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lección 2.1.5: REGISTROS ESPECIALES

 ** Utilizar el portapappeles del sistema y los registros de agujero negro **
                    ** para una edición avanzada **

 Nota: El uso del portapapeles del sistema requiere de bibliotecas X11/Wayland
       en sistemas Linux Y una compilación de Vim con la opción "+clipboard"
       (normalmente una compilación enorme). Se puede comprobar mediante
       ":version"  o ":echo has('clipboard_working')"

  1. Registros del portapapeles  +  and  *  :
     - "+y  - Copiar al portapapeles del sistema
       (Por ejemplo: "+yy para copiar en el portapapeles la línea actual)
     - "+p  - Pegar del portapapeles del sistema
     - "* es la selección principal de X11 (el botón central),
       "+ es el portapapeles

---> Prueba: "+yy después pega la línea en otra aplicación
	     mediante Ctrl-V o Cmd+V

  2. Blackhole register  _  discards text:
     - "_daw  - Delete word without saving to any register
     - Useful when you don't want to overwrite your default " register
     - Note this is using the "a Word" text object, introduced in a previous
       lession
     - "_dd   - Delete line without saving
     - "_dap  - Delete paragraph without saving
     - Combine with counts: 3"_dw

---> Practice: "_diw on any word to delete it without affecting yank history

  3. Combine with visual selections:
     - Select text with V then "+y
     - To paste from clipboard in insert mode: <CTRL-R>+
     - Try opening another application and paste from clipboard

  4. Remember:
     - Clipboard registers work across different Vim instances
     - Clipboard register is not always working
     - Blackhole prevents accidental register overwrites
     - Default " register is still available for normal yank/paste
     - Named registers (a-z) remain private to each Vim session

  5. Clipboard troubleshooting:
     - Check support with :echo has('clipboard_working')
     - 1 means available, 0 means not compiled in
     - On Linux, may need vim-gtk or vim-x11 package
       (check :version output)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lesson 2.1.6: THE BEAUTY OF MARKS

	           ** Code monkey arithmetic avoidance **

NOTE: a common conundrum when coding is moving around large chunks of code.
      The following technique helps avoid number line calculations associated
      with operations like   "a147d   or   :945,1091d a   or even worse using
      i<CTRL-R> followed by   =1091-945<ENTER>   first

  1. Move the cursor to the line below marked --->

  2. Go to the first line of the function and mark it with   ma

NOTE: exact position on line is NOT important!

  3. Navigate to the end of the line and then the end of the code block 
     with   $%

  4. Delete the block into register a with   "ad'a

MNEMONIC: into register(") named (a) put the (d)eletion from the cursor to the
          LINE containing mark(') (a)

  5. Paste the block between BBB and CCC   "ap

NOTE: practice this operation multiple times to become fluent   ma$%"ad'a

---> AAA
     function itGotRealBigRealFast() {
       if ( somethingIsTrue ) {
         doIt()
       }
       // the taxonomy of our function has changed and it
       // no longer makes alphabetical sense in its current position

       // imagine hundreds of lines of code

       // naively you could navigate to the start and end and record or
       // remember each line number
     }
     BBB
     CCC

NOTE: marks and registers do not share a namespace, therefore register a is
      completely independent of mark a. This is not true of registers and
      macros.

REFERENCE: 	Marks 		:h marks
		Mark Motions 	:h mark-motions  (difference between ' and `)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lesson 2.1 SUMMARY

  1. Text objects provide precision editing:
     - iw/aw - inner/around word
     - i[/a[ - inner/around bracket
     - i"/a" - inner/around quotes
     - it/at - inner/around tag
     - ip/ap - inner/around paragraph
     - is/as - inner/around sentence

  2. To store (yank, delete) text into, and retrieve (paste) from, a total of
     26 registers (a-z) 
  3. Yank a whole word from anywhere within a word:   yiw
  4. Change a whole word from anywhere within a word:   ciw
  5. Insert text directly from registers in insert mode:   <CTRL-R>a

  6. Insert the results of simple arithmetic operations: <CTRL-R> followed by
     =60*60<ENTER>
     in insert mode
  7. Insert the results of system calls: <CTRL-R> followed by
     =system('ls -1')<ENTER>
     in insert mode

  8. Inspect registers with   :reg
  9. Learn the final destination of whole line deletions: dd in the numbered
     registers, i.e. descending from register 1 - 9.  Appreciate that whole
     line deletions are preserved in the numbered registers longer than any
     other operation
 10. Learn the final destination of all yanks in the numbered registers and
     how ephemeral they are

 11. Place marks from command mode   m[a-zA-Z0-9]
 12. Move line-wise to a mark with   '

 13. Special registers:
     - "+/"*  - System clipboard (OS dependent)
     - "_     - Blackhole (discard deleted/yanked text)
     - "=     - Expression register

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  This concludes chapter two of the Vim Tutor. It is a work in progress.

  This chapter was written by Paul D. Parker and Christian Brabandt.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
