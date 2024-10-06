vim9script
# Vim :interface command
# VIM_TEST_SETUP let g:vimsyn_folding = 'i'
# VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax

interface Interface1
endinterface

export interface Interface2
endinterface

interface Interface2
  # comment
  var var1: number
endinterface

interface Interface3
  # comment
  def Meth1(): number
endinterface

interface Interface4
  # comment
  var var1: number
  # comment
  def Meth1(): number
endinterface

interface Interface5
  # comment
  var var1: number
  var var2: number
  # comment
  def Meth1(): number
  def Meth2(): number
endinterface
