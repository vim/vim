vim9script
# Vim9 :class command
# VIM_TEST_SETUP let g:vimsyn_folding = 'cf'
# VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax

interface Interface1
endinterface
interface Interface2
endinterface

class Class1
endclass

export class Class2
endclass

abstract class Class3
endclass

export abstract class Class4
endclass

class Class5 extends Class1
endclass

export class Class6 extends Class1
endclass

class Class7 implements Interface1, Interface2
endclass

export class Class8 implements Interface1, Interface2
endclass

class Class9
  def new()
  enddef
  def Method1(): void
    def Nested1(): void
      def Nested2(): void
      enddef
    enddef
  enddef
  def _Method2(): void
  enddef
  static def Method3(): void
  enddef
endclass

abstract class Class10
  abstract def Method1(): void
  abstract def string(): string
endclass


# Issue: #14393

interface Testable
    def SetUp()
    def TearDown()
endinterface

abstract class TestTemplate implements Testable
    var failed: number
    var passed: number

    abstract def SetUp()
    abstract def TearDown()
endclass

