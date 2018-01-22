" Name:         Gruvbox 8 Soft
" Description:  Retro groove color scheme originally designed by morhetz
" Author:       morhetz <morhetz@gmail.com>
" Maintainer:   Lifepillar <lifepillar@lifepillar.me>
" Website:      https://github.com/lifepillar/vim-gruvbox8/
" License:      Vim License (see `:help license`)
" Last Updated: Mon Jan 22 22:23:30 2018

if !(has('termguicolors') && &termguicolors) && !has('gui_running')
      \ && (!exists('&t_Co') || &t_Co < 256)
  echoerr '[Gruvbox 8 Soft] There are not enough colors.'
  finish
endif

hi clear
if exists('syntax_on')
  syntax reset
endif

let g:colors_name = 'gruvbox8_soft'

if &background ==# 'dark'
  " Color similarity table (dark background)
  "           bg0: GUI=#32302f/rgb( 50, 48, 47)  Term=236 #303030/rgb( 48, 48, 48)  [delta=1.340761]
  " neutralpurple: GUI=#b16286/rgb(177, 98,134)  Term=132 #af5f87/rgb(175, 95,135)  [delta=1.447558]
  " neutralorange: GUI=#d65d0e/rgb(214, 93, 14)  Term=166 #d75f00/rgb(215, 95,  0)  [delta=1.594261]
  "           bg1: GUI=#3c3836/rgb( 60, 56, 54)  Term=237 #3a3a3a/rgb( 58, 58, 58)  [delta=2.591691]
  "         green: GUI=#b8bb26/rgb(184,187, 38)  Term=142 #afaf00/rgb(175,175,  0)  [delta=3.417395]
  "        orange: GUI=#fe8019/rgb(254,128, 25)  Term=208 #ff8700/rgb(255,135,  0)  [delta=3.424299]
  "    neutralred: GUI=#cc241d/rgb(204, 36, 29)  Term=160 #d70000/rgb(215,  0,  0)  [delta=3.678548]
  "           bg2: GUI=#504945/rgb( 80, 73, 69)  Term=239 #4e4e4e/rgb( 78, 78, 78)  [delta=4.437203]
  "           fg0: GUI=#fdf4c1/rgb(253,244,193)  Term=230 #ffffd7/rgb(255,255,215)  [delta=4.485567]
  "   neutralblue: GUI=#458588/rgb( 69,133,136)  Term= 66 #5f8787/rgb( 95,135,135)  [delta=4.654950]
  "        yellow: GUI=#fabd2f/rgb(250,189, 47)  Term=214 #ffaf00/rgb(255,175,  0)  [delta=5.124662]
  "        purple: GUI=#d3869b/rgb(211,134,155)  Term=175 #d787af/rgb(215,135,175)  [delta=5.579873]
  "  neutralgreen: GUI=#98971a/rgb(152,151, 26)  Term=100 #878700/rgb(135,135,  0)  [delta=5.597892]
  "          aqua: GUI=#8ec07c/rgb(142,192,124)  Term=107 #87af5f/rgb(135,175, 95)  [delta=5.816248]
  "          blue: GUI=#83a598/rgb(131,165,152)  Term=109 #87afaf/rgb(135,175,175)  [delta=6.121678]
  "           bg3: GUI=#665c54/rgb(102, 92, 84)  Term= 59 #5f5f5f/rgb( 95, 95, 95)  [delta=6.186264]
  " neutralyellow: GUI=#d79921/rgb(215,153, 33)  Term=172 #d78700/rgb(215,135,  0)  [delta=6.285960]
  "           fg1: GUI=#ebdbb2/rgb(235,219,178)  Term=187 #d7d7af/rgb(215,215,175)  [delta=6.290489]
  "   neutralaqua: GUI=#689d6a/rgb(104,157,106)  Term= 71 #5faf5f/rgb( 95,175, 95)  [delta=7.301224]
  "           bg4: GUI=#7c6f64/rgb(124,111,100)  Term=243 #767676/rgb(118,118,118)  [delta=7.889685]
  "           fg2: GUI=#d5c4a1/rgb(213,196,161)  Term=187 #d7d7af/rgb(215,215,175)  [delta=8.170537]
  "           red: GUI=#fb4934/rgb(251, 73, 52)  Term=203 #ff5f5f/rgb(255, 95, 95)  [delta=8.215867]
  "           fg3: GUI=#bdae93/rgb(189,174,147)  Term=144 #afaf87/rgb(175,175,135)  [delta=8.449971]
  "          grey: GUI=#928374/rgb(146,131,116)  Term=102 #878787/rgb(135,135,135)  [delta=8.970802]
  "           fg4: GUI=#a89984/rgb(168,153,132)  Term=137 #af875f/rgb(175,135, 95)  [delta=10.269702]
  if !has('gui_running') && get(g:, 'gruvbox_transp_bg', 0)
    hi Normal ctermfg=187 ctermbg=NONE guifg=#ebdbb2 guibg=NONE guisp=NONE cterm=NONE gui=NONE
    hi CursorLineNr ctermfg=214 ctermbg=NONE guifg=#fabd2f guibg=NONE guisp=NONE cterm=NONE gui=NONE
    hi FoldColumn ctermfg=102 ctermbg=NONE guifg=#928374 guibg=NONE guisp=NONE cterm=NONE gui=NONE
    hi SignColumn ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=NONE cterm=NONE gui=NONE
    hi VertSplit ctermfg=59 ctermbg=NONE guifg=#665c54 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  else
    hi Normal ctermfg=187 ctermbg=236 guifg=#ebdbb2 guibg=#32302f guisp=NONE cterm=NONE gui=NONE
    hi CursorLineNr ctermfg=214 ctermbg=237 guifg=#fabd2f guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
    hi FoldColumn ctermfg=102 ctermbg=237 guifg=#928374 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
    hi SignColumn ctermfg=NONE ctermbg=237 guifg=NONE guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
    hi VertSplit ctermfg=59 ctermbg=236 guifg=#665c54 guibg=#32302f guisp=NONE cterm=NONE gui=NONE
  endif
  hi ColorColumn ctermfg=NONE ctermbg=237 guifg=NONE guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi Conceal ctermfg=109 ctermbg=NONE guifg=#83a598 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Cursor ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi! link CursorColumn CursorLine
  hi CursorLine ctermfg=NONE ctermbg=237 guifg=NONE guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi DiffAdd ctermfg=142 ctermbg=236 guifg=#b8bb26 guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi DiffChange ctermfg=107 ctermbg=236 guifg=#8ec07c guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi DiffDelete ctermfg=203 ctermbg=236 guifg=#fb4934 guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi DiffText ctermfg=214 ctermbg=236 guifg=#fabd2f guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi Directory ctermfg=142 ctermbg=NONE guifg=#b8bb26 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi EndOfBuffer ctermfg=236 ctermbg=NONE guifg=#32302f guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Error ctermfg=203 ctermbg=236 guifg=#fb4934 guibg=#32302f guisp=NONE cterm=NONE,bold,reverse gui=NONE,bold,reverse
  hi ErrorMsg ctermfg=236 ctermbg=203 guifg=#32302f guibg=#fb4934 guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi Folded ctermfg=102 ctermbg=237 guifg=#928374 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE,italic
  hi IncSearch ctermfg=208 ctermbg=236 guifg=#fe8019 guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi LineNr ctermfg=243 ctermbg=NONE guifg=#7c6f64 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi MatchParen ctermfg=NONE ctermbg=59 guifg=NONE guibg=#665c54 guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi ModeMsg ctermfg=214 ctermbg=NONE guifg=#fabd2f guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi MoreMsg ctermfg=214 ctermbg=NONE guifg=#fabd2f guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi NonText ctermfg=239 ctermbg=NONE guifg=#504945 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Pmenu ctermfg=187 ctermbg=239 guifg=#ebdbb2 guibg=#504945 guisp=NONE cterm=NONE gui=NONE
  hi PmenuSbar ctermfg=NONE ctermbg=239 guifg=NONE guibg=#504945 guisp=NONE cterm=NONE gui=NONE
  hi PmenuSel ctermfg=239 ctermbg=109 guifg=#504945 guibg=#83a598 guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi PmenuThumb ctermfg=NONE ctermbg=243 guifg=NONE guibg=#7c6f64 guisp=NONE cterm=NONE gui=NONE
  hi Question ctermfg=208 ctermbg=NONE guifg=#fe8019 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi! link QuickFixLine Search
  hi Search ctermfg=214 ctermbg=236 guifg=#fabd2f guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi SpecialKey ctermfg=239 ctermbg=NONE guifg=#504945 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi SpellBad ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=#83a598 cterm=NONE,undercurl gui=NONE,undercurl
  if get(g:, "gruvbox_improved_warnings", 0)
    hi SpellCap ctermfg=142 ctermbg=NONE guifg=#b8bb26 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold,italic
  else
    hi SpellCap ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=#fb4934 cterm=NONE,undercurl gui=NONE,undercurl
  endif
  hi SpellLocal ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=#8ec07c cterm=NONE,undercurl gui=NONE,undercurl
  hi SpellRare ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=#d3869b cterm=NONE,undercurl gui=NONE,undercurl
  hi StatusLine ctermfg=239 ctermbg=187 guifg=#504945 guibg=#ebdbb2 guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi StatusLineNC ctermfg=237 ctermbg=137 guifg=#3c3836 guibg=#a89984 guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi! link StatusLineTerm StatusLine
  hi! link StatusLineTermNC StatusLineNC
  hi! link TabLine TabLineFill
  hi TabLineFill ctermfg=243 ctermbg=237 guifg=#7c6f64 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi TabLineSel ctermfg=142 ctermbg=237 guifg=#b8bb26 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi Title ctermfg=142 ctermbg=NONE guifg=#b8bb26 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  if get(g:, "gruvbox_invert_selection", 0)
    hi Visual ctermfg=NONE ctermbg=59 guifg=NONE guibg=#665c54 guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  else
    hi Visual ctermfg=NONE ctermbg=59 guifg=NONE guibg=#665c54 guisp=NONE cterm=NONE gui=NONE
  endif
  hi! link VisualNOS Visual
  hi WarningMsg ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi WildMenu ctermfg=109 ctermbg=239 guifg=#83a598 guibg=#504945 guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi Boolean ctermfg=175 ctermbg=NONE guifg=#d3869b guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Character ctermfg=175 ctermbg=NONE guifg=#d3869b guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Comment ctermfg=102 ctermbg=NONE guifg=#928374 guibg=NONE guisp=NONE cterm=NONE gui=NONE,italic
  hi Conditional ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Constant ctermfg=175 ctermbg=NONE guifg=#d3869b guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Define ctermfg=107 ctermbg=NONE guifg=#8ec07c guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Debug ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Delimiter ctermfg=208 ctermbg=NONE guifg=#fe8019 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Error ctermfg=203 ctermbg=236 guifg=#fb4934 guibg=#32302f guisp=NONE cterm=NONE,bold,reverse gui=NONE,bold,reverse
  hi Exception ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Float ctermfg=175 ctermbg=NONE guifg=#d3869b guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Function ctermfg=142 ctermbg=NONE guifg=#b8bb26 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi Identifier ctermfg=109 ctermbg=NONE guifg=#83a598 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Ignore ctermfg=fg ctermbg=NONE guifg=fg guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Include ctermfg=107 ctermbg=NONE guifg=#8ec07c guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Keyword ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Label ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Macro ctermfg=107 ctermbg=NONE guifg=#8ec07c guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Number ctermfg=175 ctermbg=NONE guifg=#d3869b guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi! link Operator Normal
  hi PreCondit ctermfg=107 ctermbg=NONE guifg=#8ec07c guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi PreProc ctermfg=107 ctermbg=NONE guifg=#8ec07c guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Repeat ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi SpecialChar ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi SpecialComment ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Statement ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi StorageClass ctermfg=208 ctermbg=NONE guifg=#fe8019 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  if get(g:, "gruvbox_improved_strings", 0)
    hi Special ctermfg=208 ctermbg=237 guifg=#fe8019 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
    hi String ctermfg=187 ctermbg=237 guifg=#ebdbb2 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  else
    hi Special ctermfg=208 ctermbg=NONE guifg=#fe8019 guibg=NONE guisp=NONE cterm=NONE gui=NONE
    hi String ctermfg=142 ctermbg=NONE guifg=#b8bb26 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  endif
  hi Structure ctermfg=107 ctermbg=NONE guifg=#8ec07c guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi! link Tag Special
  hi Todo ctermfg=fg ctermbg=236 guifg=fg guibg=#32302f guisp=NONE cterm=NONE,bold gui=NONE,bold,italic
  hi Type ctermfg=214 ctermbg=NONE guifg=#fabd2f guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Typedef ctermfg=214 ctermbg=NONE guifg=#fabd2f guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi Underlined ctermfg=109 ctermbg=NONE guifg=#83a598 guibg=NONE guisp=NONE cterm=NONE,underline gui=NONE,underline
  hi! link lCursor Cursor
  hi CursorIM ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi ToolbarLine ctermfg=NONE ctermbg=59 guifg=NONE guibg=#665c54 guisp=NONE cterm=NONE gui=NONE
  hi ToolbarButton ctermfg=230 ctermbg=59 guifg=#fdf4c1 guibg=#665c54 guisp=NONE cterm=NONE,bold gui=NONE,bold
  if get(g:, "gruvbox_italic", 0)
    hi Comment cterm=italic
    hi Folded cterm=italic
    hi SpellCap cterm=italic
    hi Todo cterm=italic
  endif
  if get(g:, "gruvbox_italicize_strings", 0)
    hi Special cterm=italic gui=italic
    hi String cterm=italic gui=italic
  endif
  hi GruvboxFg0 ctermfg=230 ctermbg=NONE guifg=#fdf4c1 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxFg1 ctermfg=187 ctermbg=NONE guifg=#ebdbb2 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxFg2 ctermfg=187 ctermbg=NONE guifg=#d5c4a1 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxFg3 ctermfg=144 ctermbg=NONE guifg=#bdae93 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxFg4 ctermfg=137 ctermbg=NONE guifg=#a89984 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxGray ctermfg=102 ctermbg=NONE guifg=#928374 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxBg0 ctermfg=236 ctermbg=NONE guifg=#32302f guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxBg1 ctermfg=237 ctermbg=NONE guifg=#3c3836 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxBg2 ctermfg=239 ctermbg=NONE guifg=#504945 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxBg3 ctermfg=59 ctermbg=NONE guifg=#665c54 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxBg4 ctermfg=243 ctermbg=NONE guifg=#7c6f64 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxRed ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxRedBold ctermfg=203 ctermbg=NONE guifg=#fb4934 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi GruvboxGreen ctermfg=142 ctermbg=NONE guifg=#b8bb26 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxGreenBold ctermfg=142 ctermbg=NONE guifg=#b8bb26 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi GruvboxYellow ctermfg=214 ctermbg=NONE guifg=#fabd2f guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxYellowBold ctermfg=214 ctermbg=NONE guifg=#fabd2f guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi GruvboxBlue ctermfg=109 ctermbg=NONE guifg=#83a598 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxBlueBold ctermfg=109 ctermbg=NONE guifg=#83a598 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi GruvboxPurple ctermfg=175 ctermbg=NONE guifg=#d3869b guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxPurpleBold ctermfg=175 ctermbg=NONE guifg=#d3869b guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi GruvboxAqua ctermfg=107 ctermbg=NONE guifg=#8ec07c guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxAquaBold ctermfg=107 ctermbg=NONE guifg=#8ec07c guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi GruvboxOrange ctermfg=208 ctermbg=NONE guifg=#fe8019 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi GruvboxOrangeBold ctermfg=208 ctermbg=NONE guifg=#fe8019 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
  hi GruvboxRedSign ctermfg=203 ctermbg=237 guifg=#fb4934 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi GruvboxGreenSign ctermfg=142 ctermbg=237 guifg=#b8bb26 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi GruvboxYellowSign ctermfg=214 ctermbg=237 guifg=#fabd2f guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi GruvboxBlueSign ctermfg=109 ctermbg=237 guifg=#83a598 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi GruvboxPurpleSign ctermfg=175 ctermbg=237 guifg=#d3869b guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi GruvboxAquaSign ctermfg=107 ctermbg=237 guifg=#8ec07c guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
  hi! link iCursor Cursor
  hi! link vCursor Cursor
  hi NormalMode ctermfg=137 ctermbg=236 guifg=#a89984 guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi InsertMode ctermfg=109 ctermbg=236 guifg=#83a598 guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi ReplaceMode ctermfg=107 ctermbg=236 guifg=#8ec07c guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi VisualMode ctermfg=208 ctermbg=236 guifg=#fe8019 guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi CommandMode ctermfg=175 ctermbg=236 guifg=#d3869b guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  hi Warnings ctermfg=208 ctermbg=236 guifg=#fe8019 guibg=#32302f guisp=NONE cterm=NONE,reverse gui=NONE,reverse
  if has('nvim')
    let g:terminal_color_0  = '#32302f'
    let g:terminal_color_8  = '#928374'
    let g:terminal_color_1  = '#cc241d'
    let g:terminal_color_9  = '#fb4934'
    let g:terminal_color_2  = '#98971a'
    let g:terminal_color_10 = '#b8bb26'
    let g:terminal_color_3  = '#d79921'
    let g:terminal_color_11 = '#fabd2f'
    let g:terminal_color_4  = '#458588'
    let g:terminal_color_12 = '#83a598'
    let g:terminal_color_5  = '#b16286'
    let g:terminal_color_13 = '#d3869b'
    let g:terminal_color_6  = '#689d6a'
    let g:terminal_color_14 = '#8ec07c'
    let g:terminal_color_7  = '#a89984'
    let g:terminal_color_15 = '#ebdbb2'
  endif
  hi! link TermCursor Cursor
  hi TermCursorNC ctermfg=237 ctermbg=187 guifg=#3c3836 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
  finish
endif

" Color similarity table (light background)
"           fg0: GUI=#282828/rgb( 40, 40, 40)  Term=235 #262626/rgb( 38, 38, 38)  [delta=0.631758]
" neutralpurple: GUI=#b16286/rgb(177, 98,134)  Term=132 #af5f87/rgb(175, 95,135)  [delta=1.447558]
" neutralorange: GUI=#d65d0e/rgb(214, 93, 14)  Term=166 #d75f00/rgb(215, 95,  0)  [delta=1.594261]
"           fg1: GUI=#3c3836/rgb( 60, 56, 54)  Term=237 #3a3a3a/rgb( 58, 58, 58)  [delta=2.591691]
"    neutralred: GUI=#cc241d/rgb(204, 36, 29)  Term=160 #d70000/rgb(215,  0,  0)  [delta=3.678548]
"           red: GUI=#9d0006/rgb(157,  0,  6)  Term=124 #af0000/rgb(175,  0,  0)  [delta=3.945287]
"   neutralblue: GUI=#458588/rgb( 69,133,136)  Term= 66 #5f8787/rgb( 95,135,135)  [delta=4.654950]
"  neutralgreen: GUI=#98971a/rgb(152,151, 26)  Term=100 #878700/rgb(135,135,  0)  [delta=5.597892]
"           bg0: GUI=#f2e5bc/rgb(242,229,188)  Term=187 #d7d7af/rgb(215,215,175)  [delta=6.152606]
"           fg3: GUI=#665c54/rgb(102, 92, 84)  Term= 59 #5f5f5f/rgb( 95, 95, 95)  [delta=6.186264]
" neutralyellow: GUI=#d79921/rgb(215,153, 33)  Term=172 #d78700/rgb(215,135,  0)  [delta=6.285960]
"          aqua: GUI=#427b58/rgb( 66,123, 88)  Term= 29 #00875f/rgb(  0,135, 95)  [delta=6.512362]
"   neutralaqua: GUI=#689d6a/rgb(104,157,106)  Term= 71 #5faf5f/rgb( 95,175, 95)  [delta=7.301224]
"         green: GUI=#79740e/rgb(121,116, 14)  Term=100 #878700/rgb(135,135,  0)  [delta=7.387225]
"           fg4: GUI=#7c6f64/rgb(124,111,100)  Term=243 #767676/rgb(118,118,118)  [delta=7.889685]
"        yellow: GUI=#b57614/rgb(181,118, 20)  Term=172 #d78700/rgb(215,135,  0)  [delta=8.074928]
"        orange: GUI=#af3a03/rgb(175, 58,  3)  Term=124 #af0000/rgb(175,  0,  0)  [delta=8.117734]
"           bg3: GUI=#bdae93/rgb(189,174,147)  Term=144 #afaf87/rgb(175,175,135)  [delta=8.449971]
"        purple: GUI=#8f3f71/rgb(143, 63,113)  Term=126 #af0087/rgb(175,  0,135)  [delta=8.757905]
"          grey: GUI=#928374/rgb(146,131,116)  Term=102 #878787/rgb(135,135,135)  [delta=8.970802]
"           bg2: GUI=#d5c4a1/rgb(213,196,161)  Term=180 #d7af87/rgb(215,175,135)  [delta=9.020393]
"          blue: GUI=#076678/rgb(  7,102,120)  Term= 23 #005f5f/rgb(  0, 95, 95)  [delta=9.442168]
"           bg4: GUI=#a89984/rgb(168,153,132)  Term=137 #af875f/rgb(175,135, 95)  [delta=10.269702]
"           fg2: GUI=#503836/rgb( 80, 56, 54)  Term=237 #3a3a3a/rgb( 58, 58, 58)  [delta=12.071597]
"           bg1: GUI=#ebdbb2/rgb(235,219,178)  Term=251 #c6c6c6/rgb(198,198,198)  [delta=15.718977]
if !has('gui_running') && get(g:, 'gruvbox_transp_bg', 0)
  hi Normal ctermfg=237 ctermbg=NONE guifg=#3c3836 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi CursorLineNr ctermfg=172 ctermbg=NONE guifg=#b57614 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi FoldColumn ctermfg=102 ctermbg=NONE guifg=#928374 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi SignColumn ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi VertSplit ctermfg=144 ctermbg=NONE guifg=#bdae93 guibg=NONE guisp=NONE cterm=NONE gui=NONE
else
  hi Normal ctermfg=237 ctermbg=187 guifg=#3c3836 guibg=#f2e5bc guisp=NONE cterm=NONE gui=NONE
  hi CursorLineNr ctermfg=172 ctermbg=251 guifg=#b57614 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
  hi FoldColumn ctermfg=102 ctermbg=251 guifg=#928374 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
  hi SignColumn ctermfg=NONE ctermbg=251 guifg=NONE guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
  hi VertSplit ctermfg=144 ctermbg=187 guifg=#bdae93 guibg=#f2e5bc guisp=NONE cterm=NONE gui=NONE
endif
hi ColorColumn ctermfg=NONE ctermbg=251 guifg=NONE guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi Conceal ctermfg=23 ctermbg=NONE guifg=#076678 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Cursor ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi! link CursorColumn CursorLine
hi CursorLine ctermfg=NONE ctermbg=251 guifg=NONE guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi DiffAdd ctermfg=100 ctermbg=187 guifg=#79740e guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi DiffChange ctermfg=29 ctermbg=187 guifg=#427b58 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi DiffDelete ctermfg=124 ctermbg=187 guifg=#9d0006 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi DiffText ctermfg=172 ctermbg=187 guifg=#b57614 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi Directory ctermfg=100 ctermbg=NONE guifg=#79740e guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi EndOfBuffer ctermfg=187 ctermbg=NONE guifg=#f2e5bc guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Error ctermfg=124 ctermbg=187 guifg=#9d0006 guibg=#f2e5bc guisp=NONE cterm=NONE,bold,reverse gui=NONE,bold,reverse
hi ErrorMsg ctermfg=187 ctermbg=124 guifg=#f2e5bc guibg=#9d0006 guisp=NONE cterm=NONE,bold gui=NONE,bold
hi Folded ctermfg=102 ctermbg=251 guifg=#928374 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE,italic
hi IncSearch ctermfg=124 ctermbg=187 guifg=#af3a03 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi LineNr ctermfg=137 ctermbg=NONE guifg=#a89984 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi MatchParen ctermfg=NONE ctermbg=144 guifg=NONE guibg=#bdae93 guisp=NONE cterm=NONE,bold gui=NONE,bold
hi ModeMsg ctermfg=172 ctermbg=NONE guifg=#b57614 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi MoreMsg ctermfg=172 ctermbg=NONE guifg=#b57614 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi NonText ctermfg=180 ctermbg=NONE guifg=#d5c4a1 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Pmenu ctermfg=237 ctermbg=180 guifg=#3c3836 guibg=#d5c4a1 guisp=NONE cterm=NONE gui=NONE
hi PmenuSbar ctermfg=NONE ctermbg=180 guifg=NONE guibg=#d5c4a1 guisp=NONE cterm=NONE gui=NONE
hi PmenuSel ctermfg=180 ctermbg=23 guifg=#d5c4a1 guibg=#076678 guisp=NONE cterm=NONE,bold gui=NONE,bold
hi PmenuThumb ctermfg=NONE ctermbg=137 guifg=NONE guibg=#a89984 guisp=NONE cterm=NONE gui=NONE
hi Question ctermfg=124 ctermbg=NONE guifg=#af3a03 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi! link QuickFixLine Search
hi Search ctermfg=172 ctermbg=187 guifg=#b57614 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi SpecialKey ctermfg=180 ctermbg=NONE guifg=#d5c4a1 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi SpellBad ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=#076678 cterm=NONE,undercurl gui=NONE,undercurl
if get(g:, "gruvbox_improved_warnings", 0)
  hi SpellCap ctermfg=100 ctermbg=NONE guifg=#79740e guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold,italic
else
  hi SpellCap ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=#9d0006 cterm=NONE,undercurl gui=NONE,undercurl
endif
hi SpellLocal ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=#427b58 cterm=NONE,undercurl gui=NONE,undercurl
hi SpellRare ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=#8f3f71 cterm=NONE,undercurl gui=NONE,undercurl
hi StatusLine ctermfg=180 ctermbg=237 guifg=#d5c4a1 guibg=#3c3836 guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi StatusLineNC ctermfg=251 ctermbg=243 guifg=#ebdbb2 guibg=#7c6f64 guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi! link StatusLineTerm StatusLine
hi! link StatusLineTermNC StatusLineNC
hi! link TabLine TabLineFill
hi TabLineFill ctermfg=137 ctermbg=251 guifg=#a89984 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi TabLineSel ctermfg=100 ctermbg=251 guifg=#79740e guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi Title ctermfg=100 ctermbg=NONE guifg=#79740e guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
if get(g:, "gruvbox_invert_selection", 0)
  hi Visual ctermfg=NONE ctermbg=144 guifg=NONE guibg=#bdae93 guisp=NONE cterm=NONE,reverse gui=NONE,reverse
else
  hi Visual ctermfg=NONE ctermbg=144 guifg=NONE guibg=#bdae93 guisp=NONE cterm=NONE gui=NONE
endif
hi! link VisualNOS Visual
hi WarningMsg ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi WildMenu ctermfg=23 ctermbg=180 guifg=#076678 guibg=#d5c4a1 guisp=NONE cterm=NONE,bold gui=NONE,bold
hi Boolean ctermfg=126 ctermbg=NONE guifg=#8f3f71 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Character ctermfg=126 ctermbg=NONE guifg=#8f3f71 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Comment ctermfg=102 ctermbg=NONE guifg=#928374 guibg=NONE guisp=NONE cterm=NONE gui=NONE,italic
hi Conditional ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Constant ctermfg=126 ctermbg=NONE guifg=#8f3f71 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Define ctermfg=29 ctermbg=NONE guifg=#427b58 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Debug ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Delimiter ctermfg=124 ctermbg=NONE guifg=#af3a03 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Error ctermfg=124 ctermbg=187 guifg=#9d0006 guibg=#f2e5bc guisp=NONE cterm=NONE,bold,reverse gui=NONE,bold,reverse
hi Exception ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Float ctermfg=126 ctermbg=NONE guifg=#8f3f71 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Function ctermfg=100 ctermbg=NONE guifg=#79740e guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi Identifier ctermfg=23 ctermbg=NONE guifg=#076678 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Ignore ctermfg=fg ctermbg=NONE guifg=fg guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Include ctermfg=29 ctermbg=NONE guifg=#427b58 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Keyword ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Label ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Macro ctermfg=29 ctermbg=NONE guifg=#427b58 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Number ctermfg=126 ctermbg=NONE guifg=#8f3f71 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi! link Operator Normal
hi PreCondit ctermfg=29 ctermbg=NONE guifg=#427b58 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi PreProc ctermfg=29 ctermbg=NONE guifg=#427b58 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Repeat ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi SpecialChar ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi SpecialComment ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Statement ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi StorageClass ctermfg=124 ctermbg=NONE guifg=#af3a03 guibg=NONE guisp=NONE cterm=NONE gui=NONE
if get(g:, "gruvbox_improved_strings", 0)
  hi Special ctermfg=124 ctermbg=251 guifg=#af3a03 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
  hi String ctermfg=237 ctermbg=251 guifg=#3c3836 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
else
  hi Special ctermfg=124 ctermbg=NONE guifg=#af3a03 guibg=NONE guisp=NONE cterm=NONE gui=NONE
  hi String ctermfg=100 ctermbg=NONE guifg=#79740e guibg=NONE guisp=NONE cterm=NONE gui=NONE
endif
hi Structure ctermfg=29 ctermbg=NONE guifg=#427b58 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi! link Tag Special
hi Todo ctermfg=fg ctermbg=187 guifg=fg guibg=#f2e5bc guisp=NONE cterm=NONE,bold gui=NONE,bold,italic
hi Type ctermfg=172 ctermbg=NONE guifg=#b57614 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Typedef ctermfg=172 ctermbg=NONE guifg=#b57614 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi Underlined ctermfg=23 ctermbg=NONE guifg=#076678 guibg=NONE guisp=NONE cterm=NONE,underline gui=NONE,underline
hi! link lCursor Cursor
hi CursorIM ctermfg=NONE ctermbg=NONE guifg=NONE guibg=NONE guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi ToolbarLine ctermfg=NONE ctermbg=144 guifg=NONE guibg=#bdae93 guisp=NONE cterm=NONE gui=NONE
hi ToolbarButton ctermfg=235 ctermbg=144 guifg=#282828 guibg=#bdae93 guisp=NONE cterm=NONE,bold gui=NONE,bold
if get(g:, "gruvbox_italic", 0)
  hi Comment cterm=italic
  hi Folded cterm=italic
  hi SpellCap cterm=italic
  hi Todo cterm=italic
endif
if get(g:, "gruvbox_italicize_strings", 0)
  hi Special cterm=italic gui=italic
  hi String cterm=italic gui=italic
endif
hi GruvboxFg0 ctermfg=235 ctermbg=NONE guifg=#282828 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxFg1 ctermfg=237 ctermbg=NONE guifg=#3c3836 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxFg2 ctermfg=237 ctermbg=NONE guifg=#503836 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxFg3 ctermfg=59 ctermbg=NONE guifg=#665c54 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxFg4 ctermfg=243 ctermbg=NONE guifg=#7c6f64 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxGray ctermfg=102 ctermbg=NONE guifg=#928374 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxBg0 ctermfg=187 ctermbg=NONE guifg=#f2e5bc guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxBg1 ctermfg=251 ctermbg=NONE guifg=#ebdbb2 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxBg2 ctermfg=180 ctermbg=NONE guifg=#d5c4a1 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxBg3 ctermfg=144 ctermbg=NONE guifg=#bdae93 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxBg4 ctermfg=137 ctermbg=NONE guifg=#a89984 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxRed ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxRedBold ctermfg=124 ctermbg=NONE guifg=#9d0006 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi GruvboxGreen ctermfg=100 ctermbg=NONE guifg=#79740e guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxGreenBold ctermfg=100 ctermbg=NONE guifg=#79740e guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi GruvboxYellow ctermfg=172 ctermbg=NONE guifg=#b57614 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxYellowBold ctermfg=172 ctermbg=NONE guifg=#b57614 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi GruvboxBlue ctermfg=23 ctermbg=NONE guifg=#076678 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxBlueBold ctermfg=23 ctermbg=NONE guifg=#076678 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi GruvboxPurple ctermfg=126 ctermbg=NONE guifg=#8f3f71 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxPurpleBold ctermfg=126 ctermbg=NONE guifg=#8f3f71 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi GruvboxAqua ctermfg=29 ctermbg=NONE guifg=#427b58 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxAquaBold ctermfg=29 ctermbg=NONE guifg=#427b58 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi GruvboxOrange ctermfg=124 ctermbg=NONE guifg=#af3a03 guibg=NONE guisp=NONE cterm=NONE gui=NONE
hi GruvboxOrangeBold ctermfg=124 ctermbg=NONE guifg=#af3a03 guibg=NONE guisp=NONE cterm=NONE,bold gui=NONE,bold
hi GruvboxRedSign ctermfg=124 ctermbg=251 guifg=#9d0006 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi GruvboxGreenSign ctermfg=100 ctermbg=251 guifg=#79740e guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi GruvboxYellowSign ctermfg=172 ctermbg=251 guifg=#b57614 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi GruvboxBlueSign ctermfg=23 ctermbg=251 guifg=#076678 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi GruvboxPurpleSign ctermfg=126 ctermbg=251 guifg=#8f3f71 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi GruvboxAquaSign ctermfg=29 ctermbg=251 guifg=#427b58 guibg=#ebdbb2 guisp=NONE cterm=NONE gui=NONE
hi! link iCursor Cursor
hi! link vCursor Cursor
hi NormalMode ctermfg=243 ctermbg=187 guifg=#7c6f64 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi InsertMode ctermfg=23 ctermbg=187 guifg=#076678 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi ReplaceMode ctermfg=29 ctermbg=187 guifg=#427b58 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi VisualMode ctermfg=124 ctermbg=187 guifg=#af3a03 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi CommandMode ctermfg=126 ctermbg=187 guifg=#8f3f71 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
hi Warnings ctermfg=124 ctermbg=187 guifg=#af3a03 guibg=#f2e5bc guisp=NONE cterm=NONE,reverse gui=NONE,reverse
if has('nvim')
  let g:terminal_color_0  = '#f2e5bc'
  let g:terminal_color_8  = '#928374'
  let g:terminal_color_1  = '#cc241d'
  let g:terminal_color_9  = '#9d0006'
  let g:terminal_color_2  = '#98971a'
  let g:terminal_color_10 = '#79740e'
  let g:terminal_color_3  = '#d79921'
  let g:terminal_color_11 = '#b57614'
  let g:terminal_color_4  = '#458588'
  let g:terminal_color_12 = '#076678'
  let g:terminal_color_5  = '#b16286'
  let g:terminal_color_13 = '#8f3f71'
  let g:terminal_color_6  = '#689d6a'
  let g:terminal_color_14 = '#427b58'
  let g:terminal_color_7  = '#7c6f64'
  let g:terminal_color_15 = '#3c3836'
endif
hi! link TermCursor Cursor
hi TermCursorNC ctermfg=251 ctermbg=237 guifg=#ebdbb2 guibg=#3c3836 guisp=NONE cterm=NONE gui=NONE
finish

" Color: neutralred        rgb(204,  36,  29)    ~
" Color: neutralgreen      rgb(152, 151,  26)    ~
" Color: neutralyellow     rgb(215, 153,  33)    ~
" Color: neutralblue       rgb( 69, 133, 136)    ~
" Color: neutralpurple     rgb(177,  98, 134)    ~
" Color: neutralaqua       rgb(104, 157, 106)    ~
" Color: neutralorange     rgb(214,  93,  14)    ~
" Background: dark
" Color: bg0 rgb(50,48,47) ~
" Color: bg1             rgb(60,  56,  54)     ~
" Color: bg2             rgb(80,  73,  69)     ~
" Color: bg3             rgb(102, 92,  84)     ~
" Color: bg4             rgb(124, 111, 100)    ~
" Color: fg0             rgb(253, 244, 193)    ~
" Color: fg1             rgb(235, 219, 178)    ~
" Color: fg2             rgb(213, 196, 161)    ~
" Color: fg3             rgb(189, 174, 147)    ~
" Color: fg4             rgb(168, 153, 132)    ~
" Color: grey            rgb(146, 131, 116)    ~
" Color: red             rgb(251, 73,  52)     203
" Color: green           rgb(184, 187, 38)     ~
" Color: yellow          rgb(250, 189, 47)     ~
" Color: blue            rgb(131, 165, 152)    ~
" Color: purple          rgb(211, 134, 155)    ~
" Color: aqua            rgb(142, 192, 124)    ~
" Color: orange          rgb(254, 128, 25)     ~
"     Normal       fg1    none
"     CursorLineNr yellow none
"     FoldColumn   grey   none
"     SignColumn   none   none
"     VertSplit    bg3    none
"     Normal       fg1    bg0
"     CursorLineNr yellow bg1
"     FoldColumn   grey   bg1
"     SignColumn   none   bg1
"     VertSplit    bg3    bg0
" ColorColumn                      none   bg1
" Conceal                          blue   none
" Cursor                           none   none    reverse
" CursorColumn                  -> CursorLine
" CursorLine                       none   bg1
" DiffAdd                          green  bg0     reverse
" DiffChange                       aqua   bg0     reverse
" DiffDelete                       red    bg0     reverse
" DiffText                         yellow bg0     reverse
" Directory                        green  none    bold
" EndOfBuffer                      bg0    none
" Error                            red    bg0     bold,reverse
" ErrorMsg                         bg0    red     bold
" Folded                           grey   bg1     g=italic
" IncSearch                        orange bg0     reverse
" LineNr                           bg4    none
" MatchParen                       none   bg3     bold
" ModeMsg                          yellow none    bold
" MoreMsg                          yellow none    bold
" NonText                          bg2    none
" Pmenu                            fg1    bg2
" PmenuSbar                        none   bg2
" PmenuSel                         bg2    blue    bold
" PmenuThumb                       none   bg4
" Question                         orange none    bold
" QuickFixLine                  -> Search
" Search                           yellow bg0     reverse
" SpecialKey                       bg2    none
" SpellBad                         none   none    undercurl s=blue
" SpellCap                         green  none    t=bold g=bold,italic
" SpellCap                         none   none    undercurl s=red
" SpellLocal                       none   none    undercurl s=aqua
" SpellRare                        none   none    undercurl s=purple
" StatusLine                       bg2    fg1     reverse
" StatusLineNC                     bg1    fg4     reverse
" StatusLineTerm                -> StatusLine
" StatusLineTermNC              -> StatusLineNC
" TabLine                       -> TabLineFill
" TabLineFill                      bg4    bg1
" TabLineSel                       green  bg1
" Title                            green  none    bold
" Visual                           none   bg3     reverse
" Visual                           none   bg3
" VisualNOS                     -> Visual
" WarningMsg                       red    none    bold
" WildMenu                         blue   bg2     bold
" Boolean                          purple none
" Character                        purple none
" Comment                          grey   none    g=italic
" Conditional                      red    none
" Constant                         purple none
" Define                           aqua   none
" Debug                            red    none
" Delimiter                        orange none
" Error                            red    bg0     bold,reverse
" Exception                        red    none
" Float                            purple none
" Function                         green  none    bold
" Identifier                       blue   none
" Ignore                           fg     none
" Include                          aqua   none
" Keyword                          red    none
" Label                            red    none
" Macro                            aqua   none
" Number                           purple none
" Operator                      -> Normal
" PreCondit                        aqua   none
" PreProc                          aqua   none
" Repeat                           red    none
" SpecialChar                      red    none
" SpecialComment                   red    none
" Statement                        red    none
" StorageClass                     orange none
" Special                          orange bg1
" String                           fg1    bg1
" Special                          orange none
" String                           green  none
" Structure                        aqua   none
" Tag                           -> Special
" Todo                             fg     bg0     t=bold g=bold,italic
" Type                             yellow none
" Typedef                          yellow none
" Underlined                       blue   none    underline
" lCursor                       -> Cursor
" CursorIM                         none   none    reverse
" ToolbarLine          none              bg3
" ToolbarButton        fg0               bg3               bold
" GruvboxFg0        fg0    none
" GruvboxFg1        fg1    none
" GruvboxFg2        fg2    none
" GruvboxFg3        fg3    none
" GruvboxFg4        fg4    none
" GruvboxGray       grey   none
" GruvboxBg0        bg0    none
" GruvboxBg1        bg1    none
" GruvboxBg2        bg2    none
" GruvboxBg3        bg3    none
" GruvboxBg4        bg4    none
" GruvboxRed        red    none
" GruvboxRedBold    red    none bold
" GruvboxGreen      green  none
" GruvboxGreenBold  green  none bold
" GruvboxYellow     yellow none
" GruvboxYellowBold yellow none bold
" GruvboxBlue       blue   none
" GruvboxBlueBold   blue   none bold
" GruvboxPurple     purple none
" GruvboxPurpleBold purple none bold
" GruvboxAqua       aqua   none
" GruvboxAquaBold   aqua   none bold
" GruvboxOrange     orange none
" GruvboxOrangeBold orange none bold
" GruvboxRedSign    red    bg1
" GruvboxGreenSign  green  bg1
" GruvboxYellowSign yellow bg1
" GruvboxBlueSign   blue   bg1
" GruvboxPurpleSign purple bg1
" GruvboxAquaSign   aqua   bg1
" iCursor                       -> Cursor
" vCursor                       -> Cursor
" NormalMode                       fg4    bg0     reverse
" InsertMode                       blue   bg0     reverse
" ReplaceMode                      aqua   bg0     reverse
" VisualMode                       orange bg0     reverse
" CommandMode                      purple bg0     reverse
" Warnings                         orange bg0     reverse
" TermCursor        -> Cursor
" TermCursorNC         bg1            fg1
" Background: light
" Color: bg0 rgb(242,229,188)    ~
" Color: bg1 rgb(235, 219, 178) 251
" Color: bg2 rgb(213, 196, 161) 180
" Color: bg3               rgb(189, 174, 147)    ~
" Color: bg4               rgb(168, 153, 132)    ~
" Color: fg0               rgb( 40,  40,  40)    ~
" Color: fg1               rgb( 60,  56,  54)    ~
" Color: fg2               rgb( 80,  56,  54)    ~
" Color: fg3               rgb(102,  92,  84)    ~
" Color: fg4               rgb(124, 111, 100)    ~
" Color: grey              rgb(146, 131, 116)    ~
" Color: red               rgb(157,   0,   6)    ~
" Color: green             rgb(121, 116,  14)    ~
" Color: yellow            rgb(181, 118,  20)    ~
" Color: blue              rgb(  7, 102, 120)    ~
" Color: purple            rgb(143,  63, 113)    ~
" Color: aqua              rgb( 66, 123,  88)    ~
" Color: orange            rgb(175,  58,   3)    ~
"     Normal       fg1    none
"     CursorLineNr yellow none
"     FoldColumn   grey   none
"     SignColumn   none   none
"     VertSplit    bg3    none
"     Normal       fg1    bg0
"     CursorLineNr yellow bg1
"     FoldColumn   grey   bg1
"     SignColumn   none   bg1
"     VertSplit    bg3    bg0
" ColorColumn                      none   bg1
" Conceal                          blue   none
" Cursor                           none   none    reverse
" CursorColumn                  -> CursorLine
" CursorLine                       none   bg1
" DiffAdd                          green  bg0     reverse
" DiffChange                       aqua   bg0     reverse
" DiffDelete                       red    bg0     reverse
" DiffText                         yellow bg0     reverse
" Directory                        green  none    bold
" EndOfBuffer                      bg0    none
" Error                            red    bg0     bold,reverse
" ErrorMsg                         bg0    red     bold
" Folded                           grey   bg1     g=italic
" IncSearch                        orange bg0     reverse
" LineNr                           bg4    none
" MatchParen                       none   bg3     bold
" ModeMsg                          yellow none    bold
" MoreMsg                          yellow none    bold
" NonText                          bg2    none
" Pmenu                            fg1    bg2
" PmenuSbar                        none   bg2
" PmenuSel                         bg2    blue    bold
" PmenuThumb                       none   bg4
" Question                         orange none    bold
" QuickFixLine                  -> Search
" Search                           yellow bg0     reverse
" SpecialKey                       bg2    none
" SpellBad                         none   none    undercurl s=blue
" SpellCap                         green  none    t=bold g=bold,italic
" SpellCap                         none   none    undercurl s=red
" SpellLocal                       none   none    undercurl s=aqua
" SpellRare                        none   none    undercurl s=purple
" StatusLine                       bg2    fg1     reverse
" StatusLineNC                     bg1    fg4     reverse
" StatusLineTerm                -> StatusLine
" StatusLineTermNC              -> StatusLineNC
" TabLine                       -> TabLineFill
" TabLineFill                      bg4    bg1
" TabLineSel                       green  bg1
" Title                            green  none    bold
" Visual                           none   bg3     reverse
" Visual                           none   bg3
" VisualNOS                     -> Visual
" WarningMsg                       red    none    bold
" WildMenu                         blue   bg2     bold
" Boolean                          purple none
" Character                        purple none
" Comment                          grey   none    g=italic
" Conditional                      red    none
" Constant                         purple none
" Define                           aqua   none
" Debug                            red    none
" Delimiter                        orange none
" Error                            red    bg0     bold,reverse
" Exception                        red    none
" Float                            purple none
" Function                         green  none    bold
" Identifier                       blue   none
" Ignore                           fg     none
" Include                          aqua   none
" Keyword                          red    none
" Label                            red    none
" Macro                            aqua   none
" Number                           purple none
" Operator                      -> Normal
" PreCondit                        aqua   none
" PreProc                          aqua   none
" Repeat                           red    none
" SpecialChar                      red    none
" SpecialComment                   red    none
" Statement                        red    none
" StorageClass                     orange none
" Special                          orange bg1
" String                           fg1    bg1
" Special                          orange none
" String                           green  none
" Structure                        aqua   none
" Tag                           -> Special
" Todo                             fg     bg0     t=bold g=bold,italic
" Type                             yellow none
" Typedef                          yellow none
" Underlined                       blue   none    underline
" lCursor                       -> Cursor
" CursorIM                         none   none    reverse
" ToolbarLine          none              bg3
" ToolbarButton        fg0               bg3               bold
" GruvboxFg0        fg0    none
" GruvboxFg1        fg1    none
" GruvboxFg2        fg2    none
" GruvboxFg3        fg3    none
" GruvboxFg4        fg4    none
" GruvboxGray       grey   none
" GruvboxBg0        bg0    none
" GruvboxBg1        bg1    none
" GruvboxBg2        bg2    none
" GruvboxBg3        bg3    none
" GruvboxBg4        bg4    none
" GruvboxRed        red    none
" GruvboxRedBold    red    none bold
" GruvboxGreen      green  none
" GruvboxGreenBold  green  none bold
" GruvboxYellow     yellow none
" GruvboxYellowBold yellow none bold
" GruvboxBlue       blue   none
" GruvboxBlueBold   blue   none bold
" GruvboxPurple     purple none
" GruvboxPurpleBold purple none bold
" GruvboxAqua       aqua   none
" GruvboxAquaBold   aqua   none bold
" GruvboxOrange     orange none
" GruvboxOrangeBold orange none bold
" GruvboxRedSign    red    bg1
" GruvboxGreenSign  green  bg1
" GruvboxYellowSign yellow bg1
" GruvboxBlueSign   blue   bg1
" GruvboxPurpleSign purple bg1
" GruvboxAquaSign   aqua   bg1
" iCursor                       -> Cursor
" vCursor                       -> Cursor
" NormalMode                       fg4    bg0     reverse
" InsertMode                       blue   bg0     reverse
" ReplaceMode                      aqua   bg0     reverse
" VisualMode                       orange bg0     reverse
" CommandMode                      purple bg0     reverse
" Warnings                         orange bg0     reverse
" TermCursor        -> Cursor
" TermCursorNC         bg1            fg1
