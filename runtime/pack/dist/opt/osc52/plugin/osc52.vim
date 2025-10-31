vim9script

# Vim plugin for OSC52 clipboard support
#
# Maintainer:	The Vim Project <https://github.com/vim/vim>
# Last Change:	2025 October 14

import autoload "../autoload/osc52.vim" as osc

v:clipproviders["osc52"] = {
  "available": osc.Available,
  "paste": {
    "*": osc.Paste,
    "+": osc.Paste
  },
  "copy": {
    "*": osc.Copy,
    "+": osc.Copy
  },
}

# vim: set sw=2 sts=2 :
