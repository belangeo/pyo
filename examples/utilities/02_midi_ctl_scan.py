#!/usr/bin/env python
# encoding: utf-8

"""
Scan for Midi controller numbers. Launch this script from a terminal.

"""
from pyo import *
import time

pm_list_devices()

num = input("Enter your Midi interface number : ")

s = Server(duplex=0)
s.setMidiInputDevice(num)
s.boot().start()

print "Play with your Midi controllers..."

def pp(x):
    print "controller number =", x

scan = CtlScan(pp, False)

again = "y"
while again == "y":
    time.sleep(10)
    scan.stop()
    again = raw_input("Do you want to continue ? (y/n) : ")
    if again == "y":
        print "Continue..."
        scan.play()

s.stop()
time.sleep(1)
exit()
