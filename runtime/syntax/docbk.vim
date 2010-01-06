" Vim syntax file
" Language:	DocBook
" Maintainer:	Devin Weaver <vim@tritarget.com>
" URL:		http://tritarget.com/pub/vim/syntax/docbk.vim
" Last Change:	$Date: 2005/06/23 22:31:01 $
" Version:	$Revision: 1.2 $
" Thanks to Johannes Zellner <johannes@zellner.org> for the default to XML
" suggestion.

" REFERENCES:
"   http://docbook.org/
"   http://www.open-oasis.org/docbook/
"

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Auto detect added by Bram Moolenaar
if !exists('b:docbk_type')
  if expand('%:e') == "sgml"
    let b:docbk_type = 'sgml'
  else
    let b:docbk_type = 'xml'
  endif
endif
if 'xml' == b:docbk_type
    doau Syntax xml
    syn cluster xmlTagHook add=docbkKeyword
    syn cluster xmlRegionHook add=docbkRegion,docbkTitle,docbkRemark,docbkCite
    syn case match
elseif 'sgml' == b:docbk_type
    doau Syntax sgml
    syn cluster sgmlTagHook add=docbkKeyword
    syn cluster sgmlRegionHook add=docbkRegion,docbkTitle,docbkRemark,docbkCite
    syn case ignore
endif

" <comment> has been removed and replace with <remark> in DocBook 4.0
" <comment> kept for backwards compatability.
syn keyword docbkKeyword abbrev abstract accel ackno acronym action contained
syn keyword docbkKeyword address affiliation alt anchor answer appendix contained
syn keyword docbkKeyword application area areaset areaspec arg artheader contained
syn keyword docbkKeyword article articleinfo artpagenums attribution audiodata contained
syn keyword docbkKeyword audioobject author authorblurb authorgroup contained
syn keyword docbkKeyword authorinitials beginpage bibliodiv biblioentry contained
syn keyword docbkKeyword bibliography bibliomisc bibliomixed bibliomset contained
syn keyword docbkKeyword biblioset blockquote book bookbiblio bookinfo contained
syn keyword docbkKeyword bridgehead callout calloutlist caption caution contained
syn keyword docbkKeyword chapter citation citerefentry citetitle city contained
syn keyword docbkKeyword classname cmdsynopsis co collab collabname contained
syn keyword docbkKeyword colophon colspec command comment computeroutput contained
syn keyword docbkKeyword confdates confgroup confnum confsponsor conftitle contained
syn keyword docbkKeyword constant contractnum contractsponsor contrib contained
syn keyword docbkKeyword copyright corpauthor corpname country database contained
syn keyword docbkKeyword date dedication docinfo edition editor email contained
syn keyword docbkKeyword emphasis entry entrytbl envar epigraph equation contained
syn keyword docbkKeyword errorcode errorname errortype example fax figure contained
syn keyword docbkKeyword filename firstname firstterm footnote footnoteref contained
syn keyword docbkKeyword foreignphrase formalpara funcdef funcparams contained
syn keyword docbkKeyword funcprototype funcsynopsis funcsynopsisinfo contained
syn keyword docbkKeyword function glossary glossdef glossdiv glossentry contained
syn keyword docbkKeyword glosslist glosssee glossseealso glossterm graphic contained
syn keyword docbkKeyword graphicco group guibutton guiicon guilabel contained
syn keyword docbkKeyword guimenu guimenuitem guisubmenu hardware contained
syn keyword docbkKeyword highlights holder honorific imagedata imageobject contained
syn keyword docbkKeyword imageobjectco important index indexdiv indexentry contained
syn keyword docbkKeyword indexterm informalequation informalexample contained
syn keyword docbkKeyword informalfigure informaltable inlineequation contained
syn keyword docbkKeyword inlinegraphic inlinemediaobject interface contained
syn keyword docbkKeyword interfacedefinition invpartnumber isbn issn contained
syn keyword docbkKeyword issuenum itemizedlist itermset jobtitle keycap contained
syn keyword docbkKeyword keycode keycombo keysym keyword keywordset label contained
syn keyword docbkKeyword legalnotice lineage lineannotation link listitem contained
syn keyword docbkKeyword literal literallayout lot lotentry manvolnum contained
syn keyword docbkKeyword markup medialabel mediaobject mediaobjectco contained
syn keyword docbkKeyword member menuchoice modespec mousebutton msg msgaud contained
syn keyword docbkKeyword msgentry msgexplan msginfo msglevel msgmain contained
syn keyword docbkKeyword msgorig msgrel msgset msgsub msgtext note contained
syn keyword docbkKeyword objectinfo olink option optional orderedlist contained
syn keyword docbkKeyword orgdiv orgname otheraddr othercredit othername contained
syn keyword docbkKeyword pagenums para paramdef parameter part partintro contained
syn keyword docbkKeyword phone phrase pob postcode preface primary contained
syn keyword docbkKeyword primaryie printhistory procedure productname contained
syn keyword docbkKeyword productnumber programlisting programlistingco contained
syn keyword docbkKeyword prompt property pubdate publisher publishername contained
syn keyword docbkKeyword pubsnumber qandadiv qandaentry qandaset question contained
syn keyword docbkKeyword quote refclass refdescriptor refentry contained
syn keyword docbkKeyword refentrytitle reference refmeta refmiscinfo contained
syn keyword docbkKeyword refname refnamediv refpurpose refsect1 contained
syn keyword docbkKeyword refsect1info refsect2 refsect2info refsect3 contained
syn keyword docbkKeyword refsect3info refsynopsisdiv refsynopsisdivinfo contained
syn keyword docbkKeyword releaseinfo remark replaceable returnvalue revhistory contained
syn keyword docbkKeyword revision revnumber revremark row sbr screen contained
syn keyword docbkKeyword screenco screeninfo screenshot secondary contained
syn keyword docbkKeyword secondaryie sect1 sect1info sect2 sect2info sect3 contained
syn keyword docbkKeyword sect3info sect4 sect4info sect5 sect5info section contained
syn keyword docbkKeyword sectioninfo see seealso seealsoie seeie seg contained
syn keyword docbkKeyword seglistitem segmentedlist segtitle seriesinfo contained
syn keyword docbkKeyword seriesvolnums set setindex setinfo sgmltag contained
syn keyword docbkKeyword shortaffil shortcut sidebar simpara simplelist contained
syn keyword docbkKeyword simplesect spanspec state step street structfield contained
syn keyword docbkKeyword structname subject subjectset subjectterm contained
syn keyword docbkKeyword subscript substeps subtitle superscript surname contained
syn keyword docbkKeyword symbol synopfragment synopfragmentref synopsis contained
syn keyword docbkKeyword systemitem table tbody term tertiary tertiaryie contained
syn keyword docbkKeyword textobject tfoot tgroup thead tip title contained
syn keyword docbkKeyword titleabbrev toc tocback tocchap tocentry tocfront contained
syn keyword docbkKeyword toclevel1 toclevel2 toclevel3 toclevel4 toclevel5 contained
syn keyword docbkKeyword tocpart token trademark type ulink userinput contained
syn keyword docbkKeyword varargs variablelist varlistentry varname contained
syn keyword docbkKeyword videodata videoobject void volumenum warning contained
syn keyword docbkKeyword wordasword xref year contained

" Add special emphasis on some regions. Thanks to Rory Hunter <roryh@dcs.ed.ac.uk> for these ideas.
syn region docbkRegion start="<emphasis>"lc=10 end="</emphasis>"me=e-11 contains=xmlRegion,xmlEntity,sgmlRegion,sgmlEntity keepend
syn region docbkTitle  start="<title>"lc=7     end="</title>"me=e-8	contains=xmlRegion,xmlEntity,sgmlRegion,sgmlEntity keepend
syn region docbkRemark start="<remark>"lc=8    end="</remark>"me=e-9	contains=xmlRegion,xmlEntity,sgmlRegion,sgmlEntity keepend
syn region docbkRemark start="<comment>"lc=9  end="</comment>"me=e-10	contains=xmlRegion,xmlEntity,sgmlRegion,sgmlEntity keepend
syn region docbkCite   start="<citation>"lc=10 end="</citation>"me=e-11 contains=xmlRegion,xmlEntity,sgmlRegion,sgmlEntity keepend

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_docbk_syn_inits")
  if version < 508
    let did_docbk_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
    hi DocbkBold term=bold cterm=bold gui=bold
  else
    command -nargs=+ HiLink hi def link <args>
    hi def DocbkBold term=bold cterm=bold gui=bold
  endif

  HiLink docbkKeyword	Statement
  HiLink docbkRegion	DocbkBold
  HiLink docbkTitle	Title
  HiLink docbkRemark	Comment
  HiLink docbkCite	Constant

  delcommand HiLink
endif

let b:current_syntax = "docbk"

" vim: ts=8
