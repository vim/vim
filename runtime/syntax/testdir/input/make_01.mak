# comment

a = b
$(a) = 1

$(a)::
	@echo double-colon rule

$(a) += output
