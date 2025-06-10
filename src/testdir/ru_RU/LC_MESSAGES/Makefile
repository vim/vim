all: __PACKAGE__.mo

__PACKAGE__.mo: __PACKAGE__.po
	# Create __PACKAGE__.mo.
	OLD_PO_FILE_INPUT=yes msgfmt -o $@ $<
