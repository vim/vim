vim9script

if !exists("g:calendar_mark")
 || (g:calendar_mark != 'left'
 && g:calendar_mark != 'left-fit'
 && g:calendar_mark != 'right')
  g:calendar_mark = 'left'
endif
if !exists("g:calendar_navi")
 || (g:calendar_navi != 'top'
 && g:calendar_navi != 'bottom'
 && g:calendar_navi != 'both'
 && g:calendar_navi != '')
  g:calendar_navi = 'top'
endif
if !exists("g:calendar_navi_label")
  g:calendar_navi_label = "Prev, Today, Next"
endif
if !exists("g:calendar_diary_list_curr_idx")
  g:calendar_diary_list_curr_idx = 0
endif
if !exists("g:calendar_diary")
  if exists("g:calendar_diary_list") && len(g:calendar_diary_list) > 0
      && g:calendar_diary_list_curr_idx >= 0
      && g:calendar_diary_list_curr_idx < len(g:calendar_diary_list)
    g:calendar_diary = g:calendar_diary_list[g:calendar_diary_list_curr_idx].path
    g:calendar_diary_extension = g:calendar_diary_list[g:calendar_diary_list_curr_idx].ext
  else
    g:calendar_diary = "~/diary"
  endif
endif
if !exists("g:calendar_focus_today")
  g:calendar_focus_today = 0
endif
if !exists("g:calendar_datetime")
 || (g:calendar_datetime != ''
 && g:calendar_datetime != 'title'
 && g:calendar_datetime != 'statusline')
  g:calendar_datetime = 'title'
endif
if !exists("g:calendar_options")
  g:calendar_options = "fdc=0 nonu"
  if has("+relativenumber") || exists("+relativenumber")
    g:calendar_options ..= " nornu"
  endif
endif
if !exists("g:calendar_filetype")
  g:calendar_filetype = "markdown"
endif
if !exists("g:calendar_diary_extension")
    g:calendar_diary_extension = ".md"
endif
if !exists("g:calendar_search_grepprg")
  g:calendar_search_grepprg = "grep"
endif


#*****************************************************************
#* Default Calendar key bindings
#*****************************************************************
var calendar_keys = {
 close: 'q',
 do_action: '<CR>',
 goto_today: 't',
 show_help: '?',
 redisplay: 'r',
 goto_next_month: '<RIGHT>',
 goto_prev_month: '<LEFT>',
 goto_next_year: '<UP>',
 goto_prev_year: '<DOWN>',
}

if exists("g:calendar_keys") && typename(g:calendar_keys) == 'dict<string>'
   extend(calendar_keys, g:calendar_keys)
endif

var bufautocommandsset = false
var fridaycol = 0
var saturdaycol = 0
#*****************************************************************
#* CalendarClose : close the calendar
#*----------------------------------------------------------------
#*****************************************************************
def Close()
  bw!
enddef

#*****************************************************************
#* CalendarDoAction : call the action handler function
#*----------------------------------------------------------------
#*****************************************************************
def Action(arg: string = '')
  # for switch calendar list.
  var text = getline(".")
  var curl = line(".")
  if text =~ "^( )"
    var list_idx = 0
    curl -= 1
    while curl > 1
      if getline(curl) =~ "^([\* ])"
        list_idx += 1
        curl -= 1
      else
        g:calendar_diary_list_curr_idx = list_idx
        g:calendar_diary = g:calendar_diary_list[list_idx].path
        g:calendar_diary_extension = g:calendar_diary_list[list_idx].ext
        Show(b:CalendarDir, b:CalendarYear, b:CalendarMonth)
        return
      endif
    endwhile
  endif

  # for navi
  if exists('g:calendar_navi')
    var navi = !empty(arg) ? arg : expand("<cWORD>")
    curl = line(".")
    var curp = getpos(".")
    if navi == $"<{get(split(g:calendar_navi_label, ', '), 0, '')}"
      if b:CalendarMonth > 1
        Show(b:CalendarDir, b:CalendarYear, b:CalendarMonth - 1)
      else
        Show(b:CalendarDir, b:CalendarYear - 1, 12)
      endif
    elseif navi == $"{get(split(g:calendar_navi_label, ', '), 2, '')}>"
      if b:CalendarMonth < 12
        Show(b:CalendarDir, b:CalendarYear, b:CalendarMonth + 1)
      else
        Show(b:CalendarDir, b:CalendarYear + 1, 1)
      endif
    elseif navi == get(split(g:calendar_navi_label, ', '), 1, '')
      Show(b:CalendarDir)
      if exists('g:calendar_today')
        var CalendarToday = function(g:calendar_today)
        CalendarToday()
      endif
    elseif navi == 'NextYear'
      Show(b:CalendarDir, b:CalendarYear + 1, b:CalendarMonth)
      setpos('.', curp)
      return
    elseif navi == 'PrevYear'
      Show(b:CalendarDir, b:CalendarYear - 1, b:CalendarMonth)
      setpos('.', curp)
      return
    else
      navi = ''
    endif
    if navi != ''
      if g:calendar_focus_today == 1 && search("\*", "w") > 0
        silent execute "normal! gg/\*\<cr>"
        return
      else
        if curl < line('$') / 2
          silent execute $"normal! gg0/{navi}\<cr>"
        else
          silent execute $"normal! G$?{navi}\<cr>"
        endif
        return
      endif
    endif
  endif

  var dir = ''
  var cnr = 0
  var week = 0
  if b:CalendarDir == 0 || b:CalendarDir == 3
    dir = 'V'
    cnr = 1
    week = ((col(".") + 1) / 3) - 1
  elseif b:CalendarDir == 1
    dir = 'H'
    if exists('g:calendar_weeknm')
      cnr = col('.') - (col('.') % (24 + 5)) + 1
    else
      cnr = col('.') - (col('.') % (24)) + 1
    endif
    week = ((col(".") - cnr - 1 + cnr / 49) / 3)
  elseif b:CalendarDir == 2
    dir = 'T'
    cnr = 1
    week = ((col(".") + 1) / 3) - 1
  endif
  var lnr = 1
  var hdr = 1
  var sline = ''
  while 1
    if lnr > line('.')
      break
    endif
    sline = getline(lnr)
    if sline =~ '^\s*$'
      hdr = lnr + 1
    endif
    lnr = lnr + 1
  endwhile
  lnr = line('.')

  if exists('g:calendar_monday') && g:calendar_monday
    week = week + 1
  elseif week == 0
    week = 7
  endif
  if lnr - hdr < 2
    return
  endif

  sline = substitute(strpart(getline(hdr), cnr, 21), '\s*\(.*\)\s*', '\1', '')
  var day = ''
  if b:CalendarDir != 2
    if (col(".") - cnr) > 21
      return
    endif

    # extract day
    if g:calendar_mark == 'right' && col('.') > 1
      normal! h
      day = matchstr(expand("<cword>"), '[^0].*')
      normal! l
    else
      day = matchstr(expand("<cword>"), '[^0].*')
    endif
  else
    var c = string(col('.'))
    day = ''
    var lnum = line('.')
    var cursorchar = getline(lnum)[col('.') - 1]
    while day == '' && lnum > 2 && cursorchar != '-' && cursorchar != '+'
      day = matchstr(getline(lnum), '^.*|\zs[^|]\{-}\%' .. c .. 'c[^|]\{-}\ze|.*$')
            ->matchstr('\d\+')
      lnum = lnum - 1
      cursorchar = getline(lnum)[col('.') - 1]
    endwhile
  endif
  if day == ''
    return
  endif

  # extract year and month
  var year = -2000
  var month = 99
  if exists('g:calendar_erafmt') && g:calendar_erafmt !~ "^\s*$"
    year = str2nr(matchstr(substitute(sline, '/.*', '', ''), '\d\+'))
    month = str2nr(matchstr(substitute(sline, '.*/\(\d\d\=\).*', '\1', ""), '[^0].*'))
    if g:calendar_erafmt =~ '.*, [+-]*\d\+'
      var veranum = substitute(g:calendar_erafmt, '.*, \([+-]*\d\+\)', '\1', '')->str2nr()
      if year - veranum > 0
        year = year - veranum
      endif
    endif
  else
    year  = str2nr(matchstr(substitute(sline, '/.*', '', ''), '[^0].*'))
    month = str2nr(matchstr(substitute(sline, '\d*/\(\d\d\=\).*', '\1', ""), '[^0].*'))
  endif
  # call the action function
  call(CalendarAction, [str2nr(day), month, year, week, dir])
enddef

#*****************************************************************
#* Calendar : build calendar
#*----------------------------------------------------------------
#*   a1 : direction (mandatory)
#*   a2 : month(if given a3, it's year)
#*   a3 : if given, it's month
#*****************************************************************
export def Show(a1: number, a2: number = -1, a3: number = -1): string # TODO

  #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  #+++ ready for build
  #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  # remember today
  # divide strftime('%d') by 1 so as to get "1,2,3 .. 9" instead of "01, 02, 03 .. 09"
  var vtoday = $'{strftime('%Y')}{strftime('%m')}{strftime('%d')}'

  # get arguments
  var dir = a1
  var vyear = str2nr(strftime('%Y'))
  var vmnth = str2nr(matchstr(strftime('%m'), '[^0].*'))
  if a2 != -1 && a3 == -1
    vmnth = a2
  elseif a2 != -1 && a3 != -1
    vyear = a2
    vmnth = a3
  endif

  # remember constant
  var vmnth_org: number = vmnth
  var vyear_org: number = vyear

  if dir != 2
    # start with last month
    vmnth = vmnth - 1
    if vmnth < 1
      vmnth = 12
      vyear = vyear - 1
    endif
  endif

  # reset display variables
  var vdisplay1 = ''
  var vdisplay2 = ''
  var vheight = 1

  #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  #+++ build display
  #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  if exists("g:calendar_begin")
    var CalendarBegin = function(g:calendar_begin)
    CalendarBegin()
  endif

  var vmcntmax = 1
  var whitehrz = ''
  var whiteleft = ''
  var whitevrt = ''
  var width = 0
  var height = 0
  var whitevrtweeknm = ''
  var vwruler = "Su Mo Tu We Th Fr Sa"
  var borderhrz = ''
  if dir == 2
    if !exists('b:CalendarDir') && !(bufname('%') == '' && !&modified)
      width = &columns
      height = &lines - 2
    else
      width = winwidth(0)
      height = winheight(0)
    endif
    var hrz = width / 8 - 5
    if hrz < 0
      hrz = 0
    endif
    var h = 0
    while h < hrz
      whitehrz = $'{whitehrz} '
      h = h + 1
    endwhile
    whitehrz = $'{whitehrz}|'
    var navifix = exists('g:calendar_navi') && g:calendar_navi == 'both' ? 2 : 0
    var vrt = (height - &cmdheight - 3 - navifix) / 6 - 2
    if vrt < 0
      vrt = 0
    endif
    var whitevrta = ''
    if whitehrz == '|'
      whitevrta = whitehrz
    else
      whitevrta = whitehrz[1 : ]
    endif
    h = 0
    var leftmargin = (width - (strlen(whitehrz) + 3) * 7 - 1) / 2
    while h < leftmargin
      whiteleft = $'{whiteleft} '
      h = h + 1
    endwhile
    h = 0
    while h < vrt
      whitevrt = $"{whitevrt}\n{whiteleft}|"
      var i = 0
      while i < 7
        whitevrt = $"{whitevrt}   {whitehrz}"
        i = i + 1
      endwhile
      h = h + 1
    endwhile
    whitevrt = $"{whitevrt}\n"
    var whitevrt2 = $"{whiteleft}+"
    h = 0
    borderhrz = $"---{substitute(substitute(whitehrz, ' ', '-', 'g'), '|', '+', '')}"
    while h < 7
      whitevrt2 = $"{whitevrt2}{borderhrz}"
      h = h + 1
    endwhile
    whitevrtweeknm = $"{whitevrt}{whitevrt2}\n"
    whitevrt = $"{whitevrta}{whitevrt}{whitevrt2}\n"
    fridaycol = (strlen(whitehrz) + 3) * 5 + strlen(whiteleft) + 1
    saturdaycol = (strlen(whitehrz) + 3) * 6 + strlen(whiteleft) + 1
  else
    vmcntmax = get(g:, 'calendar_number_of_months', 3)
  endif

  # Init variables for while loop
  var vcolumn = 22
  var vnweek = -1
  var vmdays = 0
  var vparam = 0
  var vsmnth = ''

  var vmcnt = 0
  while vmcnt < vmcntmax
    vwruler = "Su Mo Tu We Th Fr Sa"
    vcolumn = 22
    vnweek = -1
    vmdays = 0
    vparam = 0
    vsmnth = ''
    #--------------------------------------------------------------
    #--- calculating
    #--------------------------------------------------------------
    # set boundary of the month
    if vmnth == 1
      vmdays = 31
      vparam = 1
      vsmnth = 'Jan'
    elseif vmnth == 2
      vmdays = 28
      vparam = 32
      vsmnth = 'Feb'
    elseif vmnth == 3
      vmdays = 31
      vparam = 60
      vsmnth = 'Mar'
    elseif vmnth == 4
      vmdays = 30
      vparam = 91
      vsmnth = 'Apr'
    elseif vmnth == 5
      vmdays = 31
      vparam = 121
      vsmnth = 'May'
    elseif vmnth == 6
      vmdays = 30
      vparam = 152
      vsmnth = 'Jun'
    elseif vmnth == 7
      vmdays = 31
      vparam = 182
      vsmnth = 'Jul'
    elseif vmnth == 8
      vmdays = 31
      vparam = 213
      vsmnth = 'Aug'
    elseif vmnth == 9
      vmdays = 30
      vparam = 244
      vsmnth = 'Sep'
    elseif vmnth == 10
      vmdays = 31
      vparam = 274
      vsmnth = 'Oct'
    elseif vmnth == 11
      vmdays = 30
      vparam = 305
      vsmnth = 'Nov'
    elseif vmnth == 12
      vmdays = 31
      vparam = 335
      vsmnth = 'Dec'
    else
      echo 'Invalid Year or Month'
      return ''
    endif
    var vleap = false
    if vyear % 400 == 0
      vleap = true
      if vmnth == 2
        vmdays = 29
      elseif vmnth >= 3
        vparam = vparam + 1
      endif
    elseif vyear % 100 == 0
      if vmnth == 2
        vmdays = 28
      endif
    elseif vyear % 4 == 0
      vleap = true
      if vmnth == 2
        vmdays = 29
      elseif vmnth >= 3
        vparam = vparam + 1
      endif
    endif

    # calc vnweek of the day
    if vnweek == -1
      vnweek = ( vyear * 365 ) + vparam
      vnweek = vnweek + ( vyear / 4 ) - ( vyear / 100 ) + ( vyear / 400 )
      if vleap
        vnweek = vnweek - 1
      endif
      vnweek = vnweek - 1
    endif

    # fix Gregorian
    if vyear <= 1752
      vnweek = vnweek - 3
    endif

    vnweek = vnweek % 7

    if exists('g:calendar_monday') && g:calendar_monday
      # if given g:calendar_monday, the week start with monday
      if vnweek == 0
        vnweek = 7
      endif
      vnweek = vnweek - 1
    endif

    var vfweek = 0
    var vfweekl = 0
    var viweek = 0
    if exists('g:calendar_weeknm')
      # if given g:calendar_weeknm, show week number(ref:ISO8601)

      #vparam <= 1. day of month
      #vnweek <= 1. weekday of month (0-6)
      #viweek <= number of week
      #vfweek <= 1. day of year

      # Mon Tue Wed Thu Fri Sat Sun
      # 6   5   4   3   2   1   0  vfweek
      # 0   1   2   3   4   5   6  vnweek

      vfweek = ((vparam % 7)  - vnweek + 14 - 2) % 7
      viweek = (vparam - vfweek - 2 + 7 ) / 7 + 1

      if vfweek < 3
        viweek = viweek - 1
      endif

      #vfweekl  <=year length
      vfweekl = 52
      if vfweek == 3 || (vfweek == 4 && vleap)
        vfweekl = 53
      endif

      if viweek == 0
        #belongs to last week number of previous year
        viweek = 52
        vleap = ((vyear - 1) % 4 == 0 &&
          ((vyear - 1) % 100 != 0 || (vyear - 1) % 400 == 0))
        if vfweek == 2 || (vfweek == 1 && vleap)
          viweek = 53
        endif
      endif

      vcolumn = vcolumn + 5
      if g:calendar_weeknm == 5
        vcolumn = vcolumn - 2
      endif
    endif

    #--------------------------------------------------------------
    #--- displaying
    #--------------------------------------------------------------
    # build header
    if exists('g:calendar_erafmt') && g:calendar_erafmt !~ "^\s*$"
      if g:calendar_erafmt =~ '.*, [+-]*\d\+'
        var veranum = str2nr(substitute(g:calendar_erafmt, '.*, \([+-]*\d\+\)', '\1', ''))
        if vyear + veranum > 0
          vdisplay2 = substitute(g:calendar_erafmt, '\(.*\), .*', '\1', '')
          vdisplay2 = $"{vdisplay2}{string(vyear + veranum)}/{string(vmnth)}("
        else
          vdisplay2 = $"{string(vyear)}/{string(vmnth)}("
        endif
      else
        vdisplay2 = $"{string(vyear)}/{string(vmnth)}("
      endif
      vdisplay2 = strpart("                           ",
        1, (vcolumn - strlen(vdisplay2)) / 2 - 2) .. vdisplay2
    else
      vdisplay2 = $"{string(vyear)}/{string(vmnth)}("
      vdisplay2 = strpart("                           ",
        1, (vcolumn - strlen(vdisplay2)) / 2 - 2) .. vdisplay2
    endif

    if exists('g:calendar_mruler') && g:calendar_mruler !~ "^\s*$"
      vdisplay2 = $"{vdisplay2}{get(split(g:calendar_mruler, ', '), vmnth - 1, '')})\n"
    else
      vdisplay2 = $"{vdisplay2}{vsmnth})\n"
    endif

    if exists('g:calendar_wruler') && g:calendar_wruler !~ "^\s*$"
      vwruler = g:calendar_wruler
    endif

    if exists('g:calendar_monday') && g:calendar_monday
      vwruler = $"{strpart(vwruler, stridx(vwruler, ' ') + 1)} "
        .. strpart(vwruler, 0, stridx(vwruler, ' '))
    endif

    if dir == 2
      var whiteruler = substitute(substitute(whitehrz, ' ', '_', 'g'), '__', '  ', '')
      vwruler = $"| {substitute(vwruler, ' ', whiteruler .. ' ', 'g')}{whiteruler}"
      vdisplay2 = $"{vdisplay2}{whiteleft}{vwruler}\n"
    else
      vdisplay2 = $"{vdisplay2} {vwruler}\n"
    endif

    if g:calendar_mark == 'right' && dir != 2
      vdisplay2 = $"{vdisplay2} "
    endif

    # build calendar
    var vinpcur = 0
    while (vinpcur < vnweek)
      if dir == 2
        if vinpcur % 7 != 0
          vdisplay2 = $"{vdisplay2}{whitehrz}"
        else
          vdisplay2 = $"{vdisplay2}{whiteleft}|"
        endif
      endif
      vdisplay2 = $"{vdisplay2}   "
      vinpcur = vinpcur + 1
    endwhile

    var vdaycur = 1
    while (vdaycur <= vmdays)
      if dir == 2
        if vinpcur % 7 != 0
          vdisplay2 = $"{vdisplay2}{whitehrz}"
        else
          vdisplay2 = $"{vdisplay2}{whiteleft}|"
        endif
      endif
      var vtarget = ''
      if vmnth < 10
        vtarget = $"{string(vyear)}0{string(vmnth)}"
      else
        vtarget = $"{string(vyear)}{string(vmnth)}"
      endif
      if vdaycur < 10
        vtarget = $"{vtarget}0{vdaycur}"
      else
        vtarget = $"{vtarget}{vdaycur}"
      endif
      # TODO: what is tried to achieve? --------------------
      # if exists("g:calendar_sign") && g:calendar_sign != ""
      #   exe "let vsign = " . g:calendar_sign . "(vdaycur, vmnth, vyear)"
      #   if vsign != ""
      #     let vsign = vsign[0]
      #     if vsign !~ "[+!#$%&@?]"
      #       let vsign = "+"
      #     endif
      #   endif
      # else
      #   let vsign = ''
      # endif
      # ----------------------------------------------------
      #
      # Vim9 patch
      var vsign = Sign(vdaycur, vmnth, vyear) ? '+' : ''

      # show mark
      if g:calendar_mark == 'right'
        if vdaycur < 10
          vdisplay2 = $"{vdisplay2} "
        endif
        vdisplay2 = $"{vdisplay2}{vdaycur}"
      elseif g:calendar_mark == 'left-fit'
        if vdaycur < 10
          vdisplay2 = $"{vdisplay2} "
        endif
      endif

      if vtarget == vtoday
        vdisplay2 = $"{vdisplay2}*"
      elseif vsign != ''
        vdisplay2 = $"{vdisplay2}{vsign}"
      else
        vdisplay2 = $"{vdisplay2} "
      endif
      if g:calendar_mark == 'left'
        if vdaycur < 10
          vdisplay2 = $"{vdisplay2} "
        endif
        vdisplay2 = $"{vdisplay2}{vdaycur}"
      endif
      if g:calendar_mark == 'left-fit'
        vdisplay2 = $"{vdisplay2}{vdaycur}"
      endif
      vdaycur = vdaycur + 1

      # fix Gregorian
      if vyear == 1752 && vmnth == 9 && vdaycur == 3
        vdaycur = 14
      endif

      vinpcur = vinpcur + 1
      if vinpcur % 7 == 0
        if exists('g:calendar_weeknm')
          if dir == 2
            vdisplay2 = $"{vdisplay2}{whitehrz}"
          endif
          if g:calendar_mark != 'right'
            vdisplay2 = $"{vdisplay2} "
          endif
          # if given g:calendar_weeknm, show week number
          if viweek < 10
            if g:calendar_weeknm == 1
              vdisplay2 = $"{vdisplay2}WK0{viweek}"
            elseif g:calendar_weeknm == 2
              vdisplay2 = $"{vdisplay2}WK {viweek}"
            elseif g:calendar_weeknm == 3
              vdisplay2 = $"{vdisplay2}KW0{viweek}"
            elseif g:calendar_weeknm == 4
              vdisplay2 = $"{vdisplay2}KW {viweek}"
            elseif g:calendar_weeknm == 5
              vdisplay2 = $"{vdisplay2} {viweek}"
            endif
          else
            if g:calendar_weeknm <= 2
              vdisplay2 = $"{vdisplay2}WK{viweek}"
            elseif g:calendar_weeknm == 3 || g:calendar_weeknm == 4
              vdisplay2 = $"{vdisplay2}KW{viweek}"
            elseif g:calendar_weeknm == 5
              vdisplay2 = $"{vdisplay2}{viweek}"
            endif
          endif
          viweek = viweek + 1

          if viweek > vfweekl
            viweek = 1
          endif

        endif
        vdisplay2 = $"{vdisplay2}\n"
        if g:calendar_mark == 'right' && dir != 2
          vdisplay2 = $"{vdisplay2} "
        endif
      endif
    endwhile

    # if it is needed, fill with space
    if vinpcur % 7 != 0
      while (vinpcur % 7 != 0)
        if dir == 2
          vdisplay2 = $"{vdisplay2}{whitehrz}"
        endif
        vdisplay2 = $"{vdisplay2}   "
        vinpcur = vinpcur + 1
      endwhile
      if exists('g:calendar_weeknm')
        if dir == 2
          vdisplay2 = $"{vdisplay2}{whitehrz}"
        endif
        if g:calendar_mark != 'right'
          vdisplay2 = $"{vdisplay2} "
        endif
        if viweek < 10
          if g:calendar_weeknm == 1
            vdisplay2 = $"{vdisplay2}WK0{viweek}"
          elseif g:calendar_weeknm == 2
            vdisplay2 = $"{vdisplay2}WK{viweek}"
          elseif g:calendar_weeknm == 3
            vdisplay2 = $"{vdisplay2}KW0{viweek}"
          elseif g:calendar_weeknm == 4
            vdisplay2 = $"{vdisplay2}KW{viweek}"
          elseif g:calendar_weeknm == 5
            vdisplay2 = $"{vdisplay2} {viweek}"
          endif
        else
          if g:calendar_weeknm <= 2
            vdisplay2 = $"{vdisplay2}WK{viweek}"
          elseif g:calendar_weeknm == 3 || g:calendar_weeknm == 4
            vdisplay2 = $"{vdisplay2}KW{viweek}"
          elseif g:calendar_weeknm == 5
            vdisplay2 = $"{vdisplay2}{viweek}"
          endif
        endif
      endif
    endif

    # build display
    var vstrline = ''
    var vtokline = 1
    var vtoken1 = ''
    var vtoken2 = ''
    if dir == 1
      # for horizontal
      #--------------------------------------------------------------
      # +---+   +---+   +------+
      # |   |   |   |   |      |
      # | 1 | + | 2 | = |  1'  |
      # |   |   |   |   |      |
      # +---+   +---+   +------+
      #--------------------------------------------------------------
      while 1
        vtoken1 = get(split(vdisplay1, "\n"), vtokline - 1, '')
        vtoken2 = get(split(vdisplay2, "\n"), vtokline - 1, '')
        if vtoken1 == '' && vtoken2 == ''
          break
        endif
        while strlen(vtoken1) < (vcolumn + 1) * vmcnt
          if strlen(vtoken1) % (vcolumn + 1) == 0
            vtoken1 = $"{vtoken1}|"
          else
            vtoken1 = $"{vtoken1} "
          endif
        endwhile
        vstrline = $"{vstrline}{vtoken1}|{vtoken2} \n"
        vtokline = vtokline + 1
      endwhile
      vdisplay1 = vstrline
      vheight = vtokline - 1
    elseif (dir == 0 || dir == 3)
      # for vertical
      #--------------------------------------------------------------
      # +---+   +---+   +---+
      # | 1 | + | 2 | = |   |
      # +---+   +---+   | 1'|
      #                 |   |
      #                 +---+
      #--------------------------------------------------------------
      vtokline = 1
      while 1
        vtoken1 = get(split(vdisplay1, "\n"), vtokline - 1, '')
        if vtoken1 == ''
          break
        endif
        vstrline = $"{vstrline}{vtoken1}\n"
        vtokline = vtokline + 1
        vheight = vheight + 1
      endwhile
      if vstrline != ''
        vstrline = $"{vstrline} \n"
        vheight = vheight + 1
      endif
      vtokline = 1
      while 1
        vtoken2 = get(split(vdisplay2, "\n"), vtokline - 1, '')
        if vtoken2 == ''
          break
        endif
        while strlen(vtoken2) < vcolumn
          vtoken2 = $"{vtoken2} "
        endwhile
        vstrline = $"{vstrline}{vtoken2}\n"
        vtokline = vtokline + 1
        vheight = vtokline + 1
      endwhile
      vdisplay1 = vstrline
    else
      vtokline = 1
      while 1
        vtoken1 = get(split(vdisplay1, "\n"), vtokline - 1, '')
        vtoken2 = get(split(vdisplay2, "\n"), vtokline - 1, '')
        if vtoken1 == '' && vtoken2 == ''
          break
        endif
        while strlen(vtoken1) < (vcolumn + 1) * vmcnt
          if strlen(vtoken1) % (vcolumn + 1) == 0
            vtoken1 = $"{vtoken1}|"
          else
            vtoken1 = $"{vtoken1} "
          endif
        endwhile
        var vright = ''
        if vtokline > 2
          if exists('g:calendar_weeknm')
            vright = whitevrtweeknm
          elseif whitehrz == '|'
            vright = whitevrt
          else
            vright = $" {whitevrt}"
          endif
        else
          vright = "\n"
        endif
        vstrline = $"{vstrline}{vtoken1}{vtoken2}{vright}"
        vtokline = vtokline + 1
      endwhile
      vdisplay1 = vstrline
      vheight = vtokline - 1
    endif

    vmnth = vmnth + 1
    vmcnt = vmcnt + 1
    if vmnth > 12
      vmnth = 1
      vyear = vyear + 1
    endif
  endwhile

  if exists("g:calendar_end")
    var CalendarEnd = function(g:calendar_end)
    CalendarEnd()
  endif
  if a1 == -1 && empty(a2) && empty(a3)
    return vdisplay1
  endif

  var diary_list = ''
  if exists("g:calendar_diary_list") && len(g:calendar_diary_list) > 0
    vdisplay1 = $"{vdisplay1}\nCalendar\n{repeat("-", vcolumn)}"
    var diary_index = 0
    for diary in g:calendar_diary_list
      if diary_index == g:calendar_diary_list_curr_idx
        diary_list = $"(*) {diary["name"]}"
        diary_list = $"\n{diary_list}{repeat(' ', vcolumn - len(diary_list))}"
      else
        diary_list = $"( ) {diary["name"]}"
        diary_list = $"\n{diary_list}{repeat(' ', vcolumn - len(diary_list))}"
      endif
      vdisplay1 = $"{vdisplay1}{diary_list}"
      diary_index = diary_index + 1
    endfor
  endif

  #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  #+++ build window
  #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  # make window
  var vwinnum = bufnr('__Calendar')
  if getbufvar(vwinnum, 'Calendar') == 'Calendar'
    vwinnum = bufwinnr(vwinnum)
  else
    vwinnum = - 1
  endif

  if vwinnum >= 0
    # if already exist
    if vwinnum != bufwinnr('%')
      exe $"{vwinnum}wincmd w"
    endif
    setlocal modifiable
    deletebufline('%', 1, '$')
  else
    # make title
    if g:calendar_datetime == "title" && !bufautocommandsset
      auto BufEnter *Calendar b:sav_titlestring = &titlestring | &titlestring = '%{strftime("%c")}'
      auto BufLeave *Calendar if exists('b:sav_titlestring') | &titlestring = b:sav_titlestring | endif
      bufautocommandsset = true
    endif

    if exists('g:calendar_navi') && !empty(dir)
      if g:calendar_navi == 'both'
        vheight = vheight + 4
      else
        vheight = vheight + 2
      endif
    endif

    # or not
    if dir == 1
      silent execute $"bo :{vheight}split __Calendar"
      setlocal winfixheight
    elseif dir == 0
      silent execute $"to :{vcolumn}vsplit __Calendar"
      setlocal winfixwidth
    elseif dir == 3
      silent execute $"bo :{vcolumn}vsplit __Calendar"
      setlocal winfixwidth
    elseif bufname('%') == '' && !&modified
      silent execute 'edit __Calendar'
    else
      silent execute 'tabnew __Calendar'
    endif
    CalendarBuildKeymap(dir, vyear, vmnth)
    setlocal noswapfile
    setlocal buftype=nofile
    setlocal bufhidden=delete
    silent! exe $"setlocal {g:calendar_options}"
    var nontext_columns = &nu ? &foldcolumn + &numberwidth : &foldcolumn
    if has("+relativenumber") || exists("+relativenumber")
      nontext_columns += &rnu ? &numberwidth : 0
    endif
    # Without this, the 'sidescrolloff' setting may cause the left side of the
    # calendar to disappear if the last inserted element is near the right
    # window border.
    setlocal nowrap
    setlocal norightleft
    setlocal modifiable
    setlocal nolist
    b:Calendar = 'Calendar'
    setlocal filetype=calendar
    # is this a vertical (0) or a horizontal (1) split?
    if dir != 2
      exe $":{vcolumn + nontext_columns}:wincmd |"
    endif
  endif
  if g:calendar_datetime == "statusline"
    setlocal statusline=%{strftime('%c')}
  endif
  b:CalendarDir = dir
  b:CalendarYear = vyear_org
  b:CalendarMonth = vmnth_org

  # navi
  var navcol = 0
  if exists('g:calendar_navi')
    var navi_label = '<'
      .. $"{get(split(g:calendar_navi_label, ', '), 0, '')} "
      .. $"{get(split(g:calendar_navi_label, ', '), 1, '')} "
      .. $"{get(split(g:calendar_navi_label, ', '), 2, '')}> "
    if dir == 1
      navcol = vcolumn + (vcolumn - strlen(navi_label) + 2) / 2
    elseif (dir == 0 || dir == 3)
      navcol = (vcolumn - strlen(navi_label) + 2) / 2
    else
      navcol = (width - strlen(navi_label)) / 2
    endif
    if navcol < 3
      navcol = 3
    endif

    if g:calendar_navi == 'top'
      execute $"normal gg{navcol}i "
      silent exec $"normal! a{navi_label}\<cr>\<cr>"
      silent put! = vdisplay1
    endif
    if g:calendar_navi == 'bottom'
      silent put! = vdisplay1
      silent exec "normal! Gi\<cr>"
      silent execute $"normal {navcol}i "
      silent exec $"normal! a{navi_label}"
    endif
    if g:calendar_navi == 'both'
      execute $"normal gg{navcol}i "
      silent exec $"normal! a{navi_label}\<cr>\<cr>"
      silent put! = vdisplay1
      silent exec "normal! Gi\<cr>"
      silent execute $"normal {navcol}i "
      silent exec $"normal! a{navi_label}"
    endif
  else
    silent put! = vdisplay1
  endif

  setlocal nomodifiable
  # In case we've gotten here from insert mode (via <C-O>:Calendar<CR>)...
  stopinsert

  vyear = vyear_org
  vmnth = vmnth_org

  #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  #+++ build highlight
  #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  # today
  syn clear
  if g:calendar_mark =~ 'left-fit'
    syn match CalToday display "\s*\*\d*"
    syn match CalMemo display "\s*[+!#$%&@?]\d*"
  elseif g:calendar_mark =~ 'right'
    syn match CalToday display "\d*\*\s*"
    syn match CalMemo display "\d*[+!#$%&@?]\s*"
  else
    syn match CalToday display "\*\s*\d*"
    syn match CalMemo display "[+!#$%&@?]\s*\d*"
  endif
  # header
  syn match CalHeader display "[^ ]*\d\+\/\d\+([^)]*)"

  # navi
  if exists('g:calendar_navi')
    exec "syn match CalNavi display \"\\(<"
      .. get(split(g:calendar_navi_label, ', '), 0, '') .. "\\|"
      .. get(split(g:calendar_navi_label, ', '), 2, '') .. ">\\)\""
    exec 'syn match CalNavi display "\s'
      .. get(split(g:calendar_navi_label, ', '), 1, '') .. '\s"hs=s+1,he=e-1'
  endif

  # saturday, sunday
  if exists('g:calendar_monday') && g:calendar_monday
    if dir == 1
      syn match CalSaturday display /|.\{15}\s\([0-9\ ]\d\)/hs=e-1 contains=ALL
      syn match CalSunday display /|.\{18}\s\([0-9\ ]\d\)/hs=e-1 contains=ALL
    elseif (dir == 0 || dir == 3)
      syn match CalSaturday display /^.\{15}\s\([0-9\ ]\d\)/hs=e-1 contains=ALL
      syn match CalSunday display /^.\{18}\s\([0-9\ ]\d\)/hs=e-1 contains=ALL
    else
      exec printf('syn match CalSunday display /^.\{%d}\s\?\([0-9\ ]\d\)/hs=e-1 contains=ALL', fridaycol)
      exec printf('syn match CalSaturday display /^.\{%d}\s\?\([0-9\ ]\d\)/hs=e-1 contains=ALL', saturdaycol)
    endif
  else
    if dir == 1
      syn match CalSaturday display /|.\{18}\s\([0-9\ ]\d\)/hs=e-1 contains=ALL
      syn match CalSunday display /|\s\([0-9\ ]\d\)/hs=e-1 contains=ALL
    elseif (dir == 0 || dir == 3)
      syn match CalSaturday display /^.\{18}\s\([0-9\ ]\d\)/hs=e-1 contains=ALL
      syn match CalSunday display /^\s\([0-9\ ]\d\)/hs=e-1 contains=ALL
    else
      exec printf('syn match CalSaturday display /^.\{%d}\s\?\([0-9\ ]\d\)/hs=e-1 contains=ALL', saturdaycol)
      syn match CalSunday display /^\s*|\s*\([0-9\ ]\d\)/hs=e-1 contains=ALL
    endif
  endif

  syn match CalCurrList display "^(\*).*$"

  # week number
  if !exists('g:calendar_weeknm') || g:calendar_weeknm <= 2
    syn match CalWeeknm display "WK[0-9\ ]\d"
  else
    syn match CalWeeknm display "KW[0-9\ ]\d"
  endif

  # ruler
  execute $'syn match CalRuler "{vwruler}"'

  if search("\*", "w") > 0
    silent execute "normal! gg/\*\<cr>"
  endif

  # --+--
  if dir == 2
    exec "syn match CalNormal display " string(borderhrz)
    exec "syn match CalNormal display " string($'^{whiteleft}+')
  endif

  return ''
enddef

# #*****************************************************************
# #* Make_dir : make directory
# #*----------------------------------------------------------------
# #*   dir : directory
# #*****************************************************************
def MakeDir(dir: string): number
  var rc: number = 0
  if (has("unix"))
    system($"mkdir {dir}")
    rc = v:shell_error
  elseif (has("win16") || has("win32") || has("win95") ||
      has("dos16") || has("dos32") || has("os2"))
    system($"mkdir \"{dir}\"")
    rc = v:shell_error
  else
    rc = 1
  endif
  if rc != 0
    confirm($"can't create directory: {dir}", "&OK")
  endif
  return rc
enddef

# #*****************************************************************
# #* diary : calendar hook function
# #*----------------------------------------------------------------
# #*   day   : day you actioned
# #*   month : month you actioned
# #*   year  : year you actioned
# #*****************************************************************
def Diary(day: number, month: number, year: number, week: number, dir: string)
  # build the file name and create directories as needed
  if !isdirectory(expand(g:calendar_diary))
    confirm("please create diary directory: {g:calendar_diary}", 'OK')
    return
  endif
  var sfile = $"{expand(g:calendar_diary)}/{printf("%04d", year)}"
  if isdirectory(sfile) == 0
    if MakeDir(sfile) != 0
      return
    endif
  endif
  sfile = $"{sfile}/{printf("%02d", month)}"
  if isdirectory(sfile) == 0
    if MakeDir(sfile) != 0
      return
    endif
  endif
  sfile = $'{expand(sfile)}/{printf("%02d", day)}{g:calendar_diary_extension}'
  sfile = substitute(sfile, ' ', '\\ ', 'g')
  var vbufnr = bufnr('__Calendar')

  # load the file
  exe "wincmd w"
  exe $"edit  {sfile}"
  exe $"setfiletype {g:calendar_filetype}"
  var folder = getbufvar(vbufnr, "CalendarDir")
  var vyear = getbufvar(vbufnr, "CalendarYear")
  var vmnth = getbufvar(vbufnr, "CalendarMonth")
  exe $"auto BufDelete {escape(sfile, ' \\')} Show({folder}, {vyear}, {vmnth})"
enddef

var CalendarAction = function("Diary")
if exists("g:calendar_action")
  CalendarAction = function(g:calendar_action)
endif

# #*****************************************************************
# #* sign : calendar sign function
# #*----------------------------------------------------------------
# #*   day   : day of sign
# #*   month : month of sign
# #*   year  : year of sign
# #*****************************************************************
def Sign(day: number, month: number, year: number): bool
  var sfile = $'{g:calendar_diary}/{printf("%04d", year)}/{printf("%02d", month)}/'
   .. $'{printf("%02d", day)}{g:calendar_diary_extension}'
  return filereadable(expand(sfile))
enddef

var CalendarSign = function("Sign")
if exists("g:calendar_sign")
  CalendarSign = function(g:calendar_sign)
endif

#*****************************************************************
#* CalendarBuildKeymap : build keymap
#*----------------------------------------------------------------
#*****************************************************************
def CalendarBuildKeymap(dir: number, vyear: number, vmnth: number)
# make keymap
nnoremap <silent> <buffer> <Plug>CalendarClose <ScriptCmd>Close()<cr>
nnoremap <silent> <buffer> <Plug>CalendarDoAction <ScriptCmd>Action()<cr>
nnoremap <silent> <buffer> <Plug>CalendarDoAction <ScriptCmd>Action()<cr>
nnoremap <silent> <buffer> <Plug>CalendarGotoToday <ScriptCmd>Show(b:CalendarDir)<cr>
nnoremap <silent> <buffer> <Plug>CalendarShowHelp <ScriptCmd>CalendarHelp()<cr>
execute $'nnoremap <silent> <buffer> <Plug>CalendarReDisplay <ScriptCmd>Show({dir}, {vyear}, {vmnth})<cr>'
var pnav = get(split(g:calendar_navi_label, ', '), 0, '')
var nnav = get(split(g:calendar_navi_label, ', '), 2, '')
execute $'nnoremap <silent> <buffer> <Plug>CalendarGotoPrevMonth <ScriptCmd>Action("<{pnav}")<cr>'
execute $'nnoremap <silent> <buffer> <Plug>CalendarGotoNextMonth <ScriptCmd>Action("{nnav}>")<cr>'
execute 'nnoremap <silent> <buffer> <Plug>CalendarGotoPrevYear <ScriptCmd>Action("PrevYear")<cr>'
execute 'nnoremap <silent> <buffer> <Plug>CalendarGotoNextYear <ScriptCmd>Action("NextYear")<cr>'

nmap <buffer> <2-LeftMouse> <Plug>CalendarDoAction

execute $'nmap <buffer> {calendar_keys['close']} <Plug>CalendarClose'
execute $'nmap <buffer> {calendar_keys['do_action']} <Plug>CalendarDoAction'
execute $'nmap <buffer> {calendar_keys['goto_today']} <Plug>CalendarGotoToday'
execute $'nmap <buffer> {calendar_keys['show_help']} <Plug>CalendarShowHelp'
execute $'nmap <buffer> {calendar_keys['redisplay']} <Plug>CalendarRedisplay'

execute $'nmap <buffer> {calendar_keys['goto_next_month']} <Plug>CalendarGotoNextMonth'
execute $'nmap <buffer> {calendar_keys['goto_prev_month']} <Plug>CalendarGotoPrevMonth'
execute $'nmap <buffer> {calendar_keys['goto_next_year']} <Plug>CalendarGotoNextYear'
execute $'nmap <buffer> {calendar_keys['goto_prev_year']} <Plug>CalendarGotoPrevYear'
enddef

# #*****************************************************************
# #* CalendarHelp : show help for Calendar
# #*----------------------------------------------------------------
# #*****************************************************************
def CalendarHelp()
  var ck = calendar_keys
  var max_width = values(ck)->mapnew((_, val) => len(val))->max()
  var offsets = ck->mapnew((_, val) => 1 + max_width - len(val))

  echohl SpecialKey
  echo $"{ck['goto_prev_month']}{repeat(' ', offsets['goto_prev_month'])}': goto prev month'"
  echo $"{ck['goto_next_month']}{repeat(' ', offsets['goto_next_month'])}': goto next month'"
  echo $"{ck['goto_prev_year']}{repeat(' ', offsets['goto_prev_year'])}': goto prev year'"
  echo $"{ck['goto_next_year']}{repeat(' ', offsets['goto_next_year'])}': goto next year'"
  echo $"{ck['goto_today']}{repeat(' ', offsets['goto_today'])}': goto today'"
  echo $"{ck['close']}{repeat(' ', offsets['close'])}': close window'"
  echo $"{ck['redisplay']}{repeat(' ', offsets['redisplay'])}': re-display window'"
  echo $"{ck['show_help']}{repeat(' ', offsets['show_help'])}': show this help'"
  if CalendarAction == function("Diary")
    echo $"{ck['do_action']}{repeat(' ', offsets['do_action'])}': show diary'"
  endif
  echo ''
  echohl Question

  var vk = [
    'calendar_erafmt',
    'calendar_mruler',
    'calendar_wruler',
    'calendar_weeknm',
    'calendar_navi_label',
    'calendar_diary',
    'calendar_mark',
    'calendar_navi',
  ]
  max_width = max(map(copy(vk), 'len(v:val)'))

  for v in vk
    var vv = get(g:, v, '')
    echo $"{v}{repeat(' ', max_width - len(v))} = {vv}"
  endfor
  echohl MoreMsg
  echo "[Hit any key]"
  echohl None
  getchar()
  redraw!
enddef

export def Search(keyword: string)
  if g:calendar_search_grepprg == "internal"
    exe $"vimgrep /{keyword}/{escape(g:calendar_diary, " ")}/**/*{g:calendar_diary_extension}|cw"
  else
    silent execute $"{g:calendar_search_grepprg} '{keyword}' {escape(g:calendar_diary, ' ')}/**/*{g:calendar_diary_extension}"
    silent execute "cw"
  endif
enddef

hi def link CalNavi     Search
hi def link CalSaturday Type
hi def link CalSunday   Statement
hi def link CalRuler    StatusLine
hi def link CalWeeknm   Comment
hi def link CalToday    Directory
hi def link CalHeader   Special
hi def link CalMemo     Identifier
hi def link CalNormal   Normal
hi def link CalCurrList Error
