vim9script

# Highlight Yank plugin
# Last Change: 2025 Mar 17

def HighlightedYank()

  var hlgroup = get(g:, "hlyank_hlgroup", "IncSearch")
  var duration = min([get(g:, "hlyank_duration", 300), 3000])
  var in_visual = get(g:, "hlyank_invisual", true)

  if v:event.operator ==? 'y'
    if !in_visual && visualmode() != null_string
      visualmode(1)
      return
    endif
    # if clipboard has autoselect (default on linux) exiting from Visual with
    # ESC generates bogus event and this highlights previous yank
    if &clipboard =~ 'autoselect' && v:event.regname == "*" && v:event.visual
      return
    endif
    var [beg, end] = [getpos("'["), getpos("']")]
    var type = v:event.regtype ?? 'v'
    var pos = getregionpos(beg, end, {type: type, exclusive: false})
    var m = matchaddpos(hlgroup, pos->mapnew((_, v) => {
      var col_beg = v[0][2] + v[0][3]
      var col_end = v[1][2] + v[1][3] + 1
      return [v[0][1], col_beg, col_end - col_beg]
    }))
    var winid = win_getid()
    timer_start(duration, (_) => m->matchdelete(winid))
  endif
enddef

augroup hlyank
  autocmd!
  autocmd TextYankPost * HighlightedYank()
augroup END
# vim:sts=2:sw=2:et:
