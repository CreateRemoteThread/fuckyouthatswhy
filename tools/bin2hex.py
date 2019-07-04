#!/usr/bin/python

import sys
import binascii

def chunks(l,n):
  for i in range(0,len(l),n):
    yield l[i:i+n]

if __name__ == "__main__":
  ROW_LEN = 16
  dataAddress = 0
  if len(sys.argv) != 2:
    print "usage: ./bin2hex.py [bin]"
    print "converts bin to ihex (no csum)"
  f = open(sys.argv[1],"rb")
  data = f.read()
  f.close()
  for l in list(chunks(data,ROW_LEN)):
    outString = ":"
    outString += "%02x" % len(l)
    outString += "%04x" % (dataAddress)
    dataAddress += ROW_LEN
    outString += "00"
    outString += binascii.hexlify(l)
    outString += "00"
    print outString
