" Vim syntax file
" Language:         dircolors(1) input file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword dircolorsTodo       contained FIXME TODO XXX NOTE

syn region  dircolorsComment    start='#' end='$' contains=dircolorsTodo,@Spell

syn keyword dircolorsKeyword    TERM LEFT LEFTCODE RIGHT RIGHTCODE END ENDCODE

syn keyword dircolorsKeyword    NORMAL NORM FILE DIR LNK LINK SYMLINK ORPHAN
                                \ MISSING FIFO PIPE SOCK BLK BLOCK CHR CHAR
                                \ DOOR EXEC
                                \ nextgroup=@dircolorsColors skipwhite

if exists("dircolors_is_slackware")
  syn keyword dircolorsKeyword  COLOR OPTIONS EIGHTBIT
endif

syn match   dircolorsExtension  '^\s*\zs[.*]\S\+'
                                \ nextgroup=@dircolorsColors skipwhite

if &t_Co == 8 || &t_Co == 16
  syn cluster dircolorsColors   contains=dircolorsBold,dircolorsUnderline,
                                \ dircolorsBlink,dircolorsReverse,
                                \ dircolorsInvisible,dircolorsBlack,
                                \ dircolorsRed,dircolorsGreen,dircolorsYellow,
                                \ dircolorsBlue,dircolorsMagenta,dircolorsCyan,
                                \ dircolorsWhite,dircolorsBGBlack,
                                \ dircolorsBGRed,dircolorsBGGreen,
                                \ dircolorsBGYellow,dircolorsBGBlue,
                                \ dircolorsBGMagenta,dircolorsBGCyan,
                                \ dircolorsBGWhite

  syn match dircolorsBold       contained '\<01\>'
  syn match dircolorsUnderline  contained '\<04\>'
  syn match dircolorsBlink      contained '\<05\>'
  syn match dircolorsReverse    contained '\<07\>'
  syn match dircolorsInvisible  contained '\<08\>'
  syn match dircolorsBlack      contained '\<30\>'
  syn match dircolorsRed        contained '\<31\>'
  syn match dircolorsGreen      contained '\<32\>'
  syn match dircolorsYellow     contained '\<33\>'
  syn match dircolorsBlue       contained '\<34\>'
  syn match dircolorsMagenta    contained '\<35\>'
  syn match dircolorsCyan       contained '\<36\>'
  syn match dircolorsWhite      contained '\<37\>'
  syn match dircolorsBGBlack    contained '\<40\>'
  syn match dircolorsBGRed      contained '\<41\>'
  syn match dircolorsBGGreen    contained '\<42\>'
  syn match dircolorsBGYellow   contained '\<43\>'
  syn match dircolorsBGBlue     contained '\<44\>'
  syn match dircolorsBGMagenta  contained '\<45\>'
  syn match dircolorsBGCyan     contained '\<46\>'
  syn match dircolorsBGWhite    contained '\<47\>'
elseif &t_Co == 256
  syn cluster dircolorsColors   contains=dircolorsColor1,dircolorsColor2,
                                \ dircolorsColor3,dircolorsColor4,
                                \ dircolorsColor5,dircolorsColor6,
                                \ dircolorsColor7,dircolorsColor8,
                                \ dircolorsColor9,dircolorsColor10,
                                \ dircolorsColor11,dircolorsColor12,
                                \ dircolorsColor13,dircolorsColor14,
                                \ dircolorsColor15,dircolorsColor16,
                                \ dircolorsColor17,dircolorsColor18,
                                \ dircolorsColor19,dircolorsColor20,
                                \ dircolorsColor21,dircolorsColor22,
                                \ dircolorsColor23,dircolorsColor24,
                                \ dircolorsColor25,dircolorsColor26,
                                \ dircolorsColor27,dircolorsColor28,
                                \ dircolorsColor29,dircolorsColor30,
                                \ dircolorsColor31,dircolorsColor32,
                                \ dircolorsColor33,dircolorsColor34,
                                \ dircolorsColor35,dircolorsColor36,
                                \ dircolorsColor37,dircolorsColor38,
                                \ dircolorsColor39,dircolorsColor40,
                                \ dircolorsColor41,dircolorsColor42,
                                \ dircolorsColor43,dircolorsColor44,
                                \ dircolorsColor45,dircolorsColor46,
                                \ dircolorsColor47,dircolorsColor48,
                                \ dircolorsColor49,dircolorsColor50,
                                \ dircolorsColor51,dircolorsColor52,
                                \ dircolorsColor53,dircolorsColor54,
                                \ dircolorsColor55,dircolorsColor56,
                                \ dircolorsColor57,dircolorsColor58,
                                \ dircolorsColor59,dircolorsColor60,
                                \ dircolorsColor61,dircolorsColor62,
                                \ dircolorsColor63,dircolorsColor64,
                                \ dircolorsColor65,dircolorsColor66,
                                \ dircolorsColor67,dircolorsColor68,
                                \ dircolorsColor69,dircolorsColor70,
                                \ dircolorsColor71,dircolorsColor72,
                                \ dircolorsColor73,dircolorsColor74,
                                \ dircolorsColor75,dircolorsColor76,
                                \ dircolorsColor77,dircolorsColor78,
                                \ dircolorsColor79,dircolorsColor80,
                                \ dircolorsColor81,dircolorsColor82,
                                \ dircolorsColor83,dircolorsColor84,
                                \ dircolorsColor85,dircolorsColor86,
                                \ dircolorsColor87,dircolorsColor88,
                                \ dircolorsColor89,dircolorsColor90,
                                \ dircolorsColor91,dircolorsColor92,
                                \ dircolorsColor93,dircolorsColor94,
                                \ dircolorsColor95,dircolorsColor96,
                                \ dircolorsColor97,dircolorsColor98,
                                \ dircolorsColor99,dircolorsColor100,
                                \ dircolorsColor101,dircolorsColor102,
                                \ dircolorsColor103,dircolorsColor104,
                                \ dircolorsColor105,dircolorsColor106,
                                \ dircolorsColor107,dircolorsColor108,
                                \ dircolorsColor109,dircolorsColor110,
                                \ dircolorsColor111,dircolorsColor112,
                                \ dircolorsColor113,dircolorsColor114,
                                \ dircolorsColor115,dircolorsColor116,
                                \ dircolorsColor117,dircolorsColor118,
                                \ dircolorsColor119,dircolorsColor120,
                                \ dircolorsColor121,dircolorsColor122,
                                \ dircolorsColor123,dircolorsColor124,
                                \ dircolorsColor125,dircolorsColor126,
                                \ dircolorsColor127,dircolorsColor128,
                                \ dircolorsColor129,dircolorsColor130,
                                \ dircolorsColor131,dircolorsColor132,
                                \ dircolorsColor133,dircolorsColor134,
                                \ dircolorsColor135,dircolorsColor136,
                                \ dircolorsColor137,dircolorsColor138,
                                \ dircolorsColor139,dircolorsColor140,
                                \ dircolorsColor141,dircolorsColor142,
                                \ dircolorsColor143,dircolorsColor144,
                                \ dircolorsColor145,dircolorsColor146,
                                \ dircolorsColor147,dircolorsColor148,
                                \ dircolorsColor149,dircolorsColor150,
                                \ dircolorsColor151,dircolorsColor152,
                                \ dircolorsColor153,dircolorsColor154,
                                \ dircolorsColor155,dircolorsColor156,
                                \ dircolorsColor157,dircolorsColor158,
                                \ dircolorsColor159,dircolorsColor160,
                                \ dircolorsColor161,dircolorsColor162,
                                \ dircolorsColor163,dircolorsColor164,
                                \ dircolorsColor165,dircolorsColor166,
                                \ dircolorsColor167,dircolorsColor168,
                                \ dircolorsColor169,dircolorsColor170,
                                \ dircolorsColor171,dircolorsColor172,
                                \ dircolorsColor173,dircolorsColor174,
                                \ dircolorsColor175,dircolorsColor176,
                                \ dircolorsColor177,dircolorsColor178,
                                \ dircolorsColor179,dircolorsColor180,
                                \ dircolorsColor181,dircolorsColor182,
                                \ dircolorsColor183,dircolorsColor184,
                                \ dircolorsColor185,dircolorsColor186,
                                \ dircolorsColor187,dircolorsColor188,
                                \ dircolorsColor189,dircolorsColor190,
                                \ dircolorsColor191,dircolorsColor192,
                                \ dircolorsColor193,dircolorsColor194,
                                \ dircolorsColor195,dircolorsColor196,
                                \ dircolorsColor197,dircolorsColor198,
                                \ dircolorsColor199,dircolorsColor200,
                                \ dircolorsColor201,dircolorsColor202,
                                \ dircolorsColor203,dircolorsColor204,
                                \ dircolorsColor205,dircolorsColor206,
                                \ dircolorsColor207,dircolorsColor208,
                                \ dircolorsColor209,dircolorsColor210,
                                \ dircolorsColor211,dircolorsColor212,
                                \ dircolorsColor213,dircolorsColor214,
                                \ dircolorsColor215,dircolorsColor216,
                                \ dircolorsColor217,dircolorsColor218,
                                \ dircolorsColor219,dircolorsColor220,
                                \ dircolorsColor221,dircolorsColor222,
                                \ dircolorsColor223,dircolorsColor224,
                                \ dircolorsColor225,dircolorsColor226,
                                \ dircolorsColor227,dircolorsColor228,
                                \ dircolorsColor229,dircolorsColor230,
                                \ dircolorsColor231,dircolorsColor232,
                                \ dircolorsColor233,dircolorsColor234,
                                \ dircolorsColor235,dircolorsColor236,
                                \ dircolorsColor237,dircolorsColor238,
                                \ dircolorsColor239,dircolorsColor240,
                                \ dircolorsColor241,dircolorsColor242,
                                \ dircolorsColor243,dircolorsColor244,
                                \ dircolorsColor245,dircolorsColor246,
                                \ dircolorsColor247,dircolorsColor248,
                                \ dircolorsColor249,dircolorsColor250,
                                \ dircolorsColor251,dircolorsColor252,
                                \ dircolorsColor253,dircolorsColor254,
                                \ dircolorsColor255

  syn match dircolorsColor1     contained '\<1\>'
  syn match dircolorsColor2     contained '\<2\>'
  syn match dircolorsColor3     contained '\<3\>'
  syn match dircolorsColor4     contained '\<4\>'
  syn match dircolorsColor5     contained '\<5\>'
  syn match dircolorsColor6     contained '\<6\>'
  syn match dircolorsColor7     contained '\<7\>'
  syn match dircolorsColor8     contained '\<8\>'
  syn match dircolorsColor9     contained '\<9\>'
  syn match dircolorsColor10    contained '\<10\>'
  syn match dircolorsColor11    contained '\<11\>'
  syn match dircolorsColor12    contained '\<12\>'
  syn match dircolorsColor13    contained '\<13\>'
  syn match dircolorsColor14    contained '\<14\>'
  syn match dircolorsColor15    contained '\<15\>'
  syn match dircolorsColor16    contained '\<16\>'
  syn match dircolorsColor17    contained '\<17\>'
  syn match dircolorsColor18    contained '\<18\>'
  syn match dircolorsColor19    contained '\<19\>'
  syn match dircolorsColor20    contained '\<20\>'
  syn match dircolorsColor21    contained '\<21\>'
  syn match dircolorsColor22    contained '\<22\>'
  syn match dircolorsColor23    contained '\<23\>'
  syn match dircolorsColor24    contained '\<24\>'
  syn match dircolorsColor25    contained '\<25\>'
  syn match dircolorsColor26    contained '\<26\>'
  syn match dircolorsColor27    contained '\<27\>'
  syn match dircolorsColor28    contained '\<28\>'
  syn match dircolorsColor29    contained '\<29\>'
  syn match dircolorsColor30    contained '\<30\>'
  syn match dircolorsColor31    contained '\<31\>'
  syn match dircolorsColor32    contained '\<32\>'
  syn match dircolorsColor33    contained '\<33\>'
  syn match dircolorsColor34    contained '\<34\>'
  syn match dircolorsColor35    contained '\<35\>'
  syn match dircolorsColor36    contained '\<36\>'
  syn match dircolorsColor37    contained '\<37\>'
  syn match dircolorsColor38    contained '\<38\>'
  syn match dircolorsColor39    contained '\<39\>'
  syn match dircolorsColor40    contained '\<40\>'
  syn match dircolorsColor41    contained '\<41\>'
  syn match dircolorsColor42    contained '\<42\>'
  syn match dircolorsColor43    contained '\<43\>'
  syn match dircolorsColor44    contained '\<44\>'
  syn match dircolorsColor45    contained '\<45\>'
  syn match dircolorsColor46    contained '\<46\>'
  syn match dircolorsColor47    contained '\<47\>'
  syn match dircolorsColor48    contained '\<48\>'
  syn match dircolorsColor49    contained '\<49\>'
  syn match dircolorsColor50    contained '\<50\>'
  syn match dircolorsColor51    contained '\<51\>'
  syn match dircolorsColor52    contained '\<52\>'
  syn match dircolorsColor53    contained '\<53\>'
  syn match dircolorsColor54    contained '\<54\>'
  syn match dircolorsColor55    contained '\<55\>'
  syn match dircolorsColor56    contained '\<56\>'
  syn match dircolorsColor57    contained '\<57\>'
  syn match dircolorsColor58    contained '\<58\>'
  syn match dircolorsColor59    contained '\<59\>'
  syn match dircolorsColor60    contained '\<60\>'
  syn match dircolorsColor61    contained '\<61\>'
  syn match dircolorsColor62    contained '\<62\>'
  syn match dircolorsColor63    contained '\<63\>'
  syn match dircolorsColor64    contained '\<64\>'
  syn match dircolorsColor65    contained '\<65\>'
  syn match dircolorsColor66    contained '\<66\>'
  syn match dircolorsColor67    contained '\<67\>'
  syn match dircolorsColor68    contained '\<68\>'
  syn match dircolorsColor69    contained '\<69\>'
  syn match dircolorsColor70    contained '\<70\>'
  syn match dircolorsColor71    contained '\<71\>'
  syn match dircolorsColor72    contained '\<72\>'
  syn match dircolorsColor73    contained '\<73\>'
  syn match dircolorsColor74    contained '\<74\>'
  syn match dircolorsColor75    contained '\<75\>'
  syn match dircolorsColor76    contained '\<76\>'
  syn match dircolorsColor77    contained '\<77\>'
  syn match dircolorsColor78    contained '\<78\>'
  syn match dircolorsColor79    contained '\<79\>'
  syn match dircolorsColor80    contained '\<80\>'
  syn match dircolorsColor81    contained '\<81\>'
  syn match dircolorsColor82    contained '\<82\>'
  syn match dircolorsColor83    contained '\<83\>'
  syn match dircolorsColor84    contained '\<84\>'
  syn match dircolorsColor85    contained '\<85\>'
  syn match dircolorsColor86    contained '\<86\>'
  syn match dircolorsColor87    contained '\<87\>'
  syn match dircolorsColor88    contained '\<88\>'
  syn match dircolorsColor89    contained '\<89\>'
  syn match dircolorsColor90    contained '\<90\>'
  syn match dircolorsColor91    contained '\<91\>'
  syn match dircolorsColor92    contained '\<92\>'
  syn match dircolorsColor93    contained '\<93\>'
  syn match dircolorsColor94    contained '\<94\>'
  syn match dircolorsColor95    contained '\<95\>'
  syn match dircolorsColor96    contained '\<96\>'
  syn match dircolorsColor97    contained '\<97\>'
  syn match dircolorsColor98    contained '\<98\>'
  syn match dircolorsColor99    contained '\<99\>'
  syn match dircolorsColor100   contained '\<100\>'
  syn match dircolorsColor101   contained '\<101\>'
  syn match dircolorsColor102   contained '\<102\>'
  syn match dircolorsColor103   contained '\<103\>'
  syn match dircolorsColor104   contained '\<104\>'
  syn match dircolorsColor105   contained '\<105\>'
  syn match dircolorsColor106   contained '\<106\>'
  syn match dircolorsColor107   contained '\<107\>'
  syn match dircolorsColor108   contained '\<108\>'
  syn match dircolorsColor109   contained '\<109\>'
  syn match dircolorsColor110   contained '\<110\>'
  syn match dircolorsColor111   contained '\<111\>'
  syn match dircolorsColor112   contained '\<112\>'
  syn match dircolorsColor113   contained '\<113\>'
  syn match dircolorsColor114   contained '\<114\>'
  syn match dircolorsColor115   contained '\<115\>'
  syn match dircolorsColor116   contained '\<116\>'
  syn match dircolorsColor117   contained '\<117\>'
  syn match dircolorsColor118   contained '\<118\>'
  syn match dircolorsColor119   contained '\<119\>'
  syn match dircolorsColor120   contained '\<120\>'
  syn match dircolorsColor121   contained '\<121\>'
  syn match dircolorsColor122   contained '\<122\>'
  syn match dircolorsColor123   contained '\<123\>'
  syn match dircolorsColor124   contained '\<124\>'
  syn match dircolorsColor125   contained '\<125\>'
  syn match dircolorsColor126   contained '\<126\>'
  syn match dircolorsColor127   contained '\<127\>'
  syn match dircolorsColor128   contained '\<128\>'
  syn match dircolorsColor129   contained '\<129\>'
  syn match dircolorsColor130   contained '\<130\>'
  syn match dircolorsColor131   contained '\<131\>'
  syn match dircolorsColor132   contained '\<132\>'
  syn match dircolorsColor133   contained '\<133\>'
  syn match dircolorsColor134   contained '\<134\>'
  syn match dircolorsColor135   contained '\<135\>'
  syn match dircolorsColor136   contained '\<136\>'
  syn match dircolorsColor137   contained '\<137\>'
  syn match dircolorsColor138   contained '\<138\>'
  syn match dircolorsColor139   contained '\<139\>'
  syn match dircolorsColor140   contained '\<140\>'
  syn match dircolorsColor141   contained '\<141\>'
  syn match dircolorsColor142   contained '\<142\>'
  syn match dircolorsColor143   contained '\<143\>'
  syn match dircolorsColor144   contained '\<144\>'
  syn match dircolorsColor145   contained '\<145\>'
  syn match dircolorsColor146   contained '\<146\>'
  syn match dircolorsColor147   contained '\<147\>'
  syn match dircolorsColor148   contained '\<148\>'
  syn match dircolorsColor149   contained '\<149\>'
  syn match dircolorsColor150   contained '\<150\>'
  syn match dircolorsColor151   contained '\<151\>'
  syn match dircolorsColor152   contained '\<152\>'
  syn match dircolorsColor153   contained '\<153\>'
  syn match dircolorsColor154   contained '\<154\>'
  syn match dircolorsColor155   contained '\<155\>'
  syn match dircolorsColor156   contained '\<156\>'
  syn match dircolorsColor157   contained '\<157\>'
  syn match dircolorsColor158   contained '\<158\>'
  syn match dircolorsColor159   contained '\<159\>'
  syn match dircolorsColor160   contained '\<160\>'
  syn match dircolorsColor161   contained '\<161\>'
  syn match dircolorsColor162   contained '\<162\>'
  syn match dircolorsColor163   contained '\<163\>'
  syn match dircolorsColor164   contained '\<164\>'
  syn match dircolorsColor165   contained '\<165\>'
  syn match dircolorsColor166   contained '\<166\>'
  syn match dircolorsColor167   contained '\<167\>'
  syn match dircolorsColor168   contained '\<168\>'
  syn match dircolorsColor169   contained '\<169\>'
  syn match dircolorsColor170   contained '\<170\>'
  syn match dircolorsColor171   contained '\<171\>'
  syn match dircolorsColor172   contained '\<172\>'
  syn match dircolorsColor173   contained '\<173\>'
  syn match dircolorsColor174   contained '\<174\>'
  syn match dircolorsColor175   contained '\<175\>'
  syn match dircolorsColor176   contained '\<176\>'
  syn match dircolorsColor177   contained '\<177\>'
  syn match dircolorsColor178   contained '\<178\>'
  syn match dircolorsColor179   contained '\<179\>'
  syn match dircolorsColor180   contained '\<180\>'
  syn match dircolorsColor181   contained '\<181\>'
  syn match dircolorsColor182   contained '\<182\>'
  syn match dircolorsColor183   contained '\<183\>'
  syn match dircolorsColor184   contained '\<184\>'
  syn match dircolorsColor185   contained '\<185\>'
  syn match dircolorsColor186   contained '\<186\>'
  syn match dircolorsColor187   contained '\<187\>'
  syn match dircolorsColor188   contained '\<188\>'
  syn match dircolorsColor189   contained '\<189\>'
  syn match dircolorsColor190   contained '\<190\>'
  syn match dircolorsColor191   contained '\<191\>'
  syn match dircolorsColor192   contained '\<192\>'
  syn match dircolorsColor193   contained '\<193\>'
  syn match dircolorsColor194   contained '\<194\>'
  syn match dircolorsColor195   contained '\<195\>'
  syn match dircolorsColor196   contained '\<196\>'
  syn match dircolorsColor197   contained '\<197\>'
  syn match dircolorsColor198   contained '\<198\>'
  syn match dircolorsColor199   contained '\<199\>'
  syn match dircolorsColor200   contained '\<200\>'
  syn match dircolorsColor201   contained '\<201\>'
  syn match dircolorsColor202   contained '\<202\>'
  syn match dircolorsColor203   contained '\<203\>'
  syn match dircolorsColor204   contained '\<204\>'
  syn match dircolorsColor205   contained '\<205\>'
  syn match dircolorsColor206   contained '\<206\>'
  syn match dircolorsColor207   contained '\<207\>'
  syn match dircolorsColor208   contained '\<208\>'
  syn match dircolorsColor209   contained '\<209\>'
  syn match dircolorsColor210   contained '\<210\>'
  syn match dircolorsColor211   contained '\<211\>'
  syn match dircolorsColor212   contained '\<212\>'
  syn match dircolorsColor213   contained '\<213\>'
  syn match dircolorsColor214   contained '\<214\>'
  syn match dircolorsColor215   contained '\<215\>'
  syn match dircolorsColor216   contained '\<216\>'
  syn match dircolorsColor217   contained '\<217\>'
  syn match dircolorsColor218   contained '\<218\>'
  syn match dircolorsColor219   contained '\<219\>'
  syn match dircolorsColor220   contained '\<220\>'
  syn match dircolorsColor221   contained '\<221\>'
  syn match dircolorsColor222   contained '\<222\>'
  syn match dircolorsColor223   contained '\<223\>'
  syn match dircolorsColor224   contained '\<224\>'
  syn match dircolorsColor225   contained '\<225\>'
  syn match dircolorsColor226   contained '\<226\>'
  syn match dircolorsColor227   contained '\<227\>'
  syn match dircolorsColor228   contained '\<228\>'
  syn match dircolorsColor229   contained '\<229\>'
  syn match dircolorsColor230   contained '\<230\>'
  syn match dircolorsColor231   contained '\<231\>'
  syn match dircolorsColor232   contained '\<232\>'
  syn match dircolorsColor233   contained '\<233\>'
  syn match dircolorsColor234   contained '\<234\>'
  syn match dircolorsColor235   contained '\<235\>'
  syn match dircolorsColor236   contained '\<236\>'
  syn match dircolorsColor237   contained '\<237\>'
  syn match dircolorsColor238   contained '\<238\>'
  syn match dircolorsColor239   contained '\<239\>'
  syn match dircolorsColor240   contained '\<240\>'
  syn match dircolorsColor241   contained '\<241\>'
  syn match dircolorsColor242   contained '\<242\>'
  syn match dircolorsColor243   contained '\<243\>'
  syn match dircolorsColor244   contained '\<244\>'
  syn match dircolorsColor245   contained '\<245\>'
  syn match dircolorsColor246   contained '\<246\>'
  syn match dircolorsColor247   contained '\<247\>'
  syn match dircolorsColor248   contained '\<248\>'
  syn match dircolorsColor249   contained '\<249\>'
  syn match dircolorsColor250   contained '\<250\>'
  syn match dircolorsColor251   contained '\<251\>'
  syn match dircolorsColor252   contained '\<252\>'
  syn match dircolorsColor253   contained '\<253\>'
  syn match dircolorsColor254   contained '\<254\>'
  syn match dircolorsColor255   contained '\<255\>'
else
  syn cluster dircolorsColors   contains=dircolorsNumber
  syn match   dircolorsNumber   '\<\d\+\>'
endif

hi def link dircolorsTodo       Todo
hi def link dircolorsComment    Comment
hi def link dircolorsKeyword    Keyword
hi def link dircolorsExtension  Keyword

if &t_Co == 8 || &t_Co == 16
  hi def      dircolorsBold       term=bold cterm=bold gui=bold
  hi def      dircolorsUnderline  term=underline cterm=underline gui=underline
  hi def link dircolorsBlink      Normal
  hi def      dircolorsReverse    term=reverse cterm=reverse gui=reverse
  hi def link dircolorsInvisible  Ignore
  hi def      dircolorsBlack      ctermfg=Black guifg=Black
  hi def      dircolorsRed        ctermfg=Red guifg=Red
  hi def      dircolorsGreen      ctermfg=Green guifg=Green
  hi def      dircolorsYellow     ctermfg=Yellow guifg=Yellow
  hi def      dircolorsBlue       ctermfg=Blue guifg=Blue
  hi def      dircolorsMagenta    ctermfg=Magenta guifg=Magenta
  hi def      dircolorsCyan       ctermfg=Cyan guifg=Cyan
  hi def      dircolorsWhite      ctermfg=White guifg=White
  hi def      dircolorsBGBlack    ctermbg=Black ctermfg=White
                                  \ guibg=Black guifg=White
  hi def      dircolorsBGRed      ctermbg=DarkRed guibg=DarkRed
  hi def      dircolorsBGGreen    ctermbg=DarkGreen guibg=DarkGreen
  hi def      dircolorsBGYellow   ctermbg=DarkYellow guibg=DarkYellow
  hi def      dircolorsBGBlue     ctermbg=DarkBlue guibg=DarkBlue
  hi def      dircolorsBGMagenta  ctermbg=DarkMagenta guibg=DarkMagenta
  hi def      dircolorsBGCyan     ctermbg=DarkCyan guibg=DarkCyan
  hi def      dircolorsBGWhite    ctermbg=White ctermfg=Black
                                  \ guibg=White guifg=Black
elseif &t_Co == 256
  hi def    dircolorsColor1     ctermfg=1   guifg=1
  hi def    dircolorsColor2     ctermfg=2   guifg=2
  hi def    dircolorsColor3     ctermfg=3   guifg=3
  hi def    dircolorsColor4     ctermfg=4   guifg=4
  hi def    dircolorsColor5     ctermfg=5   guifg=5
  hi def    dircolorsColor6     ctermfg=6   guifg=6
  hi def    dircolorsColor7     ctermfg=7   guifg=7
  hi def    dircolorsColor8     ctermfg=8   guifg=8
  hi def    dircolorsColor9     ctermfg=9   guifg=9
  hi def    dircolorsColor10    ctermfg=10  guifg=10
  hi def    dircolorsColor11    ctermfg=11  guifg=11
  hi def    dircolorsColor12    ctermfg=12  guifg=12
  hi def    dircolorsColor13    ctermfg=13  guifg=13
  hi def    dircolorsColor14    ctermfg=14  guifg=14
  hi def    dircolorsColor15    ctermfg=15  guifg=15
  hi def    dircolorsColor16    ctermfg=16  guifg=16
  hi def    dircolorsColor17    ctermfg=17  guifg=17
  hi def    dircolorsColor18    ctermfg=18  guifg=18
  hi def    dircolorsColor19    ctermfg=19  guifg=19
  hi def    dircolorsColor20    ctermfg=20  guifg=20
  hi def    dircolorsColor21    ctermfg=21  guifg=21
  hi def    dircolorsColor22    ctermfg=22  guifg=22
  hi def    dircolorsColor23    ctermfg=23  guifg=23
  hi def    dircolorsColor24    ctermfg=24  guifg=24
  hi def    dircolorsColor25    ctermfg=25  guifg=25
  hi def    dircolorsColor26    ctermfg=26  guifg=26
  hi def    dircolorsColor27    ctermfg=27  guifg=27
  hi def    dircolorsColor28    ctermfg=28  guifg=28
  hi def    dircolorsColor29    ctermfg=29  guifg=29
  hi def    dircolorsColor30    ctermfg=30  guifg=30
  hi def    dircolorsColor31    ctermfg=31  guifg=31
  hi def    dircolorsColor32    ctermfg=32  guifg=32
  hi def    dircolorsColor33    ctermfg=33  guifg=33
  hi def    dircolorsColor34    ctermfg=34  guifg=34
  hi def    dircolorsColor35    ctermfg=35  guifg=35
  hi def    dircolorsColor36    ctermfg=36  guifg=36
  hi def    dircolorsColor37    ctermfg=37  guifg=37
  hi def    dircolorsColor38    ctermfg=38  guifg=38
  hi def    dircolorsColor39    ctermfg=39  guifg=39
  hi def    dircolorsColor40    ctermfg=40  guifg=40
  hi def    dircolorsColor41    ctermfg=41  guifg=41
  hi def    dircolorsColor42    ctermfg=42  guifg=42
  hi def    dircolorsColor43    ctermfg=43  guifg=43
  hi def    dircolorsColor44    ctermfg=44  guifg=44
  hi def    dircolorsColor45    ctermfg=45  guifg=45
  hi def    dircolorsColor46    ctermfg=46  guifg=46
  hi def    dircolorsColor47    ctermfg=47  guifg=47
  hi def    dircolorsColor48    ctermfg=48  guifg=48
  hi def    dircolorsColor49    ctermfg=49  guifg=49
  hi def    dircolorsColor50    ctermfg=50  guifg=50
  hi def    dircolorsColor51    ctermfg=51  guifg=51
  hi def    dircolorsColor52    ctermfg=52  guifg=52
  hi def    dircolorsColor53    ctermfg=53  guifg=53
  hi def    dircolorsColor54    ctermfg=54  guifg=54
  hi def    dircolorsColor55    ctermfg=55  guifg=55
  hi def    dircolorsColor56    ctermfg=56  guifg=56
  hi def    dircolorsColor57    ctermfg=57  guifg=57
  hi def    dircolorsColor58    ctermfg=58  guifg=58
  hi def    dircolorsColor59    ctermfg=59  guifg=59
  hi def    dircolorsColor60    ctermfg=60  guifg=60
  hi def    dircolorsColor61    ctermfg=61  guifg=61
  hi def    dircolorsColor62    ctermfg=62  guifg=62
  hi def    dircolorsColor63    ctermfg=63  guifg=63
  hi def    dircolorsColor64    ctermfg=64  guifg=64
  hi def    dircolorsColor65    ctermfg=65  guifg=65
  hi def    dircolorsColor66    ctermfg=66  guifg=66
  hi def    dircolorsColor67    ctermfg=67  guifg=67
  hi def    dircolorsColor68    ctermfg=68  guifg=68
  hi def    dircolorsColor69    ctermfg=69  guifg=69
  hi def    dircolorsColor70    ctermfg=70  guifg=70
  hi def    dircolorsColor71    ctermfg=71  guifg=71
  hi def    dircolorsColor72    ctermfg=72  guifg=72
  hi def    dircolorsColor73    ctermfg=73  guifg=73
  hi def    dircolorsColor74    ctermfg=74  guifg=74
  hi def    dircolorsColor75    ctermfg=75  guifg=75
  hi def    dircolorsColor76    ctermfg=76  guifg=76
  hi def    dircolorsColor77    ctermfg=77  guifg=77
  hi def    dircolorsColor78    ctermfg=78  guifg=78
  hi def    dircolorsColor79    ctermfg=79  guifg=79
  hi def    dircolorsColor80    ctermfg=80  guifg=80
  hi def    dircolorsColor81    ctermfg=81  guifg=81
  hi def    dircolorsColor82    ctermfg=82  guifg=82
  hi def    dircolorsColor83    ctermfg=83  guifg=83
  hi def    dircolorsColor84    ctermfg=84  guifg=84
  hi def    dircolorsColor85    ctermfg=85  guifg=85
  hi def    dircolorsColor86    ctermfg=86  guifg=86
  hi def    dircolorsColor87    ctermfg=87  guifg=87
  hi def    dircolorsColor88    ctermfg=88  guifg=88
  hi def    dircolorsColor89    ctermfg=89  guifg=89
  hi def    dircolorsColor90    ctermfg=90  guifg=90
  hi def    dircolorsColor91    ctermfg=91  guifg=91
  hi def    dircolorsColor92    ctermfg=92  guifg=92
  hi def    dircolorsColor93    ctermfg=93  guifg=93
  hi def    dircolorsColor94    ctermfg=94  guifg=94
  hi def    dircolorsColor95    ctermfg=95  guifg=95
  hi def    dircolorsColor96    ctermfg=96  guifg=96
  hi def    dircolorsColor97    ctermfg=97  guifg=97
  hi def    dircolorsColor98    ctermfg=98  guifg=98
  hi def    dircolorsColor99    ctermfg=99  guifg=99
  hi def    dircolorsColor100   ctermfg=100 guifg=100
  hi def    dircolorsColor101   ctermfg=101 guifg=101
  hi def    dircolorsColor102   ctermfg=102 guifg=102
  hi def    dircolorsColor103   ctermfg=103 guifg=103
  hi def    dircolorsColor104   ctermfg=104 guifg=104
  hi def    dircolorsColor105   ctermfg=105 guifg=105
  hi def    dircolorsColor106   ctermfg=106 guifg=106
  hi def    dircolorsColor107   ctermfg=107 guifg=107
  hi def    dircolorsColor108   ctermfg=108 guifg=108
  hi def    dircolorsColor109   ctermfg=109 guifg=109
  hi def    dircolorsColor110   ctermfg=110 guifg=110
  hi def    dircolorsColor111   ctermfg=111 guifg=111
  hi def    dircolorsColor112   ctermfg=112 guifg=112
  hi def    dircolorsColor113   ctermfg=113 guifg=113
  hi def    dircolorsColor114   ctermfg=114 guifg=114
  hi def    dircolorsColor115   ctermfg=115 guifg=115
  hi def    dircolorsColor116   ctermfg=116 guifg=116
  hi def    dircolorsColor117   ctermfg=117 guifg=117
  hi def    dircolorsColor118   ctermfg=118 guifg=118
  hi def    dircolorsColor119   ctermfg=119 guifg=119
  hi def    dircolorsColor120   ctermfg=120 guifg=120
  hi def    dircolorsColor121   ctermfg=121 guifg=121
  hi def    dircolorsColor122   ctermfg=122 guifg=122
  hi def    dircolorsColor123   ctermfg=123 guifg=123
  hi def    dircolorsColor124   ctermfg=124 guifg=124
  hi def    dircolorsColor125   ctermfg=125 guifg=125
  hi def    dircolorsColor126   ctermfg=126 guifg=126
  hi def    dircolorsColor127   ctermfg=127 guifg=127
  hi def    dircolorsColor128   ctermfg=128 guifg=128
  hi def    dircolorsColor129   ctermfg=129 guifg=129
  hi def    dircolorsColor130   ctermfg=130 guifg=130
  hi def    dircolorsColor131   ctermfg=131 guifg=131
  hi def    dircolorsColor132   ctermfg=132 guifg=132
  hi def    dircolorsColor133   ctermfg=133 guifg=133
  hi def    dircolorsColor134   ctermfg=134 guifg=134
  hi def    dircolorsColor135   ctermfg=135 guifg=135
  hi def    dircolorsColor136   ctermfg=136 guifg=136
  hi def    dircolorsColor137   ctermfg=137 guifg=137
  hi def    dircolorsColor138   ctermfg=138 guifg=138
  hi def    dircolorsColor139   ctermfg=139 guifg=139
  hi def    dircolorsColor140   ctermfg=140 guifg=140
  hi def    dircolorsColor141   ctermfg=141 guifg=141
  hi def    dircolorsColor142   ctermfg=142 guifg=142
  hi def    dircolorsColor143   ctermfg=143 guifg=143
  hi def    dircolorsColor144   ctermfg=144 guifg=144
  hi def    dircolorsColor145   ctermfg=145 guifg=145
  hi def    dircolorsColor146   ctermfg=146 guifg=146
  hi def    dircolorsColor147   ctermfg=147 guifg=147
  hi def    dircolorsColor148   ctermfg=148 guifg=148
  hi def    dircolorsColor149   ctermfg=149 guifg=149
  hi def    dircolorsColor150   ctermfg=150 guifg=150
  hi def    dircolorsColor151   ctermfg=151 guifg=151
  hi def    dircolorsColor152   ctermfg=152 guifg=152
  hi def    dircolorsColor153   ctermfg=153 guifg=153
  hi def    dircolorsColor154   ctermfg=154 guifg=154
  hi def    dircolorsColor155   ctermfg=155 guifg=155
  hi def    dircolorsColor156   ctermfg=156 guifg=156
  hi def    dircolorsColor157   ctermfg=157 guifg=157
  hi def    dircolorsColor158   ctermfg=158 guifg=158
  hi def    dircolorsColor159   ctermfg=159 guifg=159
  hi def    dircolorsColor160   ctermfg=160 guifg=160
  hi def    dircolorsColor161   ctermfg=161 guifg=161
  hi def    dircolorsColor162   ctermfg=162 guifg=162
  hi def    dircolorsColor163   ctermfg=163 guifg=163
  hi def    dircolorsColor164   ctermfg=164 guifg=164
  hi def    dircolorsColor165   ctermfg=165 guifg=165
  hi def    dircolorsColor166   ctermfg=166 guifg=166
  hi def    dircolorsColor167   ctermfg=167 guifg=167
  hi def    dircolorsColor168   ctermfg=168 guifg=168
  hi def    dircolorsColor169   ctermfg=169 guifg=169
  hi def    dircolorsColor170   ctermfg=170 guifg=170
  hi def    dircolorsColor171   ctermfg=171 guifg=171
  hi def    dircolorsColor172   ctermfg=172 guifg=172
  hi def    dircolorsColor173   ctermfg=173 guifg=173
  hi def    dircolorsColor174   ctermfg=174 guifg=174
  hi def    dircolorsColor175   ctermfg=175 guifg=175
  hi def    dircolorsColor176   ctermfg=176 guifg=176
  hi def    dircolorsColor177   ctermfg=177 guifg=177
  hi def    dircolorsColor178   ctermfg=178 guifg=178
  hi def    dircolorsColor179   ctermfg=179 guifg=179
  hi def    dircolorsColor180   ctermfg=180 guifg=180
  hi def    dircolorsColor181   ctermfg=181 guifg=181
  hi def    dircolorsColor182   ctermfg=182 guifg=182
  hi def    dircolorsColor183   ctermfg=183 guifg=183
  hi def    dircolorsColor184   ctermfg=184 guifg=184
  hi def    dircolorsColor185   ctermfg=185 guifg=185
  hi def    dircolorsColor186   ctermfg=186 guifg=186
  hi def    dircolorsColor187   ctermfg=187 guifg=187
  hi def    dircolorsColor188   ctermfg=188 guifg=188
  hi def    dircolorsColor189   ctermfg=189 guifg=189
  hi def    dircolorsColor190   ctermfg=190 guifg=190
  hi def    dircolorsColor191   ctermfg=191 guifg=191
  hi def    dircolorsColor192   ctermfg=192 guifg=192
  hi def    dircolorsColor193   ctermfg=193 guifg=193
  hi def    dircolorsColor194   ctermfg=194 guifg=194
  hi def    dircolorsColor195   ctermfg=195 guifg=195
  hi def    dircolorsColor196   ctermfg=196 guifg=196
  hi def    dircolorsColor197   ctermfg=197 guifg=197
  hi def    dircolorsColor198   ctermfg=198 guifg=198
  hi def    dircolorsColor199   ctermfg=199 guifg=199
  hi def    dircolorsColor200   ctermfg=200 guifg=200
  hi def    dircolorsColor201   ctermfg=201 guifg=201
  hi def    dircolorsColor202   ctermfg=202 guifg=202
  hi def    dircolorsColor203   ctermfg=203 guifg=203
  hi def    dircolorsColor204   ctermfg=204 guifg=204
  hi def    dircolorsColor205   ctermfg=205 guifg=205
  hi def    dircolorsColor206   ctermfg=206 guifg=206
  hi def    dircolorsColor207   ctermfg=207 guifg=207
  hi def    dircolorsColor208   ctermfg=208 guifg=208
  hi def    dircolorsColor209   ctermfg=209 guifg=209
  hi def    dircolorsColor210   ctermfg=210 guifg=210
  hi def    dircolorsColor211   ctermfg=211 guifg=211
  hi def    dircolorsColor212   ctermfg=212 guifg=212
  hi def    dircolorsColor213   ctermfg=213 guifg=213
  hi def    dircolorsColor214   ctermfg=214 guifg=214
  hi def    dircolorsColor215   ctermfg=215 guifg=215
  hi def    dircolorsColor216   ctermfg=216 guifg=216
  hi def    dircolorsColor217   ctermfg=217 guifg=217
  hi def    dircolorsColor218   ctermfg=218 guifg=218
  hi def    dircolorsColor219   ctermfg=219 guifg=219
  hi def    dircolorsColor220   ctermfg=220 guifg=220
  hi def    dircolorsColor221   ctermfg=221 guifg=221
  hi def    dircolorsColor222   ctermfg=222 guifg=222
  hi def    dircolorsColor223   ctermfg=223 guifg=223
  hi def    dircolorsColor224   ctermfg=224 guifg=224
  hi def    dircolorsColor225   ctermfg=225 guifg=225
  hi def    dircolorsColor226   ctermfg=226 guifg=226
  hi def    dircolorsColor227   ctermfg=227 guifg=227
  hi def    dircolorsColor228   ctermfg=228 guifg=228
  hi def    dircolorsColor229   ctermfg=229 guifg=229
  hi def    dircolorsColor230   ctermfg=230 guifg=230
  hi def    dircolorsColor231   ctermfg=231 guifg=231
  hi def    dircolorsColor232   ctermfg=232 guifg=232
  hi def    dircolorsColor233   ctermfg=233 guifg=233
  hi def    dircolorsColor234   ctermfg=234 guifg=234
  hi def    dircolorsColor235   ctermfg=235 guifg=235
  hi def    dircolorsColor236   ctermfg=236 guifg=236
  hi def    dircolorsColor237   ctermfg=237 guifg=237
  hi def    dircolorsColor238   ctermfg=238 guifg=238
  hi def    dircolorsColor239   ctermfg=239 guifg=239
  hi def    dircolorsColor240   ctermfg=240 guifg=240
  hi def    dircolorsColor241   ctermfg=241 guifg=241
  hi def    dircolorsColor242   ctermfg=242 guifg=242
  hi def    dircolorsColor243   ctermfg=243 guifg=243
  hi def    dircolorsColor244   ctermfg=244 guifg=244
  hi def    dircolorsColor245   ctermfg=245 guifg=245
  hi def    dircolorsColor246   ctermfg=246 guifg=246
  hi def    dircolorsColor247   ctermfg=247 guifg=247
  hi def    dircolorsColor248   ctermfg=248 guifg=248
  hi def    dircolorsColor249   ctermfg=249 guifg=249
  hi def    dircolorsColor250   ctermfg=250 guifg=250
  hi def    dircolorsColor251   ctermfg=251 guifg=251
  hi def    dircolorsColor252   ctermfg=252 guifg=252
  hi def    dircolorsColor253   ctermfg=253 guifg=253
  hi def    dircolorsColor254   ctermfg=254 guifg=254
  hi def    dircolorsColor255   ctermfg=255 guifg=255
else
  hi def link dircolorsNumber     Number
endif

let b:current_syntax = "dircolors"

let &cpo = s:cpo_save
unlet s:cpo_save
