#!/usr/bin/python

import sys

f = open(sys.argv[1],"rb")
data = f.read()
f.close()

out = []
for d in data:
  out.append("0x%02x" % ord(d))

print ",".join(out)
