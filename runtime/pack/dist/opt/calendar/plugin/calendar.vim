vim9script

#=============================================================================
# What Is This: Calendar
# File: calendar.vim
# Author: Yasuhiro Matsumoto <mattn.jp@gmail.com>
# Last Change: 2025 Aug 10
# Version: 2.9 TODO ???
# Thanks:
#     Ubaldo Tiberi                 : porting to Vim9
#     Tobias Columbus               : customizable key bindings
#     Daniel P. Wright              : doc/calendar.txt
#     SethMilliken                  : gave a hint for 2.4
#     bw1                           : bug fix, new weeknm format
#     Ingo Karkat                   : bug fix
#     Thinca                        : bug report, bug fix
#     Yu Pei                        : bug report
#     Per Winkvist                  : bug fix
#     Serge (gentoosiast) Koksharov : bug fix
#     Vitor Antunes                 : bug fix
#     Olivier Mengue                : bug fix
#     Noel Henson                   : today action
#     Per Winkvist                  : bug report
#     Peter Findeisen               : bug fix
#     Chip Campbell                 : gave a hint for 1.3z
#     PAN Shizhu                    : gave a hint for 1.3y
#     Eric Wald                     : bug fix
#     Sascha Wuestemann             : advise
#     Linas Vasiliauskas            : bug report
#     Per Winkvist                  : bug report
#     Ronald Hoelwarth              : gave a hint for 1.3s
#     Vikas Agnihotri               : bug report
#     Steve Hall                    : gave a hint for 1.3q
#     James Devenish                : bug fix
#     Carl Mueller                  : gave a hint for 1.3o
#     Klaus Fabritius               : bug fix
#     Stucki                        : gave a hint for 1.3m
#     Rosta                         : bug report
#     Richard Bair                  : bug report
#     Yin Hao Liew                  : bug report
#     Bill McCarthy                 : bug fix and gave a hint
#     Srinath Avadhanula            : bug fix
#     Ronald Hoellwarth             : few advices
#     Juan Orlandini                : added higlighting of days with data
#     Ray                           : bug fix
#     Ralf.Schandl                  : gave a hint for 1.3
#     Bhaskar Karambelkar           : bug fix
#     Suresh Govindachar            : gave a hint for 1.2, bug fix
#     Michael Geddes                : bug fix
#     Leif Wickland                 : bug fix
# ChangeLog:
#     2.8  : bug fix
#     2.7  : vim7ish, customizable key bindings
#     2.6  : new week number format
#     2.5  : bug fix, 7.2 don't have relativenumber.
#     2.4  : added g:calendar_options.
#     2.3  : week number like ISO8601
#            g:calendar_monday and g:calendar_weeknm work together
#     2.2  : http://gist.github.com/355513#file_customizable_keymap.diff
#            http://gist.github.com/355513#file_winfixwidth.diff
#     2.1  : bug fix, set filetype 'calendar'.
#     2.0  : bug fix, many bug fix and enhancements.
#     1.9  : bug fix, use nnoremap.
#     1.8  : bug fix, E382 when close diary.
#     1.7  : bug fix, week number was broken on 2008.
#     1.6  : added calendar_begin action.
#            added calendar_end action.
#     1.5  : bug fix, fixed ruler formating with strpart.
#            bug fix, using winfixheight.
#     1.4a : bug fix, week number was broken on 2005.
#            added calendar_today action.
#            bug fix, about wrapscan.
#            bug fix, about today mark.
#            bug fix, about today navigation.
#     1.4  : bug fix, and one improvement.
#            bug 1:
#              when marking the current date, there is not distinguished e.g. between
#              20041103 and 20040113, both dates are marked as today
#            bug 2:
#              the navigation mark "today" doesn't work
#            improvement:
#              the mapping t worked only when today was displayed, now it works always
#              and redisplays the cuurent month and today
#     1.3z : few changes
#            asign <Left>, <Right> for navigation.
#            set ws for search navigation.
#            add tag for GetLatestVimScripts(AutoInstall)
#     1.3y : bug fix, few changes
#            changed color syntax name. (ex. CalNavi, see bottom of this)
#            changed a map CalendarV for <Leader>cal
#            changed a map CalendarH for <Leader>caL
#            (competitive map for cvscommand.vim)
#            the date on the right-hand side didn't work correctoly.
#            make a map to rebuild Calendar window(r).
#     1.3x : bug fix
#            viweek can't refer when not set calendar_weeknm.
#     1.3w : bug fix
#            on leap year, week number decreases.
#     1.3v : bug fix
#            add nowrapscan
#            use s:bufautocommandsset for making title
#            don't focus to navi when doubleclick bottom next>.
#     1.3u : bug fix
#             when enter diary first time,
#              it don't warn that you don't have diary directory.
#     1.3t : bug fix
#             make sure the variables for help
#     1.3s : bug fix
#             make a map CalendarV for <Leader>ca
#            add option calendar_navi_label
#             see Additional:
#            add option calendar_focus_today
#             see Additional:
#            add map ? for help
#     1.3r : bug fix
#             if clicked navigator, cursor go to strange position.
#     1.3q : bug fix
#             coundn't set calendar_navi
#              in its horizontal direction
#     1.3p : bug fix
#             coundn't edit diary when the calendar is
#              in its horizontal direction
#     1.3o : add option calendar_mark, and delete calendar_rmark
#             see Additional:
#            add option calendar_navi
#             see Additional:
#     1.3n : bug fix
#             s:CalendarSign() should use filereadable(expand(sfile)).
#     1.3m : tuning
#             using topleft or botright for opening Calendar.
#            use filereadable for s:CalendarSign().
#     1.3l : bug fix
#             if set calendar_monday, it can see that Sep 1st is Sat
#               as well as Aug 31st.
#     1.3k : bug fix
#             it didn't escape the file name on calendar.
#     1.3j : support for fixed Gregorian
#             added the part of Sep 1752.
#     1.3i : bug fix
#             Calculation mistake for week number.
#     1.3h : add option for position of displaying '*' or '+'.
#             see Additional:
#     1.3g : centering header
#            add option for show name of era.
#             see Additional:
#            bug fix
#             <Leader>ca didn't show current month.
#     1.3f : bug fix
#            there was yet another bug of today's sign.
#     1.3e : added usage for <Leader>
#            support handler for sign.
#            see Additional:
#     1.3d : added higlighting of days that have calendar data associated
#             with it.
#            bug fix for calculates date.
#     1.3c : bug fix for MakeDir()
#            if CalendarMakeDir(sfile) != 0
#               v
#            if s:CalendarMakeDir(sfile) != 0
#     1.3b : bug fix for calendar_monday.
#            it didn't work g:calendar_monday correctly.
#            add g:calendar_version.
#            add argument on action handler.
#            see Additional:
#     1.3a : bug fix for MakeDir().
#            it was not able to make directory.
#     1.3  : support handler for action.
#            see Additional:
#     1.2g : bug fix for today's sign.
#            it could not display today's sign correctly.
#     1.2f : bug fix for current Date.
#            vtoday variable calculates date as 'YYYYMMDD'
#            while the loop calculates date as 'YYYYMMD' i.e just 1 digit
#            for date if < 10 so if current date is < 10 , the if condiction
#            to check for current date fails and current date is not
#            highlighted.
#            simple solution changed vtoday calculation line divide the
#            current-date by 1 so as to get 1 digit date.
#     1.2e : change the way for setting title.
#            auto configuration for g:calendar_wruler with g:calendar_monday
#     1.2d : add option for show week number.
#              let g:calendar_weeknm = 1
#            add separator if horizontal.
#            change all option's name
#              g:calendar_mnth -> g:calendar_mruler
#              g:calendar_week -> g:calendar_wruler
#              g:calendar_smnd -> g:calendar_monday
#     1.2c : add option for that the week starts with monday.
#              let g:calendar_smnd = 1
#     1.2b : bug fix for modifiable.
#            setlocal nomodifiable (was set)
#     1.2a : add default options.
#            nonumber,foldcolumn=0,nowrap... as making gap
#     1.2  : support wide display.
#            add a command CalendarH
#            add map <s-left> <s-right>
#     1.1c : extra.
#            add a titlestring for today.
#     1.1b : bug fix by Michael Geddes.
#            it happend when do ':Calender' twice
#     1.1a : fix misspell.
#            Calender -> Calendar
#     1.1  : bug fix.
#            it"s about strftime("%m")
#     1.0a : bug fix by Leif Wickland.
#            it"s about strftime("%w")
#     1.0  : first release.
# TODO:
#     add the option for diary which is separate or single file.

# *****************************************************************
# * Calendar commands
# *****************************************************************

if exists('g:loaded_calendar') && g:loaded_calendar
  finish
endif
g:loaded_calendar = true

import autoload "../autoload/calendar.vim"

command! -nargs=* Calendar  calendar.Show(0, <args>)
command! -nargs=* CalendarVR  calendar.Show(3, <args>)
command! -nargs=* CalendarH calendar.Show(1, <args>)
command! -nargs=* CalendarT calendar.Show(2, <args>)

command! -nargs=* CalendarSearch calendar.Search("<args>")

if !get(g:, 'calendar_no_mappings', false)
  if !hasmapto('<Plug>CalendarV')
    nmap <unique> <Leader>cal <Plug>CalendarV
  endif
  if !hasmapto('<Plug>CalendarH')
    nmap <unique> <Leader>caL <Plug>CalendarH
  endif
endif
nnoremap <silent> <Plug>CalendarV <ScriptCmd>calendar.Show(0)<CR>
nnoremap <silent> <Plug>CalendarH <ScriptCmd>calendar.Show(1)<CR>
nnoremap <silent> <Plug>CalendarT <ScriptCmd>calendar.Show(2)<CR>

# vi: et sw=2 ts=2
