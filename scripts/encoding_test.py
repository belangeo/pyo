# -*- encoding: mbcs -*-

import os
import sys
import locale
from pyo64 import *

# sys.getfilesystemencoding() should be used to set the encoding line added by E-Pyo.

print("Default encoding: ", sys.getdefaultencoding())
print("File system encoding: ", sys.getfilesystemencoding())
print("Locale preferred encoding: ", locale.getpreferredencoding())
print("Locale default encoding: ", locale.getdefaultlocale())

# Python 3
p = 'bébêtte/noise.aif'.encode("mbcs")

# Python 2
#p = 'bébêtte/noise.aif'

# sndinfo, upsamp, downsamp, savefile, savefileFromTable 
# need a python layer to encode the path in python 3.
info = sndinfo(p)
print(info)

s = Server().boot()

sf = SfPlayer(p, mul=0.1).out()

s.gui(locals())

# Objects with file input #
# Sf_family
# CvlVerb
# SndTable
# PyoObject.save methods...
