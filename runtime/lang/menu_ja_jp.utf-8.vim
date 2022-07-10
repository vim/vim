" Menu Translations:	  Japanese
" Maintainer:           Ada (Haowen) Yu <me@yuhaowen.com>
" Previous Maintainer:  MURAOKA Taro  <koron.kaoriya@gmail.com>
" Last Change:          2022 July 10
" Original translations
"
" Generated with the scripts from:
"
"       https://github.com/adaext/vim-menutrans-helper

" Quit when menu translations have already been done.

if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1
let s:keepcpo = &cpo
set cpo&vim

scriptencoding utf-8

" Help menu
menutrans &Help ヘルプ(&H)
" Help menuitems and dialog {{{1
menutrans &Overview<Tab><F1> 概略(&O)<Tab><F1>
menutrans &User\ Manual ユーザーマニュアル(&U)
menutrans &How-to\ Links &How-toリンク
menutrans &Find\.\.\. 検索(&F)\.\.\.
menutrans &Credits クレジット(&C)
menutrans Co&pying 著作権情報(&P)
menutrans &Sponsor/Register スポンサー/登録(&S)
menutrans O&rphans 孤児(&R)
menutrans &Version バージョン情報(&V)
menutrans &About Vimについて(&A)

" fun! s:Helpfind()
if !exists("g:menutrans_help_dialog")
  let g:menutrans_help_dialog = "ヘルプを検索したいコマンドもしくは単語を入力してください:\n\n挿入モードのコマンドには i_ を先頭に付加します. (例: i_CTRL-X)\nコマンドライン編集コマンドには c_ を先頭に付加します. (例: c_<Del>)\nオプションの名前には ' を付加します. (例: 'shiftwidth')"
endif
" }}}

" File menu
menutrans &File ファイル(&F)
" File menuitems {{{1
menutrans &Open\.\.\.<Tab>:e 開く(&O)\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp 分割して開く(&L)\.\.\.<Tab>:sp
menutrans Open\ Tab\.\.\.<Tab>:tabnew タブページで開く<Tab>:tabnew
menutrans &New<Tab>:enew 新規作成(&N)<Tab>:enew
menutrans &Close<Tab>:close 閉じる(&C)<Tab>:close
menutrans &Save<Tab>:w 保存(&S)<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav 名前を付けて保存(&A)\.\.\.<Tab>:sav
menutrans Split\ &Diff\ With\.\.\. 差分表示(&D)\.\.\.
menutrans Split\ Patched\ &By\.\.\. パッチ結果を表示(&B)\.\.\.
menutrans &Print 印刷(&P)
menutrans Sa&ve-Exit<Tab>:wqa 保存して終了(&V)<Tab>:wqa
menutrans E&xit<Tab>:qa 終了(&X)<Tab>:qa
" }}}

" Edit menu
menutrans &Edit 編集(&E)
" Edit menuitems {{{1
menutrans &Undo<Tab>u 取り消す(&U)<Tab>u
menutrans &Redo<Tab>^R もう一度やる(&R)<Tab>^R
menutrans Rep&eat<Tab>\. 繰り返す(&E)<Tab>\.
menutrans Cu&t<Tab>"+x 切り取り(&T)<Tab>"+x
menutrans &Copy<Tab>"+y コピー(&C)<Tab>"+y
menutrans &Paste<Tab>"+gP 貼り付け(&P)<Tab>"+gP
menutrans Put\ &Before<Tab>[p 前に貼る(&B)<Tab>[p
menutrans Put\ &After<Tab>]p 後に貼る(&A)<Tab>]p
menutrans &Delete<Tab>x 消す(&D)<Tab>x
menutrans &Select\ All<Tab>ggVG 全て選択(&S)<Tab>ggVG
menutrans &Find\.\.\. 検索(&F)\.\.\.
menutrans Find\ and\ Rep&lace\.\.\. 置換(&L)\.\.\.
menutrans &Find<Tab>/ 検索(&F)<Tab>/
menutrans Find\ and\ Rep&lace<Tab>:%s 置換(&L)<Tab>:%s
menutrans Find\ and\ Rep&lace<Tab>:s 置換(&L)<Tab>:s
menutrans Settings\ &Window 設定ウィンドウ(&W)
menutrans Startup\ &Settings 起動時の設定(&S)

" Edit/Global Settings
menutrans &Global\ Settings 全体設定(&G)
" Edit.Global Settings menuitems and dialogs {{{2
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls! 
menutrans Toggle\ &Ignoring\ Case<Tab>:set\ ic! 
menutrans Toggle\ &Showing\ Matched\ Pairs<Tab>:set\ sm! 
menutrans &Context\ Lines カーソル周辺行数(&C)
menutrans &Virtual\ Edit 仮想編集(&V)
" Edit.Global Settings.Virtual Edit menuitems {{{3
menutrans Never 無効
menutrans Block\ Selection ブロック選択時
menutrans Insert\ Mode 挿入モード時
menutrans Block\ and\ Insert ブロック/挿入モード時
menutrans Always 常時
" }}}
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im! 
menutrans Toggle\ Vi\ C&ompatibility<Tab>:set\ cp! 
menutrans Search\ &Path\.\.\. 検索パス(&P)\.\.\.
menutrans Ta&g\ Files\.\.\. タグファイル(&G)\.\.\.

" GUI options
menutrans Toggle\ &Toolbar ツールバー表示切替(&T)
menutrans Toggle\ &Bottom\ Scrollbar スクロールバー(下)表示切替(&B)
menutrans Toggle\ &Left\ Scrollbar スクロールバー(左)表示切替(&L)
menutrans Toggle\ &Right\ Scrollbar スクロールバー(右)表示切替(&R)

" fun! s:SearchP()
if !exists("g:menutrans_path_dialog")
  let g:menutrans_path_dialog = "ファイルの検索パスを入力してください:\nディレクトリ名はカンマ ( , ) で区切ってください."
endif

" fun! s:TagFiles()
if !exists("g:menutrans_tags_dialog")
  let g:menutrans_tags_dialog = "タグファイルの名前を入力してください:\n名前はカンマ ( , ) で区切ってください."
endif
" }}}

" Edit/File Settings
menutrans F&ile\ Settings ファイル設定(&I)
" Edit.File Settings menuitems and dialogs {{{2
" Boolean options
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu! 
menutrans Toggle\ Relati&ve\ Line\ Numbering<Tab>:set\ rnu! 
menutrans Toggle\ &List\ Mode<Tab>:set\ list! 
menutrans Toggle\ Line\ &Wrapping<Tab>:set\ wrap! 
menutrans Toggle\ W&rapping\ at\ Word<Tab>:set\ lbr! 
menutrans Toggle\ Tab\ &Expanding<Tab>:set\ et! 
menutrans Toggle\ &Auto\ Indenting<Tab>:set\ ai! 
menutrans Toggle\ &C-Style\ Indenting<Tab>:set\ cin! 

" other options
menutrans &Shiftwidth シフト幅(&S)
menutrans Soft\ &Tabstop ソフトウェアタブ幅(&T)
menutrans Te&xt\ Width\.\.\. テキスト幅(&X)\.\.\.
menutrans &File\ Format\.\.\. 改行記号選択(&F)\.\.\.

" fun! s:TextWidth()
if !exists("g:menutrans_textwidth_dialog")
  let g:menutrans_textwidth_dialog = "テキストの幅('textwidth')を設定してください (0で整形を無効化):"
endif

" fun! s:FileFormat()
if !exists("g:menutrans_fileformat_dialog")
  let g:menutrans_fileformat_dialog = "ファイル出力の際の改行記号の形式を選んでください."
endif
if !exists("g:menutrans_fileformat_choices")
  let g:menutrans_fileformat_choices = "&Unix\n&Dos\n&Mac\nキャンセル(&C)"
endif
" }}}
menutrans Show\ C&olor\ Schemes\ in\ Menu 色テーマをメニューに表示(&O)
menutrans C&olor\ Scheme 色テーマ選択(&O)
menutrans None なし
menutrans Show\ &Keymaps\ in\ Menu キーマップをメニューに表示(&K)
menutrans &Keymap キーマップ(&K)
menutrans Select\ Fo&nt\.\.\. フォント設定(&N)\.\.\.
" }}}

" Programming menu
menutrans &Tools ツール(&T)
" Tools menuitems {{{1
menutrans &Jump\ to\ This\ Tag<Tab>g^] タグジャンプ(&J)<Tab>g^]
menutrans Jump\ &Back<Tab>^T 戻る(&B)<Tab>^T
menutrans Build\ &Tags\ File タグファイル作成(&T)

" Tools.Spelling Menu
menutrans &Spelling スペリング(&S)
" Tools.Spelling menuitems and dialog {{{2
menutrans &Spell\ Check\ On スペルチェック有効(&S)
menutrans Spell\ Check\ &Off スペルチェック無効(&O)
menutrans To\ &Next\ Error<Tab>]s 次のエラー(&N)<Tab>]s
menutrans To\ &Previous\ Error<Tab>[s 前のエラー(&P)<Tab>[s
menutrans Suggest\ &Corrections<Tab>z= 修正候補(&C)<Tab>z=
menutrans &Repeat\ Correction<Tab>:spellrepall 修正を繰り返す(&R)<Tab>:spellrepall
menutrans Set\ Language\ to\ "en" 言語を\ "en"\ に設定する
menutrans Set\ Language\ to\ "en_au" 言語を\ "en_au"\ に設定する
menutrans Set\ Language\ to\ "en_ca" 言語を\ "en_ca"\ に設定する
menutrans Set\ Language\ to\ "en_gb" 言語を\ "en_gb"\ に設定する
menutrans Set\ Language\ to\ "en_nz" 言語を\ "en_nz"\ に設定する
menutrans Set\ Language\ to\ "en_us" 言語を\ "en_us"\ に設定する
menutrans &Find\ More\ Languages 他の言語を検索する(&F)

" func! s:SpellLang()
if !exists("g:menutrans_set_lang_to")
  " let g:menutrans_set_lang_to = "TRANSLATION MISSING"
endif
" }}}

" Tools.Fold Menu
menutrans &Folding 折畳み(&F)
" Tools.Fold menuitems {{{2
" open close folds
menutrans &Enable/Disable\ Folds<Tab>zi 有効/無効切替(&E)<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv カーソル行を表示(&V)<Tab>zv
menutrans Vie&w\ Cursor\ Line\ Only<Tab>zMzx カーソル行だけを表示(&W)<Tab>zMzx
menutrans C&lose\ More\ Folds<Tab>zm 折畳みを閉じる(&L)<Tab>zm
menutrans &Close\ All\ Folds<Tab>zM 全折畳みを閉じる(&C)<Tab>zM
menutrans O&pen\ More\ Folds<Tab>zr 折畳みを開く(&P)<Tab>zr
menutrans &Open\ All\ Folds<Tab>zR 全折畳みを開く(&O)<Tab>zR
" fold method
menutrans Fold\ Met&hod 折畳み方法(&H)
" Tools.Fold.Fold Method menuitems {{{3
menutrans M&anual 手動(&A)
menutrans I&ndent インデント(&N)
menutrans E&xpression 式評価(&X)
menutrans S&yntax シンタックス(&Y)
menutrans &Diff 差分(&D)
menutrans Ma&rker マーカー(&R)
" }}}
" create and delete folds
menutrans Create\ &Fold<Tab>zf 折畳み作成(&F)<Tab>zf
menutrans &Delete\ Fold<Tab>zd 折畳み削除(&D)<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD 全折畳み削除(&A)<Tab>zD
" moving around in folds
menutrans Fold\ Col&umn\ Width 折畳みカラム幅(&U)
" }}}

" Tools.Diff Menu
menutrans &Diff 差分(&D)
" Tools.Diff menuitems {{{2
menutrans &Update 更新(&U)
menutrans &Get\ Block ブロック抽出(&G)
menutrans &Put\ Block ブロック適用(&P)
" }}}

menutrans &Make<Tab>:make ビルド(&M)<Tab>:make
menutrans &List\ Errors<Tab>:cl エラーリスト(&L)<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl! メッセージリスト(&I)<Tab>:cl!
menutrans &Next\ Error<Tab>:cn 次のエラーへ(&N)<Tab>:cn
menutrans &Previous\ Error<Tab>:cp 前のエラーへ(&P)<Tab>:cp
menutrans &Older\ List<Tab>:cold 古いリスト(&O)<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew 新しいリスト(&E)<Tab>:cnew
menutrans Error\ &Window エラーウィンドウ(&W)
" Tools.Error Window menuitems {{{2
menutrans &Update<Tab>:cwin 更新(&U)<Tab>:cwin
menutrans &Open<Tab>:copen 開く(&O)<Tab>:copen
menutrans &Close<Tab>:cclose 閉じる(&C)<Tab>:cclose
" }}}
menutrans Show\ Compiler\ Se&ttings\ in\ Menu コンパイラ設定をメニューに表示(&T)
menutrans Se&t\ Compiler コンパイラ設定(&T)
menutrans &Convert\ to\ HEX<Tab>:%!xxd HEXへ変換(&C)<Tab>:%!xxd
menutrans Conve&rt\ Back<Tab>:%!xxd\ -r HEXから逆変換(&R)<Tab>%!xxd\ -r
" }}}

" Buffer menu
menutrans &Buffers バッファ(&B)
" menutrans Dummy TRANSLATION\ MISSING
" Buffer menuitems and dialog {{{1
menutrans &Refresh\ Menu メニュー再読込(&R)
menutrans &Delete 削除(&D)
menutrans &Alternate 裏へ切替(&A)
menutrans &Next 次のバッファ(&N)
menutrans &Previous 前のバッファ(&P)

" func! s:BMMunge(fname, bnum)
if !exists("g:menutrans_no_file")
  let g:menutrans_no_file = "[無名]"
endif
" }}}

" Window menu
menutrans &Window ウィンドウ(&W)
" Window menuitems {{{1
menutrans &New<Tab>^Wn 新規作成(&N)<Tab>^Wn
menutrans S&plit<Tab>^Ws 分割(&P)<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^ 裏バッファへ分割(&L)<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv 垂直分割(&V)<Tab>^Wv
menutrans Split\ File\ E&xplorer ファイルエクスプローラ(&X)
menutrans &Close<Tab>^Wc 閉じる(&C)<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo 他を閉じる(&O)<Tab>^Wo
menutrans Move\ &To 移動(&T)
menutrans &Top<Tab>^WK 上(&T)<Tab>^WK
menutrans &Bottom<Tab>^WJ 下(&B)<Tab>^WJ
menutrans &Left\ Side<Tab>^WH 左(&L)<Tab>^WH
menutrans &Right\ Side<Tab>^WL 右(&R)<Tab>^WL
menutrans Rotate\ &Up<Tab>^WR 上にローテーション(&U)<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr 下にローテーション(&D)<Tab>^Wr
menutrans &Equal\ Size<Tab>^W= 同じ高さに(&E)<Tab>^W=
menutrans &Max\ Height<Tab>^W_ 最大高に(&M)<Tab>^W_
menutrans M&in\ Height<Tab>^W1_ 最小高に(&i)<Tab>^W1_
menutrans Max\ &Width<Tab>^W\| 最大幅に(&W)<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\| 最小幅に(&H)<Tab>^W1\|
" }}}

" The popup menu {{{1
menutrans &Undo 取り消す(&U)
menutrans Cu&t 切り取り(&T)
menutrans &Copy コピー(&C)
menutrans &Paste 貼り付け(&P)
menutrans &Delete 削除(&D)
menutrans Select\ Blockwise 矩形ブロック選択
menutrans Select\ &Word 単語選択(&W)
menutrans Select\ &Sentence 文選択(&S)
menutrans Select\ Pa&ragraph 段落選択(&R)
menutrans Select\ &Line 行選択(&L)
menutrans Select\ &Block ブロック選択(&B)
menutrans Select\ &All すべて選択(&A)

" func! <SID>SpellPopup()
if !exists("g:menutrans_spell_change_ARG_to")
  let g:menutrans_spell_change_ARG_to = '"%s"\ を変更'
endif
if !exists("g:menutrans_spell_add_ARG_to_word_list")
  let g:menutrans_spell_add_ARG_to_word_list = '"%s"\ を単語リストに追加'
endif
if !exists("g:menutrans_spell_ignore_ARG")
  let g:menutrans_spell_ignore_ARG = '"%s"\ を無視'
endif
" }}}

" The GUI toolbar {{{1
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    let did_toolbar_tmenu = 1
    tmenu ToolBar.Open ファイルを開く
    tmenu ToolBar.Save 現在のファイルを保存
    tmenu ToolBar.SaveAll すべてのファイルを保存
    tmenu ToolBar.Print 印刷
    tmenu ToolBar.Undo 取り消し
    tmenu ToolBar.Redo もう一度やる
    tmenu ToolBar.Cut クリップボードへ切り取り
    tmenu ToolBar.Copy クリップボードへコピー
    tmenu ToolBar.Paste クリップボードから貼り付け
    if !has("gui_athena")
      tmenu ToolBar.Replace 検索 / 置換...
      tmenu ToolBar.FindNext 次を検索
      tmenu ToolBar.FindPrev 前を検索
    endif
    tmenu ToolBar.LoadSesn セッション読込
    tmenu ToolBar.SaveSesn セッション保存
    tmenu ToolBar.RunScript Vimスクリプト実行
    tmenu ToolBar.Make プロジェクトをMake
    tmenu ToolBar.RunCtags tags作成
    tmenu ToolBar.TagJump タグジャンプ
    tmenu ToolBar.Help Vimヘルプ
    tmenu ToolBar.FindHelp Vimヘルプ検索
  endfun
endif
" }}}

" Syntax menu
menutrans &Syntax シンタックス(&S)
" Syntax menuitems {{{1
menutrans &Show\ File\ Types\ in\ Menu 対応形式をメニューに表示(&S)
menutrans &Off 無効化(&O)
menutrans &Manual 手動設定(&M)
menutrans A&utomatic 自動設定(&U)
menutrans On/Off\ for\ &This\ File 
menutrans Co&lor\ Test カラーテスト(&L)
menutrans &Highlight\ Test ハイライトテスト(&H)
menutrans &Convert\ to\ HTML HTMLへコンバート(&C)

" From synmenu.vim
menutrans Set\ '&syntax'\ Only 'syntax'だけ設定(&S)
menutrans Set\ '&filetype'\ Too 'filetype'も設定(&F)
" menutrans AB TRANSLATION\ MISSING
" menutrans A2ps\ config TRANSLATION\ MISSING
" menutrans Aap TRANSLATION\ MISSING
" menutrans ABAP/4 TRANSLATION\ MISSING
" menutrans Abaqus TRANSLATION\ MISSING
" menutrans ABC\ music\ notation TRANSLATION\ MISSING
" menutrans ABEL TRANSLATION\ MISSING
" menutrans AceDB\ model TRANSLATION\ MISSING
" menutrans Ada TRANSLATION\ MISSING
" menutrans AfLex TRANSLATION\ MISSING
" menutrans ALSA\ config TRANSLATION\ MISSING
" menutrans Altera\ AHDL TRANSLATION\ MISSING
" menutrans Amiga\ DOS TRANSLATION\ MISSING
" menutrans AMPL TRANSLATION\ MISSING
" menutrans Ant\ build\ file TRANSLATION\ MISSING
" menutrans ANTLR TRANSLATION\ MISSING
" menutrans Apache\ config TRANSLATION\ MISSING
" menutrans Apache-style\ config TRANSLATION\ MISSING
" menutrans Applix\ ELF TRANSLATION\ MISSING
" menutrans APT\ config TRANSLATION\ MISSING
" menutrans Arc\ Macro\ Language TRANSLATION\ MISSING
" menutrans Arch\ inventory TRANSLATION\ MISSING
" menutrans Arduino TRANSLATION\ MISSING
" menutrans ART TRANSLATION\ MISSING
" menutrans Ascii\ Doc TRANSLATION\ MISSING
" menutrans ASP\ with\ VBScript TRANSLATION\ MISSING
" menutrans ASP\ with\ Perl TRANSLATION\ MISSING
" menutrans Assembly TRANSLATION\ MISSING
" menutrans 680x0 TRANSLATION\ MISSING
" menutrans AVR TRANSLATION\ MISSING
" menutrans Flat TRANSLATION\ MISSING
" menutrans GNU TRANSLATION\ MISSING
" menutrans GNU\ H-8300 TRANSLATION\ MISSING
" menutrans Intel\ IA-64 TRANSLATION\ MISSING
" menutrans Microsoft TRANSLATION\ MISSING
" menutrans Netwide TRANSLATION\ MISSING
" menutrans PIC TRANSLATION\ MISSING
" menutrans Turbo TRANSLATION\ MISSING
" menutrans VAX\ Macro\ Assembly TRANSLATION\ MISSING
" menutrans Z-80 TRANSLATION\ MISSING
" menutrans xa\ 6502\ cross\ assember TRANSLATION\ MISSING
" menutrans ASN\.1 TRANSLATION\ MISSING
" menutrans Asterisk\ config TRANSLATION\ MISSING
" menutrans Asterisk\ voicemail\ config TRANSLATION\ MISSING
" menutrans Atlas TRANSLATION\ MISSING
" menutrans Autodoc TRANSLATION\ MISSING
" menutrans AutoHotKey TRANSLATION\ MISSING
" menutrans AutoIt TRANSLATION\ MISSING
" menutrans Automake TRANSLATION\ MISSING
" menutrans Avenue TRANSLATION\ MISSING
" menutrans Awk TRANSLATION\ MISSING
" menutrans AYacc TRANSLATION\ MISSING
" menutrans B TRANSLATION\ MISSING
" menutrans Baan TRANSLATION\ MISSING
" menutrans Bash TRANSLATION\ MISSING
" menutrans Basic TRANSLATION\ MISSING
" menutrans FreeBasic TRANSLATION\ MISSING
" menutrans IBasic TRANSLATION\ MISSING
" menutrans QBasic TRANSLATION\ MISSING
" menutrans Visual\ Basic TRANSLATION\ MISSING
" menutrans Bazaar\ commit\ file TRANSLATION\ MISSING
" menutrans Bazel TRANSLATION\ MISSING
" menutrans BC\ calculator TRANSLATION\ MISSING
" menutrans BDF\ font TRANSLATION\ MISSING
" menutrans BibTeX TRANSLATION\ MISSING
" menutrans Bibliography\ database TRANSLATION\ MISSING
" menutrans Bibliography\ Style TRANSLATION\ MISSING
" menutrans BIND TRANSLATION\ MISSING
" menutrans BIND\ config TRANSLATION\ MISSING
" menutrans BIND\ zone TRANSLATION\ MISSING
" menutrans Blank TRANSLATION\ MISSING
" menutrans C TRANSLATION\ MISSING
" menutrans C++ TRANSLATION\ MISSING
" menutrans C# TRANSLATION\ MISSING
" menutrans Cabal\ Haskell\ build\ file TRANSLATION\ MISSING
" menutrans Calendar TRANSLATION\ MISSING
" menutrans Cascading\ Style\ Sheets TRANSLATION\ MISSING
" menutrans CDL TRANSLATION\ MISSING
" menutrans Cdrdao\ TOC TRANSLATION\ MISSING
" menutrans Cdrdao\ config TRANSLATION\ MISSING
" menutrans Century\ Term TRANSLATION\ MISSING
" menutrans CH\ script TRANSLATION\ MISSING
" menutrans ChaiScript TRANSLATION\ MISSING
" menutrans Changelog TRANSLATION\ MISSING
" menutrans CHILL TRANSLATION\ MISSING
" menutrans Cheetah\ template TRANSLATION\ MISSING
" menutrans Chicken TRANSLATION\ MISSING
" menutrans ChordPro TRANSLATION\ MISSING
" menutrans Clean TRANSLATION\ MISSING
" menutrans Clever TRANSLATION\ MISSING
" menutrans Clipper TRANSLATION\ MISSING
" menutrans Clojure TRANSLATION\ MISSING
" menutrans Cmake TRANSLATION\ MISSING
" menutrans Cmod TRANSLATION\ MISSING
" menutrans Cmusrc TRANSLATION\ MISSING
" menutrans Cobol TRANSLATION\ MISSING
" menutrans Coco/R TRANSLATION\ MISSING
" menutrans Cold\ Fusion TRANSLATION\ MISSING
" menutrans Conary\ Recipe TRANSLATION\ MISSING
" menutrans Config TRANSLATION\ MISSING
" menutrans Cfg\ Config\ file TRANSLATION\ MISSING
" menutrans Configure\.in TRANSLATION\ MISSING
" menutrans Generic\ Config\ file TRANSLATION\ MISSING
" menutrans CRM114 TRANSLATION\ MISSING
" menutrans Crontab TRANSLATION\ MISSING
" menutrans CSDL TRANSLATION\ MISSING
" menutrans CSP TRANSLATION\ MISSING
" menutrans Ctrl-H TRANSLATION\ MISSING
" menutrans Cucumber TRANSLATION\ MISSING
" menutrans CUDA TRANSLATION\ MISSING
" menutrans CUPL TRANSLATION\ MISSING
" menutrans Simulation TRANSLATION\ MISSING
" menutrans CVS TRANSLATION\ MISSING
" menutrans commit\ file TRANSLATION\ MISSING
" menutrans cvsrc TRANSLATION\ MISSING
" menutrans Cyn++ TRANSLATION\ MISSING
" menutrans Cynlib TRANSLATION\ MISSING
" menutrans DE TRANSLATION\ MISSING
" menutrans D TRANSLATION\ MISSING
" menutrans Dart TRANSLATION\ MISSING
" menutrans Datascript TRANSLATION\ MISSING
" menutrans Debian TRANSLATION\ MISSING
" menutrans Debian\ ChangeLog TRANSLATION\ MISSING
" menutrans Debian\ Control TRANSLATION\ MISSING
" menutrans Debian\ Copyright TRANSLATION\ MISSING
" menutrans Debian\ Sources\.list TRANSLATION\ MISSING
" menutrans Denyhosts TRANSLATION\ MISSING
" menutrans Desktop TRANSLATION\ MISSING
" menutrans Dict\ config TRANSLATION\ MISSING
" menutrans Dictd\ config TRANSLATION\ MISSING
" menutrans Diff TRANSLATION\ MISSING
" menutrans Digital\ Command\ Lang TRANSLATION\ MISSING
" menutrans Dircolors TRANSLATION\ MISSING
" menutrans Dirpager TRANSLATION\ MISSING
" menutrans Django\ template TRANSLATION\ MISSING
" menutrans DNS/BIND\ zone TRANSLATION\ MISSING
" menutrans Dnsmasq\ config TRANSLATION\ MISSING
" menutrans DocBook TRANSLATION\ MISSING
" menutrans auto-detect TRANSLATION\ MISSING
" menutrans SGML TRANSLATION\ MISSING
" menutrans XML TRANSLATION\ MISSING
" menutrans Dockerfile TRANSLATION\ MISSING
" menutrans Dot TRANSLATION\ MISSING
" menutrans Doxygen TRANSLATION\ MISSING
" menutrans C\ with\ doxygen TRANSLATION\ MISSING
" menutrans C++\ with\ doxygen TRANSLATION\ MISSING
" menutrans IDL\ with\ doxygen TRANSLATION\ MISSING
" menutrans Java\ with\ doxygen TRANSLATION\ MISSING
" menutrans DataScript\ with\ doxygen TRANSLATION\ MISSING
" menutrans Dracula TRANSLATION\ MISSING
" menutrans DSSSL TRANSLATION\ MISSING
" menutrans DTD TRANSLATION\ MISSING
" menutrans DTML\ (Zope) TRANSLATION\ MISSING
" menutrans DTrace TRANSLATION\ MISSING
" menutrans Dts/dtsi TRANSLATION\ MISSING
" menutrans Dune TRANSLATION\ MISSING
" menutrans Dylan TRANSLATION\ MISSING
" menutrans Dylan\ interface TRANSLATION\ MISSING
" menutrans Dylan\ lid TRANSLATION\ MISSING
" menutrans EDIF TRANSLATION\ MISSING
" menutrans Eiffel TRANSLATION\ MISSING
" menutrans Eight TRANSLATION\ MISSING
" menutrans Elinks\ config TRANSLATION\ MISSING
" menutrans Elm\ filter\ rules TRANSLATION\ MISSING
" menutrans Embedix\ Component\ Description TRANSLATION\ MISSING
" menutrans ERicsson\ LANGuage TRANSLATION\ MISSING
" menutrans ESMTP\ rc TRANSLATION\ MISSING
" menutrans ESQL-C TRANSLATION\ MISSING
" menutrans Essbase\ script TRANSLATION\ MISSING
" menutrans Esterel TRANSLATION\ MISSING
" menutrans Eterm\ config TRANSLATION\ MISSING
" menutrans Euphoria\ 3 TRANSLATION\ MISSING
" menutrans Euphoria\ 4 TRANSLATION\ MISSING
" menutrans Eviews TRANSLATION\ MISSING
" menutrans Exim\ conf TRANSLATION\ MISSING
" menutrans Expect TRANSLATION\ MISSING
" menutrans Exports TRANSLATION\ MISSING
" menutrans FG TRANSLATION\ MISSING
" menutrans Falcon TRANSLATION\ MISSING
" menutrans Fantom TRANSLATION\ MISSING
" menutrans Fetchmail TRANSLATION\ MISSING
" menutrans FlexWiki TRANSLATION\ MISSING
" menutrans Focus\ Executable TRANSLATION\ MISSING
" menutrans Focus\ Master TRANSLATION\ MISSING
" menutrans FORM TRANSLATION\ MISSING
" menutrans Forth TRANSLATION\ MISSING
" menutrans Fortran TRANSLATION\ MISSING
" menutrans FoxPro TRANSLATION\ MISSING
" menutrans FrameScript TRANSLATION\ MISSING
" menutrans Fstab TRANSLATION\ MISSING
" menutrans Fvwm TRANSLATION\ MISSING
" menutrans Fvwm\ configuration TRANSLATION\ MISSING
" menutrans Fvwm2\ configuration TRANSLATION\ MISSING
" menutrans Fvwm2\ configuration\ with\ M4 TRANSLATION\ MISSING
" menutrans GDB\ command\ file TRANSLATION\ MISSING
" menutrans GDMO TRANSLATION\ MISSING
" menutrans Gedcom TRANSLATION\ MISSING
" menutrans Git TRANSLATION\ MISSING
" menutrans Output TRANSLATION\ MISSING
" menutrans Commit TRANSLATION\ MISSING
" menutrans Rebase TRANSLATION\ MISSING
" menutrans Send\ Email TRANSLATION\ MISSING
" menutrans Gitolite TRANSLATION\ MISSING
" menutrans Gkrellmrc TRANSLATION\ MISSING
" menutrans Gnash TRANSLATION\ MISSING
" menutrans Go TRANSLATION\ MISSING
" menutrans Godoc TRANSLATION\ MISSING
" menutrans GP TRANSLATION\ MISSING
" menutrans GPG TRANSLATION\ MISSING
" menutrans Grof TRANSLATION\ MISSING
" menutrans Group\ file TRANSLATION\ MISSING
" menutrans Grub TRANSLATION\ MISSING
" menutrans GNU\ Server\ Pages TRANSLATION\ MISSING
" menutrans GNUplot TRANSLATION\ MISSING
" menutrans GrADS\ scripts TRANSLATION\ MISSING
" menutrans Gretl TRANSLATION\ MISSING
" menutrans Groff TRANSLATION\ MISSING
" menutrans Groovy TRANSLATION\ MISSING
" menutrans GTKrc TRANSLATION\ MISSING
" menutrans HIJK TRANSLATION\ MISSING
" menutrans Haml TRANSLATION\ MISSING
" menutrans Hamster TRANSLATION\ MISSING
" menutrans Haskell TRANSLATION\ MISSING
" menutrans Haskell-c2hs TRANSLATION\ MISSING
" menutrans Haskell-literate TRANSLATION\ MISSING
" menutrans HASTE TRANSLATION\ MISSING
" menutrans HASTE\ preproc TRANSLATION\ MISSING
" menutrans Hercules TRANSLATION\ MISSING
" menutrans Hex\ dump TRANSLATION\ MISSING
" menutrans XXD TRANSLATION\ MISSING
" menutrans Intel\ MCS51 TRANSLATION\ MISSING
" menutrans Hg\ commit TRANSLATION\ MISSING
" menutrans Hollywood TRANSLATION\ MISSING
" menutrans HTML TRANSLATION\ MISSING
" menutrans HTML\ with\ M4 TRANSLATION\ MISSING
" menutrans HTML\ with\ Ruby\ (eRuby) TRANSLATION\ MISSING
" menutrans Cheetah\ HTML\ template TRANSLATION\ MISSING
" menutrans Django\ HTML\ template TRANSLATION\ MISSING
" menutrans Vue TRANSLATION\ MISSING
" menutrans js\ HTML\ template TRANSLATION\ MISSING
" menutrans HTML/OS TRANSLATION\ MISSING
" menutrans XHTML TRANSLATION\ MISSING
" menutrans Host\.conf TRANSLATION\ MISSING
" menutrans Hosts\ access TRANSLATION\ MISSING
" menutrans Hyper\ Builder TRANSLATION\ MISSING
" menutrans Icewm\ menu TRANSLATION\ MISSING
" menutrans Icon TRANSLATION\ MISSING
" menutrans IDL\Generic\ IDL TRANSLATION\ MISSING
" menutrans IDL\Microsoft\ IDL TRANSLATION\ MISSING
" menutrans Indent\ profile TRANSLATION\ MISSING
" menutrans Inform TRANSLATION\ MISSING
" menutrans Informix\ 4GL TRANSLATION\ MISSING
" menutrans Initng TRANSLATION\ MISSING
" menutrans Inittab TRANSLATION\ MISSING
" menutrans Inno\ setup TRANSLATION\ MISSING
" menutrans Innovation\ Data\ Processing TRANSLATION\ MISSING
" menutrans Upstream\ dat TRANSLATION\ MISSING
" menutrans Upstream\ log TRANSLATION\ MISSING
" menutrans Upstream\ rpt TRANSLATION\ MISSING
" menutrans Upstream\ Install\ log TRANSLATION\ MISSING
" menutrans Usserver\ log TRANSLATION\ MISSING
" menutrans USW2KAgt\ log TRANSLATION\ MISSING
" menutrans InstallShield\ script TRANSLATION\ MISSING
" menutrans Interactive\ Data\ Lang TRANSLATION\ MISSING
" menutrans IPfilter TRANSLATION\ MISSING
" menutrans J TRANSLATION\ MISSING
" menutrans JAL TRANSLATION\ MISSING
" menutrans JAM TRANSLATION\ MISSING
" menutrans Jargon TRANSLATION\ MISSING
" menutrans Java TRANSLATION\ MISSING
" menutrans JavaCC TRANSLATION\ MISSING
" menutrans Java\ Server\ Pages TRANSLATION\ MISSING
" menutrans Java\ Properties TRANSLATION\ MISSING
" menutrans JavaScript TRANSLATION\ MISSING
" menutrans JavaScriptReact TRANSLATION\ MISSING
" menutrans Jess TRANSLATION\ MISSING
" menutrans Jgraph TRANSLATION\ MISSING
" menutrans Jovial TRANSLATION\ MISSING
" menutrans JSON TRANSLATION\ MISSING
" menutrans Kconfig TRANSLATION\ MISSING
" menutrans KDE\ script TRANSLATION\ MISSING
" menutrans Kimwitu++ TRANSLATION\ MISSING
" menutrans Kivy TRANSLATION\ MISSING
" menutrans KixTart TRANSLATION\ MISSING
" menutrans L TRANSLATION\ MISSING
" menutrans Lace TRANSLATION\ MISSING
" menutrans LamdaProlog TRANSLATION\ MISSING
" menutrans Latte TRANSLATION\ MISSING
" menutrans Ld\ script TRANSLATION\ MISSING
" menutrans LDAP TRANSLATION\ MISSING
" menutrans LDIF TRANSLATION\ MISSING
" menutrans Configuration TRANSLATION\ MISSING
" menutrans Less TRANSLATION\ MISSING
" menutrans Lex TRANSLATION\ MISSING
" menutrans LFTP\ config TRANSLATION\ MISSING
" menutrans Libao TRANSLATION\ MISSING
" menutrans LifeLines\ script TRANSLATION\ MISSING
" menutrans Lilo TRANSLATION\ MISSING
" menutrans Limits\ config TRANSLATION\ MISSING
" menutrans Linden\ scripting TRANSLATION\ MISSING
" menutrans Liquid TRANSLATION\ MISSING
" menutrans Lisp TRANSLATION\ MISSING
" menutrans Lite TRANSLATION\ MISSING
" menutrans LiteStep\ RC TRANSLATION\ MISSING
" menutrans Locale\ Input TRANSLATION\ MISSING
" menutrans Login\.access TRANSLATION\ MISSING
" menutrans Login\.defs TRANSLATION\ MISSING
" menutrans Logtalk TRANSLATION\ MISSING
" menutrans LOTOS TRANSLATION\ MISSING
" menutrans LotusScript TRANSLATION\ MISSING
" menutrans Lout TRANSLATION\ MISSING
" menutrans LPC TRANSLATION\ MISSING
" menutrans Lua TRANSLATION\ MISSING
" menutrans Lynx\ Style TRANSLATION\ MISSING
" menutrans Lynx\ config TRANSLATION\ MISSING
" menutrans M TRANSLATION\ MISSING
" menutrans M4 TRANSLATION\ MISSING
" menutrans MaGic\ Point TRANSLATION\ MISSING
" menutrans Mail\ aliases TRANSLATION\ MISSING
" menutrans Mailcap TRANSLATION\ MISSING
" menutrans Mallard TRANSLATION\ MISSING
" menutrans Makefile TRANSLATION\ MISSING
" menutrans MakeIndex TRANSLATION\ MISSING
" menutrans Man\ page TRANSLATION\ MISSING
" menutrans Man\.conf TRANSLATION\ MISSING
" menutrans Maple\ V TRANSLATION\ MISSING
" menutrans Markdown TRANSLATION\ MISSING
" menutrans Markdown\ with\ R\ statements TRANSLATION\ MISSING
" menutrans Mason TRANSLATION\ MISSING
" menutrans Mathematica TRANSLATION\ MISSING
" menutrans Matlab TRANSLATION\ MISSING
" menutrans Maxima TRANSLATION\ MISSING
" menutrans MEL\ (for\ Maya) TRANSLATION\ MISSING
" menutrans Meson TRANSLATION\ MISSING
" menutrans Messages\ (/var/log) TRANSLATION\ MISSING
" menutrans Metafont TRANSLATION\ MISSING
" menutrans MetaPost TRANSLATION\ MISSING
" menutrans MGL TRANSLATION\ MISSING
" menutrans MIX TRANSLATION\ MISSING
" menutrans MMIX TRANSLATION\ MISSING
" menutrans Modconf TRANSLATION\ MISSING
" menutrans Model TRANSLATION\ MISSING
" menutrans Modsim\ III TRANSLATION\ MISSING
" menutrans Modula\ 2 TRANSLATION\ MISSING
" menutrans Modula\ 3 TRANSLATION\ MISSING
" menutrans Monk TRANSLATION\ MISSING
" menutrans Motorola\ S-Record TRANSLATION\ MISSING
" menutrans Mplayer\ config TRANSLATION\ MISSING
" menutrans MOO TRANSLATION\ MISSING
" menutrans Mrxvtrc TRANSLATION\ MISSING
" menutrans MS-DOS/Windows TRANSLATION\ MISSING
" menutrans 4DOS\ \.bat\ file TRANSLATION\ MISSING
" menutrans \.bat\/\.cmd\ file TRANSLATION\ MISSING
" menutrans \.ini\ file TRANSLATION\ MISSING
" menutrans Message\ text TRANSLATION\ MISSING
" menutrans Module\ Definition TRANSLATION\ MISSING
" menutrans Registry TRANSLATION\ MISSING
" menutrans Resource\ file TRANSLATION\ MISSING
" menutrans Msql TRANSLATION\ MISSING
" menutrans MuPAD TRANSLATION\ MISSING
" menutrans Murphi TRANSLATION\ MISSING
" menutrans MUSHcode TRANSLATION\ MISSING
" menutrans Muttrc TRANSLATION\ MISSING
" menutrans NO TRANSLATION\ MISSING
" menutrans N1QL TRANSLATION\ MISSING
" menutrans Nanorc TRANSLATION\ MISSING
" menutrans Nastran\ input/DMAP TRANSLATION\ MISSING
" menutrans Natural TRANSLATION\ MISSING
" menutrans NeoMutt\ setup\ files TRANSLATION\ MISSING
" menutrans Netrc TRANSLATION\ MISSING
" menutrans Ninja TRANSLATION\ MISSING
" menutrans Novell\ NCF\ batch TRANSLATION\ MISSING
" menutrans Not\ Quite\ C\ (LEGO) TRANSLATION\ MISSING
" menutrans Nroff TRANSLATION\ MISSING
" menutrans NSIS\ script TRANSLATION\ MISSING
" menutrans Obj\ 3D\ wavefront TRANSLATION\ MISSING
" menutrans Objective\ C TRANSLATION\ MISSING
" menutrans Objective\ C++ TRANSLATION\ MISSING
" menutrans OCAML TRANSLATION\ MISSING
" menutrans Occam TRANSLATION\ MISSING
" menutrans Omnimark TRANSLATION\ MISSING
" menutrans OpenROAD TRANSLATION\ MISSING
" menutrans Open\ Psion\ Lang TRANSLATION\ MISSING
" menutrans Oracle\ config TRANSLATION\ MISSING
" menutrans PQ TRANSLATION\ MISSING
" menutrans Packet\ filter\ conf TRANSLATION\ MISSING
" menutrans Palm\ resource\ compiler TRANSLATION\ MISSING
" menutrans Pam\ config TRANSLATION\ MISSING
" menutrans PApp TRANSLATION\ MISSING
" menutrans Pascal TRANSLATION\ MISSING
" menutrans Password\ file TRANSLATION\ MISSING
" menutrans PCCTS TRANSLATION\ MISSING
" menutrans PDF TRANSLATION\ MISSING
" menutrans Perl TRANSLATION\ MISSING
" menutrans Perl\ 6 TRANSLATION\ MISSING
" menutrans Perl\ POD TRANSLATION\ MISSING
" menutrans Perl\ XS TRANSLATION\ MISSING
" menutrans Template\ toolkit TRANSLATION\ MISSING
" menutrans Template\ toolkit\ Html TRANSLATION\ MISSING
" menutrans Template\ toolkit\ JS TRANSLATION\ MISSING
" menutrans PHP TRANSLATION\ MISSING
" menutrans PHP\ 3-4 TRANSLATION\ MISSING
" menutrans Phtml\ (PHP\ 2) TRANSLATION\ MISSING
" menutrans Pike TRANSLATION\ MISSING
" menutrans Pine\ RC TRANSLATION\ MISSING
" menutrans Pinfo\ RC TRANSLATION\ MISSING
" menutrans PL/M TRANSLATION\ MISSING
" menutrans PL/SQL TRANSLATION\ MISSING
" menutrans Pli TRANSLATION\ MISSING
" menutrans PLP TRANSLATION\ MISSING
" menutrans PO\ (GNU\ gettext) TRANSLATION\ MISSING
" menutrans Postfix\ main\ config TRANSLATION\ MISSING
" menutrans PostScript TRANSLATION\ MISSING
" menutrans PostScript\ Printer\ Description TRANSLATION\ MISSING
" menutrans Povray TRANSLATION\ MISSING
" menutrans Povray\ scene\ descr TRANSLATION\ MISSING
" menutrans Povray\ configuration TRANSLATION\ MISSING
" menutrans PPWizard TRANSLATION\ MISSING
" menutrans Prescribe\ (Kyocera) TRANSLATION\ MISSING
" menutrans Printcap TRANSLATION\ MISSING
" menutrans Privoxy TRANSLATION\ MISSING
" menutrans Procmail TRANSLATION\ MISSING
" menutrans Product\ Spec\ File TRANSLATION\ MISSING
" menutrans Progress TRANSLATION\ MISSING
" menutrans Prolog TRANSLATION\ MISSING
" menutrans ProMeLa TRANSLATION\ MISSING
" menutrans Proto TRANSLATION\ MISSING
" menutrans Protocols TRANSLATION\ MISSING
" menutrans Purify\ log TRANSLATION\ MISSING
" menutrans Pyrex TRANSLATION\ MISSING
" menutrans Python TRANSLATION\ MISSING
" menutrans Quake TRANSLATION\ MISSING
" menutrans Quickfix\ window TRANSLATION\ MISSING
" menutrans R TRANSLATION\ MISSING
" menutrans R\ help TRANSLATION\ MISSING
" menutrans R\ noweb TRANSLATION\ MISSING
" menutrans Racc\ input TRANSLATION\ MISSING
" menutrans Radiance TRANSLATION\ MISSING
" menutrans Raml TRANSLATION\ MISSING
" menutrans Ratpoison TRANSLATION\ MISSING
" menutrans RCS TRANSLATION\ MISSING
" menutrans RCS\ log\ output TRANSLATION\ MISSING
" menutrans RCS\ file TRANSLATION\ MISSING
" menutrans Readline\ config TRANSLATION\ MISSING
" menutrans Rebol TRANSLATION\ MISSING
" menutrans ReDIF TRANSLATION\ MISSING
" menutrans Rego TRANSLATION\ MISSING
" menutrans Relax\ NG TRANSLATION\ MISSING
" menutrans Remind TRANSLATION\ MISSING
" menutrans Relax\ NG\ compact TRANSLATION\ MISSING
" menutrans Renderman TRANSLATION\ MISSING
" menutrans Renderman\ Shader\ Lang TRANSLATION\ MISSING
" menutrans Renderman\ Interface\ Bytestream TRANSLATION\ MISSING
" menutrans Resolv\.conf TRANSLATION\ MISSING
" menutrans Reva\ Forth TRANSLATION\ MISSING
" menutrans Rexx TRANSLATION\ MISSING
" menutrans Robots\.txt TRANSLATION\ MISSING
" menutrans RockLinux\ package\ desc\. TRANSLATION\ MISSING
" menutrans Rpcgen TRANSLATION\ MISSING
" menutrans RPL/2 TRANSLATION\ MISSING
" menutrans ReStructuredText TRANSLATION\ MISSING
" menutrans ReStructuredText\ with\ R\ statements TRANSLATION\ MISSING
" menutrans RTF TRANSLATION\ MISSING
" menutrans Ruby TRANSLATION\ MISSING
" menutrans Rust TRANSLATION\ MISSING
" menutrans S-Sm TRANSLATION\ MISSING
" menutrans S-Lang TRANSLATION\ MISSING
" menutrans Samba\ config TRANSLATION\ MISSING
" menutrans SAS TRANSLATION\ MISSING
" menutrans Sass TRANSLATION\ MISSING
" menutrans Sather TRANSLATION\ MISSING
" menutrans Sbt TRANSLATION\ MISSING
" menutrans Scala TRANSLATION\ MISSING
" menutrans Scheme TRANSLATION\ MISSING
" menutrans Scilab TRANSLATION\ MISSING
" menutrans Screen\ RC TRANSLATION\ MISSING
" menutrans SCSS TRANSLATION\ MISSING
" menutrans SDC\ Synopsys\ Design\ Constraints TRANSLATION\ MISSING
" menutrans SDL TRANSLATION\ MISSING
" menutrans Sed TRANSLATION\ MISSING
" menutrans Sendmail\.cf TRANSLATION\ MISSING
" menutrans Send-pr TRANSLATION\ MISSING
" menutrans Sensors\.conf TRANSLATION\ MISSING
" menutrans Service\ Location\ config TRANSLATION\ MISSING
" menutrans Service\ Location\ registration TRANSLATION\ MISSING
" menutrans Service\ Location\ SPI TRANSLATION\ MISSING
" menutrans Services TRANSLATION\ MISSING
" menutrans Setserial\ config TRANSLATION\ MISSING
" menutrans SGML\ catalog TRANSLATION\ MISSING
" menutrans SGML\ DTD TRANSLATION\ MISSING
" menutrans SGML\ Declaration TRANSLATION\ MISSING
" menutrans SGML-linuxdoc TRANSLATION\ MISSING
" menutrans Shell\ script TRANSLATION\ MISSING
" menutrans sh\ and\ ksh TRANSLATION\ MISSING
" menutrans csh TRANSLATION\ MISSING
" menutrans tcsh TRANSLATION\ MISSING
" menutrans zsh TRANSLATION\ MISSING
" menutrans SiCAD TRANSLATION\ MISSING
" menutrans Sieve TRANSLATION\ MISSING
" menutrans Simula TRANSLATION\ MISSING
" menutrans Sinda TRANSLATION\ MISSING
" menutrans Sinda\ compare TRANSLATION\ MISSING
" menutrans Sinda\ input TRANSLATION\ MISSING
" menutrans Sinda\ output TRANSLATION\ MISSING
" menutrans SiSU TRANSLATION\ MISSING
" menutrans SKILL TRANSLATION\ MISSING
" menutrans SKILL\ for\ Diva TRANSLATION\ MISSING
" menutrans Slice TRANSLATION\ MISSING
" menutrans SLRN TRANSLATION\ MISSING
" menutrans Slrn\ rc TRANSLATION\ MISSING
" menutrans Slrn\ score TRANSLATION\ MISSING
" menutrans SmallTalk TRANSLATION\ MISSING
" menutrans Smarty\ Templates TRANSLATION\ MISSING
" menutrans SMIL TRANSLATION\ MISSING
" menutrans SMITH TRANSLATION\ MISSING
" menutrans Sn-Sy TRANSLATION\ MISSING
" menutrans SNMP\ MIB TRANSLATION\ MISSING
" menutrans SNNS TRANSLATION\ MISSING
" menutrans SNNS\ network TRANSLATION\ MISSING
" menutrans SNNS\ pattern TRANSLATION\ MISSING
" menutrans SNNS\ result TRANSLATION\ MISSING
" menutrans Snobol4 TRANSLATION\ MISSING
" menutrans Snort\ Configuration TRANSLATION\ MISSING
" menutrans SPEC\ (Linux\ RPM) TRANSLATION\ MISSING
" menutrans Specman TRANSLATION\ MISSING
" menutrans Spice TRANSLATION\ MISSING
" menutrans Spyce TRANSLATION\ MISSING
" menutrans Speedup TRANSLATION\ MISSING
" menutrans Splint TRANSLATION\ MISSING
" menutrans Squid\ config TRANSLATION\ MISSING
" menutrans SQL TRANSLATION\ MISSING
" menutrans SAP\ HANA TRANSLATION\ MISSING
" menutrans MySQL TRANSLATION\ MISSING
" menutrans SQL\ Anywhere TRANSLATION\ MISSING
" menutrans SQL\ (automatic) TRANSLATION\ MISSING
" menutrans SQL\ (Oracle) TRANSLATION\ MISSING
" menutrans SQL\ Forms TRANSLATION\ MISSING
" menutrans SQLJ TRANSLATION\ MISSING
" menutrans SQL-Informix TRANSLATION\ MISSING
" menutrans SQR TRANSLATION\ MISSING
" menutrans Ssh TRANSLATION\ MISSING
" menutrans ssh_config TRANSLATION\ MISSING
" menutrans sshd_config TRANSLATION\ MISSING
" menutrans Standard\ ML TRANSLATION\ MISSING
" menutrans Stata TRANSLATION\ MISSING
" menutrans SMCL TRANSLATION\ MISSING
" menutrans Stored\ Procedures TRANSLATION\ MISSING
" menutrans Strace TRANSLATION\ MISSING
" menutrans Streaming\ descriptor\ file TRANSLATION\ MISSING
" menutrans Subversion\ commit TRANSLATION\ MISSING
" menutrans Sudoers TRANSLATION\ MISSING
" menutrans SVG TRANSLATION\ MISSING
" menutrans Symbian\ meta-makefile TRANSLATION\ MISSING
" menutrans Sysctl\.conf TRANSLATION\ MISSING
" menutrans Systemd TRANSLATION\ MISSING
" menutrans SystemVerilog TRANSLATION\ MISSING
" menutrans T TRANSLATION\ MISSING
" menutrans TADS TRANSLATION\ MISSING
" menutrans Tags TRANSLATION\ MISSING
" menutrans TAK TRANSLATION\ MISSING
" menutrans TAK\ compare TRANSLATION\ MISSING
" menutrans TAK\ input TRANSLATION\ MISSING
" menutrans TAK\ output TRANSLATION\ MISSING
" menutrans Tar\ listing TRANSLATION\ MISSING
" menutrans Task\ data TRANSLATION\ MISSING
" menutrans Task\ 42\ edit TRANSLATION\ MISSING
" menutrans Tcl/Tk TRANSLATION\ MISSING
" menutrans TealInfo TRANSLATION\ MISSING
" menutrans Telix\ Salt TRANSLATION\ MISSING
" menutrans Termcap/Printcap TRANSLATION\ MISSING
" menutrans Terminfo TRANSLATION\ MISSING
" menutrans Tera\ Term TRANSLATION\ MISSING
" menutrans TeX TRANSLATION\ MISSING
" menutrans TeX/LaTeX TRANSLATION\ MISSING
" menutrans plain\ TeX TRANSLATION\ MISSING
" menutrans Initex TRANSLATION\ MISSING
" menutrans ConTeXt TRANSLATION\ MISSING
" menutrans TeX\ configuration TRANSLATION\ MISSING
" menutrans Texinfo TRANSLATION\ MISSING
" menutrans TF\ mud\ client TRANSLATION\ MISSING
" menutrans Tidy\ configuration TRANSLATION\ MISSING
" menutrans Tilde TRANSLATION\ MISSING
" menutrans Tmux\ configuration TRANSLATION\ MISSING
" menutrans TPP TRANSLATION\ MISSING
" menutrans Trasys\ input TRANSLATION\ MISSING
" menutrans Treetop TRANSLATION\ MISSING
" menutrans Trustees TRANSLATION\ MISSING
" menutrans TSS TRANSLATION\ MISSING
" menutrans Command\ Line TRANSLATION\ MISSING
" menutrans Geometry TRANSLATION\ MISSING
" menutrans Optics TRANSLATION\ MISSING
" menutrans Typescript TRANSLATION\ MISSING
" menutrans TypescriptReact TRANSLATION\ MISSING
" menutrans UV TRANSLATION\ MISSING
" menutrans Udev\ config TRANSLATION\ MISSING
" menutrans Udev\ permissions TRANSLATION\ MISSING
" menutrans Udev\ rules TRANSLATION\ MISSING
" menutrans UIT/UIL TRANSLATION\ MISSING
" menutrans UnrealScript TRANSLATION\ MISSING
" menutrans Updatedb\.conf TRANSLATION\ MISSING
" menutrans Upstart TRANSLATION\ MISSING
" menutrans Valgrind TRANSLATION\ MISSING
" menutrans Vera TRANSLATION\ MISSING
" menutrans Verbose\ TAP\ Output TRANSLATION\ MISSING
" menutrans Verilog-AMS\ HDL TRANSLATION\ MISSING
" menutrans Verilog\ HDL TRANSLATION\ MISSING
" menutrans Vgrindefs TRANSLATION\ MISSING
" menutrans VHDL TRANSLATION\ MISSING
" menutrans Vim TRANSLATION\ MISSING
" menutrans Vim\ help\ file TRANSLATION\ MISSING
" menutrans Vim\ script TRANSLATION\ MISSING
" menutrans Viminfo\ file TRANSLATION\ MISSING
" menutrans Virata\ config TRANSLATION\ MISSING
" menutrans VOS\ CM\ macro TRANSLATION\ MISSING
" menutrans VRML TRANSLATION\ MISSING
" menutrans Vroom TRANSLATION\ MISSING
" menutrans VSE\ JCL TRANSLATION\ MISSING
" menutrans WXYZ TRANSLATION\ MISSING
" menutrans WEB TRANSLATION\ MISSING
" menutrans CWEB TRANSLATION\ MISSING
" menutrans WEB\ Changes TRANSLATION\ MISSING
" menutrans WebAssembly TRANSLATION\ MISSING
" menutrans Webmacro TRANSLATION\ MISSING
" menutrans Website\ MetaLanguage TRANSLATION\ MISSING
" menutrans wDiff TRANSLATION\ MISSING
" menutrans Wget\ config TRANSLATION\ MISSING
" menutrans Whitespace\ (add) TRANSLATION\ MISSING
" menutrans WildPackets\ EtherPeek\ Decoder TRANSLATION\ MISSING
" menutrans WinBatch/Webbatch TRANSLATION\ MISSING
" menutrans Windows\ Scripting\ Host TRANSLATION\ MISSING
" menutrans WSML TRANSLATION\ MISSING
" menutrans WvDial TRANSLATION\ MISSING
" menutrans X\ Keyboard\ Extension TRANSLATION\ MISSING
" menutrans X\ Pixmap TRANSLATION\ MISSING
" menutrans X\ Pixmap\ (2) TRANSLATION\ MISSING
" menutrans X\ resources TRANSLATION\ MISSING
" menutrans XBL TRANSLATION\ MISSING
" menutrans Xinetd\.conf TRANSLATION\ MISSING
" menutrans Xmodmap TRANSLATION\ MISSING
" menutrans Xmath TRANSLATION\ MISSING
" menutrans XML\ Schema\ (XSD) TRANSLATION\ MISSING
" menutrans XQuery TRANSLATION\ MISSING
" menutrans Xslt TRANSLATION\ MISSING
" menutrans XFree86\ Config TRANSLATION\ MISSING
" menutrans YAML TRANSLATION\ MISSING
" menutrans Yacc TRANSLATION\ MISSING
" menutrans Zimbu TRANSLATION\ MISSING
" }}}

" Netrw menu {{{1
" Plugin loading may be after menu translation
" So giveup testing if Netrw Plugin is loaded
" if exists("g:loaded_netrwPlugin")
  " menutrans Help<tab><F1> TRANSLATION\ MISSING
  " menutrans Bookmarks TRANSLATION\ MISSING
  " menutrans History TRANSLATION\ MISSING
  " menutrans Go\ Up\ Directory<tab>- TRANSLATION\ MISSING
  " menutrans Apply\ Special\ Viewer<tab>x TRANSLATION\ MISSING
  " menutrans Bookmarks\ and\ History TRANSLATION\ MISSING
  " Netrw.Bookmarks and History menuitems {{{2
  " menutrans Bookmark\ Current\ Directory<tab>mb TRANSLATION\ MISSING
  " menutrans Bookmark\ Delete TRANSLATION\ MISSING
  " menutrans Goto\ Prev\ Dir\ (History)<tab>u TRANSLATION\ MISSING
  " menutrans Goto\ Next\ Dir\ (History)<tab>U TRANSLATION\ MISSING
  " menutrans List<tab>qb TRANSLATION\ MISSING
  " }}}
  " menutrans Browsing\ Control TRANSLATION\ MISSING
  " Netrw.Browsing Control menuitems {{{2
  " menutrans Horizontal\ Split<tab>o TRANSLATION\ MISSING
  " menutrans Vertical\ Split<tab>v TRANSLATION\ MISSING
  " menutrans New\ Tab<tab>t TRANSLATION\ MISSING
  " menutrans Preview<tab>p TRANSLATION\ MISSING
  " menutrans Edit\ File\ Hiding\ List<tab><ctrl-h> TRANSLATION\ MISSING
  " menutrans Edit\ Sorting\ Sequence<tab>S TRANSLATION\ MISSING
  " menutrans Quick\ Hide/Unhide\ Dot\ Files<tab>gh TRANSLATION\ MISSING
  " menutrans Refresh\ Listing<tab><ctrl-l> TRANSLATION\ MISSING
  " menutrans Settings/Options<tab>:NetrwSettings TRANSLATION\ MISSING
  " }}}
  " menutrans Delete\ File/Directory<tab>D TRANSLATION\ MISSING
  " menutrans Edit\ File/Dir TRANSLATION\ MISSING
  " Netrw.Edit File menuitems {{{2
  " menutrans Create\ New\ File<tab>% TRANSLATION\ MISSING
  " menutrans In\ Current\ Window<tab><cr> TRANSLATION\ MISSING
  " menutrans Preview\ File/Directory<tab>p TRANSLATION\ MISSING
  " menutrans In\ Previous\ Window<tab>P TRANSLATION\ MISSING
  " menutrans In\ New\ Window<tab>o TRANSLATION\ MISSING
  " menutrans In\ New\ Tab<tab>t TRANSLATION\ MISSING
  " menutrans In\ New\ Vertical\ Window<tab>v TRANSLATION\ MISSING
  " }}}
  " menutrans Explore TRANSLATION\ MISSING
  " Netrw.Explore menuitems {{{2
  " menutrans Directory\ Name TRANSLATION\ MISSING
  " menutrans Filenames\ Matching\ Pattern\ (curdir\ only)<tab>:Explore\ */ TRANSLATION\ MISSING
  " menutrans Filenames\ Matching\ Pattern\ (+subdirs)<tab>:Explore\ **/ TRANSLATION\ MISSING
  " menutrans Files\ Containing\ String\ Pattern\ (curdir\ only)<tab>:Explore\ *// TRANSLATION\ MISSING
  " menutrans Files\ Containing\ String\ Pattern\ (+subdirs)<tab>:Explore\ **// TRANSLATION\ MISSING
  " menutrans Next\ Match<tab>:Nexplore TRANSLATION\ MISSING
  " menutrans Prev\ Match<tab>:Pexplore TRANSLATION\ MISSING
  " }}}
  " menutrans Make\ Subdirectory<tab>d TRANSLATION\ MISSING
  " menutrans Marked\ Files TRANSLATION\ MISSING
  " Netrw.Marked Files menuitems {{{2
  " menutrans Mark\ File<tab>mf TRANSLATION\ MISSING
  " menutrans Mark\ Files\ by\ Regexp<tab>mr TRANSLATION\ MISSING
  " menutrans Hide-Show-List\ Control<tab>a TRANSLATION\ MISSING
  " menutrans Copy\ To\ Target<tab>mc TRANSLATION\ MISSING
  " menutrans Delete<tab>D TRANSLATION\ MISSING
  " menutrans Diff<tab>md TRANSLATION\ MISSING
  " menutrans Edit<tab>me TRANSLATION\ MISSING
  " menutrans Exe\ Cmd<tab>mx TRANSLATION\ MISSING
  " menutrans Move\ To\ Target<tab>mm TRANSLATION\ MISSING
  " menutrans Obtain<tab>O TRANSLATION\ MISSING
  " menutrans Print<tab>mp TRANSLATION\ MISSING
  " menutrans Replace<tab>R TRANSLATION\ MISSING
  " menutrans Set\ Target<tab>mt TRANSLATION\ MISSING
  " menutrans Tag<tab>mT TRANSLATION\ MISSING
  " menutrans Zip/Unzip/Compress/Uncompress<tab>mz TRANSLATION\ MISSING
  " }}}
  " menutrans Obtain\ File<tab>O TRANSLATION\ MISSING
  " menutrans Style TRANSLATION\ MISSING
  " Netrw.Style menuitems {{{2
  " menutrans Listing TRANSLATION\ MISSING
  " Netrw.Style.Listing menuitems {{{3
  " menutrans thin<tab>i TRANSLATION\ MISSING
  " menutrans long<tab>i TRANSLATION\ MISSING
  " menutrans wide<tab>i TRANSLATION\ MISSING
  " menutrans tree<tab>i TRANSLATION\ MISSING
  " }}}
  " menutrans Normal-Hide-Show TRANSLATION\ MISSING
  " Netrw.Style.Normal-Hide_show menuitems {{{3
  " menutrans Show\ All<tab>a TRANSLATION\ MISSING
  " menutrans Normal<tab>a TRANSLATION\ MISSING
  " menutrans Hidden\ Only<tab>a TRANSLATION\ MISSING
  " }}}
  " menutrans Reverse\ Sorting\ Order<tab>r TRANSLATION\ MISSING
  " menutrans Sorting\ Method TRANSLATION\ MISSING
  " Netrw.Style.Sorting Method menuitems {{{3
  " menutrans Name<tab>s TRANSLATION\ MISSING
  " menutrans Time<tab>s TRANSLATION\ MISSING
  " menutrans Size<tab>s TRANSLATION\ MISSING
  " menutrans Exten<tab>s TRANSLATION\ MISSING
  " }}}
  " }}}
  " menutrans Rename\ File/Directory<tab>R TRANSLATION\ MISSING
  " menutrans Set\ Current\ Directory<tab>c TRANSLATION\ MISSING
  " menutrans Targets TRANSLATION\ MISSING
" endif
" }}}

" Shellmenu menu
" Shellmenu menuitems {{{1
" From shellmenu.vim
" menutrans ShellMenu TRANSLATION\ MISSING
" menutrans MAIL TRANSLATION\ MISSING
" menutrans eval TRANSLATION\ MISSING
" menutrans Statements TRANSLATION\ MISSING
" menutrans for TRANSLATION\ MISSING
" menutrans case TRANSLATION\ MISSING
" menutrans if TRANSLATION\ MISSING
" menutrans if-else TRANSLATION\ MISSING
" menutrans elif TRANSLATION\ MISSING
" menutrans while TRANSLATION\ MISSING
" menutrans break TRANSLATION\ MISSING
" menutrans continue TRANSLATION\ MISSING
" menutrans function TRANSLATION\ MISSING
" menutrans return TRANSLATION\ MISSING
" menutrans return-true TRANSLATION\ MISSING
" menutrans return-false TRANSLATION\ MISSING
" menutrans exit TRANSLATION\ MISSING
" menutrans shift TRANSLATION\ MISSING
" menutrans trap TRANSLATION\ MISSING
" menutrans Test TRANSLATION\ MISSING
" menutrans Existence TRANSLATION\ MISSING
" menutrans Existence\ -\ file TRANSLATION\ MISSING
" menutrans Existence\ -\ file\ (not\ empty) TRANSLATION\ MISSING
" menutrans Existence\ -\ directory TRANSLATION\ MISSING
" menutrans Existence\ -\ executable TRANSLATION\ MISSING
" menutrans Existence\ -\ readable TRANSLATION\ MISSING
" menutrans Existence\ -\ writable TRANSLATION\ MISSING
" menutrans String\ is\ empty TRANSLATION\ MISSING
" menutrans String\ is\ not\ empty TRANSLATION\ MISSING
" menutrans Strings\ are\ equal TRANSLATION\ MISSING
" menutrans Strings\ are\ not\ equal TRANSLATION\ MISSING
" menutrans Value\ is\ greater\ than TRANSLATION\ MISSING
" menutrans Value\ is\ greater\ equal TRANSLATION\ MISSING
" menutrans Values\ are\ equal TRANSLATION\ MISSING
" menutrans Values\ are\ not\ equal TRANSLATION\ MISSING
" menutrans Value\ is\ less\ than TRANSLATION\ MISSING
" menutrans Value\ is\ less\ equal TRANSLATION\ MISSING
" menutrans ParmSub TRANSLATION\ MISSING
" menutrans Substitute\ word\ if\ parm\ not\ set TRANSLATION\ MISSING
" menutrans Set\ parm\ to\ word\ if\ not\ set TRANSLATION\ MISSING
" menutrans Substitute\ word\ if\ parm\ set\ else\ nothing TRANSLATION\ MISSING
" menutrans If\ parm\ not\ set\ print\ word\ and\ exit TRANSLATION\ MISSING
" menutrans SpShVars TRANSLATION\ MISSING
" menutrans Number\ of\ positional\ parameters TRANSLATION\ MISSING
" menutrans All\ positional\ parameters\ (quoted\ spaces) TRANSLATION\ MISSING
" menutrans All\ positional\ parameters\ (unquoted\ spaces) TRANSLATION\ MISSING
" menutrans Flags\ set TRANSLATION\ MISSING
" menutrans Return\ code\ of\ last\ command TRANSLATION\ MISSING
" menutrans Process\ number\ of\ this\ shell TRANSLATION\ MISSING
" menutrans Process\ number\ of\ last\ background\ command TRANSLATION\ MISSING
" menutrans Environ TRANSLATION\ MISSING
" menutrans HOME TRANSLATION\ MISSING
" menutrans PATH TRANSLATION\ MISSING
" menutrans CDPATH TRANSLATION\ MISSING
" menutrans MAILCHECK TRANSLATION\ MISSING
" menutrans PS1 TRANSLATION\ MISSING
" menutrans PS2 TRANSLATION\ MISSING
" menutrans IFS TRANSLATION\ MISSING
" menutrans SHACCT TRANSLATION\ MISSING
" menutrans SHELL TRANSLATION\ MISSING
" menutrans LC_CTYPE TRANSLATION\ MISSING
" menutrans LC_MESSAGES TRANSLATION\ MISSING
" menutrans Builtins TRANSLATION\ MISSING
" menutrans cd TRANSLATION\ MISSING
" menutrans echo TRANSLATION\ MISSING
" menutrans exec TRANSLATION\ MISSING
" menutrans export TRANSLATION\ MISSING
" menutrans getopts TRANSLATION\ MISSING
" menutrans hash TRANSLATION\ MISSING
" menutrans newgrp TRANSLATION\ MISSING
" menutrans pwd TRANSLATION\ MISSING
" menutrans read TRANSLATION\ MISSING
" menutrans readonly TRANSLATION\ MISSING
" menutrans times TRANSLATION\ MISSING
" menutrans type TRANSLATION\ MISSING
" menutrans umask TRANSLATION\ MISSING
" menutrans wait TRANSLATION\ MISSING
" menutrans Set TRANSLATION\ MISSING
" menutrans unset TRANSLATION\ MISSING
" menutrans Mark\ created\ or\ modified\ variables\ for\ export TRANSLATION\ MISSING
" menutrans Exit\ when\ command\ returns\ non-zero\ status TRANSLATION\ MISSING
" menutrans Disable\ file\ name\ expansion TRANSLATION\ MISSING
" menutrans Locate\ and\ remember\ commands\ when\ being\ looked\ up TRANSLATION\ MISSING
" menutrans All\ assignment\ statements\ are\ placed\ in\ the\ environment\ for\ a\ command TRANSLATION\ MISSING
" menutrans Read\ commands\ but\ do\ not\ execute\ them TRANSLATION\ MISSING
" menutrans Exit\ after\ reading\ and\ executing\ one\ command TRANSLATION\ MISSING
" menutrans Treat\ unset\ variables\ as\ an\ error\ when\ substituting TRANSLATION\ MISSING
" menutrans Print\ shell\ input\ lines\ as\ they\ are\ read TRANSLATION\ MISSING
" menutrans Print\ commands\ and\ their\ arguments\ as\ they\ are\ executed TRANSLATION\ MISSING
" }}}

" termdebug menu
" termdebug menuitems {{{1
" From termdebug.vim
" menutrans Set\ breakpoint TRANSLATION\ MISSING
" menutrans Clear\ breakpoint TRANSLATION\ MISSING
" menutrans Run\ until TRANSLATION\ MISSING
" menutrans Evaluate TRANSLATION\ MISSING
" menutrans WinBar TRANSLATION\ MISSING
" menutrans Step TRANSLATION\ MISSING
" menutrans Next TRANSLATION\ MISSING
" menutrans Finish TRANSLATION\ MISSING
" menutrans Cont TRANSLATION\ MISSING
" menutrans Stop TRANSLATION\ MISSING
" }}}

" debchangelog menu
" debchangelog menuitems {{{1
" From debchangelog.vim
" menutrans &Changelog TRANSLATION\ MISSING
" menutrans &New\ Version TRANSLATION\ MISSING
" menutrans &Add\ Entry TRANSLATION\ MISSING
" menutrans &Close\ Bug TRANSLATION\ MISSING
" menutrans Set\ &Distribution TRANSLATION\ MISSING
" menutrans &unstable TRANSLATION\ MISSING
" menutrans &frozen TRANSLATION\ MISSING
" menutrans &stable TRANSLATION\ MISSING
" menutrans frozen\ unstable TRANSLATION\ MISSING
" menutrans stable\ unstable TRANSLATION\ MISSING
" menutrans stable\ frozen TRANSLATION\ MISSING
" menutrans stable\ frozen\ unstable TRANSLATION\ MISSING
" menutrans Set\ &Urgency TRANSLATION\ MISSING
" menutrans &low TRANSLATION\ MISSING
" menutrans &medium TRANSLATION\ MISSING
" menutrans &high TRANSLATION\ MISSING
" menutrans U&nfinalise TRANSLATION\ MISSING
" menutrans &Finalise TRANSLATION\ MISSING
" }}}

" ada menu
" ada menuitems {{{1
" From ada.vim
" menutrans Tag TRANSLATION\ MISSING
" menutrans List TRANSLATION\ MISSING
" menutrans Jump TRANSLATION\ MISSING
" menutrans Create\ File TRANSLATION\ MISSING
" menutrans Create\ Dir TRANSLATION\ MISSING
" menutrans Highlight TRANSLATION\ MISSING
" menutrans Toggle\ Space\ Errors TRANSLATION\ MISSING
" menutrans Toggle\ Lines\ Errors TRANSLATION\ MISSING
" menutrans Toggle\ Rainbow\ Color TRANSLATION\ MISSING
" menutrans Toggle\ Standard\ Types TRANSLATION\ MISSING
" }}}

" gnat menu
" gnat menuitems {{{1
" From gnat.vim
" menutrans GNAT TRANSLATION\ MISSING
" menutrans Build TRANSLATION\ MISSING
" menutrans Pretty\ Print TRANSLATION\ MISSING
" menutrans Find TRANSLATION\ MISSING
" menutrans Set\ Projectfile\.\.\. TRANSLATION\ MISSING
" }}}

let &cpo = s:keepcpo
unlet s:keepcpo

" vim: set ts=4 sw=4 noet fdm=marker fdc=4 :
