#!/usr/bin/env python

"""

Illuminates each pixel in succession.

"""

import opc
import random
import sys
import time

OPC_SERVER_ADDRESS = 'localhost:7890'
NUM_PIXELS = 4058

# Create a client object
client = opc.Client(OPC_SERVER_ADDRESS)

if client.can_connect():
    print('Connected to {0}.'.format(OPC_SERVER_ADDRESS))
else:
    sys.stderr.write('Could not connect to {0}.'.format(OPC_SERVER_ADDRESS))
    sys.exit(1)

pixels = [(0, 0, 48)] * NUM_PIXELS

while True:
    for i in range(0, NUM_PIXELS):
        pixels[i - 1] = (64, 64, 0)
        pixels[i] = (255, 255, 255)
        client.put_pixels(pixels, channel=0)
        time.sleep(0.001)

    pixels = [(0, 0, 32)] * NUM_PIXELS
