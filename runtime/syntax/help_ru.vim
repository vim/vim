" Vim syntax file
" Language:     Russian Vim program help files *.rux
" Maintainer:   Restorer, <restorer@mail2k.ru>
" Last Change:  2025 Jul 07
" 

" Проверяем язык локали и установки опции 'helplang'
" Если не русский, то выходим из скрипта.
if (v:lang !~? 'ru' || v:lang !~? 'russian') && &helplang !~? 'ru'
  finish
endif

" Определение синтаксиса русских гиперссылок
syntax match helpHyperTextJump	"\\\@<!|[^"*~# |]\+|" contains=helpBar
syntax match helpHyperTextEntry	"\*[^"*|]\+\*\s"he=e-1 contains=helpStar
syntax match helpHyperTextEntry	"\*[^"*|]\+\*$" contains=helpStar

" Определение синтаксиса статьи, подраздела и т. п.
syntax match helpHeadline	"^[А-ЯЁ]\{2,}[ .]\=[-,А-ЯЁA-Z0-9 .()]*"

" Определение синтаксиса заголовка основного файла справки
syntax match helpVim		"\<РЕДАКТОР VIM — общий обзор\>"
" Определение синтаксиса наименования справочника
syntax match helpVim		"\<СПРАВОЧНИК ПО РЕДАКТОРУ VIM\>"
" Определение синтаксиса наименования руководства пользователя
syntax match helpVim		"\<РУКОВОДСТВО ПОЛЬЗОВАТЕЛЯ ПО РЕДАКТОРУ VIM\>"
" Определение синтаксиса автора справочника, руководства пользователя
syntax match helpAutor		"^\s\+\<автор\%[ы:] .*$"
" Определение синтаксиса примечаний в тексте, начала примеров и т. п.
syntax keyword helpNote		Примечание\. Совет\. Пример\. Прмимер, Примеры:
syntax keyword helpWarning	Внимание!
syntax keyword helpDeprecated	Недействующий Недействующая Недействующее
syntax keyword helpDeprecated	недейстующиий недействующая недействующее
syntax keyword helpDeprecated	Устаревший Устаревшая Устаревшее
syntax keyword helpDeprecated	устаревший устаревшая устаревшее
" Определение синтаксиса Ex-команд в документации Vim (старый стиль)
syntax match helpCommand	~\":\w\+\%[!]\"~hs=s+1,he=e-1
" Определение синтаксиса специальных обозначений
syntax match helpSpecial	"{[-а-яёА-ЯЁ0-9'":%#=[\]<>.,]\+}"
syntax match helpSpecial	"{[-а-яёА-ЯЁ0-9'"*+/:%#=[\]<>.,_]\+}"
syntax match helpSpecial	"\s\[[-а-яё^А-ЯЁ0-9_]\{2,}]"ms=s+1
syntax match helpSpecial	"<[-а-яёА-ЯЁ0-9_]\+>"
syntax match helpSpecial	"\[диапазон]"
syntax match helpSpecial	"\[счётчик]"
syntax match helpSpecial	"\[число]"
syntax match helpSpecial	"\[+число]"
syntax match helpSpecial	"\[-число]"
syntax match helpSpecial	"\[кол-во]"
syntax match helpSpecial	"\[строка]"
syntax match helpSpecial	"\[смещение]"
syntax match helpSpecial	"\[параметр]"
syntax match helpSpecial	"\[параметры]"
syntax match helpSpecial	"CTRL-{символ}"
syntax region helpNotVi		start="{Доступно только" start="{В редкторе Vim" start="{В редакторе Vi" end="}" contains=helpLeadBlank,helpHyperTextJump
" Определение синтаксиса примечаний переводчика
syntax region helpTrnsNote	start="\[Примеч\. перев\. — " end="\]\." contains=helpComment
" Группа подсветки синтаксиса Ex-команд внутри строки документации Vim
hi def link helpCommand		VimCommand
" Группа подсветки синтаксиса примеров в документации Vim
hi def link helpExample		SpecialComment
"hi def link helpExample		PreCondit
" Группа подсветки синтаксиса примечаний переводчика
hi def link helpTrnsNote	Comment
" Группа подсветки синтаксиса автора справочника, руководства пользователя
hi def link helpAutor		HelpVim
"
" vim: ts=8 sw=2
