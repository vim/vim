" Vim :# and :number commands


#
# 42
# l
# p
# #

# lp#

# 42 l
# 42 p
# 42 #

# 42 lp#


#    | echo "..."
#    " comment
# 42 | echo "..."
# 42 " comment
# l  | echo "..."
# l  " comment


number
number 42
number l
number p
number #

number lp#

number 42 l
number 42 p
number 42 #

number 42 lp#

number    | echo "..."
number     " comment
number 42 | echo "..."
number 42 " comment
number l  | echo "..."
number l  " comment

def Vim9Context()
  :#
  :# 42
  :# l
  :# p
  :# #

  :# lp#

  :# 42 l
  :# 42 p
  :# 42 #

  :# 42 lp#

  :#    | echo "..."
  :#    # comment
  :# 42 | echo "..."
  :# 42 # comment
  :# l  | echo "..."
  :# l  # comment


  number
  number 42
  number l
  number p
  number #

  number lp#

  number 42 l
  number 42 p
  number 42 #

  number 42 lp#

  number    | echo "..."
  number    # comment
  number 42 | echo "..."
  number 42 # comment
  number l  | echo "..."
  number l  # comment
enddef

