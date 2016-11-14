#!/usr/bin/env python

from __future__ import division
import math
import optparse
import sys

#-------------------------------------------------------------------------------
# Illumicone simulator, based on code from: https://github.com/zestyping/openpixelcontrol

NUM_STRINGS = 36
PIXELS_PER_STRING = 100
SCALE = 10 # You can also think of this as the length of the strands.
print  "scale: " + str(SCALE)
PIXEL_DISTANCE = SCALE / PIXELS_PER_STRING
print "\npixel distance: " + str(PIXEL_DISTANCE)
HEIGHT = math.sqrt(SCALE * SCALE / 2)
print "\ncone height: " + str(HEIGHT)
MIN_RADIUS = HEIGHT * .1 # i.e. the radius of the little circle on top
print "\nmin radius: " + str(MIN_RADIUS)

#-------------------------------------------------------------------------------

result = ['[']
theta = 0
for s in range(NUM_STRINGS):
    theta = 2 * math.pi * s / (NUM_STRINGS - 1)

    for p in range(PIXELS_PER_STRING):
        z = HEIGHT - PIXEL_DISTANCE * p
        radius = PIXEL_DISTANCE * p + MIN_RADIUS
        x = math.sin(theta) * radius
        y = math.cos(theta) * radius

        result.append('  {"point": [%.4f, %.4f, %.4f]},' % (x, y, z))

# trim off last comma
result[-1] = result[-1][:-1]

result.append(']')
print '\n'.join(result)

