" Vim keymap file for Bulgarian and Russian characters, `bds' layout.
" Can be used with utf-8 or cp1251 file encodings.
" This file itself is in utf-8

" Maintainer: Boyko Bantchev <boykobb@gmail.com>
" URI: http://www.math.bas.bg/softeng/bantchev/misc/vim/bulgarian-bds.vim
" Last Changed: 2006 Oct 18

" This keymap corresponds to what is called Bulgarian standard,
" or BDS (БДС) typewriter keyboard layout.
" In addition to the Bulgarian alphabet, BDS prescribes the presence
" of the following characters:
"     —  The Cyrillic letters Э (capital), and ы and э (small)
"        (these are present in the Russian alphabet).
"     —  The latin capital letters I and V (these are used to type
"        Roman numerals without having to leave Cyrillic mode).
"     —  „ and “ (Bulgarian quotation style), and « and » (Russian quotation
"        style).
"     —  §, №, —, •, ·, ±, ¬, ¤, and €
"
" Some punctuation characters that are present in ascii are mapped in BDS
" to keys different from the ones they occupy in the qwerty layout, because
" the latter are used to type other characters.
"
" In this keymap also defined (not in BDS) are the Russian letters Ё (capital)
" and ё (small), as well as the Russian capital letter Ы (see above the small
" counterpart).  This way, using the bulgarian-bds keymap, one can access both
" the Bulgarian and the Russian alphabets.

scriptencoding utf-8

let b:keymap_name = "bds"

loadkeymap
D       А       CYRILLIC CAPITAL LETTER A
?       Б       CYRILLIC CAPITAL LETTER BE
L       В       CYRILLIC CAPITAL LETTER VE
H       Г       CYRILLIC CAPITAL LETTER GHE
O       Д       CYRILLIC CAPITAL LETTER DE
E       Е       CYRILLIC CAPITAL LETTER IE
E::     Ё       CYRILLIC CAPITAL LETTER IO
G       Ж       CYRILLIC CAPITAL LETTER ZHE
P       З       CYRILLIC CAPITAL LETTER ZE
R       И       CYRILLIC CAPITAL LETTER I
X       Й       CYRILLIC CAPITAL LETTER SHORT I
U       К       CYRILLIC CAPITAL LETTER KA
>       Л       CYRILLIC CAPITAL LETTER EL
:       М       CYRILLIC CAPITAL LETTER EM
K       Н       CYRILLIC CAPITAL LETTER EN
F       О       CYRILLIC CAPITAL LETTER O
M       П       CYRILLIC CAPITAL LETTER PE
<       Р       CYRILLIC CAPITAL LETTER ER
I       С       CYRILLIC CAPITAL LETTER ES
J       Т       CYRILLIC CAPITAL LETTER TE
W       У       CYRILLIC CAPITAL LETTER U
B       Ф       CYRILLIC CAPITAL LETTER EF
N       Х       CYRILLIC CAPITAL LETTER HA
{       Ц       CYRILLIC CAPITAL LETTER TSE
\"      Ч       CYRILLIC CAPITAL LETTER CHE
T       Ш       CYRILLIC CAPITAL LETTER SHA
Y       Щ       CYRILLIC CAPITAL LETTER SHCHA
C       Ъ       CYRILLIC CAPITAL LETTER HARD SIGN
CX      Ы       CYRILLIC CAPITAL LETTER YERU
A       Ь       CYRILLIC CAPITAL LETTER SOFT SIGN
V       Э       CYRILLIC CAPITAL LETTER REVERSED E
Z       Ю       CYRILLIC CAPITAL LETTER YU
S       Я       CYRILLIC CAPITAL LETTER YA
d       а       CYRILLIC SMALL LETTER A
\/      б       CYRILLIC SMALL LETTER BE
l       в       CYRILLIC SMALL LETTER VE
h       г       CYRILLIC SMALL LETTER GHE
o       д       CYRILLIC SMALL LETTER DE
e       е       CYRILLIC SMALL LETTER IE
e::     ё       CYRILLIC SMALL LETTER IO
g       ж       CYRILLIC SMALL LETTER ZHE
p       з       CYRILLIC SMALL LETTER ZE
r       и       CYRILLIC SMALL LETTER I
x       й       CYRILLIC SMALL LETTER SHORT I
u       к       CYRILLIC SMALL LETTER KA
\.      л       CYRILLIC SMALL LETTER EL
;       м       CYRILLIC SMALL LETTER EM
k       н       CYRILLIC SMALL LETTER EN
f       о       CYRILLIC SMALL LETTER O
m       п       CYRILLIC SMALL LETTER PE
,       р       CYRILLIC SMALL LETTER ER
i       с       CYRILLIC SMALL LETTER ES
j       т       CYRILLIC SMALL LETTER TE
w       у       CYRILLIC SMALL LETTER U
b       ф       CYRILLIC SMALL LETTER EF
n       х       CYRILLIC SMALL LETTER HA
[       ц       CYRILLIC SMALL LETTER TSE
'       ч       CYRILLIC SMALL LETTER CHE
t       ш       CYRILLIC SMALL LETTER SHA
y       щ       CYRILLIC SMALL LETTER SHCHA
c       ъ       CYRILLIC SMALL LETTER HARD SIGN
Q       ы       CYRILLIC SMALL LETTER YERU
a       ь       CYRILLIC SMALL LETTER SOFT SIGN
v       э       CYRILLIC SMALL LETTER REVERSED E
z       ю       CYRILLIC SMALL LETTER YU
s       я       CYRILLIC SMALL LETTER YA
_       I       LATIN CAPITAL LETTER I
+       V       LATIN CAPITAL LETTER V
$       "       QUOTATION MARK
\\      (       LEFT PARENTHESIS
|       )       RIGHT PARENTHESIS
#       +       PLUS SIGN
q       ,       COMMA
(       -       HYPHEN-MINUS
=       .       FULL STOP (PERIOD)
*       /       SOLIDUS (SLASH)
&       :       COLON
]       ;       SEMICOLON
^       =       EQUALS SIGN
@       ?       QUESTION MARK
}       §       SECTION SIGN (PARAGRAPH SIGN)
)       №       NUMERO SIGN
--      —       EM DASH
,,      „       DOUBLE LOW-9 QUOTATION MARK
``      “       LEFT DOUBLE QUOTATION  MARK
<<      «       LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
>>      »       RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
00      •       BULLET
..      ·       MIDDLE DOT      
+-      ±       PLUS-MINUS SIGN
~~      ¬       NOT SIGN
@@      ¤       CURRENCY SIGN
$$      €       EURO SIGN
