# comment

a = b
$(a) = 1

$(a)::
	@echo double-colon rule

$(a): prerequisite
$(a) += output
define say
$(a): prerequisite
	echo $1
endef
default:
	$(call say,"Hello (world)!")

foo:
	echo "bar$$" baz
