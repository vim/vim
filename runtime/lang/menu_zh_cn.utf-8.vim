" Menu Translations:	Simplified Chinese (UTF-8)
" Translated By:	Wang Jun <junw@turbolinux.com.cn>
" Last Change:		Tue Sep  4 11:26:52 CST 2001

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1

scriptencoding utf-8

" Help menu
menutrans &Help			帮助(&H)
menutrans &Overview<Tab><F1>	预览(&O)<Tab><F1>
menutrans &User\ Manual		用户手册(&U)
menutrans &GUI			图形界面(&G)
menutrans &How-to\ links	HOWTO文档\.\.\.(&H)
menutrans &Credits		作者(&C)
menutrans Co&pying		版权(&P)
menutrans &Version		版本(&V)
menutrans &About		关于\ Vim(&A)

" File menu
menutrans &File				文件(&F)
menutrans &Open\.\.\.<Tab>:e		打开(&O)\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	分割窗口并打开(&L)<Tab>:sp
menutrans &New<Tab>:enew		新建(&N)<Tab>:enew
menutrans &Close<Tab>:close		关闭(&C)<Tab>:close
menutrans &Save<Tab>:w			保存(&S)<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	另存为(&A)\.\.\.<Tab>:sav
menutrans Split\ &Diff\ with\.\.\.	分割比较(&Diff)\.\.\.
menutrans Split\ Patched\ &By\.\.\.	分割打补丁(&Patch)\.\.\.
menutrans &Print			打印(&P)
menutrans Sa&ve-Exit<Tab>:wqa		保存并退出(&V)<Tab>:wqa
menutrans E&xit<Tab>:qa			退出(&X)<Tab>:qa

" Edit menu
menutrans &Edit				编辑(&E)
menutrans &Undo<Tab>u			恢复(&U)<Tab>u
menutrans &Redo<Tab>^R			重做(&R)<Tab>^R
menutrans Rep&eat<Tab>\.		重复上次动作(&E)<Tab>\.
menutrans Cu&t<Tab>"+x			剪切(&T)<Tab>"+x
menutrans &Copy<Tab>"+y			复制(&C)<Tab>"+y
menutrans &Paste<Tab>"+gP		粘帖(&P)<Tab>"+gP
menutrans Put\ &Before<Tab>[p		贴到光标前(&B)<Tab>[p
menutrans Put\ &After<Tab>]p		贴到光标后(&A)<Tab>]p
menutrans &Delete<Tab>x			删除(&D)<Tab>x
menutrans &Select\ all<Tab>ggVG		全选(&S)<Tab>ggvG
menutrans &Find\.\.\.			查找(&F)\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.	查找替换(&L)\.\.\.
menutrans Settings\ &Window		设定窗口(&W)
menutrans &Global\ Settings		全局设定(&G)

" Build boolean options
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!	开/关增量查找模式<Tab>:set\ hls!
menutrans Toggle\ &Ignore-case<Tab>:set\ ic!		开/关忽略大小写模式<Tab>:set\ ic!
menutrans Toggle\ &Showmatch<Tab>:set\ sm!		开/关匹配显示<Tab>:set sm!
menutrans &Context\ lines			上下文行数(&C)

menutrans &Virtual\ Edit			可视化编辑模式(&V)
menutrans Never					从不
menutrans Block\ Selection			块选择
menutrans Insert\ mode				插入模式
menutrans Block\ and\ Insert			块选择与插入模式
menutrans Always				所有模式

menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!	开/关插入模式<Tab>:set\ im!

menutrans Search\ &Path\.\.\.			查找路径\.\.\.(&P)

menutrans Ta&g\ Files\.\.\.			标签文件\.\.\.(&g)

" GUI options
menutrans Toggle\ &Toolbar			开/关工具条(&T)
menutrans Toggle\ &Bottom\ Scrollbar		开/关底部滚动条(&B)
menutrans Toggle\ &Left\ Scrollbar		开/关左端滚动条(&L)
menutrans Toggle\ &Right\ Scrollbar		开/关右端滚动条(&R)


" Edit/File Settings
menutrans F&ile\ Settings			文件设定(&i)

" Boolean options
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	开/关显示行号<Tab>:set\ nu!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		开/关显示Tab<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrap<Tab>:set\ wrap!		开/关自动折行<Tab>:set\ wrap!
menutrans Toggle\ W&rap\ at\ word<Tab>:set\ lbr!	开/关词尾折行<Tab>:set\ lbr!
menutrans Toggle\ &expand-tab<Tab>:set\ et!		开/关expand-tab<Tab>:set\ et!
menutrans Toggle\ &auto-indent<Tab>:set\ ai!		开/关auto-indent<Tab>:set\ ai!
menutrans Toggle\ &C-indenting<Tab>:set\ cin!		开/关C-indent<Tab>:set\ cin!


" other options
menutrans &Shiftwidth			缩排宽度(&S)
menutrans Soft\ &Tabstop		伪Tab宽度(&T)
menutrans Te&xt\ Width\.\.\.		页面宽度(&x)\.\.\.
menutrans &File\ Format\.\.\.		文件格式(&F)\.\.\.

menutrans C&olor\ Scheme		调色板(&o)
menutrans Select\ Fo&nt\.\.\.		选择字体(&n)\.\.\.


" Programming menu
menutrans &Tools			工具(&T)
menutrans &Jump\ to\ this\ tag<Tab>g^]	检索光标处的标签关键字(tag)(&J)<Tab>g^]
menutrans Jump\ &back<Tab>^T		跳回检索前的位置(&B)<Tab>^T
menutrans Build\ &Tags\ File		建立标签索引文件\ Tags(&T)
menutrans &Folding			Folding设定(&F)
menutrans &Diff				比较(&D)
menutrans &Make<Tab>:make		执行\ Make(&M)<Tab>:make
menutrans &List\ Errors<Tab>:cl		列出编译错误(&E)<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!	列出所有信息(&I)<Tab>:cl!
menutrans &Next\ Error<Tab>:cn		下一个编译错误处(&N)<Tab>:cn
menutrans &Previous\ Error<Tab>:cp	上一个编译错误处(&P)<Tab>:cp
menutrans &Older\ List<Tab>:cold	旧错误列表(&O)<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew	新错误列表(&E)<Tab>:cnew
menutrans Error\ &Window		错误信息窗口(&W)
menutrans &Set\ Compiler		设置编译器(&S)
menutrans &Convert\ to\ HEX<Tab>:%!xxd	转换成16进制<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!xxd\ -r	从16进制转换回文字<Tab>:%!xxd\ -r

" Tools.Fold Menu
menutrans &Enable/Disable\ folds<Tab>zi		使用/不使用Folding(&E)<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv		查看此行(&V)<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx	只查看此行(&W)<Tab>zMzx
menutrans C&lose\ more\ folds<Tab>zm		关闭Folds(&L)<Tab>zm
menutrans &Close\ all\ folds<Tab>zM		关闭所有Folds(&C)<Tab>zM
menutrans O&pen\ more\ folds<Tab>zr		展开Folds(&P)<Tab>zr
menutrans &Open\ all\ folds<Tab>zR		展开所有Folds(&O)<Tab>zR
" fold method
menutrans Fold\ Met&hod				Fold方式(&H)
menutrans Create\ &Fold<Tab>zf			建立Fold(&F)<Tab>zf
menutrans &Delete\ Fold<Tab>zd			删除Fold(&D)<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD		删除所有Fold(&A)<Tab>zD
" moving around in folds
menutrans Fold\ column\ &width			设定Fold栏宽(&W)

" Tools.Diff Menu
menutrans &Update		更新(&U)
menutrans &Get\ Block		取得不同部分(&G)
menutrans &Put\ Block		将不同部分应用到对方(&P)


" Names for buffer menu.
menutrans &Buffers		缓冲区(&B)
menutrans &Refresh\ menu	更新(&R)
menutrans &Delete		删除(&D)
menutrans &Alternate		修改(&L)
menutrans &Next			下一个(&N)
menutrans &Previous		前一个(&P)

" Window menu
menutrans &Window			窗口(&W)
menutrans &New<Tab>^Wn			新建窗口(&N)<Tab>^Wn
menutrans S&plit<Tab>^Ws		分割窗口(&P)<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^	分割到#(&L)<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv	垂直分割(&V)<Tab>^Wv
menutrans Split\ File\ E&xplorer	文件浏览器式分割(&X)
menutrans &Close<Tab>^Wc		关闭窗口(&C)<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo	关闭其它窗口(&O)<Tab>^Wo
menutrans Move\ &To			移动到(&T)
menutrans &Top<Tab>^WK			顶端(&T)<Tab>^WK
menutrans &Bottom<Tab>^WJ		底端(&B)<Tab>^WJ
menutrans &Left\ side<Tab>^WH		左边(&L)<Tab>^WH
menutrans &Right\ side<Tab>^WL		右边(&R)<Tab>^WL
" menutrans Ne&xt<Tab>^Ww		下一个(&X)<Tab>^Ww
" menutrans P&revious<Tab>^WW		上一个(&R)<Tab>^WW
menutrans Rotate\ &Up<Tab>^WR		上移窗口(&U)<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		下移窗口(&D)<Tab>^Wr
menutrans &Equal\ Size<Tab>^W=		所有窗口等高(&E)<Tab>^W=
menutrans &Max\ Height<Tab>^W_		最大高度(&M)<Tab>^W
menutrans M&in\ Height<Tab>^W1_		最小高度(&i)<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|		最大宽度(&W)<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|		最小宽度(&h)<Tab>^W1\|
"
" The popup menu
menutrans &Undo			撤销(&U)
menutrans Cu&t			剪切(&T)
menutrans &Copy			复制(&C)
menutrans &Paste		粘帖(&P)
menutrans &Delete		删除(&D)
menutrans Select\ Blockwise	Blockwise选择
menutrans Select\ &Word		选择单词(&W)
menutrans Select\ &Line		选择行(&L)
menutrans Select\ &Block	选择块(&B)
menutrans Select\ &All		全选(&A)
"
" The GUI toolbar
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		打开文件
    tmenu ToolBar.Save		保存当前文件
    tmenu ToolBar.SaveAll	保存全部文件
    tmenu ToolBar.Print		打印
    tmenu ToolBar.Undo		撤销上次修改
    tmenu ToolBar.Redo		重做上次撤销的动作
    tmenu ToolBar.Cut		剪切至剪贴板
    tmenu ToolBar.Copy		复制到剪贴板
    tmenu ToolBar.Paste		由剪贴板粘帖
    tmenu ToolBar.Find		查找...
    tmenu ToolBar.FindNext	查找下一个
    tmenu ToolBar.FindPrev	查找上一个
    tmenu ToolBar.Replace	替换...
    tmenu ToolBar.LoadSesn	加载会话
    tmenu ToolBar.SaveSesn	保存当前的会话
    tmenu ToolBar.RunScript	运行Vim脚本
    tmenu ToolBar.Make		执行 Make
    tmenu ToolBar.Shell		打开一个命令窗口
    tmenu ToolBar.RunCtags	执行 ctags
    tmenu ToolBar.TagJump	跳到当前光标位置的标签
    tmenu ToolBar.Help		Vim 帮助
    tmenu ToolBar.FindHelp	查找 Vim 帮助
  endfun
endif

" Syntax menu
menutrans &Syntax		语法(&S)
menutrans Set\ '&syntax'\ only	只设定\ 'syntax'(&s)
menutrans Set\ '&filetype'\ too	也设定\ 'filetype'(&f)
menutrans &Off			关闭(&O)
menutrans &Manual		手动设定(&M)
menutrans A&utomatic		自动设定(&U)
menutrans on/off\ for\ &This\ file	只对这个文件打开/关闭(&T)
menutrans Co&lor\ test		色彩显示测试(&L)
menutrans &Highlight\ test	语法效果测试(&H)
menutrans &Convert\ to\ HTML	转换成\ HTML\ 格式(&C)
