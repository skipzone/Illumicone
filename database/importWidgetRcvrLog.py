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

dbConn = None

payloadTypeTable = {}

sqlInsertWidgetPacketRow = """
    INSERT widget_packet (rcvr_timestamp, widget_id, is_active, channel, payload_type_id)
    VALUES (%(timestamp)s, %(widgetId)s, %(isActive)s, %(channel)s, %(payloadTypeId)s)
"""

lineCount = 0


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


def loadPayloadTypeTable():

    global payloadTypeTable

    query = ("SELECT payload_type_id, payload_type_desc FROM payload_type")

    try:
        cursor = dbConn.cursor()
        cursor.execute(query)
    except mysql.connector.errors.Error as e:
        sys.stderr.write('An error occurred while executing "{0}".  {1}\n'.format(query, e))
        return False

    payloadTypeTable = {}
    try:
        for (payloadTypeId, payloadTypeDesc) in cursor:
            payloadTypeTable[payloadTypeDesc] = payloadTypeId
    except mysql.connector.errors.Error as e:
        sys.stderr.write('An error occurred while processing results from "{0}".  {1}\n'.format(query, e))

    cursor.close()

    #print("payloadTypeTable = {0}".format(payloadTypeTable))

    return True


def importStressTest(widgetData):

    global payloadTypeTable
    global sqlInsertWidgetPacketRow
    global lineCount

    sqlInsertStressTestPayloadRow = """
        INSERT stress_test_payload (widget_packet_id, payload_count, tx_failure_count)
        VALUES (%(widgetPacketId)s, %(payloadCount)s, %(txFailureCount)s)
    """

    try:
        cursor = dbConn.cursor()
        widgetData['payloadTypeId'] = payloadTypeTable['stress test']
        cursor.execute(sqlInsertWidgetPacketRow, widgetData)
        widgetData['widgetPacketId'] = cursor.lastrowid
        cursor.execute(sqlInsertStressTestPayloadRow, widgetData)
        dbConn.commit()
        cursor.close()

    except mysql.connector.errors.Error as e:
        dbConn.rollback()
        sys.stderr.write('importStressTest for line {0}:  {1}\n'.format(lineCount, e))
        return False

    return True


def importPositionVelocity(widgetData):

    global payloadTypeTable
    global sqlInsertWidgetPacketRow
    global lineCount

    sqlInsertPositionVelocityPayloadRow = """
        INSERT position_velocity_payload (widget_packet_id, position, velocity)
        VALUES (%(widgetPacketId)s, %(position)s, %(velocity)s)
    """

    try:
        cursor = dbConn.cursor()
        widgetData['payloadTypeId'] = payloadTypeTable['position/velocity']
        cursor.execute(sqlInsertWidgetPacketRow, widgetData)
        widgetData['widgetPacketId'] = cursor.lastrowid
        cursor.execute(sqlInsertPositionVelocityPayloadRow, widgetData)
        dbConn.commit()
        cursor.close()

    except mysql.connector.errors.Error as e:
        dbConn.rollback()
        sys.stderr.write('importPositionVelocity for line {0}:  {1}\n'.format(lineCount, e))
        return False

    return True


def importMeasurementVector(widgetData, measurements):

    global payloadTypeTable
    global sqlInsertWidgetPacketRow
    global lineCount

    sqlInsertMeasurementVectorPayloadRow = """
        INSERT measurement_vector_payload (widget_packet_id, measurement_idx, measurement)
        VALUES (%s, %s, %s)
    """

    try:
        cursor = dbConn.cursor()
        widgetData['payloadTypeId'] = payloadTypeTable['measurement vector']
        cursor.execute(sqlInsertWidgetPacketRow, widgetData)
        widgetPacketId = cursor.lastrowid
        for i, measurement in enumerate(measurements):
            #print("i={0} measurement={1}".format(i, measurement))
            cursor.execute(sqlInsertMeasurementVectorPayloadRow, (widgetPacketId, i, measurement))
        dbConn.commit()
        cursor.close()

    except mysql.connector.errors.Error as e:
        dbConn.rollback()
        sys.stderr.write('importMeasurementVector for line {0}:  {1}\n'.format(lineCount, e))
        return False

    return True


def importCustom(widgetData):

    global payloadTypeTable
    global sqlInsertWidgetPacketRow
    global lineCount

    sqlInsertCustomPayloadRow = """
        INSERT custom_payload (widget_packet_id, payload_length, data_bytes)
        VALUES (%(widgetPacketId)s, %(payloadLength)s, %(dataBytes)s)
    """

    try:
        cursor = dbConn.cursor()
        widgetData['payloadTypeId'] = payloadTypeTable['custom']
        cursor.execute(sqlInsertWidgetPacketRow, widgetData)
        widgetData['widgetPacketId'] = cursor.lastrowid
        cursor.execute(sqlInsertCustomPayloadRow, widgetData)
        dbConn.commit()
        cursor.close()

    except mysql.connector.errors.Error as e:
        dbConn.rollback()
        sys.stderr.write('importCustom for line {0}:  {1}\n'.format(lineCount, e))
        return False

    return True


def processLogFile(logFileName):

    global lineCount

    # 2016-09-01 23:58:31.124:  
    timestampPattern = r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}):  '

    # Got position+velocity payload; Id = 8, active  , ch = 0, position = 30, velocity = 0
    positionVelocityPayloadPattern = r'Got position\+velocity payload; Id = (\d+),\s*(\w+)\s*, ch = (\d+), position = (-?\d+), velocity = (-?\d+)$'

    # Got measurement vector payload; Id = 4, active  , ch = 0, numMeasurements = 5
    measurementVectorPayloadPattern = r'Got measurement vector payload; Id = (\d+),\s*(\w+)\s*, ch = (\d+), numMeasurements = (\d+)$'
    # Got stress test payload; Id = 6, active  , ch = 1, seq = 1, fails = 0 (0% )
    stressTestPayloadPattern = r'Got stress test payload; Id = (\d+),\s*(\w+)\s*, ch = (\d+), seq = (-?\d+), fails = (-?\d+) \(\d+%\s*\)$'

    # Got custom payload; Id = 4, active  , ch = 0, bufLen = 31
    customPayloadPattern = r'Got custom payload; Id = (\d+),\s*(\w+)\s*, ch = (\d+), bufLen = (\d+)$'

    lineCount = 0
    currentState = LogState.search

    try:
        with open(logFileName, 'r') as f:
            for line in f:
                lineCount += 1
                line = line.rstrip()
                #print(line)

                if currentState is LogState.search:

                    m = re.search(timestampPattern + stressTestPayloadPattern, line)
                    if m is not None:
                        widgetData = {
                            'timestamp' : m.group(1),
                            'widgetId' : m.group(2),
                            'isActive' : m.group(3) == 'active',
                            'channel' : m.group(4),
                            'payloadCount' : m.group(5),
                            'txFailureCount' : m.group(6) }
                        #print('Got data:  {0}'.format(widgetData))
                        importStressTest(widgetData)
                        next

                    m = re.search(timestampPattern + positionVelocityPayloadPattern, line)
                    if m is not None:
                        widgetData = {
                            'timestamp' : m.group(1),
                            'widgetId' : m.group(2),
                            'isActive' : m.group(3) == 'active',
                            'channel' : m.group(4),
                            'position' : m.group(5),
                            'velocity' : m.group(6) }
                        #print('Got data:  {0}'.format(widgetData))
                        importPositionVelocity(widgetData)
                        next

                    m = re.search(timestampPattern + measurementVectorPayloadPattern, line)
                    if m is not None:
                        widgetData = {
                            'timestamp' : m.group(1),
                            'widgetId' : m.group(2),
                            'isActive' : m.group(3) == 'active',
                            'channel' : m.group(4),
                            'numMeasurements' : int(m.group(5)) }
                        measurements = []
                        currentState = LogState.startMeasurementVector
                        next

                    m = re.search(timestampPattern + customPayloadPattern, line)
                    if m is not None:
                        widgetData = {
                            'timestamp' : m.group(1),
                            'widgetId' : m.group(2),
                            'isActive' : m.group(3) == 'active',
                            'channel' : m.group(4),
                            'payloadLength' : int(m.group(5)) }
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
                        if len(measurements) == widgetData['numMeasurements']:
                            #print('Got data:  {0}  {1}'.format(widgetData, measurements))
                            importMeasurementVector(widgetData, measurements)
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
                    if re.search(r'^(?:\s*0[xX][0-9a-fA-F]{{2}}){{{0}}}$'.format(widgetData['payloadLength']), line):
                        widgetData['dataBytes'] = ''.join([chr(int(h, 16)) for h in line.split(' ')])
                        #print('Got data:  {0}'.format(widgetData))
                        importCustom(widgetData)
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

    global dbConn

    if len(argv) <> 2:
        usage()
        return 2

    inputFileName = argv[1]
    if not os.path.exists(inputFileName):
        sys.stderr.write('File {0} does not exist.\n'.format(inputFileName))
        return 1

    dbConn = openDbConnection(dbConnectionInfo)
    if dbConn is None:
        return 1

    if not loadPayloadTypeTable():
        return 1

    processLogFile(inputFileName)

    closeDbConnection(dbConn)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

