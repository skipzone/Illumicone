#!/usr/bin/env python

from __future__ import division
import math
import optparse
import sys

#-------------------------------------------------------------------------------
# Illumicone simulator, based on code from: https://github.com/zestyping/openpixelcontrol

NUM_STRINGS = 36
PIXELS_PER_STRING = 100
PIXEL_DISTANCE = 1 / PIXELS_PER_STRING


#-------------------------------------------------------------------------------
# make the cone: assume each strand has length of 1, and that the height and radius are equal
# i.e. h = r = ~.707
# if there are 100 pixels, they are .01 apart

result = ['[']
theta = 0
for s in range(NUM_STRINGS):
    theta = 2 * math.pi * s / (NUM_STRINGS - 1)

    for p in range(PIXELS_PER_STRING):
        z = .707 - .007 * p #TODO: explain these magic numbers
        radius = .007 * p + .03
        x = math.sin(theta) * radius
        y = math.cos(theta) * radius

        result.append('  {"point": [%.4f, %.4f, %.4f]},' % (x, y, z))

# trim off last comma
result[-1] = result[-1][:-1]

result.append(']')
print '\n'.join(result)

