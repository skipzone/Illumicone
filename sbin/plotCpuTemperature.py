#! /usr/bin/python

'''
    ----------------------------------------------------------------------------
    This program plots the logged temperature of a Raspberry Pi 4 CPU

    Ross Butler  December 2019
    ----------------------------------------------------------------------------

    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
'''


from datetime import datetime, timedelta
import os
import re
import matplotlib
import matplotlib.pyplot as plt
from struct import *
import sys
#from time import sleep


lastTimestamp = None


def readLogFile(logFileName):

    # 2019-12-08T13:21:01-07:00 47.0 116.6
    temperatureLinePattern = r'(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})-\d{2}:\d{2} (-?\d+\.?\d*) (-?\d+\.?\d*)$'

    x = []
    y = []

    try:
        with open(logFileName, 'r') as f:
            for line in f:
                line = line.rstrip()

                m = re.search(temperatureLinePattern, line)
                if m is not None:
                    timestamp = datetime.strptime(m.group(1), '%Y-%m-%dT%H:%M:%S')
                    tempF = float(m.group(3))
                    x.append(timestamp)
                    y.append(tempF)
                    next

                # If we're here then the line was unrecognized, so ignore it.

    except IOError as e:
        sys.stderr.write('An error occurred with log file {0}.  {1}\n'.format(logFileName, e))

    return x, y


def doPlot(x, y):

    plt.plot(x, y)
    plt.gcf().autofmt_xdate()
    myFmt = matplotlib.dates.DateFormatter('%d %b %H:%M')
    plt.gca().xaxis.set_major_formatter(myFmt)
    plt.title(r'Raspberry Pi 4 CPU Temperature')
    plt.xlabel(r'Date & Time');
    plt.ylabel(r'CPU Temperature ($\degree$F)')

    plt.show()


def usage():
    print('Usage:  plotCpuTemperature.py log_file_name')
    return


def main(argv):

    if len(argv) != 2:
        usage()
        return 2

    inputFileName = argv[1]
    if not os.path.exists(inputFileName):
        sys.stderr.write('File {0} does not exist.\n'.format(inputFileName))
        return 1

    x, y = readLogFile(inputFileName)
    doPlot(x, y)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

