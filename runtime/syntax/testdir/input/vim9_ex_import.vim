vim9script
# Vim9 :import command
# VIM_TEST_SETUP hi link vimImportName Todo


import "foo.vim"
import true ? "foo.vim" : "bar.vim"

import true ?
  "foo.vim" :
  "bar.vim"

import true
  ? "foo.vim"
  : "bar.vim"

import true ? # comment
  # comment 
  "foo.vim" :
  # comment
  "bar.vim"

import true # comment
  # comment 
  ? "foo.vim"
  # comment
  : "bar.vim"

import "foo.vim" as bar
import true ? "foo.vim" : "bar.vim" as baz

import true ?
  "foo.vim" :
  "bar.vim"
  as baz

import true
  ? "foo.vim"
  : "bar.vim"
  as baz

import true ? # comment
  # comment 
  "foo.vim" :
  # comment
  "bar.vim"
  # comment
  as baz

import true # comment
  # comment 
  ? "foo.vim"
  # comment
  : "bar.vim"
  # comment
  as baz

echo "Foo" | import "foo.vim"


# autoload

import autoload "foo.vim"
import autoload true ? "foo.vim" : "bar.vim"

import autoload true ?
  "foo.vim" :
  "bar.vim"

import autoload true
  ? "foo.vim"
  : "bar.vim"

import autoload true ? # comment
  # comment 
  "foo.vim" :
  # comment
  "bar.vim"

import autoload true # comment
  # comment 
  ? "foo.vim"
  # comment
  : "bar.vim"

import autoload "foo.vim" as bar
import autoload true ? "foo.vim" : "bar.vim" as baz

import autoload true ?
  "foo.vim" :
  "bar.vim"
  as baz

import autoload true
  ? "foo.vim"
  : "bar.vim"
  as baz

import autoload true ? # comment
  # comment 
  "foo.vim" :
  # comment
  "bar.vim"
  # comment
  as baz

import autoload true # comment
  # comment 
  ? "foo.vim"
  # comment
  : "bar.vim"
  # comment
  as baz


# "as" keyword in expr

var as = "modules/"
import true ? as .. "foo.vim" : as .. "bar.vim" as other

