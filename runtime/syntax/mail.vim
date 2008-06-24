" Vim syntax file
" Language:		Mail file
" Previous Maintainer:	Felix von Leitner <leitner@math.fu-berlin.de>
" Maintainer:		Gautam Iyer <gi1242@users.sourceforge.net>
" Last Change:		Thu 17 Jan 2008 11:25:44 AM PST

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" The mail header is recognized starting with a "keyword:" line and ending
" with an empty line or other line that can't be in the header. All lines of
" the header are highlighted. Headers of quoted messages (quoted with >) are
" also highlighted.

" Syntax clusters
syn cluster mailHeaderFields	contains=mailHeaderKey,mailSubject,mailHeaderEmail,@mailLinks
syn cluster mailLinks		contains=mailURL,mailEmail
syn cluster mailQuoteExps	contains=mailQuoteExp1,mailQuoteExp2,mailQuoteExp3,mailQuoteExp4,mailQuoteExp5,mailQuoteExp6

syn case match
" For "From " matching case is required. The "From " is not matched in quoted
" emails
" According to RFC 2822 any printable ASCII character can appear in a field
" name, except ':'.
syn region	mailHeader	contains=@mailHeaderFields,@NoSpell start="^From .*\d\d\d\d$" skip="^\s" end="\v^[!-9;-~]*([^!-~]|$)"me=s-1
syn match	mailHeaderKey	contained contains=mailEmail,@NoSpell "^From\s.*\d\d\d\d$"
" Usenet headers
syn match	mailHeaderKey	contained contains=mailHeaderEmail,mailEmail,@NoSpell "\v(^(\> ?)*)@<=(Newsgroups|Followup-To|Message-ID|Supersedes|Control):.*$"

syn case ignore
" Nothing else depends on case. Headers in properly quoted (with "> " or ">")
" emails are matched
syn region	mailHeader	keepend contains=@mailHeaderFields,@mailQuoteExps,@NoSpell start="^\z(\(> \?\)*\)\v(newsgroups|x-([a-z\-])*|path|xref|message-id|from|((in-)?reply-)?to|b?cc|subject|return-path|received|date|replied):" skip="^\z1\s" end="\v^\z1[!-9;-~]*([^!-~]|$)"me=s-1 end="\v^\z1@!"me=s-1 end="\v^\z1(\> ?)+"me=s-1

syn region	mailHeaderKey	contained contains=mailHeaderEmail,mailEmail,@mailQuoteExps,@NoSpell start="\v(^(\> ?)*)@<=(to|b?cc):" skip=",$" end="$"
syn match	mailHeaderKey	contained contains=mailHeaderEmail,mailEmail,@NoSpell "\v(^(\> ?)*)@<=(from|reply-to):.*$"
syn match	mailHeaderKey	contained contains=@NoSpell "\v(^(\> ?)*)@<=date:"
syn match	mailSubject	contained "\v^subject:.*$"
syn match	mailSubject	contained contains=@NoSpell "\v(^(\> ?)+)@<=subject:.*$"

" Anything in the header between < and > is an email address
syn match	mailHeaderEmail	contained contains=@NoSpell "<.\{-}>"

" Mail Signatures. (Begin with "-- ", end with change in quote level)
syn region	mailSignature	keepend contains=@mailLinks,@mailQuoteExps start="^--\s$" end="^$" end="^\(> \?\)\+"me=s-1
syn region	mailSignature	keepend contains=@mailLinks,@mailQuoteExps,@NoSpell start="^\z(\(> \?\)\+\)--\s$" end="^\z1$" end="^\z1\@!"me=s-1 end="^\z1\(> \?\)\+"me=s-1

" URLs start with a known protocol or www,web,w3.
syn match mailURL contains=@NoSpell `\v<(((https?|ftp|gopher)://|(mailto|file|news):)[^' 	<>"]+|(www|web|w3)[a-z0-9_-]*\.[a-z0-9._-]+\.[^' 	<>"]+)[a-z0-9/]`
syn match mailEmail contains=@NoSpell "\v[_=a-z\./+0-9-]+\@[a-z0-9._-]+\a{2}"

" Make sure quote markers in regions (header / signature) have correct color
syn match mailQuoteExp1	contained "\v^(\> ?)"
syn match mailQuoteExp2	contained "\v^(\> ?){2}"
syn match mailQuoteExp3	contained "\v^(\> ?){3}"
syn match mailQuoteExp4	contained "\v^(\> ?){4}"
syn match mailQuoteExp5	contained "\v^(\> ?){5}"
syn match mailQuoteExp6	contained "\v^(\> ?){6}"

" Even and odd quoted lines. order is imporant here!
syn match mailQuoted1	contains=mailHeader,@mailLinks,mailSignature,@NoSpell "^\([a-z]\+>\|[]|}>]\).*$"
syn match mailQuoted2	contains=mailHeader,@mailLinks,mailSignature,@NoSpell "^\(\([a-z]\+>\|[]|}>]\)[ \t]*\)\{2}.*$"
syn match mailQuoted3	contains=mailHeader,@mailLinks,mailSignature,@NoSpell "^\(\([a-z]\+>\|[]|}>]\)[ \t]*\)\{3}.*$"
syn match mailQuoted4	contains=mailHeader,@mailLinks,mailSignature,@NoSpell "^\(\([a-z]\+>\|[]|}>]\)[ \t]*\)\{4}.*$"
syn match mailQuoted5	contains=mailHeader,@mailLinks,mailSignature,@NoSpell "^\(\([a-z]\+>\|[]|}>]\)[ \t]*\)\{5}.*$"
syn match mailQuoted6	contains=mailHeader,@mailLinks,mailSignature,@NoSpell "^\(\([a-z]\+>\|[]|}>]\)[ \t]*\)\{6}.*$"

" Need to sync on the header. Assume we can do that within 100 lines
if exists("mail_minlines")
    exec "syn sync minlines=" . mail_minlines
else
    syn sync minlines=100
endif

" Define the default highlighting.
hi def link mailHeader		Statement
hi def link mailHeaderKey	Type
hi def link mailSignature	PreProc
hi def link mailHeaderEmail	mailEmail
hi def link mailEmail		Special
hi def link mailURL		String
hi def link mailSubject		LineNR
hi def link mailQuoted1		Comment
hi def link mailQuoted3		mailQuoted1
hi def link mailQuoted5		mailQuoted1
hi def link mailQuoted2		Identifier
hi def link mailQuoted4		mailQuoted2
hi def link mailQuoted6		mailQuoted2
hi def link mailQuoteExp1	mailQuoted1
hi def link mailQuoteExp2	mailQuoted2
hi def link mailQuoteExp3	mailQuoted3
hi def link mailQuoteExp4	mailQuoted4
hi def link mailQuoteExp5	mailQuoted5
hi def link mailQuoteExp6	mailQuoted6

let b:current_syntax = "mail"
