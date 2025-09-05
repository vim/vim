vim9script

# Vim plugin for setting the background and foreground colours depending on
# the terminal response.
#
# Maintainer:	The Vim Project <https://github.com/vim/vim>
# Last Change:	2025 Sep 05

if exists("g:loaded_colorresp")
  finish
endif
g:loaded_colorresp = 1

augroup ColorResp
  au!
  au TermResponseAll osc {
    var parts: list<string> = matchlist(v:termosc, '\(\d\+\);rgb:\(\w\+\)/\(\w\+\)/\(\w\+\)')
    if len(parts) >= 5
      var type: string = parts[1]
      var rval: number = str2nr(parts[2][: 1], 16)
      var gval: number = str2nr(parts[3][: 1], 16)
      var bval: number = str2nr(parts[4][: 1], 16)

      if type == '11'
        # Detect light or dark background by parsing OSC 11 RGB background reply
        # from terminal. Sum the RGB values roughly; if bright enough, set
        # 'background' to 'light', otherwise set it to 'dark'.
        var new_bg_val: string = (3 * char2nr('6') < char2nr(parts[2]) + char2nr(parts[3]) + char2nr(parts[4])) ? "light" : "dark"

        v:termrbgresp = v:termosc
        &background = new_bg_val
        # For backwards compatibility
        if exists('#TermResponseAll#background')
          doautocmd <nomodeline> TermResponseAll background
        endif
      else
        v:termrfgresp = v:termosc
        # For backwards compatibility
        if exists('#TermResponseAll#foreground')
          doautocmd <nomodeline> TermResponseAll foreground
        endif
      endif
    endif
  }
  au VimEnter * ++once {
    call echoraw(&t_RB)
    call echoraw(&t_RF)
  }
augroup END

# vim: set sw=2 sts=2 :
