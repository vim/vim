# String and Bytes literals
# https://docs.python.org/3/reference/lexical_analysis.html#string-and-bytes-literals

# Strings
test = 'String with escapes \' and \" and \t'
test = "String with escapes \040 and \xFF"
test = 'String with escapes \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = "String with escaped \\ backslash and ignored \
newline"
test = '''String with quotes ' and "
and escapes \t and \040 and \xFF
and escapes \u00A1 and \U00010605'''
test = """String with quotes ' and "
and escapes \t and \040 and \xFF
and escapes \u00A1 and \U00010605"""

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

# Unicode literals: Prefix is allowed but ignored (https://peps.python.org/pep-0414)
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

# Raw Unicode literals are not allowed
test = ur'Invalid string with \' and \" and \t'
test = uR"Invalid string with \040 and \xFF"
test = Ur'Invalid string with \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = UR"Invalid string with \\ backslashes and literal \
newline"
test = ru'Invalid string with \' and \" and \t'
test = rU"Invalid string with \040 and \xFF"
test = Ru'Invalid string with \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = RU"Invalid string with \\ backslashes and literal \
newline"
test = ur'''Invalid string with ' and "
and \t and \040 and \xFF
and \u00A1 and \U00010605'''
test = RU"""Invalid string with ' and "
and \t and \040 and \xFF
and \u00A1 and \U00010605"""

# Formatted string literals (f-strings)
# https://docs.python.org/3/reference/lexical_analysis.html#f-strings
test = f'F-string with escapes \' and \" and \t and fields {foo} and {bar}'
test = F"F-string with escapes \040 and \xFF and fields {foo} and {bar}"
test = f'F-string with escapes \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK} and fields {foo} and {bar}'
test = F"F-string with literal {{field}} and fields {foo} and {bar}"
test = f'''F-string with quotes ' and "
and escapes \t and \040 and \xFF
and escapes \u00A1 and \U00010605
and fields {1}, {2} and {1
    +
    2}'''
test = F"""F-string with quotes ' and "
and escapes \t and \040 and \xFF
and escapes \u00A1 and \U00010605
and fields {1}, {2} and {1
    +
    2}"""

# Raw formatted string literals
test = fr'Raw f-string with literal \' and \" and \t and fields {foo} and {bar}'
test = fR"Raw f-string with literal \040 and \xFF and fields {foo} and {bar}"
test = Fr'Raw f-string with literal \u00A1 and \U00010605 and fields \N{FIELD, NOT, ESCAPE} and {foo} and {bar}'
test = FR"Raw f-string with literal {{field}} and fields {foo} and {bar}"
test = rf'Raw f-string with literal \' and \" and \t and fields {foo} and {bar}'
test = rF"Raw f-string with literal \040 and \xFF and fields {foo} and {bar}"
test = Rf'Raw f-string with literal \u00A1 and \U00010605 and fields \N{FIELD, NOT, ESCAPE} and {foo} and {bar}'
test = RF"Raw f-string with literal {{field}} and fields {foo} and {bar}"
test = fr'''Raw f-string with quotes ' and "
and literal \t and \040 and \xFF
and literal \u00A1 and \U00010605
and fields {1}, {2} and {1
    +
    2}'''
test = RF"""Raw f-string with quotes ' and "
and literal \t and \040 and \xFF
and literal \u00A1 and \U00010605
and fields {1}, {2} and {1
    +
    2}"""

# F-string replacement fields
test = f"String is {
    "one plus "
    "two plus "
    "three"}"
test = f"Number is {
    1 +
    2 +
    3}"
test = f"Float is {1.23}"
test = f"abc{a # This is a comment }
    + 1}"
test = f"def{a # So is this :
    + 2}"
test = f"ghi{a # And this "
    + 3}"
test = f"He said his name is {name!r}."
test = f"He said his name is {repr(name)}."
test = f"result: {value:{width}}"
test = f"result: {value:{width}.{precision}}"
test = f"result: {value:{
        width
    }.{
        precision
    }}"
test = f"result: {value:{width:d}.{precision!s}}"
test = f"result: {value:{options}{width}{grouping}{precision}{type}}"
test = f"{number:#0x}"
test = f"{number:+#0x}"
test = f"{number:<+#0x}"
test = f"{number: <+#0x}"
test = f"{number:<#0x}"
test = f"{number: <#0x}"
test = f"{string=}"
test = f"{string=!r}"
test = f"{string=:20}"
test = f"{string=!r:20}"
test = f"{ string = }"
test = f"{ string = !r}"
test = f"{ string = !r }"
test = f"{ string = :20}"
test = f"{ string = !r:20}"
test = f"{ string = !r :20}"
test = f"abc {a["x"]} def"
test = f"List contains:\n{"\n".join(a)}"
test = f"Today's date is {datetime.now()}"
test = f"Today's formatted date is {datetime.now():%Y-%m-%d %H:%M:%S}"
test = f"Date is {datetime.datetime(2010, 7, 4, 12, 15, 58)}"
test = f"Formatted date is {datetime.datetime(2010, 7, 4, 12, 15, 58):%Y-%m-%d %H:%M:%S}"
test = f"Lambda returns {(lambda x: x**2)}"
test = f"Zero padded lambda returns {(lambda x: x**2):09}"
test = f"Space padded lambda returns {(lambda x: x**2):{width}}"
test = f"List copy is {items[:]}"
test = f"List slice is {items[1:]}"
test = f"List slice is {items[:9]}"
test = f"List elements are {items[:2:]}"
test = f"Padded list copy is {items[:]:99}"
test = f"Left-aligned list slice is {items[1:]:<99}"
test = f"Right aligned list slice is {items[:9]:>99}"
test = f"Center-aligned list elements are {items[:2:]:^99}"
test = f"Expression is {x == 1}"
test = f"Expression is {x != 1}"
test = f"Expression is {(x := 1)}"
test = f"Debug expression is {x == 1=}"
test = f"Debug expression is {x != 1=}"
test = f"Debug expression is {(x := 1)=}"
test = f"List comprehension returns { [x**2 for x in range(10)] }"
test = f"List comprehension returns { [
        x**2 for x in range(10)
    ] }"
test = f"Padded list comprehension returns { [x**2 for x in range(10)] :99}"
test = f"Dict comprehension returns { {x: x**2 for x in range(10)} }"
test = f"Dict comprehension returns { {
        x: x**2 for x in range(10)
    } }"
test = f"Padded dict comprehension returns { {x: x**2 for x in range(10)} :99}"

# Bytes
test = b'Bytes with escapes \' and \" and \t'
test = B"Bytes with escapes \040 and \xFF"
test = b'Bytes with SyntaxWarning \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = B"Bytes with escaped \\ backslash and ignored \
newline"
test = b'''Bytes with quotes ' and "
and escapes \t and \040 and \xFF
and SyntaxWarning \u00A1 and \U00010605'''
test = B"""Bytes with quotes ' and "
and escapes \t and \040 and \xFF
and SyntaxWarning \u00A1 and \U00010605"""

# Raw bytes
test = br'Raw bytes with literal \' and \" and \t'
test = bR"Raw bytes with literal \040 and \xFF"
test = Br'Raw bytes with literal \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = BR"Raw bytes with literal \\ backslashes and literal \
newline"
test = rb'Raw bytes with literal \' and \" and \t'
test = rB"Raw bytes with literal \040 and \xFF"
test = Rb'Raw bytes with literal \u00A1 and \U00010605 and \N{INVERTED EXCLAMATION MARK}'
test = RB"Raw bytes with literal \\ backslashes and literal \
newline"
test = br'''Raw bytes with quotes ' and "
and literal \t and \040 and \xFF
and literal \u00A1 and \U00010605'''
test = RB"""Raw bytes with quotes ' and "
and literal \t and \040 and \xFF
and literal \u00A1 and \U00010605"""
