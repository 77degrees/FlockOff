#!/usr/bin/python3

import json
import argparse
import sqlite3
import serial
import time
import sys

class surveyJson():
    def __init__(self, raw:str) -> None:
        self._json = None
        self._wifiInx = 0
        self._wifiCount = 0
        self._bleInx = 0
        self._bleCount = 0

        self._loadSurvey(raw)

    def _loadSurvey(self, data:str) -> bool:
        self._json = json.loads(data)

        self._wifiCount = len(self._json['WiFiDevices'])
        self._bleCount = len(self._json['BLEDevices'])
        return (True)

    def getWiFiDevCount(self) ->int:
        return (self._wifiCount)

    def getBLEDevCount(self) ->int:
        return (self._bleCount)

    def getItem(self, key:str) -> None|str|int|float:
        try:
            return (self._json[key])
        except KeyError:
            return (None)

    def getHeader(self) -> None|tuple[str,str,float,float,int]:
        notes = self.getItem('SurveyNotes')
        dateTime = self.getItem('DateTime')
        longitude = self.getItem('LocationLongLat')[1]
        lattitude = self.getItem('LocationLongLat')[0]
        satCount = self.getItem('SatelliteCount')

        if notes == None or dateTime == None or longitude == None or lattitude == None or satCount == None:
            return None

        else:
            return(notes, dateTime, longitude, lattitude, satCount)

    def getNextWifiDevice(self) -> None|tuple[str,str,str,int,int]:
        if self._wifiInx >= self._wifiCount:
            return (None)

        try:
            type = self._json['WiFiDevices'][self._wifiInx]['Subtype']
            ssid = self._json['WiFiDevices'][self._wifiInx]['SSID']
            bssid = self._json['WiFiDevices'][self._wifiInx]['BSSID']
            channel = self._json['WiFiDevices'][self._wifiInx]['Channel']
            rssi = self._json['WiFiDevices'][self._wifiInx]['RSSSI']
        except KeyError:
            return (None)

        self._wifiInx = self._wifiInx + 1

        return ((type, ssid, bssid, channel, rssi))

    def getWifiDevice(self, inx:int) -> None|tuple[str,str,str,int,int]:
        if inx >= self._wifiCount:
            return (None)

        try:
            type = self._json['WiFiDevices'][inx]['Subtype']
            ssid = self._json['WiFiDevices'][inx]['SSID']
            bssid = self._json['WiFiDevices'][inx]['BSSID']
            channel = self._json['WiFiDevices'][inx]['Channel']
            rssi = self._json['WiFiDevices'][inx]['RSSSI']
        except KeyError:
            return (None)


        return ((type, ssid, bssid, channel, rssi))

    def getNextBTDevice(self) -> None|tuple[str,str,int,list,list]:
        if self._bleInx >= self._bleCount:
            return (None)

        try:
            name = self._json['BLEDevices'][self._bleInx]['Name']
            mac = self._json['BLEDevices'][self._bleInx]['MAC']
            rssi = self._json['BLEDevices'][self._bleInx]['RSSI']

            uuid16 = self._json['BLEDevices'][self._bleInx]['UUID16bit']
            uuid128 = self._json['BLEDevices'][self._bleInx]['UUID128bit']
        except KeyError:
            return (None)

        self._bleInx = self._bleInx + 1

        return ((name, mac, rssi, uuid16, uuid128))

    def getBTDevice(self, inx:int) -> None|tuple[str,str,int,list,list]:
        if self.inx >= self._bleInx:
            return (None)

        try:
            name = self._json['BLEDevices'][inx]['name']
            mac = self._json['BLEDevices'][inx]['mac']
            rssi = self._json['BLEDevices'][inx]['rssi']

            uuid16 = self._json['BLEDevices'][inx]['UUID16bit']
            uuid128 = self._json['BLEDevices'][inx]['UUID128bit']
        except KeyError:
            return (None)

        self._bleInx = self._bleInx + 1

        return ((name, mac, rssi, uuid16, uuid128))


class sq3db():
    def __init__(self, fileName: str) -> None:
        self._dbConnection = sqlite3.connect(fileName)
        self._cursor = self._dbConnection.cursor()

        self._createTables()

    def _createTables(self) -> bool:
        try:
            self._cursor.execute("""
                    CREATE TABLE IF NOT EXISTS surveys (
                        surveyInx INTEGER PRIMARY KEY AUTOINCREMENT,
                        notes TEXT NOT NULL,
                        dateTime TEXT NOT NULL,
                        longitude REAL NOT NULL,
                        lattitude REAL NOT NULL,
                        satCount INTEGER NOT NULL
                    );
            """)
        except sqlite3.OperationalError as ex:
            print (ex)

        try:
            self._cursor.execute("""
                    CREATE TABLE IF NOT EXISTS wifi (
                        wifiInx INTEGER PRIMARY KEY AUTOINCREMENT,
                        surveyInx INTEGER NOT NULL,
                        type TEXT NOT NULL,
                        ssid TEXT NOT NULL,
                        bssid TEXT NOT NULL,
                        rssi INTEGER NOT NULL,
                        channel INTEGER NOT NULL,
                        FOREIGN KEY (surveyInx)
                            REFERENCES surveys (surveyInx)
                                ON DELETE CASCADE
                                ON UPDATE NO ACTION
                    );
            """)
        except sqlite3.OperationalError as ex:
            print (ex)

        try:
            self._cursor.execute("""
                    CREATE TABLE IF NOT EXISTS btle (
                        btInx INTEGER PRIMARY KEY AUTOINCREMENT,
                        surveyInx INTEGER NOT NULL,
                        name TEXT NOT NULL,
                        mac TEXT NOT NULL,
                        rssi INTEGER NOT NULL,
                        FOREIGN KEY (surveyInx)
                            REFERENCES surveys (surveyInx)
                                ON DELETE CASCADE
                                ON UPDATE NO ACTION
                    );
            """)
        except sqlite3.OperationalError as ex:
            print (ex)

        try:
            self._cursor.execute("""
                    CREATE TABLE IF NOT EXISTS uuid16 (
                        uuidInx INTEGER PRIMARY KEY AUTOINCREMENT,
                        btInx INTEGER NOT NULL,
                        uuid16 INTEGER NULL,\
                        FOREIGN KEY (btInx)
                            REFERENCES btle (btInx)
                                ON DELETE CASCADE
                                ON UPDATE NO ACTION
                    );
            """)
        except sqlite3.OperationalError as ex:
            print (ex)

        try:
            self._cursor.execute("""
                    CREATE TABLE IF NOT EXISTS uuid128 (
                        uuidInx INTEGER PRIMARY KEY AUTOINCREMENT,
                        btInx INTEGER NOT NULL,
                        uuid128 TEXT NULL,
                        FOREIGN KEY (btInx)
                            REFERENCES btle (btInx)
                                ON DELETE CASCADE
                                ON UPDATE NO ACTION
                    );
            """)
        except sqlite3.OperationalError as ex:
            print (ex)

        return (True)

    def doCommit(self) ->None:
        self._dbConnection.commit()

    def insertSurveyHeader(self, header:tuple) ->None|int:
        qstring = 'INSERT INTO surveys (notes, dateTime, longitude, lattitude, satCount) values ("{}","{}",{},{},{});'.format(
                    header[0], header[1], header[2], header[3], header[4])

        self._cursor.execute(qstring)
        self._dbConnection.commit()

        return (self._cursor.execute('select seq from sqlite_sequence where name = "surveys"').fetchone()[0])

    def insertWiFiDevice(self, pk:int, wifi:tuple, delayCommit:bool=False) ->None:
        qstring = 'INSERT INTO wifi (surveyInx, type, ssid, bssid, channel, rssi) values ({},"{}","{}","{}",{},{});'.format(
                    pk, wifi[0], wifi[1], wifi[2], wifi[3], wifi[4])

        self._cursor.execute(qstring)

        if not delayCommit == True:
            self._dbConnection.commit()

    def insertBLEDevice(self, pk:int, bt:tuple, delayCommit:bool=False) ->None:
        btFK = 0
        qstring = 'INSERT INTO btle (surveyInx, name, mac, rssi) values ({},"{}","{}",{});'.format(
                    pk, bt[0], bt[1], bt[2])

        self._cursor.execute(qstring)

        if not len(bt[3]) == 0 or not len(bt[4]) == 0:
            self._dbConnection.commit
            btFK = self._cursor.execute('select seq from sqlite_sequence where name = "btle"').fetchone()[0]

            for uuid16 in bt[3]:
                qstring = 'INSERT INTO uuid16 (btInx, uuid16) values ({},{})'.format(btFK, uuid16)
                self._cursor.execute(qstring)

            for uuid16 in bt[4]:
                qstring = 'INSERT INTO uuid16 (btInx, uuid16) values ({},{})'.format(btFK, uuid16)
                self._cursor.execute(qstring)

        if not delayCommit == True:
            self._dbConnection.commit()



def stripReturn(b:bytes) ->str:
    b = b.decode('utf-8')
    first = b.find('\n') + 1
    second = b.rfind('\n')
    return (b[first:second])

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', dest='port', required=True, help='Flocker serial port')
    parser.add_argument('--dir', dest='ls', action='store_true', help="list files")
    parser.add_argument('--cat', dest='cat', required=False, type=str, default=None, help='file to cat')
    parser.add_argument('--load', dest='load', required=False, type=str, default=None, help='file to load')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true', help='Extra output')
    parser.add_argument('--dbfile', dest='dbfile', required=True, type=str, help='Sqlite3 DB filename')
    args = parser.parse_args()

    # start with a connection to the device
    s = serial.Serial(port = args.port, baudrate = 115200, bytesize = 8, parity = 'N', stopbits = 1, timeout = 1.0)

    if args.ls == True:
        s.write(b'ls -d\r\n')
        s.read_until()
        time.sleep(1.0)
        print (s.read_all().decode('ascii'))
        sys.exit(0)

    if not args.cat is None:
        cmd = 'cat -d {}\r\n'.format(args.cat)
        s.write(bytes(cmd.encode('utf-8')))
        #read firs line (echo of command), and discard
        tmp = s.read_until().decode('ascii')
        # read the real thing
        tmp = s.read_until().decode('ascii')
        print (tmp)
        sys.exit(0)

    raw = ''

    if not args.load is None:
        cmd = 'cat -d {}\r\n'.format(args.load)
        s.write(bytes(cmd.encode('utf-8')))
        #read firs line (echo of command), and discard
        tmp = s.read_until().decode('ascii')
        # read the real thing
        raw = s.read_until().decode('ascii')

    if len(raw) < 20:
        print ('bad read from file {}'.format(args.load))
        sys.exit(1)

    surv = surveyJson(raw)

    db = sq3db(args.dbfile)

    wcount = 0
    bcount = 0

    h = surv.getHeader()
    if not h is None:
        surveyPK = db.insertSurveyHeader(h)

        w = surv.getNextWifiDevice()
        while not w is None:
            wcount += 1
            db.insertWiFiDevice(surveyPK, w, True)
            w = surv.getNextWifiDevice()

        b = surv.getNextBTDevice()
        while not b is None:
            bcount += 1
            db.insertBLEDevice(surveyPK, b, True)
            b = surv.getNextBTDevice()

        db.doCommit()

        print ('Loaded {} WiFi devices, {} BTLE devices from survey "{}" at {}'.format(
                wcount, bcount, surv.getItem('SurveyNotes'), surv.getItem('DateTime')))
