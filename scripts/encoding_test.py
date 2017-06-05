# -*- encoding: mbcs -*-
# The encoding line is for E-Pyo tempfile only.
# sys.getfilesystemencoding() should be used to set
# the encoding line added by E-Pyo.

import os
import sys
import locale
from pyo import *

print("Default encoding: ", sys.getdefaultencoding())
print("File system encoding: ", sys.getfilesystemencoding())
print("Locale preferred encoding: ", locale.getpreferredencoding())
print("Locale default encoding: ", locale.getdefaultlocale())

# need a python layer to encode the path in python 3.

# Python 3
p = 'bébêtte/noise.aif'.encode(sys.getfilesystemencoding())

# Python 2
#p = 'bébêtte/noise.aif'

info = sndinfo(p)
print(info)

s = Server().boot()
sf = SfPlayer(p, mul=0.1).out()
s.gui(locals())

# Objects with file input #
# Sf_family, CvlVerb, SndTable, PyoObject.save methods...
# sndinfo, upsamp, downsamp, savefile, savefileFromTable 
