# String literals
# https://docs.python.org/2/reference/lexical_analysis.html#string-literals

# Strings: Source encoding, no Unicode escape sequences
test = 'String with escapes \' and \" and \t'
test = "String with escapes \040 and \xFF"
test = 'String with literal \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = "String with escaped \\ backslash and ignored \
newline"
test = '''String with quotes ' and "
and escapes \t and \040 and \xFF
and literal \u00A1 and \U00010605'''
test = """String with quotes ' and "
and escapes \t and \040 and \xFF
and literal \u00A1 and \U00010605"""

# Raw strings
test = r'Raw string with literal \' and \" and \t'
test = R"Raw string with literal \040 and \xFF"
test = r'Raw string with literal \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = R"Raw string with literal \\ backslashes and literal \
newline"
test = r'''Raw string with quotes ' and "
and literal \t and \040 and \xFF
and literal \u00A1 and \U00010605'''
test = R"""Raw string with quotes ' and "
and literal \t and \040 and \xFF
and literal \u00A1 and \U00010605"""

# B-strings: Prefix is allowed but ignored (https://peps.python.org/pep-3112)
test = b'String with escapes \' and \" and \t'
test = B"String with escapes \040 and \xFF"
test = b'String with literal \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = B"String with escaped \\ backslash and ignored \
newline"
test = b'''String with quotes ' and "
and escapes \t and \040 and \xFF
and literal \u00A1 and \U00010605'''
test = B"""String with quotes ' and "
and escapes \t and \040 and \xFF
and literal \u00A1 and \U00010605"""

# Raw b-strings
test = br'Raw string with literal \' and \" and \t'
test = bR"Raw string with literal \040 and \xFF"
test = Br'Raw string with literal \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = BR"Raw string with literal \\ backslashes and literal \
newline"
test = br'''Raw string with quotes ' and "
and literal \t and \040 and \xFF
and literal \u00A1 and \U00010605'''
test = BR"""Raw string with quotes ' and "
and literal \t and \040 and \xFF
and literal \u00A1 and \U00010605"""

# Unicode strings
test = u'String with escapes \' and \" and \t'
test = U"String with escapes \040 and \xFF"
test = u'String with escapes \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = U"String with escaped \\ backslash and ignored \
newline"
test = u'''String with quotes ' and "
and escapes \t and \040 and \xFF
and escapes \u00A1 and \U00010605'''
test = U"""String with quotes ' and "
and escapes \t and \040 and \xFF
and escapes \u00A1 and \U00010605"""

# Raw Unicode strings: Only Unicode escape sequences
test = ur'Raw Unicode string with literal \' and \" and \t'
test = uR"Raw Unicode string with literal \040 and \xFF"
test = Ur'Raw Unicode string with escapes \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = UR"Raw Unicode string with literal \\ backslashes and literal \
newline"
test = ur'''Raw Unicode string with quotes ' and "
and literal \t and \040 and \xFF
and escapes \u00A1 and \U00010605'''
test = UR"""Raw Unicode string with quotes ' and "
and literal \t and \040 and \xFF
and escapes \u00A1 and \U00010605"""

# vim: syntax=python2
