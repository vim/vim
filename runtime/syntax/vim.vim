" Vim syntax file
" Language:	Vim 8.0 script
" Maintainer:	Charles E. Campbell <NdrOchipS@PcampbellAfamily.Mbiz>
" Last Change:	March 11, 2020
" Version:	8.0-30
" URL:	http://www.drchip.org/astronaut/vim/index.html#SYNTAX_VIM
" Automatically generated keyword lists: {{{1

" Quit when a syntax file was already loaded {{{2
if exists("b:current_syntax")
  finish
endif
let s:keepcpo= &cpo
set cpo&vim

" vimTodo: contains common special-notices for comments {{{2
" Use the vimCommentGroup cluster to add your own.
syn keyword vimTodo contained	COMBAK	FIXME	TODO	XXX
syn cluster vimCommentGroup	contains=vimTodo,@Spell

" regular vim commands {{{2
syn keyword vimCommand contained	a ar[gs] argl[ocal] ba[ll] bm[odified] breaka[dd] bun[load] cabc[lear] cal[l] cc cf[ile] changes cla[st] cnf[ile] comc[lear] cp[revious] cstag debugg[reedy] deletl dep diffpu[t] dl dr[op] ec em[enu] ene[w] files fini[sh] folddoc[losed] gr[ep] helpc[lose] his[tory] il[ist] isp[lit] keepa l[ist] laf[ter] lbel[ow] lcscope lfdo lgrepa[dd] lma lo[adview] lop[en] lua m[ove] mes mkvie[w] nbc[lose] noh[lsearch] ol[dfiles] pa[ckadd] po[p] prof[ile] pta[g] ptr[ewind] py3f[ile] pythonx quita[ll] redraws[tatus] rew[ind] rubyf[ile] sIg sa[rgument] sba[ll] sbr[ewind] scl scscope sfir[st] sgl sic sin sm[ap] snoreme spelld[ump] spellw[rong] srg st[op] stj[ump] sunmenu syn tN[ext] tabd[o] tabm[ove] tabr[ewind] tch[dir] tf[irst] tlmenu tm[enu] to[pleft] tu[nmenu] undol[ist] up[date] vi[sual] vmapc[lear] wa[ll] winp[os] wundo xme xr[estore]
syn keyword vimCommand contained	ab arga[dd] argu[ment] bad[d] bn[ext] breakd[el] bw[ipeout] cabo[ve] cat[ch] ccl[ose] cfdo chd[ir] cle[arjumps] cnor comp[iler] cpf[ile] cun delc[ommand] deletp di[splay] diffs[plit] dli[st] ds[earch] echoe[rr] en[dif] eval filet fir[st] foldo[pen] grepa[dd] helpf[ind] i imapc[lear] iuna[bbrev] keepalt la[st] lan[guage] lbo[ttom] ld[o] lfir[st] lh[elpgrep] lmak[e] loadk lp[revious] luado ma[rk] messages mod[e] nbs[tart] nor omapc[lear] packl[oadall] popu[p] profd[el] ptf[irst] pts[elect] py3f[ile] pyx r[ead] redrawt[abline] ri[ght] rundo sIl sal[l] sbf[irst] sc scp se[t] sg sgn sie sip sme snoremenu spelli[nfo] spr[evious] sri sta[g] stopi[nsert] sus[pend] sync ta[g] tabe[dit] tabn[ext] tabs tcld[o] th[row] tln tma[p] tp[revious] tunma[p] unh[ide] v vie[w] vne[w] wh[ile] wn[ext] wv[iminfo] xmenu xunme
syn keyword vimCommand contained	abc[lear] argd[elete] as[cii] bd[elete] bo[tright] breakl[ist] cN[ext] cad[dbuffer] cb[uffer] cd cfir[st] che[ckpath] clo[se] co[py] con[tinue] cq[uit] cuna[bbrev] delel delf[unction] dif[fupdate] difft[his] do dsp[lit] echom[sg] endf[unction] ex filetype fix[del] for gui helpg[rep] ia in j[oin] keepj[umps] lab[ove] lat lc[d] le[ft] lg[etfile] lhi[story] lmapc[lear] loadkeymap lpf[ile] luafile mak[e] mk[exrc] mz[scheme] new nore on[ly] pc[lose] pp[op] promptf[ind] ptj[ump] pu[t] py[thon] pyxdo rec[over] reg[isters] rightb[elow] rv[iminfo] sIn san[dbox] sbl[ast] scI scr[iptnames] setf[iletype] sgI sgp sig sir smenu so[urce] spellr[are] sr srl star[tinsert] sts[elect] sv[iew] syncbind tab tabf[ind] tabnew tags tclf[ile] tj[ump] tlnoremenu tmapc[lear] tr[ewind] u[ndo] unl ve[rsion] vim[grep] vs[plit] win[size] wp[revious] x[it] xnoreme xunmenu
syn keyword vimCommand contained	abo[veleft] argdo au bel[owright] bp[revious] bro[wse] cNf[ile] cadde[xpr] cbe[fore] cdo cg[etfile] checkt[ime] cmapc[lear] col[der] conf[irm] cr[ewind] cw[indow] delep dell diffg[et] dig[raphs] doau e[dit] echon endfo[r] exi[t] filt[er] fo[ld] fu[nction] gvim helpt[ags] iabc[lear] inor ju[mps] keepp[atterns] lad[dexpr] later lch[dir] lefta[bove] lgetb[uffer] ll lne[xt] loc[kmarks] lr[ewind] lv[imgrep] marks mks[ession] mzf[ile] nmapc[lear] nos[wapfile] opt[ions] pe[rl] pre[serve] promptr[epl] ptl[ast] pw[d] pydo pyxfile red[o] res[ize] ru[ntime] sI sIp sav[eas] sbm[odified] sce scripte[ncoding] setg[lobal] sgc sgr sign sl[eep] smile sor[t] spellr[epall] srI srn startg[replace] sun[hide] sw[apname] syntime tabN[ext] tabfir[st] tabo[nly] tc[l] te[aroff] tl[ast] tlu tn[ext] try una[bbreviate] unlo[ckvar] verb[ose] vimgrepa[dd] wN[ext] winc[md] wq xa[ll] xnoremenu xwininfo
syn keyword vimCommand contained	addd arge[dit] bN[ext] bf[irst] br[ewind] bufdo c[hange] caddf[ile] cbel[ow] ce[nter] cgetb[uffer] chi[story] cn[ext] colo[rscheme] cons[t] cs d[elete] deletel delm[arks] diffo[ff] dir doaut ea el[se] endt[ry] exu[sage] fin[d] foldc[lose] g h[elp] hi if intro k lN[ext] laddb[uffer] lb[uffer] lcl[ose] lex[pr] lgete[xpr] lla[st] lnew[er] lockv[ar] ls lvimgrepa[dd] mat[ch] mksp[ell] n[ext] noa nu[mber] ownsyntax ped[it] prev[ious] ps[earch] ptn[ext] py3 pyf[ile] q[uit] redi[r] ret[ab] rub[y] sIc sIr sbN[ext] sbn[ext] scg scriptv[ersion] setl[ocal] sge sh[ell] sil[ent] sla[st] sn[ext] sp[lit] spellr[rare] src srp startr[eplace] sunme sy t tabc[lose] tabl[ast] tabp[revious] tcd ter[minal] tlm tlunmenu tno[remap] ts[elect] undoj[oin] uns[ilent] vert[ical] viu[sage] w[rite] windo wqa[ll] xmapc[lear] xprop y[ank]
syn keyword vimCommand contained	al[l] argg[lobal] b[uffer] bl[ast] brea[k] buffers ca caf[ter] cbo[ttom] cex[pr] cgete[xpr] cl[ist] cnew[er] com cope[n] cscope debug deletep delp diffp[atch] dj[ump] dp earlier elsei[f] endw[hile] f[ile] fina[lly] foldd[oopen] go[to] ha[rdcopy] hid[e] ij[ump] is[earch] kee[pmarks] lNf[ile] laddf[ile] lbe[fore] lcs lf[ile] lgr[ep] lli[st] lnf[ile] lol[der] lt[ag] lw[indow] menut[ranslate] mkv[imrc] nb[key] noautocmd o[pen] p[rint] perld[o] pro ptN[ext] ptp[revious] py3do python3 qa[ll] redr[aw] retu[rn] rubyd[o] sIe sN[ext] sb[uffer] sbp[revious] sci scs sf[ind] sgi si sim[alt] sm[agic] sno[magic] spe[llgood] spellu[ndo] sre[wind]
syn match   vimCommand contained	"\<z[-+^.=]\=\>"
syn keyword vimCommand contained	def endd[ef] disa[ssemble] vim9[script] imp[ort] exp[ort]
syn keyword vimStdPlugin contained	Arguments Break Cfilter Clear Continue DiffOrig Evaluate Finish Gdb Lfilter Man N[ext] Over P[rint] Program Run S Source Step Stop Termdebug TermdebugCommand TOhtml Winbar XMLent XMLns

" vimOptions are caught only when contained in a vimSet {{{2
syn keyword vimOption contained	acd ambw arshape background ballooneval bex bl brk buftype cf cinkeys cmdwinheight com completeslash cpoptions cscoperelative csre cursorcolumn delcombine digraph eadirection emo equalprg expandtab fdls fex fileignorecase fml foldlevel formatexpr gcr go guifontset helpheight history hlsearch imaf ims includeexpr infercase iskeyword keywordprg laststatus lispwords lrm magic maxfuncdepth menuitems mm modifiable mousemodel mzq numberwidth opfunc patchexpr pfn pp printfont pumwidth pythonthreehome redrawtime ri rs sb scroll sect sft shellredir shiftwidth showmatch signcolumn smarttab sp spf srr startofline suffixes switchbuf ta tagfunc tbi term termwintype tgc titlelen toolbariconsize ttimeout ttymouse twt undofile varsofttabstop verbosefile viminfofile wak weirdinvert wig wildoptions winheight wm wrapscan
syn keyword vimOption contained	ai anti autochdir backspace balloonevalterm bexpr bo browsedir casemap cfu cino cmp comments concealcursor cpp cscopetag cst cursorline dex dip eb emoji errorbells exrc fdm ff filetype fmr foldlevelstart formatlistpat gd gp guifontwide helplang hk ic imak imsearch incsearch insertmode isp km lazyredraw list ls makeef maxmapdepth mfd mmd modified mouses mzquantum nuw osfiletype patchmode ph preserveindent printheader pvh pyx regexpengine rightleft rtp sbo scrollbind sections sh shellslash shm showmode siso smc spc spl ss statusline suffixesadd sws tabline taglength tbidi termbidi terse tgst titleold top ttimeoutlen ttyscroll tx undolevels vartabstop vfile virtualedit warn wfh wildchar wim winminheight wmh write
syn keyword vimOption contained	akm antialias autoindent backup balloonexpr bg bomb bs cb ch cinoptions cms commentstring conceallevel cpt cscopetagorder csto cursorlineopt dg dir ed enc errorfile fcl fdn ffs fillchars fo foldmarker formatoptions gdefault grepformat guiheadroom hf hkmap icon imc imsf inde is isprint kmp lbr listchars lsp makeencoding maxmem mh mmp more mouseshape mzschemedll odev pa path pheader previewheight printmbcharset pvp pyxversion relativenumber rightleftcmd ru sbr scrollfocus secure shcf shelltemp shortmess showtabline sj smd spell splitbelow ssl stl sw sxe tabpagemax tagrelative tbis termencoding textauto thesaurus titlestring tpm ttm ttytype uc undoreload vb vi visualbell wb wfw wildcharm winaltkeys winminwidth wmnu writeany
syn keyword vimOption contained	al ar autoread backupcopy bdir bh breakat bsdir cc charconvert cinw co compatible confirm crb cscopeverbose csverb cwh dict directory edcompatible encoding errorformat fcs fdo fic fixendofline foldclose foldmethod formatprg gfm grepprg guioptions hh hkmapp iconstring imcmdline imst indentexpr isf joinspaces kp lcs lm luadll makeprg maxmempattern mis mmt mouse mouset mzschemegcdll oft packpath pdev pi previewpopup printmbfont pvw qe remap rl rubydll sc scrolljump sel shell shelltype shortname shq slm sn spellcapcheck splitright ssop stmp swapfile sxq tabstop tags tbs termguicolors textmode tildeop tl tr tty tw udf updatecount vbs viewdir vop wc wh wildignore wincolor winptydll wmw writebackup
syn keyword vimOption contained	aleph arab autowrite backupdir bdlay bin breakindent bsk ccv ci cinwords cocu complete copyindent cryptmethod csl cuc debug dictionary display ef endofline esckeys fdc fdt fileencoding fixeol foldcolumn foldminlines fp gfn gtl guipty hi hkp ignorecase imd imstatusfunc indentkeys isfname js langmap linebreak lmap lw mat maxmemtot mkspellmem mod mousef mousetime nf ofu para penc pm previewwindow printoptions pw quoteescape renderoptions rlc ruf scb scrolloff selection shellcmdflag shellxescape showbreak si sm so spellfile spr st sts swapsync syn tag tagstack tc termwinkey textwidth timeout tm ts ttybuiltin twk udir updatetime vdir viewoptions vsts wcm whichwrap wildignorecase window winwidth wop writedelay
syn keyword vimOption contained	allowrevins arabic autowriteall backupext belloff binary breakindentopt bt cd cin clipboard cole completefunc cot cscopepathcomp cspc cul deco diff dy efm eol et fde fen fileencodings fk foldenable foldnestmax fs gfs gtt guitablabel hid hl im imdisable imstyle indk isi key langmenu lines lnr lz matchpairs mco ml modeline mousefocus mp nrformats omnifunc paragraphs perldll pmbcs printdevice prompt pythondll rdt report rnu ruler scf scrollopt selectmode shellpipe shellxquote showcmd sidescroll smartcase softtabstop spelllang sps sta su swb synmaxcol tagbsearch tal tcldll termwinscroll tf timeoutlen to tsl ttyfast tws ul ur ve vif vts wcr wi wildmenu winfixheight wiv wrap ws
syn keyword vimOption contained	altkeymap arabicshape aw backupskip beval bk bri bufhidden cdpath cindent cm colorcolumn completeopt cp cscopeprg csprg culopt def diffexpr ea ei ep eventignore fdi fenc fileformat fkmap foldexpr foldopen fsync gfw guicursor guitabtooltip hidden hlg imactivatefunc imi inc inex isident keymap langnoremap linespace loadplugins ma matchtime mef mle modelineexpr mousehide mps nu opendevice paste pex pmbfn printencoding pt pythonhome re restorescreen ro rulerformat scl scs sessionoptions shellquote shiftround showfulltag sidescrolloff smartindent sol spellsuggest sr stal sua swf syntax tagcase tb tenc termwinsize tfu title toolbar tsr ttym twsl undodir ut verbose viminfo wa wd wic wildmode winfixwidth wiw wrapmargin ww
syn keyword vimOption contained	ambiwidth ari awa balloondelay bevalterm bkc briopt buflisted cedit cink cmdheight columns completepopup cpo cscopequickfix csqf cursorbind define diffopt ead ek equalalways ex fdl fencs fileformats flp foldignore foldtext ft ghr guifont helpfile highlight hls imactivatekey iminsert include inf isk keymodel langremap lisp lpl macatsui maxcombine menc mls modelines mousem msm number operatorfunc pastetoggle pexpr popt printexpr pumheight pythonthreedll readonly revins rop runtimepath scr

" vimOptions: These are the turn-off setting variants {{{2
syn keyword vimOption contained	noacd noallowrevins noantialias noarabic noarshape noautoread noaw noballooneval nobevalterm nobk nobreakindent nocf nocindent nocopyindent nocscoperelative nocsre nocuc nocursorcolumn nodelcombine nodigraph noed noemo noeol noesckeys noexpandtab nofic nofixeol nofoldenable nogd nohid nohkmap nohls noicon noimc noimdisable noinfercase nojoinspaces nolangremap nolinebreak nolnr nolrm nomacatsui noml nomod nomodelineexpr nomodified nomousef nomousehide nonumber noopendevice nopi nopreviewwindow nopvw norelativenumber norestorescreen nori norl noro noru nosb noscb noscrollbind noscs nosft noshelltemp noshortname noshowfulltag noshowmode nosm nosmartindent nosmd nosol nosplitbelow nospr nossl nostartofline noswapfile nota notagrelative notbi notbs noterse notextmode notgst notimeout noto notr nottybuiltin notx noundofile novisualbell nowarn noweirdinvert nowfw nowildignorecase nowinfixheight nowiv nowrap nowrite nowritebackup
syn keyword vimOption contained	noai noaltkeymap noar noarabicshape noautochdir noautowrite noawa noballoonevalterm nobin nobl nobri noci nocompatible nocp nocscopetag nocst nocul nocursorline nodg noea noedcompatible noemoji noequalalways noet noexrc nofileignorecase nofk nofs nogdefault nohidden nohkmapp nohlsearch noignorecase noimcmdline noincsearch noinsertmode nojs nolazyredraw nolisp noloadplugins nolz nomagic nomle nomodeline nomodifiable nomore nomousefocus nonu noodev nopaste nopreserveindent noprompt noreadonly noremap norevins norightleft nornu nors noruler nosc noscf noscrollfocus nosecure noshellslash noshiftround noshowcmd noshowmatch nosi nosmartcase nosmarttab nosn nospell nosplitright nosr nosta nostmp noswf notagbsearch notagstack notbidi notermbidi notextauto notf notildeop notitle notop nottimeout nottyfast noudf novb nowa nowb nowfh nowic nowildmenu nowinfixwidth nowmnu nowrapscan nowriteany nows
syn keyword vimOption contained	noakm noanti noarab noari noautoindent noautowriteall nobackup nobeval nobinary nobomb nobuflisted nocin noconfirm nocrb nocscopeverbose nocsverb nocursorbind nodeco nodiff noeb noek noendofline noerrorbells noex nofen nofixendofline nofkmap nofsync noguipty nohk nohkp noic noim noimd noinf nois nolangnoremap nolbr nolist nolpl noma nomh

" vimOptions: These are the invertible variants {{{2
syn keyword vimOption contained	invacd invallowrevins invantialias invarabic invarshape invautoread invaw invballooneval invbevalterm invbk invbreakindent invcf invcindent invcopyindent invcscoperelative invcsre invcuc invcursorcolumn invdelcombine invdigraph inved invemo inveol invesckeys invexpandtab invfic invfixeol invfoldenable invgd invhid invhkmap invhls invicon invimc invimdisable invinfercase invjoinspaces invlangremap invlinebreak invlnr invlrm invmacatsui invml invmod invmodelineexpr invmodified invmousef invmousehide invnumber invopendevice invpi invpreviewwindow invpvw invrelativenumber invrestorescreen invri invrl invro invru invsb invscb invscrollbind invscs invsft invshelltemp invshortname invshowfulltag invshowmode invsm invsmartindent invsmd invsol invsplitbelow invspr invssl invstartofline invswapfile invta invtagrelative invtbi invtbs invterse invtextmode invtgst invtimeout invto invtr invttybuiltin invtx invundofile invvisualbell invwarn invweirdinvert invwfw invwildignorecase invwinfixheight invwiv invwrap invwrite invwritebackup
syn keyword vimOption contained	invai invaltkeymap invar invarabicshape invautochdir invautowrite invawa invballoonevalterm invbin invbl invbri invci invcompatible invcp invcscopetag invcst invcul invcursorline invdg invea invedcompatible invemoji invequalalways invet invexrc invfileignorecase invfk invfs invgdefault invhidden invhkmapp invhlsearch invignorecase invimcmdline invincsearch invinsertmode invjs invlazyredraw invlisp invloadplugins invlz invmagic invmle invmodeline invmodifiable invmore invmousefocus invnu invodev invpaste invpreserveindent invprompt invreadonly invremap invrevins invrightleft invrnu invrs invruler invsc invscf invscrollfocus invsecure invshellslash invshiftround invshowcmd invshowmatch invsi invsmartcase invsmarttab invsn invspell invsplitright invsr invsta invstmp invswf invtagbsearch invtagstack invtbidi invtermbidi invtextauto invtf invtildeop invtitle invtop invttimeout invttyfast invudf invvb invwa invwb invwfh invwic invwildmenu invwinfixwidth invwmnu invwrapscan invwriteany invws
syn keyword vimOption contained	invakm invanti invarab invari invautoindent invautowriteall invbackup invbeval invbinary invbomb invbuflisted invcin invconfirm invcrb invcscopeverbose invcsverb invcursorbind invdeco invdiff inveb invek invendofline inverrorbells invex invfen invfixendofline invfkmap invfsync invguipty invhk invhkp invic invim invimd invinf invis invlangnoremap invlbr invlist invlpl invma invmh

" termcap codes (which can also be set) {{{2
syn keyword vimOption contained	t_8b t_AB t_al t_bc t_BE t_ce t_cl t_Co t_Cs t_CV t_db t_DL t_EI t_F2 t_F4 t_F6 t_F8 t_fs t_IE t_k1 t_k2 t_K3 t_K4 t_K5 t_K6 t_K7 t_K8 t_K9 t_kb t_KB t_kd t_KD t_ke t_KE t_KF t_KG t_kh t_KH t_kI t_KI t_KJ t_KK t_kl t_KL t_kN t_kP t_kr t_ks t_ku t_le t_mb t_md t_me t_mr t_ms t_nd t_op t_PE t_PS t_RB t_RC t_RF t_Ri t_RI t_RS t_RT t_RV t_Sb t_SC t_se t_Sf t_SH t_Si t_SI t_so t_sr t_SR t_ST t_te t_Te t_TE t_ti t_TI t_ts t_Ts t_u7 t_ue t_us t_ut t_vb t_ve t_vi t_vs t_VS t_WP t_WS t_xn t_xs t_ZH t_ZR
syn keyword vimOption contained	t_8f t_AF t_AL t_BD t_cd t_Ce t_cm t_cs t_CS t_da t_dl t_EC t_F1 t_F3 t_F5 t_F7 t_F9 t_GP t_IS t_K1 t_k3 t_k4 t_k5 t_k6 t_k7 t_k8 t_k9 t_KA t_kB t_KC t_kD
syn match   vimOption contained	"t_%1"
syn match   vimOption contained	"t_#2"
syn match   vimOption contained	"t_#4"
syn match   vimOption contained	"t_@7"
syn match   vimOption contained	"t_*7"
syn match   vimOption contained	"t_&8"
syn match   vimOption contained	"t_%i"
syn match   vimOption contained	"t_k;"

" unsupported settings: some were supported by vi but don't do anything in vim {{{2
" others have been dropped along with msdos support
syn keyword vimErrSetting contained	bioskey biosk conskey consk autoprint beautify flash graphic hardtabs mesg novice open op optimize redraw slow slowopen sourceany w300 w1200 w9600 hardtabs ht nobioskey nobiosk noconskey noconsk noautoprint nobeautify noflash nographic nohardtabs nomesg nonovice noopen noop nooptimize noredraw noslow noslowopen nosourceany now300 now1200 now9600 w1200 w300 w9600

" AutoCmd Events {{{2
syn case ignore
syn keyword vimAutoEvent contained	BufAdd BufDelete BufFilePost BufHidden BufNew BufRead BufReadPost BufUnload BufWinEnter BufWinLeave BufWipeout BufWrite BufWriteCmd BufWritePost BufWritePre CmdlineChanged CmdlineEnter CmdlineLeave CmdUndefined CmdwinEnter CmdwinLeave ColorScheme ColorSchemePre CompleteChanged CompleteDone CompleteDonePre CursorHold CursorHoldI CursorMoved CursorMovedI DiffUpdated DirChanged EncodingChanged ExitPre FileAppendCmd FileAppendPost FileAppendPre FileChangedRO FileChangedShell FileChangedShellPost FileEncoding FileReadCmd FileReadPost FileReadPre FileType FileWriteCmd FileWritePost FileWritePre FilterReadPost FilterReadPre FilterWritePost FilterWritePre FocusGained FocusLost FuncUndefined GUIEnter GUIFailed InsertChange InsertCharPre InsertEnter InsertLeave MenuPopup OptionSet QuickFixCmdPost QuickFixCmdPre QuitPre RemoteReply SafeState SafeStateAgain SessionLoadPost ShellCmdPost ShellFilterPost SourceCmd SourcePost SourcePre SpellFileMissing StdinReadPost StdinReadPre SwapExists Syntax TabClosed TabEnter TabLeave TabNew TermChanged TerminalOpen TerminalWinOpen TermResponse TextChanged TextChangedI TextChangedP TextYankPost User VimEnter VimLeave VimLeavePre VimResized WinEnter WinLeave WinNew
syn keyword vimAutoEvent contained	BufCreate BufEnter BufFilePre BufLeave BufNewFile BufReadCmd BufReadPre

" Highlight commonly used Groupnames {{{2
syn keyword vimGroup contained	Comment Constant String Character Number Boolean Float Identifier Function Statement Conditional Repeat Label Operator Keyword Exception PreProc Include Define Macro PreCondit Type StorageClass Structure Typedef Special SpecialChar Tag Delimiter SpecialComment Debug Underlined Ignore Error Todo

" Default highlighting groups {{{2
syn keyword vimHLGroup contained	ColorColumn Cursor CursorColumn CursorIM CursorLine CursorLineNr DiffAdd DiffChange DiffDelete DiffText Directory EndOfBuffer ErrorMsg FoldColumn Folded IncSearch LineNr LineNrAbove LineNrBelow MatchParen Menu ModeMsg MoreMsg NonText Normal Pmenu PmenuSbar PmenuSel PmenuThumb Question QuickFixLine Scrollbar Search SignColumn SpecialKey SpellBad SpellCap SpellLocal SpellRare StatusLine StatusLineNC StatusLineTerm TabLine TabLineFill TabLineSel Terminal Title Tooltip VertSplit Visual VisualNOS WarningMsg WildMenu
syn match vimHLGroup contained	"Conceal"
syn case match

" Function Names {{{2
syn keyword vimFuncName contained	abs appendbufline asin assert_fails assert_notmatch balloon_gettext bufadd bufname byteidx char2nr ch_evalexpr ch_log ch_readraw cindent complete_check cosh deepcopy diff_hlID eval exists feedkeys findfile fnamemodify foldtextresult get getchar getcmdtype getenv getftype getmatches getreg gettagstack getwinvar has_key histget iconv inputlist interrupt isnan job_start js_encode libcall list2str log mapcheck matchdelete max nextnonblank popup_atcursor popup_dialog popup_getoptions popup_notification prevnonblank prop_add prop_type_add pum_getpos rand reg_recording remote_foreground remove round screencol searchdecl serverlist setenv setpos settagstack sign_define sign_placelist sin sound_playevent split str2list strftime strpart submatch synID systemlist taglist term_dumpload term_getcursor term_getstatus term_sendkeys term_setsize test_autochdir test_getvalue test_null_dict test_null_string test_scrollbar test_unknown timer_start toupper type values winbufnr win_findbuf winheight winline winsaveview winwidth
syn keyword vimFuncName contained	acos argc assert_beeps assert_false assert_report balloon_show bufexists bufnr byteidxcomp ch_canread ch_evalraw ch_logfile ch_sendexpr clearmatches complete_info count delete echoraw eventhandler exp filereadable float2nr foldclosed foreground getbufinfo getcharmod getcmdwintype getfontname getimstatus getmousepos getregtype getwininfo glob haslocaldir histnr indent inputrestore invert items job_status json_decode libcallnr listener_add log10 match matchend min nr2char popup_beval popup_filter_menu popup_getpos popup_setoptions printf prop_clear prop_type_change pumvisible range reltime remote_peek rename rubyeval screenpos searchpair setbufline setfperm setqflist setwinvar sign_getdefined sign_undefine sinh sound_playfile sqrt str2nr strgetchar strptime substitute synIDattr tabpagebuflist tan term_dumpwrite term_getjob term_gettitle term_setansicolors term_start test_feedinput test_ignore_error test_null_job test_option_not_set test_setmouse test_void timer_stop tr undofile virtcol wincol win_getid win_id2tabwin winnr win_screenpos wordcount
syn keyword vimFuncName contained	add argidx assert_equal assert_inrange assert_true balloon_split buflisted bufwinid call ch_close ch_getbufnr ch_open ch_sendraw col confirm cscope_connection deletebufline empty executable expand filewritable floor foldclosedend funcref getbufline getcharsearch getcompletion getfperm getjumplist getpid gettabinfo getwinpos glob2regpat hasmapto hlexists index inputsave isdirectory job_getchannel job_stop json_encode line listener_flush luaeval matchadd matchlist mkdir or popup_clear popup_filter_yesno popup_hide popup_settext prompt_setcallback prop_find prop_type_delete py3eval readdir reltimefloat remote_read repeat screenattr screenrow searchpairpos setbufvar setline setreg sha256 sign_getplaced sign_unplace sort sound_stop srand strcharpart stridx strridx swapinfo synIDtrans tabpagenr tanh term_getaltscreen term_getline term_gettty term_setapi term_wait test_garbagecollect_now test_null_blob test_null_list test_override test_settime timer_info timer_stopall trim undotree visualmode windowsversion win_gettype win_id2win winrestcmd win_splitmove writefile
syn keyword vimFuncName contained	and arglistid assert_equalfile assert_match atan browse bufload bufwinnr ceil ch_close_in ch_getjob ch_read ch_setoptions complete copy cursor did_filetype environ execute expandcmd filter fmod foldlevel function getbufvar getcmdline getcurpos getfsize getline getpos gettabvar getwinposx globpath histadd hlID input inputsecret isinf job_info join keys line2byte listener_remove map matchaddpos matchstr mode pathshorten popup_close popup_findinfo popup_menu popup_show prompt_setinterrupt prop_list prop_type_get pyeval readfile reltimestr remote_send resolve screenchar screenstring searchpos setcharsearch setloclist settabvar shellescape sign_jump sign_unplacelist sound_clear spellbadword state strchars string strtrans swapname synstack tabpagewinnr tempname term_getansicolors term_getscrolled term_list term_setkill test_alloc_fail test_garbagecollect_soon test_null_channel test_null_partial test_refcount test_srand_seed timer_pause tolower trunc uniq wildmenumode win_execute win_gotoid winlayout winrestview win_type xor
syn keyword vimFuncName contained	append argv assert_exception assert_notequal atan2 browsedir bufloaded byte2line changenr chdir ch_info ch_readblob ch_status complete_add cos debugbreak diff_filler escape exepath extend finddir fnameescape foldtext garbagecollect getchangelist getcmdpos getcwd getftime getloclist getqflist gettabwinvar getwinposy has histdel hostname inputdialog insert islocked job_setoptions js_decode len lispindent localtime maparg matcharg matchstrpos mzeval perleval popup_create popup_findpreview popup_move pow prompt_setprompt prop_remove prop_type_list pyxeval reg_executing remote_expr remote_startserver reverse screenchars search server2client setcmdpos setmatches settabwinvar shiftwidth sign_place simplify soundfold spellsuggest str2float strdisplaywidth strlen strwidth synconcealed system tagfiles term_dumpdiff term_getattr term_getsize term_scrape term_setrestore

"--- syntax here and above generated by mkvimvim ---
" Special Vim Highlighting (not automatic) {{{1

" Set up folding commands {{{2
if exists("g:vimsyn_folding") && g:vimsyn_folding =~# '[afhlmpPrt]'
 if g:vimsyn_folding =~# 'a'
  com! -nargs=* VimFolda <args> fold
 else
  com! -nargs=* VimFolda <args>
 endif
 if g:vimsyn_folding =~# 'f'
  com! -nargs=* VimFoldf <args> fold
 else
  com! -nargs=* VimFoldf <args>
 endif
 if g:vimsyn_folding =~# 'h'
  com! -nargs=* VimFoldh <args> fold
 else
  com! -nargs=* VimFoldh <args>
 endif
 if g:vimsyn_folding =~# 'l'
  com! -nargs=* VimFoldl <args> fold
 else
  com! -nargs=* VimFoldl <args>
 endif
 if g:vimsyn_folding =~# 'm'
  com! -nargs=* VimFoldm <args> fold
 else
  com! -nargs=* VimFoldm <args>
 endif
 if g:vimsyn_folding =~# 'p'
  com! -nargs=* VimFoldp <args> fold
 else
  com! -nargs=* VimFoldp <args>
 endif
 if g:vimsyn_folding =~# 'P'
  com! -nargs=* VimFoldP <args> fold
 else
  com! -nargs=* VimFoldP <args>
 endif
 if g:vimsyn_folding =~# 'r'
  com! -nargs=* VimFoldr <args> fold
 else
  com! -nargs=* VimFoldr <args>
 endif
 if g:vimsyn_folding =~# 't'
  com! -nargs=* VimFoldt <args> fold
 else
  com! -nargs=* VimFoldt <args>
 endif
else
 com! -nargs=*	VimFolda	<args>
 com! -nargs=*	VimFoldf	<args>
 com! -nargs=*	VimFoldh	<args>
 com! -nargs=*	VimFoldl	<args>
 com! -nargs=*	VimFoldm	<args>
 com! -nargs=*	VimFoldp	<args>
 com! -nargs=*	VimFoldP	<args>
 com! -nargs=*	VimFoldr	<args>
 com! -nargs=*	VimFoldt	<args>
endif

" commands not picked up by the generator (due to non-standard format) {{{2
syn keyword vimCommand contained	py3

" Deprecated variable options {{{2
if exists("g:vim_minlines")
 let g:vimsyn_minlines= g:vim_minlines
endif
if exists("g:vim_maxlines")
 let g:vimsyn_maxlines= g:vim_maxlines
endif
if exists("g:vimsyntax_noerror")
 let g:vimsyn_noerror= g:vimsyntax_noerror
endif

" Variable options {{{2
if exists("g:vim_maxlines")
 let s:vimsyn_maxlines= g:vim_maxlines
else
 let s:vimsyn_maxlines= 60
endif

" Numbers {{{2
" =======
syn match vimNumber	"\<\d\+\%(\.\d\+\%([eE][+-]\=\d\+\)\=\)\=" skipwhite nextgroup=vimGlobal,vimSubst,vimCommand,vimComment
syn match vimNumber	"-\d\+\%(\.\d\+\%([eE][+-]\=\d\+\)\=\)\="  skipwhite nextgroup=vimGlobal,vimSubst,vimCommand,vimComment
syn match vimNumber	"\<0[xX]\x\+"		       skipwhite nextgroup=vimGlobal,vimSubst,vimCommand,vimComment
syn match vimNumber	"\%(^\|\A\)\zs#\x\{6}"             	       skipwhite nextgroup=vimGlobal,vimSubst,vimCommand,vimComment

" All vimCommands are contained by vimIsCommand. {{{2
syn match vimCmdSep	"[:|]\+"	skipwhite nextgroup=vimAddress,vimAutoCmd,vimEcho,vimIsCommand,vimExtCmd,vimFilter,vimLet,vimMap,vimMark,vimSet,vimSyntax,vimUserCmd
syn match vimIsCommand	"\<\h\w*\>"	contains=vimCommand
syn match vimVar	      contained	"\<\h[a-zA-Z0-9#_]*\>"
syn match vimVar		"\<[bwglstav]:\h[a-zA-Z0-9#_]*\>"
syn match vimVar	      	"\s\zs&\a\+\>"
syn match vimFBVar      contained   "\<[bwglstav]:\h[a-zA-Z0-9#_]*\>"
syn keyword vimCommand  contained	in

" Insertions And Appends: insert append {{{2
" =======================
syn region vimInsert	matchgroup=vimCommand start="^[: \t]*\(\d\+\(,\d\+\)\=\)\=a\%[ppend]$"		matchgroup=vimCommand end="^\.$""
syn region vimInsert	matchgroup=vimCommand start="^[: \t]*\(\d\+\(,\d\+\)\=\)\=c\%[hange]$"		matchgroup=vimCommand end="^\.$""
syn region vimInsert	matchgroup=vimCommand start="^[: \t]*\(\d\+\(,\d\+\)\=\)\=i\%[nsert]$"		matchgroup=vimCommand end="^\.$""
syn region vimInsert	matchgroup=vimCommand start="^[: \t]*\(\d\+\(,\d\+\)\=\)\=starti\%[nsert]$"	matchgroup=vimCommand end="^\.$""

" Behave! {{{2
" =======
syn match   vimBehave	"\<be\%[have]\>" skipwhite nextgroup=vimBehaveModel,vimBehaveError
syn keyword vimBehaveModel contained	mswin	xterm
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_nobehaveerror")
 syn match   vimBehaveError contained	"[^ ]\+"
endif

" Filetypes {{{2
" =========
syn match   vimFiletype	"\<filet\%[ype]\(\s\+\I\i*\)*"	skipwhite contains=vimFTCmd,vimFTOption,vimFTError
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_vimFTError")
 syn match   vimFTError  contained	"\I\i*"
endif
syn keyword vimFTCmd    contained	filet[ype]
syn keyword vimFTOption contained	detect indent off on plugin

" Augroup : vimAugroupError removed because long augroups caused sync'ing problems. {{{2
" ======= : Trade-off: Increasing synclines with slower editing vs augroup END error checking.
syn cluster vimAugroupList	contains=vimAugroup,vimIsCommand,vimUserCmd,vimExecute,vimNotFunc,vimFuncName,vimFunction,vimFunctionError,vimLineComment,vimNotFunc,vimMap,vimSpecFile,vimOper,vimNumber,vimOperParen,vimComment,vimString,vimSubst,vimMark,vimRegister,vimAddress,vimFilter,vimCmplxRepeat,vimComment,vimLet,vimSet,vimAutoCmd,vimRegion,vimSynLine,vimNotation,vimCtrlChar,vimFuncVar,vimContinue,vimSetEqual,vimOption
if exists("g:vimsyn_folding") && g:vimsyn_folding =~# 'a'
 syn region  vimAugroup	fold matchgroup=vimAugroupKey start="\<aug\%[roup]\>\ze\s\+\K\k*" end="\<aug\%[roup]\>\ze\s\+[eE][nN][dD]\>"	contains=vimAutoCmd,@vimAugroupList
else
 syn region  vimAugroup	matchgroup=vimAugroupKey start="\<aug\%[roup]\>\ze\s\+\K\k*" end="\<aug\%[roup]\>\ze\s\+[eE][nN][dD]\>"		contains=vimAutoCmd,@vimAugroupList
endif
syn match   vimAugroup	"aug\%[roup]!"	contains=vimAugroupKey
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_noaugrouperror")
 syn match   vimAugroupError	"\<aug\%[roup]\>\s\+[eE][nN][dD]\>"
endif
syn keyword vimAugroupKey contained	aug[roup]

" Operators: {{{2
" =========
syn cluster	vimOperGroup	contains=vimEnvvar,vimFunc,vimFuncVar,vimOper,vimOperParen,vimNumber,vimString,vimRegister,vimContinue
syn match	vimOper	"\%#=1\(==\|!=\|>=\|<=\|=\~\|!\~\|>\|<\|=\)[?#]\{0,2}"	skipwhite nextgroup=vimString,vimSpecFile
syn match	vimOper	"\(\<is\|\<isnot\)[?#]\{0,2}\>"			skipwhite nextgroup=vimString,vimSpecFile
syn match	vimOper	"||\|&&\|[-+.!]"				skipwhite nextgroup=vimString,vimSpecFile
syn region	vimOperParen 	matchgroup=vimParenSep	start="(" end=")" contains=@vimOperGroup
syn region	vimOperParen	matchgroup=vimSep		start="{" end="}" contains=@vimOperGroup nextgroup=vimVar,vimFuncVar
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_noopererror")
 syn match	vimOperError	")"
endif

" Functions : Tag is provided for those who wish to highlight tagged functions {{{2
" =========
syn cluster	vimFuncList	contains=vimCommand,vimFunctionError,vimFuncKey,Tag,vimFuncSID
syn cluster	vimFuncBodyList	contains=vimAbb,vimAddress,vimAugroupKey,vimAutoCmd,vimCmplxRepeat,vimComment,vimContinue,vimCtrlChar,vimEcho,vimEchoHL,vimExecute,vimIsCommand,vimFBVar,vimFunc,vimFunction,vimFuncVar,vimGlobal,vimHighlight,vimIsCommand,vimLet,vimLetHereDoc,vimLineComment,vimMap,vimMark,vimNorm,vimNotation,vimNotFunc,vimNumber,vimOper,vimOperParen,vimRegion,vimRegister,vimSearch,vimSet,vimSpecFile,vimString,vimSubst,vimSynLine,vimUnmap,vimUserCommand
syn match	vimFunction	"\<\(fu\%[nction]\|def\)!\=\s\+\%(<[sS][iI][dD]>\|[sSgGbBwWtTlL]:\)\=\%(\i\|[#.]\|{.\{-1,}}\)*\ze\s*("	contains=@vimFuncList nextgroup=vimFuncBody

if exists("g:vimsyn_folding") && g:vimsyn_folding =~# 'f'
 syn region	vimFuncBody  contained	fold start="\ze\s*("	matchgroup=vimCommand end="\<\(endf\>\|endfu\%[nction]\>\|enddef\>\)"		contains=@vimFuncBodyList
else
 syn region	vimFuncBody  contained	start="\ze\s*("	matchgroup=vimCommand end="\<\(endf\>\|endfu\%[nction]\>\|enddef\>\)"		contains=@vimFuncBodyList
endif
syn match	vimFuncVar   contained	"a:\(\K\k*\|\d\+\)"
syn match	vimFuncSID   contained	"\c<sid>\|\<s:"
syn keyword	vimFuncKey   contained	fu[nction]
syn keyword	vimFuncKey   contained	def
syn match	vimFuncBlank contained	"\s\+"

syn keyword	vimPattern   contained	start	skip	end

" Special Filenames, Modifiers, Extension Removal: {{{2
" ===============================================
syn match	vimSpecFile	"<c\(word\|WORD\)>"	nextgroup=vimSpecFileMod,vimSubst
syn match	vimSpecFile	"<\([acs]file\|amatch\|abuf\)>"	nextgroup=vimSpecFileMod,vimSubst
syn match	vimSpecFile	"\s%[ \t:]"ms=s+1,me=e-1	nextgroup=vimSpecFileMod,vimSubst
syn match	vimSpecFile	"\s%$"ms=s+1	nextgroup=vimSpecFileMod,vimSubst
syn match	vimSpecFile	"\s%<"ms=s+1,me=e-1	nextgroup=vimSpecFileMod,vimSubst
syn match	vimSpecFile	"#\d\+\|[#%]<\>"	nextgroup=vimSpecFileMod,vimSubst
syn match	vimSpecFileMod	"\(:[phtre]\)\+"	contained

" User-Specified Commands: {{{2
" =======================
syn cluster	vimUserCmdList	contains=vimAddress,vimSyntax,vimHighlight,vimAutoCmd,vimCmplxRepeat,vimComment,vimCtrlChar,vimEscapeBrace,vimFunc,vimFuncName,vimFunction,vimFunctionError,vimIsCommand,vimMark,vimNotation,vimNumber,vimOper,vimRegion,vimRegister,vimLet,vimSet,vimSetEqual,vimSetString,vimSpecFile,vimString,vimSubst,vimSubstRep,vimSubstRange,vimSynLine
syn keyword	vimUserCommand	contained	com[mand]
syn match	vimUserCmd	"\<com\%[mand]!\=\>.*$"	contains=vimUserAttrb,vimUserAttrbError,vimUserCommand,@vimUserCmdList
syn match	vimUserAttrbError	contained	"-\a\+\ze\s"
syn match	vimUserAttrb	contained	"-nargs=[01*?+]"	contains=vimUserAttrbKey,vimOper
syn match	vimUserAttrb	contained	"-complete="		contains=vimUserAttrbKey,vimOper nextgroup=vimUserAttrbCmplt,vimUserCmdError
syn match	vimUserAttrb	contained	"-range\(=%\|=\d\+\)\="	contains=vimNumber,vimOper,vimUserAttrbKey
syn match	vimUserAttrb	contained	"-count\(=\d\+\)\="	contains=vimNumber,vimOper,vimUserAttrbKey
syn match	vimUserAttrb	contained	"-bang\>"		contains=vimOper,vimUserAttrbKey
syn match	vimUserAttrb	contained	"-bar\>"		contains=vimOper,vimUserAttrbKey
syn match	vimUserAttrb	contained	"-buffer\>"		contains=vimOper,vimUserAttrbKey
syn match	vimUserAttrb	contained	"-register\>"		contains=vimOper,vimUserAttrbKey
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_nousercmderror")
 syn match	vimUserCmdError	contained	"\S\+\>"
endif
syn case ignore
syn keyword	vimUserAttrbKey   contained	bar	ban[g]	cou[nt]	ra[nge] com[plete]	n[args]	re[gister]
syn keyword	vimUserAttrbCmplt contained	augroup buffer behave color command compiler cscope dir environment event expression file file_in_path filetype function help highlight history locale mapping menu option packadd shellcmd sign syntax syntime tag tag_listfiles user var
syn keyword	vimUserAttrbCmplt contained	custom customlist nextgroup=vimUserAttrbCmpltFunc,vimUserCmdError
syn match	vimUserAttrbCmpltFunc contained	",\%([sS]:\|<[sS][iI][dD]>\)\=\%(\h\w*\%(#\h\w*\)\+\|\h\w*\)"hs=s+1 nextgroup=vimUserCmdError

syn case match
syn match	vimUserAttrbCmplt contained	"custom,\u\w*"

" Lower Priority Comments: after some vim commands... {{{2
" =======================
syn match	vimComment	excludenl +\s"[^\-:.%#=*].*$+lc=1	contains=@vimCommentGroup,vimCommentString
syn match	vimComment	+\<endif\s\+".*$+lc=5	contains=@vimCommentGroup,vimCommentString
syn match	vimComment	+\<else\s\+".*$+lc=4	contains=@vimCommentGroup,vimCommentString
syn region	vimCommentString	contained oneline start='\S\s\+"'ms=e	end='"'

" Environment Variables: {{{2
" =====================
syn match	vimEnvvar	"\$\I\i*"
syn match	vimEnvvar	"\${\I\i*}"

" In-String Specials: {{{2
" Try to catch strings, if nothing else matches (therefore it must precede the others!)
"  vimEscapeBrace handles ["]  []"] (ie. "s don't terminate string inside [])
syn region	vimEscapeBrace	oneline   contained transparent start="[^\\]\(\\\\\)*\[\zs\^\=\]\=" skip="\\\\\|\\\]" end="]"me=e-1
syn match	vimPatSepErr	contained	"\\)"
syn match	vimPatSep	contained	"\\|"
syn region	vimPatSepZone	oneline   contained   matchgroup=vimPatSepZ start="\\%\=\ze(" skip="\\\\" end="\\)\|[^\\]['"]"	contains=@vimStringGroup
syn region	vimPatRegion	contained transparent matchgroup=vimPatSepR start="\\[z%]\=(" end="\\)"	contains=@vimSubstList oneline
syn match	vimNotPatSep	contained	"\\\\"
syn cluster	vimStringGroup	contains=vimEscapeBrace,vimPatSep,vimNotPatSep,vimPatSepErr,vimPatSepZone,@Spell
syn region	vimString	oneline keepend	start=+[^a-zA-Z>!\\@]"+lc=1 skip=+\\\\\|\\"+ matchgroup=vimStringEnd end=+"+	contains=@vimStringGroup
syn region	vimString	oneline keepend	start=+[^a-zA-Z>!\\@]'+lc=1 end=+'+
syn region	vimString	oneline	start=+=!+lc=1	skip=+\\\\\|\\!+ end=+!+	contains=@vimStringGroup
syn region	vimString	oneline	start="=+"lc=1	skip="\\\\\|\\+" end="+"	contains=@vimStringGroup
syn region	vimString	oneline	start="\s/\s*\A"lc=1 skip="\\\\\|\\+" end="/"	contains=@vimStringGroup
syn match	vimString	contained	+"[^"]*\\$+	skipnl nextgroup=vimStringCont
syn match	vimStringCont	contained	+\(\\\\\|.\)\{-}[^\\]"+

" Substitutions: {{{2
" =============
syn cluster	vimSubstList	contains=vimPatSep,vimPatRegion,vimPatSepErr,vimSubstTwoBS,vimSubstRange,vimNotation
syn cluster	vimSubstRepList	contains=vimSubstSubstr,vimSubstTwoBS,vimNotation
syn cluster	vimSubstList	add=vimCollection
syn match	vimSubst	"\(:\+\s*\|^\s*\||\s*\)\<\%(\<s\%[ubstitute]\>\|\<sm\%[agic]\>\|\<sno\%[magic]\>\)[:#[:alpha:]]\@!" nextgroup=vimSubstPat
"syn match	vimSubst	"\%(^\|[^\\]\)\<s\%[ubstitute]\>[:#[:alpha:]]\@!"	nextgroup=vimSubstPat contained
syn match	vimSubst	"\%(^\|[^\\\"']\)\<s\%[ubstitute]\>[:#[:alpha:]\"']\@!"	nextgroup=vimSubstPat contained
syn match	vimSubst	"/\zs\<s\%[ubstitute]\>\ze/"		nextgroup=vimSubstPat
syn match	vimSubst	"\(:\+\s*\|^\s*\)s\ze#.\{-}#.\{-}#"		nextgroup=vimSubstPat
syn match	vimSubst1       contained	"\<s\%[ubstitute]\>"	nextgroup=vimSubstPat
syn match	vimSubst2       contained	"s\%[ubstitute]\>"	nextgroup=vimSubstPat
syn region	vimSubstPat     contained	matchgroup=vimSubstDelim start="\z([^a-zA-Z( \t[\]&]\)"rs=s+1 skip="\\\\\|\\\z1" end="\z1"re=e-1,me=e-1	 contains=@vimSubstList	nextgroup=vimSubstRep4	oneline
syn region	vimSubstRep4    contained	matchgroup=vimSubstDelim start="\z(.\)" skip="\\\\\|\\\z1" end="\z1" matchgroup=vimNotation end="<[cC][rR]>" contains=@vimSubstRepList	nextgroup=vimSubstFlagErr	oneline
syn region	vimCollection   contained transparent	start="\\\@<!\[" skip="\\\[" end="\]"	contains=vimCollClass
syn match	vimCollClassErr contained	"\[:.\{-\}:\]"
syn match	vimCollClass    contained transparent	"\%#=1\[:\(alnum\|alpha\|blank\|cntrl\|digit\|graph\|lower\|print\|punct\|space\|upper\|xdigit\|return\|tab\|escape\|backspace\):\]"
syn match	vimSubstSubstr  contained	"\\z\=\d"
syn match	vimSubstTwoBS   contained	"\\\\"
syn match	vimSubstFlagErr contained	"[^< \t\r|]\+" contains=vimSubstFlags
syn match	vimSubstFlags   contained	"[&cegiIpr]\+"

" 'String': {{{2
syn match	vimString	"[^(,]'[^']\{-}\zs'"

" Marks, Registers, Addresses, Filters: {{{2
syn match	vimMark	"'[a-zA-Z0-9]\ze[-+,!]"	nextgroup=vimFilter,vimMarkNumber,vimSubst
syn match	vimMark	"'[<>]\ze[-+,!]"		nextgroup=vimFilter,vimMarkNumber,vimSubst
syn match	vimMark	",\zs'[<>]\ze"		nextgroup=vimFilter,vimMarkNumber,vimSubst
syn match	vimMark	"[!,:]\zs'[a-zA-Z0-9]"	nextgroup=vimFilter,vimMarkNumber,vimSubst
syn match	vimMark	"\<norm\%[al]\s\zs'[a-zA-Z0-9]"	nextgroup=vimFilter,vimMarkNumber,vimSubst
syn match	vimMarkNumber	"[-+]\d\+"		contained contains=vimOper nextgroup=vimSubst2
syn match	vimPlainMark contained	"'[a-zA-Z0-9]"
syn match	vimRange	"[`'][a-zA-Z0-9],[`'][a-zA-Z0-9]"	contains=vimMark	skipwhite nextgroup=vimFilter

syn match	vimRegister	'[^,;[{: \t]\zs"[a-zA-Z0-9.%#:_\-/]\ze[^a-zA-Z_":0-9]'
syn match	vimRegister	'\<norm\s\+\zs"[a-zA-Z0-9]'
syn match	vimRegister	'\<normal\s\+\zs"[a-zA-Z0-9]'
syn match	vimRegister	'@"'
syn match	vimPlainRegister contained	'"[a-zA-Z0-9\-:.%#*+=]'

syn match	vimAddress	",\zs[.$]"	skipwhite nextgroup=vimSubst1
syn match	vimAddress	"%\ze\a"	skipwhite nextgroup=vimString,vimSubst1

syn match	vimFilter 		"^!!\=[^"]\{-}\(|\|\ze\"\|$\)"	contains=vimOper,vimSpecFile
syn match	vimFilter contained	"!!\=[^"]\{-}\(|\|\ze\"\|$\)"	contains=vimOper,vimSpecFile

" Complex Repeats: (:h complex-repeat) {{{2
" ===============
syn match	vimCmplxRepeat	'[^a-zA-Z_/\\()]q[0-9a-zA-Z"]\>'lc=1
syn match	vimCmplxRepeat	'@[0-9a-z".=@:]\ze\($\|[^a-zA-Z]\>\)'

" Set command and associated set-options (vimOptions) with comment {{{2
syn region	vimSet		matchgroup=vimCommand start="\<\%(setl\%[ocal]\|setg\%[lobal]\|se\%[t]\)\>" skip="\%(\\\\\)*\\." end="$" end="|" matchgroup=vimNotation end="<[cC][rR]>" keepend oneline contains=vimSetEqual,vimOption,vimErrSetting,vimComment,vimSetString,vimSetMod
syn region	vimSetEqual	contained	start="[=:]\|[-+^]=" skip="\\\\\|\\\s" end="[| \t]\|$"me=e-1	contains=vimCtrlChar,vimSetSep,vimNotation,vimEnvvar oneline
syn region	vimSetString	contained	start=+="+hs=s+1	skip=+\\\\\|\\"+  end=+"+		contains=vimCtrlChar
syn match	vimSetSep	contained	"[,:]" skipwhite nextgroup=vimCommand
syn match	vimSetMod	contained	"&vim\=\|[!&?<]\|all&"

" Let: {{{2
" ===
syn keyword	vimLet	let	unl[et]	skipwhite nextgroup=vimVar,vimFuncVar,vimLetHereDoc
VimFoldh syn region vimLetHereDoc	matchgroup=vimLetHereDocStart start='=<<\s\+\%(trim\>\)\=\s*\z(\L\S*\)'	matchgroup=vimLetHereDocStop end='^\s*\z1\s*$'	contains=vimComment

" Abbreviations: {{{2
" =============
syn keyword vimAbb	ab[breviate] ca[bbrev] inorea[bbrev] cnorea[bbrev] norea[bbrev] ia[bbrev] skipwhite nextgroup=vimMapMod,vimMapLhs

" Autocmd: {{{2
" =======
syn match	vimAutoEventList	contained	"\(!\s\+\)\=\(\a\+,\)*\a\+"	contains=vimAutoEvent nextgroup=vimAutoCmdSpace
syn match	vimAutoCmdSpace	contained	"\s\+"	nextgroup=vimAutoCmdSfxList
syn match	vimAutoCmdSfxList	contained	"\S*"
syn keyword	vimAutoCmd	au[tocmd] do[autocmd] doautoa[ll]	skipwhite nextgroup=vimAutoEventList

" Echo And Execute: -- prefer strings! {{{2
" ================
syn region	vimEcho	oneline excludenl matchgroup=vimCommand start="\<ec\%[ho]\>" skip="\(\\\\\)*\\|" end="$\||" contains=vimFunc,vimFuncVar,vimString,vimVar
syn region	vimExecute	oneline excludenl matchgroup=vimCommand start="\<exe\%[cute]\>" skip="\(\\\\\)*\\|" end="$\||\|<[cC][rR]>" contains=vimFuncVar,vimIsCommand,vimOper,vimNotation,vimOperParen,vimString,vimVar
syn match	vimEchoHL	"echohl\="	skipwhite nextgroup=vimGroup,vimHLGroup,vimEchoHLNone
syn case ignore
syn keyword	vimEchoHLNone	none
syn case match

" Maps: {{{2
" ====
syn match	vimMap		"\<map\>!\=\ze\s*[^(]" skipwhite nextgroup=vimMapMod,vimMapLhs
syn keyword	vimMap		cm[ap] cno[remap] im[ap] ino[remap] lm[ap] ln[oremap] nm[ap] nn[oremap] no[remap] om[ap] ono[remap] smap snor[emap] tno[remap] tm[ap] vm[ap] vn[oremap] xm[ap] xn[oremap] skipwhite nextgroup=vimMapBang,vimMapMod,vimMapLhs
syn keyword	vimMap		mapc[lear] smapc[lear]
syn keyword	vimUnmap		cu[nmap] iu[nmap] lu[nmap] nun[map] ou[nmap] sunm[ap] tunma[p] unm[ap] unm[ap] vu[nmap] xu[nmap] skipwhite nextgroup=vimMapBang,vimMapMod,vimMapLhs
syn match	vimMapLhs	contained	"\S\+"			contains=vimNotation,vimCtrlChar skipwhite nextgroup=vimMapRhs
syn match	vimMapBang	contained	"!"			skipwhite nextgroup=vimMapMod,vimMapLhs
syn match	vimMapMod	contained	"\%#=1\c<\(buffer\|expr\|\(local\)\=leader\|nowait\|plug\|script\|sid\|unique\|silent\)\+>" contains=vimMapModKey,vimMapModErr skipwhite nextgroup=vimMapMod,vimMapLhs
syn match	vimMapRhs	contained	".*" contains=vimNotation,vimCtrlChar	skipnl nextgroup=vimMapRhsExtend
syn match	vimMapRhsExtend	contained	"^\s*\\.*$"			contains=vimContinue
syn case ignore
syn keyword	vimMapModKey	contained	buffer	expr	leader	localleader	nowait	plug	script	sid	silent	unique
syn case match

" Menus: {{{2
" =====
syn cluster	vimMenuList contains=vimMenuBang,vimMenuPriority,vimMenuName,vimMenuMod
syn keyword	vimCommand	am[enu] an[oremenu] aun[menu] cme[nu] cnoreme[nu] cunme[nu] ime[nu] inoreme[nu] iunme[nu] me[nu] nme[nu] nnoreme[nu] noreme[nu] nunme[nu] ome[nu] onoreme[nu] ounme[nu] unme[nu] vme[nu] vnoreme[nu] vunme[nu] skipwhite nextgroup=@vimMenuList
syn match	vimMenuName	"[^ \t\\<]\+"	contained nextgroup=vimMenuNameMore,vimMenuMap
syn match	vimMenuPriority	"\d\+\(\.\d\+\)*"	contained skipwhite nextgroup=vimMenuName
syn match	vimMenuNameMore	"\c\\\s\|<tab>\|\\\."	contained nextgroup=vimMenuName,vimMenuNameMore contains=vimNotation
syn match	vimMenuMod    contained	"\c<\(script\|silent\)\+>"  skipwhite contains=vimMapModKey,vimMapModErr nextgroup=@vimMenuList
syn match	vimMenuMap	"\s"	contained skipwhite nextgroup=vimMenuRhs
syn match	vimMenuRhs	".*$"	contained contains=vimString,vimComment,vimIsCommand
syn match	vimMenuBang	"!"	contained skipwhite nextgroup=@vimMenuList

" Angle-Bracket Notation: (tnx to Michael Geddes) {{{2
" ======================
syn case ignore
syn match	vimNotation	"\%#=1\(\\\|<lt>\)\=<\([scamd]-\)\{0,4}x\=\(f\d\{1,2}\|[^ \t:]\|cr\|lf\|linefeed\|return\|k\=del\%[ete]\|bs\|backspace\|tab\|esc\|right\|left\|help\|undo\|insert\|ins\|mouse\|k\=home\|k\=end\|kplus\|kminus\|kdivide\|kmultiply\|kenter\|kpoint\|space\|k\=\(page\)\=\(\|down\|up\|k\d\>\)\)>" contains=vimBracket
syn match	vimNotation	"\%#=1\(\\\|<lt>\)\=<\([scam2-4]-\)\{0,4}\(right\|left\|middle\)\(mouse\)\=\(drag\|release\)\=>"	contains=vimBracket
syn match	vimNotation	"\%#=1\(\\\|<lt>\)\=<\(bslash\|plug\|sid\|space\|bar\|nop\|nul\|lt\)>"			contains=vimBracket
syn match	vimNotation	'\(\\\|<lt>\)\=<C-R>[0-9a-z"%#:.\-=]'he=e-1				contains=vimBracket
syn match	vimNotation	'\%#=1\(\\\|<lt>\)\=<\%(q-\)\=\(line[12]\|count\|bang\|reg\|args\|mods\|f-args\|f-mods\|lt\)>'	contains=vimBracket
syn match	vimNotation	"\%#=1\(\\\|<lt>\)\=<\([cas]file\|abuf\|amatch\|cword\|cWORD\|client\)>"		contains=vimBracket
syn match	vimBracket contained	"[\\<>]"
syn case match

" User Function Highlighting: {{{2
" (following Gautam Iyer's suggestion)
" ==========================
syn match vimFunc		"\%(\%([sSgGbBwWtTlL]:\|<[sS][iI][dD]>\)\=\%(\w\+\.\)*\I[a-zA-Z0-9_.]*\)\ze\s*("		contains=vimFuncName,vimUserFunc,vimExecute
syn match vimUserFunc contained	"\%(\%([sSgGbBwWtTlL]:\|<[sS][iI][dD]>\)\=\%(\w\+\.\)*\I[a-zA-Z0-9_.]*\)\|\<\u[a-zA-Z0-9.]*\>\|\<if\>"	contains=vimNotation

" Errors And Warnings: {{{2
" ====================
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_novimfunctionerror")
 syn match	vimFunctionError	"\s\zs[a-z0-9]\i\{-}\ze\s*("			contained contains=vimFuncKey,vimFuncBlank
 syn match	vimFunctionError	"\s\zs\%(<[sS][iI][dD]>\|[sSgGbBwWtTlL]:\)\d\i\{-}\ze\s*("	contained contains=vimFuncKey,vimFuncBlank
 syn match	vimElseIfErr	"\<else\s\+if\>"
 syn match	vimBufnrWarn	/\<bufnr\s*(\s*["']\.['"]\s*)/
endif

syn match vimNotFunc	"\<if\>\|\<el\%[seif]\>\|\<return\>\|\<while\>"	skipwhite nextgroup=vimOper,vimOperParen,vimVar,vimFunc,vimNotation

" Norm: {{{2
" ====
syn match	vimNorm		"\<norm\%[al]!\=" skipwhite nextgroup=vimNormCmds
syn match	vimNormCmds contained	".*$"

" Syntax: {{{2
"=======
syn match	vimGroupList	contained	"@\=[^ \t,]*"	contains=vimGroupSpecial,vimPatSep
syn match	vimGroupList	contained	"@\=[^ \t,]*,"	nextgroup=vimGroupList contains=vimGroupSpecial,vimPatSep
syn keyword	vimGroupSpecial	contained	ALL	ALLBUT	CONTAINED	TOP
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_novimsynerror")
 syn match	vimSynError	contained	"\i\+"
 syn match	vimSynError	contained	"\i\+="	nextgroup=vimGroupList
endif
syn match	vimSynContains	contained	"\<contain\(s\|edin\)="	nextgroup=vimGroupList
syn match	vimSynKeyContainedin	contained	"\<containedin="	nextgroup=vimGroupList
syn match	vimSynNextgroup	contained	"nextgroup="	nextgroup=vimGroupList

syn match	vimSyntax	"\<sy\%[ntax]\>"	contains=vimCommand skipwhite nextgroup=vimSynType,vimComment
syn match	vimAuSyntax	contained	"\s+sy\%[ntax]"	contains=vimCommand skipwhite nextgroup=vimSynType,vimComment
syn cluster vimFuncBodyList add=vimSyntax

" Syntax: case {{{2
syn keyword	vimSynType	contained	case	skipwhite nextgroup=vimSynCase,vimSynCaseError
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_novimsyncaseerror")
 syn match	vimSynCaseError	contained	"\i\+"
endif
syn keyword	vimSynCase	contained	ignore	match

" Syntax: clear {{{2
syn keyword	vimSynType	contained	clear	skipwhite nextgroup=vimGroupList

" Syntax: cluster {{{2
syn keyword	vimSynType	contained	cluster	skipwhite nextgroup=vimClusterName
syn region	vimClusterName	contained	matchgroup=vimGroupName start="\h\w*" skip="\\\\\|\\|" matchgroup=vimSep end="$\||" contains=vimGroupAdd,vimGroupRem,vimSynContains,vimSynError
syn match	vimGroupAdd	contained	"add="	nextgroup=vimGroupList
syn match	vimGroupRem	contained	"remove="	nextgroup=vimGroupList
syn cluster vimFuncBodyList add=vimSynType,vimGroupAdd,vimGroupRem

" Syntax: iskeyword {{{2
syn keyword	vimSynType	contained	iskeyword	skipwhite nextgroup=vimIskList
syn match	vimIskList	contained	'\S\+'	contains=vimIskSep
syn match	vimIskSep	contained	','

" Syntax: include {{{2
syn keyword	vimSynType	contained	include	skipwhite nextgroup=vimGroupList
syn cluster vimFuncBodyList add=vimSynType

" Syntax: keyword {{{2
syn cluster	vimSynKeyGroup	contains=vimSynNextgroup,vimSynKeyOpt,vimSynKeyContainedin
syn keyword	vimSynType	contained	keyword	skipwhite nextgroup=vimSynKeyRegion
syn region	vimSynKeyRegion	contained oneline keepend	matchgroup=vimGroupName start="\h\w*" skip="\\\\\|\\|" matchgroup=vimSep end="|\|$" contains=@vimSynKeyGroup
syn match	vimSynKeyOpt	contained	"\%#=1\<\(conceal\|contained\|transparent\|skipempty\|skipwhite\|skipnl\)\>"
syn cluster vimFuncBodyList add=vimSynType

" Syntax: match {{{2
syn cluster	vimSynMtchGroup	contains=vimMtchComment,vimSynContains,vimSynError,vimSynMtchOpt,vimSynNextgroup,vimSynRegPat,vimNotation
syn keyword	vimSynType	contained	match	skipwhite nextgroup=vimSynMatchRegion
syn region	vimSynMatchRegion	contained keepend	matchgroup=vimGroupName start="\h\w*" matchgroup=vimSep end="|\|$" contains=@vimSynMtchGroup
syn match	vimSynMtchOpt	contained	"\%#=1\<\(conceal\|transparent\|contained\|excludenl\|keepend\|skipempty\|skipwhite\|display\|extend\|skipnl\|fold\)\>"
if has("conceal")
 syn match	vimSynMtchOpt	contained	"\<cchar="	nextgroup=vimSynMtchCchar
 syn match	vimSynMtchCchar	contained	"\S"
endif
syn cluster vimFuncBodyList add=vimSynMtchGroup

" Syntax: off and on {{{2
syn keyword	vimSynType	contained	enable	list	manual	off	on	reset

" Syntax: region {{{2
syn cluster	vimSynRegPatGroup	contains=vimPatSep,vimNotPatSep,vimSynPatRange,vimSynNotPatRange,vimSubstSubstr,vimPatRegion,vimPatSepErr,vimNotation
syn cluster	vimSynRegGroup	contains=vimSynContains,vimSynNextgroup,vimSynRegOpt,vimSynReg,vimSynMtchGrp
syn keyword	vimSynType	contained	region	skipwhite nextgroup=vimSynRegion
syn region	vimSynRegion	contained keepend	matchgroup=vimGroupName start="\h\w*" skip="\\\\\|\\|" end="|\|$" contains=@vimSynRegGroup
syn match	vimSynRegOpt	contained	"\%#=1\<\(conceal\(ends\)\=\|transparent\|contained\|excludenl\|skipempty\|skipwhite\|display\|keepend\|oneline\|extend\|skipnl\|fold\)\>"
syn match	vimSynReg	contained	"\(start\|skip\|end\)="he=e-1	nextgroup=vimSynRegPat
syn match	vimSynMtchGrp	contained	"matchgroup="	nextgroup=vimGroup,vimHLGroup
syn region	vimSynRegPat	contained extend	start="\z([-`~!@#$%^&*_=+;:'",./?]\)"  skip="\\\\\|\\\z1"  end="\z1"  contains=@vimSynRegPatGroup skipwhite nextgroup=vimSynPatMod,vimSynReg
syn match	vimSynPatMod	contained	"\%#=1\(hs\|ms\|me\|hs\|he\|rs\|re\)=[se]\([-+]\d\+\)\="
syn match	vimSynPatMod	contained	"\%#=1\(hs\|ms\|me\|hs\|he\|rs\|re\)=[se]\([-+]\d\+\)\=," nextgroup=vimSynPatMod
syn match	vimSynPatMod	contained	"lc=\d\+"
syn match	vimSynPatMod	contained	"lc=\d\+," nextgroup=vimSynPatMod
syn region	vimSynPatRange	contained	start="\["	skip="\\\\\|\\]"   end="]"
syn match	vimSynNotPatRange	contained	"\\\\\|\\\["
syn match	vimMtchComment	contained	'"[^"]\+$'
syn cluster vimFuncBodyList add=vimSynType

" Syntax: sync {{{2
" ============
syn keyword vimSynType	contained	sync	skipwhite	nextgroup=vimSyncC,vimSyncLines,vimSyncMatch,vimSyncError,vimSyncLinebreak,vimSyncLinecont,vimSyncRegion
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_novimsyncerror")
 syn match	vimSyncError	contained	"\i\+"
endif
syn keyword	vimSyncC	contained	ccomment	clear	fromstart
syn keyword	vimSyncMatch	contained	match	skipwhite	nextgroup=vimSyncGroupName
syn keyword	vimSyncRegion	contained	region	skipwhite	nextgroup=vimSynReg
syn match	vimSyncLinebreak	contained	"\<linebreaks="	skipwhite	nextgroup=vimNumber
syn keyword	vimSyncLinecont	contained	linecont	skipwhite	nextgroup=vimSynRegPat
syn match	vimSyncLines	contained	"\(min\|max\)\=lines="	nextgroup=vimNumber
syn match	vimSyncGroupName	contained	"\h\w*"	skipwhite	nextgroup=vimSyncKey
syn match	vimSyncKey	contained	"\<groupthere\|grouphere\>"	skipwhite nextgroup=vimSyncGroup
syn match	vimSyncGroup	contained	"\h\w*"	skipwhite	nextgroup=vimSynRegPat,vimSyncNone
syn keyword	vimSyncNone	contained	NONE

" Additional IsCommand: here by reasons of precedence {{{2
" ====================
syn match	vimIsCommand	"<Bar>\s*\a\+"	transparent contains=vimCommand,vimNotation

" Highlighting: {{{2
" ============
syn cluster	vimHighlightCluster		contains=vimHiLink,vimHiClear,vimHiKeyList,vimComment
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_novimhictermerror")
 syn match	vimHiCtermError	contained	"\D\i*"
endif
syn match	vimHighlight	"\<hi\%[ghlight]\>"	skipwhite nextgroup=vimHiBang,@vimHighlightCluster
syn match	vimHiBang	contained	"!"	skipwhite nextgroup=@vimHighlightCluster

syn match	vimHiGroup	contained	"\i\+"
syn case ignore
syn keyword	vimHiAttrib	contained	none bold inverse italic nocombine reverse standout strikethrough underline undercurl
syn keyword	vimFgBgAttrib	contained	none bg background fg foreground
syn case match
syn match	vimHiAttribList	contained	"\i\+"	contains=vimHiAttrib
syn match	vimHiAttribList	contained	"\i\+,"he=e-1	contains=vimHiAttrib nextgroup=vimHiAttribList
syn case ignore
syn keyword	vimHiCtermColor	contained	black blue brown cyan darkblue darkcyan darkgray darkgreen darkgrey darkmagenta darkred darkyellow gray green grey lightblue lightcyan lightgray lightgreen lightgrey lightmagenta lightred magenta red white yellow
syn match	vimHiCtermColor	contained	"\<color\d\{1,3}\>"

syn case match
syn match	vimHiFontname	contained	"[a-zA-Z\-*]\+"
syn match	vimHiGuiFontname	contained	"'[a-zA-Z\-* ]\+'"
syn match	vimHiGuiRgb	contained	"#\x\{6}"

" Highlighting: hi group key=arg ... {{{2
syn cluster	vimHiCluster contains=vimGroup,vimHiGroup,vimHiTerm,vimHiCTerm,vimHiStartStop,vimHiCtermFgBg,vimHiGui,vimHiGuiFont,vimHiGuiFgBg,vimHiKeyError,vimNotation
syn region	vimHiKeyList	contained oneline start="\i\+" skip="\\\\\|\\|" end="$\||"	contains=@vimHiCluster
if !exists("g:vimsyn_noerror") && !exists("g:vimsyn_vimhikeyerror")
 syn match	vimHiKeyError	contained	"\i\+="he=e-1
endif
syn match	vimHiTerm	contained	"\cterm="he=e-1		nextgroup=vimHiAttribList
syn match	vimHiStartStop	contained	"\c\(start\|stop\)="he=e-1	nextgroup=vimHiTermcap,vimOption
syn match	vimHiCTerm	contained	"\ccterm="he=e-1		nextgroup=vimHiAttribList
syn match	vimHiCtermFgBg	contained	"\ccterm[fb]g="he=e-1	nextgroup=vimHiNmbr,vimHiCtermColor,vimFgBgAttrib,vimHiCtermError
syn match	vimHiGui	contained	"\cgui="he=e-1		nextgroup=vimHiAttribList
syn match	vimHiGuiFont	contained	"\cfont="he=e-1		nextgroup=vimHiFontname
syn match	vimHiGuiFgBg	contained	"\cgui\%([fb]g\|sp\)="he=e-1	nextgroup=vimHiGroup,vimHiGuiFontname,vimHiGuiRgb,vimFgBgAttrib
syn match	vimHiTermcap	contained	"\S\+"		contains=vimNotation
syn match	vimHiNmbr	contained	'\d\+'

" Highlight: clear {{{2
syn keyword	vimHiClear	contained	clear	nextgroup=vimHiGroup

" Highlight: link {{{2
" see tst24 (hi def vs hi) (Jul 06, 2018)
"syn region	vimHiLink	contained oneline matchgroup=vimCommand start="\(\<hi\%[ghlight]\s\+\)\@<=\(\(def\%[ault]\s\+\)\=link\>\|\<def\>\)" end="$"	contains=vimHiGroup,vimGroup,vimHLGroup,vimNotation
syn region	vimHiLink	contained oneline matchgroup=vimCommand start="\(\<hi\%[ghlight]\s\+\)\@<=\(\(def\%[ault]\s\+\)\=link\>\|\<def\>\)" end="$"	contains=@vimHiCluster
syn cluster vimFuncBodyList add=vimHiLink

" Control Characters: {{{2
" ==================
syn match	vimCtrlChar	"[--]"

" Beginners - Patterns that involve ^ {{{2
" =========
syn match	vimLineComment	+^[ \t:]*".*$+	contains=@vimCommentGroup,vimCommentString,vimCommentTitle
syn match	vimCommentTitle	'"\s*\%([sS]:\|\h\w*#\)\=\u\w*\(\s\+\u\w*\)*:'hs=s+1	contained contains=vimCommentTitleLeader,vimTodo,@vimCommentGroup
syn match	vimContinue	"^\s*\\"
syn region	vimString	start="^\s*\\\z(['"]\)" skip='\\\\\|\\\z1' end="\z1" oneline keepend contains=@vimStringGroup,vimContinue
syn match	vimCommentTitleLeader	'"\s\+'ms=s+1	contained

" Searches And Globals: {{{2
" ====================
syn match	vimSearch	'^\s*[/?].*'		contains=vimSearchDelim
syn match	vimSearchDelim	'^\s*\zs[/?]\|[/?]$'	contained
syn region	vimGlobal	matchgroup=Statement start='\<g\%[lobal]!\=/'  skip='\\.' end='/'	skipwhite nextgroup=vimSubst
syn region	vimGlobal	matchgroup=Statement start='\<v\%[global]!\=/' skip='\\.' end='/'	skipwhite nextgroup=vimSubst

" Embedded Scripts:  {{{2
" ================
"   perl,ruby     : Benoit Cerrina
"   python,tcl    : Johannes Zellner
"   mzscheme, lua : Charles Campbell

" Allows users to specify the type of embedded script highlighting
" they want:  (perl/python/ruby/tcl support)
"   g:vimsyn_embed == 0   : don't embed any scripts
"   g:vimsyn_embed =~# 'l' : embed lua      (but only if vim supports it)
"   g:vimsyn_embed =~# 'm' : embed mzscheme (but only if vim supports it)
"   g:vimsyn_embed =~# 'p' : embed perl     (but only if vim supports it)
"   g:vimsyn_embed =~# 'P' : embed python   (but only if vim supports it)
"   g:vimsyn_embed =~# 'r' : embed ruby     (but only if vim supports it)
"   g:vimsyn_embed =~# 't' : embed tcl      (but only if vim supports it)
if !exists("g:vimsyn_embed")
 let g:vimsyn_embed= "lmpPr"
endif

" [-- lua --] {{{3
let s:luapath= fnameescape(expand("<sfile>:p:h")."/lua.vim")
if !filereadable(s:luapath)
 for s:luapath in split(globpath(&rtp,"syntax/lua.vim"),"\n")
  if filereadable(fnameescape(s:luapath))
   let s:luapath= fnameescape(s:luapath)
   break
  endif
 endfor
endif
if (g:vimsyn_embed =~# 'l' && has("lua")) && filereadable(s:luapath)
 unlet! b:current_syntax
 syn cluster vimFuncBodyList	add=vimLuaRegion
 exe "syn include @vimLuaScript ".s:luapath
 VimFoldl syn region vimLuaRegion matchgroup=vimScriptDelim start=+lua\s*<<\s*\z(.*\)$+ end=+^\z1$+	contains=@vimLuaScript
 VimFoldl syn region vimLuaRegion matchgroup=vimScriptDelim start=+lua\s*<<\s*$+ end=+\.$+	contains=@vimLuaScript
 syn cluster vimFuncBodyList	add=vimLuaRegion
else
 syn region vimEmbedError start=+lua\s*<<\s*\z(.*\)$+ end=+^\z1$+
 syn region vimEmbedError start=+lua\s*<<\s*$+ end=+\.$+
endif
unlet s:luapath

" [-- perl --] {{{3
let s:perlpath= fnameescape(expand("<sfile>:p:h")."/perl.vim")
if !filereadable(s:perlpath)
 for s:perlpath in split(globpath(&rtp,"syntax/perl.vim"),"\n")
  if filereadable(fnameescape(s:perlpath))
   let s:perlpath= fnameescape(s:perlpath)
   break
  endif
 endfor
endif
if (g:vimsyn_embed =~# 'p' && has("perl")) && filereadable(s:perlpath)
 unlet! b:current_syntax
 syn cluster vimFuncBodyList	add=vimPerlRegion
 exe "syn include @vimPerlScript ".s:perlpath
 VimFoldp syn region vimPerlRegion  matchgroup=vimScriptDelim start=+pe\%[rl]\s*<<\s*\z(\S*\)\ze\(\s*["#].*\)\=$+ end=+^\z1\ze\(\s*[#"].*\)\=$+	contains=@vimPerlScript
 VimFoldp syn region vimPerlRegion	matchgroup=vimScriptDelim start=+pe\%[rl]\s*<<\s*$+ end=+\.$+			contains=@vimPerlScript
 syn cluster vimFuncBodyList	add=vimPerlRegion
else
 syn region vimEmbedError start=+pe\%[rl]\s*<<\s*\z(.*\)$+ end=+^\z1$+
 syn region vimEmbedError start=+pe\%[rl]\s*<<\s*$+ end=+\.$+
endif
unlet s:perlpath

" [-- ruby --] {{{3
let s:rubypath= fnameescape(expand("<sfile>:p:h")."/ruby.vim")
if !filereadable(s:rubypath)
 for s:rubypath in split(globpath(&rtp,"syntax/ruby.vim"),"\n")
  if filereadable(fnameescape(s:rubypath))
   let s:rubypath= fnameescape(s:rubypath)
   break
  endif
 endfor
endif
if (g:vimsyn_embed =~# 'r' && has("ruby")) && filereadable(s:rubypath)
 syn cluster vimFuncBodyList	add=vimRubyRegion
 unlet! b:current_syntax
 exe "syn include @vimRubyScript ".s:rubypath
 VimFoldr syn region vimRubyRegion	matchgroup=vimScriptDelim start=+rub[y]\s*<<\s*\z(\S*\)\ze\(\s*#.*\)\=$+ end=+^\z1\ze\(\s*".*\)\=$+	contains=@vimRubyScript
 syn region vimRubyRegion	matchgroup=vimScriptDelim start=+rub[y]\s*<<\s*$+ end=+\.$+			contains=@vimRubyScript
 syn cluster vimFuncBodyList	add=vimRubyRegion
else
 syn region vimEmbedError start=+rub[y]\s*<<\s*\z(.*\)$+ end=+^\z1$+
 syn region vimEmbedError start=+rub[y]\s*<<\s*$+ end=+\.$+
endif
unlet s:rubypath

" [-- python --] {{{3
let s:pythonpath= fnameescape(expand("<sfile>:p:h")."/python.vim")
if !filereadable(s:pythonpath)
 for s:pythonpath in split(globpath(&rtp,"syntax/python.vim"),"\n")
  if filereadable(fnameescape(s:pythonpath))
   let s:pythonpath= fnameescape(s:pythonpath)
   break
  endif
 endfor
endif
if g:vimsyn_embed =~# 'P' && has("pythonx") && filereadable(s:pythonpath)
 unlet! b:current_syntax
 syn cluster vimFuncBodyList	add=vimPythonRegion
 exe "syn include @vimPythonScript ".s:pythonpath
 VimFoldP syn region vimPythonRegion matchgroup=vimScriptDelim start=+py\%[thon]3\=\s*<<\s*\z(\S*\)\ze\(\s*#.*\)\=$+ end=+^\z1\ze\(\s*".*\)\=$+	contains=@vimPythonScript
 VimFoldP syn region vimPythonRegion matchgroup=vimScriptDelim start=+py\%[thon]3\=\s*<<\s*$+ end=+\.$+			contains=@vimPythonScript
 VimFoldP syn region vimPythonRegion matchgroup=vimScriptDelim start=+Py\%[thon]2or3\s*<<\s*\z(\S*\)\ze\(\s*#.*\)\=$+ end=+^\z1\ze\(\s*".*\)\=$+	contains=@vimPythonScript
 VimFoldP syn region vimPythonRegion matchgroup=vimScriptDelim start=+Py\%[thon]2or3\=\s*<<\s*$+ end=+\.$+			contains=@vimPythonScript
 syn cluster vimFuncBodyList	add=vimPythonRegion
else
 syn region vimEmbedError start=+py\%[thon]3\=\s*<<\s*\z(.*\)$+ end=+^\z1$+
 syn region vimEmbedError start=+py\%[thon]3\=\s*<<\s*$+ end=+\.$+
endif
unlet s:pythonpath

" [-- tcl --] {{{3
if has("win32") || has("win95") || has("win64") || has("win16")
 " apparently has("tcl") has been hanging vim on some windows systems with cygwin
 let s:trytcl= (&shell !~ '\<\%(bash\>\|4[nN][tT]\|\<zsh\)\>\%(\.exe\)\=$')
else
 let s:trytcl= 1
endif
if s:trytcl
 let s:tclpath= fnameescape(expand("<sfile>:p:h")."/tcl.vim")
 if !filereadable(s:tclpath)
  for s:tclpath in split(globpath(&rtp,"syntax/tcl.vim"),"\n")
   if filereadable(fnameescape(s:tclpath))
    let s:tclpath= fnameescape(s:tclpath)
    break
   endif
  endfor
 endif
 if (g:vimsyn_embed =~# 't' && has("tcl")) && filereadable(s:tclpath)
  unlet! b:current_syntax
  syn cluster vimFuncBodyList	add=vimTclRegion
  exe "syn include @vimTclScript ".s:tclpath
  VimFoldt syn region vimTclRegion matchgroup=vimScriptDelim start=+tc[l]\=\s*<<\s*\z(.*\)$+ end=+^\z1$+	contains=@vimTclScript
  VimFoldt syn region vimTclRegion matchgroup=vimScriptDelim start=+tc[l]\=\s*<<\s*$+ end=+\.$+	contains=@vimTclScript
  syn cluster vimFuncBodyList	add=vimTclScript
 else
  syn region vimEmbedError start=+tc[l]\=\s*<<\s*\z(.*\)$+ end=+^\z1$+
  syn region vimEmbedError start=+tc[l]\=\s*<<\s*$+ end=+\.$+
 endif
 unlet s:tclpath
else
 syn region vimEmbedError start=+tc[l]\=\s*<<\s*\z(.*\)$+ end=+^\z1$+
 syn region vimEmbedError start=+tc[l]\=\s*<<\s*$+ end=+\.$+
endif
unlet s:trytcl

" [-- mzscheme --] {{{3
let s:mzschemepath= fnameescape(expand("<sfile>:p:h")."/scheme.vim")
if !filereadable(s:mzschemepath)
 for s:mzschemepath in split(globpath(&rtp,"syntax/mzscheme.vim"),"\n")
  if filereadable(fnameescape(s:mzschemepath))
   let s:mzschemepath= fnameescape(s:mzschemepath)
   break
  endif
 endfor
endif
if (g:vimsyn_embed =~# 'm' && has("mzscheme")) && filereadable(s:mzschemepath)
 unlet! b:current_syntax
 let s:iskKeep= &isk
 syn cluster vimFuncBodyList	add=vimMzSchemeRegion
 exe "syn include @vimMzSchemeScript ".s:mzschemepath
 let &isk= s:iskKeep
 unlet s:iskKeep
 VimFoldm syn region vimMzSchemeRegion matchgroup=vimScriptDelim start=+mz\%[scheme]\s*<<\s*\z(.*\)$+ end=+^\z1$+	contains=@vimMzSchemeScript
 VimFoldm syn region vimMzSchemeRegion matchgroup=vimScriptDelim start=+mz\%[scheme]\s*<<\s*$+ end=+\.$+		contains=@vimMzSchemeScript
 syn cluster vimFuncBodyList	add=vimMzSchemeRegion
else
 syn region vimEmbedError start=+mz\%[scheme]\s*<<\s*\z(.*\)$+ end=+^\z1$+
 syn region vimEmbedError start=+mz\%[scheme]\s*<<\s*$+ end=+\.$+
endif
unlet s:mzschemepath

" Synchronize (speed) {{{2
"============
if exists("g:vimsyn_minlines")
 exe "syn sync minlines=".g:vimsyn_minlines
endif
exe "syn sync maxlines=".s:vimsyn_maxlines
syn sync linecont	"^\s\+\\"
syn sync match vimAugroupSyncA	groupthere NONE	"\<aug\%[roup]\>\s\+[eE][nN][dD]"

" ====================
" Highlighting Settings {{{2
" ====================

if !exists("skip_vim_syntax_inits")
 if !exists("g:vimsyn_noerror")
  hi def link vimBehaveError	vimError
  hi def link vimCollClassErr	vimError
  hi def link vimErrSetting	vimError
  hi def link vimEmbedError	vimError
  hi def link vimFTError	vimError
  hi def link vimFunctionError	vimError
  hi def link vimFunc         	vimError
  hi def link vimHiAttribList	vimError
  hi def link vimHiCtermError	vimError
  hi def link vimHiKeyError	vimError
  hi def link vimKeyCodeError	vimError
  hi def link vimMapModErr	vimError
  hi def link vimSubstFlagErr	vimError
  hi def link vimSynCaseError	vimError
  hi def link vimBufnrWarn	vimWarn
 endif

 hi def link vimAbb	vimCommand
 hi def link vimAddress	vimMark
 hi def link vimAugroupError	vimError
 hi def link vimAugroupKey	vimCommand
 hi def link vimAuHighlight	vimHighlight
 hi def link vimAutoCmdOpt	vimOption
 hi def link vimAutoCmd	vimCommand
 hi def link vimAutoEvent	Type
 hi def link vimAutoSet	vimCommand
 hi def link vimBehaveModel	vimBehave
 hi def link vimBehave	vimCommand
 hi def link vimBracket	Delimiter
 hi def link vimCmplxRepeat	SpecialChar
 hi def link vimCommand	Statement
 hi def link vimComment	Comment
 hi def link vimCommentString	vimString
 hi def link vimCommentTitle	PreProc
 hi def link vimCondHL	vimCommand
 hi def link vimContinue	Special
 hi def link vimCtrlChar	SpecialChar
 hi def link vimEchoHLNone	vimGroup
 hi def link vimEchoHL	vimCommand
 hi def link vimElseIfErr	Error
 hi def link vimElseif	vimCondHL
 hi def link vimEnvvar	PreProc
 hi def link vimError	Error
 hi def link vimFBVar	vimVar
 hi def link vimFgBgAttrib	vimHiAttrib
 hi def link vimFold	Folded
 hi def link vimFTCmd	vimCommand
 hi def link vimFTOption	vimSynType
 hi def link vimFuncKey	vimCommand
 hi def link vimFuncName	Function
 hi def link vimFuncSID	Special
 hi def link vimFuncVar	Identifier
 hi def link vimGroupAdd	vimSynOption
 hi def link vimGroupName	vimGroup
 hi def link vimGroupRem	vimSynOption
 hi def link vimGroupSpecial	Special
 hi def link vimGroup	Type
 hi def link vimHiAttrib	PreProc
 hi def link vimHiClear	vimHighlight
 hi def link vimHiCtermFgBg	vimHiTerm
 hi def link vimHiCTerm	vimHiTerm
 hi def link vimHighlight	vimCommand
 hi def link vimHiGroup	vimGroupName
 hi def link vimHiGuiFgBg	vimHiTerm
 hi def link vimHiGuiFont	vimHiTerm
 hi def link vimHiGuiRgb	vimNumber
 hi def link vimHiGui	vimHiTerm
 hi def link vimHiNmbr	Number
 hi def link vimHiStartStop	vimHiTerm
 hi def link vimHiTerm	Type
 hi def link vimHLGroup	vimGroup
 hi def link vimHLMod	PreProc
 hi def link vimInsert	vimString
 hi def link vimIskSep	Delimiter
 hi def link vimKeyCode	vimSpecFile
 hi def link vimKeyword	Statement
 hi def link vimLet	vimCommand
 hi def link vimLetHereDoc	vimString
 hi def link vimLetHereDocStart	Special
 hi def link vimLetHereDocStop	Special
 hi def link vimLineComment	vimComment
 hi def link vimMapBang	vimCommand
 hi def link vimMapModKey	vimFuncSID
 hi def link vimMapMod	vimBracket
 hi def link vimMap	vimCommand
 hi def link vimMark	Number
 hi def link vimMarkNumber	vimNumber
 hi def link vimMenuMod	vimMapMod
 hi def link vimMenuNameMore	vimMenuName
 hi def link vimMenuName	PreProc
 hi def link vimMtchComment	vimComment
 hi def link vimNorm	vimCommand
 hi def link vimNotation	Special
 hi def link vimNotFunc	vimCommand
 hi def link vimNotPatSep	vimString
 hi def link vimNumber	Number
 hi def link vimOperError	Error
 hi def link vimOper	Operator
 hi def link vimOption	PreProc
 hi def link vimParenSep	Delimiter
 hi def link vimPatSepErr	vimError
 hi def link vimPatSepR	vimPatSep
 hi def link vimPatSep	SpecialChar
 hi def link vimPatSepZone	vimString
 hi def link vimPatSepZ	vimPatSep
 hi def link vimPattern	Type
 hi def link vimPlainMark	vimMark
 hi def link vimPlainRegister	vimRegister
 hi def link vimRegister	SpecialChar
 hi def link vimScriptDelim	Comment
 hi def link vimSearchDelim	Statement
 hi def link vimSearch	vimString
 hi def link vimSep	Delimiter
 hi def link vimSetMod	vimOption
 hi def link vimSetSep	Statement
 hi def link vimSetString	vimString
 hi def link vimSpecFile	Identifier
 hi def link vimSpecFileMod	vimSpecFile
 hi def link vimSpecial	Type
 hi def link vimStatement	Statement
 hi def link vimStringCont	vimString
 hi def link vimString	String
 hi def link vimStringEnd	vimString
 hi def link vimSubst1	vimSubst
 hi def link vimSubstDelim	Delimiter
 hi def link vimSubstFlags	Special
 hi def link vimSubstSubstr	SpecialChar
 hi def link vimSubstTwoBS	vimString
 hi def link vimSubst	vimCommand
 hi def link vimSynCaseError	Error
 hi def link vimSynCase	Type
 hi def link vimSyncC	Type
 hi def link vimSyncError	Error
 hi def link vimSyncGroupName	vimGroupName
 hi def link vimSyncGroup	vimGroupName
 hi def link vimSyncKey	Type
 hi def link vimSyncNone	Type
 hi def link vimSynContains	vimSynOption
 hi def link vimSynError	Error
 hi def link vimSynKeyContainedin	vimSynContains
 hi def link vimSynKeyOpt	vimSynOption
 hi def link vimSynMtchGrp	vimSynOption
 hi def link vimSynMtchOpt	vimSynOption
 hi def link vimSynNextgroup	vimSynOption
 hi def link vimSynNotPatRange	vimSynRegPat
 hi def link vimSynOption	Special
 hi def link vimSynPatRange	vimString
 hi def link vimSynRegOpt	vimSynOption
 hi def link vimSynRegPat	vimString
 hi def link vimSynReg	Type
 hi def link vimSyntax	vimCommand
 hi def link vimSynType	vimSpecial
 hi def link vimTodo	Todo
 hi def link vimUnmap	vimMap
 hi def link vimUserAttrbCmpltFunc	Special
 hi def link vimUserAttrbCmplt	vimSpecial
 hi def link vimUserAttrbKey	vimOption
 hi def link vimUserAttrb	vimSpecial
 hi def link vimUserAttrbError	Error
 hi def link vimUserCmdError	Error
 hi def link vimUserCommand	vimCommand
 hi def link vimUserFunc	Normal
 hi def link vimVar	Identifier
 hi def link vimWarn	WarningMsg
endif

" Current Syntax Variable: {{{2
let b:current_syntax = "vim"

" ---------------------------------------------------------------------
" Cleanup: {{{1
delc VimFolda
delc VimFoldf
delc VimFoldl
delc VimFoldm
delc VimFoldp
delc VimFoldP
delc VimFoldr
delc VimFoldt
let &cpo = s:keepcpo
unlet s:keepcpo
" vim:ts=18  fdm=marker
