#! /usr/bin/python

from enum import Enum
import mysql.connector
import os
import re
import sys


dbConnectionInfo = {
    'host'      : '127.0.0.1',
    'database'  : 'widget_activity',
    'user'      : 'ross',
    'password'  : 'woof'
}


class LogState(Enum):
    search = 1
    startMeasurementVector = 2
    inMeasurementVector = 3
    startCustomContents = 4
    inCustomContents = 5


def openDbConnection(connInfo):

    try:
        conn = mysql.connector.connect(**connInfo)

    except mysql.connector.errors.Error as e:
        sys.stderr.write('Unable to open database connection.  {0}\n'.format(e))
        return None

    return conn


def closeDbConnection(conn):

    try:
        conn.close()

    except mysql.connector.errors.Error as e:
        sys.stderr.write('An error occurred while closing the database connection.  {0}\n'.format(e))

    return


def doTestQuery(conn):

    try:
        cursor = conn.cursor()
    except mysql.connector.errors.Error as e:
        sys.stderr.write('Unable to get cursor.  {0}\n'.format(e))
        return

    query = ("SELECT payload_type_id, payload_type_desc FROM payload_type")

    try:
        cursor.execute(query)
    except mysql.connector.errors.Error as e:
        sys.stderr.write('An error occurred while executing "{0}".  {1}\n'.format(query, e))
        cursor.close()
        return

    try:
        for (payloadTypeId, payloadTypeDesc) in cursor:
            print("{0}, {1}".format(payloadTypeId, payloadTypeDesc))
    except mysql.connector.errors.Error as e:
        sys.stderr.write('An error occurred while processing results from "{0}".  {1}\n'.format(query, e))

    cursor.close()

    return


def processLogFile(logFileName):

    # 2016-09-01 23:58:31.124:  
    timestampPattern = r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}):  '

    # Got position+velocity payload; Id = 8, active  , ch = 0, position = 30, velocity = 0
    positionVelocityPayloadPattern = r'Got position\+velocity payload; Id = (\d+),\s*(\w+)\s*, ch = (\d+), position = (-?\d+), velocity = (-?\d+)$'

    # Got measurement vector payload; Id = 4, active  , ch = 0, numMeasurements = 5
    measurementVectorPayloadPattern = r'Got measurement vector payload; Id = (\d+),\s*(\w+)\s*, ch = (\d+), numMeasurements = (\d+)$'
    # Got stress test payload; Id = 6, active  , ch = 1, seq = 1, fails = 0 (0% )
    stressTestPayloadPattern = r'Got stress test payload; Id = (\d+),\s*(\w+)\s*, ch = (\d+), seq = (-?\d+), fails = (-?\d+) \(\d+%\s*\)$'

    # Got custom payload; Id = 4, active  , ch = 0, buflen = 32
    # TODO:  untested
    customPayloadPattern = r'Got custom payload; Id = (\d+),\s*(\w+)\s*, ch = (\d+), buflen = (\d+)$'

    lineCount = 0
    currentState = LogState.search
    numMeasurements = None
    measurements = []

    try:
        with open(logFileName, 'r') as f:
            for line in f:
                lineCount += 1
                line = line.rstrip()
                #print(line)

##    startCustomContents = 4
##    inCustomContents = 5

                if currentState is LogState.search:

                    m = re.search(timestampPattern + positionVelocityPayloadPattern, line)
                    if m is not None:
                        timestamp = m.group(1)
                        widgetId = m.group(2)
                        isActive = m.group(3) == 'active'
                        channel = m.group(4)
                        position = m.group(5)
                        velocity = m.group(6)
                        print('Got data:  timestamp={0} id={1} isActive={2} channel={3} position={4} velocity={5}'.format(
                            timestamp, widgetId, isActive, channel, position, velocity))
                        # TODO: insert row
                        next

                    m = re.search(timestampPattern + measurementVectorPayloadPattern, line)
                    if m is not None:
                        timestamp = m.group(1)
                        widgetId = m.group(2)
                        isActive = m.group(3) == 'active'
                        channel = m.group(4)
                        numMeasurements = int(m.group(5))
                        measurements = []
                        currentState = LogState.startMeasurementVector
                        next

                    m = re.search(timestampPattern + stressTestPayloadPattern, line)
                    if m is not None:
                        timestamp = m.group(1)
                        widgetId = m.group(2)
                        isActive = m.group(3) == 'active'
                        channel = m.group(4)
                        payloadCount = m.group(5)
                        txFailureCount = m.group(6)
                        print('Got data:  timestamp={0} id={1} isActive={2} channel={3} payloadCount={4} txFailureCount={5}'.format(
                            timestamp, widgetId, isActive, channel, payloadCount, txFailureCount))
                        # TODO: insert row
                        next

                    m = re.search(timestampPattern + customPayloadPattern, line)
                    if m is not None:
                        timestamp = m.group(1)
                        widgetId = m.group(2)
                        isActive = m.group(3) == 'active'
                        channel = m.group(4)
                        buflen = int(m.group(5))
                        currentState = LogState.startCustomContents
                        next

                    # If we're here then the line was unrecognized, so ignore it.

                elif currentState is LogState.startMeasurementVector:

                    if line == 'Measurements:':
                        currentState = LogState.inMeasurementVector
                    else:
                        sys.stderr.write('Missing "Measurements:" line for measurement vector payload at line {0}.\n'.format(
                            lineCount))
                        currentState = LogState.search
                    next

                elif currentState is LogState.inMeasurementVector:
                    m = re.search(r'\s*(-?\d+)$', line)
                    if m is not None:
                        measurements.append(int(m.group(1)))
                        if len(measurements) == numMeasurements:
                            print('Got data:  timestamp={0} id={1} isActive={2} channel={3}'
                                  ' numMeasurements={4} measurements={5}'.format(
                                      timestamp, widgetId, isActive, channel, numMeasurements, measurements))
                            # TODO: insert row
                            currentState = LogState.search
                    else:
                        sys.stderr.write('Missing or unrecognized measurement line at line {0}.\n'.format(lineCount))
                        currentState = LogState.search

                elif currentState is LogState.startCustomContents:

                    if line == 'Contents:':
                        currentState = LogState.inCustomContents
                    else:
                        sys.stderr.write('Missing "Contents:" line for custom payload at line {0}.\n'.format(
                            lineCount))
                        currentState = LogState.search
                    next

                elif currentState is LogState.inCustomContents:
                    # The contents are a hex number repeated bufline times.
                    if re.search(r'^(?:\s*0[xX][0-9a-fA-F]{{2}}){{{0}}}$'.format(buflen), line):
                        contentBytes = line.split(' ')
                        print('Got data:  timestamp={0} id={1} isActive={2} channel={3}'
                              ' buflen={4} contentBytes={5}'.format(
                                  timestamp, widgetId, isActive, channel, buflen, contentBytes))
                        # TODO: insert row
                        currentState = LogState.search
                    else:
                        sys.stderr.write('Missing or unrecognized contents line at line {0}.\n'.format(lineCount))
                        currentState = LogState.search

                else:

                    sys.stderr.write('Unsupported state {0}.'.format(currentState))
                    sys.exit(1)


    except IOError as e:
        sys.stderr.write('An error with log file {0}.  {1}\n'.format(logFileName, e))
    
    return


def usage():
    print('Usage:  importWidgetRcvrLog.py <filename>')
    return


def main(argv):

    if len(argv) <> 2:
        usage()
        sys.exit(2)

    inputFileName = argv[1]
    if not os.path.exists(inputFileName):
        sys.stderr.write('File {0} does not exist.\n'.format(inputFileName))
        sys.exit(1)


    dbConn = openDbConnection(dbConnectionInfo)
    if dbConn is None:
        sys.exit(1)

    #doTestQuery(dbConn)

    processLogFile(inputFileName)

    closeDbConnection(dbConn)

    sys.exit(0)


if __name__ == "__main__":
    main(sys.argv)

