" Vim syntax file
" Language:	    lftp(1) configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/lftp/
" Latest Revision:  2004-05-22
" arch-tag:	    f2537c49-5d64-42b8-beb4-13a09dd723d2

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Set iskeyword since we need `-' (and potentially others) in keywords.
" For version 5.x: Set it globally
" For version 6.x: Set it locally
if version >= 600
  command -nargs=1 SetIsk setlocal iskeyword=<args>
else
  command -nargs=1 SetIsk set iskeyword=<args>
endif
SetIsk 48-57,97-122,-
delcommand SetIsk

" comments
syn region  lftpComment		display oneline matchgroup=lftpComment start="#" end="$" contains=lftpTodo

" todo
syn keyword lftpTodo		contained TODO FIXME XXX NOTE

" strings
syn region  lftpString		contained display start=+"+ skip=+\\$\|\\"+ end=+"+ end=+$+

" numbers
syn match   lftpNumber		contained display "\<\d\+\(\.\d\+\)\=\>"

" booleans and other things
syn keyword lftpBoolean		contained yes no on off true false

" intervals
syn keyword lftpInterval	contained infinity inf never forever
syn match   lftpInterval	contained "\<\(\d\+\(\.\d\+\)\=[dhms]\)\+\>"

" commands
syn keyword lftpKeywords	alias anon at bookmark cache cat cd chmod close
syn keyword lftpKeywords	cls command debug du echo exit fg find get get1
syn keyword lftpKeywords	glob help history jobs kill lcd lftp lpwd ls
syn keyword lftpKeywords	mget mirror mkdir module
syn keyword lftpKeywords	more mput mrm mv nlist open pget put pwd queue
syn keyword lftpKeywords	quote reget recls rels renlist repeat
syn keyword lftpKeywords	reput rm rmdir scache site source suspend user
syn keyword lftpKeywords	version wait zcat zmore

" settings
syn region  lftpSet		matchgroup=lftpKeywords start="set" end=";" end="$" contains=lftpString,lftpNumber,lftpBoolean,lftpInterval,lftpSettingsPrefix,lftpSettings
syn match   lftpSettingsPrefix	contained '\<\%(bmk\|cache\|cmd\|color\|dns\):'
syn match   lftpSettingsPrefix	contained '\<\%(file\|fish\|ftp\|hftp\):'
syn match   lftpSettingsPrefix	contained '\<\%(http\|https\|mirror\|module\):'
syn match   lftpSettingsPrefix	contained '\<\%(net\|sftp\|ssl\|xfer\):'
" bmk:
syn keyword lftpSettings	contained save-p[asswords]
" cache:
syn keyword lftpSettings	contained cache-em[pty-listings] en[able]
syn keyword lftpSettings	contained exp[ire] siz[e]
" cmd:
syn keyword lftpSettings	contained at[-exit] cls-c[ompletion-default]
syn keyword lftpSettings	contained cls-d[efault] cs[h-history]
syn keyword lftpSettings	contained default-p[rotocol] default-t[itle]
syn keyword lftpSettings	contained fai[l-exit] in[teractive]
syn keyword lftpSettings	contained lo[ng-running] ls[-default]
syn keyword lftpSettings	contained mo[ve-background] prom[pt]
syn keyword lftpSettings	contained rem[ote-completion]
syn keyword lftpSettings	contained save-c[wd-history] save-r[l-history]
syn keyword lftpSettings	contained set-t[erm-status] statu[s-interval]
syn keyword lftpSettings	contained te[rm-status] verb[ose] verify-h[ost]
syn keyword lftpSettings	contained verify-path verify-path[-cached]
" color:
syn keyword lftpSettings	contained dir[-colors] use-c[olor]
" dns:
syn keyword lftpSettings	contained S[RV-query] cache-en[able]
syn keyword lftpSettings	contained cache-ex[pire] cache-s[ize]
syn keyword lftpSettings	contained fat[al-timeout] o[rder] use-fo[rk]
" file:
syn keyword lftpSettings	contained ch[arset]
" fish:
syn keyword lftpSettings	contained connect[-program] sh[ell]
" ftp:
syn keyword lftpSettings	contained acct anon-p[ass] anon-u[ser]
syn keyword lftpSettings	contained au[to-sync-mode] b[ind-data-socket]
syn keyword lftpSettings	contained ch[arset] cli[ent] dev[ice-prefix]
syn keyword lftpSettings	contained fi[x-pasv-address] fxp-f[orce]
syn keyword lftpSettings	contained fxp-p[assive-source] h[ome] la[ng]
syn keyword lftpSettings	contained list-e[mpty-ok] list-o[ptions]
syn keyword lftpSettings	contained nop[-interval] pas[sive-mode]
syn keyword lftpSettings	contained port-i[pv4] port-r[ange] prox[y]
syn keyword lftpSettings	contained rest-l[ist] rest-s[tor]
syn keyword lftpSettings	contained retry-530 retry-530[-anonymous]
syn keyword lftpSettings	contained sit[e-group] skey-a[llow]
syn keyword lftpSettings	contained skey-f[orce] ssl-allow
syn keyword lftpSettings	contained ssl-allow[-anonymous] ssl-au[th]
syn keyword lftpSettings	contained ssl-f[orce] ssl-protect-d[ata]
syn keyword lftpSettings	contained ssl-protect-l[ist] stat-[interval]
syn keyword lftpSettings	contained sy[nc-mode] timez[one] use-a[bor]
syn keyword lftpSettings	contained use-fe[at] use-fx[p] use-hf[tp]
syn keyword lftpSettings	contained use-mdtm use-mdtm[-overloaded]
syn keyword lftpSettings	contained use-ml[sd] use-p[ret] use-q[uit]
syn keyword lftpSettings	contained use-site-c[hmod] use-site-i[dle]
syn keyword lftpSettings	contained use-site-u[time] use-siz[e]
syn keyword lftpSettings	contained use-st[at] use-te[lnet-iac]
syn keyword lftpSettings	contained verify-a[ddress] verify-p[ort]
syn keyword lftpSettings	contained w[eb-mode]
" hftp:
syn keyword lftpSettings	contained w[eb-mode] cache prox[y]
syn keyword lftpSettings	contained use-au[thorization] use-he[ad]
syn keyword lftpSettings	contained use-ty[pe]
" http:
syn keyword lftpSettings	contained accept accept-c[harset]
syn keyword lftpSettings	contained accept-l[anguage] cache coo[kie]
syn keyword lftpSettings	contained pos[t-content-type] prox[y]
syn keyword lftpSettings	contained put-c[ontent-type] put-m[ethod]
syn keyword lftpSettings	contained ref[erer] set-c[ookies] user[-agent]
" https:
syn keyword lftpSettings	contained prox[y]
" mirror:
syn keyword lftpSettings	contained exc[lude-regex] o[rder]
syn keyword lftpSettings	contained parallel-d[irectories]
syn keyword lftpSettings	contained parallel-t[ransfer-count]
syn keyword lftpSettings	contained use-p[get-n]
" module:
syn keyword lftpSettings	contained pat[h]
" net:
syn keyword lftpSettings	contained connection-l[imit]
syn keyword lftpSettings	contained connection-t[akeover]
syn keyword lftpSettings	contained id[le] limit-m[ax] limit-r[ate]
syn keyword lftpSettings	contained limit-total-m[ax] limit-total-r[ate]
syn keyword lftpSettings	contained max-ret[ries] no-[proxy]
syn keyword lftpSettings	contained pe[rsist-retries]
syn keyword lftpSettings	contained reconnect-interval-b[ase]
syn keyword lftpSettings	contained reconnect-interval-ma[x]
syn keyword lftpSettings	contained reconnect-interval-mu[ltiplier]
syn keyword lftpSettings	contained socket-bind-ipv4 socket-bind-ipv6
syn keyword lftpSettings	contained socket-bu[ffer] socket-m[axseg]
syn keyword lftpSettings	contained timeo[ut]
" sftp:
syn keyword lftpSettings	contained connect[-program]
syn keyword lftpSettings	contained max-p[ackets-in-flight]
syn keyword lftpSettings	contained prot[ocol-version] ser[ver-program]
syn keyword lftpSettings	contained size-r[ead] size-w[rite]
" ssl:
syn keyword lftpSettings	contained ca-f[ile] ca-p[ath] ce[rt-file]
syn keyword lftpSettings	contained crl-f[ile] crl-p[ath] k[ey-file]
syn keyword lftpSettings	contained verify-c[ertificate]
" xfer:
syn keyword lftpSettings	contained clo[bber] dis[k-full-fatal]
syn keyword lftpSettings	contained eta-p[eriod] eta-t[erse]
syn keyword lftpSettings	contained mak[e-backup] max-red[irections]
syn keyword lftpSettings	contained ra[te-period]

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_lftp_syn_inits")
  if version < 508
    let did_lftp_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink lftpComment		Comment
  HiLink lftpTodo		Todo
  HiLink lftpString		String
  HiLink lftpNumber		Number
  HiLink lftpBoolean		Boolean
  HiLink lftpInterval		Number
  HiLink lftpKeywords		Keyword
  HiLink lftpSettingsPrefix	PreProc
  HiLink lftpSettings		Type
  delcommand HiLink
endif

let b:current_syntax = "lftp"

" vim: set sts=2 sw=2:
