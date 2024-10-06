vim9script
# Vim9 special methods new*(), empty(), len(), string()

def new()
enddef

def newOther()
enddef

def newyetanother()
enddef

def empty(): bool
  return true
enddef

def len(): number
  return 0
enddef

def string(): string
  return ""
enddef

class A
  def new()
    def newNested()
    enddef
    def empty(): bool
      return true
    enddef
    def len(): number
      return 0
    enddef
    def string(): string
      return ""
    enddef
  enddef

  def newOther()
    def newNested()
    enddef
    def empty(): bool
      return true
    enddef
    def len(): number
      return 0
    enddef
    def string(): string
      return ""
    enddef
  enddef

  def newyetanother()
    def newNested()
    enddef
    def empty(): bool
      return true
    enddef
    def len(): number
      return 0
    enddef
    def string(): string
      return ""
    enddef
  enddef

  def empty(): bool
    def newNested()
    enddef
    def empty(): bool
      return true
    enddef
    def len(): number
      return 0
    enddef
    def string(): string
      return ""
    enddef
    return true
  enddef

  def len(): number
    def newNested()
    enddef
    def empty(): bool
	return true
    enddef
    def len(): number
	return 0
    enddef
    def string(): string
	return ""
    enddef
    return 0
  enddef

  def string(): string
    def newNested()
    enddef
    def empty(): bool
	return true
    enddef
    def len(): number
	return 0
    enddef
    def string(): string
	return ""
    enddef
    return ""
  enddef
endclass

