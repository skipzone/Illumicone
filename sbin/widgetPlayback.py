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


from datetime import datetime, timedelta
import os
import re
import socket
from struct import *
import sys
from time import sleep


"""
static struct sockaddr_in widgetSockAddr[16];
static int widgetSock[16];
    unsigned int widgetIdNumber = widgetIdToInt(widgetId);
    unsigned int portNumber = widgetPortNumberBase + widgetIdNumber;

// UDP to the pattern controller
#pragma pack(1)
struct UdpPayload {
    uint8_t id;
    uint8_t channel;
    uint8_t isActive;
    int16_t position;
    int16_t velocity;
};
"""

# TODO:  get these from config
patconIpAddress = '127.0.0.1'   #'192.168.69.103'
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
            if (intervalS > 0 and intervalS < timeCompressionThresholdSeconds):
                sleep(intervalS)

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
    pvPayloadPattern = timestampPattern + r'pv: id=(\d+) a=(\d+) ch=(\d+) p=(-?\d+) v=(-?\d+)$'

    # 2016-09-01 23:58:31.124:  mvec: id=7 a=0 ch=0 n=3 1 2 3
    ##mvecPayloadPattern = r'mvec: id=(\d+) a=(\d+) ch=(\d+) n=(\d+)(\s+-?\d+)+$'
    mvecPayloadPattern = timestampPattern + r'mvec: id=(\d+) a=(\d+) ch=(\d+) n=(\d+)((?:\s+-?\d+)+)$'

    # 2016-09-01 23:58:31.124:  custom: id=7 a=0 ch=0 bufLen=3 0x01 0x02 0x03
    customPayloadPattern = timestampPattern + r'custom: id=(\d+) a=(\d+) ch=(\d+) bufLen=(\d+)((?:\s+0x[0-9a-fA-F]{2})+)$'

    lineCount = 0

    try:
        with open(logFileName, 'r') as f:
            for line in f:
                if lineCount % 1000 == 0:
                    print('Processed {0} lines.  Last timestamp was {1}.'.format(lineCount, lastTimestamp))
                lineCount += 1
                line = line.rstrip()

                m = re.search(pvPayloadPattern, line)
                if m is not None:
                    widgetData = {
                        'timestamp' : m.group(1),
                        'widgetId' : int(m.group(2)),
                        'isActive' : int(m.group(3)),
                        'channel' : int(m.group(4)),
                        'position' : int(m.group(5)),
                        'velocity' : int(m.group(6)) }
                    sendPv(widgetData)
                    next

                m = re.search(mvecPayloadPattern, line)
                if m is not None:
                    numMeasurements = int(m.group(5))
                    measurements = [int(measmt) for measmt in m.group(6).split()]
                    if (len(measurements) == numMeasurements):
                        widgetData = {
                            'timestamp' : m.group(1),
                            'widgetId' : int(m.group(2)),
                            'isActive' : int(m.group(3)),
                            'channel' : int(m.group(4)),
                            'measurements' : measurements }
                        sendMvec(widgetData)
                    else:
                        sys.stderr.write('measurement count inconsistency at line {0}\n'.format(lineCount))
                    next

                m = re.search(customPayloadPattern, line)
                if m is not None:
                    payloadLength = int(m.group(5))
                    hexBytes = [h for h in m.group(6).split()]
                    if (len(hexBytes) / 4 == payloadLength):
                        widgetData = {
                            'timestamp' : m.group(1),
                            'widgetId' : int(m.group(2)),
                            'isActive' : int(m.group(3)),
                            'channel' : int(m.group(4)),
                            'payloadLength' : int(m.group(5)),
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

    if len(argv) <> 2:
        usage()
        return 2

    inputFileName = argv[1]
    if not os.path.exists(inputFileName):
        sys.stderr.write('File {0} does not exist.\n'.format(inputFileName))
        return 1

    clientSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    processLogFile(inputFileName)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

