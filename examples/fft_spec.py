#! /usr/bin/env python
# encoding: utf-8
"""
Display the sonogram of a sound using a PyoMatrixObject.
A better display can be achieved by using a custom drawing.

After the sound finished to play, call "m.view()" from the 
interpreter widget of the Server window to show the spectrum.

created by belangeo, 31-05-2011

"""
from pyo import *

s = Server().boot()

son = 'baseballmajeur_m.aif'
info = sndinfo(son)
a = SfPlayer(son, mul=.25).mix(1).out()

size = 512
m = NewMatrix(width=size, height=info[0]/size)

fin = FFT(a*100, overlaps=1)
mag = Sqrt(fin["real"]*fin["real"] + fin["imag"]*fin["imag"])
rec = MatrixRec(mag*2-1, m, 0).play()

s.gui(locals())
