" Vim syntax file
" Language:	jj description
" Maintainer:	Gregory Anders <greg@gpanders.com>
" Last Change:	2024 May 8
" 2025 Apr 17 by Vim Project (don't require space to start comments, #17130)

if exists('b:current_syntax')
  finish
endif

syn match jjAdded "A .*" contained
syn match jjRemoved "D .*" contained
syn match jjChanged "M .*" contained

syn region jjComment start="^JJ:" end="$" contains=jjAdded,jjRemoved,jjChanged

syn include @jjCommitDiff syntax/diff.vim
syn region jjCommitDiff start=/\%(^diff --\%(git\|cc\|combined\) \)\@=/ end=/^\%(diff --\|$\|@@\@!\|[^[:alnum:]\ +-]\S\@!\)\@=/ fold contains=@jjCommitDiff

if get(g:, 'jjdescription_summary_length') < 0
  syn match   jjdescriptionSummary	"^.*$" contained containedin=jjcommitFirstLine nextgroup=jjcommitOverflow contains=@Spell
elseif get(g:, 'jjdescription_summary_length', 1) > 0
  exe 'syn match   jjdescriptionSummary	"^.*\%<' . (get(g:, 'jjdescription_summary_length', 50) + 1) . 'v." contained containedin=jjcommitFirstLine nextgroup=jjcommitOverflow contains=@Spell'
endif
syn match   jjcommitOverflow	".*" contained contains=@Spell
syn match   jjcommitBlank	"^.\+" contained contains=@Spell
syn match   jjcommitFirstLine	"\%^.*" nextgroup=jjcommitBlank,jjComment skipnl

hi def link jjcommitSummary	Keyword
hi def link jjComment		Comment
hi def link jjAdded		Added
hi def link jjRemove		Removed
hi def link jjChange		Changed
hi def link jjcommitBlank	Error

let b:current_syntax = 'jjdescription'
