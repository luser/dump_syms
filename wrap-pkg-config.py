#!/usr/bin/env python
from __future__ import print_function

import subprocess
import sys

if sys.platform == 'win32':
    result = 1
else:
    result = subprocess.call(['pkg-config'] + sys.argv[1:])
# invert exit code
result = 1 if result == 0 else 0
print("%d" % result)
