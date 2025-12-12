#!/usr/bin/python3

import json
import argparse
import sqlite3
import serial
import time

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', dest='port', required=True, help='Flocker serial port')
    parser.add_argument('-c', '--command', dest='cmd', required=True, type=str, help="what to do?")
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true', help='Extra output')
    args = parser.parse_args()

    s = serial.Serial(port = args.port, baudrate = 115200, bytesize = 8, parity = 'N', stopbits = 1, timeout = 1.0)
    s.write(b'\r\n')
    time.sleep(0.5)
    d = s.read_all()

    time.sleep(0.5)
    w = bytes(('{}\r\n'.format(args.cmd)).encode('utf-8'))
    s.write(w)
    time.sleep(0.1)

    while (s.in_waiting):
        d += s.read_all()
        time.sleep(0.1)

    print (d.decode('utf-8'))


    s.close()