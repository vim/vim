# Generator of Vim script Syntax File

This directory contains a Vim script generator, that will parse the Vim source file and
generate a vim.vim syntax file.

Files in this directory where copied from https://github.com/vim-jp/syntax-vim-ex/
and included here on Feb, 13th, 2024 for the Vim Project.

- Maintainer: Hirohito Higashi
- License: Vim License

## How to generate

    $ make

This will generate `../vim.vim`

## Files

Name                 |Description
---------------------|------------------------------------------------------
`Makefile`           |Makefile to generate ../vim.vim
`README.md`          |This file
`gen_syntax_vim.vim` |Script to generate vim.vim
`update_date.vim`    |Script to update "Last Change:"
`vim.vim.base`       |Template for vim.vim
