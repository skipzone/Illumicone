#! /usr/bin/python

'''
    ----------------------------------------------------------------------------
    This program plays back widgetRcvr data logs, recreating and sending widget
    data messages to patternController.

    Ross Butler  February 2019
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


import argparse
from datetime import datetime, timedelta
import os
import re
import socket
from struct import *
import sys
from time import sleep

defaultPatconIpAddress = '127.0.0.1'

patconIpAddress = None

# TODO:  get these from command line
widgetPortNumberBase = 4200
timeCompressionThresholdSeconds = 5

lineCount = 0
lastTimestamp = None
clientSock = None


def waitUntilTimestamp(timestamp):

    global lastTimestamp
    global timeCompressionThresholdSeconds

    try:
        thisTimestamp = datetime.strptime(timestamp, '%Y-%m-%d %H:%M:%S.%f')
        if lastTimestamp:
            interval = thisTimestamp - lastTimestamp
            intervalS = interval.days * 3600 * 24 + interval.seconds + interval.microseconds / 1000000.0
            if intervalS > 0:
                if intervalS < timeCompressionThresholdSeconds:
                    sleep(intervalS)
                else:
                    print('{0}:  Advancing to {1}.'.format(datetime.now(), timestamp))

        lastTimestamp = thisTimestamp

    except ValueError as e:
        sys.stderr.write('Invalid timestamp "{0} at line {1}.  {2}\n'.format(timestamp, lineCount, e))


def sendPv(widgetData):
    waitUntilTimestamp(widgetData['timestamp'])
    #print('Sending pv data:  {0}'.format(widgetData))
    message = pack('=BBBhh',
        widgetData['widgetId'],
        widgetData['channel'],
        widgetData['isActive'],
        widgetData['position'],
        widgetData['velocity'])
    clientSock.sendto(message, (patconIpAddress, widgetPortNumberBase + widgetData['widgetId']))


def sendMvec(widgetData):
    waitUntilTimestamp(widgetData['timestamp'])
    #print('Sending mvec data:  {0}'.format(widgetData))
    for i, measmt in enumerate(widgetData['measurements']):
        message = pack('=BBBhh',
            widgetData['widgetId'],
            i,                      # channel
            widgetData['isActive'],
            measmt,                 # position
            0)                      # velocity is always zero
        clientSock.sendto(message, (patconIpAddress, widgetPortNumberBase + widgetData['widgetId']))

def sendCustom(widgetData):
    waitUntilTimestamp(widgetData['timestamp'])
    #print('Sending custom data:  {0}'.format(widgetData))


def processLogFile(logFileName):

    global lineCount

    # 2016-09-01 23:58:31.124:
    timestampPattern = r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}):  '

    # 2016-09-01 23:58:31.124:  pv: id=10 a=0 ch=0 p=178 v=0
    # 2016-09-01 23:58:31.124:  pv: r=0 id=10 a=0 ch=0 p=178 v=0
    pvPayloadPattern = timestampPattern + r'pv: (r=(\d+) )?id=(\d+) a=(\d+) ch=(\d+) p=(-?\d+) v=(-?\d+)$'

    # 2016-09-01 23:58:31.124:  mvec: id=7 a=0 ch=0 n=3 1 2 3
    # 2019-12-15 17:31:36.743:  mvec: r=0 id=4 a=1 ch=0 n=14      18    236     65     24  -3975   -131   1497      0    255
    mvecPayloadPattern = timestampPattern + r'mvec: (r=(\d+))? id=(\d+) a=(\d+) ch=(\d+) n=(\d+)((?:\s+-?\d+)+)$'

    # 2016-09-01 23:58:31.124:  custom: id=7 a=0 ch=0 bufLen=3 0x01 0x02 0x03
    # 2016-09-01 23:58:31.124:  custom: r=0 id=7 a=0 ch=0 bufLen=3 0x01 0x02 0x03
    customPayloadPattern = \
        timestampPattern + r'custom: (r=(\d+))? id=(\d+) a=(\d+) ch=(\d+) bufLen=(\d+)((?:\s+0x[0-9a-fA-F]{2})+)$'

    lineCount = 0

    try:
        with open(logFileName, 'r') as f:
            for line in f:
                if lineCount % 100 == 0:
                    print('{0}:  Processed {1} lines.  Last timestamp was {2}.'.format(datetime.now(), lineCount, lastTimestamp))
                lineCount += 1
                line = line.rstrip()

                m = re.search(pvPayloadPattern, line)
                if m is not None:
                    widgetData = {
                        'timestamp' : m.group(1),
                        'radio' : int(m.group(3)) if m.group(3) else 0,
                        'widgetId' : int(m.group(4)),
                        'isActive' : int(m.group(5)),
                        'channel' : int(m.group(6)),
                        'position' : int(m.group(7)),
                        'velocity' : int(m.group(8)) }
                    sendPv(widgetData)
                    next

                m = re.search(mvecPayloadPattern, line)
                if m is not None:
                    numMeasurements = int(m.group(7))
                    measurements = [int(measmt) for measmt in m.group(8).split()]
                    if (len(measurements) == numMeasurements):
                        widgetData = {
                            'timestamp' : m.group(1),
                            'radio' : int(m.group(3)) if m.group(3) else 0,
                            'widgetId' : int(m.group(4)),
                            'isActive' : int(m.group(5)),
                            'channel' : int(m.group(6)),
                            'measurements' : measurements }
                        sendMvec(widgetData)
                    else:
                        sys.stderr.write('measurement count inconsistency at line {0}\n'.format(lineCount))
                    next

                m = re.search(customPayloadPattern, line)
                if m is not None:
                    payloadLength = int(m.group(7))
                    hexBytes = [h for h in m.group(8).split()]
                    if (len(hexBytes) / 4 == payloadLength):
                        widgetData = {
                            'timestamp' : m.group(1),
                            'radio' : int(m.group(3)) if m.group(3) else 0,
                            'widgetId' : int(m.group(4)),
                            'isActive' : int(m.group(5)),
                            'channel' : int(m.group(6)),
                            'payloadLength' : payloadLength,
                            'hexBytes' : hexBytes }
                        sendCustom(widgetData)
                    else:
                        sys.stderr.write('payload length inconsistency at line {0}\n'.format(lineCount))
                    next

                # If we're here then the line was unrecognized, so ignore it.

    except IOError as e:
        sys.stderr.write('An error occurred with log file {0}.  {1}\n'.format(logFileName, e))

    return


def usage():
    print('Usage:  widgetPlayback.py data_log_file_name')
    return


def main(argv):

    global clientSock
    global patconIpAddress

    ap = argparse.ArgumentParser(description='This program replays widgetRcvr data logs and sends the recorded widget messages to patternController.')

    ap.add_argument('inputFileName', action='store', help='widgetRcvr data log file name')
    ap.add_argument("-p", "--patcon-ip-address", nargs='?', default=defaultPatconIpAddress, help='IP address of host running patternController', dest='patconIpAddress')
    args = ap.parse_args()

    patconIpAddress = args.patconIpAddress
    
    if not os.path.exists(args.inputFileName):
        sys.stderr.write('File {0} does not exist.\n'.format(args.inputFileName))
        return 1

    clientSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    processLogFile(args.inputFileName)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

