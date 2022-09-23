" Vim syntax file
" Language: HLS Playlist
" Maintainer: Benoît Ryder <benoit@ryder.fr>
" Latest Revision: 2022-09-23

if exists("b:current_syntax")
  finish
endif

" Comment line
syn match  m3u8Comment  "^#\(EXT\)\@!.*$"
" Segment URL
syn match  m3u8Url      "^[^#].*$"

" Unknown tags, assume an attribute list or nothing
syn match  m3u8TagUnknown    "^#EXT[^:]*$"
syn region m3u8TagLine matchgroup=m3u8TagUnknown    start="^#EXT[^:]*\ze:"  end="$" keepend contains=m3u8AttributeList

" Basic Tags
syn match  m3u8TagHeader     "^#EXTM3U$"
syn region m3u8TagLine matchgroup=m3u8TagHeader     start="^#EXT-X-VERSION\ze:"  end="$" keepend contains=m3u8ValueInt

" Media or Multivariant Playlist Tags
syn match  m3u8TagHeader     "^#EXT-X-INDEPENDENT-SEGMENTS$"
syn region m3u8TagLine matchgroup=m3u8TagDelimiter  start="^#EXT-X-START\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-DEFINE\ze:"  end="$" keepend contains=m3u8AttributeList

" Media Playlist Tags
syn region m3u8TagLine matchgroup=m3u8TagHeader     start="^#EXT-X-TARGETDURATION\ze:"  end="$" keepend contains=m3u8ValueFloat
syn region m3u8TagLine matchgroup=m3u8TagHeader     start="^#EXT-X-MEDIA-SEQUENCE\ze:"  end="$" keepend contains=m3u8ValueInt
syn region m3u8TagLine matchgroup=m3u8TagHeader     start="^#EXT-X-DISCONTINUITY-SEQUENCE\ze:"  end="$" keepend contains=m3u8ValueInt
syn match  m3u8TagDelimiter  "^#EXT-X-ENDLIST$"
syn region m3u8TagLine matchgroup=m3u8TagHeader     start="^#EXT-X-PLAYLIST-TYPE\ze:"  end="$" keepend contains=m3u8AttributeEnum
syn match  m3u8TagStandard   "^#EXT-X-I-FRAME-ONLY$"
syn region m3u8TagLine matchgroup=m3u8TagHeader     start="^#EXT-X-PART-INF\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagHeader     start="^#EXT-X-SERVER-CONTROL\ze:"  end="$" keepend contains=m3u8AttributeList

" Media Segment Tags
syn region m3u8TagLine matchgroup=m3u8TagStatement  start="^#EXTINF\ze:"  end="$" keepend contains=m3u8ValueFloat,m3u8ExtInfDesc
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-BYTERANGE\ze:"  end="$" keepend contains=m3u8ValueInt
syn match  m3u8TagDelimiter  "^#EXT-X-DISCONTINUITY$"
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-KEY\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-MAP\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-PROGRAM-DATE-TIME\ze:"  end="$" keepend contains=m3u8ValueDateTime
syn match  m3u8TagDelimiter  "^#EXT-X-GAP$"
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-BITRATE\ze:"  end="$" keepend contains=m3u8ValueFloat
syn region m3u8TagLine matchgroup=m3u8TagStatement  start="^#EXT-X-PART\ze:"  end="$" keepend contains=m3u8AttributeList

" Media Metadata Tags
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-DATERANGE\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-SKIP\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStatement  start="^#EXT-X-PRELOAD-HINT\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStatement  start="^#EXT-X-RENDITION-REPORT\ze:"  end="$" keepend contains=m3u8AttributeList

" Multivariant Playlist Tags
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-MEDIA\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStatement  start="^#EXT-X-STREAM-INF\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStatement  start="^#EXT-X-I-FRAME-STREAM-INF\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-SESSION-DATA\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-SESSION-KEY\ze:"  end="$" keepend contains=m3u8AttributeList
syn region m3u8TagLine matchgroup=m3u8TagStandard   start="^#EXT-X-CONTENT-STEERING\ze:"  end="$" keepend contains=m3u8AttributeList

" Attributes
syn region m3u8AttributeList  start=":" end="$" keepend contained
  \ contains=m3u8AttributeName,m3u8AttributeInt,m3u8AttributeHex,m3u8AttributeFloat,m3u8AttributeString,m3u8AttributeEnum,m3u8AttributeResolution,m3u8AttributeUri
" Common attributes
syn match  m3u8AttributeName        "[A-Za-z-]\+\ze=" contained
syn match  m3u8AttributeEnum        "=\zs[A-Za-z][A-Za-z0-9-_]*" contained
syn match  m3u8AttributeString      +=\zs"[^"]*"+ contained
syn match  m3u8AttributeInt         "=\zs\d\+" contained
syn match  m3u8AttributeFloat       "=\zs-\?\d*\.\d*" contained
syn match  m3u8AttributeHex         "=\zs0[xX]\d*" contained
syn match  m3u8AttributeResolution  "=\zs\d\+x\d\+" contained
" Allow different highligting for URI attributes
syn region m3u8AttributeUri matchgroup=m3u8AttributeName    start="\zsURI\ze" end="\(,\|$\)" contained contains=m3u8UriQuotes
syn region m3u8UriQuotes    matchgroup=m3u8AttributeString  start=+"+ end=+"+ keepend contained contains=m3u8UriValue
syn match  m3u8UriValue             /[^" ]\+/ contained
" Individual values
syn match  m3u8ValueInt             "[0-9]\+" contained
syn match  m3u8ValueFloat           "\(\d\+\|\d*\.\d*\)" contained
syn match  m3u8ExtInfDesc           ",\zs.*$" contained
syn match  m3u8ValueDateTime        "\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d\(\.\d*\)\?\(Z\|\d\d:\?\d\d\)$" contained


" Define default highlighting

hi def link m3u8Comment  Comment
hi def link m3u8Url      NONE

hi def link m3u8TagHeader     Special
hi def link m3u8TagStandard   Define
hi def link m3u8TagDelimiter  Delimiter
hi def link m3u8TagStatement  Statement
hi def link m3u8TagUnknown    Special

hi def link m3u8UriQuotes            String
hi def link m3u8UriValue             Underlined
hi def link m3u8AttributeQuotes      String
hi def link m3u8AttributeName        Identifier
hi def link m3u8AttributeInt         Number
hi def link m3u8AttributeHex         Number
hi def link m3u8AttributeFloat       Float
hi def link m3u8AttributeString      String
hi def link m3u8AttributeEnum        Constant
hi def link m3u8AttributeResolution  Constant
hi def link m3u8ValueInt             Number
hi def link m3u8ValueFloat           Float
hi def link m3u8ExtInfDesc           String
hi def link m3u8ValueDateTime        Constant


let b:current_syntax = "m3u8"

" vim: sts=2 sw=2 et
