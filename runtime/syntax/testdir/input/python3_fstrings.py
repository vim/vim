# F-strings (formatted string literals)
# https://docs.python.org/3/reference/lexical_analysis.html#f-strings

# Strings
test = 'Single-quoted string with escapes \' and \04 and fields {one} and {two}'
test = "Double-quoted string with escapes \" and \xFF and fields {one} and {two}"
test = '''Triple-quoted string
with escapes \t and \04
and quotes ' and "
and fields {one} and {two}'''
test = """Triple-quoted string
with escapes \t and \xFF
and quotes ' and "
and fields {one} and {two}"""

# Raw strings
test = r'Single-quoted raw string with escapes \' and \04 and fields {one} and {two}'
test = R"Double-quoted raw string with escapes \" and \xFF and fields {one} and {two}"
test = r'''Triple-quoted raw string
with escapes \t and \04
and quotes ' and "
and fields {one} and {two}'''
test = R"""Triple-quoted raw string
with escapes \t and \xFF
and quotes ' and "
and fields {one} and {two}"""

# F-strings
test = f'Single-quoted f-string with escapes \' and \04 and fields {one} and {two}'
test = F"Double-quoted f-string with escapes \" and \xFF and fields {one} and {two}"
test = f'Single-quoted f-string with an {{escaped}} field and fields {one} and {two}'
test = F"Double-quoted f-string with an {{escaped}} field and fields {one} and {two}"
test = f'''Triple-quoted f-string
with escapes \t and \04
and quotes ' and "
and fields {one}, {two} and {1
    +
    2}'''
test = F"""Triple-quoted f-string
with escapes \t and \xFF
and quotes ' and "
and fields {one}, {two} and {1
    +
    2}"""

# Raw f-strings
test = rf'Single-quoted raw f-string with escapes \' and \04 and fields {one} and {two}'
test = rF"Double-quoted raw f-string with escapes \" and \xFF and fields {one} and {two}"
test = Rf'Single-quoted raw f-string with escapes \' and \04 and fields {one} and {two}'
test = RF"Double-quoted raw f-string with escapes \" and \xFF and fields {one} and {two}"
test = fr'Single-quoted raw f-string with an {{escaped}} field and fields {one} and {two}'
test = fR"Double-quoted raw f-string with an {{escaped}} field and fields {one} and {two}"
test = Fr'Single-quoted raw f-string with an {{escaped}} field and fields {one} and {two}'
test = FR"Double-quoted raw f-string with an {{escaped}} field and fields {one} and {two}"
test = rF'''Triple-quoted raw f-string
with escapes \t and \04
and quotes ' and "
and fields {one}, {two} and {1
    +
    2}'''
test = fR"""Triple-quoted raw f-string
with escapes \t and \xFF
and quotes ' and "
and fields {one}, {two} and {1
    +
    2}"""

# Comments, nested fields and nested quotes
test = f"abc{a # This is a comment }"
    + 3}"
test = f"He said his name is {name!r}."
test = f"He said his name is {repr(name)}."
test = f"result: {value:{width}}"
test = f"result: {value:{width}.{precision}}"
test = f"{today:%B %d, %Y}"
test = f"{today=:%B %d, %Y}"
test = f"{number:#0x}"
test = f"{number:+#0x}"
test = f"{number:<+#0x}"
test = f"{number: <+#0x}"
test = f"{number:<#0x}"
test = f"{number: <#0x}"
test = f"{ foo = }"
test = f"{line = }"
test = f"{line = :20}"
test = f"{line = !r:20}"
test = f"abc {a["x"]} def"
test = f"List a contains:\n{"\n".join(a)}"
