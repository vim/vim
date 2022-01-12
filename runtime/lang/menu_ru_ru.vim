" Menu Translations:	Russian
" Maintainer:		Restorer <restorers@users.sourceforge.net>
" Previous Maintainer:	Sergey Alyoshin <alyoshin.s@gmail.com>
"			vassily ragosin <vrr[at]users.sourceforge.net>
" Last Change:		18 Aug 2020
" URL:			
"
"
" Adopted for RuVim project by Vassily Ragosin.
" First translation: Tim Alexeevsky <realtim [at] mail.ru>,
" based on ukrainian translation by Bohdan Vlasyuk <bohdan@vstu.edu.ua>
"
"
" Quit when menu translations have already been done.
"
" Check is
"
if exists("did_menu_trans")
   finish
endif
let did_menu_trans = 1
let s:keepcpo= &cpo
set cpo&vim

scriptencoding utf-8

" Top
menutrans &File				&Файл
menutrans &Edit				&Правка
menutrans &Tools			С&ервис
menutrans &Syntax			Син&таксис
menutrans &Buffers			&Буферы
menutrans &Window			&Окно
menutrans &Help				&Справка
"
"
"
" Submenu of menu Help
menutrans &Overview<Tab><F1>		&Содержание<Tab><F1>
menutrans &User\ Manual			&Руководство\ пользователя
menutrans &How-to\ links		&Инструкции
menutrans &Find\.\.\.			&Найти\.\.\.
"--------------------
menutrans &Credits			Со&авторы
menutrans Co&pying			&Лицензия
menutrans &Sponsor/Register		Сод&ействие\ и\ регистрация
menutrans O&rphans			&Помочь\ детям
"--------------------
menutrans &Version			&Текущая\ версия
menutrans &About			&О\ программе
"
"
" Submenu of File menu
menutrans &Open\.\.\.<Tab>:e		&Открыть\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	От&крыть\ в\ новом\ окне\.\.\.<Tab>:vsplit
menutrans Open\ Tab\.\.\.<Tab>:tabnew	Откры&ть\ в\ новой\ вкладке\.\.\.<Tab>:tabnew
menutrans &New<Tab>:enew		Созд&ать<Tab>:enew
menutrans &Close<Tab>:close		&Закрыть<Tab>:close
"--------------------
menutrans &Save<Tab>:w			&Сохранить<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	Со&хранить\ как\.\.\.<Tab>:sav
"--------------------
menutrans Split\ &Diff\ with\.\.\.	Сра&внить\ с\.\.\.
menutrans Split\ Patched\ &By\.\.\.	Сравн&ить\ и\ исправить\.\.\.
"--------------------
menutrans &Print			&Печать\.\.\.
menutrans Sa&ve-Exit<Tab>:wqa		Сохра&нить\ и\ выйти<Tab>:wqa
menutrans E&xit<Tab>:qa			В&ыход<Tab>:qa
"
"
" Submenu of Edit menu
menutrans &Undo<Tab>u			&Отменить<Tab>u
menutrans &Redo<Tab>^R			В&ернуть<Tab>CTRL+R
menutrans Rep&eat<Tab>\.		Повторит&ь<Tab>\.
"--------------------
menutrans Cu&t<Tab>"+x			&Вырезать<Tab>"+x
menutrans &Copy<Tab>"+y			&Копировать<Tab>"+y
menutrans &Paste<Tab>"+gP		Вст&авить<Tab>"+g\ SHIFT+P
menutrans Put\ &Before<Tab>[p		Поместить\ п&еред<Tab>[p
menutrans Put\ &After<Tab>]p		Поместить\ по&сле<Tab>]p
menutrans &Delete<Tab>x			&Удалить<Tab>x
menutrans &Select\ All<Tab>ggVG		В&ыделить\ всё<Tab>gg\ SHIFT+V\ SHIFT+G
"--------------------
" Athena GUI only
menutrans &Find<Tab>/			&Найти<Tab>/
menutrans Find\ and\ Rep&lace<Tab>:%s	&Заменить<Tab>:%s
" End Athena GUI only
menutrans &Find\.\.\.<Tab>/		&Найти\.\.\.<Tab>/
menutrans Find\ and\ Rep&lace\.\.\.	&Заменить\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.<Tab>:%s	&Заменить\.\.\.<Tab>:%s
menutrans Find\ and\ Rep&lace\.\.\.<Tab>:s	&Заменить\.\.\.<Tab>:s
"--------------------
menutrans Settings\ &Window		&Все\ параметры\.\.\.
menutrans Startup\ &Settings		Параметры\ запус&ка
menutrans &Global\ Settings		&Общие\ параметры
menutrans F&ile\ Settings		Параметр&ы\ текущего\ буфера
menutranslate Show\ C&olor\ Schemes\ in\ Menu	Показать\ меню\ выбора\ цве&товой\ схемы
menutrans C&olor\ Scheme		Цветовая\ с&хема
menutranslate Show\ &Keymaps\ in\ Menu	Показать\ меню\ выбора\ раскладки\ к&лавиатуры
menutrans &Keymap			&Раскладка\ клавиатуры
menutrans Select\ Fo&nt\.\.\.		&Шрифт\.\.\.
">>>----------------- Edit/Global settings
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!		Подсвечивать\ &совпадения<Tab>:set\ hls!
menutrans Toggle\ &Ignoring\ Case<Tab>:set\ ic!			&Регистронезависимый\ поиск<Tab>:set\ ic!
menutrans Toggle\ &Showing\ Matched\ Pairs<Tab>:set\ sm!	Показывать\ парные\ &элементы<Tab>:set\ sm!
menutrans &Context\ lines				Контекстных\ стр&ок\ для\ текущей\ строки
menutrans &Virtual\ Edit				Вир&туальное\ редактирование
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!		Режим\ &вставки<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatibility<Tab>:set\ cp!	&Совместимость\ с\ редактором\ Vi<Tab>:set\ cp!
menutrans Search\ &Path\.\.\.				&Каталоги\ для\ поиска\ файлов\.\.\.
menutrans Ta&g\ Files\.\.\.				Индексные\ файлы\.\.\.
"
menutrans Toggle\ &Toolbar				Показ\ &панели\ инструментов
menutrans Toggle\ &Bottom\ Scrollbar			Показ\ полосы\ прокрутки\ вни&зу
menutrans Toggle\ &Left\ Scrollbar			Показ\ полосы\ прокрутки\ с&лева
menutrans Toggle\ &Right\ Scrollbar			Показ\ полосы\ прокрутки\ спр&ава
">>>->>>------------- Edit/Global settings/Virtual edit
menutranslate Never				Выключено\ во\ всех\ режимах
menutranslate Block\ Selection		Включено\ в\ режиме\ визуального\ блока
menutranslate Insert\ mode			Включено\ в\ режиме\ вставки
menutranslate Block\ and\ Insert		Включено\ в\ режимах\ визуального\ блока\ и\ вставки
menutranslate Always				Включено\ во\ всех\ режимах
">>>----------------- Edit/File settings
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	Показ\ &нумерации\ строк<Tab>:set\ nu!
menutrans Toggle\ relati&ve\ Line\ Numbering<Tab>:set\ rnu!	Показ\ относите&льной\ нумерации\ строк<Tab>:set\ nru!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		Показ\ &непечатаемых\ знаков<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrapping<Tab>:set\ wrap!	&Разбивка\ строк\ по\ границе\ окна<Tab>:set\ wrap!
menutrans Toggle\ W&rapping\ at\ word<Tab>:set\ lbr!	Разбивка\ строк\ по\ &границе\ слов<Tab>:set\ lbr!
menutrans Toggle\ Tab\ &Expanding<Tab>:set\ et!		Заменить\ символы\ &табуляции\ на\ пробелы<Tab>:set\ et!
menutrans Toggle\ &Auto\ Indenting<Tab>:set\ ai!	Установить\ отступ\ как\ у\ текущей\ &строки<Tab>:set\ ai!
menutrans Toggle\ &C-Style\ Indenting<Tab>:set\ cin!	Установить\ отступ\ как\ в\ &языке\ Си<Tab>:set\ cin!
">>>---
menutrans &Shiftwidth					Вели&чина\ отступа
menutrans Soft\ &Tabstop				Ширина\ &табуляции
menutrans Te&xt\ Width\.\.\.				&Ширина\ текста\.\.\.
menutrans &File\ Format\.\.\.				&Формат\ файла\.\.\.
"
"
"
" Submenu of Tools menu
menutrans &Jump\ to\ this\ tag<Tab>g^]			&Перейти\ по\ индексному\ указателю<Tab>g\ CTRL+]
menutrans Jump\ &back<Tab>^T				&Вернуться\ назад<Tab>CTRL+T
menutrans Build\ &Tags\ File				Создать\ ф&айл\ индексов
"-------------------
menutrans &Folding					Структура\ текста
menutrans &Spelling					Проверка\ пр&авописания
menutrans &Diff						&Сравнение\ текста
"-------------------
menutrans &Make<Tab>:make				Ко&мпиляция<Tab>:make
menutrans &List\ Errors<Tab>:cl				Список\ результатов<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!			Список\ все&х\ сообщений<Tab>:cl!
menutrans &Next\ Error<Tab>:cn				Следу&ющая\ запись\ из\ списка<Tab>:cn
menutrans &Previous\ Error<Tab>:cp			Пр&едыдущая\ запись\ из\ списка<Tab>:cp
menutrans &Older\ List<Tab>:cold			Более\ стар&ый\ список\ результатов<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew			Более\ све&жий\ список\ результатов<Tab>:cnew
menutrans Error\ &Window				Ок&но\ со\ списком\ результатов
menutranslate Show\ Compiler\ Se&ttings\ in\ Menu	Показать\ меню\ выбора\ &компилятора
menutrans Se&T\ Compiler				Выбрать\ &компилятор
"-------------------
menutrans &Convert\ to\ HEX<Tab>:%!xxd			Преобразовать\ в\ HEX<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!xxd\ -r			Преобразовать\ и&з\ HEX<Tab>:%!xxd\ -r
">>>---------------- Tools/Spelling
menutrans &Spell\ Check\ On				&Проверять
menutrans Spell\ Check\ &Off				&Без\ проверки
menutrans To\ &Next\ error<Tab>]s			С&ледующая\ ошибка<Tab>]s
menutrans To\ &Previous\ error<Tab>[s			Пр&едыдущая\ ошибка<Tab>[s
menutrans Suggest\ &Corrections<Tab>z=                  Варианты\ написания<Tab>z=
menutrans &Repeat\ correction<Tab>:spellrepall		Заменить\ все<Tab>:spellrepall
"-------------------
menutranslate Set\ language\ to\ "en"			Включить\ для\ языка\ "en"
menutranslate Set\ language\ to\ "en_au"		Включить\ для\ языка\ "en_au"
menutranslate Set\ language\ to\ "en_ca"		Включить\ для\ языка\ "en_ca"
menutranslate Set\ language\ to\ "en_gb"		Включить\ для\ языка\ "en_gb"
menutranslate Set\ language\ to\ "en_nz"		Включить\ для\ языка\ "en_nz"
menutranslate Set\ language\ to\ "en_us"		Включить\ для\ языка\ "en_us"
menutranslate &Find\ More\ Languages			Проверка\ для\ других\ &языков
let g:menutrans_set_lang_to =				'Включить проверку для языка'
">>>---------------- Folds
menutrans &Enable/Disable\ folds<Tab>zi			Показать\ или\ убрать\ структуру<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv			Просмотр\ строки\ под\ &курсором<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx		Просмотр\ &только\ строки\ под\ курсором<Tab>z\ SHIFT+M\ zx
menutrans C&lose\ more\ folds<Tab>zm			Свернуть\ вло&женные\ блоки\ структуры<Tab>zm
menutrans &Close\ all\ folds<Tab>zM			Свернуть\ &все\ блоки\ структуры<Tab>z\ SHIFT+M
menutrans &Open\ all\ folds<Tab>zR			Развернуть\ в&се\ блоки\ структуры<Tab>z\ SHIFT+R
menutrans O&pen\ more\ folds<Tab>zr			Ра&звернуть\ вложенные\ блоки\ структуры<Tab>zr
menutrans Fold\ Met&hod					&Метод\ разметки\ структуры
menutrans Create\ &Fold<Tab>zf				Со&здать\ блок\ структуры<Tab>zf
menutrans &Delete\ Fold<Tab>zd				&Убрать\ блок\ структуры<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD			Убрать\ вс&е\ блоки\ структуры<Tab>z\ SHIFT+D
menutrans Fold\ col&umn\ width				&Ширина\ столбца\ со\ значками\ структуры
">>>->>>----------- Tools/Folds/Fold Method
menutrans M&anual					Установить\ вру&чную
menutrans I&ndent					По\ о&тступам
menutrans E&xpression					По\ &вычисленным\ условиям
menutrans S&yntax					По\ &синтаксису
menutranslate &Diff                                     По\ различиям\ в\ тексте
menutrans Ma&rker					По\ &маркерам
">>>--------------- Sub of Tools/Diff
menutrans &Update					О&бновить\ содержимое\ окон
menutrans &Get\ Block					Перенести\ в\ текущий\ буфер
menutrans &Put\ Block					Перенести\ из\ текущего\ буфера
">>>--------------- Tools/Diff/Error window
menutrans &Update<Tab>:cwin				О&бновить<Tab>:cwin
menutrans &Close<Tab>:cclose				&Закрыть<Tab>:cclose
menutrans &Open<Tab>:copen				&Открыть<Tab>:copen
"
"
" Syntax menu
"
menutrans &Show\ File\ Types\ in\ menu			Показать\ меню\ выбора\ типа\ &файла
menutrans Set\ '&syntax'\ only				&Задать\ значение\ только\ 'syntax'
menutrans Set\ '&filetype'\ too				Задать\ &также\ значение\ 'filetype'
menutrans &Off						&Отключить\ подсветку
menutrans &Manual					Включать\ подсветку\ вру&чную
menutrans A&utomatic					Включать\ подсветку\ &автоматически
menutrans on/off\ for\ &This\ file			Переключить\ режим\ для\ текущего\ файла
menutrans Co&lor\ test					Проверка\ &цветов
menutrans &Highlight\ test				Проверка\ под&светки
menutrans &Convert\ to\ HTML				С&оздать\ HTML-файл\ с\ CSS
"
"
" Buffers menu
"
menutrans &Refresh\ menu				&Обновить\ меню
menutrans &Delete					&Удалить\ буфер
menutrans &Alternate					&Соседний\ буфер
menutrans &Next						С&ледующий\ буфер
menutrans &Previous					&Предыдущий\ буфер
"
"
" Submenu of Window menu
"
menutrans &New<Tab>^Wn					&Создать<Tab>CTRL+W\ n
menutrans S&plit<Tab>^Ws				Разделить\ по\ &горизонтали<Tab>CTRL+W\ s
menutrans Split\ &Vertically<Tab>^Wv			Разделить\ по\ &вертикали<Tab>CTRL+W\ v
menutrans Sp&lit\ To\ #<Tab>^W^^			С&оседний\ файл\ в\ новом\ окне<Tab>CTRL+W\ CTRL+^
menutrans Split\ File\ E&xplorer			Диспетчер\ файлов
"
menutrans &Close<Tab>^Wc				&Закрыть\ текущее\ окно<Tab>CTRL+W\ c
menutrans Close\ &Other(s)<Tab>^Wo			З&акрыть\ другие\ окна<Tab>CTRL+W\ o
"
menutrans Move\ &To					&Переместить
menutrans Rotate\ &Up<Tab>^WR				Сдвинуть\ ввер&х<Tab>CTRL+W\ SHIFT+R
menutrans Rotate\ &Down<Tab>^Wr				Сдвинуть\ в&низ<Tab>CTRL+W\ r
"
menutrans &Equal\ Size<Tab>^W=				Выровнять\ раз&мер<Tab>CTRL+W\ =
menutrans &Max\ Height<Tab>^W_				Максимальной\ в&ысоты<Tab>CTRL+W\ _
menutrans M&in\ Height<Tab>^W1_				Минимальной\ высо&ты<Tab>CTRL+W\ 1_
menutrans Max\ &Width<Tab>^W\|				Максимальной\ &ширины<Tab>CTRL+W\ \|
menutrans Min\ Widt&h<Tab>^W1\|				Минимальной\ ш&ирины<Tab>CTRL+W\ 1\|
">>>----------------- Submenu of Window/Move To
menutrans &Top<Tab>^WK					В&верх<Tab>CTRL+W\ SHIFT+K
menutrans &Bottom<Tab>^WJ				В&низ<Tab>CTRL+W\ SHIFT+J
menutrans &Left\ side<Tab>^WH				В&лево<Tab>CTRL+W\ SHIFT+H
menutrans &Right\ side<Tab>^WL				В&право<Tab>CTRL+W\ SHIFT+L
"
"
" The popup menu
"
"
menutrans &Undo						&Отменить
menutrans Cu&t						&Вырезать
menutrans &Copy						&Копировать
menutrans &Paste					Вст&авить
menutrans &Delete					&Удалить
menutrans Select\ Blockwise				Блоковое\ выделение
menutrans Select\ &Word					Выделить\ с&лово
menutrans Select\ &Line					Выделить\ с&троку
menutrans Select\ &Block				Выделить\ &блок
menutrans Select\ &All					В&ыделить\ всё
menutrans Select\ &Sentence				Выделить\ предло&жение
menutrans Select\ Pa&ragraph				Выделить\ аб&зац
"
" The Spelling popup menu
"
"
let g:menutrans_spell_change_ARG_to =			'Исправить\ "%s"\ на'
let g:menutrans_spell_add_ARG_to_word_list =		'Добавить\ "%s"\ в\ словарь'
let g:menutrans_spell_ignore_ARG =			'Пропустить\ "%s"'
"
" The GUI toolbar
"
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.New					Создать документ
    tmenu ToolBar.Open					Открыть файл
    tmenu ToolBar.Save					Сохранить файл
    tmenu ToolBar.SaveAll				Сохранить все файлы
    tmenu ToolBar.Print					Печать
    tmenu ToolBar.Undo					Отменить
    tmenu ToolBar.Redo					Вернуть
    tmenu ToolBar.Cut					Вырезать
    tmenu ToolBar.Copy					Копировать
    tmenu ToolBar.Paste					Вставить
    tmenu ToolBar.Find					Найти...
    tmenu ToolBar.FindNext				Найти следующее
    tmenu ToolBar.FindPrev				Найти предыдущее
    tmenu ToolBar.Replace				Заменить...
    tmenu ToolBar.NewSesn				Создать сеанс редактирования
    tmenu ToolBar.LoadSesn				Загрузить сеанс редактирования
    tmenu ToolBar.SaveSesn				Сохранить сеанс редактирования
    tmenu ToolBar.RunScript				Выполнить командный файл программы Vim
    tmenu ToolBar.Shell					Командная оболочка
    tmenu ToolBar.Make					Компиляция
    tmenu ToolBar.RunCtags				Создать файл индексов
    tmenu ToolBar.TagJump				Перейти по индексному указателю
    tmenu ToolBar.Help					Справка
    tmenu ToolBar.FindHelp				Поиск в справке
    tmenu ToolBar.WinClose				Закрыть текущее окно
    tmenu ToolBar.WinMax				Максимальная высота текущего окна
    tmenu ToolBar.WinMin				Минимальная высота текущего окна
    tmenu ToolBar.WinSplit				Разделить окно по горизонтали
    tmenu ToolBar.WinVSplit				Разделить окно по вертикали
    tmenu ToolBar.WinMaxWidth				Максимальная ширина текущего окна
    tmenu ToolBar.WinMinWidth				Минимальная ширина текущего окна
  endfun
endif
"
"
" Dialog texts
"
" Find in help dialog
"
let g:menutrans_help_dialog = "Укажите команду или слово, которые требуется найти в документации.\n\nЧтобы найти команды режима вставки, используйте приставку i_ (например, i_CTRL-X)\nЧтобы найти команды командной строки, используйте приставку c_ (например, c_<Del>)\nЧтобы найти информацию о параметрах, используйте символ ' (например, 'shftwidth')"
"
" Search path dialog
"
let g:menutrans_path_dialog = "Укажите через запятую наименования каталогов, где будет выполняться поиск файлов"
"
" Tag files dialog
"
let g:menutrans_tags_dialog = "Укажите через запятую наименования файлов индексов"
"
" Text width dialog
"
let g:menutrans_textwidth_dialog = "Укажите количество символов для установки ширины текста\nЧтобы отменить форматирование, укажите 0"
"
" File format dialog
"
let g:menutrans_fileformat_dialog = "Выберите формат файла"
let g:menutrans_fileformat_choices = "&1. Unix\n&2. Dos\n&3. Mac\nО&тмена"
"
let menutrans_no_file = "[Безымянный]"

" Menus to handle Russian encodings
" Thanks to Pavlo Bohmat for the idea
" vassily ragosin <vrr[at]users.sourceforge.net>
"
an 10.355 &File.-SEP-					<Nop>
an 10.360.20 &File.Открыть\ в\ кодировке\.\.\..CP1251	:browse e ++enc=cp1251<CR>
an 10.360.30 &File.Открыть\ в\ кодировке\.\.\..CP866	:browse e ++enc=cp866<CR>
an 10.360.30 &File.Открыть\ в\ кодировке\.\.\..KOI8-R	:browse e ++enc=koi8-r<CR>
an 10.360.40 &File.Открыть\ в\ кодировке\.\.\..UTF-8	:browse e ++enc=utf-8<CR>
an 10.365.20 &File.Сохранить\ с\ кодировкой\.\.\..CP1251	:browse w ++enc=cp1251<CR>
an 10.365.30 &File.Сохранить\ с\ кодировкой\.\.\..CP866	:browse w ++enc=cp866<CR>
an 10.365.30 &File.Сохранить\ с\ кодировкой\.\.\..KOI8-R	:browse w ++enc=koi8-r<CR>
an 10.365.40 &File.Сохранить\ с\ кодировкой\.\.\..UTF-8	:browse w ++enc=utf-8<CR>
"

let &cpo = s:keepcpo
unlet s:keepcpo
