#!python3

"""

Illuminates each pixel in succession.

"""

import opc
import random
import sys
import time

OPC_SERVER_ADDRESS = 'localhost:7890'
NUM_PIXELS = 4800
BACKGROUND_COLOR = (0, 0, 64)
LIT_COLOR = (255, 255, 255)
GLOW_COLOR = (64, 64, 0)

# Create a client object
client = opc.Client(OPC_SERVER_ADDRESS)

if client.can_connect():
    print('Connected to {0}.'.format(OPC_SERVER_ADDRESS))
else:
    sys.stderr.write('Could not connect to {0}.'.format(OPC_SERVER_ADDRESS))
    sys.exit(1)

pixels = [BACKGROUND_COLOR] * NUM_PIXELS
client.put_pixels(pixels, channel=0)
time.sleep(1)

while True:
    for i in range(0, NUM_PIXELS):
        if i > 0:
            pixels[i - 1] = GLOW_COLOR
        pixels[i] = LIT_COLOR
        client.put_pixels(pixels, channel=0)
        # time.sleep(0.001)

    time.sleep(3)
    pixels = [BACKGROUND_COLOR] * NUM_PIXELS
