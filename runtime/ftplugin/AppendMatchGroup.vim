" Vim filetype plugin file utility
" Language:    * (various)
" Maintainer:  Dave Silvia <dsilvia@mchsi.com>
" Date:        6/30/2004

" The start of match (b:SOM) default is:
"       '\<'
" The end of match (b:EOM) default is:
"       '\>'
"
" If you want to use some other start/end of match, just assign the
" value to the b:SOM|EOM variable in your filetype script.
"
" SEE: :h pattern.txt
"      :h pattern-searches
"      :h regular-expression
"      :h matchit

let s:myName=expand("<sfile>:t")

" matchit.vim not loaded -- don't do anyting
if !exists("loaded_matchit")
	echomsg s:myName.": matchit.vim not loaded -- finishing without loading"
	finish
endif

" already been here -- don't redefine
if exists("*AppendMatchGroup")
	finish
endif

" Function To Build b:match_words
" The following function, 'AppendMatchGroup', helps to increase
" readability of your filetype script if you choose to use matchit.
" It also precludes many construction errors, reducing the
" construction to simply invoking the function with the match words.
" As an example, let's take the ubiquitous if/then/else/endif type
" of construct.  This is how the entry in your filetype script would look.
"
"     " source the AppendMatchGroup function file
"     runtime ftplugin/AppendMatchGroup.vim
"
"     " fill b:match_words
"     call AppendMatchGroup('if,then,else,endif')
"
" And the b:match_words constructed would look like:
"
"     \<if\>:\<then\>:\<else\>:\<endif\>
" 
" Use of AppendMatchGroup makes your filetype script is a little
" less busy and a lot more readable.  Additionally, it
" checks three critical things:
"
"      1)  Do you have at least 2 entries in your match group.
"
"      2)  Does the buffer variable 'b:match_words' exist?  if not, create it.
"
"      3)  If the buffer variable 'b:match_words' does exist, is the last
"          character a ','?  If not, add it before appending.
" 
" You should now be able to match 'if/then/else/endif' in succession
" in your source file, in just about any construction you may have
" chosen for them.
"
" To add another group, simply call 'AppendMatchGroup again.  E.G.:
"
"      call AppendMatchGroup('while,do,endwhile')

function AppendMatchGroup(mwordList)
	let List=a:mwordList
	let Comma=match(List,',')
	if Comma == -1 || Comma == strlen(List)-1
		echoerr "Must supply a comma separated list of at least 2 entries."
		echoerr "Supplied list: <".List.">"
		return
	endif
	let listEntryBegin=0
	let listEntryEnd=Comma
	let listEntry=strpart(List,listEntryBegin,listEntryEnd-listEntryBegin)
	let List=strpart(List,Comma+1)
	let Comma=match(List,',')
	" if listEntry is all spaces || List is empty || List is all spaces
	if (match(listEntry,'\s\+') == 0 && match(listEntry,'\S\+') == -1)
			\ || List == '' || (match(List,'\s\+') == 0 && match(List,'\S\+') == -1)
		echoerr "Can't use all spaces for an entry <".listEntry.">"
		echoerr "Remaining supplied list: <".List.">"
		return
	endif

	if !exists("b:SOM")
		let b:SOM='\<'
	endif
	if !exists("b:EOM")
		let b:EOM='\>'
	endif
	if !exists("b:match_words")
		let b:match_words=''
	endif
	if b:match_words != '' && match(b:match_words,',$') == -1
		let b:match_words=b:match_words.','
	endif
	" okay, all set add first entry in this list
	let b:match_words=b:match_words.b:SOM.listEntry.b:EOM.':'
	while Comma != -1
		let listEntryEnd=Comma
		let listEntry=strpart(List,listEntryBegin,listEntryEnd-listEntryBegin)
		let List=strpart(List,Comma+1)
		let Comma=match(List,',')
		" if listEntry is all spaces
		if match(listEntry,'\s\+') == 0 && match(listEntry,'\S\+') == -1
			echoerr "Can't use all spaces for an entry <".listEntry."> - skipping"
			echoerr "Remaining supplied list: <".List.">"
			continue
		endif
		let b:match_words=b:match_words.b:SOM.listEntry.b:EOM.':'
	endwhile
	let listEntry=List
	let b:match_words=b:match_words.b:SOM.listEntry.b:EOM
endfunction

" TODO:  Write a wrapper to handle multiple groups in one function call.
"        Don't see a lot of utility in this as it would undoubtedly warrant
"        continuation lines in the filetype script and it would be a toss
"        up as to which is more readable: individual calls one to a line or
"        a single call with continuation lines.  I vote for the former.
