" Vim :import command
" VIM_TEST_SETUP hi link vimImportName Todo


import "foo.vim"
impor v:true ? "foo.vim" : "bar.vim"

import v:true ?
      \ "foo.vim" :
      \ "bar.vim"

import v:true
      \ ? "foo.vim"
      \ : "bar.vim"

import v:true ?
      "\ comment 
      \ "foo.vim" :
      "\ comment 
      \ "bar.vim"

import v:true
      "\ comment 
      \ ? "foo.vim"
      "\ comment
      \ : "bar.vim"

import "foo.vim" as bar
import v:true ? "foo.vim" : "bar.vim" as baz

import v:true ?
      \ "foo.vim" :
      \ "bar.vim"
      \ as baz

import v:true
      \ ? "foo.vim"
      \ : "bar.vim"
      \ as baz

import v:true ?
      "\ comment 
      \ "foo.vim" :
      "\ comment 
      \ "bar.vim"
      "\ comment
      \ as baz

import v:true
      "\ comment 
      \ ? "foo.vim"
      "\ comment
      \ : "bar.vim"
      "\ comment
      \ as baz

echo "Foo" | import "foo.vim"


" autoload

import autoload "foo.vim"
import autoload v:true ? "foo.vim" : "bar.vim"

import autoload v:true ?
      \ "foo.vim" :
      \ "bar.vim"

import autoload v:true
      \ ? "foo.vim"
      \ : "bar.vim"

import autoload v:true ?
      "\ comment 
      \"foo.vim" :
      "\ comment
      \ "bar.vim"

import autoload v:true
      "\ comment 
      \ ? "foo.vim"
      "\ comment
      \ : "bar.vim"

import autoload "foo.vim" as bar
import autoload v:true ? "foo.vim" : "bar.vim" as baz

import autoload v:true ?
      \ "foo.vim" :
      \ "bar.vim"
      \ as baz

import autoload v:true
      \ ? "foo.vim"
      \ : "bar.vim"
      \ as baz

import autoload v:true ?
      "\ comment 
      \ "foo.vim" :
      "\ comment 
      \ "bar.vim"
      "\ comment
      \ as baz

import autoload v:true
      "\ comment 
      \ ? "foo.vim"
      "\ comment
      \ : "bar.vim"
      "\ comment
      \ as baz


" "as" keyword in expr

let as = "modules/"
import v:true ? as .. "foo.vim" : as .. "bar.vim" as other

