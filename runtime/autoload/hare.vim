vim9script

# Helper functions for Hare.
# Language:     Hare
# Maintainer:   Amelia Clarke <selene@perilune.dev>
# Last Updated: 2025 Sep 06
# Upstream:     https://git.sr.ht/~sircmpwn/hare.vim

# Returns the value of HAREPATH, if it exists. Otherwise, returns a safe
# default.
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
  return mapnew(path, (_, n) => escape(n, ' ,;'))->join(',')
enddef

# Converts a module identifier into a path.
export def IncludeExpr(): string
  var path = trim(v:fname, ':', 2)->substitute('::', '/', 'g')

  # If the module cannot be found, it might be a member instead. Try removing
  # the final component until a directory is found.
  while !finddir(path)
    const head = fnamemodify(path, ':h')
    if head == '.'
      break
    endif
    path = head
  endwhile

  return path
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

# Attempts to parse the directories in $HAREPATH from the output of `hare
# version -v`. Otherwise, returns an empty list.
def ParsePath(): list<string>
  if !executable('hare')
    return []
  endif

  silent const lines = systemlist('hare version -v')
  const min = match(lines, '^HAREPATH') + 1
  if min == 0
    return []
  endif

  const max = match(lines, '^\S', min)
  return (max < 0 ? slice(lines, min) : slice(lines, min, max))
    ->mapnew((_, n) => matchstr(n, '^\s*\zs.*'))
enddef

# vim: et sts=2 sw=2 ts=8 tw=80
