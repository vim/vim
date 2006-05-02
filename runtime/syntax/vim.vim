" Vim syntax file
" Language:	Vim 7.0 script
" Maintainer:	Dr. Charles E. Campbell, Jr. <NdrOchipS@PcampbellAfamily.Mbiz>
" Last Change:	May 02, 2006
" Version:	7.0-50
" Automatically generated keyword lists: {{{1

" Quit when a syntax file was already loaded {{{2
if exists("b:current_syntax")
  finish
endif

" vimTodo: contains common special-notices for comments {{{2
" Use the vimCommentGroup cluster to add your own.
syn keyword vimTodo contained	COMBAK	FIXME	TODO	XXX
syn cluster vimCommentGroup	contains=vimTodo,@Spell

" regular vim commands {{{2
syn keyword vimCommand contained	ab[breviate] abc[lear] abo[veleft] al[l] arga[dd] argd[elete] argdo arge[dit] argg[lobal] argl[ocal] ar[gs] argu[ment] as[cii] bad[d] ba[ll] bd[elete] be bel[owright] bf[irst] bl[ast] bm[odified] bn[ext] bN[ext] bo[tright] bp[revious] brea[k] breaka[dd] breakd[el] breakl[ist] br[ewind] bro[wse] bufdo b[uffer] buffers bun[load] bw[ipeout] ca[bbrev] cabc[lear] caddb[uffer] cad[dexpr] caddf[ile] cal[l] cat[ch] cb[uffer] cc ccl[ose] cd ce[nter] cex[pr] cf[ile] cfir[st] cgetb[uffer] cgete[xpr] cg[etfile] c[hange] changes chd[ir] che[ckpath] checkt[ime] cla[st] cl[ist] clo[se] cmapc[lear] cnew[er] cn[ext] cN[ext] cnf[ile] cNf[ile] cnorea[bbrev] col[der] colo[rscheme] comc[lear] comp[iler] conf[irm] con[tinue] cope[n] co[py] cpf[ile] cp[revious] cq[uit] cr[ewind] cuna[bbrev] cu[nmap] cw[indow] debugg[reedy] delc[ommand] d[elete] DeleteFirst delf[unction] delm[arks] diffg[et] diffoff diffpatch diffpu[t] diffsplit diffthis diffu[pdate] dig[raphs] di[splay] dj[ump] dl[ist] dr[op] ds[earch] dsp[lit] earlier echoe[rr] echom[sg] echon e[dit] el[se] elsei[f] em[enu] emenu* endfo[r] endf[unction] en[dif] endt[ry] endw[hile] ene[w] ex exi[t] Explore exu[sage] f[ile] files filetype fina[lly] fin[d] fini[sh] fir[st] fix[del] fo[ld] foldc[lose] folddoc[losed] foldd[oopen] foldo[pen] for fu[nction] go[to] gr[ep] grepa[dd] ha[rdcopy] h[elp] helpf[ind] helpg[rep] helpt[ags] Hexplore hid[e] his[tory] I ia[bbrev] iabc[lear] if ij[ump] il[ist] imapc[lear] inorea[bbrev] is[earch] isp[lit] iuna[bbrev] iu[nmap] j[oin] ju[mps] k keepalt keepj[umps] kee[pmarks] laddb[uffer] lad[dexpr] laddf[ile] lan[guage] la[st] later lb[uffer] lc[d] lch[dir] lcl[ose] le[ft] lefta[bove] lex[pr] lf[ile] lfir[st] lgetb[uffer] lgete[xpr] lg[etfile] lgr[ep] lgrepa[dd] lh[elpgrep] l[ist] ll lla[st] lli[st] lmak[e] lm[ap] lmapc[lear] lnew[er] lne[xt] lN[ext] lnf[ile] lNf[ile] ln[oremap] lo[adview] loc[kmarks] lockv[ar] lol[der] lop[en] lpf[ile] lp[revious] lr[ewind] ls lt[ag] lu[nmap] lv[imgrep] lvimgrepa[dd] lw[indow] mak[e] ma[rk] marks mat[ch] menut[ranslate] mk[exrc] mks[ession] mksp[ell] mkvie[w] mkv[imrc] mod[e] m[ove] mzf[ile] mz[scheme] nbkey NetrwSettings new n[ext] N[ext] nmapc[lear] noh[lsearch] norea[bbrev] Nread nu[mber] nun[map] Nw omapc[lear] on[ly] o[pen] opt[ions] ou[nmap] pc[lose] ped[it] pe[rl] perld[o] po[p] popu popu[p] pp[op] pre[serve] prev[ious] p[rint] P[rint] profd[el] prof[ile] prompt promptf[ind] promptr[epl] ps[earch] pta[g] ptf[irst] ptj[ump] ptl[ast] ptn[ext] ptN[ext] ptp[revious] ptr[ewind] pts[elect] pu[t] pw[d] pyf[ile] py[thon] qa[ll] q[uit] quita[ll] r[ead] rec[over] redi[r] red[o] redr[aw] redraws[tatus] reg[isters] res[ize] ret[ab] retu[rn] rew[ind] ri[ght] rightb[elow] rub[y] rubyd[o] rubyf[ile] ru[ntime] rv[iminfo] sal[l] san[dbox] sa[rgument] sav[eas] sba[ll] sbf[irst] sbl[ast] sbm[odified] sbn[ext] sbN[ext] sbp[revious] sbr[ewind] sb[uffer] scripte[ncoding] scrip[tnames] se[t] setf[iletype] setg[lobal] setl[ocal] Sexplore sf[ind] sfir[st] sh[ell] sign sil[ent] sim[alt] sla[st] sl[eep] sm[agic] sm[ap] smapc[lear] sme smenu sn[ext] sN[ext] sni[ff] sno[magic] snor[emap] snoreme snoremenu sor[t] so[urce] spelld[ump] spe[llgood] spelli[nfo] spellr[epall] spellu[ndo] spellw[rong] sp[lit] spr[evious] sre[wind] sta[g] startg[replace] star[tinsert] startr[eplace] stj[ump] st[op] stopi[nsert] sts[elect] sun[hide] sunm[ap] sus[pend] sv[iew] syncbind t tab tabc[lose] tabd[o] tabe[dit] tabf[ind] tabfir[st] tabl[ast] tabmove tabnew tabn[ext] tabN[ext] tabo[nly] tabp[revious] tabr[ewind] tabs ta[g] tags tc[l] tcld[o] tclf[ile] te[aroff] tf[irst] the th[row] tj[ump] tl[ast] tm tm[enu] tn[ext] tN[ext] to[pleft] tp[revious] tr[ewind] try ts[elect] tu tu[nmenu] una[bbreviate] u[ndo] undoj[oin] undol[ist] unh[ide] unlo[ckvar] unm[ap] up[date] verb[ose] ve[rsion] vert[ical] Vexplore vie[w] vim[grep] vimgrepa[dd] vi[sual] viu[sage] vmapc[lear] vne[w] vs[plit] vu[nmap] wa[ll] wh[ile] winc[md] windo winp[os] win[size] wn[ext] wN[ext] wp[revious] wq wqa[ll] w[rite] ws[verb] wv[iminfo] X xa[ll] x[it] xm[ap] xmapc[lear] xme xmenu XMLent XMLns xn[oremap] xnoreme xnoremenu xu[nmap] y[ank] 
syn match   vimCommand contained	"\<z[-+^.=]"

" vimOptions are caught only when contained in a vimSet {{{2
syn keyword vimOption contained	acd ai akm al aleph allowrevins altkeymap ambiwidth ambw anti antialias ar arab arabic arabicshape ari arshape autochdir autoindent autoread autowrite autowriteall aw awa background backspace backup backupcopy backupdir backupext backupskip balloondelay ballooneval balloonexpr bdir bdlay beval bex bexpr bg bh bin binary biosk bioskey bk bkc bl bomb breakat brk browsedir bs bsdir bsk bt bufhidden buflisted buftype casemap cb ccv cd cdpath cedit cf cfu ch charconvert ci cin cindent cink cinkeys cino cinoptions cinw cinwords clipboard cmdheight cmdwinheight cmp cms co columns com comments commentstring compatible complete completefunc completeopt confirm consk conskey copyindent cot cp cpo cpoptions cpt cscopepathcomp cscopeprg cscopequickfix cscopetag cscopetagorder cscopeverbose cspc csprg csqf cst csto csverb cuc cul cursorcolumn cursorline cwh debug deco def define delcombine dex dg dict dictionary diff diffexpr diffopt digraph dip dir directory display dy ea ead eadirection eb ed edcompatible ef efm ei ek enc encoding endofline eol ep equalalways equalprg errorbells errorfile errorformat esckeys et eventignore ex expandtab exrc fcl fcs fdc fde fdi fdl fdls fdm fdn fdo fdt fen fenc fencs fex ff ffs fileencoding fileencodings fileformat fileformats filetype fillchars fk fkmap flp fml fmr fo foldclose foldcolumn foldenable foldexpr foldignore foldlevel foldlevelstart foldmarker foldmethod foldminlines foldnestmax foldopen foldtext formatexpr formatlistpat formatoptions formatprg fp fs fsync ft gcr gd gdefault gfm gfn gfs gfw ghr go gp grepformat grepprg gtl gtt guicursor guifont guifontset guifontwide guiheadroom guioptions guipty guitablabel guitabtooltip helpfile helpheight helplang hf hh hi hid hidden highlight history hk hkmap hkmapp hkp hl hlg hls hlsearch ic icon iconstring ignorecase im imactivatekey imak imc imcmdline imd imdisable imi iminsert ims imsearch inc include includeexpr incsearch inde indentexpr indentkeys indk inex inf infercase insertmode is isf isfname isi isident isk iskeyword isp isprint joinspaces js key keymap keymodel keywordprg km kmp kp langmap langmenu laststatus lazyredraw lbr lcs linebreak lines linespace lisp lispwords list listchars lm lmap loadplugins lpl ls lsp lw lz ma macatsui magic makeef makeprg mat matchpairs matchtime maxcombine maxfuncdepth maxmapdepth maxmem maxmempattern maxmemtot mco mef menuitems mfd mh mis mkspellmem ml mls mm mmd mmp mmt mod modeline modelines modifiable modified more mouse mousef mousefocus mousehide mousem mousemodel mouses mouseshape mouset mousetime mp mps msm mzq mzquantum nf nrformats nu number numberwidth nuw oft ofu omnifunc operatorfunc opfunc osfiletype pa para paragraphs paste pastetoggle patchexpr patchmode path pdev penc pex pexpr pfn ph pheader pi pm pmbcs pmbfn popt preserveindent previewheight previewwindow printdevice printencoding printexpr printfont printheader printmbcharset printmbfont printoptions prompt pt pumheight pvh pvw qe quoteescape readonly remap report restorescreen revins ri rightleft rightleftcmd rl rlc ro rs rtp ru ruf ruler rulerformat runtimepath sb sbo sbr sc scb scr scroll scrollbind scrolljump scrolloff scrollopt scs sect sections secure sel selection selectmode sessionoptions sft sh shcf shell shellcmdflag shellpipe shellquote shellredir shellslash shelltemp shelltype shellxquote shiftround shiftwidth shm shortmess shortname showbreak showcmd showfulltag showmatch showmode showtabline shq si sidescroll sidescrolloff siso sj slm sm smartcase smartindent smarttab smc smd sn so softtabstop sol sp spc spell spellcapcheck spellfile spelllang spellsuggest spf spl splitbelow splitright spr sps sr srr ss ssl ssop st sta stal startofline statusline stl stmp sts su sua suffixes suffixesadd sw swapfile swapsync swb swf switchbuf sws sxq syn synmaxcol syntax ta tabline tabpagemax tabstop tag tagbsearch taglength tagrelative tags tagstack tal tb tbi tbidi tbis tbs tenc term termbidi termencoding terse textauto textmode textwidth tf tgst thesaurus tildeop timeout timeoutlen title titlelen titleold titlestring tl tm to toolbar toolbariconsize top tpm tr ts tsl tsr ttimeout ttimeoutlen ttm tty ttybuiltin ttyfast ttym ttymouse ttyscroll ttytype tw tx uc ul undolevels updatecount updatetime ut vb vbs vdir ve verbose verbosefile vfile vi viewdir viewoptions viminfo virtualedit visualbell vop wa wak warn wb wc wcm wd weirdinvert wfh wfw wh whichwrap wi wig wildchar wildcharm wildignore wildmenu wildmode wildoptions wim winaltkeys window winfixheight winfixwidth winheight winminheight winminwidth winwidth wiv wiw wm wmh wmnu wmw wop wrap wrapmargin wrapscan write writeany writebackup writedelay ws ww 

" vimOptions: These are the turn-off setting variants {{{2
syn keyword vimOption contained	noacd noai noakm noallowrevins noaltkeymap noanti noantialias noar noarab noarabic noarabicshape noari noarshape noautochdir noautoindent noautoread noautowrite noautowriteall noaw noawa nobackup noballooneval nobeval nobin nobinary nobiosk nobioskey nobk nobl nobomb nobuflisted nocf noci nocin nocindent nocompatible noconfirm noconsk noconskey nocopyindent nocp nocscopetag nocscopeverbose nocst nocsverb nocuc nocul nocursorcolumn nocursorline nodeco nodelcombine nodg nodiff nodigraph nodisable noea noeb noed noedcompatible noek noendofline noeol noequalalways noerrorbells noesckeys noet noex noexpandtab noexrc nofen nofk nofkmap nofoldenable nogd nogdefault noguipty nohid nohidden nohk nohkmap nohkmapp nohkp nohls nohlsearch noic noicon noignorecase noim noimc noimcmdline noimd noincsearch noinf noinfercase noinsertmode nois nojoinspaces nojs nolazyredraw nolbr nolinebreak nolisp nolist noloadplugins nolpl nolz noma nomacatsui nomagic nomh noml nomod nomodeline nomodifiable nomodified nomore nomousef nomousefocus nomousehide nonu nonumber nopaste nopi nopreserveindent nopreviewwindow noprompt nopvw noreadonly noremap norestorescreen norevins nori norightleft norightleftcmd norl norlc noro nors noru noruler nosb nosc noscb noscrollbind noscs nosecure nosft noshellslash noshelltemp noshiftround noshortname noshowcmd noshowfulltag noshowmatch noshowmode nosi nosm nosmartcase nosmartindent nosmarttab nosmd nosn nosol nospell nosplitbelow nosplitright nospr nosr nossl nosta nostartofline nostmp noswapfile noswf nota notagbsearch notagrelative notagstack notbi notbidi notbs notermbidi noterse notextauto notextmode notf notgst notildeop notimeout notitle noto notop notr nottimeout nottybuiltin nottyfast notx novb novisualbell nowa nowarn nowb noweirdinvert nowfh nowfw nowildmenu nowinfixheight nowinfixwidth nowiv nowmnu nowrap nowrapscan nowrite nowriteany nowritebackup nows 

" vimOptions: These are the invertible variants {{{2
syn keyword vimOption contained	invacd invai invakm invallowrevins invaltkeymap invanti invantialias invar invarab invarabic invarabicshape invari invarshape invautochdir invautoindent invautoread invautowrite invautowriteall invaw invawa invbackup invballooneval invbeval invbin invbinary invbiosk invbioskey invbk invbl invbomb invbuflisted invcf invci invcin invcindent invcompatible invconfirm invconsk invconskey invcopyindent invcp invcscopetag invcscopeverbose invcst invcsverb invcuc invcul invcursorcolumn invcursorline invdeco invdelcombine invdg invdiff invdigraph invdisable invea inveb inved invedcompatible invek invendofline inveol invequalalways inverrorbells invesckeys invet invex invexpandtab invexrc invfen invfk invfkmap invfoldenable invgd invgdefault invguipty invhid invhidden invhk invhkmap invhkmapp invhkp invhls invhlsearch invic invicon invignorecase invim invimc invimcmdline invimd invincsearch invinf invinfercase invinsertmode invis invjoinspaces invjs invlazyredraw invlbr invlinebreak invlisp invlist invloadplugins invlpl invlz invma invmacatsui invmagic invmh invml invmod invmodeline invmodifiable invmodified invmore invmousef invmousefocus invmousehide invnu invnumber invpaste invpi invpreserveindent invpreviewwindow invprompt invpvw invreadonly invremap invrestorescreen invrevins invri invrightleft invrightleftcmd invrl invrlc invro invrs invru invruler invsb invsc invscb invscrollbind invscs invsecure invsft invshellslash invshelltemp invshiftround invshortname invshowcmd invshowfulltag invshowmatch invshowmode invsi invsm invsmartcase invsmartindent invsmarttab invsmd invsn invsol invspell invsplitbelow invsplitright invspr invsr invssl invsta invstartofline invstmp invswapfile invswf invta invtagbsearch invtagrelative invtagstack invtbi invtbidi invtbs invtermbidi invterse invtextauto invtextmode invtf invtgst invtildeop invtimeout invtitle invto invtop invtr invttimeout invttybuiltin invttyfast invtx invvb invvisualbell invwa invwarn invwb invweirdinvert invwfh invwfw invwildmenu invwinfixheight invwinfixwidth invwiv invwmnu invwrap invwrapscan invwrite invwriteany invwritebackup invws 

" termcap codes (which can also be set) {{{2
syn keyword vimOption contained	t_AB t_AF t_al t_AL t_bc t_cd t_ce t_Ce t_cl t_cm t_Co t_cs t_Cs t_CS t_CV t_da t_db t_dl t_DL t_EI    end insert mode (block cursor shape)            *t_EI* *'t_EI'* t_F1 t_F2 t_F3 t_F4 t_F5 t_F6 t_F7 t_F8 t_F9 t_fs t_IE t_IS t_k1 t_K1 t_k2 t_k3 t_K3 t_k4 t_K4 t_k5 t_K5 t_k6 t_K6 t_k7 t_K7 t_k8 t_K8 t_k9 t_K9 t_KA t_kb t_kB t_KB t_KC t_kd t_kD t_KD t_ke t_KE t_KF t_KG t_kh t_KH t_kI t_KI t_KJ t_KK t_kl t_KL t_kN t_kP t_kr t_ks t_ku t_le t_mb t_md t_me t_mr t_ms t_nd t_op t_RI t_RV t_Sb t_se t_Sf t_SI    start insert mode (bar cursor shape)            *t_SI* *'t_SI'* t_so t_sr t_te t_ti t_ts t_ue t_us t_ut t_vb t_ve t_vi t_vs t_WP t_WS t_xs t_ZH t_ZR 
syn match   vimOption contained	"t_%1"
syn match   vimOption contained	"t_#2"
syn match   vimOption contained	"t_#4"
syn match   vimOption contained	"t_@7"
syn match   vimOption contained	"t_*7"
syn match   vimOption contained	"t_&8"
syn match   vimOption contained	"t_%i"
syn match   vimOption contained	"t_k;"

" unsupported settings: these are supported by vi but don't do anything in vim {{{2
syn keyword vimErrSetting contained	hardtabs ht w1200 w300 w9600 

" AutoCmd Events {{{2
syn case ignore
syn keyword vimAutoEvent contained	BufAdd BufCreate BufDelete BufEnter BufFilePost BufFilePre BufHidden BufLeave BufNew BufNewFile BufRead BufReadCmd BufReadPost BufReadPre BufUnload BufWinEnter BufWinLeave BufWipeout BufWrite BufWriteCmd BufWritePost BufWritePre Cmd-event CmdwinEnter CmdwinLeave ColorScheme CursorHold CursorHoldI CursorMoved CursorMovedI EncodingChanged FileAppendCmd FileAppendPost FileAppendPre FileChangedRO FileChangedShell FileChangedShellPost FileEncoding FileReadCmd FileReadPost FileReadPre FileType FileWriteCmd FileWritePost FileWritePre FilterReadPost FilterReadPre FilterWritePost FilterWritePre FocusGained FocusLost FuncUndefined GUIEnter InsertChange InsertEnter InsertLeave MenuPopup QuickFixCmdPost QuickFixCmdPre RemoteReply SessionLoadPost ShellCmdPost ShellFilterPost SourcePre SpellFileMissing StdinReadPost StdinReadPre SwapExists Syntax TabEnter TabLeave TermChanged TermResponse User UserGettingBored VimEnter VimLeave VimLeavePre VimResized WinEnter WinLeave 

" Highlight commonly used Groupnames {{{2
syn keyword vimGroup contained	Comment Constant String Character Number Boolean Float Identifier Function Statement Conditional Repeat Label Operator Keyword Exception PreProc Include Define Macro PreCondit Type StorageClass Structure Typedef Special SpecialChar Tag Delimiter SpecialComment Debug Underlined Ignore Error Todo 

" Default highlighting groups {{{2
syn keyword vimHLGroup contained	Cursor CursorColumn CursorIM CursorLine DiffAdd DiffChange DiffDelete DiffText Directory ErrorMsg FoldColumn Folded IncSearch LineNr MatchParen Menu ModeMsg MoreMsg NonText Normal Pmenu PmenuSbar PmenuSel PmenuThumb Question Scrollbar Search SignColumn SpecialKey SpellBad SpellCap SpellLocal SpellRare StatusLine StatusLineNC TabLine TabLineFill TabLineSel Title Tooltip VertSplit Visual VisualNOS WarningMsg WildMenu 
syn match vimHLGroup contained	"Conceal"
syn case match

" Function Names {{{2
syn keyword vimFuncName contained	add append argc argidx argv browse browsedir bufexists buflisted bufloaded bufname bufnr bufwinnr byte2line byteidx call changenr char2nr cindent col complete complete_add complete_check confirm copy count cscope_connection cursor deepcopy delete did_filetype diff_filler diff_hlID empty escape eval eventhandler executable exists expand expr8 extend feedkeys filereadable filewritable filter finddir findfile fnamemodify foldclosed foldclosedend foldlevel foldtext foldtextresult foreground function garbagecollect get getbufline getbufvar getchar getcharmod getcmdline getcmdpos getcmdtype getcwd getfontname getfperm getfsize getftime getftype getline getloclist getpos getqflist getreg getregtype gettabwinvar getwinposx getwinposy getwinvar glob globpath has has_key hasmapto histadd histdel histget histnr hlexists hlID hostname iconv indent index input inputdialog inputlist inputrestore inputsave inputsecret insert isdirectory islocked items join keys len libcall libcallnr line line2byte lispindent localtime map maparg mapcheck match matcharg matchend matchlist matchstr max min mkdir mode nextnonblank nr2char pathshorten prevnonblank printf pumvisible range readfile reltime reltimestr remote_expr remote_foreground remote_peek remote_read remote_send remove rename repeat resolve reverse search searchdecl searchpair searchpairpos searchpos server2client serverlist setbufvar setcmdpos setline setloclist setpos setqflist setreg settabwinvar setwinvar simplify sort soundfold spellbadword spellsuggest split str2nr strftime stridx string strlen strpart strridx strtrans submatch substitute synID synIDattr synIDtrans system tabpagebuflist tabpagenr tabpagewinnr tagfiles taglist tempname tolower toupper tr type values virtcol visualmode winbufnr wincol winheight winline winnr winrestcmd winrestview winsaveview winwidth writefile 

"--- syntax above generated by mkvimvim ---
" Special Vim Highlighting (not automatic) {{{1

" Numbers {{{2
" =======
syn match vimNumber	"\<\d\+\([lL]\|\.\d\+\)\="
syn match vimNumber	"-\d\+\([lL]\|\.\d\+\)\="
syn match vimNumber	"\<0[xX]\x\+"
syn match vimNumber	"#\x\{6}"

" All vimCommands are contained by vimIsCommands. {{{2
syn match vimCmdSep	"[:|]\+"	skipwhite nextgroup=vimAddress,vimAutoCmd,vimCommand,vimExtCmd,vimFilter,vimLet,vimMap,vimMark,vimSet,vimSyntax,vimUserCmd
syn match vimIsCommand	"\<\h\w*\>"	contains=vimCommand
syn match vimVar		"\<[bwglsav]:\K\k*\>"
syn match vimVar contained	"\<\K\k*\>"
syn keyword vimCommand contained	in

" Insertions And Appends: insert append {{{2
" =======================
syn region vimInsert	matchgroup=vimCommand start="^[: \t]*\(\d\+\(,\d\+\)\=\)\=a\%[ppend]$"	matchgroup=vimCommand end="^\.$""
syn region vimInsert	matchgroup=vimCommand start="^[: \t]*\(\d\+\(,\d\+\)\=\)\=c\%[hange]$"	matchgroup=vimCommand end="^\.$""
syn region vimInsert	matchgroup=vimCommand start="^[: \t]*\(\d\+\(,\d\+\)\=\)\=i\%[nsert]$"	matchgroup=vimCommand end="^\.$""

" Behave! {{{2
" =======
syn match   vimBehave	"\<be\%[have]\>" skipwhite nextgroup=vimBehaveModel,vimBehaveError
syn keyword vimBehaveModel contained	mswin	xterm
if !exists("g:vimsyntax_noerror")
 syn match   vimBehaveError contained	"[^ ]\+"
endif

" Filetypes {{{2
" =========
syn match   vimFiletype	"\<filet\%[ype]\(\s\+\I\i*\)*"	skipwhite contains=vimFTCmd,vimFTOption,vimFTError
if !exists("g:vimsyntax_noerror")
 syn match   vimFTError  contained	"\I\i*"
endif
syn keyword vimFTCmd    contained	filet[ype]
syn keyword vimFTOption contained	detect indent off on plugin

" Augroup : vimAugroupError removed because long augroups caused sync'ing problems. {{{2
" ======= : Trade-off: Increasing synclines with slower editing vs augroup END error checking.
syn cluster vimAugroupList	contains=vimIsCommand,vimFunction,vimFunctionError,vimLineComment,vimSpecFile,vimOper,vimNumber,vimComment,vimString,vimSubst,vimMark,vimRegister,vimAddress,vimFilter,vimCmplxRepeat,vimComment,vimLet,vimSet,vimAutoCmd,vimRegion,vimSynLine,vimNotation,vimCtrlChar,vimFuncVar,vimContinue
syn region  vimAugroup	start="\<aug\%[roup]\>\s\+\K\k*" end="\<aug\%[roup]\>\s\+[eE][nN][dD]\>"	contains=vimAugroupKey,vimAutoCmd,@vimAugroupList keepend
syn match   vimAugroup	"aug\%[roup]!" contains=vimAugroupKey
if !exists("g:vimsyntax_noerror")
 syn match   vimAugroupError	"\<aug\%[roup]\>\s\+[eE][nN][dD]\>"
endif
syn keyword vimAugroupKey contained	aug[roup]

" Functions : Tag is provided for those who wish to highlight tagged functions {{{2
" =========
syn cluster vimFuncList	contains=vimCommand,vimFuncKey,Tag,vimFuncSID
syn cluster vimFuncBodyList	contains=vimIsCommand,vimFunction,vimFunctionError,vimFuncBody,vimLineComment,vimSpecFile,vimOper,vimNumber,vimComment,vimString,vimSubst,vimMark,vimRegister,vimAddress,vimFilter,vimCmplxRepeat,vimComment,vimLet,vimSet,vimAutoCmd,vimRegion,vimSynLine,vimNotation,vimCtrlChar,vimFuncVar,vimContinue
if !exists("g:vimsyntax_noerror")
 syn match   vimFunctionError	"\<fu\%[nction]!\=\s\+\zs\U\i\{-}\ze\s*("                	contains=vimFuncKey,vimFuncBlank nextgroup=vimFuncBody
endif
syn match   vimFunction	"\<fu\%[nction]!\=\s\+\(<[sS][iI][dD]>\|[Ss]:\|\u\)\i*\ze\s*("	contains=@vimFuncList nextgroup=vimFuncBody
syn region  vimFuncBody  contained	start=")"	end="\<endf\%[unction]"		contains=@vimFuncBodyList
syn match   vimFuncVar   contained	"a:\(\I\i*\|\d\+\)"
syn match   vimFuncSID   contained	"\c<sid>\|\<s:"
syn keyword vimFuncKey   contained	fu[nction]
syn match   vimFuncBlank contained	"\s\+"

syn keyword vimPattern   contained	start	skip	end

" Operators: {{{2
" =========
syn cluster vimOperGroup	contains=vimOper,vimOperParen,vimNumber,vimString,vimRegister,vimContinue
syn match  vimOper	"\(==\|!=\|>=\|<=\|=\~\|!\~\|>\|<\|=\)[?#]\{0,2}"	skipwhite nextgroup=vimString,vimSpecFile
syn match  vimOper	"||\|&&\|[-+.]"	skipwhite nextgroup=vimString,vimSpecFile
syn region vimOperParen 	oneline matchgroup=vimOper start="(" end=")" contains=@vimOperGroup
syn region vimOperParen	oneline matchgroup=vimSep  start="{" end="}" contains=@vimOperGroup nextgroup=vimVar
if !exists("g:vimsyntax_noerror")
 syn match  vimOperError	")"
endif

" Special Filenames, Modifiers, Extension Removal: {{{2
" ===============================================
syn match vimSpecFile	"<c\(word\|WORD\)>"	nextgroup=vimSpecFileMod,vimSubst
syn match vimSpecFile	"<\([acs]file\|amatch\|abuf\)>"	nextgroup=vimSpecFileMod,vimSubst
syn match vimSpecFile	"\s%[ \t:]"ms=s+1,me=e-1	nextgroup=vimSpecFileMod,vimSubst
syn match vimSpecFile	"\s%$"ms=s+1	nextgroup=vimSpecFileMod,vimSubst
syn match vimSpecFile	"\s%<"ms=s+1,me=e-1	nextgroup=vimSpecFileMod,vimSubst
syn match vimSpecFile	"#\d\+\|[#%]<\>"	nextgroup=vimSpecFileMod,vimSubst
syn match vimSpecFileMod	"\(:[phtre]\)\+"	contained

" User-Specified Commands: {{{2
" =======================
syn cluster vimUserCmdList	contains=vimAddress,vimSyntax,vimHighlight,vimAutoCmd,vimCmplxRepeat,vimComment,vimCtrlChar,vimEscapeBrace,vimFilter,vimFunc,vimFunction,vimIsCommand,vimMark,vimNotation,vimNumber,vimOper,vimRegion,vimRegister,vimLet,vimSet,vimSetEqual,vimSetString,vimSpecFile,vimString,vimSubst,vimSubstRep,vimSubstRange,vimSynLine
syn keyword vimUserCommand	contained	com[mand]
syn match   vimUserCmd	"\<com\%[mand]!\=\>.*$"	contains=vimUserAttrb,vimUserCommand,@vimUserCmdList
syn match   vimUserAttrb	contained	"-n\%[args]=[01*?+]"	contains=vimUserAttrbKey,vimOper
syn match   vimUserAttrb	contained	"-com\%[plete]="	contains=vimUserAttrbKey,vimOper nextgroup=vimUserAttrbCmplt,vimUserCmdError
syn match   vimUserAttrb	contained	"-ra\%[nge]\(=%\|=\d\+\)\="	contains=vimNumber,vimOper,vimUserAttrbKey
syn match   vimUserAttrb	contained	"-cou\%[nt]=\d\+"	contains=vimNumber,vimOper,vimUserAttrbKey
syn match   vimUserAttrb	contained	"-bang\=\>"	contains=vimOper,vimUserAttrbKey
syn match   vimUserAttrb	contained	"-bar\>"	contains=vimOper,vimUserAttrbKey
syn match   vimUserAttrb	contained	"-re\%[gister]\>"	contains=vimOper,vimUserAttrbKey
if !exists("g:vimsyntax_noerror")
 syn match   vimUserCmdError	contained	"\S\+\>"
endif
syn case ignore
syn keyword vimUserAttrbKey   contained	bar	ban[g]	cou[nt]	ra[nge] com[plete]	n[args]	re[gister]
syn keyword vimUserAttrbCmplt contained	augroup buffer command dir environment event expression file function help highlight mapping menu option something tag tag_listfiles var
syn keyword vimUserAttrbCmplt contained	custom customlist nextgroup=vimUserAttrbCmpltFunc,vimUserCmdError
syn match   vimUserAttrbCmpltFunc contained	",\%(\h\w*\%(#\u\w*\)\+\|\u\w*\)"hs=s+1 nextgroup=vimUserCmdError

syn case match
syn match   vimUserAttrbCmplt contained	"custom,\u\w*"

" Errors: {{{2
" ======
if !exists("g:vimsyntax_noerror")
 syn match  vimElseIfErr	"\<else\s\+if\>"
endif

" Lower Priority Comments: after some vim commands... {{{2
" =======================
syn match  vimComment	excludenl +\s"[^\-:.%#=*].*$+lc=1	contains=@vimCommentGroup,vimCommentString
syn match  vimComment	+\<endif\s\+".*$+lc=5	contains=@vimCommentGroup,vimCommentString
syn match  vimComment	+\<else\s\+".*$+lc=4	contains=@vimCommentGroup,vimCommentString
syn region vimCommentString	contained oneline start='\S\s\+"'ms=e	end='"'

" Environment Variables: {{{2
" =====================
syn match vimEnvvar	"\$\I\i*"
syn match vimEnvvar	"\${\I\i*}"

" In-String Specials: {{{2
" Try to catch strings, if nothing else matches (therefore it must precede the others!)
"  vimEscapeBrace handles ["]  []"] (ie. "s don't terminate string inside [])
syn region vimEscapeBrace	oneline   contained transparent start="[^\\]\(\\\\\)*\[\zs\^\=\]\=" skip="\\\\\|\\\]" end="]"me=e-1
syn match  vimPatSepErr	contained	"\\)"
syn match  vimPatSep	contained	"\\|"
syn region vimPatSepZone	oneline   contained   matchgroup=vimPatSepZ start="\\%\=\ze(" skip="\\\\" end="\\)\|[^\]['"]"	contains=@vimStringGroup
syn region vimPatRegion	contained transparent matchgroup=vimPatSepR start="\\[z%]\=(" end="\\)"	contains=@vimSubstList oneline
syn match  vimNotPatSep	contained	"\\\\"
syn cluster vimStringGroup	contains=vimEscapeBrace,vimPatSep,vimNotPatSep,vimPatSepErr,vimPatSepZone
syn region vimString	oneline keepend	start=+[^:a-zA-Z>!\\@]"+lc=1 skip=+\\\\\|\\"+ end=+"+	contains=@vimStringGroup
syn region vimString	oneline keepend	start=+[^:a-zA-Z>!\\@]'+lc=1 end=+'+
syn region vimString	oneline	start=+=!+lc=1	skip=+\\\\\|\\!+ end=+!+	contains=@vimStringGroup
syn region vimString	oneline	start="=+"lc=1	skip="\\\\\|\\+" end="+"	contains=@vimStringGroup
syn region vimString	oneline	start="\s/\s*\A"lc=1 skip="\\\\\|\\+" end="/"	contains=@vimStringGroup
syn match  vimString	contained	+"[^"]*\\$+	skipnl nextgroup=vimStringCont
syn match  vimStringCont	contained	+\(\\\\\|.\)\{-}[^\\]"+

" Substitutions: {{{2
" =============
syn cluster vimSubstList	contains=vimPatSep,vimPatRegion,vimPatSepErr,vimSubstTwoBS,vimSubstRange,vimNotation
syn cluster vimSubstRepList	contains=vimSubstSubstr,vimSubstTwoBS,vimNotation
syn cluster vimSubstList	add=vimCollection
syn match   vimSubst	"\(:\+\s*\|^\s*\||\s*\)\<s\%[ubstitute][:[:alpha:]]\@!" nextgroup=vimSubstPat
syn match   vimSubst	"s\%[ubstitute][:[:alpha:]]\@!"	nextgroup=vimSubstPat contained
syn match   vimSubst	"/\zss\%[ubstitute]\ze/"	nextgroup=vimSubstPat
syn match   vimSubst1       contained	"s\%[ubstitute]\>"	nextgroup=vimSubstPat
syn region  vimSubstPat     contained	matchgroup=vimSubstDelim start="\z([^a-zA-Z( \t[\]&]\)"rs=s+1 skip="\\\\\|\\\z1" end="\z1"re=e-1,me=e-1	 contains=@vimSubstList	nextgroup=vimSubstRep4	oneline
syn region  vimSubstRep4    contained	matchgroup=vimSubstDelim start="\z(.\)" skip="\\\\\|\\\z1" end="\z1" matchgroup=vimNotation end="<[cC][rR]>" contains=@vimSubstRepList	nextgroup=vimSubstFlagErr	oneline
syn region  vimCollection   contained transparent	start="\\\@<!\[" skip="\\\[" end="\]"	contains=vimCollClass
syn match   vimCollClassErr contained	"\[:.\{-\}:\]"
syn match   vimCollClass    contained transparent	"\[:\(alnum\|alpha\|blank\|cntrl\|digit\|graph\|lower\|print\|punct\|space\|upper\|xdigit\|return\|tab\|escape\|backspace\):\]"
syn match   vimSubstSubstr  contained	"\\z\=\d"
syn match   vimSubstTwoBS   contained	"\\\\"
syn match   vimSubstFlagErr contained	"[^< \t\r|]\+" contains=vimSubstFlags
syn match   vimSubstFlags   contained	"[&cegiIpr]\+"

" 'String': {{{2
syn match  vimString	"[^(,]'[^']\{-}\zs'"

" Marks, Registers, Addresses, Filters: {{{2
syn match  vimMark	"'[a-zA-Z0-9]\ze[-+,!]"	nextgroup=vimOper,vimMarkNumber,vimSubst
syn match  vimMark	"'[<>]\ze[-+,!]"		nextgroup=vimOper,vimMarkNumber,vimSubst
syn match  vimMark	",\zs'[<>]\ze"		nextgroup=vimOper,vimMarkNumber,vimSubst
syn match  vimMark	"[!,:]\zs'[a-zA-Z0-9]"	nextgroup=vimOper,vimMarkNumber,vimSubst
syn match  vimMark	"\<norm\%[al]\s\zs'[a-zA-Z0-9]"	nextgroup=vimOper,vimMarkNumber,vimSubst
syn match  vimMarkNumber	"[-+]\d\+"		nextgroup=vimSubst contained contains=vimOper
syn match  vimPlainMark contained	"'[a-zA-Z0-9]"

syn match  vimRegister	'[^,;]\zs"[a-zA-Z0-9.%#:_\-/][^a-zA-Z_"]\ze'
syn match  vimRegister	'\<norm\s\+\zs"[a-zA-Z0-9]'
syn match  vimRegister	'\<normal\s\+\zs"[a-zA-Z0-9]'
syn match  vimRegister	'@"'
syn match  vimPlainRegister contained	'"[a-zA-Z0-9\-:.%#*+=]'

syn match  vimAddress	",\zs[.$]"	skipwhite nextgroup=vimSubst1
syn match  vimAddress	"%\ze\a"	skipwhite nextgroup=vimString,vimSubst1

syn match  vimFilter contained	"^!.\{-}\(|\|$\)"		contains=vimSpecFile
syn match  vimFilter contained	"\A!.\{-}\(|\|$\)"ms=s+1	contains=vimSpecFile

" Complex repeats (:h complex-repeat) {{{2
"syn match  vimCmplxRepeat	'[^a-zA-Z_/\\()]q[0-9a-zA-Z"]'lc=1
"syn match  vimCmplxRepeat	'@[0-9a-z".=@:]\ze\($\|[^a-zA-Z]\)'

" Set command and associated set-options (vimOptions) with comment {{{2
syn region vimSet		matchgroup=vimCommand start="\<\%(setl\%[ocal]\|setg\%[lobal]\|set\)\>" skip="\%(\\\\\)*\\." end="$" matchgroup=vimNotation end="<[cC][rR]>" keepend oneline contains=vimSetEqual,vimOption,vimErrSetting,vimComment,vimSetString,vimSetMod
syn region vimSetEqual  contained	start="="	skip="\\\\\|\\\s" end="[| \t]\|$"me=e-1 contains=vimCtrlChar,vimSetSep,vimNotation oneline
syn region vimSetString contained	start=+="+hs=s+1	skip=+\\\\\|\\"+  end=+"+	contains=vimCtrlChar
syn match  vimSetSep    contained	"[,:]"
syn match  vimSetMod	contained	"&vim\|[!&]\|all&"

" Let {{{2
" ===
syn keyword vimLet	let	unl[et]	skipwhite nextgroup=vimVar

" Autocmd {{{2
" =======
syn match   vimAutoEventList	contained	"\(!\s\+\)\=\(\a\+,\)*\a\+"	contains=vimAutoEvent nextgroup=vimAutoCmdSpace
syn match   vimAutoCmdSpace	contained	"\s\+"	nextgroup=vimAutoCmdSfxList
syn match   vimAutoCmdSfxList	contained	"\S*"
syn keyword vimAutoCmd	au[tocmd] do[autocmd] doautoa[ll]	skipwhite nextgroup=vimAutoEventList

" Echo and Execute -- prefer strings! {{{2
" ================
syn region  vimEcho	oneline excludenl matchgroup=vimCommand start="\<ec\%[ho]\>" skip="\(\\\\\)*\\|" end="$\||" contains=vimFunc,vimString,varVar
syn region  vimExecute	oneline excludenl matchgroup=vimCommand start="\<exe\%[cute]\>" skip="\(\\\\\)*\\|" end="$\||\|<[cC][rR]>" contains=vimIsCommand,vimString,vimOper,vimVar,vimNotation,vimOperParen
syn match   vimEchoHL	"echohl\="	skipwhite nextgroup=vimGroup,vimHLGroup,vimEchoHLNone
syn case ignore
syn keyword vimEchoHLNone	none
syn case match

" Maps {{{2
" ====
syn match   vimMap	"\<map!\=\ze\s*[^(]" skipwhite nextgroup=vimMapMod,vimMapLhs
syn keyword vimMap	cm[ap] cno[remap] im[ap] ino[remap] nm[ap] nn[oremap] no[remap] om[ap] ono[remap] vm[ap] vn[oremap] skipwhite nextgroup=vimMapBang,vimMapMod,vimMapLhs
syn match   vimMapLhs    contained	"\S\+"	contains=vimNotation,vimCtrlChar skipwhite nextgroup=vimMapRhs
syn match   vimMapBang   contained	"!"	skipwhite nextgroup=vimMapMod,vimMapLhs
syn match   vimMapMod    contained	"\c<\(buffer\|expr\|\(local\)\=leader\|plug\|script\|sid\|unique\|silent\)\+>" contains=vimMapModKey,vimMapModErr skipwhite nextgroup=vimMapMod,vimMapLhs
syn match   vimMapRhs    contained  ".*" contains=vimNotation,vimCtrlChar skipnl nextgroup=vimMapRhsExtend
syn match   vimMapRhsExtend contained "^\s*\\.*$" contains=vimContinue
syn case ignore
syn keyword vimMapModKey contained	buffer	expr	leader	localleader	plug	script	sid	silent	unique
syn case match

" Menus {{{2
" =====
syn cluster vimMenuList contains=vimMenuBang,vimMenuPriority,vimMenuName,vimMenuMod
syn keyword vimCommand	am[enu] an[oremenu] aun[menu] cme[nu] cnoreme[nu] cunme[nu] ime[nu] inoreme[nu] iunme[nu] me[nu] nme[nu] nnoreme[nu] noreme[nu] nunme[nu] ome[nu] onoreme[nu] ounme[nu] unme[nu] vme[nu] vnoreme[nu] vunme[nu] skipwhite nextgroup=@vimMenuList
syn match   vimMenuName	"[^ \t\\<]\+"	contained nextgroup=vimMenuNameMore,vimMenuMap
syn match   vimMenuPriority	"\d\+\(\.\d\+\)*"	contained skipwhite nextgroup=vimMenuName
syn match   vimMenuNameMore	"\c\\\s\|<tab>\|\\\."	contained nextgroup=vimMenuName,vimMenuNameMore contains=vimNotation
syn match   vimMenuMod    contained	"\c<\(script\|silent\)\+>"  skipwhite contains=vimMapModKey,vimMapModErr nextgroup=@vimMenuList
syn match   vimMenuMap	"\s"	contained skipwhite nextgroup=vimMenuRhs
syn match   vimMenuRhs	".*$"	contained contains=vimString,vimComment,vimIsCommand
syn match   vimMenuBang	"!"	contained skipwhite nextgroup=@vimMenuList

" Angle-Bracket Notation (tnx to Michael Geddes) {{{2
" ======================
syn case ignore
syn match vimNotation	"\(\\\|<lt>\)\=<\([scam]-\)\{0,4}x\=\(f\d\{1,2}\|[^ \t:]\|cr\|lf\|linefeed\|return\|k\=del\%[ete]\|bs\|backspace\|tab\|esc\|right\|left\|help\|undo\|insert\|ins\|k\=home\|k\=end\|kplus\|kminus\|kdivide\|kmultiply\|kenter\|space\|k\=\(page\)\=\(\|down\|up\)\)>" contains=vimBracket
syn match vimNotation	"\(\\\|<lt>\)\=<\([scam2-4]-\)\{0,4}\(right\|left\|middle\)\(mouse\)\=\(drag\|release\)\=>"	contains=vimBracket
syn match vimNotation	"\(\\\|<lt>\)\=<\(bslash\|plug\|sid\|space\|bar\|nop\|nul\|lt\)>"		contains=vimBracket
syn match vimNotation	'\(\\\|<lt>\)\=<C-R>[0-9a-z"%#:.\-=]'he=e-1			contains=vimBracket
syn match vimNotation	'\(\\\|<lt>\)\=<\%(q-\)\=\(line[12]\|count\|bang\|reg\|args\|f-args\|lt\)>'	contains=vimBracket
syn match vimNotation	"\(\\\|<lt>\)\=<\([cas]file\|abuf\|amatch\|cword\|cWORD\|client\)>"		contains=vimBracket
syn match vimBracket contained	"[\\<>]"
syn case match

" User Function Highlighting (following Gautam Iyer's suggestion) {{{2
" ==========================
syn match vimFunc		"\%([sS]:\|<[sS][iI][dD]>\)\=\I\i*\ze\s*("	contains=vimFuncName,vimUserFunc,vimCommand,vimNotFunc,vimExecute
syn match vimUserFunc contained	"\%([sS]:\|<[sS][iI][dD]>\)\i\+\|\<\u\i*\>\|\<if\>"	contains=vimNotation,vimCommand
syn match vimNotFunc  contained	"\<[aiAIrR]("

" Norm
" ====
syn match vimNorm		"\<norm\%[al]!\=" skipwhite nextgroup=vimNormCmds
syn match vimNormCmds contained	".*$"

" Syntax {{{2
"=======
syn match   vimGroupList	contained	"@\=[^ \t,]*"	contains=vimGroupSpecial,vimPatSep
syn match   vimGroupList	contained	"@\=[^ \t,]*,"	nextgroup=vimGroupList contains=vimGroupSpecial,vimPatSep
syn keyword vimGroupSpecial	contained	ALL	ALLBUT
if !exists("g:vimsyntax_noerror")
 syn match   vimSynError	contained	"\i\+"
 syn match   vimSynError	contained	"\i\+="	nextgroup=vimGroupList
endif
syn match   vimSynContains	contained	"\<contain\(s\|edin\)="	nextgroup=vimGroupList
syn match   vimSynKeyContainedin	contained	"\<containedin="	nextgroup=vimGroupList
syn match   vimSynNextgroup	contained	"nextgroup="	nextgroup=vimGroupList

syn match   vimSyntax	"\<sy\%[ntax]\>"	contains=vimCommand skipwhite nextgroup=vimSynType,vimComment
syn match   vimAuSyntax	contained	"\s+sy\%[ntax]"	contains=vimCommand skipwhite nextgroup=vimSynType,vimComment

" Syntax: case {{{2
syn keyword vimSynType	contained	case	skipwhite nextgroup=vimSynCase,vimSynCaseError
if !exists("g:vimsyntax_noerror")
 syn match   vimSynCaseError	contained	"\i\+"
endif
syn keyword vimSynCase	contained	ignore	match

" Syntax: clear {{{2
syn keyword vimSynType	contained	clear	skipwhite nextgroup=vimGroupList

" Syntax: cluster {{{2
syn keyword vimSynType	contained	cluster	skipwhite nextgroup=vimClusterName
syn region  vimClusterName	contained	matchgroup=vimGroupName start="\k\+" skip="\\\\\|\\|" matchgroup=vimSep end="$\||" contains=vimGroupAdd,vimGroupRem,vimSynContains,vimSynError
syn match   vimGroupAdd	contained	"add="	nextgroup=vimGroupList
syn match   vimGroupRem	contained	"remove="	nextgroup=vimGroupList

" Syntax: include {{{2
syn keyword vimSynType	contained	include	skipwhite nextgroup=vimGroupList

" Syntax: keyword {{{2
syn cluster vimSynKeyGroup	contains=vimSynNextgroup,vimSynKeyOpt,vimSynKeyContainedin
syn keyword vimSynType	contained	keyword	skipwhite nextgroup=vimSynKeyRegion
syn region  vimSynKeyRegion	contained keepend	matchgroup=vimGroupName start="\k\+" skip="\\\\\|\\|" matchgroup=vimSep end="|\|$" contains=@vimSynKeyGroup
syn match   vimSynKeyOpt	contained	"\<\(conceal\|contained\|transparent\|skipempty\|skipwhite\|skipnl\)\>"

" Syntax: match {{{2
syn cluster vimSynMtchGroup	contains=vimMtchComment,vimSynContains,vimSynError,vimSynMtchOpt,vimSynNextgroup,vimSynRegPat,vimNotation
syn keyword vimSynType	contained	match	skipwhite nextgroup=vimSynMatchRegion
syn region  vimSynMatchRegion	contained keepend	matchgroup=vimGroupName start="\k\+" matchgroup=vimSep end="|\|$" contains=@vimSynMtchGroup
syn match   vimSynMtchOpt	contained	"\<\(conceal\|transparent\|contained\|excludenl\|skipempty\|skipwhite\|display\|extend\|skipnl\|fold\)\>"
if has("conceal")
 syn match   vimSynMtchOpt	contained	"\<cchar="	nextgroup=VimSynMtchCchar
 syn match   vimSynMtchCchar	contained	"."
endif

" Syntax: off and on {{{2
syn keyword vimSynType	contained	enable	list	manual	off	on	reset

" Syntax: region {{{2
syn cluster vimSynRegPatGroup	contains=vimPatSep,vimNotPatSep,vimSynPatRange,vimSynNotPatRange,vimSubstSubstr,vimPatRegion,vimPatSepErr,vimNotation
syn cluster vimSynRegGroup	contains=vimSynContains,vimSynNextgroup,vimSynRegOpt,vimSynReg,vimSynMtchGrp
syn keyword vimSynType	contained	region	skipwhite nextgroup=vimSynRegion
syn region  vimSynRegion	contained keepend	matchgroup=vimGroupName start="\k\+" skip="\\\\\|\\|" end="|\|$" contains=@vimSynRegGroup
syn match   vimSynRegOpt	contained	"\<\(conceal\(ends\)\=\|transparent\|contained\|excludenl\|skipempty\|skipwhite\|display\|keepend\|oneline\|extend\|skipnl\|fold\)\>"
syn match   vimSynReg	contained	"\(start\|skip\|end\)="he=e-1	nextgroup=vimSynRegPat
syn match   vimSynMtchGrp	contained	"matchgroup="	nextgroup=vimGroup,vimHLGroup
syn region  vimSynRegPat	contained extend	start="\z([-`~!@#$%^&*_=+;:'",./?]\)"  skip="\\\\\|\\\z1"  end="\z1"  contains=@vimSynRegPatGroup skipwhite nextgroup=vimSynPatMod,vimSynReg
syn match   vimSynPatMod	contained	"\(hs\|ms\|me\|hs\|he\|rs\|re\)=[se]\([-+]\d\+\)\="
syn match   vimSynPatMod	contained	"\(hs\|ms\|me\|hs\|he\|rs\|re\)=[se]\([-+]\d\+\)\=," nextgroup=vimSynPatMod
syn match   vimSynPatMod	contained	"lc=\d\+"
syn match   vimSynPatMod	contained	"lc=\d\+," nextgroup=vimSynPatMod
syn region  vimSynPatRange	contained	start="\["	skip="\\\\\|\\]"   end="]"
syn match   vimSynNotPatRange	contained	"\\\\\|\\\["
syn match   vimMtchComment	contained	'"[^"]\+$'

" Syntax: sync {{{2
" ============
syn keyword vimSynType	contained	sync	skipwhite	nextgroup=vimSyncC,vimSyncLines,vimSyncMatch,vimSyncError,vimSyncLinebreak,vimSyncLinecont,vimSyncRegion
if !exists("g:vimsyntax_noerror")
 syn match   vimSyncError	contained	"\i\+"
endif
syn keyword vimSyncC	contained	ccomment	clear	fromstart
syn keyword vimSyncMatch	contained	match	skipwhite	nextgroup=vimSyncGroupName
syn keyword vimSyncRegion	contained	region	skipwhite	nextgroup=vimSynReg
syn match   vimSyncLinebreak	contained	"\<linebreaks="	skipwhite	nextgroup=vimNumber
syn keyword vimSyncLinecont	contained	linecont	skipwhite	nextgroup=vimSynRegPat
syn match   vimSyncLines	contained	"\(min\|max\)\=lines="	nextgroup=vimNumber
syn match   vimSyncGroupName	contained	"\k\+"	skipwhite	nextgroup=vimSyncKey
syn match   vimSyncKey	contained	"\<groupthere\|grouphere\>"	skipwhite nextgroup=vimSyncGroup
syn match   vimSyncGroup	contained	"\k\+"	skipwhite	nextgroup=vimSynRegPat,vimSyncNone
syn keyword vimSyncNone	contained	NONE

" Additional IsCommand, here by reasons of precedence {{{2
" ====================
syn match vimIsCommand	"<Bar>\s*\a\+"	transparent contains=vimCommand,vimNotation

" Highlighting {{{2
" ============
syn cluster vimHighlightCluster	contains=vimHiLink,vimHiClear,vimHiKeyList,vimComment
syn match   vimHighlight	"\<hi\%[ghlight]\>" skipwhite nextgroup=vimHiBang,@vimHighlightCluster
syn match   vimHiBang	contained	"!"	  skipwhite nextgroup=@vimHighlightCluster

syn match   vimHiGroup	contained	"\i\+"
syn case ignore
syn keyword vimHiAttrib	contained	none bold inverse italic reverse standout underline undercurl
syn keyword vimFgBgAttrib	contained	none bg background fg foreground
syn case match
syn match   vimHiAttribList	contained	"\i\+"	contains=vimHiAttrib
syn match   vimHiAttribList	contained	"\i\+,"he=e-1	contains=vimHiAttrib nextgroup=vimHiAttribList
syn case ignore
syn keyword vimHiCtermColor	contained	black blue brown cyan darkBlue darkcyan darkgray darkgreen darkgrey darkmagenta darkred darkyellow gray green grey lightblue lightcyan lightgray lightgreen lightgrey lightmagenta lightred magenta red white yellow

syn case match
syn match   vimHiFontname	contained	"[a-zA-Z\-*]\+"
syn match   vimHiGuiFontname	contained	"'[a-zA-Z\-* ]\+'"
syn match   vimHiGuiRgb	contained	"#\x\{6}"
if !exists("g:vimsyntax_noerror")
 syn match   vimHiCtermError	contained	"[^0-9]\i*"
endif

" Highlighting: hi group key=arg ... {{{2
syn cluster vimHiCluster contains=vimHiGroup,vimHiTerm,vimHiCTerm,vimHiStartStop,vimHiCtermFgBg,vimHiGui,vimHiGuiFont,vimHiGuiFgBg,vimHiKeyError,vimNotation
syn region vimHiKeyList	contained oneline start="\i\+" skip="\\\\\|\\|" end="$\||"	contains=@vimHiCluster
if !exists("g:vimsyntax_noerror")
 syn match  vimHiKeyError	contained	"\i\+="he=e-1
endif
syn match  vimHiTerm	contained	"\cterm="he=e-1		nextgroup=vimHiAttribList
syn match  vimHiStartStop	contained	"\c\(start\|stop\)="he=e-1	nextgroup=vimHiTermcap,vimOption
syn match  vimHiCTerm	contained	"\ccterm="he=e-1		nextgroup=vimHiAttribList
syn match  vimHiCtermFgBg	contained	"\ccterm[fb]g="he=e-1	nextgroup=vimNumber,vimHiCtermColor,vimFgBgAttrib,vimHiCtermError
syn match  vimHiGui	contained	"\cgui="he=e-1		nextgroup=vimHiAttribList
syn match  vimHiGuiFont	contained	"\cfont="he=e-1		nextgroup=vimHiFontname
syn match  vimHiGuiFgBg	contained	"\cgui\%([fb]g\|sp\)="he=e-1	nextgroup=vimHiGroup,vimHiGuiFontname,vimHiGuiRgb,vimFgBgAttrib
syn match  vimHiTermcap	contained	"\S\+"		contains=vimNotation

" Highlight: clear {{{2
syn keyword vimHiClear	contained	clear	nextgroup=vimHiGroup

" Highlight: link {{{2
syn region vimHiLink	contained oneline matchgroup=vimCommand start="\<\(def\s\+\)\=link\>\|\<def\>" end="$"	contains=vimHiGroup,vimGroup,vimHLGroup,vimNotation

" Control Characters {{{2
" ==================
syn match vimCtrlChar	"[--]"

" Beginners - Patterns that involve ^ {{{2
" =========
syn match  vimLineComment	+^[ \t:]*".*$+	contains=@vimCommentGroup,vimCommentString,vimCommentTitle
syn match  vimCommentTitle	'"\s*\u\w*\(\s\+\u\w*\)*:'hs=s+1	contained contains=vimCommentTitleLeader,vimTodo,@vimCommentGroup
syn match  vimContinue	"^\s*\\"
syn region vimString	start="^\s*\\\z(['"]\)" skip='\\\\\|\\\z1' end="\z1" oneline keepend contains=@vimStringGroup,vimContinue
syn match  vimCommentTitleLeader	'"\s\+'ms=s+1	contained

" Searches And Globals: {{{2
" ====================
syn match vimSearch	'^\s*[/?].*'		contains=vimSearchDelim
syn match vimSearchDelim	'^\s*\zs[/?]\|[/?]$'	contained
syn region vimGlobal	matchgroup=Statement start='\<g\%[lobal]!\=/' skip='\\.' end='/'
syn region vimGlobal	matchgroup=Statement start='\<v\%[global]!\=/' skip='\\.' end='/'

" Scripts  : perl,ruby : Benoit Cerrina {{{2
" =======    python,tcl: Johannes Zellner

" allow users to prevent embedded script syntax highlighting
" when vim doesn't have perl/python/ruby/tcl support.  Do
" so by setting g:vimembedscript= 0 in the user's <.vimrc>.
if !exists("g:vimembedscript")
 let g:vimembedscript= 1
endif

" [-- perl --] {{{3
if (has("perl") || g:vimembedscript) && filereadable(expand("<sfile>:p:h")."/perl.vim")
 unlet! b:current_syntax
 syn include @vimPerlScript <sfile>:p:h/perl.vim
 syn region vimPerlRegion matchgroup=vimScriptDelim start=+pe\%[rl]\s*<<\s*\z(.*\)$+ end=+^\z1$+ contains=@vimPerlScript
 syn region vimPerlRegion matchgroup=vimScriptDelim start=+pe\%[rl]\s*<<\s*$+ end=+\.$+ contains=@vimPerlScript
endif

" [-- ruby --] {{{3
if (has("ruby") || g:vimembedscript) && filereadable(expand("<sfile>:p:h")."/ruby.vim")
 unlet! b:current_syntax
 syn include @vimRubyScript <sfile>:p:h/ruby.vim
 syn region vimRubyRegion matchgroup=vimScriptDelim start=+rub[y]\s*<<\s*\z(.*\)$+ end=+^\z1$+ contains=@vimRubyScript
 syn region vimRubyRegion matchgroup=vimScriptDelim start=+rub[y]\s*<<\s*$+ end=+\.$+ contains=@vimRubyScript
endif

" [-- python --] {{{3
if (has("python") || g:vimembedscript) && filereadable(expand("<sfile>:p:h")."/python.vim")
 unlet! b:current_syntax
 syn include @vimPythonScript <sfile>:p:h/python.vim
 syn region vimPythonRegion matchgroup=vimScriptDelim start=+py\%[thon]\s*<<\s*\z(.*\)$+ end=+^\z1$+ contains=@vimPythonScript
 syn region vimPythonRegion matchgroup=vimScriptDelim start=+py\%[thon]\s*<<\s*$+ end=+\.$+ contains=@vimPythonScript
endif

" [-- tcl --] {{{3
if (has("tcl") || g:vimembedscript) && filereadable(expand("<sfile>:p:h")."/tcl.vim")
 unlet! b:current_syntax
 syn include @vimTclScript <sfile>:p:h/tcl.vim
 syn region vimTclRegion matchgroup=vimScriptDelim start=+tc[l]\=\s*<<\s*\z(.*\)$+ end=+^\z1$+ contains=@vimTclScript
 syn region vimTclRegion matchgroup=vimScriptDelim start=+tc[l]\=\s*<<\s*$+ end=+\.$+ contains=@vimTclScript
endif

" [-- mzscheme --] {{{3
if (has("mzscheme") || g:vimembedscript) && filereadable(expand("<sfile>:p:h")."/scheme.vim")
 unlet! b:current_syntax
 let iskKeep= &isk
 syn include @vimMzSchemeScript <sfile>:p:h/scheme.vim
 let &isk= iskKeep
 syn region vimMzSchemeRegion matchgroup=vimScriptDelim start=+mz\%[scheme]\s*<<\s*\z(.*\)$+ end=+^\z1$+ contains=@vimMzSchemeScript
 syn region vimMzSchemeRegion matchgroup=vimScriptDelim start=+mz\%[scheme]\s*<<\s*$+ end=+\.$+ contains=@vimMzSchemeScript
endif

" Synchronize (speed) {{{2
"============
if exists("g:vim_minlines")
 exe "syn sync minlines=".g:vim_minlines
endif
if exists("g:vim_maxlines")
 exe "syn sync maxlines=".g:vim_maxlines
else
 syn sync maxlines=60
endif
syn sync linecont	"^\s\+\\"
syn sync match vimAugroupSyncA	groupthere NONE	"\<aug\%[roup]\>\s\+[eE][nN][dD]"

" Highlighting Settings {{{2
" ====================

hi def link vimAuHighlight	vimHighlight
hi def link vimSubst1	vimSubst
hi def link vimBehaveModel	vimBehave

if !exists("g:vimsyntax_noerror")
 hi def link vimBehaveError	vimError
 hi def link vimCollClassErr	vimError
 hi def link vimErrSetting	vimError
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
endif

hi def link vimAddress	vimMark
hi def link vimAugroupKey	vimCommand
hi def link vimAutoCmdOpt	vimOption
hi def link vimAutoCmd	vimCommand
hi def link vimAutoSet	vimCommand
hi def link vimBehave	vimCommand
hi def link vimCommentString	vimString
hi def link vimCondHL	vimCommand
hi def link vimEchoHLNone	vimGroup
hi def link vimEchoHL	vimCommand
hi def link vimElseif	vimCondHL
hi def link vimFgBgAttrib	vimHiAttrib
hi def link vimFTCmd	vimCommand
hi def link vimFTOption	vimSynType
hi def link vimFuncKey	vimCommand
hi def link vimGroupAdd	vimSynOption
hi def link vimGroupRem	vimSynOption
hi def link vimHiCtermFgBg	vimHiTerm
hi def link vimHiCTerm	vimHiTerm
hi def link vimHighlight	vimCommand
hi def link vimHiGroup	vimGroupName
hi def link vimHiGuiFgBg	vimHiTerm
hi def link vimHiGuiFont	vimHiTerm
hi def link vimHiGuiRgb	vimNumber
hi def link vimHiGui	vimHiTerm
hi def link vimHiStartStop	vimHiTerm
hi def link vimHLGroup	vimGroup
hi def link vimInsert	vimString
hi def link vimKeyCode	vimSpecFile
hi def link vimLet	vimCommand
hi def link vimLineComment	vimComment
hi def link vimMapBang	vimCommand
hi def link vimMapModKey	vimFuncSID
hi def link vimMapMod	vimBracket
hi def link vimMap	vimCommand
hi def link vimMarkNumber	vimNumber
hi def link vimMenuMod	vimMapMod
hi def link vimMenuNameMore	vimMenuName
hi def link vimMtchComment	vimComment
hi def link vimNorm	vimCommand
hi def link vimNotFunc	vimCommand
hi def link vimNotPatSep	vimString
hi def link vimPatSepR	vimPatSep
hi def link vimPatSepZ	vimPatSep
hi def link vimPatSepErr	vimPatSep
hi def link vimPatSepZone	vimString
hi def link vimPlainMark	vimMark
hi def link vimPlainRegister	vimRegister
hi def link vimSearch	vimString
hi def link vimSearchDelim	Statement
hi def link vimSetMod	vimOption
hi def link vimSetString	vimString
hi def link vimSpecFileMod	vimSpecFile
hi def link vimStringCont	vimString
hi def link vimSubstTwoBS	vimString
hi def link vimSubst	vimCommand
hi def link vimSyncGroupName	vimGroupName
hi def link vimSyncGroup	vimGroupName
hi def link vimSynContains	vimSynOption
hi def link vimSynKeyContainedin	vimSynContains
hi def link vimSynKeyOpt	vimSynOption
hi def link vimSynMtchGrp	vimSynOption
hi def link vimSynMtchOpt	vimSynOption
hi def link vimSynNextgroup	vimSynOption
hi def link vimSynNotPatRange	vimSynRegPat
hi def link vimSynPatRange	vimString
hi def link vimSynRegOpt	vimSynOption
hi def link vimSynRegPat	vimString
hi def link vimSyntax	vimCommand
hi def link vimSynType	vimSpecial
hi def link vimUserAttrbCmplt	vimSpecial
hi def link vimUserAttrbKey	vimOption
hi def link vimUserAttrb	vimSpecial
hi def link vimUserCommand	vimCommand
hi def link vimUserFunc	Normal

hi def link vimAutoEvent	Type
hi def link vimBracket	Delimiter
hi def link vimCmplxRepeat	SpecialChar
hi def link vimCommand	Statement
hi def link vimComment	Comment
hi def link vimCommentTitle	PreProc
hi def link vimContinue	Special
hi def link vimCtrlChar	SpecialChar
hi def link vimElseIfErr	Error
hi def link vimEnvvar	PreProc
hi def link vimError	Error
hi def link vimFold	Folded
hi def link vimFuncName	Function
hi def link vimFuncSID	Special
hi def link vimFuncVar	Identifier
hi def link vimGroup	Type
hi def link vimGroupSpecial	Special
hi def link vimHLMod	PreProc
hi def link vimHiAttrib	PreProc
hi def link vimHiTerm	Type
hi def link vimKeyword	Statement
hi def link vimMark	Number
hi def link vimMenuName	PreProc
hi def link vimNotation	Special
hi def link vimNumber	Number
hi def link vimOper	Operator
hi def link vimOption	PreProc
hi def link vimOperError	Error
hi def link vimPatSep	SpecialChar
hi def link vimPattern	Type
hi def link vimRegister	SpecialChar
hi def link vimScriptDelim	Comment
hi def link vimSep	Delimiter
hi def link vimSetSep	Statement
hi def link vimSpecFile	Identifier
hi def link vimSpecial	Type
hi def link vimStatement	Statement
hi def link vimString	String
hi def link vimSubstDelim	Delimiter
hi def link vimSubstFlags	Special
hi def link vimSubstSubstr	SpecialChar
hi def link vimSynCase	Type
hi def link vimSynCaseError	Error
hi def link vimSynError	Error
hi def link vimSynOption	Special
hi def link vimSynReg	Type
hi def link vimSyncC	Type
hi def link vimSyncError	Error
hi def link vimSyncKey	Type
hi def link vimSyncNone	Type
hi def link vimTodo	Todo
hi def link vimUserCmdError	Error
hi def link vimUserAttrbCmpltFunc	Special

" Current Syntax Variable: {{{2
let b:current_syntax = "vim"
" vim:ts=18  fdm=marker
