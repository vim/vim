# Imports
import json
import os.path as p
from json import dumps, loads
from json import dumps as d
from os import *
from . import submodule
from ...pkg.sub import thing

# Lazy imports (PEP 810)
lazy import json
lazy from json import dumps
lazy from . import submodule
lazy from collections import OrderedDict as OD

# `lazy` is a soft keyword that's only recognized in imports
lazy = 1
def lazy(): pass
