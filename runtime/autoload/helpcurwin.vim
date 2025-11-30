vim9script

# Open Vim help on {subject} in the current window (rather than a new split)
#
# Maintainer:   The Vim Project <https://github.com/vim/vim>
# Last change:  2025 Nov 30
# Usage:        :help pi_helpcurwin.txt

export def Open(subject: string): void

  const HELPCURWIN: func = (): string => {
  if !getcompletion(subject, 'help')->empty() || subject->empty()
    if &buftype != 'help'
      execute 'silent noautocmd keepalt enew'
      setlocal buftype=help noswapfile
    endif
  endif
  return $'help {subject}'
  }

  var contmod: bool = true
  if &modified
    echohl MoreMsg
    echo $"Buffer {bufname()} is modified - continue? (y/n)"
    echohl None
    contmod = getcharstr() == 'y' ? true : false
  endif
  if contmod
    try
      execute HELPCURWIN()
    catch
      echohl Error
      # {subject} isn't valid.  Echo 'helpcurwin: E149:' (w/o 'Vim(help):')
      echo $"helpcurwin: {v:exception->substitute('^[^:]\+:', '', '')}"
      echohl None
    endtry
  else
    echo $"Aborted opening in current window, :help {subject}"
  endif

enddef

# vim: ts=8 sts=2 sw=2 et
