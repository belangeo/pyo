#!/usr/bin/env python
# encoding: utf-8
"""
Frequency modulation synthesis with graphical control of the amplitude
envelope and the modulation index shape.

The graph() method is defined in trajectory tables (LinTable, CosTable, ExpTable and
CurveTable), also for line objects (Linseg and Expseg). This method shows a grapher
window with which the user can set the shape of the trajectory.

With the focus on the graph, the copy menu item (Ctrl+C) saves to the
clipboard the list of points in the format needed by LinTable, ExpTable, etc.
Useful to experiment graphically and then copy/paste the result in the script.

* WxPython graphical library must be installed. Installer can be downloaded at:
http://www.wxpython.org/

"""
from pyo import *

s = Server(duplex=0).boot()

NOTE_DUR = 2

# Tables intialization
amp_table = CosTable([(0,0), (100,1), (1024,.3), (8192,0)])
ind_table = LinTable([(0,20), (1024,3), (8192,0)])

# call the graph() method of each table
amp_table.graph(title="Amplitude envelope")
# yrange argument allow to set minimum and maximum bundaries of the graph
ind_table.graph(yrange=(0, 20), title="Modulation index shape")

# Trig the notes
met = Metro(NOTE_DUR).play()

# Pick a random frequency
note = TrigXnoiseMidi(met, dist=12, scale=0, mrange=(36, 60))
freq = Snap(note, choice=[0,2,4,5,7,9,11], scale=1)

# Read the tables
amp = TrigEnv(met, table=amp_table, dur=NOTE_DUR, mul=.3)
ind = TrigEnv(met, table=ind_table, dur=NOTE_DUR)

fm = FM(carrier=freq, ratio=[.997, 1.002], index=ind, mul=amp).out()

s.gui(locals())
