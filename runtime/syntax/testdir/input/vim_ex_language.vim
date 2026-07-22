" Vim :language command
" VIM_TEST_SETUP hi link vimLanguageName Todo


" print commands

language
language collate
language ctype
language time
language messages


" set commands

language          C
language collate  C
language ctype    C
language time     C
language messages C

language          POSIX
language collate  POSIX
language ctype    POSIX
language time     POSIX
language messages POSIX

language          de_DE.UTF-8@euro
language collate  de_DE.UTF-8@euro
language ctype    de_DE.UTF-8@euro
language time     de_DE.UTF-8@euro
language messages de_DE.UTF-8@euro

" tail comment and trailing bar

language          " comment
language collate  " comment
language ctype    " comment
language time     " comment
language messages " comment

language          de_DE.UTF-8@euro " comment
language collate  de_DE.UTF-8@euro " comment
language ctype    de_DE.UTF-8@euro " comment
language time     de_DE.UTF-8@euro " comment
language messages de_DE.UTF-8@euro " comment

language          C                " comment
language collate  C                " comment
language ctype    C                " comment
language time     C                " comment
language messages C                " comment

language          POSIX            " comment
language collate  POSIX            " comment
language ctype    POSIX            " comment
language time     POSIX            " comment
language messages POSIX            " comment

language          de_DE.UTF-8@euro " comment
language collate  de_DE.UTF-8@euro " comment
language ctype    de_DE.UTF-8@euro " comment
language time     de_DE.UTF-8@euro " comment
language messages de_DE.UTF-8@euro " comment

language          | echo "..."
language collate  | echo "..."
language ctype    | echo "..."
language time     | echo "..."
language messages | echo "..."

language          C                | echo "..."
language collate  C                | echo "..."
language ctype    C                | echo "..."
language time     C                | echo "..."
language messages C                | echo "..."

language          POSIX            | echo "..."
language collate  POSIX            | echo "..."
language ctype    POSIX            | echo "..."
language time     POSIX            | echo "..."
language messages POSIX            | echo "..."

language          de_DE.UTF-8@euro | echo "..."
language collate  de_DE.UTF-8@euro | echo "..."
language ctype    de_DE.UTF-8@euro | echo "..."
language time     de_DE.UTF-8@euro | echo "..."
language messages de_DE.UTF-8@euro | echo "..."

def Vim9Context()

  # print commands

  language
  language collate
  language ctype
  language time
  language messages


  # set commands

  language          C
  language collate  C
  language ctype    C
  language time     C
  language messages C

  language          POSIX
  language collate  POSIX
  language ctype    POSIX
  language time     POSIX
  language messages POSIX

  language          de_DE.UTF-8@euro
  language collate  de_DE.UTF-8@euro
  language ctype    de_DE.UTF-8@euro
  language time     de_DE.UTF-8@euro
  language messages de_DE.UTF-8@euro


  # tail comment and trailing bar

  language          # comment
  language collate  # comment
  language ctype    # comment
  language time     # comment
  language messages # comment

  language          C                # comment
  language collate  C                # comment
  language ctype    C                # comment
  language time     C                # comment
  language messages C                # comment

  language          POSIX            # comment
  language collate  POSIX            # comment
  language ctype    POSIX            # comment
  language time     POSIX            # comment
  language messages POSIX            # comment

  language          de_DE.UTF-8@euro # comment
  language collate  de_DE.UTF-8@euro # comment
  language ctype    de_DE.UTF-8@euro # comment
  language time     de_DE.UTF-8@euro # comment
  language messages de_DE.UTF-8@euro # comment

  language          | echo "..."
  language collate  | echo "..."
  language ctype    | echo "..."
  language time     | echo "..."
  language messages | echo "..."

  language          C                | echo "..."
  language collate  C                | echo "..."
  language ctype    C                | echo "..."
  language time     C                | echo "..."
  language messages C                | echo "..."

  language          POSIX            | echo "..."
  language collate  POSIX            | echo "..."
  language ctype    POSIX            | echo "..."
  language time     POSIX            | echo "..."
  language messages POSIX            | echo "..."

  language          de_DE.UTF-8@euro | echo "..."
  language collate  de_DE.UTF-8@euro | echo "..."
  language ctype    de_DE.UTF-8@euro | echo "..."
  language time     de_DE.UTF-8@euro | echo "..."
  language messages de_DE.UTF-8@euro | echo "..."
enddef

