vim9script

# Language:           ConTeXt typesetting engine
# Maintainer:         Nicola Vitacolonna <nvitacolonna@gmail.com>
# Former Maintainers: Nikolai Weibull <now@bitwi.se>
# Contributors:       Enno Nagel
# Last Change:        2024 Mar 29
#                     2024 Apr 03 by The Vim Project (removed :CompilerSet definition)
#                     2025 Mar 11 by The Vim Project (add comment for Dispatch)

if exists("g:current_compiler")
  finish
endif

import autoload '../autoload/context.vim'

g:current_compiler = 'context'

# CompilerSet makeprg=context
if get(b:, 'context_ignore_makefile', get(g:, 'context_ignore_makefile', 0)) ||
  (!filereadable('Makefile') && !filereadable('makefile'))
  var makeprg =  join(context.ConTeXtCmd(shellescape(expand('%:p:t'))), ' ')
  execute 'CompilerSet makeprg=' .. escape(makeprg, ' ')
else
  g:current_compiler = 'make'
endif

const context_errorformat = join([
  "%-Popen source%.%#> %f",
  "%-Qclose source%.%#> %f",
  "%-Popen source%.%#name '%f'",
  "%-Qclose source%.%#name '%f'",
  "tex %trror%.%#error on line %l in file %f: %m",
  "%Elua %trror%.%#error on line %l in file %f:",
  "%+Emetapost %#> error: %#",
  "%Emetafun%.%#error: %m",
  "! error: %#%m",
  "%-C %#",
  "%C! %m",
  "%Z[ctxlua]%m",
  "%+C<*> %.%#",
  "%-C%.%#",
  "%Z...%m",
  "%-Zno-error",
  "%-G%.%#"], ",")

execute 'CompilerSet errorformat=' .. escape(context_errorformat, ' ')

# vim: sw=2 fdm=marker
