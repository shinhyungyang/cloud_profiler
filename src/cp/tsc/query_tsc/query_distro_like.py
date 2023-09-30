#!/usr/bin/env python3

from pprint import pformat
import distro
import sys

for line in pformat(distro.os_release_info()).split('\n'):
  if "'id_like'" in line:
    s = line.split(':')
    distro = s[1].strip(' ,')
    print(distro)
    sys.exit(0) 

sys.exit(1)
