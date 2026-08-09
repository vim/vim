vim9script
# Issue #20270: dictionary keys must not look like scope dictionaries.

var scope_keys = {
  # A comment may appear before the first key.
  a: 1,
  b: -2,
  g: 'three',
  l: [4],
  s: {nested: 5},
  t: true,
  v: null,
  w: () => 8,
}

# Scope dictionaries used as values must keep their highlighting.
var scope_values = {
  buffer: b:,
  global: g:,
  tab: t:,
  vim: v:,
  window: w:,
}

# Keep key and value contexts across line breaks and comments.
var multiline = {
  a:
    # This is still part of the value.
    b:,
  nested:
    {
      # This is a literal key in a nested dictionary.
      s: 9,
    },
}

# Nested delimiters must not end the containing dictionary value.
var complex = {
  call: Func({b: 1}, [g:, {v: 2}]),
  other: {'b': 1, "g": 2, [key]: 3, non_scope: 4},
}
