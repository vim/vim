" Translation options window: English
" Maintainer: Bram Moolenaar <Bram@vim.org> (Restorer <restorers@users.sf.org>)
" Last Change: 26 Aug 2020


if exists("did_optwin_trans")
  finish
endif
let did_optwin_trans = 1



" !!!  Change to the one that will be used in your file  !!!
scriptencoding utf-8



" The name of the file should be the following:
"
"               optwin_{language}[{.encoding}].vim
"
" optwin_    --- required part;
" {language} --- required part. Must be exactly the same as in the v:lang
"		 variable;
" {encoding} --- optional, but desirable, part. Specifies the character encoding
"                that this file will be used for.
"
" To find out more, read the file README.txt in the /lang directory

" Наименование файла должно быть следующего вида:
"
"               optwin_{язык}[{.кодировка}].vim
"
" optwin_     --- обязательная часть;
" {язык}      --- обязательная часть. Должна быть точно такой же, как в
"		  переменной v:lang;
" {кодировка} --- не обязательная, но желательная, часть. Указывает кодировку
"		  символов для которой будет применяться этот файл.
"
" Чтобы узнать подробности, читайте файл README.txt в каталоге /lang


" For translators:
" Important! The total length of the string must not exceed 79 characters.
" The string starts with a quotation mark.  Each new row is a new element in the
" array.  Each element of the array is enclosed in single quotes. All special
" characters will be printed as is.  There may be as many rows (array elements)
" as you need, but be careful.  When adding a new row (an array element), the
" backslash character is placed in front of it. 

" Для переводчиков:
" Важно! Общая длина строки не должна превышать 79 символов.
" Строка начинается с символа кавычки. Каждая новая строка - это новый элемент
" массива.  Каждый элемент массива заключается в одинарные кавычки.  Все
" специальные символы будут печататься как есть.  Строк (элементов массива)
" может быть столько, сколько нужно, но соблюдайте благоразумие.  При добавлении
" новой строки (элемента массива), перед ним ставится символ обратная наклонная
" черта. 

let s:banner=[
    \ '" Each "set" line shows the current value of an option (on the left).',
    \ '" Hit <CR> on a "set" line to execute it.',
    \ '"            A boolean option will be toggled.',
    \ '"            For other options you can edit the value before hitting <CR>.',
    \ '" Hit <CR> on a help line to open a help window on this option.',
    \ '" Hit <CR> on an index line to jump there.',
    \ '" Hit <Space> on a "set" line to refresh it.'
    \ ]


" For translators:
" Attention! Do not change the zero index of the array (the very first one). It
" must match the value in the English version of this file. If it doesn't match,
" it means something is wrong.
" Important! The total length of the string must not exceed 79 characters.
" Each new row is a new element in the array. Each element of the array is
" enclosed in single quotes. All special characters will be printed as is.
" The number of rows (array elements) must be the same as in the English
" version.

" Для переводчиков:
" Внимание! Нулевой индекс массива (самый первый) не изменять.  Он должен
" совпадать со значением в английской версии этого файла.  Если не совпадает,
" значит что-то не так.
" Важно! Общая длина строки не должна превышать 79 символов.
" Каждая новая строка - это новый элемент массива.  Каждый элемент массива
" заключается в одинарные кавычки.  Все специальные символы будут печататься как
" есть.
" Строк (элементов массива) должно быть столько же, сколько в английской версии.

let s:contents=[
"\ Since the content numbering starts with one, we will use the zero index for
"\ staff use. Let's write down the total number of content items here. 
    \ 27,
    \ 'important',
    \ 'moving around, searching and patterns',
    \ 'tags',
    \ 'displaying text',
    \ 'syntax, highlighting and spelling',
    \ 'multiple windows',
    \ 'multiple tab pages',
    \ 'terminal',
    \ 'using the mouse',
    \ 'GUI',
    \ 'printing',
    \ 'messages and info',
    \ 'selecting text',
    \ 'editing text',
    \ 'tabs and indenting',
    \ 'folding',
    \ 'diff mode',
    \ 'mapping',
    \ 'reading and writing files',
    \ 'the swap file',
    \ 'command line editing',
    \ 'executing external commands',
    \ 'running make and jumping to errors (quickfix)',
    \ 'system specific',
    \ 'language specific',
    \ 'multi-byte characters',
    \ 'various'
    \ ]


" For translators:
" Attention! Do not translate the dictionary key (word in single quotes before a
" colon). Only what is in square brackets is translated.
" Each line is enclosed in double quotes. All special characters are escaped by
" the backslash character.
" Important! The total length of a single line must not exceed 79 characters.
" Each new row is a individual element of the array in square brackets. It is
" separated from the previous one by a comma.  There may be as many rows
" (elements in square brackets) as you need, but be careful. The tab character
" is used to align the rows.
" The total number of entries in the dictionary must be the same as in the
" English version.

" Для переводчиков:
" Внимание! Ключ словаря (слово в одинарных кавычках перед двоеточием) не
" переводить. Переводится только то, что в квадратных скобках.
" Каждая строка заключается в двойные кавычки. Все специальные символы
" экранируются символом обратная наклонная черта.
" Важно! Общая длина одной строки не должна превышать 79 символов.
" Каждая новая строка - это отдельный элемент массива в квадратных скобках. Он
" отделяется от предыдущего символом запятая.  Строк (элементов в квадратных
" скобках) может быть столько, сколько требуется, но соблюдайте благоразумие.
" Для выравнивания строк используется символ табуляции.
" Общее количество записей в словаре должно быть столько же, сколько в
" английской версии.

let s:optdesc={
   "\ important
    \ 'compatible':["\tbehave very Vi compatible (not advisable)"],
    \ 'cpoptions': ["\tlist of flags to specify Vi compatibility"],
    \ 'insertmode':["\tuse Insert mode as the default mode"],
    \ 'paste':["\tpaste mode, insert typed text literally"],
    \ 'pastetoggle':["\tkey sequence to toggle paste mode"],
    \ 'runtimepath':["\tlist of directories used for runtime files and plugins"],
    \ 'packpath':["\tlist of directories used for plugin packages"],
    \ 'helpfile':["\tname of the main help file"],
   "\ moving around, searching and patterns
    \ 'whichwrap':["\tlist of flags specifying which commands wrap to another line", "\t(local to window)"],
    \ 'startofline':["\tmany jump commands move the cursor to the first non-blank", "\tcharacter of a line"],
    \ 'paragraphs':["\tnroff macro names that separate paragraphs"],
    \ 'sections':["\tnroff macro names that separate sections"],
    \ 'path':["\tlist of directory names used for file searching", "\t(global or local to buffer)"],
    \ 'cdpath':["\tlist of directory names used for :cd"],
    \ 'autochdir':["\tchange to directory of file in buffer"],
    \ 'wrapscan':["\tsearch commands wrap around the end of the buffer"],
    \ 'incsearch':["\tshow match for partly typed search command"],
    \ 'magic':["\tchange the way backslashes are used in search patterns"],
    \ 'regexpengine':["\tselect the default regexp engine used"],
    \ 'ignorecase':["\tignore case when using a search pattern"],
    \ 'smartcase': ["\toverride 'ignorecase' when pattern has upper case characters"],
    \ 'casemap':["\twhat method to use for changing case of letters"],
    \ 'maxmempattern':["\tmaximum amount of memory in Kbyte used for pattern matching"],
    \ 'define':["\tpattern for a macro definition line", "\t(global or local to buffer)"],
    \ 'include':["\tpattern for an include-file line", "\t(local to buffer)"],
    \ 'includeexpr':["\texpression used to transform an include line to a file name", "\t(local to buffer)"],
   "\ tags
    \ 'tagbsearch':["\tuse binary searching in tags files"],
    \ 'taglength':["\tnumber of significant characters in a tag name or zero"],
    \ 'tags':["\tlist of file names to search for tags", "\t(global or local to buffer)"],
    \ 'tagcase':["\thow to handle case when searching in tags files:", "\t\"followic\" to follow 'ignorecase', \"ignore\" or \"match\"", "\t(global or local to buffer)"],
    \ 'tagrelative':["\tfile names in a tags file are relative to the tags file"],
    \ 'tagstack':["\ta :tag command will use the tagstack"],
    \ 'showfulltag':["\twhen completing tags in Insert mode show more info"],
    \ 'tagfunc':["\ta function to be used to perform tag searches", "\t(local to buffer)"],
    \ 'cscopeprg':["\tcommand for executing cscope"],
    \ 'cscopetag':["\tuse cscope for tag commands"],
    \ 'cscopetagorder':["\t0 or 1; the order in which \":cstag\" performs a search"],
    \ 'cscopeverbose':["\tgive messages when adding a cscope database"],
    \ 'cscopepathcomp':["\thow many components of the path to show"],
    \ 'cscopequickfix':["\twhen to open a quickfix window for cscope"],
    \ 'cscoperelative':["\tfile names in a cscope file are relative to that file"],
   "\ displaying text
    \ 'scroll':["\tnumber of lines to scroll for CTRL-U and CTRL-D", "\t(local to window)"],
    \ 'scrolloff':["\tnumber of screen lines to show around the cursor"],
    \ 'wrap':["\tlong lines wrap", "\t(local to window)"],
    \ 'linebreak':["\twrap long lines at a character in 'breakat'", "\t(local to window)"],
    \ 'breakindent':["\tpreserve indentation in wrapped text", "\t(local to window)"],
    \ 'breakindentopt':["\tadjust breakindent behaviour", "\t(local to window)"],
    \ 'breakat':["\twhich characters might cause a line break"],
    \ 'showbreak':["\tstring to put before wrapped screen lines"],
    \ 'sidescroll':["\tminimal number of columns to scroll horizontally"],
    \ 'sidescrolloff':["\tminimal number of columns to keep left and right of the cursor"],
    \ 'display':["\tinclude \"lastline\" to show the last line even if it doesn't fit", "\tinclude \"uhex\" to show unprintable characters as a hex number"],
    \ 'fillchars':["\tcharacters to use for the status line, folds and filler lines"],
    \ 'cmdheight':["\tnumber of lines used for the command-line"],
    \ 'columns':["\twidth of the display"],
    \ 'lines':["\tnumber of lines in the display"],
    \ 'window':["\tnumber of lines to scroll for CTRL-F and CTRL-B"],
    \ 'lazyredraw':["\tdon't redraw while executing macros"],
    \ 'redrawtime':["\ttimeout for 'hlsearch' and :match highlighting in msec"],
    \ 'writedelay':["\tdelay in msec for each char written to the display", "\t(for debugging)"],
    \ 'list':["\tshow <Tab> as ^I and end-of-line as $", "\t(local to window)"],
    \ 'listchars':["\tlist of strings used for list mode"],
    \ 'number':["\tshow the line number for each line", "\t(local to window)"],
    \ 'relativenumber':["\tshow the relative line number for each line", "\t(local to window)"],
    \ 'numberwidth':["\tnumber of columns to use for the line number", "\t(local to window)"],
    \ 'conceallevel':["\tcontrols whether concealable text is hidden", "\t(local to window)"],
    \ 'concealcursor':["\tmodes in which text in the cursor line can be concealed", "\t(local to window)"],
   "\ syntax, highlihting and spelling
    \ 'background':["\t\"dark\" or \"light\"; the background color brightness"],
    \ 'filetype':["\ttype of file; triggers the FileType event when set", "\t(local to buffer)"],
    \ 'syntax':["\tname of syntax highlighting used", "\t(local to buffer)"],
    \ 'synmaxcol':["\tmaximum column to look for syntax items", "\t(local to buffer)"],
    \ 'highlight':["\twhich highlighting to use for various occasions"],
    \ 'hlsearch':["\thighlight all matches for the last used search pattern"],
    \ 'wincolor':["\thighlight group to use for the window", "\t(local to window)"],
    \ 'termguicolors':["\tuse GUI colors for the terminal"],
    \ 'cursorcolumn':["\thighlight the screen column of the cursor", "\t(local to window)"],
    \ 'cursorline':["\thighlight the screen line of the cursor", "\t(local to window)"],
    \ 'cursorlineopt':["\tspecifies which area 'cursorline' highlights", "\t(local to window)"],
    \ 'colorcolumn':["\tcolumns to highlight", "\t(local to window)"],
    \ 'spell':["\thighlight spelling mistakes", "\t(local to window)"],
    \ 'spelllang':["\tlist of accepted languages", "\t(local to window)"],
    \ 'spellfile':["\tfile that \"zg\" adds good words to", "\t(local to buffer)"],
    \ 'spellcapcheck':["\tpattern to locate the end of a sentence", "\t(local to buffer)"],
    \ 'spelloptions':["\tflags to change how spell checking works", "\t(local to buffer)"],
    \ 'spellsuggest':["\tmethods used to suggest corrections"],
    \ 'mkspellmem':["\tamount of memory used by :mkspell before compressing"],
   "\ multiple windows
    \ 'laststatus':["\t0, 1 or 2; when to use a status line for the last window"],
    \ 'statusline':["\talternate format to be used for a status line"],
    \ 'equalalways':["\tmake all windows the same size when adding/removing windows"],
    \ 'eadirection':["\tin which direction 'equalalways' works: \"ver\", \"hor\" or \"both\""],
    \ 'winheight':["\tminimal number of lines used for the current window"],
    \ 'winminheight':["\tminimal number of lines used for any window"],
    \ 'winfixheight':["\tkeep the height of the window", "\t(local to window)"],
    \ 'winfixwidth':["\tkeep the width of the window", "\t(local to window)"],
    \ 'winwidth':["\tminimal number of columns used for the current window"],
    \ 'winminwidth':["\tminimal number of columns used for any window"],
    \ 'helpheight':["\tinitial height of the help window"],
    \ 'previewpopup':["\tuse a popup window for preview"],
    \ 'previewheight':["\tdefault height for the preview window"],
    \ 'previewwindow':["\tidentifies the preview window", "\t(local to window)"],
    \ 'hidden':["\tdon't unload a buffer when no longer shown in a window"],
    \ 'switchbuf':["\t\"useopen\" and/or \"split\"; which window to use when jumping", "\tto a buffer"],
    \ 'splitbelow':["\ta new window is put below the current one"],
    \ 'splitright':["\ta new window is put right of the current one"],
    \ 'scrollbind':["\tthis window scrolls together with other bound windows", "\t(local to window)"],
    \ 'scrollopt':["\t\"ver\", \"hor\" and/or \"jump\"; list of options for 'scrollbind'"],
    \ 'cursorbind':["\tthis window's cursor moves together with other bound windows", "\t(local to window)"],
    \ 'termwinsize':["\tsize of a terminal window", "\t(local to window)"],
    \ 'termwinkey':["\tkey that precedes Vim commands in a terminal window", "\t(local to window)"],
    \ 'termwinscroll':["\tmax number of lines to keep for scrollback in a terminal window", "\t(local to window)"],
    \ 'termwintype':["\ttype of pty to use for a terminal window"],
    \ 'winptydll':["\tname of the winpty dynamic library"],
   "\ multiple tab pages
    \ 'showtabline':["\t0, 1 or 2; when to use a tab pages line"],
    \ 'tabpagemax':["\tmaximum number of tab pages to open for -p and \"tab all\""],
    \ 'tabline':["\tcustom tab pages line"],
    \ 'guitablabel':["\tcustom tab page label for the GUI"],
    \ 'guitabtooltip':["\tcustom tab page tooltip for the GUI"],
   "\ terminal
    \ 'term':["\tname of the used terminal"],
    \ 'ttytype':["\talias for 'term'"],
    \ 'ttybuiltin':["\tcheck built-in termcaps first"],
    \ 'ttyfast':["\tterminal connection is fast"],
    \ 'weirdinvert':["\tterminal that requires extra redrawing"],
    \ 'esckeys':["\trecognize keys that start with <Esc> in Insert mode"],
    \ 'scrolljump':["\tminimal number of lines to scroll at a time"],
    \ 'ttyscroll':["\tmaximum number of lines to use scrolling instead of redrawing"],
    \ 'guicursor':["\tspecifies what the cursor looks like in different modes"],
    \ 'title':["\tshow info in the window title"],
    \ 'titlelen':["\tpercentage of 'columns' used for the window title"],
    \ 'titlestring':["\twhen not empty, string to be used for the window title"],
    \ 'titleold':["\tstring to restore the title to when exiting Vim"],
    \ 'icon':["\tset the text of the icon for this window"],
    \ 'iconstring':["\twhen not empty, text for the icon of this window"],
    \ 'restorescreen':["\trestore the screen contents when exiting Vim"],
   "\ using the mouse
    \ 'mouse':["\tlist of flags for using the mouse"],
    \ 'mousefocus':["\tthe window with the mouse pointer becomes the current one"],
    \ 'scrollfocus':["\tthe window with the mouse pointer scrolls with the mouse wheel"],
    \ 'mousehide':["\thide the mouse pointer while typing"],
    \ 'mousemodel':["\t\"extend\", \"popup\" or \"popup_setpos\"; what the right", "\tmouse button is used for"],
    \ 'mousetime':["\tmaximum time in msec to recognize a double-click"],
    \ 'ttymouse':["\t\"xterm\", \"xterm2\", \"dec\" or \"netterm\"; type of mouse"],
    \ 'mouseshape':["\twhat the mouse pointer looks like in different modes"],
   "\ GUI
    \ 'guifont':["\tlist of font names to be used in the GUI"],
    \ 'guifontset':["\tpair of fonts to be used, for multibyte editing"],
    \ 'guifontwide':["\tlist of font names to be used for double-wide characters"],
    \ 'antialias':["\tuse smooth, antialiased fonts"],
    \ 'guioptions':["\tlist of flags that specify how the GUI works"],
    \ 'toolbar':["\t\"icons\", \"text\" and/or \"tooltips\"; how to show the toolbar"],
    \ 'toolbariconsize':["\tsize of toolbar icons"],
    \ 'guiheadroom':["\troom (in pixels) left above/below the window"],
    \ 'renderoptions':["\toptions for text rendering"],
    \ 'guipty':["\tuse a pseudo-tty for I/O to external commands"],
    \ 'browsedir':["\t\"last\", \"buffer\" or \"current\": which directory used for the file browser"],
    \ 'langmenu':["\tlanguage to be used for the menus"],
    \ 'menuitems':["\tmaximum number of items in one menu"],
    \ 'winaltkeys':["\t\"no\", \"yes\" or \"menu\"; how to use the ALT key"],
    \ 'linespace':["\tnumber of pixel lines to use between characters"],
    \ 'balloondelay':["\tdelay in milliseconds before a balloon may pop up"],
    \ 'ballooneval':["\tuse balloon evaluation in the GUI"],
    \ 'balloonevalterm':["\tuse balloon evaluation in the terminal"],
    \ 'balloonexpr':["\texpression to show in balloon eval"],
    \ 'macatsui':["\tuse ATSUI text drawing; disable to avoid display problems"],
   "\ printing
    \ 'printoptions':["\tlist of items that control the format of :hardcopy output"],
    \ 'printdevice':["\tname of the printer to be used for :hardcopy"],
    \ 'printexpr':["\texpression used to print the PostScript file for :hardcopy"],
    \ 'printfont':["\tname of the font to be used for :hardcopy"],
    \ 'printheader':["\tformat of the header used for :hardcopy"],
    \ 'printencoding':["\tencoding used to print the PostScript file for :hardcopy"],
    \ 'printmbcharset':["\tthe CJK character set to be used for CJK output from :hardcopy"],
    \ 'printmbfont':["\tlist of font names to be used for CJK output from :hardcopy"],
   "\ messages and info
    \ 'terse':["\tadd 's' flag in 'shortmess' (don't show search message)"],
    \ 'shortmess':["\tlist of flags to make messages shorter"],
    \ 'showcmd':["\tshow (partial) command keys in the status line"],
    \ 'showmode':["\tdisplay the current mode in the status line"],
    \ 'ruler':["\tshow cursor position below each window"],
    \ 'rulerformat':["\talternate format to be used for the ruler"],
    \ 'report':["\tthreshold for reporting number of changed lines"],
    \ 'verbose':["\tthe higher the more messages are given"],
    \ 'verbosefile':["\tfile to write messages in"],
    \ 'more':["\tpause listings when the screen is full"],
    \ 'confirm':["\tstart a dialog when a command fails"],
    \ 'errorbells':["\tring the bell for error messages"],
    \ 'visualbell':["\tuse a visual bell instead of beeping"],
    \ 'belloff':["\tdo not ring the bell for these reasons"],
    \ 'helplang':["\tlist of preferred languages for finding help"],
   "\ selecting text
    \ 'selection':["\t\"old\", \"inclusive\" or \"exclusive\"; how selecting text behaves"],
    \ 'selectmode':["\t\"mouse\", \"key\" and/or \"cmd\"; when to start Select mode", "\tinstead of Visual mode"],
    \ 'clipboard':["\t\"unnamed\" to use the * register like unnamed register", "\t\"autoselect\" to always put selected text on the clipboard"],
    \ 'keymodel':["\t\"startsel\" and/or \"stopsel\"; what special keys can do"],
   "\ editing text
    \ 'undolevels':["\tmaximum number of changes that can be undone", "\t(global or local to buffer)"],
    \ 'undofile':["\tautomatically save and restore undo history"],
    \ 'undodir':["\tlist of directories for undo files"],
    \ 'undoreload':["\tmaximum number lines to save for undo on a buffer reload"],
    \ 'modified':["\tchanges have been made and not written to a file", "\t(local to buffer)"],
    \ 'readonly':["\tbuffer is not to be written", "\t(local to buffer)"],
    \ 'modifiable':["\tchanges to the text are not possible", "\t(local to buffer)"],
    \ 'textwidth':["\tline length above which to break a line", "\t(local to buffer)"],
    \ 'wrapmargin':["\tmargin from the right in which to break a line", "\t(local to buffer)"],
    \ 'backspace':["\tspecifies what <BS>, CTRL-W, etc. can do in Insert mode"],
    \ 'comments':["\tdefinition of what comment lines look like", "\t(local to buffer)"],
    \ 'formatoptions':["\tlist of flags that tell how automatic formatting works", "\t(local to buffer)"],
    \ 'formatlistpat':["\tpattern to recognize a numbered list", "\t(local to buffer)"],
    \ 'formatexpr':["\texpression used for \"gq\" to format lines", "\t(local to buffer)"],
    \ 'complete':["\tspecifies how Insert mode completion works for CTRL-N and CTRL-P", "\t(local to buffer)"],
    \ 'completeopt':["\twhether to use a popup menu for Insert mode completion"],
    \ 'completepopup':["\toptions for the Insert mode completion info popup"],
    \ 'pumheight':["\tmaximum height of the popup menu"],
    \ 'pumwidth':["\tminimum width of the popup menu"],
    \ 'completefunc':["\tuser defined function for Insert mode completion", "\t(local to buffer)"],
    \ 'omnifunc':["\tfunction for filetype-specific Insert mode completion", "\t(local to buffer)"],
    \ 'dictionary':["\tlist of dictionary files for keyword completion", "\t(global or local to buffer)"],
    \ 'thesaurus':["\tlist of thesaurus files for keyword completion", "\t(global or local to buffer)"],
    \ 'infercase':["\tadjust case of a keyword completion match", "\t(local to buffer)"],
    \ 'digraph':["\tenable entering digraphs with c1 <BS> c2"],
    \ 'tildeop':["\tthe \"~\" command behaves like an operator"],
    \ 'operatorfunc':["\tfunction called for the\"g@\"  operator"],
    \ 'showmatch':["\twhen inserting a bracket, briefly jump to its match"],
    \ 'matchtime':["\ttenth of a second to show a match for 'showmatch'"],
    \ 'matchpairs':["\tlist of pairs that match for the \"%\" command", "\t(local to buffer)"],
    \ 'joinspaces':["\tuse two spaces after '.' when joining a line"],
    \ 'nrformats':["\t\"alpha\", \"octal\" and/or \"hex\"; number formats recognized for", "\tCTRL-A and CTRL-X commands", "\t(local to buffer)"],
   "\ tabs and indenting
    \ 'tabstop':["\tnumber of spaces a <Tab> in the text stands for", "\t(local to buffer)"],
    \ 'shiftwidth':["\tnumber of spaces used for each step of (auto)indent", "\t(local to buffer)"],
    \ 'vartabstop':["\tlist of number of spaces a tab counts for", "\t(local to buffer)"],
    \ 'varsofttabstop':["\tlist of number of spaces a soft tabsstop counts for", "\t(local to buffer)"],
    \ 'smarttab':["\ta <Tab> in an indent inserts 'shiftwidth' spaces"],
    \ 'softtabstop':["\tif non-zero, number of spaces to insert for a <Tab>", "\t(local to buffer)"],
    \ 'shiftround':["\tround to 'shiftwidth' for \"<<\" and \">>\""],
    \ 'expandtab':["\texpand <Tab> to spaces in Insert mode", "\t(local to buffer)"],
    \ 'autoindent':["\tautomatically set the indent of a new line", "\t(local to buffer)"],
    \ 'smartindent':["\tdo clever autoindenting", "\t(local to buffer)"],
    \ 'cindent':["\tenable specific indenting for C code", "\t(local to buffer)"],
    \ 'cinoptions':["\toptions for C-indenting", "\t(local to buffer)"],
    \ 'cinkeys':["\tkeys that trigger C-indenting in Insert mode", "\t(local to buffer)"],
    \ 'cinwords':["\tlist of words that cause more C-indent", "\t(local to buffer)"],
    \ 'indentexpr':["\texpression used to obtain the indent of a line", "\t(local to buffer)"],
    \ 'indentkeys':["\tkeys that trigger indenting with 'indentexpr' in Insert mode", "\t(local to buffer)"],
    \ 'copyindent':["\tcopy whitespace for indenting from previous line", "\t(local to buffer)"],
    \ 'preserveindent':["\tpreserve kind of whitespace when changing indent", "\t(local to buffer)"],
    \ 'lisp':["\tenable lisp mode", "\t(local to buffer)"],
    \ 'lispwords':["\twords that change how lisp indenting works"],
   "\ folding
    \ 'foldenable':["\tset to display all folds open", "\t(local to window)"],
    \ 'foldlevel':["\tfolds with a level higher than this number will be closed", "\t(local to window)"],
    \ 'foldlevelstart':["\tvalue for 'foldlevel' when starting to edit a file"],
    \ 'foldcolumn':["\twidth of the column used to indicate folds", "\t(local to window)"],
    \ 'foldtext':["\texpression used to display the text of a closed fold", "\t(local to window)"],
    \ 'foldclose':["\tset to \"all\" to close a fold when the cursor leaves it"],
    \ 'foldopen':["\tspecifies for which commands a fold will be opened"],
    \ 'foldminlines':["\tminimum number of screen lines for a fold to be closed", "\t(local to window)"],
    \ 'commentstring':["\ttemplate for comments; used to put the marker in"],
    \ 'foldmethod':["\tfolding type: \"manual\", \"indent\", \"expr\", \"marker\" or \"syntax\"", "\t(local to window)"],
    \ 'foldexpr':["\texpression used when 'foldmethod' is \"expr\"", "\t(local to window)"],
    \ 'foldignore':["\tused to ignore lines when 'foldmethod' is \"indent\"", "\t(local to window)"],
    \ 'foldmarker':["\tmarkers used when 'foldmethod' is \"marker\"", "\t(local to window)"],
    \ 'foldnestmax':["\tmaximum fold depth for when 'foldmethod' is \"indent\" or \"syntax\"", "\t(local to window)"],
   "\ diff mode
    \ 'diff':["\tuse diff mode for the current window", "\t(local to window)"],
    \ 'diffopt':["\toptions for using diff mode"],
    \ 'diffexpr':["\texpression used to obtain a diff file"],
    \ 'patchexpr':["\texpression used to patch a file"],
   "\ mapping
    \ 'maxmapdepth':["\tmaximum depth of mapping"],
    \ 'remap':["\trecognize mappings in mapped keys"],
    \ 'timeout':["\tallow timing out halfway into a mapping"],
    \ 'ttimeout':["\tallow timing out halfway into a key code"],
    \ 'timeoutlen':["\ttime in msec for 'timeout'"],
    \ 'ttimeoutlen':["\ttime in msec for 'ttimeout'"],
   "\ reading and writing files
    \ 'modeline':["\tenable using settings from modelines when reading a file", "\t(local to buffer)"],
    \ 'modelineexpr':["\tallow setting expression options from a modeline"],
    \ 'modelines':["\tnumber of lines to check for modelines"],
    \ 'binary':["\tbinary file editing", "\t(local to buffer)"],
    \ 'endofline':["\tlast line in the file has an end-of-line", "\t(local to buffer)"],
    \ 'fixendofline':["\tfixes missing end-of-line at end of text file", "\t(local to buffer)"],
    \ 'bomb':["\tprepend a Byte Order Mark to the file", "\t(local to buffer)"],
    \ 'fileformat':["\tend-of-line format: \"dos\", \"unix\" or \"mac\"", "\t(local to buffer)"],
    \ 'fileformats':["\tlist of file formats to look for when editing a file"],
    \ 'textmode':["\tobsolete, use 'fileformat'", "\t(local to buffer)"],
    \ 'textauto':["\tobsolete, use 'fileformats'"],
    \ 'write':["\twriting files is allowed"],
    \ 'writebackup':["\twrite a backup file before overwriting a file"],
    \ 'backup':["\tkeep a backup after overwriting a file"],
    \ 'backupskip':["\tpatterns that specify for which files a backup is not made"],
    \ 'backupcopy':["\twhether to make the backup as a copy or rename the existing file", "\t(global or local to buffer)"],
    \ 'backupdir':["\tlist of directories to put backup files in"],
    \ 'backupext':["\tfile name extension for the backup file"],
    \ 'autowrite':["\tautomatically write a file when leaving a modified buffer"],
    \ 'autowriteall':["\tas 'autowrite', but works with more commands"],
    \ 'writeany':["\talways write without asking for confirmation"],
    \ 'autoread':["\tautomatically read a file when it was modified outside of Vim", "\t(global or local to buffer)"],
    \ 'patchmode':["\tkeep oldest version of a file; specifies file name extension"],
    \ 'fsync':["\tforcibly sync the file to disk after writing it"],
    \ 'shortname':["\tuse 8.3 file names", "\t(local to buffer)"],
    \ 'cryptmethod':["\tencryption method for file writing: zip or blowfish", "\t(local to buffer)"],
   "\ the swap file
    \ 'directory':["\tlist of directories for the swap file"],
    \ 'swapfile':["\tuse a swap file for this buffer", "\t(local to buffer)"],
    \ 'swapsync':["\t\"sync\", \"fsync\" or empty; how to flush a swap file to disk"],
    \ 'updatecount':["\tnumber of characters typed to cause a swap file update"],
    \ 'updatetime':["\ttime in msec after which the swap file will be updated"],
    \ 'maxmem':["\tmaximum amount of memory in Kbyte used for one buffer"],
    \ 'maxmemtot':["\tmaximum amount of memory in Kbyte used for all buffers"],
   "\ command line editing
    \ 'history':["\thow many command lines are remembered "],
    \ 'wildchar':["\tkey that triggers command-line expansion"],
    \ 'wildcharm':["\tlike 'wildchar' but can also be used in a mapping"],
    \ 'wildmode':["\tspecifies how command line completion works"],
    \ 'wildoptions':["\tempty or \"tagfile\" to list file name of matching tags"],
    \ 'suffixes':["\tlist of file name extensions that have a lower priority"],
    \ 'suffixesadd':["\tlist of file name extensions added when searching for a file", "\t(local to buffer)"],
    \ 'wildignore':["\tlist of patterns to ignore files for file name completion"],
    \ 'fileignorecase':["\tignore case when using file names"],
    \ 'wildignorecase':["\tignore case when completing file names"],
    \ 'wildmenu':["\tcommand-line completion shows a list of matches"],
    \ 'cedit':["\tkey used to open the command-line window"],
    \ 'cmdwinheight':["\theight of the command-line window"],
   "\ executing external commands
    \ 'shell':["\tname of the shell program used for external commands"],
    \ 'shelltype':["\twhen to use the shell or directly execute a command"],
    \ 'shellquote':["\tcharacter(s) to enclose a shell command in"],
    \ 'shellxquote':["\tlike 'shellquote' but include the redirection"],
    \ 'shellxescape':["\tcharacters to escape when 'shellxquote' is ("],
    \ 'shellcmdflag':["\targument for 'shell' to execute a command"],
    \ 'shellredir':["\tused to redirect command output to a file"],
    \ 'shelltemp':["\tuse a temp file for shell commands instead of using a pipe"],
    \ 'equalprg':["\tprogram used for \"=\" command", "\t(global or local to buffer)"],
    \ 'formatprg':["\tprogram used to format lines with \"gq\" command"],
    \ 'keywordprg':["\tprogram used for the \"K\" command"],
    \ 'warn':["\twarn when using a shell command and a buffer has changes"],
   "\ running make and jumping to errors (quickfix)
    \ 'errorfile':["\tname of the file that contains error messages"],
    \ 'errorformat':["\tlist of formats for error messages", "\t(global or local to buffer)"],
    \ 'makeprg':["\tprogram used for the \":make\" command", "\t(global or local to buffer)"],
    \ 'shellpipe':["\tstring used to put the output of \":make\" in the error file"],
    \ 'makeef':["\tname of the errorfile for the 'makeprg' command"],
    \ 'grepprg':["\tprogram used for the \":grep\" command", "\t(global or local to buffer)"],
    \ 'grepformat':["\tlist of formats for output of 'grepprg'"],
    \ 'makeencoding':["\tencoding of the \":make\" and \":grep\" output", "\t(global or local to buffer)"],
    \ 'quickfixtextfunc':["\tfunction to display text in the quickfix window"],
   "\ system specific
    \ 'osfiletype':["\tOS-specific information about the type of file", "\t(local to buffer)"],
    \ 'shellslash':["\tuse forward slashes in file names; for Unix-like shells"],
    \ 'completeslash':["\tspecifies slash/backslash used for completion"],
   "\ language specific
    \ 'isfname':["\tspecifies the characters in a file name"],
    \ 'isident':["\tspecifies the characters in an identifier"],
    \ 'iskeyword':["\tspecifies the characters in a keyword", "\t(local to buffer)"],
    \ 'isprint':["\tspecifies printable characters"],
    \ 'quoteescape':["\tspecifies escape characters in a string", "\t(local to buffer)"],
    \ 'rightleft':["\tdisplay the buffer right-to-left", "\t(local to window)"],
    \ 'rightleftcmd':["\twhen to edit the command-line right-to-left", "\t(local to window)"],
    \ 'revins':["\tinsert characters backwards"],
    \ 'allowrevins':["\tallow CTRL-_ in Insert and Command-line mode to toggle 'revins'"],
    \ 'aleph':["\tthe ASCII code for the first letter of the Hebrew alphabet"],
    \ 'hkmap':["\tuse Hebrew keyboard mapping"],
    \ 'hkmapp':["\tuse phonetic Hebrew keyboard mapping"],
    \ 'altkeymap':["\tuse Farsi as the second language when 'revins' is set"],
    \ 'fkmap':["\tuse Farsi keyboard mapping"],
    \ 'arabic':["\tprepare for editing Arabic text", "\t(local to window)"],
    \ 'arabicshape':["\tperform shaping of Arabic characters"],
    \ 'termbidi':["\tterminal will perform bidi handling"],
    \ 'keymap':["\tname of a keyboard mapping"],
    \ 'langmap':["\tlist of characters that are translated in Normal mode"],
    \ 'langremap':["\tapply 'langmap' to mapped characters"],
    \ 'imdisable':["\twhen set never use IM; overrules following IM options"],
    \ 'iminsert':["\tin Insert mode: 1: use :lmap; 2: use IM; 0: neither", "\t(local to window)"],
    \ 'imstyle':["\tinput method style, 0: on-the-spot, 1: over-the-spot"],
    \ 'imsearch':["\tentering a search pattern: 1: use :lmap; 2: use IM; 0: neither", "\t(local to window)"],
    \ 'imcmdline':["\twhen set always use IM when starting to edit a command line"],
    \ 'imstatusfunc':["\tfunction to obtain IME status"],
    \ 'imactivatefunc':["\tfunction to enable/disable IME"],
   "\ mulit-byte characters
    \ 'encoding':["\tcharacter encoding used in Vim: \"latin1\", \"utf-8\"", "\t\"euc-jp\", \"big5\", etc."],
    \ 'fileencoding':["\tcharacter encoding for the current file", "\t(local to buffer)"],
    \ 'fileencodings':["\tautomatically detected character encodings"],
    \ 'termencoding':["\tcharacter encoding used by the terminal"],
    \ 'charconvert':["\texpression used for character encoding conversion"],
    \ 'delcombine':["\tdelete combining (composing) characters on their own"],
    \ 'maxcombine':["\tmaximum number of combining (composing) characters displayed"],
    \ 'imactivatekey':["\tkey that activates the X input method"],
    \ 'ambiwidth':["\twidth of ambiguous width characters"],
    \ 'emoji':["\temoji characters are full width"],
   "\ various
    \ 'virtualedit':["\twhen to use virtual editing: \"block\", \"insert\" and/or \"all\""],
    \ 'eventignore':["\tlist of autocommand events which are to be ignored"],
    \ 'loadplugins':["\tload plugin scripts when starting up"],
    \ 'exrc':["\tenable reading .vimrc/.exrc/.gvimrc in the current directory"],
    \ 'secure':["\tsafer working with script files in the current directory"],
    \ 'gdefault':["\tuse the 'g' flag for \":substitute\""],
    \ 'edcompatible':["\t'g' and 'c' flags of \":substitute\" toggle"],
    \ 'opendevice':["\tallow reading/writing devices"],
    \ 'maxfuncdepth':["\tmaximum depth of function calls"],
    \ 'sessionoptions':["\tlist of words that specifies what to put in a session file"],
    \ 'viewoptions':["\tlist of words that specifies what to save for :mkview"],
    \ 'viewdir':["\tdirectory where to store files with :mkview"],
    \ 'viminfo':["\tlist that specifies what to write in the viminfo file"],
    \ 'viminfofile':["\tfile name used for the viminfo file"],
    \ 'bufhidden':["\twhat happens with a buffer when it's no longer in a window", "\t(local to buffer)"],
    \ 'buftype':["\t\"\", \"nofile\", \"nowrite\" or \"quickfix\": type of buffer", "\t(local to buffer)"],
    \ 'buflisted':["\twhether the buffer shows up in the buffer list", "\t(local to buffer)"],
    \ 'debug':["\tset to \"msg\" to see all error messages"],
    \ 'signcolumn':["\twhether to show the signcolumn", "\t(local to window)"],
    \ 'mzquantum':["\tinterval in milliseconds between polls for MzScheme threads"],
    \ 'luadll':["\tname of the Lua dynamic library"],
    \ 'perldll':["\tname of the Perl dynamic library"],
    \ 'pyxversion':["\twhether to use Python 2 or 3"],
    \ 'pythondll':["\tname of the Python 2 dynamic library"],
    \ 'pythonhome':["\tname of the Python 2 home directory"],
    \ 'pythonthreedll':["\tname of the Python 3 dynamic library"],
    \ 'pythonthreehome':["\tname of the Python 3 home directory"],
    \ 'rubydll':["\tname of the Ruby dynamic library"],
    \ 'tcldll':["\tname of the Tcl dynamic library"],
    \ 'mzschemedll':["\tname of the Tcl dynamic library"],
    \ 'mzschemegcdll':["\tname of the Tcl GC dynamic library"],
    \ }









"
" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
" ============================================================================
" |                                                                          |
" |                             Attention!                                   |
" |                                                                          |
" |   Everything below can only be changed by the person who maintains the   |
" |                       optwin.vim module                                  |
" |                                                                          |
" |                                                                          |
" |                             Внимание!                                    |
" |                                                                          |
" |   Всё что ниже, может изменять только тот, кто сопровождает модуль       |
" |                            optwin.vim                                    |
" |                                                                          |
" ============================================================================ 
" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
"




fun! <SID>GetLenTrans()
    let g:contlen = len(s:contents)
    let g:optdesclen = len(s:optdesc)
endfun

fun! <SID>Whattodo()
if !exists("g:optwin_trans_ok")
    call <SID>GetLenTrans()
    elseif 2 == g:optwin_trans_ok
	"copy arrays
	let g:contents = copy(s:contents)
	let g:optdesc = copy(s:optdesc)
	let g:banner = copy(s:banner)
    elseif 1 == g:optwin_trans_ok
	    "copy arrays
	    let g:contents = copy(s:contents)
	    let g:optdesc = copy(s:optdesc)
	    let g:banner = copy(s:banner)
    else
	finish
endif
endfun


let g:optwin_fl = expand("<sfile>:t")
call <SID>Whattodo()

delfun <SID>GetLenTrans
delfun <SID>Whattodo
finish


" vim: ts=8 sw=4
