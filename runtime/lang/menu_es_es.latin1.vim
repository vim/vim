" Menu Translations:	Español
" Maintainer:		Alejandro López-Valencia <dradul@users.sourceforge.net>
" Version:		6.4.p0-1
" Last Change:		2005 Dec 01
"

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1

" The translations below are in latin1, but they work for cp1252,
" iso-8859-15 without conversion as well.
if &enc != "cp1252" && &enc != "iso-8859-15"
  scriptencoding latin1
endif


" Help menu
menutrans &Help			Ay&uda
menutrans &Overview<Tab><F1>	Tabla\ de\ &contenidos<Tab><F1>
menutrans &User\ Manual		&Manual\ del\ usuario
menutrans &How-to\ links	&Enlaces\ a\ ¿Cómo\ hago\.\.\.?
menutrans &Find\.\.\.		&Buscar\ en\ la\ ayuda
menutrans &Credits		&Reconocimientos
menutrans O&rphans		Ayude\ a\ los\ niños\ &huérfanos
menutrans Co&pying		&Términos\ de\ Licencia
menutrans Sponsor/Register	Benefactor/Regístrese
menutrans &Version		&Versión\ e\ \información\ de\ configuración
menutrans &About		&Acerca\ de\ Vim.

" File menu
menutrans &File				&Archivo
menutrans &Open\.\.\.<Tab>:e		&Abrir\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	A&brir\ en\ otra\ ventana\.\.\.<Tab>:sp
menutrans &New<Tab>:enew		&Nuevo<Tab>:enew
menutrans &Close<Tab>:close		&Cerrar<Tab>:close
menutrans &Save<Tab>:w			&Guardar<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	Guardar\ &como\.\.\.<Tab>:sav
menutrans Split\ &Diff\ with\.\.\.	&Mostrar\ diferencias\ con\.\.\.
menutrans Split\ Patched\ &By\.\.\.	Mostrar\ &parcheado\ por\.\.\.
menutrans &Print			&Imprimir
menutrans Sa&ve-Exit<Tab>:wqa		Gua&rdar\ y\ salir<Tab>:wqa
menutrans E&xit<Tab>:qa			&Salir<Tab>:qa

" Edit menu
menutrans &Edit				&Editar
menutrans &Undo<Tab>u			&Deshacer<Tab>u
menutrans &Redo<Tab>^R			&Rehacer<Tab>^R
menutrans Rep&eat<Tab>\.		Repe&tir<Tab>\.
menutrans Cu&t<Tab>"+x			Cor&tar<Tab>"+x
menutrans &Copy<Tab>"+y			&Copiar<Tab>"+y
menutrans &Paste<Tab>"+gP		&Pegar<Tab>"+gP
menutrans Put\ &Before<Tab>[p		Poner\ &antes<Tab>[p
menutrans Put\ &After<Tab>]p		Poner\ &después<Tab>]p
if has("win32") || has("win16")
  menutrans &Delete<Tab>x		S&uprimir<Tab>x
endif
menutrans &Select\ all<Tab>ggVG		&Seleccionar\ todo<Tab>ggVG
menutrans &Find\.\.\.			&Buscar\.\.\.
menutrans &Find<Tab>/			&Buscar<Tab>/
menutrans Find\ and\ Rep&lace\.\.\.     Buscar\ y\ R&eemplazar\.\.\.
menutrans Find\ and\ Rep&lace<Tab>:%s	Buscar\ y\ R&eemplazar<Tab>:%s
menutrans Find\ and\ Rep&lace		Buscar\ y\ R&eemplazar
menutrans Find\ and\ Rep&lace<Tab>:s	Buscar\ y\ R&eemplazar<Tab>:s
menutrans Settings\ &Window		&Ventana\ de\ opciones

" Edit/Global Settings
menutrans &Global\ Settings		Opciones\ &globales

menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!	Activar/Desactivar\ &realzado\ de\ sintaxis<Tab>:set\ hls!
menutrans Toggle\ &Ignore-case<Tab>:set\ ic!		Activar/Desactivar\ &ignorar\ mayúsculas\ y\ minúsculas<Tab>:set\ ic!
menutrans Toggle\ &Showmatch<Tab>:set\ sm!		Activar/Desactivar\ &mostrar\ coincidencias<Tab>:set\ sm!

menutrans &Context\ lines		Líneas\ de\ &contexto

menutrans &Virtual\ Edit		Edición\ &virtual
menutrans Never				Nunca
menutrans Block\ Selection		Selección\ de\ bloque
menutrans Insert\ mode			Modo\ de\ inserción
menutrans Block\ and\ Insert		Bloque\ e\ inserción
menutrans Always			Siempre

menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!	Activar/Desactivar\ modo\ de\ in&serción<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatible<Tab>:set\ cp!	Activar/Desactivar\ compatiblidad\ con\ Vi<Tab>:set\ cp!

menutrans Search\ &Path\.\.\.		Ruta\ de\ &búsqueda\.\.\.

menutrans Ta&g\ Files\.\.\.		Ficheros\ de\ &etiquetas\.\.\.

" GUI options
menutrans Toggle\ &Toolbar		Ocultar/Mostrar\ barra\ de\ &herramientas
menutrans Toggle\ &Bottom\ Scrollbar	Ocultar/Mostrar\ barra\ de\ desplazamiento\ &inferior
menutrans Toggle\ &Left\ Scrollbar	Ocultar/Mostrar\ barra\ de\ desplazamiento\ i&zquierda
menutrans Toggle\ &Right\ Scrollbar	Ocultar/Mostrar\ barra\ de\ desplazamiento\ &derecha

let g:menutrans_path_dialog = "Introduzca la ruta de búsqueda para los ficheros.\nSepare los nombres de los directorios con una coma."
let g:menutrans_tags_dialog = "Introduzca los nombres de los fichero de tags.\nSepare los nombres con una coma."

" Edit/File Settings
menutrans F&ile\ Settings		Opciones\ del\ &fichero

" Boolean options
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	Activar/Desactivar\ &numeración\ de\ líneas<Tab>:set\ nu!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		Activar/Desactivar\ modo\ de\ lista<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrap<Tab>:set\ wrap!		Activar/Desactivar\ &quiebre\ de\ líneas<Tab>:set\ wrap!
menutrans Toggle\ W&rap\ at\ word<Tab>:set\ lbr!	Activar/Desactivar\ quiebre\ entre\ &palabras<Tab>:set\ lbr!
menutrans Toggle\ &expand-tab<Tab>:set\ et!		Activar/Desactivar\ &expansión\ de\ marcas\ de\ \tabulado<Tab>:set\ et!
menutrans Toggle\ &auto-indent<Tab>:set\ ai!		Activar/Desactivar\ &auto-sangrado<Tab>:set\ ai!
menutrans Toggle\ &C-indenting<Tab>:set\ cin!		Activar/Desactivar\ sangrado\ &C<Tab>:set\ cin!

" other options
menutrans &Shiftwidth			Anchura\ del\ &sangrado

menutrans Soft\ &Tabstop		&Tabulado\ «blando»

menutrans Te&xt\ Width\.\.\.		Anchura\ del\ te&xto\.\.\.
let g:menutrans_textwidth_dialog = "Introduzca el nuevo ancho del texto (0 para desactivar el quiebre de línea): "

menutrans &File\ Format\.\.\.		&Formato\ del\ fichero\.\.\.
let g:menutrans_fileformat_dialog = "Seleccione el formato para escribir el fichero"

menutrans C&olor\ Scheme		Esquema\ de\ c&olores
menutrans blue		azul
menutrans darkblue	azul\ oscuro
menutrans default	original
menutrans desert	desierto
menutrans evening	vespertino
menutrans morning	matutino
menutrans peachpuff	melocotón
menutrans shine		brillante

menutrans Select\ Fo&nt\.\.\.		Seleccionar\ fue&nte\.\.\.

menutrans &Keymap	Asociación\ de\ teclados
menutrans None		Ninguna
menutrans accents	acentos
menutrans arabic	árabe
menutrans czech		checo
menutrans greek		griego
menutrans hebrew	hebreo
menutrans hebrewp	hebreo\ fonético
menutrans lithuanian-baltic	lituano-báltico
menutrans russian-jcuken	ruso-«jcuken»
menutrans russian-jcukenwin	ruso-«jcuken»\ Windows
menutrans russian-yawerty	ruso-«yawerty»
menutrans serbian-latin		serbio-latino
menutrans serbian	serbio
menutrans slovak	eslovaco


" Programming menu
menutrans &Tools			&Herramientas
menutrans &Jump\ to\ this\ tag<Tab>g^]	&Saltar\ a\ este\ etiqueta<Tab>g^]
menutrans Jump\ &back<Tab>^T		Saltar\ &atrás<Tab>^T
menutrans Build\ &Tags\ File		Crear\ fichero\ de\ &etiquetas\
menutrans &Diff				Modo\ de\ &diferencias
menutrans &Folding			&Plegado
menutrans &Make<Tab>:make		Ejecutar\ «&Make»<Tab>:make
menutrans &List\ Errors<Tab>:cl		&Lista\ de\ errores<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!	L&ista\ de\ mensajes<Tab>:cl!
menutrans &Next\ Error<Tab>:cn		&Error\ siguiente<Tab>:cn
menutrans &Previous\ Error<Tab>:cp	Error\ p&revio<Tab>:cp
menutrans &Older\ List<Tab>:cold	Lista\ de\ &viejos\ a\ nuevos<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew	Lista\ de\ &nuevos\ a\ viejos<Tab>:cnew
menutrans Error\ &Window		Ven&tana\ de\ errores
menutrans &Set\ Compiler		Esco&ger\ el\ compilador\ a\ usar
menutrans &Convert\ to\ HEX<Tab>:%!xxd	Convertir\ a\ formato\ &hexadecimal<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!xxd\ -r	&Convertir\ al\ formato\ original<Tab>:%!xxd\ -r

" Tools.Fold Menu
menutrans &Enable/Disable\ folds<Tab>zi		&Activar/Desactivar\ pliegues<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv		&Ver\ línea\ del\ cursor<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx	Ve&r\ sólo\ la\ línea\ del\ cursor<Tab>zMzx
menutrans C&lose\ more\ folds<Tab>zm		C&errar\ más\ pliegues<Tab>zm
menutrans &Close\ all\ folds<Tab>zM		&Cerrar\ todos\ los\ pliegues<Tab>zM
menutrans O&pen\ more\ folds<Tab>zr		Abrir\ &más\ pliegues<Tab>zr
menutrans &Open\ all\ folds<Tab>zR		&Abrir\ todos\ los\ pliegues<Tab>zR
" fold method
menutrans Fold\ Met&hod				&Método\ de\ plegado
" create and delete folds
menutrans Create\ &Fold<Tab>zf			Crear\ &pliegue<Tab>zf
menutrans &Delete\ Fold<Tab>zd			&Suprimir\ pliegue<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD		Suprimir\ &todos\ los\ pligues<Tab>zD
" moving around in folds
menutrans Fold\ col&umn\ width			A&nchura\ de\ columna\ del\ pliegue

" Tools.Diff Menu
menutrans &Update	&Actualizar
menutrans &Get\ Block	&Obtener\ bloque
menutrans &Put\ Block	&Poner\ bloque

"Tools.Error Menu
menutrans &Update<Tab>:cwin	&Actualizar
menutrans &Open<Tab>:copen	A&brir
menutrans &Close<Tab>:cclose	&Cerrar

" Names for buffer menu.
menutrans &Buffers		&Buffers
menutrans &Refresh\ menu	&Refrescar\ menú
menutrans &Delete		&Suprimir
menutrans &Alternate		&Alternar
menutrans &Next			Si&guiente
menutrans &Previous		&Previo
let g:menutrans_no_file = "[Sin fichero]"

" Window menu
menutrans &Window			&Ventana
menutrans &New<Tab>^Wn			Ventana\ &nueva<Tab>^Wn
menutrans S&plit<Tab>^Ws		&Dividir\ la\ ventana<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^	D&ividir\ en\ el\ marcador\ (#)<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv    Dividir\ &verticalmente<Tab>^Wv
menutrans Split\ File\ E&xplorer	&Abrir\ el\ «Explorador\ de\ ficheros»
menutrans &Close<Tab>^Wc		&Cerrar\ esta\ ventana<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo	Cerrar\ &otra(s)\ ventana(s)<Tab>^Wo
menutrans Move\ &To			Mov&er\ a
menutrans &Top<Tab>^WK			&Arriba<Tab>^WK
menutrans &Bottom<Tab>^WJ		A&bajo<Tab>^WJ
menutrans &Left\ side<Tab>^WH		Lado\ &izquierdo<Tab>^WH
menutrans &Right\ side<Tab>^WL		Lado\ &derecho<Tab>^WL
menutrans Rotate\ &Up<Tab>^WR		&Rotar\ hacia\ arriba<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		Rotar\ hacia\ a&bajo<Tab>^Wr
menutrans &Equal\ Size<Tab>^W=		Mismo\ &tamaño<Tab>^W=
menutrans &Max\ Height<Tab>^W_		Altura\ &máxima<Tab>^W_
menutrans M&in\ Height<Tab>^W1_		Altura\ mí&nima<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|		Anchura\ má&xima<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|		Anc&hura\ mínima<Tab>^W1\|

" The popup menu
menutrans &Undo			&Deshacer
menutrans Cu&t			Cor&tar
menutrans &Copy			&Copiar
menutrans &Paste		&Pegar
menutrans &Delete		&Borrar
menutrans Select\ Blockwise	Seleccionar\ por\ bloque
menutrans Select\ &Word		Seleccionar\ &palabra
menutrans Select\ &Line		Seleccionar\ una\ &línea
menutrans Select\ &Block	Seleccionar\ un\ &bloque
menutrans Select\ &All		Seleccionar\ &todo

" The GUI toolbar
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		Abrir fichero
    tmenu ToolBar.Save		Guardar fichero
    tmenu ToolBar.SaveAll	Guardar todos los ficheros
    tmenu ToolBar.Print		Imprimir
    tmenu ToolBar.Undo		Deshacer
    tmenu ToolBar.Redo		Rehacer
    tmenu ToolBar.Cut		Cortar
    tmenu ToolBar.Copy		Copiar
    tmenu ToolBar.Paste		Pegar
    tmenu ToolBar.Find		Buscar...
    tmenu ToolBar.FindNext	Buscar siguiente
    tmenu ToolBar.FindPrev	Buscar precedente
    tmenu ToolBar.Replace	Buscar y reemplazar
    if 0	" disabled; These are in the Windows menu
      tmenu ToolBar.New		Ventana nueva
      tmenu ToolBar.WinSplit	Dividir ventana
      tmenu ToolBar.WinMax	Altura máxima
      tmenu ToolBar.WinMin	Altura mínima
      tmenu ToolBar.WinVSplit	Dividir verticalmente
      tmenu ToolBar.WinMaxWidth	Anchura máxima
      tmenu ToolBar.WinMinWidth	Anchura mínima
      tmenu ToolBar.WinClose	Cerrar ventana
    endif
    tmenu ToolBar.LoadSesn	Cargar sesión
    tmenu ToolBar.SaveSesn	Guardar sesión
    tmenu ToolBar.RunScript	Ejecutar un archivo de órdenes
    tmenu ToolBar.Make		Ejecutar «Make»
    tmenu ToolBar.Shell		Abrir un intérprete de comandos
    tmenu ToolBar.RunCtags	Generar un fichero de etiquetas
    tmenu ToolBar.TagJump	Saltar a una etiqueta
    tmenu ToolBar.Help		Ayuda
    tmenu ToolBar.FindHelp	Buscar en la ayuda...
  endfun
endif

" Syntax menu
menutrans &Syntax			&Sintaxis
menutrans &Show\ filetypes\ in\ menu	&Mostrar\ listas\ de\ «tipo\ de\ fichero»
menutrans Set\ '&syntax'\ only		Activar\ sólo\ sintaxis
menutrans Set\ '&filetype'\ too		Activar\ también\ «tipo\ de\ fichero»
menutrans &Off				&Desactivar\ sintaxis
menutrans &Manual			sintaxis\ &manual
menutrans A&utomatic			sintaxis\ a&utomática
menutrans on/off\ for\ &This\ file	Activar/Desactivar\ en\ es&te\ fichero
menutrans Co&lor\ test			&Prueba\ de\ colores
menutrans &Highlight\ test		Prueba\ de\ &realzado
menutrans &Convert\ to\ HTML		&Convertir\ a\ HTML

" Find Help dialog text
let g:menutrans_help_dialog = "Introduzca un nombre de comando o palabra para obtener ayuda;\n\nAnteponga i_ para comandos de entrada (e.g.: i_CTRL-X)\nAnteponga c_ para comandos de la línea de comandos (e.g.: c_<Del>)\nAnteponga ` para un nombre de opción (e.g.: `shiftwidth`)"
