vim9script

# Helper functions for Hare.
# Language:    Hare
# Maintainer:  Amelia Clarke <selene@perilune.dev>
# Last Change: 2026 Jan 24
# Upstream:    https://git.sr.ht/~sircmpwn/hare.vim

# Returns the value of $HAREPATH, if it exists. Otherwise, returns a safe
# default value.
export def GetPath(): string
  var path: list<string>
  if !empty($HAREPATH)
    path = split($HAREPATH, ':')
  else
    path = ParsePath()
    if empty(path)
      return '/usr/src/hare/stdlib,/usr/src/hare/third-party'
    endif
  endif
  return map(path, (_, n) => escape(n, ' ,;'))->join(',')
enddef

# Modifies quickfix or location list entries to refer to the correct paths after
# running :make or :lmake, respectively.
export def QuickFixPaths()
  var GetList: func
  var SetList: func

  if expand('<amatch>') =~ '^l'
    GetList = function('getloclist', [0])
    SetList = function('setloclist', [0])
  else
    GetList = function('getqflist')
    SetList = function('setqflist')
  endif

  final list = GetList({ items: 0 })
  for n in list.items
    if !empty(n.module)
      n.filename = findfile(n.module)
    endif
  endfor
  SetList([], 'r', list)
enddef

# Attempts to parse a list of directories from the output of `hare version -v`.
# Otherwise, returns an empty list.
def ParsePath(): list<string>
  if !executable('hare')
    return []
  endif

  silent final lines = systemlist('hare version -v')
  const min = match(lines, '^HAREPATH') + 1
  if min == 0
    return []
  endif

  const max = match(lines, '^\S', min)
  return (max < 0 ? slice(lines, min) : slice(lines, min, max))
    ->map((_, n) => matchstr(n, '^\s*\zs.*'))
enddef

# vim: et sts=2 sw=2 ts=8 tw=80
