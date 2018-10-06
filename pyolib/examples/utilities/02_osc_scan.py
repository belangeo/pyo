#!/usr/bin/env python
# encoding: utf-8
from __future__ import print_function
"""
Scan Open Sound Control inputs. Launch this script from a terminal.

"""
from pyo import *
import time, sys

if sys.version_info[0] < 3:
    input = raw_input

port = eval(input("Enter the incoming port number : "))

s = Server().boot().start()

print("Play with your OSC interface...")

go = True
def pp(address, *args):
    if go:
        print("Address =", address)
        print("Values =", args)
        print("---------------")

scan = OscDataReceive(port, "*", pp)

again = "y"
while again == "y":
    time.sleep(10)
    go = False
    again = input("Do you want to continue ? (y/n) : ")
    if again == "y":
        print("Continue...")
        go = True

s.stop()
time.sleep(1)
sys.exit()
