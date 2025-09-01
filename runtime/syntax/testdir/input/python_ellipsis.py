# Ellipsis Literal
# https://docs.python.org/3/library/constants.html#Ellipsis

# Placeholders
...
	...
x = ...
y = ... # Comment
class C: ...
lambda: ...

# Types
numbers: Tuple[int, ...]

# Doctests
"""A doctest

>>> class A:
...     def __init__(self):
...		...
... class B: ...
... x = ...
"""

class C:
	"""
	>>> class C:
	...	def __init__(self):
	...		...
	"""
