#!/usr/bin/python

import sys
import json

data = {}

with open(sys.argv[1]) as f:
  data = json.load(f)

data["data"].sort(key=lambda tup: tup["load"])
out = ""

for i in range(0,len(data["data"])):
  d_elt = data["data"][i]
  # d_elt["fn"]
  # d_elt["load"]
  d_blk = ""
  with open(d_elt["fn"],"rb") as f:
    d_blk = f.read()
  if i == len(data["data"]) - 1:
    print "MONITOR: LAST BLOCK"
    d_size = len(d_blk)
    out += d_blk
  else:
    print "MONITOR: PADDING"
    d_size = data["data"][i+1]["load"] - d_elt["load"]
    out += d_blk + (d_size - len(d_blk)) * "\xFF"

f2 = open("out.bin","wb")
f2.write(out)
f2.close()
