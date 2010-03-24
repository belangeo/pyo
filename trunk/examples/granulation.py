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
s = Server(buffersize=512).boot()

snd = SndTable(DEMOS_PATH + '/transparent.aif')
env = HannTable()

pos = Phasor(snd.getRate()*.1, 0, snd.getSize())
pnz = Noise(5)

trs = Choice([.5,.75,1,1.25], 1000)
dur = Noise(.002, .1)

gran = Granulator(table=snd, env=env, pitch=[.999, 1.0011], pos=pos+pnz, 
                  dur=dur*trs, grains=500, basedur=.1, mul=.01).out()
         
s.gui(locals())

