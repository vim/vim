" Test for the calendar plugin.
" Some features are not tested, like CalendarT since it depends on the screen 
" resolution and the screen size. The same for the test 


packadd calendar

func Test_calendar_basic()

  Calendar 1998, 10
  call WaitForAssert({-> assert_equal(2, winnr('$'))})

  " Check the current layout should be something like: ['row', [['leaf', 1000], ['leaf', 1003]]]
  let expected_value = 'row'
  call assert_equal(expected_value, winlayout()[0])
  let expected_value = 2
  call assert_equal(expected_value, len(winlayout()))
  call assert_equal(expected_value, len(winlayout()[1]))

  " Check October 1998
  let expected_value =<< END
   <Prev Today Next> 

     1998/9(Sep)      
 Su Mo Tu We Th Fr Sa 
        1  2  3  4  5 
  6  7  8  9 10 11 12 
 13 14 15 16 17 18 19 
 20 21 22 23 24 25 26 
 27 28 29 30          
 
     1998/10(Oct)     
 Su Mo Tu We Th Fr Sa 
              1  2  3 
  4  5  6  7  8  9 10 
 11 12 13 14 15 16 17 
 18 19 20 21 22 23 24 
 25 26 27 28 29 30 31 
 
     1998/11(Nov)     
 Su Mo Tu We Th Fr Sa 
  1  2  3  4  5  6  7 
  8  9 10 11 12 13 14 
 15 16 17 18 19 20 21 
 22 23 24 25 26 27 28 
 29 30                

END
  call assert_equal(expected_value, getline(1, '$'))

  " Move to next page
  exe "norm \<right>"

  let expected_value_right =<< END
   <Prev Today Next> 

     1998/10(Oct)     
 Su Mo Tu We Th Fr Sa 
              1  2  3 
  4  5  6  7  8  9 10 
 11 12 13 14 15 16 17 
 18 19 20 21 22 23 24 
 25 26 27 28 29 30 31 
 
     1998/11(Nov)     
 Su Mo Tu We Th Fr Sa 
  1  2  3  4  5  6  7 
  8  9 10 11 12 13 14 
 15 16 17 18 19 20 21 
 22 23 24 25 26 27 28 
 29 30                
 
     1998/12(Dec)     
 Su Mo Tu We Th Fr Sa 
        1  2  3  4  5 
  6  7  8  9 10 11 12 
 13 14 15 16 17 18 19 
 20 21 22 23 24 25 26 
 27 28 29 30 31       

END
  call assert_equal(expected_value_right, getline(1, '$'))

  " Move back to the previous page
  exe "norm \<left>"
  call assert_equal(expected_value, getline(1, '$'))

  " Close calendar
  exe "norm q"
  call assert_equal(1, winnr('$'))

  %bw!
endfunc

func Test_calendarH_basic()

  CalendarH 2020, 2
  call WaitForAssert({-> assert_equal(2, winnr('$'))})

  " Check the current layout should be something like: ['col', [['leaf', 1000], ['leaf', 1003]]]
  let expected_value = 'col'
  call assert_equal(expected_value, winlayout()[0])
  let expected_value = 2
  call assert_equal(expected_value, len(winlayout()))
  call assert_equal(expected_value, len(winlayout()[1]))

  " Check February 2020 (Covid time!)
  let expected_value =<< END
                         <Prev Today Next> 

|     2020/1(Jan)      |     2020/2(Feb)      |     2020/3(Mar) 
| Su Mo Tu We Th Fr Sa | Su Mo Tu We Th Fr Sa | Su Mo Tu We Th Fr Sa 
|           1  2  3  4 |                    1 |  1  2  3  4  5  6  7 
|  5  6  7  8  9 10 11 |  2  3  4  5  6  7  8 |  8  9 10 11 12 13 14 
| 12 13 14 15 16 17 18 |  9 10 11 12 13 14 15 | 15 16 17 18 19 20 21 
| 19 20 21 22 23 24 25 | 16 17 18 19 20 21 22 | 22 23 24 25 26 27 28 
| 26 27 28 29 30 31    | 23 24 25 26 27 28 29 | 29 30 31             

END
  call assert_equal(expected_value, getline(1, '$'))

  " Move to next page
  exe "norm \<right>"

  let expected_value_right =<< END
                         <Prev Today Next> 

|     2020/2(Feb)      |     2020/3(Mar)      |     2020/4(Apr) 
| Su Mo Tu We Th Fr Sa | Su Mo Tu We Th Fr Sa | Su Mo Tu We Th Fr Sa 
|                    1 |  1  2  3  4  5  6  7 |           1  2  3  4 
|  2  3  4  5  6  7  8 |  8  9 10 11 12 13 14 |  5  6  7  8  9 10 11 
|  9 10 11 12 13 14 15 | 15 16 17 18 19 20 21 | 12 13 14 15 16 17 18 
| 16 17 18 19 20 21 22 | 22 23 24 25 26 27 28 | 19 20 21 22 23 24 25 
| 23 24 25 26 27 28 29 | 29 30 31             | 26 27 28 29 30       

END
  call assert_equal(expected_value_right, getline(1, '$'))

  " Move back to the previous page
  exe "norm \<left>"
  call assert_equal(expected_value, getline(1, '$'))

  " Close calendar
  exe "norm q"
  call assert_equal(1, winnr('$'))

  %bw!
endfunc

func Test_calendarVR_basic()
  " OBS: this test is identical to Test_calendar_basic()

  CalendarVR 1998, 10
  call WaitForAssert({-> assert_equal(2, winnr('$'))})

  " Check the current layout should be something like: ['row', [['leaf', 1000], ['leaf', 1003]]]
  let expected_value = 'row'
  call assert_equal(expected_value, winlayout()[0])
  let expected_value = 2
  call assert_equal(expected_value, len(winlayout()))
  call assert_equal(expected_value, len(winlayout()[1]))

  " Check October 1998
  let expected_value =<< END
   <Prev Today Next> 

     1998/9(Sep)      
 Su Mo Tu We Th Fr Sa 
        1  2  3  4  5 
  6  7  8  9 10 11 12 
 13 14 15 16 17 18 19 
 20 21 22 23 24 25 26 
 27 28 29 30          
 
     1998/10(Oct)     
 Su Mo Tu We Th Fr Sa 
              1  2  3 
  4  5  6  7  8  9 10 
 11 12 13 14 15 16 17 
 18 19 20 21 22 23 24 
 25 26 27 28 29 30 31 
 
     1998/11(Nov)     
 Su Mo Tu We Th Fr Sa 
  1  2  3  4  5  6  7 
  8  9 10 11 12 13 14 
 15 16 17 18 19 20 21 
 22 23 24 25 26 27 28 
 29 30                

END
  call assert_equal(expected_value, getline(1, '$'))

  " Move to next page
  exe "norm \<right>"

  let expected_value_right =<< END
   <Prev Today Next> 

     1998/10(Oct)     
 Su Mo Tu We Th Fr Sa 
              1  2  3 
  4  5  6  7  8  9 10 
 11 12 13 14 15 16 17 
 18 19 20 21 22 23 24 
 25 26 27 28 29 30 31 
 
     1998/11(Nov)     
 Su Mo Tu We Th Fr Sa 
  1  2  3  4  5  6  7 
  8  9 10 11 12 13 14 
 15 16 17 18 19 20 21 
 22 23 24 25 26 27 28 
 29 30                
 
     1998/12(Dec)     
 Su Mo Tu We Th Fr Sa 
        1  2  3  4  5 
  6  7  8  9 10 11 12 
 13 14 15 16 17 18 19 
 20 21 22 23 24 25 26 
 27 28 29 30 31       

END
  call assert_equal(expected_value_right, getline(1, '$'))

  " Move back to the previous page
  exe "norm \<left>"
  call assert_equal(expected_value, getline(1, '$'))

  " Close calendar
  exe "norm q"
  call assert_equal(1, winnr('$'))

  %bw!
endfunc

" TODO: help in writing this test
" func Test_calendarSearch_basic()
"   " Create file
"   let year = strftime('%y')
"   let month = strftime('%m')
"   let day = strftime('%m')

"   let filename = $"~/diary/{year}/{month}/{day}.md"->fnamemodify(':p')

"   Calendar
"   call WaitForAssert({-> assert_equal(2, winnr('$'))})
"   exe "norm \<cr>"

"   norm! iFoo
"   exe "norm! :wq\<cr>"

"   %bw!
" endfunc

func Test_config()
  " test g:calendar_navi
  let g:calendar_navi = 'both'
  let g:calendar_navi_label = '<--, |, -->'

  Calendar
  call WaitForAssert({-> assert_equal(2, winnr('$'))})

  " Check the current layout should be something like: ['row', [['leaf', 1000], ['leaf', 1003]]]
  let expected_value = 'row'
  call assert_equal(expected_value, winlayout()[0])
  let expected_value = 2
  call assert_equal(expected_value, len(winlayout()))
  call assert_equal(expected_value, len(winlayout()[1]))

  " Check label both on top and in the bottom
  let expected_value = '<<-- | -->>'
  call assert_match(expected_value, trim(getline(1)))
  call assert_match(expected_value, trim(getline('$')))

  exe "norm q"
  call assert_equal(1, winnr('$'))

  " test g:calendar_mark
  let g:calendar_mark = 'right'
  let today = strftime('%d')
  let expected_value = $'{today}\*'
  Calendar
  call WaitForAssert({-> assert_equal(2, winnr('$'))})
  call assert_match(matchstr(getline('.'), today), today)
  exe "norm q"
  call assert_equal(1, winnr('$'))

  let g:calendar_mark = 'left-fit'
  let expected_value = $'\*{today}'
  Calendar
  call WaitForAssert({-> assert_equal(2, winnr('$'))})
  call assert_match(matchstr(getline('.'), today), today)
  exe "norm q"
  call assert_equal(1, winnr('$'))

  " Test other opts
  let g:calendar_mruler = 'Gen, Feb, Mar, Apr, Mag, Giu, Lug, Ago, Set, Ott, Nov, Dic'
  let g:calendar_wruler = 'Do Lu Ma Me Gi Ve Sa'
  Calendar 2012, 1
  call WaitForAssert({-> assert_equal(2, winnr('$'))})
  
  let expected_value =<< END
      <<-- | -->> 

     2011/12(Dic)     
 Do Lu Ma Me Gi Ve Sa 
              1  2  3 
  4  5  6  7  8  9 10 
 11 12 13 14 15 16 17 
 18 19 20 21 22 23 24 
 25 26 27 28 29 30 31 
 
     2012/1(Gen)      
 Do Lu Ma Me Gi Ve Sa 
  1  2  3  4  5  6  7 
  8  9 10 11 12 13 14 
 15 16 17 18 19 20 21 
 22 23 24 25 26 27 28 
 29 30 31             
 
     2012/2(Feb)      
 Do Lu Ma Me Gi Ve Sa 
           1  2  3  4 
  5  6  7  8  9 10 11 
 12 13 14 15 16 17 18 
 19 20 21 22 23 24 25 
 26 27 28 29          

      <<-- | -->> 
END
  call assert_equal(expected_value, getline(1, '$')) 
  exe "norm q"
  call assert_equal(1, winnr('$'))
  
  " start from monday
  let g:calendar_monday = v:true
  CalendarVR 1990, 4

  let expected_value =<< END
      <<-- | -->> 

     1990/3(Mar)      
 Lu Ma Me Gi Ve Sa Do 
           1  2  3  4 
  5  6  7  8  9 10 11 
 12 13 14 15 16 17 18 
 19 20 21 22 23 24 25 
 26 27 28 29 30 31    
 
     1990/4(Apr)      
 Lu Ma Me Gi Ve Sa Do 
                    1 
  2  3  4  5  6  7  8 
  9 10 11 12 13 14 15 
 16 17 18 19 20 21 22 
 23 24 25 26 27 28 29 
 30                   
 
     1990/5(Mag)      
 Lu Ma Me Gi Ve Sa Do 
     1  2  3  4  5  6 
  7  8  9 10 11 12 13 
 14 15 16 17 18 19 20 
 21 22 23 24 25 26 27 
 28 29 30 31          

      <<-- | -->> 
END

  echom assert_equal(expected_value, getline(1, '$')) 
  exe "norm q"
  call assert_equal(1, winnr('$'))
  
  
  let g:calendar_weeknm = 2 " WK 1
  CalendarVR 1989, 11
  
  let expected_value =<< END
        <<-- | -->> 

       1989/10(Ott)        
 Lu Ma Me Gi Ve Sa Do      
                    1 WK39 
  2  3  4  5  6  7  8 WK40 
  9 10 11 12 13 14 15 WK41 
 16 17 18 19 20 21 22 WK42 
 23 24 25 26 27 28 29 WK43 
 30 31                WK44 
 
       1989/11(Nov)        
 Lu Ma Me Gi Ve Sa Do      
        1  2  3  4  5 WK44 
  6  7  8  9 10 11 12 WK45 
 13 14 15 16 17 18 19 WK46 
 20 21 22 23 24 25 26 WK47 
 27 28 29 30          WK48 
 
       1989/12(Dic)        
 Lu Ma Me Gi Ve Sa Do      
              1  2  3 WK48 
  4  5  6  7  8  9 10 WK49 
 11 12 13 14 15 16 17 WK50 
 18 19 20 21 22 23 24 WK51 
 25 26 27 28 29 30 31 WK52 

        <<-- | -->> 
END
  
  echom assert_equal(expected_value, getline(1, '$')) 
  exe "norm q"
  call assert_equal(1, winnr('$'))

  " multiple months per calendar
  let g:calendar_number_of_months = 5
  Calendar 1023, 9
  call WaitForAssert({-> assert_equal(2, winnr('$'))})

  let expected_value =<< END
        <<-- | -->> 

        1023/8(Ago)        
 Lu Ma Me Gi Ve Sa Do      
     1  2  3  4  5  6 WK31 
  7  8  9 10 11 12 13 WK32 
 14 15 16 17 18 19 20 WK33 
 21 22 23 24 25 26 27 WK34 
 28 29 30 31          WK35 
 
        1023/9(Set)        
 Lu Ma Me Gi Ve Sa Do      
              1  2  3 WK35 
  4  5  6  7  8  9 10 WK36 
 11 12 13 14 15 16 17 WK37 
 18 19 20 21 22 23 24 WK38 
 25 26 27 28 29 30    WK39 
 
       1023/10(Ott)        
 Lu Ma Me Gi Ve Sa Do      
                    1 WK39 
  2  3  4  5  6  7  8 WK40 
  9 10 11 12 13 14 15 WK41 
 16 17 18 19 20 21 22 WK42 
 23 24 25 26 27 28 29 WK43 
 30 31                WK44 
 
       1023/11(Nov)        
 Lu Ma Me Gi Ve Sa Do      
        1  2  3  4  5 WK44 
  6  7  8  9 10 11 12 WK45 
 13 14 15 16 17 18 19 WK46 
 20 21 22 23 24 25 26 WK47 
 27 28 29 30          WK48 
 
       1023/12(Dic)        
 Lu Ma Me Gi Ve Sa Do      
              1  2  3 WK48 
  4  5  6  7  8  9 10 WK49 
 11 12 13 14 15 16 17 WK50 
 18 19 20 21 22 23 24 WK51 
 25 26 27 28 29 30 31 WK52 

        <<-- | -->> 
END

  echom assert_equal(expected_value, getline(1, '$')) 
  exe "norm q"
  call assert_equal(1, winnr('$'))

  " eras
  let g:calendar_erafmt = 'Heisei,-1988'

  Calendar 2000, 1
  call WaitForAssert({-> assert_equal(2, winnr('$'))})
  
  let expected_value =<< END
        <<-- | -->> 

       1999/12(Dic)        
 Lu Ma Me Gi Ve Sa Do      
        1  2  3  4  5 WK48 
  6  7  8  9 10 11 12 WK49 
 13 14 15 16 17 18 19 WK50 
 20 21 22 23 24 25 26 WK51 
 27 28 29 30 31       WK52 
 
        2000/1(Gen)        
 Lu Ma Me Gi Ve Sa Do      
                 1  2 WK52 
  3  4  5  6  7  8  9 WK 1 
 10 11 12 13 14 15 16 WK 2 
 17 18 19 20 21 22 23 WK 3 
 24 25 26 27 28 29 30 WK 4 
 31                   WK5  
 
        2000/2(Feb)        
 Lu Ma Me Gi Ve Sa Do      
     1  2  3  4  5  6 WK 5 
  7  8  9 10 11 12 13 WK 6 
 14 15 16 17 18 19 20 WK 7 
 21 22 23 24 25 26 27 WK 8 
 28 29                WK9  
 
        2000/3(Mar)        
 Lu Ma Me Gi Ve Sa Do      
        1  2  3  4  5 WK 9 
  6  7  8  9 10 11 12 WK10 
 13 14 15 16 17 18 19 WK11 
 20 21 22 23 24 25 26 WK12 
 27 28 29 30 31       WK13 
 
        2000/4(Apr)        
 Lu Ma Me Gi Ve Sa Do      
                 1  2 WK13 
  3  4  5  6  7  8  9 WK14 
 10 11 12 13 14 15 16 WK15 
 17 18 19 20 21 22 23 WK16 
 24 25 26 27 28 29 30 WK17 

        <<-- | -->> 
END

  echom assert_equal(expected_value, getline(1, '$'))
  exe "norm q"
  call assert_equal(1, winnr('$'))

  " Teardown optional variables and reset of some variables
  let g:calendar_number_of_months = 3
  let g:calendar_navi_label = 'Prev, Today, Next'
  unlet g:calendar_monday
  unlet g:calendar_mruler
  unlet g:calendar_wruler
  unlet g:calendar_weeknm
  unlet g:calendar_erafmt

  " %bw!
endfunc

func Test_syntax_highlight()

  Calendar 2020, 3
  call WaitForAssert({-> assert_equal(2, winnr('$'))})

  "2020/2(Feb)
  let linenr = 3
  let date_idx = getline(linenr)->match('\S') + 1
  let hi_group = synIDattr(synID(linenr, date_idx, 1), 'name')
  call assert_match(hi_group, 'CalHeader')

  " Su Mo Tu ...
  let linenr = 4
  let week_day = getline(linenr)->match('\S') + 1
  let hi_group = synIDattr(synID(linenr, week_day, 1), 'name')
  call assert_match(hi_group, 'CalRuler')

  " 8th line is this:
  " 16 17 18 19 20 21 22 
  let linenr = 8
  " CalSunday
  let first_day_idx = getline(linenr)->match('\S') + 1
  let hi_group = synIDattr(synID(linenr, first_day_idx, 1), 'name')
  call assert_match(hi_group, 'CalSunday')

  " Check in the middle, column 5
  " No hlgroup
  let middle_idx = 5
  let hi_group = synIDattr(synID(linenr, middle_idx, 1), 'name')
  call assert_true(empty(hi_group))

  " Check end
  " CalSaturday
  let last_day_idx = len(getline(linenr)) - 1 
  let hi_group = synIDattr(synID(linenr, last_day_idx, 1), 'name')
  call assert_match(hi_group, 'CalSaturday')

  exe "norm q"
  call assert_equal(1, winnr('$'))

  " Test CalendarH
  CalendarH 2020, 3

  "2020/2(Feb)
  let linenr = 2
  let date_idx = getline(linenr)->match('\S') + 1
  let hi_group = synIDattr(synID(linenr, date_idx, 1), 'name')
  call assert_match(hi_group, 'CalHeader')

  " Su Mo Tu ...
  let linenr = 3
  let week_day = getline(linenr)->match('\S') + 1
  let hi_group = synIDattr(synID(linenr, week_day, 1), 'name')
  call assert_match(hi_group, 'CalRuler')

  " February
  let linenr = 8
  let first_day_idx = getline(linenr)->match('\S') + 1
  let hi_group = synIDattr(synID(linenr, first_day_idx, 1), 'name')
  call assert_match(hi_group, 'CalSunday')

  let middle_idx = 13
  let hi_group = synIDattr(synID(linenr, middle_idx, 1), 'name')
  call assert_true(empty(hi_group))

  let last_day_idx = 22
  let hi_group = synIDattr(synID(linenr, last_day_idx, 1), 'name')
  call assert_match(hi_group, 'CalSaturday')

  " March
  let first_day_idx = 26
  let hi_group = synIDattr(synID(linenr, first_day_idx, 1), 'name')
  call assert_match(hi_group, 'CalSunday')

  let middle_idx = 30
  let hi_group = synIDattr(synID(linenr, middle_idx, 1), 'name')
  call assert_true(empty(hi_group))

  let last_day_idx = 45
  let hi_group = synIDattr(synID(linenr, last_day_idx, 1), 'name')
  call assert_match(hi_group, 'CalSaturday')

  " April
  let first_day_idx = 50
  let hi_group = synIDattr(synID(linenr, first_day_idx, 1), 'name')
  call assert_match(hi_group, 'CalSunday')

  let middle_idx = 56
  let hi_group = synIDattr(synID(linenr, middle_idx, 1), 'name')
  call assert_true(empty(hi_group))

  let last_day_idx = 68
  let hi_group = synIDattr(synID(linenr, last_day_idx, 1), 'name')
  call assert_match(hi_group, 'CalSaturday')

  exe "norm q"
  call assert_equal(1, winnr('$'))

  %bw!
endfunc

func Test_hooks()

  function g:MyCalBegin()
    call setreg('a', "Calendar started")
  endfunction
  let g:calendar_begin = 'g:MyCalBegin'
  
  function g:MyCalToday()
    call setreg('b', "Calendar today")
  endfunction
  let g:calendar_today = 'g:MyCalToday'

  function g:MyCalEnd()
    call setreg('c', "Calendar ended")
  endfunction
  let g:calendar_end = 'g:MyCalEnd'

  Calendar 1624, 12
  call WaitForAssert({-> assert_equal(2, winnr('$'))})
  let actual_value = getreg('a')
  call assert_match("Calendar started", actual_value)

  " Select "Today" in the navigation panel on top
  exe "norm ggfT\<cr>"
  let actual_value = getreg('b')
  call assert_match("Calendar today", actual_value)

  exe "norm q"
  let actual_value = getreg('c')
  call assert_match("Calendar ended", actual_value)

  unlet g:calendar_begin
  unlet g:calendar_today
  unlet g:calendar_end

  %bw!
endfunc
