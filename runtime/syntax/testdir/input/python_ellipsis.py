# Ellipsis Literal
# https://docs.python.org/3/library/constants.html#Ellipsis

# Placeholders
...
	...
x = ...
y = ... # Comment
class C: ...
lambda: ...

# Annotations
numbers: Tuple[int, ...]

# Doctests
"""A doctest

>>> class A:
...     def __init__(self):
...		...
>>> class B: ...
>>> x = ...
>>> raise ValueError('multi\n    line\ndetail')
Traceback (most recent call last):
    ...
ValueError: multi
    line
detail
>>> print(list(range(20)))  # doctest: +ELLIPSIS
[0, 1, ..., 18, 19]
>>> exec(s)  #doctest: +ELLIPSIS
-3.21716034272e-0...7
"""

class C:
	"""
	>>> class C:
	...	def __init__(self):
	...		...
	"""

# Numpy
x[..., 0]

# Issue #18263 (Python highlighting ellipsis, false positive)
a = ".." # comment
