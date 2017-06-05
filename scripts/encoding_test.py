# -*- encoding: utf-8 -*-
### The encoding line is for E-Pyo tempfile only.
### sys.getfilesystemencoding() should be used to set
### the encoding line added by E-Pyo.

import os
import sys
import locale
from pyo import *

print("Default encoding: ", sys.getdefaultencoding())
print("File system encoding: ", sys.getfilesystemencoding())
print("Locale preferred encoding: ", locale.getpreferredencoding())
print("Locale default encoding: ", locale.getdefaultlocale())

### Need a python layer to encode the path in python 3.

# Python 3
p = 'bébêtte/noise.aif'.encode(sys.getfilesystemencoding())

# Python 2
#p = 'bébêtte/noise.aif'

info = sndinfo(p)
print(info)

s = Server().boot()
sf = SfPlayer(p, mul=0.1).out()
s.gui(locals())

"""
1 - Adapt encoding line for E-Pyo tempfile.

2 - Use widestring in C layer.

3 - "if sys.version > 2" -> path.encode(sys.getfilesystemencoding())
    For python2, don't do anything.
 
Objects with file input:
Sf_family, CvlVerb, SndTable, ControlRead, ControlRec
NoteinRead, NoteinRec, Record
sndinfo, upsamp, downsamp, savefile, savefileFromTable
Server: recordOptions, recstart
PyoTableObject: save, write, read  
PyoMatrixObject: write, read  

"""
