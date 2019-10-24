#! /usr/bin/python

import math
import sys


def main(argv):

    totalLength = 0;
    totalLength2 = 0;

    for n in range(1, 32):
        
        if n <= 13:
            l = round(1 + (13 - n) * 0.3333, 2)
        elif n <= 18:
            l = 0;
        else:
            l = round(1 + (n - 19) * 0.3333, 2)

        if n <= 3 or n >= 29:
            l2 = 5
        elif n <= 6 or n >= 26:
            l2 = 4
        elif n <= 13 or n >= 19:
	        l2 = 3
        else:
            l2 = 0

        totalLength += l
        totalLength2 += l2

        print('n={0}  l={1}  l2={2}'.format(n, l, l2))

    print('total l = {0}, total l2 = {1}'.format(totalLength, totalLength2))



if __name__ == "__main__":
    sys.exit(main(sys.argv))

