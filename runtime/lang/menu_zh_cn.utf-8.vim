" Menu Translations:    Simplified Chinese
" Maintainer:           Ada (Haowen) Yu <me@yuhaowen.com>
" Previous Maintainer:  Shun Bai <baishunde@gmail.com>, Yuheng Xie <elephant@linux.net.cn>
" Last Change:          2022 July 6
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
menutrans &Help 帮助(&H)
" Help menuitems and dialog {{{1
menutrans &Overview<Tab><F1> 概述(&O)<Tab><F1>
menutrans &User\ Manual 用户手册(&U)
menutrans &How-to\ Links 如何使用(&H)
menutrans &Find\.\.\. 查找(&F)\.\.\.
menutrans &Credits 致谢(&C)
menutrans Co&pying 版权(&P)
menutrans &Sponsor/Register 赞助/注册(&S)
menutrans O&rphans 拯救孤儿(&R)
menutrans &Version 版本(&V)
menutrans &About 关于(&A)

" fun! s:Helpfind()
if !exists("g:menutrans_help_dialog")
  let g:menutrans_help_dialog = "输入命令或单词以获得帮助:\n\n前缀 i_ 表示输入模式下的命令(如: i_CTRL-X)\n前缀 c_ 表示命令行下的编辑命令(如: c_<Del>)\n前缀 ' 表示选项名(如: 'shiftwidth')"
endif
" }}}

" File menu
menutrans &File 文件(&F)
" File menuitems {{{1
menutrans &Open\.\.\.<Tab>:e 打开(&O)\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp 在拆分窗口打开(&L)\.\.\.<Tab>:sp
menutrans Open\ Tab\.\.\.<Tab>:tabnew 在标签页打开\.\.\.<Tab>:tabnew
menutrans &New<Tab>:enew 新建(&N)<Tab>:enew
menutrans &Close<Tab>:close 关闭(&C)<Tab>:close
menutrans &Save<Tab>:w 保存(&S)<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav 另存为(&A)\.\.\.<Tab>:sav
menutrans Split\ &Diff\ With\.\.\. 拆分窗口以对比差异(Diff)(&D)\.\.\.
menutrans Split\ Patched\ &By\.\.\. 拆分窗口以进行修补(Patch)(&B)\.\.\.
menutrans &Print 打印(&P)
menutrans Sa&ve-Exit<Tab>:wqa 保存并退出(&V)<Tab>:wqa
menutrans E&xit<Tab>:qa 退出(&X)<Tab>:qa
" }}}

" Edit menu
menutrans &Edit 编辑(&E)
" Edit menuitems {{{1
menutrans &Undo<Tab>u 撤销(&U)<Tab>u
menutrans &Redo<Tab>^R 恢复(&R)<Tab>^R
menutrans Rep&eat<Tab>\. 重复(&E)<Tab>\.
menutrans Cu&t<Tab>"+x 剪切(&T)<Tab>"+x
menutrans &Copy<Tab>"+y 复制(&C)<Tab>"+y
menutrans &Paste<Tab>"+gP 粘贴(&P)<Tab>"+gP
menutrans Put\ &Before<Tab>[p 粘贴到光标前(&B)<Tab>[p
menutrans Put\ &After<Tab>]p 粘贴到光标后(&A)<Tab>]p
menutrans &Delete<Tab>x 删除(&D)<Tab>x
menutrans &Select\ All<Tab>ggVG 全选(&S)<Tab>ggVG
menutrans &Find\.\.\. 查找(&F)\.\.\.
menutrans Find\ and\ Rep&lace\.\.\. 查找和替换(&L)\.\.\.
menutrans &Find<Tab>/ 查找(&F)<Tab>/
menutrans Find\ and\ Rep&lace<Tab>:%s 查找和替换(&L)<Tab>:%s
menutrans Find\ and\ Rep&lace<Tab>:s 查找和替换(&L)<Tab>:s
menutrans Settings\ &Window 设置窗口(&W)
menutrans Startup\ &Settings 启动设置(&S)

" Edit/Global Settings
menutrans &Global\ Settings 全局设置(&G)
" Edit.Global Settings menuitems and dialogs {{{2
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls! 开/关高亮查找内容(&H)<Tab>:set\ hls!
menutrans Toggle\ &Ignoring\ Case<Tab>:set\ ic! 开/关忽略大小写(&I)<Tab>:set\ ic!
menutrans Toggle\ &Showing\ Matched\ Pairs<Tab>:set\ sm! 开/关显示括号匹配(&S)<Tab>:set\ sm!
menutrans &Context\ Lines 上下文行数(&C)
menutrans &Virtual\ Edit 虚拟编辑(&V)
" Edit.Global Settings.Virtual Edit menuitems {{{3
menutrans Never 从不
menutrans Block\ Selection 只在选定矩形块时
menutrans Insert\ Mode 只在插入模式时
menutrans Block\ and\ Insert 在选定矩形块和插入模式时
menutrans Always 始终
" }}}
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im! 开/关插入模式(&M)<Tab>:set\ im!
menutrans Toggle\ Vi\ C&ompatibility<Tab>:set\ cp! 开/关\ Vi\ 兼容性(&O)<Tab>:set\ cp!
menutrans Search\ &Path\.\.\. 搜索路径(&P)\.\.\.
menutrans Ta&g\ Files\.\.\. 标记文件(Tags)(&G)\.\.\.

" GUI options
menutrans Toggle\ &Toolbar 开/关工具栏(&T)
menutrans Toggle\ &Bottom\ Scrollbar 开/关底部滚动条(&B)
menutrans Toggle\ &Left\ Scrollbar 开/关左侧滚动条(&L)
menutrans Toggle\ &Right\ Scrollbar 开/关右侧滚动条(&R)

" fun! s:SearchP()
if !exists("g:menutrans_path_dialog")
  let g:menutrans_path_dialog = "输入搜索路径。\n用逗号分隔目录名。"
endif

" fun! s:TagFiles()
if !exists("g:menutrans_tags_dialog")
  let g:menutrans_tags_dialog = "输入标记文件(Tags)名称。\n用逗号分隔文件名。"
endif
" }}}

" Edit/File Settings
menutrans F&ile\ Settings 文件设置(&I)
" Edit.File Settings menuitems and dialogs {{{2
" Boolean options
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu! 开/关行号(&N)<Tab>:set\ nu!
menutrans Toggle\ Relati&ve\ Line\ Numbering<Tab>:set\ rnu! 开/关相对行号(&V)<Tab>:set\ rnu!
menutrans Toggle\ &List\ Mode<Tab>:set\ list! 开/关列表模式(&L)<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrapping<Tab>:set\ wrap! 开/关换行(&W)<Tab>:set\ wrap!
menutrans Toggle\ W&rapping\ at\ Word<Tab>:set\ lbr! 开/关词尾换行(&R)<Tab>:set\ lbr!
menutrans Toggle\ Tab\ &Expanding<Tab>:set\ et! 开/关制表符扩展(&E)<Tab>:set\ et!
menutrans Toggle\ &Auto\ Indenting<Tab>:set\ ai! 开/关自动缩进(&A)<Tab>:set\ ai!
menutrans Toggle\ &C-Style\ Indenting<Tab>:set\ cin! 开/关\ C\ 语言式缩进(&C)<Tab>:set\ cin!

" other options
menutrans &Shiftwidth 缩进宽度(&S)
menutrans Soft\ &Tabstop 软制表位宽度(Soft\ Tabstop)(&T)
menutrans Te&xt\ Width\.\.\. 文本宽度(&X)\.\.\.
menutrans &File\ Format\.\.\. 文件格式(&F)\.\.\.

" fun! s:TextWidth()
if !exists("g:menutrans_textwidth_dialog")
  let g:menutrans_textwidth_dialog = "输入文本宽度(每行最大字符数，0 表示禁用):"
endif

" fun! s:FileFormat()
if !exists("g:menutrans_fileformat_dialog")
  let g:menutrans_fileformat_dialog = "选择文件的保存格式:"
endif
if !exists("g:menutrans_fileformat_choices")
  let g:menutrans_fileformat_choices = "&Unix\n&Dos\n&Mac\n取消(&C)"
endif
" }}}
menutrans Show\ C&olor\ Schemes\ in\ Menu 在菜单中显示配色方案(&O)
menutrans C&olor\ Scheme 配色方案(&O)
" menutrans None TRANSLATION\ MISSING
menutrans Show\ &Keymaps\ in\ Menu 在菜单中显示键盘映射(&K)
menutrans &Keymap 键盘映射(&K)
menutrans Select\ Fo&nt\.\.\. 选择字体(&N)\.\.\.
" }}}

" Programming menu
menutrans &Tools 工具(&T)
" Tools menuitems {{{1
menutrans &Jump\ to\ This\ Tag<Tab>g^] 跳转到这个标记(Tag)(&J)<Tab>g^]
menutrans Jump\ &Back<Tab>^T 跳转回(&B)<Tab>^T
menutrans Build\ &Tags\ File 生成标记文件(Tags)(&T)

" Tools.Spelling Menu
menutrans &Spelling 拼写检查(&S)
" Tools.Spelling menuitems and dialog {{{2
menutrans &Spell\ Check\ On 打开拼写检查(&S)
menutrans Spell\ Check\ &Off 关闭拼写检查(&O)
menutrans To\ &Next\ Error<Tab>]s 上一个错误(&N)<Tab>]s
menutrans To\ &Previous\ Error<Tab>[s 下一个错误(&P)<Tab>[s
menutrans Suggest\ &Corrections<Tab>z= 更正建议(&C)<Tab>z=
menutrans &Repeat\ Correction<Tab>:spellrepall 更正全部同类错误(&R)<Tab>:spellrepall
menutrans Set\ Language\ to\ "en" 设置语言为\ "en"
menutrans Set\ Language\ to\ "en_au" 设置语言为\ "en_au"
menutrans Set\ Language\ to\ "en_ca" 设置语言为\ "en_ca"
menutrans Set\ Language\ to\ "en_gb" 设置语言为\ "en_gb"
menutrans Set\ Language\ to\ "en_nz" 设置语言为\ "en_nz"
menutrans Set\ Language\ to\ "en_us" 设置语言为\ "en_us"
menutrans &Find\ More\ Languages 查找更多语言(&F)

" func! s:SpellLang()
if !exists("g:menutrans_set_lang_to")
  let g:menutrans_set_lang_to = "设置语言为"
endif
" }}}

" Tools.Fold Menu
menutrans &Folding 折叠(&F)
" Tools.Fold menuitems {{{2
" open close folds
menutrans &Enable/Disable\ Folds<Tab>zi 启用/禁用折叠(&E)<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv 展开光标所在行(&V)<Tab>zv
menutrans Vie&w\ Cursor\ Line\ Only<Tab>zMzx 只展开光标所在行(&W)<Tab>zMzx
menutrans C&lose\ More\ Folds<Tab>zm 折叠一级(&L)<Tab>zm
menutrans &Close\ All\ Folds<Tab>zM 折叠全部(&C)<Tab>zM
menutrans O&pen\ More\ Folds<Tab>zr 展开一级(&P)<Tab>zr
menutrans &Open\ All\ Folds<Tab>zR 展开全部(&O)<Tab>zR
" fold method
menutrans Fold\ Met&hod 折叠方式(&H)
" Tools.Fold.Fold Method menuitems {{{3
menutrans M&anual 手动(&A)
menutrans I&ndent 缩进(&N)
menutrans E&xpression 表达式(&X)
menutrans S&yntax 语法(&Y)
menutrans &Diff 差异(Diff)(&D)
menutrans Ma&rker 记号(Marker)(&R)
" }}}
" create and delete folds
menutrans Create\ &Fold<Tab>zf 创建折叠(&F)<Tab>zf
menutrans &Delete\ Fold<Tab>zd 删除折叠(&D)<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD 删除全部折叠(&A)<Tab>zD
" moving around in folds
menutrans Fold\ Col&umn\ Width 折叠操作栏宽度(&W)
" }}}

" Tools.Diff Menu
menutrans &Diff 差异(Diff)(&D)
" Tools.Diff menuitems {{{2
menutrans &Update 刷新(&U)
menutrans &Get\ Block 采用对侧文本块(&G)
menutrans &Put\ Block 采用本侧文本块(&P)
" }}}

menutrans &Make<Tab>:make 生成(Make)(&M)<Tab>:make
menutrans &List\ Errors<Tab>:cl 列出错误(&L)<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl! 列出消息(&I)<Tab>:cl!
menutrans &Next\ Error<Tab>:cn 下一个错误(&N)<Tab>:cn
menutrans &Previous\ Error<Tab>:cp 上一个错误(&P)<Tab>:cp
menutrans &Older\ List<Tab>:cold 较旧的错误列表(&O)<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew 较新的错误列表(&E)<Tab>:cnew
menutrans Error\ &Window 错误窗口(&W)
" Tools.Error Window menuitems {{{2
menutrans &Update<Tab>:cwin 刷新(&U)<Tab>:cwin
menutrans &Open<Tab>:copen 打开(&O)<Tab>:copen
menutrans &Close<Tab>:cclose 关闭(&C)<Tab>:cclose
" }}}
menutrans Show\ Compiler\ Se&ttings\ in\ Menu 在菜单中显示编译器设置(&T)
menutrans Se&t\ Compiler 设置编译器(&T)
menutrans &Convert\ to\ HEX<Tab>:%!xxd 转换成十六进制(&C)<Tab>:%!xxd
menutrans Conve&rt\ Back<Tab>:%!xxd\ -r 转换回(&R)<Tab>:%!xxd\ -r
" }}}

" Buffer menu
menutrans &Buffers 缓冲区(&B)
" menutrans Dummy TRANSLATION\ MISSING
" Buffer menuitems and dialog {{{1
menutrans &Refresh\ Menu 刷新本菜单(&R)
menutrans &Delete 删除(&D)
menutrans &Alternate 切换(&A)
menutrans &Next 下一个(&N)
menutrans &Previous 上一个(&P)

" func! s:BMMunge(fname, bnum)
if !exists("g:menutrans_no_file")
  let g:menutrans_no_file = "[无文件]"
endif
" }}}

" Window menu
menutrans &Window 窗口(&W)
" Window menuitems {{{1
menutrans &New<Tab>^Wn 新建(&N)<Tab>^Wn
menutrans S&plit<Tab>^Ws 拆分(&P)<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^ 拆分并显示缓冲区\ #(&L)<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv 垂直拆分(&V)<Tab>^Wv
menutrans Split\ File\ E&xplorer 拆分并打开文件浏览器(&X)
menutrans &Close<Tab>^Wc 关闭(&C)<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo 除此之外全部关闭(&O)<Tab>^Wo
menutrans Move\ &To 移动到(&T)
menutrans &Top<Tab>^WK 顶端(&T)<Tab>^WK
menutrans &Bottom<Tab>^WJ 底端(&B)<Tab>^WJ
menutrans &Left\ Side<Tab>^WH 左边(&L)<Tab>^WH
menutrans &Right\ Side<Tab>^WL 右边(&R)<Tab>^WL
menutrans Rotate\ &Up<Tab>^WR 向上轮换(&U)<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr 向下轮换(&D)<Tab>^Wr
menutrans &Equal\ Size<Tab>^W= 平均分布(&E)<Tab>^W=
menutrans &Max\ Height<Tab>^W_ 最大高度(&M)<Tab>^W
menutrans M&in\ Height<Tab>^W1_ 最小高度(&I)<Tab>^W1_
menutrans Max\ &Width<Tab>^W\| 最大宽度(&W)<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\| 最小宽度(&H)<Tab>^W1\|
" }}}

" The popup menu {{{1
menutrans &Undo 撤销(&U)
menutrans Cu&t 剪切(&T)
menutrans &Copy 复制(&C)
menutrans &Paste 粘贴(&P)
menutrans &Delete 删除(&D)
menutrans Select\ Blockwise 改为选定矩形块
menutrans Select\ &Word 选定单词(&W)
menutrans Select\ &Sentence 选定句(&S)
menutrans Select\ Pa&ragraph 选定段落(&R)
menutrans Select\ &Line 选定行(&L)
menutrans Select\ &Block 选定矩形块(&B)
menutrans Select\ &All 全选(&A)

" func! <SID>SpellPopup()
if !exists("g:menutrans_spell_change_ARG_to")
  let g:menutrans_spell_change_ARG_to = '将\ "%s"\ 更改为'
endif
if !exists("g:menutrans_spell_add_ARG_to_word_list")
  let g:menutrans_spell_add_ARG_to_word_list = '将\ "%s"\ 添加到词典'
endif
if !exists("g:menutrans_spell_ignore_ARG")
  let g:menutrans_spell_ignore_ARG = '忽略\ "%s"'
endif
" }}}

" The GUI toolbar {{{1
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    let did_toolbar_tmenu = 1
    tmenu ToolBar.Open 打开文件
    tmenu ToolBar.Save 保存当前文件
    tmenu ToolBar.SaveAll 全部保存
    tmenu ToolBar.Print 打印
    tmenu ToolBar.Undo 撤销
    tmenu ToolBar.Redo 恢复
    tmenu ToolBar.Cut 剪切到剪贴板
    tmenu ToolBar.Copy 复制到剪贴板
    tmenu ToolBar.Paste 从剪贴板粘贴
    if !has("gui_athena")
      tmenu ToolBar.Replace 查找和替换...
      tmenu ToolBar.FindNext 查找下一个
      tmenu ToolBar.FindPrev 查找上一个
    endif
    tmenu ToolBar.LoadSesn 加载会话
    tmenu ToolBar.SaveSesn 保存当前会话
    tmenu ToolBar.RunScript 运行 Vim 脚本
    tmenu ToolBar.Make 生成当前项目 (:make)
    tmenu ToolBar.RunCtags 在当前目录生成标记(Tags) (!ctags -R .)
    tmenu ToolBar.TagJump 跳转到光标所在标记(Tag)
    tmenu ToolBar.Help Vim 帮助
    tmenu ToolBar.FindHelp 在 Vim 帮助中查找
  endfun
endif
" }}}

" Syntax menu
menutrans &Syntax 语法(&S)
" Syntax menuitems {{{1
menutrans &Show\ File\ Types\ in\ Menu 在菜单中显示文件类型(&S)
menutrans &Off 关闭(&O)
menutrans &Manual 手动(&M)
menutrans A&utomatic 自动(&U)
menutrans On/Off\ for\ &This\ File 只对这个文件开/关(&T)
menutrans Co&lor\ Test 色彩测试(&L)
menutrans &Highlight\ Test 高亮测试(&H)
menutrans &Convert\ to\ HTML 转换成\ HTML(&C)

" From synmenu.vim
menutrans Set\ '&syntax'\ Only 只设置\ 'syntax'(&S)
menutrans Set\ '&filetype'\ Too 也设置\ 'filetype'(&F)
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
menutrans Oracle\ config Oracle\ 配置文件
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
menutrans Vim\ help\ file Vim\ 帮助文件
menutrans Vim\ script Vim\ 脚本
menutrans Viminfo\ file Vim信息文件
menutrans Virata\ config Virata\ 主配置文件
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
  menutrans Help<tab><F1> 帮助<tab><F1>
  menutrans Bookmarks 书签
  menutrans History 历史记录
  menutrans Go\ Up\ Directory<tab>- 向上一级<tab>-
  menutrans Apply\ Special\ Viewer<tab>x 用默认程序打开<tab>x
  menutrans Bookmarks\ and\ History 书签和历史记录
  " Netrw.Bookmarks and History menuitems {{{2
  menutrans Bookmark\ Current\ Directory<tab>mb 添加书签<tab>mb
  menutrans Bookmark\ Delete 移除书签
  menutrans Goto\ Prev\ Dir\ (History)<tab>u 后退(历史记录)<tab>u
  menutrans Goto\ Next\ Dir\ (History)<tab>U 前进(历史记录)<tab>U
  menutrans List<tab>qb 完整列表<tab>qb
  " }}}
  menutrans Browsing\ Control 控制
  " Netrw.Browsing Control menuitems {{{2
  menutrans Horizontal\ Split<tab>o 在拆分窗口打开<tab>o
  menutrans Vertical\ Split<tab>v 在垂直拆分窗口打开<tab>v
  menutrans New\ Tab<tab>t 在标签页打开<tab>t
  menutrans Preview<tab>p 预览<tab>p
  menutrans Edit\ File\ Hiding\ List<tab><ctrl-h> 编辑隐藏条件(Hiding\ List)<tab><ctrl-h>
  menutrans Edit\ Sorting\ Sequence<tab>S 编辑排序条件(Sorting\ Sequence)<tab>S
  menutrans Quick\ Hide/Unhide\ Dot\ Files<tab>gh 快速隐藏/显示以\.开头的文件<tab>gh
  menutrans Refresh\ Listing<tab><ctrl-l> 刷新<tab><ctrl-l>
  menutrans Settings/Options<tab>:NetrwSettings 设置/选项<tab>:NetrwSettings
  " }}}
  menutrans Delete\ File/Directory<tab>D 删除文件/目录<tab>D
  menutrans Edit\ File/Dir 编辑文件/目录
  " Netrw.Edit File menuitems {{{2
  menutrans Create\ New\ File<tab>% 新建文件<tab>%
  menutrans In\ Current\ Window<tab><cr> 在当前窗口<tab><cr>
  menutrans Preview\ File/Directory<tab>p 预览文件/目录<tab>p
  menutrans In\ Previous\ Window<tab>P 在上一个窗口<tab>P
  menutrans In\ New\ Window<tab>o 在新窗口<tab>o
  menutrans In\ New\ Tab<tab>t 在新标签页<tab>t
  menutrans In\ New\ Vertical\ Window<tab>v 在新垂直窗口<tab>v
  " }}}
  menutrans Explore 浏览
  " Netrw.Explore menuitems {{{2
  menutrans Directory\ Name 指定目录名
  menutrans Filenames\ Matching\ Pattern\ (curdir\ only)<tab>:Explore\ */ 匹配指定文件名模式(当前目录)<tab>:Explore\ */
  menutrans Filenames\ Matching\ Pattern\ (+subdirs)<tab>:Explore\ **/ 匹配指定文件名模式(含子目录)<tab>:Explore\ **/
  menutrans Files\ Containing\ String\ Pattern\ (curdir\ only)<tab>:Explore\ *// 内容包含指定字符串模式(当前目录)<tab>:Explore\ *//
  menutrans Files\ Containing\ String\ Pattern\ (+subdirs)<tab>:Explore\ **// 内容包含指定字符串模式(含子目录)<tab>:Explore\ **//
  menutrans Next\ Match<tab>:Nexplore 下一个匹配项<tab>:Nexplore
  menutrans Prev\ Match<tab>:Pexplore 上一个匹配项<tab>:Pexplore
  " }}}
  menutrans Make\ Subdirectory<tab>d 新建子目录<tab>d
  menutrans Marked\ Files 选定的(Marked)文件
  " Netrw.Marked Files menuitems {{{2
  menutrans Mark\ File<tab>mf 选定(Mark)/取消<tab>mf
  menutrans Mark\ Files\ by\ Regexp<tab>mr 用正则表达式(Regexp)选定<tab>mr
  menutrans Hide-Show-List\ Control<tab>a 隐藏/显示<tab>a
  menutrans Copy\ To\ Target<tab>mc 复制到目标<tab>mc
  menutrans Delete<tab>D 删除<tab>D
  menutrans Diff<tab>md 差异(Diff)<tab>md
  menutrans Edit<tab>me 编辑<tab>me
  menutrans Exe\ Cmd<tab>mx 作为参数运行命令<tab>mx
  menutrans Move\ To\ Target<tab>mm 移动到目标<tab>mm
  menutrans Obtain<tab>O 获取<tab>O
  menutrans Print<tab>mp 打印<tab>mp
  menutrans Replace<tab>R 替换<tab>R
  menutrans Set\ Target<tab>mt 设置目标<tab>mt
  menutrans Tag<tab>mT 生成标记文件(Tags)<tab>mT
  menutrans Zip/Unzip/Compress/Uncompress<tab>mz 压缩/解压缩<tab>mz
  " }}}
  menutrans Obtain\ File<tab>O 获取文件<tab>O
  menutrans Style 显示风格
  " Netrw.Style menuitems {{{2
  menutrans Listing 列表形式
  " Netrw.Style.Listing menuitems {{{3
  menutrans thin<tab>i 紧凑<thin)<tab>i
  menutrans long<tab>i 详细(long)<tab>i
  menutrans wide<tab>i 多列(wide)<tab>i
  menutrans tree<tab>i 树状(tree)<tab>i
  " }}}
  menutrans Normal-Hide-Show 显示/隐藏
  " Netrw.Style.Normal-Hide_show menuitems {{{3
  menutrans Show\ All<tab>a 显示全部
  menutrans Normal<tab>a 不显示隐藏文件
  menutrans Hidden\ Only<tab>a 只显示隐藏文件
  " }}}
  menutrans Reverse\ Sorting\ Order<tab>r 升序/降序<tab>r
  menutrans Sorting\ Method 排序方式
  " Netrw.Style.Sorting Method menuitems {{{3
  menutrans Name<tab>s 文件名<tab>s
  menutrans Time<tab>s 修改时间<tab>s
  menutrans Size<tab>s 大小<tab>s
  menutrans Exten<tab>s 扩展名<tab>s
  " }}}
  " }}}
  menutrans Rename\ File/Directory<tab>R 重命名文件/目录<tab>R
  menutrans Set\ Current\ Directory<tab>c 设置\ Vim\ 工作目录<tab>c
  menutrans Targets 目标
" endif
" }}}

" Shellmenu menu
" Shellmenu menuitems {{{1
" From shellmenu.vim
menutrans Stmts 语句
" menutrans trap TRANSLATION\ MISSING
menutrans Test 测试
menutrans existence 存在
menutrans existence\ -\ file 存在\ -\ 文件
menutrans existence\ -\ file\ (not\ empty) 存在\ -\ 文件(非空)
menutrans existence\ -\ directory 存在目录
menutrans existence\ -\ executable 存在可执行
menutrans existence\ -\ readable 存在可读
menutrans existence\ -\ writable 存在可写
menutrans String\ is\ empty 字符串为空
menutrans String\ is\ not\ empty 字符串非空
menutrans Strings\ is\ equal 字符串值相等
menutrans Strings\ is\ not\ equal 字符串值不相等
menutrans Values\ is\ greater\ than 值大于
menutrans Values\ is\ greater\ equal 值大于等于
menutrans Values\ is\ equal 值相等
menutrans Values\ is\ not\ equal 值不相等
menutrans Values\ is\ less\ than 值小于
menutrans Values\ is\ less\ equal 值小于等于
menutrans ParmSub 参数替换
menutrans Substitute\ word\ if\ parm\ not\ set 如果参数没设置就替换该词
menutrans Set\ parm\ to\ word\ if\ not\ set 参数未设置就设为该词
menutrans Substitute\ word\ if\ parm\ set\ else\ nothing 如果参数设置就替换该词，否则什么都不做
menutrans If\ parm\ not\ set\ print\ word\ and\ exit 如果参数没有设置就打印该词并退出
menutrans SpShVars Shell\ 特殊变量
menutrans Number\ of\ positional\ parameters 位置参数的数目
menutrans All\ positional\ parameters\ (quoted\ spaces) 所有位置参数(quoted\ spaces)
menutrans All\ positional\ parameters\ (unquoted\ spaces) 所有位置参数(unquoted\ spaces)
menutrans Flags\ set 设置标志
menutrans Return\ code\ of\ last\ command 返回前一条命令的代码
menutrans Process\ number\ of\ this\ shell shell\ 中进程号
menutrans Process\ number\ of\ last\ background\ command 前一条背景命令的进程号
menutrans Environ 环境变量
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
menutrans mark\ modified\ or\ modified\ variables 标记更改或未更改的变量
menutrans exit\ when\ command\ returns\ non-zero\ exit\ code 当命令返回非零代码时退出
menutrans Disable\ file\ name\ generation 禁用文件名生成
menutrans remember\ function\ commands 记住函数命令
menutrans All\ keyword\ arguments\ are\ placed\ in\ the\ environment 所有关键字参数被放到环境里
menutrans Read\ commands\ but\ do\ not\ execute\ them 读命令但是不要执行
menutrans Exit\ after\ reading\ and\ executing\ one\ command 读并执行命令之后退出
menutrans Treat\ unset\ variables\ as\ an\ error\ when\ substituting 替换时把恢复命令视为错误
menutrans Print\ shell\ input\ lines\ as\ they\ are\ read 读\ shell\ 输入行的时候打印
menutrans Print\ commands\ and\ their\ arguments\ as\ they\ are\ executed 被执行时打印命令和参数
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
