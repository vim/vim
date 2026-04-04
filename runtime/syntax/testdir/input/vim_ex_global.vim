" Vim :global and :v commands


global/foo/echo "..."
global!/foo/echo "..."
vglobal/foo/echo "..."

global  /foo/ echo "..."
global! /foo/ echo "..."
vglobal /foo/ echo "..."

" docs state ! is not allowed as the delimiter but it works
global!!foo!echo "..."
global! !foo! echo "..."

" command with range
global/./.,/^$/join

" recursive
global/found/v/not\%(found\)/echo "..."


def Vim9Context()
  global/foo/echo "..."
  global!/foo/echo "..."
  vglobal/foo/echo "..."

  global  /foo/ echo "..."
  global! /foo/ echo "..."
  vglobal /foo/ echo "..."

  # docs state ! is not allowed as the delimiter but it works
  global!!foo!echo "..."
  global! !foo! echo "..."

  # command with range
  global/./.,/^$/join

  # recursive
  global/found/v/not\%(found\)/echo "..."
enddef

