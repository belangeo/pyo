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

s = Server(sr=44100, nchnls=2, buffersize=1024, duplex=0).boot()

wav = SquareTable(6)
env = CosTable([(0,0), (100,1), (500,.3), (8191,0)])

met = Metro(.1, 10).play()
lfo = Phasor(.1, 0, .5, 0)
amp = TrigEnv(met, env, mul=.1)
pit = TrigXnoiseMidi(met, dist='loopseg', x1=1, x2=lfo, scale=1, mrange=(60,84))

b = Osc(wav, pit,  mul=amp).out()

s.gui(locals())


######################
######################
# 0 = uniform
# 1 = linear min
# 2 = linear max
# 3 = triangular
# 4 = exponential min (x1 = slope {0 =  no slope -> 10 = sharp slope})
# 5 = exponential max (x1 = slope {0 =  no slope -> 10 = sharp slope})
# 6 = biexponential (x1 = bandwidth {0 =  huge bandwidth -> 10 = narrow bandwidth})
# 7 = cauchy (x1 = bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth})
# 8 = weibull (x1 = locator {0 -> 1}, x2 = shape {0.5 = linear min, 1.5 = expon min, 3.2 = simili gaussian})
# 9 = gaussian (x1 = locator {0 -> 1}, x2 = bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth})
# 10 = poisson (x1 = gravity center {0 = low values -> 10 = high values}, x2 = compress/expand range {0.1 = full compress -> 10 full expand})
# 11 = walker (x1 = max value {0.1 -> 1}, x2 = max step {0.1 -> 1})
# 12 = loopseg (x1 = max value {0.1 -> 1}, x2 = max step {0.1 -> 1})
######################
######################
