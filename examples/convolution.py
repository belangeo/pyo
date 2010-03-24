#!/usr/bin/env python
# encoding: utf-8
"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

sf = Noise(.5)

TLEN = 1000

inmic = Input()

t1 = NewTable(length=sampsToSec(TLEN), chnls=1)
r1 = TableRec(inmic, table=t1, fadetime=.001)

t2 = NewTable(length=sampsToSec(TLEN), chnls=1)
r2 = TableRec(inmic, table=t2, fadetime=.001)

t3 = NewTable(length=sampsToSec(TLEN), chnls=1)
r3 = TableRec(inmic, table=t3, fadetime=.001)

t4 = NewTable(length=sampsToSec(TLEN), chnls=1)
r4 = TableRec(inmic, table=t4, fadetime=.001)

pha = Sig(0)
pha.ctrl()

t = NewTable(length=sampsToSec(TLEN), chnls=1)
m = TableMorph(pha, t, [t1,t2,t3,t4])
a = Convolve(sf, table=t, size=t.getSize(), mul=.2).out()

s.gui(locals())