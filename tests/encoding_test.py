#! -*- encoding: utf-8 -*-
import os
import sys
import locale
from random import uniform
from pyo import *

### The encoding line is for E-Pyo tempfile only.
### sys.getfilesystemencoding() should be used to set
### the encoding line added by E-Pyo.

print("Default encoding: ", sys.getdefaultencoding())
print("File system encoding: ", sys.getfilesystemencoding())
print("Locale preferred encoding: ", locale.getpreferredencoding())
print("Locale default encoding: ", locale.getdefaultlocale())

s = Server().boot()

### Need a python layer to encode the path in python 3.
# Python 3
#p = 'bébêtte/noise.aif'.encode(sys.getfilesystemencoding())
# Python 2
p = 'bébêtte/noise.aif'

########## SNDINFO ###############
info = sndinfo(p)
print("sndinfo output:\n", info)

####### SAVEFILE ##############
sr, dur, chnls, path = 44100, 5, 2, os.path.join("bébêtte", 'savefile.aif')
samples = [[uniform(-0.5,0.5) for i in range(sr*dur)] for i in range(chnls)]
savefile(samples=samples, path=path, sr=sr, channels=chnls, fileformat=1, sampletype=1)
print("Savefile record:", os.path.isfile(os.path.join("bébêtte", 'savefile.aif')))

####### SAVEFILEFROMTABLE #########
home = os.path.expanduser('~')
path1 = SNDS_PATH + '/transparent.aif'
path2 = os.path.join("bébêtte", 'savefileFromTable.aif')
t = SndTable(path1)
savefileFromTable(table=t, path=path2, fileformat=1, sampletype=1)
print("SavefileFromTable record:", os.path.isfile(os.path.join("bébêtte", 'savefileFromTable.aif')))

##### UPSAMP/DOWNSAMP ######
# upsample a signal 3 times
upfile = os.path.join("bébêtte", 'upsamp.aif')
upsamp(p, upfile, 2, 256)
print("upsamp record:", os.path.isfile(os.path.join("bébêtte", 'upsamp.aif')))
# downsample the upsampled signal 3 times
downfile = os.path.join("bébêtte", 'downsamp.aif')
downsamp(upfile, downfile, 3, 256)
print("downsamp record:", os.path.isfile(os.path.join("bébêtte", 'downsamp.aif')))

######### SfPlayer ###########
sf1 = SfPlayer(p, mul=0.1)
sf1.setPath(downfile)

p2 = os.path.join("bébêtte", 'transparent.aif')
######### SfMarkerShuffler ###########
sf2 = SfMarkerShuffler(p2, mul=0.3)

######### SfMarkerLooper ###########
sf3 = SfMarkerLooper(p2, mul=.3)
rnd = RandInt(len(sf3.getMarkers()), 2)
sf3.mark = rnd

######### SndTable ###########
tab1 = SndTable(p)
tab1.setSound(p2)
tab1.append(p2, 0.2)
tab1.insert(p2, 0.1, 0.1)
tabplay = Osc(tab1, [tab1.getRate(), tab1.getRate() * 1.01], mul=0.5)

######### CvlVerb impulse path ###########
impl = os.path.join("bébêtte", 'IRMediumHallStereo.wav')
cv = CvlVerb(tabplay, impl, size=1024, bal=0.7).out()

######### Record ###########
recfile = os.path.join("bébêtte", 'recfile.wav')
rec = Record(cv, recfile, chnls=2, fileformat=0, sampletype=0, buffering=4, quality=0.40)
recplay = None
def reccallback():
    global rec, recplay
    rec.stop()
    del rec
#    recplay = SfPlayer(recfile, loop=True).out()
#    print("Start playback!")
print("Recording 4 seconds of sounds...")
after = CallAfter(reccallback, 4)

######### Server ###########
servrecfile = os.path.join("bébêtte", 'servrecfile.wav')
s.recordOptions(dur=-1, filename=servrecfile)
servrecfile = os.path.join("bébêtte", 'servrecfile2.wav')
s.recstart(servrecfile)

s.gui(locals(), exit=False)

def delfile(f):
    if os.path.isfile(f):
        os.remove(f)

# On windows, we should delete Sf* objects before deleting the audio files.
del sf1
del recplay
delfile(os.path.join("bébêtte", 'savefile.aif'))
delfile(os.path.join("bébêtte", 'savefileFromTable.aif'))
delfile(os.path.join("bébêtte", 'upsamp.aif'))
delfile(os.path.join("bébêtte", 'downsamp.aif'))
delfile(os.path.join("bébêtte", 'recfile.wav'))
delfile(os.path.join("bébêtte", 'servrecfile.wav'))
delfile(os.path.join("bébêtte", 'servrecfile2.wav'))

"""
1 - Adapt encoding line for E-Pyo tempfile. **done**

2 - Use widestring in C layer.

3 - "if sys.version > 2" -> path.encode(sys.getfilesystemencoding())
    For python2, don't do anything.
    Function stringencode(strng) in _core.py
 
Objects with file input:
sndinfo, savefile, savefileFromTable, upsamp, downsamp **done**
Sf_family **done**
CvlVerb **
SndTable **
Record **
Server: recordOptions, recstart **

ControlRead, ControlRec *nothing to do?*
Expr *to_unicode?* *nothing to do?*
NoteinRead, NoteinRec *nothing to do?*
PyoTableObject: write, read *nothing to do?*
PyoMatrixObject: write, read *nothing to do?*

4 - For method of a C class, use stringencode(st) before the call and
    PyArg_ParseTuple(args, "s#", &self->path, &psize) in the method body.
    METH_VARARGS must be used as the method flag.

"""
