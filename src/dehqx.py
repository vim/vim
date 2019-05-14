# Python script to get both the data and resource fork from a BinHex encoded
# file.
# Author: MURAOKA Taro <koron.kaoriya@gmail.com>
# Last Change: 2018 Mar 27
#
# Copyright (C) 2003,12 MURAOKA Taro <koron.kaoriya@gmail.com>
# THIS FILE IS DISTRIBUTED UNDER THE VIM LICENSE.

import sys
import binhex

input = sys.argv[1]
conv = binhex.HexBin(input)
info = conv.FInfo
out = conv.FName
out_data = out
out_rsrc = out + '.rsrcfork'

# This uses the print statement on Python 2, print function on Python 3.
#print('out_rsrc=' + out_rsrc)
print('In file: ' + input)

outfile = open(out_data, 'wb')
print('  Out data fork: ' + out_data)
while 1:
    d = conv.read(128000)
    if not d: break
    outfile.write(d)
outfile.close()
conv.close_data()

d = conv.read_rsrc(128000)
if d:
    print('  Out rsrc fork: ' + out_rsrc)
    outfile = open(out_rsrc, 'wb')
    outfile.write(d)
    while 1:
        d = conv.read_rsrc(128000)
        if not d: break
        outfile.write(d)
    outfile.close()

conv.close()

# vim:set ts=8 sts=4 sw=4 et:
