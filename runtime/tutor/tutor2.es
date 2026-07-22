===============================================================================
=     B i e n v e n i d o   a l   t u t o r   d e   V I M  -  Versión 1.7     =
===============================================================================
=	                         CAPÍTULO DOS                                 =
===============================================================================

     Hic Sunt Dracones: si esta es tu primera vez usando Vim y tienes la
     intención de aprovechar el capítulo de introducción, simplemente escribe
     :q!<ENTER> y ejecuta vimtutor para empezar por el Capítulo 1.

     El tiempo aproximado para completar este capítulo es de 8-10 minutos,
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
     - Escribe  di(  o  dib  (eliminar dentro de los paréntesis)
     - Escribe  da(  o  dab  (eliminar alrededor de los paréntesis)
     - Prueba lo mismo con  i" o con  a"  para las comillas
     - Prueba con  it o con  at  para etiquetas HTML/XML

---> Ejemplos de prueba: con {llaves}, [corchetes], <ángulos> y "comillas".

  3. Manipulación de párrafos y frases:
     - Utiliza  dip  para eliminar el párrafo donde se encuentra el cursor
     - Utiliza  vap  para seleccionar visualmente el párrafo entero
     - Prueba  das  para eliminar una frase
       (funciona entre símbolos de puntuación .!?)

  4. Combinaciones avanzadas:
     - ciwnuevo<ESC>     - Cambiar la palabra actual por "nuevo"
     - ciw"<CTRL-R>-"<ESC> - Encierra la palabra actual entre comillas
     - gUit              - Convertir a mayúsculas el contenido de la
			               etiqueta HTML donde esté el cursor
     - va"p              - Seleccionar el texto entre comillas y pegarlo sobre él

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

  3. Escribe  ciw<CTRL-R>  seguido por =60*60*24<ENTER>

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

  2. El registro de agujero negro _  para texto descartado:
     - "_daw  - Elimina una palabra sin guardarla en ningún registro
     - Útil cuando no quieres sobreescribir el registro predeterminado "
     - Ten en cuenta que utiliza el objeto de texto "una Palabra" visto
       en la lección anterior
       lession
     - "_dd   - Elimina una línea sin guardarla
     - "_dap  - Elimina un párrafo sin almacenarlo
     - Combinado con un conteo: 3"_dw

---> Practica: Utiliza  "_diw en cualquier palabra para eliminarla sin afectar
               al historial de copiado

  3. Combinado con unas selecciones visuales:
     - Selecciona un texto con V y después pulsa "+y
     - Para pegarlo desde el portapapeles en el modo insertar pulsa: <CTRL-R>+
     - Intenta abrir otra aplicación y péga el texto desde el portapapeles

  4. Recuerda:
     - Los registros del portapapeles funcionan a través de diferentes
       instancias de Vim
     - El registro del portapapelesno siempre está funcional
     - El agujero negro previene el sobreescribir accidentalmente un registro
     - El registro predeterminado " todavía está disponible para una acción
       normal de copado/pegado
     - Los registros nominales (a-z) permanecen privados para cada sesión de Vim

  5. Problemas con el portapapeles:
     - Comprueba que tu Vim lo admite mediante :echo has('clipboard_working')
     - Si el comando anterior devuelve un 1 significa que está disponible,
       Si devuelve un 0, significa que esa compilación de Vim no lo admite.
     - En Linux, se puede necesitar el paquete vim-gtk o vim-x11 package
       (comprueba :version output)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lección 2.1.6: LA BELLEZA DE LAS MARCAS

		** El programador que rehuye de la aritmética **

NOTA: Un dilema común cuando se está creando código es mover grandes porciones
      de código.
      La siguiente técnica ayuda a evitar los cálculos asociados a los números
      de línea con operaciones como  "a147d   o   :945,1091d a   o o incluso peor
      i<CTRL-R> seguido por   =1091-945<ENTER>

  1. Mueve el cursor hasta la línea inferior marcada con --->

  2. Ve a la primera línea de la función marcada con   ma

NOTA: ¡La posición exacta dentro de la línea NO es importante!

  3. Navega hasta el final de la línea y después hasta el final del
     bloque del código mediante  $%

  4. Elimina el bloque y guárdalo dentro del registro a mediante  "ad'a

REGLA NEMOTÉCNICA: dentro del registro(") llamado (a) coloca lo que he
		   (d)eliminado desde el cursor hasta la LÍNEA que
                   contiene la marca(') (a)

  5. Pega el contenido del bloque entre BBB y CCC mediante   "ap

NOTA: practica esta operación varias veces hasta que tengas soltura   ma$%"ad'a

---> AAA
     function itGotRealBigRealFast() {
       if ( somethingIsTrue ) {
         doIt()
       }
       // La taxonomía de nuestra función ha cambiado y
       // Ya no tiene sentido alfabético en su posición actual

       // imagina que aquí hay cientos de líneas de código

       // de manera ingenua podrías navegar hasta el comienzo y hasta el final y
       // apuntar o recordar cada número de la línea
     }
     BBB
     CCC

NOTA: Las marcas y los registros no comparten sus nombres. Así pues el
      registro a es completamente independiente de la marca a.
      Esto no se cumple con los registros y las macros.

REFERENCIA: 	Marcas 				:h marks
		Movimientos con las marcas	:h mark-motions
						(diferencia entre ' y `)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		     Lección 2.1 RESUMEN

  1. Los objetos de texto ofrecen precisión a la hora de editar:
     - iw/aw - dentro/alrededor de una palabra
     - i[/a[ - dentro/alrededor de un paréntesis, corchete, etc
     - i"/a" - dentro/alrededor de unas comillas quotes
     - it/at - dentro/alrededor de una etiqueta
     - ip/ap - dentro/alrededor de un párrafo
     - is/as - dentro/alrededor de una frase

  2. Para almacenar (copiar o eliminar) un texto dentro y volverlo a utilizar
     (pegarlo), existen un total de 26 registros nominales (a-z)
  3. Copiar una palabra complete con el cursor situado en cualquierparte
     dentro de esa palabra mediante:   yiw
  4. Cambiar una palabra completa con el cursor en cualquier parte de esa
     palabra mediante:   ciw
  5. Insertar texto directamente desde los registros en el modo insertar
     mediante el comando:   <CTRL-R>a

  6. Insertar los resultado de una simple operación aritmética
     en el modo insertar mediante: <CTRL-R> seguido por  =60*60<ENTER>
  7. Insertar los resultado de una llamada del sistema en el modo insertar
     mediante: <CTRL-R> seguido por  =system('ls -1')<ENTER>

  8. Inspeccionar el contenido de los registros con   :reg
  9. Aprender el destino final al que va a parar la eliminación de
     una línea complete: dd en los registros numerados.
     Por ejemplo: descendiendo desde el registro 1 al 9. i.e. descending from register 1 - 9.
     Ten en cuenta cómo las eliminaciones completas de las líneas son preservadas en
     los registros numerado y permanecen más que cualquier otra operación realizada
 10. Aprender cual es el destino final de todos los objetos que copiamos
     en los registros numerados y cómo son de efímeros

 11. Ubicar marcas desde el modo de comandos   m[a-zA-Z0-9]
 12. Mover de manera inteligente una línea a una marca con   '

 13. Registros especiales:
     - "+/"*  - Portapapeles del sistema (depende el sistema operativo utilizado)
     - "_     - El agujero negro (descarta texto eliminado/copiado)
     - "=     - Registro de expresión

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Con esto concluye el capítulo dos del Tutor de Vim.
  Este es un trabajo en progreso.

  Este capítulo fue escrito por Paul D. Parker y Christian Brabandt.
  Traducido por Victorhck.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
