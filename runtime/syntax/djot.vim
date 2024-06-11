" Vim syntax file
" Language: Djot
" Maintainer: John MacFarlane
" Latest Revision: 2022-10-06

if exists("b:current_syntax")
  finish
endif

syn match heading '^##* .*$'
syn match blockquote '^\s*>\%(\s\|$\)'

syn region math matchgroup=delimiter skip='[^`]{1,}' start='[$][$]\?\z(``*\)' end='\z1\|^\s*$'
syn region codespan matchgroup=delimiter skip='[^`]{1,}' start='\z(``*\)' end='\z1\|^\s*$'

syn region comment matchgroup=delimiter start='%' end='%' contained
syn region string start='"' end='"' skip='\\"'
syn region attributes matchgroup=delimiter start="{[^\[\]_*'\"=\\+-]\@=" end="}" contains=string,comment

syn region emphasis matchgroup=delimiter start='_[^\s}]\@=\|{_' end='_}\|[^\s{]\@=_\|^\s*$' contains=@inline
syn region strong matchgroup=delimiter start='\*[^\s}]\@=\|{\*' end='[^\s{]\@=\*\|\*}\|^\s*$' contains=@inline

syn region superscript matchgroup=delimiter start='\^[^\s}]\@=\|{\^' end='\^}\|[^\s{]\@=\^\|^\s*$' contains=@inline
syn region subscript matchgroup=delimiter start='\~[^\s}]\@=\|{\~' end='\~}\|[^\s{]\@=\~\|^\s*$' contains=@inline

syn region highlight matchgroup=delimiter start='{=' end='=}\|^\s*$' contains=@inline
syn match rawattribute "`\@<={=[A-Za-z0-9]*}"

syn region insert matchgroup=delimiter start='{+' end='+}\|^\s*$' contains=@inline
syn region delete matchgroup=delimiter start='{-' end='-}\|^\s*$' contains=@inline

syn match inlinelink '\[\%([^\]\\]\|\\[\]\\]\|[\r\n]\)*\](\%([^)\\]\|\\[)\\]\)*)' contains=@inline transparent
syn match linkurl '(\%([^)\\]\|\\[)\\]\)*)'hs=s+1,he=e-1 containedin=inlinelink contained

syn match referencelink '\[\%([^\]\\]\|\\[\]\\]\|[\r\n]\)*\]\[\%([^]\\]\|\\[]\\]\)*\]' contains=@inline transparent
syn match linklabel '\]\zs\[\%([^]\\]\|\\[]\\]\)*\]'hs=s+1,he=e-1 containedin=referencelink contained

syn match span '\[\%([^\]\\]\|\\[\]\\]\|[\r\n]\)*\][{]\@=' contains=@inline transparent

syn match footnoteref '\[\^[^]]*\]'

syn match openbrace /[{]["']/he=e-1
syn match closebrace /["'][}]/hs=s+1

syn match emoji ':[a-zA-Z0-9_+-]\+:'

syn match escape '\\[\r\n ~!@#$%^&*(){}`\[\]/=\\?+|\'",<-]'he=e-1

syn region djotautolinkurl matchgroup=delimiter start=/</ end=/>/

syn cluster inline contains=linkurl,emphasis,strong,codespan,attributes,rawattribute,insert,delete,superscript,subscript,highlight,math,smartquote,openbrace,closebrace,emoji,escape,footnoteref,span

syn region codeblock matchgroup=delimiter start='^\s*\z(````*\)\s*=\?\w*\s*$' end='^\s*\z1`*\s*$'

hi emphasis term=italic cterm=italic gui=italic
hi strong term=bold cterm=bold gui=bold
hi def link insert Todo
hi def link delete Error
hi def link superscript Statement
hi def link subscript Statement
hi def link highlight Todo
hi def link heading  Label
hi def link codespan Tag
hi def link math Statement
hi def link emoji Statement
hi def link span Statement
hi def link codeblock Tag
hi def link string String
hi def link inlinelink Typedef
hi def link footnoteref Statement
hi def link linkurl Underlined
hi def link djotautolinkurl Underlined
hi def link comment Comment
hi def link linklabel Underlined
hi def link escaped Typedef
hi def link attributes Identifier
hi def link rawattribute Identifier
hi def link delimiter Ignore
hi def link escape Ignore
hi def link openbrace Ignore
hi def link closebrace Ignore
hi def link blockquote Comment

let b:current_syntax = "djot"
