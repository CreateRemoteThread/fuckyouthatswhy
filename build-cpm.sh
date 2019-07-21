#!/bin/sh

z80asm -o bios2.com bios2.z80
cd tools
rm out.array
python stitch.py test.json
python bin2array.py out.bin > out.array

