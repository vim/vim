# comment

a = b
$(a) = 1

$(a)::
	@echo double-colon rule

$(a) += output

define say
	echo $1
endef

default:
	$(call say,"Hello (world)!")
