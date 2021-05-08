# encoding: utf-8

# Copyright 2009-2021 Olivier Belanger
# 
# This file is part of pyo, a python module to help digital signal
# processing script creation.
#
# pyo is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# pyo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with pyo.  If not, see <http://www.gnu.org/licenses/>.


import os
import sys
import time
import inspect
import locale
from subprocess import call
from weakref import proxy

import builtins

bytes_t = bytes
unicode_t = str

def tobytes(strng, encoding="utf-8"):
    "Convert unicode string to bytes."
    return bytes(strng, encoding=encoding)

if hasattr(builtins, "pyo_use_double"):
    from .._pyo64 import *
    import pyo as current_pyo
else:
    from .._pyo import *
    import pyo as current_pyo

######################################################################
### Utilities
######################################################################
current_pyo_path = os.path.dirname(current_pyo.__file__)
SNDS_PATH = os.path.join(current_pyo_path, "lib", "snds")
XNOISE_DICT = {
    "uniform": 0,
    "linear_min": 1,
    "linear_max": 2,
    "triangle": 3,
    "expon_min": 4,
    "expon_max": 5,
    "biexpon": 6,
    "cauchy": 7,
    "weibull": 8,
    "gaussian": 9,
    "poisson": 10,
    "walker": 11,
    "loopseg": 12,
}
FILE_FORMATS = {"wav": 0, "wave": 0, "aif": 1, "aiff": 1, "au": 2, "": 3, "sd2": 4, "flac": 5, "caf": 6, "ogg": 7}
FUNCTIONS_INIT_LINES = {
    "midiToHz": "midiToHz(x)",
    "hzToMidi": "hzToMidi(x)",
    "midiToTranspo": "midiToTranspo(x)",
    "sampsToSec": "sampsToSec(x)",
    "secToSamps": "secToSamps(x)",
    "beatToDur": "beatToDur(bpm, beat)",
    "linToCosCurve": "linToCosCurve(data, yrange=[0, 1], totaldur=1, points=1024, log=False)",
    "rescale": "rescale(data, xmin=0.0, xmax=1.0, ymin=0.0, ymax=1.0, xlog=False, ylog=False)",
    "distanceToSegment": "distanceToSegment(p, p1, p2, xmin=0.0, xmax=1.0, "
    "ymin=0.0, ymax=1.0, xlog=False, ylog=False)",
    "reducePoints": "reducePoints(pointlist, tolerance=0.02)",
    "serverCreated": "serverCreated()",
    "serverBooted": "serverBooted()",
    "class_args": "class_args(cls)",
    "getVersion": "getVersion()",
    "getPrecision": "getPrecision()",
    "convertStringToSysEncoding": "convertStringToSysEncoding(str)",
    "convertArgsToLists": "convertArgsToLists(*args)",
    "wrap": "wrap(arg, i)",
    "floatmap": "floatmap(x, min=0, max=1, exp=1)",
    "getPyoKeywords": "getPyoKeywords()",
}


def get_random_integer(mx=32767):
    if sys.version_info[0] < 3 or sys.version_info[1] < 3:
        seed = int(str(time.clock()).split(".")[1])
    else:
        seed = int(str(time.process_time()).split(".")[1])
    return (seed * 31351 + 21997) % mx


def listscramble(lst):
    if sys.version_info[0] < 3 or sys.version_info[1] < 3:
        seed = int(str(time.clock()).split(".")[1])
    else:
        seed = int(str(time.process_time()).split(".")[1])
    l = lst[:]
    new = []
    pos = 1
    while l:
        pos = (pos * seed) % len(l)
        new.append(l[pos])
        del l[pos]
    return new

def stringencode(st):
    if sys.version_info[0] >= 3:
        if sys.version_info[1] <= 5:
            if type(st) is str:
                st = st.encode(sys.getfilesystemencoding())
        else:
            if type(st) is str:
                st = st.encode(locale.getpreferredencoding())
    return st

class PyoError(Exception):
    """Base class for all pyo exceptions."""


class PyoServerStateException(PyoError):
    """Error raised when an operation requires the server to be booted."""


class PyoArgumentTypeError(PyoError):
    """Error raised when if an object got an invalid argument."""


def isAudioObject(obj):
    return isinstance(obj, PyoObject) or hasattr(obj, "stream")


def isTableObject(obj):
    return isinstance(obj, PyoTableObject) or hasattr(obj, "tablestream")


def isMatrixObject(obj):
    return isinstance(obj, PyoMatrixObject) or hasattr(obj, "matrixstream")


def isPVObject(obj):
    return isinstance(obj, PyoPVObject) or hasattr(obj, "pv_stream")


def pyoArgsAssert(obj, format, *args):
    i = 0
    expected = ""
    for i, arg in enumerate(args):
        f = format[i]
        argtype = type(arg)
        if f == "O":
            if not isAudioObject(arg) and argtype not in [list, int, float]:
                expected = "float or PyoObject"
        elif f == "o":
            if not isAudioObject(arg) and argtype not in [list]:
                expected = "PyoObject"
        elif f == "T":
            if not isTableObject(arg) and argtype not in [int, float, list]:
                expected = "int, float or PyoTableObject"
        elif f == "t":
            if not isTableObject(arg) and argtype not in [list]:
                expected = "PyoTableObject"
        elif f == "m":
            if not isMatrixObject(arg) and argtype not in [list]:
                expected = "PyoMatrixObject"
        elif f == "p":
            if not isPVObject(arg) and argtype not in [list]:
                expected = "PyoPVObject"
        elif f == "n":
            if argtype not in [list, int, float]:
                expected = "any number"
        elif f == "N":
            if argtype not in [int, float]:
                expected = "any number - list not allowed"
        elif f == "f":
            if argtype not in [list, float]:
                expected = "float"
        elif f == "F":
            if argtype not in [float]:
                expected = "float - list not allowed"
        elif f == "i":
            if argtype not in [list, int]:
                expected = "integer"
        elif f == "I":
            if argtype not in [int]:
                expected = "integer - list not allowed"
        elif f == "s":
            if argtype not in [list, bytes_t, unicode_t]:
                expected = "string"
        elif f == "S":
            if argtype not in [bytes_t, unicode_t]:
                expected = "string - list not allowed"
        elif f == "b":
            if argtype not in [bool, list, int]:
                expected = "boolean"
        elif f == "B":
            if argtype not in [bool, int]:
                expected = "boolean - list not allowed"
        elif f == "l":
            if argtype not in [list]:
                expected = "list"
        elif f == "L":
            if argtype not in [list, type(None)]:
                expected = "list or None"
        elif f == "u":
            if argtype not in [tuple]:
                expected = "tuple"
        elif f == "x":
            if argtype not in [list, tuple]:
                expected = "list or tuple"
        elif f == "c":
            if not callable(arg) and argtype not in [list, tuple, type(None)]:
                expected = "callable"
        elif f == "C":
            if not callable(arg) and argtype not in [type(None)]:
                expected = "callable - list not allowed"
        elif f == "z":
            pass

        if expected:
            break

    if expected:
        name = obj.__class__.__name__
        err = 'bad argument at position %d to "%s" (%s expected, got %s)'
        raise PyoArgumentTypeError(err % (i, name, expected, argtype))


def convertStringToSysEncoding(strng):
    if type(strng) not in [bytes_t, unicode_t]:
        strng = strng.decode("utf-8")
    strng = strng.encode(sys.getfilesystemencoding())
    return strng


def convertArgsToLists(*args):
    converted = list(args)
    for i, arg in enumerate(converted):
        if not isinstance(arg, PyoObjectBase) and not isinstance(arg, list):
            converted[i] = [arg]

    max_length = max(len(arg) for arg in converted)
    converted.append(max_length)
    return tuple(converted)


def wrap(arg, i):
    x = arg[i % len(arg)]
    if isinstance(x, PyoObjectBase):
        return x[0]
    else:
        return x


def removeExtraDecimals(x):
    if isinstance(x, float):
        return "=%.2f" % x
    elif type(x) in [bytes_t, unicode_t]:
        return '="%s"' % x
    else:
        return "=" + str(x)


def class_args(cls):
    try:
        arg = str(inspect.signature(cls))
        return cls.__name__ + arg
    except:
        try:
            # Try for a built-in function
            name = cls.__name__
            if name in FUNCTIONS_INIT_LINES:
                return FUNCTIONS_INIT_LINES[name]
        except:
            return ""

def beatToDur(beat, bpm=120):
    if type(beat) is tuple or type(bpm) is tuple:
        if type(beat) is not tuple:
            beat = (beat,)
        if type(bpm) is not tuple:
            bpm = (bpm,)
        lmax = max(len(beat), len(bpm))
        if lmax == 1:
            return 60.0 / bpm[0] * beat[0]
        else:
            return tuple([60.0 / wrap(bpm, i) * wrap(beat, i) for i in range(lmax)])

    if type(beat) is not list:
        beat = [beat]
    if type(bpm) is not list:
        bpm = [bpm]
    lmax = max(len(beat), len(bpm))
    if lmax == 1:
        return 60.0 / bpm[0] * beat[0]
    else:
        return [60.0 / wrap(bpm, i) * wrap(beat, i) for i in range(lmax)]


def getVersion():
    major, minor, rev = PYO_VERSION.split(".")
    return (int(major), int(minor), int(rev))


def getPrecision():
    if hasattr(builtins, "pyo_use_double"):
        return 64
    else:
        return 32


def getWeakMethodRef(x):
    if type(x) in [list, tuple]:
        tmp = []
        for y in x:
            if hasattr(y, "__self__"):
                y = WeakMethod(y)
            tmp.append(y)
        x = tmp
    else:
        if hasattr(x, "__self__"):
            x = WeakMethod(x)
    return x


class WeakMethod(object):

    def __new__(cls, callobj):
        if callable(callobj):
            return super(WeakMethod, cls).__new__(cls)

    def __init__(self, callobj):
        if hasattr(callobj, "__self__"):
            self.target = proxy(callobj.__self__)
            self.method = proxy(callobj.__func__)
            self.isMethod = True
        else:
            self.method = callobj
            self.isMethod = False

    def __call__(self, *args, **kwargs):
        """Call the method with args and kwargs as needed."""
        if self.isMethod:
            return self.method(self.target, *args, **kwargs)
        else:
            return self.method(*args, **kwargs)


######################################################################
### PyoObjectBase -> abstract class for pyo objects
######################################################################
class PyoObjectBase(object):

    # Descriptive word for this kind of object, for use in printing
    # descriptions of the object. Subclasses need to set this.
    _STREAM_TYPE = ""

    def __init__(self):
        self._base_objs = []
        self._trig_objs = None
        self.__index = 0
        self._stop_delay = -1
        self._is_mul_attribute = False
        self._use_wait_time_on_stop = False
        self._allow_auto_start = True
        self._linked_objects = []
        if not serverCreated():
            raise PyoServerStateException("You must create and boot a Server before creating any audio object.")
        if not serverBooted():
            raise PyoServerStateException("The Server must be booted before creating any audio object.")

    def dump(self):
        attrs = dir(self)
        pp = "< Instance of %s class >" % self.__class__.__name__
        pp += "\n-----------------------------"
        pp += "\nNumber of %s streams: %d" % (self._STREAM_TYPE, len(self))
        pp += "\n--- Attributes ---"
        for attr in attrs:
            pp += "\n" + attr + ": " + str(getattr(self, attr))
        pp += "\n-----------------------------"
        return pp

    def getBaseObjects(self):
        return self._base_objs

    def getServer(self):
        return self._base_objs[0].getServer()

    def getSamplingRate(self):
        return self._base_objs[0].getServer().getSamplingRate()

    def getBufferSize(self):
        return self._base_objs[0].getServer().getBufferSize()

    def allowAutoStart(self, switch=True):
        self._allow_auto_start = switch

    def useWaitTimeOnStop(self):
        self._use_wait_time_on_stop = True

    def addLinkedObject(self, x):
        self._linked_objects.append(x)

    def setStopDelay(self, x): 
        self._stop_delay = x
        return self

    def getStopDelay(self):
        return self._stop_delay

    def __iter__(self):
        self.__index = 0
        return self

    def __next__(self):
        if self.__index >= len(self):
            raise StopIteration
        x = self[self.__index]
        self.__index += 1
        return x

    def next(self):
        # In Python 2.x, __next__() method is called next().
        return self.__next__()

    def __getitem__(self, i):
        if i == "trig":
            return self._trig_objs
        if type(i) == slice:
            return self._base_objs[i]
        elif isinstance(i, int) and i < len(self._base_objs):
            return self._base_objs[i]
        else:
            if type(i) in [bytes_t, unicode_t]:
                args = (self.__class__.__name__, i)
                print("Object %s has no stream named '%s'!" % args)
            else:
                args = (self._STREAM_TYPE, self.__class__.__name__)
                print("'i' too large in indexing %s object %s!" % args)

    def __setitem__(self, i, x):
        self._base_objs[i] = x

    def __len__(self):
        return len(self._base_objs)

    def __repr__(self):
        return "< Instance of %s class >" % self.__class__.__name__

    def __dir__(self):
        init = getattr(self.__class__, "__init__")
        args, _, _, _, _, _, _ = inspect.getfullargspec(init)
        args = [a for a in args if hasattr(self.__class__, a) and a != "self"]
        return args

    def _autoplay(self, dur=0, delay=0):
        if self.getServer().getAutoStartChildren() and self._allow_auto_start:
            children = [getattr(self, at) for at in dir(self)] + self._linked_objects
            for obj in children:
                if isAudioObject(obj):
                    if not hasattr(obj, "_allow_auto_start"):
                        if obj._getStream().isOutputting():  # if outputting, don't call play().
                            return
                        obj.play(dur[0], delay[0])
                    elif obj._allow_auto_start:
                        if obj.isOutputting():
                            return
                        obj.play(dur, delay)
                elif type(obj) is list:  # Handle list of audio objects.
                    for subobj in obj:
                        if isAudioObject(subobj):
                            if not hasattr(subobj, "_allow_auto_start"):
                                if subobj._getStream().isOutputting():
                                    return
                                subobj.play(dur[0], delay[0])
                            elif subobj._allow_auto_start:
                                if subobj.isOutputting():
                                    return
                                subobj.play(dur, delay)

    def _autostop(self, wait=0):
        if self.getServer().getAutoStartChildren():
            children = [(getattr(self, at), at) for at in dir(self)] + [(obj, "") for obj in self._linked_objects]
            for tup in children:
                if isAudioObject(tup[0]):
                    if not hasattr(tup[0], "_allow_auto_start"):
                        # XXX_base objects always wait, even for mul attribute.
                        tup[0].stop(wait)
                    elif tup[0]._allow_auto_start:
                        if tup[1] == "mul":
                            # Start fadeout immediately.
                            tup[0]._is_mul_attribute = True
                            tup[0].stop(wait)
                            tup[0]._is_mul_attribute = False
                        else:
                            # Every other attributes wait.
                            tup[0].stop(wait)
                # Handle list of audio objects.
                elif type(tup[0]) is list:
                    ismul = tup[1] == "mul"
                    for subobj in tup[0]:
                        if isAudioObject(subobj) and subobj._allow_auto_start:
                            if not hasattr(subobj, "_allow_auto_start"):
                                # XXX_base objects always wait, even for mul attribute.
                                subobj.stop(wait)
                            elif subobj._allow_auto_start:
                                if ismul:
                                    # Start fadeout immediately.
                                    subobj._is_mul_attribute = True
                                    subobj.stop(wait)
                                    subobj._is_mul_attribute = False
                                else:
                                    # Every other attributes wait.
                                    subobj.stop(wait)


######################################################################
### PyoObject -> base class for pyo sound objects
######################################################################
class PyoObject(PyoObjectBase):

    _STREAM_TYPE = "audio"

    def __init__(self, mul=1.0, add=0.0):
        PyoObjectBase.__init__(self)
        self._target_dict = {}
        self._signal_dict = {}
        self._callback_dict = {}
        self._keep_trace = []
        self._mul = mul
        self._add = add
        self._op_duplicate = 1
        self._zeros = None
        self._base_players = None
        self._time_objs = None

    def __add__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _add_dummy = Dummy([obj + wrap(x, i // self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                _add_dummy = x + self
            else:
                _add_dummy = Dummy([wrap(self._base_objs, i) + obj for i, obj in enumerate(x)])
        self._keep_trace.append(_add_dummy)
        return _add_dummy

    def __radd__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _add_dummy = Dummy([obj + wrap(x, i // self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            _add_dummy = Dummy([wrap(self._base_objs, i) + obj for i, obj in enumerate(x)])
        self._keep_trace.append(_add_dummy)
        return _add_dummy

    def __iadd__(self, x):
        self.setAdd(x)
        return self

    def __sub__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _add_dummy = Dummy([obj - wrap(x, i // self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                _add_dummy = Dummy([wrap(self._base_objs, i) - wrap(x, i) for i in range(lmax)])
            else:
                _add_dummy = Dummy([wrap(self._base_objs, i) - obj for i, obj in enumerate(x)])
        self._keep_trace.append(_add_dummy)
        return _add_dummy

    def __rsub__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _add_dummy = Dummy([wrap(x, i // self._op_duplicate) - obj for i, obj in enumerate(self._base_objs)])
        else:
            _add_dummy = Dummy([obj - wrap(self._base_objs, i) for i, obj in enumerate(x)])
        self._keep_trace.append(_add_dummy)
        return _add_dummy

    def __isub__(self, x):
        self.setSub(x)
        return self

    def __mul__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _mul_dummy = Dummy([obj * wrap(x, i // self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                _mul_dummy = x * self
            else:
                _mul_dummy = Dummy([wrap(self._base_objs, i) * obj for i, obj in enumerate(x)])
        self._keep_trace.append(_mul_dummy)
        return _mul_dummy

    def __rmul__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _mul_dummy = Dummy([obj * wrap(x, i // self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            _mul_dummy = Dummy([wrap(self._base_objs, i) * obj for i, obj in enumerate(x)])
        self._keep_trace.append(_mul_dummy)
        return _mul_dummy

    def __imul__(self, x):
        self.setMul(x)
        return self

    def __truediv__(self, x):
        return self.__div__(x)

    def __rtruediv__(self, x):
        return self.__rdiv__(x)

    def __itruediv__(self, x):
        return self.__idiv__(x)

    def __div__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _mul_dummy = Dummy([obj / wrap(x, i // self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                _mul_dummy = Dummy([wrap(self._base_objs, i) / wrap(x, i) for i in range(lmax)])
            else:
                _mul_dummy = Dummy([wrap(self._base_objs, i) / obj for i, obj in enumerate(x)])
        self._keep_trace.append(_mul_dummy)
        return _mul_dummy

    def __rdiv__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _mul_dummy = Dummy([wrap(x, i // self._op_duplicate) / obj for i, obj in enumerate(self._base_objs)])
        else:
            _mul_dummy = Dummy([obj / wrap(self._base_objs, i) for i, obj in enumerate(x)])
        self._keep_trace.append(_mul_dummy)
        return _mul_dummy

    def __idiv__(self, x):
        self.setDiv(x)
        return self

    def __pow__(self, x):
        return Pow(self, x)

    def __rpow__(self, x):
        return Pow(x, self)

    def __mod__(self, x):
        return Wrap(self, 0, x)

    def __neg__(self):
        if self._zeros is None:
            self._zeros = Sig(0)
        return self._zeros - self

    def __lt__(self, x):
        return self.__do_comp__(comp=x, mode="<")

    def __le__(self, x):
        return self.__do_comp__(comp=x, mode="<=")

    def __eq__(self, x):
        return self.__do_comp__(comp=x, mode="==")

    def __ne__(self, x):
        return self.__do_comp__(comp=x, mode="!=", default=True)

    def __gt__(self, x):
        return self.__do_comp__(comp=x, mode=">")

    def __ge__(self, x):
        return self.__do_comp__(comp=x, mode=">=")

    def __do_comp__(self, comp, mode, default=False):
        if comp is None:
            return default
        else:
            return Compare(self, comp=comp, mode=mode)

    def isPlaying(self, all=False):
        pyoArgsAssert(self, "B", all)
        if all:
            return [obj._getStream().isPlaying() for obj in self._base_objs]
        else:
            return self._base_objs[0]._getStream().isPlaying()

    def isOutputting(self, all=False):
        pyoArgsAssert(self, "B", all)
        if all:
            return [obj._getStream().isOutputting() for obj in self._base_objs]
        else:
            return self._base_objs[0]._getStream().isOutputting()

    def get(self, all=False):
        pyoArgsAssert(self, "B", all)
        if not all:
            return self._base_objs[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self._base_objs]

    def _init_play(self):
        temp = self._allow_auto_start
        self._allow_auto_start = False
        self.play()
        self._allow_auto_start = temp

    def play(self, dur=0, delay=0):
        pyoArgsAssert(self, "nn", dur, delay)
        dur, delay, lmax = convertArgsToLists(dur, delay)
        if not self.isPlaying() and not self.isOutputting():
            self._autoplay(dur, delay)
        if self._trig_objs is not None:
            if isinstance(self._trig_objs, list):
                for i in range(lmax):
                    for obj in self._trig_objs:
                        obj.play(wrap(dur, i), wrap(delay, i))
            else:
                self._trig_objs.play(dur, delay)
        if self._base_players is not None:
            [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_players)]
        if self._time_objs is not None:
            # We don't send 'dur' argument to time_stream to avoid a stop() call.
            [obj.play(0, wrap(delay, i)) for i, obj in enumerate(self._time_objs)]
        if hasattr(self, "_in_fader"):
            if 0 in dur:
                self._in_fader.play(0, min(delay))
            else:
                durtmp = 0.0
                for i in range(lmax):
                    if (wrap(dur, i) + wrap(delay, i)) > durtmp:
                        durtmp = wrap(dur, i) + wrap(delay, i)
                self._in_fader.play(durtmp, min(delay))
        if hasattr(self, "_in_fader2"):
            if 0 in dur:
                self._in_fader2.play(0, min(delay))
            else:
                durtmp = 0.0
                for i in range(lmax):
                    if (wrap(dur, i) + wrap(delay, i)) > durtmp:
                        durtmp = wrap(dur, i) + wrap(delay, i)
                self._in_fader2.play(durtmp, min(delay))

        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        pyoArgsAssert(self, "iInn", chnl, inc, dur, delay)
        dur, delay, lmax = convertArgsToLists(dur, delay)
        if not self.isPlaying() and not self.isOutputting():
            self._autoplay(dur, delay)
        if self._trig_objs is not None:
            if isinstance(self._trig_objs, list):
                for i in range(lmax):
                    for obj in self._trig_objs:
                        obj.play(wrap(dur, i), wrap(delay, i))
            else:
                self._trig_objs.play(dur, delay)
        if self._base_players is not None:
            [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_players)]
        if self._time_objs is not None:
            # We don't send 'dur' argument to time_stream to avoid a stop() call.
            [obj.play(0, wrap(delay, i)) for i, obj in enumerate(self._time_objs)]
        if hasattr(self, "_in_fader"):
            if 0 in dur:
                self._in_fader.play(0, min(delay))
            else:
                durtmp = 0.0
                for i in range(lmax):
                    if (wrap(dur, i) + wrap(delay, i)) > durtmp:
                        durtmp = wrap(dur, i) + wrap(delay, i)
                self._in_fader.play(durtmp, min(delay))
        if hasattr(self, "_in_fader2"):
            if 0 in dur:
                self._in_fader2.play(0, min(delay))
            else:
                durtmp = 0.0
                for i in range(lmax):
                    if (wrap(dur, i) + wrap(delay, i)) > durtmp:
                        durtmp = wrap(dur, i) + wrap(delay, i)
                self._in_fader2.play(durtmp, min(delay))

        if isinstance(chnl, list):
            [obj.out(wrap(chnl, i), wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:
                [obj.out(i * inc, wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(listscramble(self._base_objs))]
                # prevent normal order to happen.
                while [obj._getStream().getOutputChannel() for obj in self._base_objs] == list(range(len(self._base_objs))):
                    [obj.out(i * inc, wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(listscramble(self._base_objs))]
            else:
                [obj.out(chnl + i * inc, wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_objs)]
        return self

    def stop(self, wait=0):
        if self.isPlaying() or self.isOutputting():
            self._autostop(wait)
        if self._is_mul_attribute and not self._use_wait_time_on_stop:
            wait = 0
        if self._stop_delay != -1:
            wait = self._stop_delay

        if self._trig_objs is not None:
            if isinstance(self._trig_objs, list):
                [obj.stop(wait) for obj in self._trig_objs]
            else:
                self._trig_objs.stop(wait)
        if self._base_players is not None:
            [obj.stop(wait) for obj in self._base_players]
        if hasattr(self, "_in_fader"):
            self._in_fader.stop(wait)
        if hasattr(self, "_in_fader2"):
            self._in_fader2.stop(wait)
        # This is not good for TableRec objects, only for Looper.
        # It's moved locally to the Looper definition.
        # if self._time_objs is not None:
        #    [obj.stop() for obj in self._time_objs]
        [obj.stop(wait) for obj in self._base_objs]
        return self

    def mix(self, voices=1):
        return Mix(self, voices)

    def range(self, min, max):
        pyoArgsAssert(self, "nn", min, max)
        min, max, lmax = convertArgsToLists(min, max)
        if lmax > 1:
            mul = [(wrap(max, i) - wrap(min, i)) * 0.5 for i in range(lmax)]
            add = [(wrap(max, i) + wrap(min, i)) * 0.5 for i in range(lmax)]
        else:
            mul = (max[0] - min[0]) * 0.5
            add = (max[0] + min[0]) * 0.5
        self.setMul(mul)
        self.setAdd(add)
        return self

    def setMul(self, x):
        pyoArgsAssert(self, "O", x)
        self._mul = x
        x, _ = convertArgsToLists(x)
        [obj.setMul(wrap(x, i // self._op_duplicate)) for i, obj in enumerate(self._base_objs)]

    def setAdd(self, x):
        pyoArgsAssert(self, "O", x)
        self._add = x
        x, _ = convertArgsToLists(x)
        [obj.setAdd(wrap(x, i // self._op_duplicate)) for i, obj in enumerate(self._base_objs)]

    def setSub(self, x):
        pyoArgsAssert(self, "O", x)
        self._add = x
        x, _ = convertArgsToLists(x)
        [obj.setSub(wrap(x, i // self._op_duplicate)) for i, obj in enumerate(self._base_objs)]

    def setDiv(self, x):
        pyoArgsAssert(self, "O", x)
        self._mul = x
        x, _ = convertArgsToLists(x)
        [obj.setDiv(wrap(x, i // self._op_duplicate)) for i, obj in enumerate(self._base_objs)]

    def set(self, attr, value, port=0.025, callback=None):
        pyoArgsAssert(self, "SnnC", attr, value, port, callback)
        self._target_dict[attr] = value
        self._callback_dict[attr] = callback
        init = getattr(self, attr)
        if attr in self._signal_dict:
            if isinstance(self._signal_dict[attr], VarPort):
                if self._signal_dict[attr].isPlaying():
                    init = self._signal_dict[attr].get(True)
                    self._signal_dict[attr].stop()
        self._signal_dict[attr] = VarPort(value, port, init, self._reset_from_set, attr)
        setattr(self, attr, self._signal_dict[attr])

    def _reset_from_set(self, attr=None):
        if isinstance(getattr(self, attr), VarPort):
            setattr(self, attr, self._target_dict[attr])
            if self._callback_dict[attr] is not None:
                self._callback_dict[attr]()
                del self._callback_dict[attr]
        self._signal_dict[attr].stop()

    @property
    def mul(self):
        return self._mul

    @mul.setter
    def mul(self, x):
        self.setMul(x)

    @property
    def add(self):
        return self._add

    @add.setter
    def add(self, x):
        self.setAdd(x)


######################################################################
### PyoTableObject -> base class for pyo table objects
######################################################################
class PyoTableObject(PyoObjectBase):

    _STREAM_TYPE = "table"

    def __init__(self, size=0):
        PyoObjectBase.__init__(self)
        self._size = size

    def write(self, path, oneline=True):
        pyoArgsAssert(self, "SB", path, oneline)
        f = open(path, "w")
        if oneline:
            f.write(str([obj.getTable() for obj in self._base_objs]))
        else:
            text = "["
            for obj in self._base_objs:
                text += "["
                for i, val in enumerate(obj.getTable()):
                    if (i % 8) == 0:
                        text += "\n"
                    text += str(val) + ", "
                text += "]"
            text += "]"
            f.write(text)
        f.close()

    def read(self, path):
        pyoArgsAssert(self, "S", path)
        f = open(path, "r")
        f_list = eval(f.read())
        f_len = len(f_list)
        f.close()
        [obj.setData(f_list[i % f_len]) for i, obj in enumerate(self._base_objs)]
        # adjust the _size attribute.
        if f_len == 1:
            self._size = self._base_objs[0].getSize()
        else:
            self._size = [obj.getSize() for obj in self._base_objs]

    def getBuffer(self, chnl=0):
        if chnl < 0 or chnl >= len(self):
            print("getBuffer(chnl): `chnl` argument out-of-range...")
        else:
            return self._base_objs[chnl].getTableStream()

    def setSize(self, size):
        pyoArgsAssert(self, "I", size)
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]

    def getSize(self, all=False):
        pyoArgsAssert(self, "B", all)
        if all:
            return [obj.getSize() for obj in self._base_objs]
        else:
            if isinstance(self._size, list):
                return self._size[0]
            else:
                return self._size

    def put(self, value, pos=0):
        pyoArgsAssert(self, "NI", value, pos)
        [obj.put(value, pos) for obj in self._base_objs]

    def get(self, pos):
        pyoArgsAssert(self, "I", pos)
        values = [obj.get(pos) for obj in self._base_objs]
        if len(values) == 1:
            return values[0]
        else:
            return values

    def getTable(self, all=False):
        pyoArgsAssert(self, "B", all)
        if all:
            return [obj.getTable() for obj in self._base_objs]
        else:
            return self._base_objs[0].getTable()

    def normalize(self, level=0.99):
        [obj.normalize(level) for obj in self._base_objs]
        return self

    def reset(self):
        [obj.reset() for obj in self._base_objs]
        return self

    def removeDC(self):
        [obj.removeDC() for obj in self._base_objs]
        return self

    def reverse(self):
        [obj.reverse() for obj in self._base_objs]
        return self

    def invert(self):
        [obj.invert() for obj in self._base_objs]
        return self

    def rectify(self):
        [obj.rectify() for obj in self._base_objs]
        return self

    def pow(self, exp=10):
        pyoArgsAssert(self, "N", exp)
        [obj.pow(exp) for obj in self._base_objs]
        return self

    def bipolarGain(self, gpos=1, gneg=1):
        pyoArgsAssert(self, "NN", gpos, gneg)
        [obj.bipolarGain(gpos, gneg) for obj in self._base_objs]
        return self

    def lowpass(self, freq=1000):
        pyoArgsAssert(self, "N", freq)
        [obj.lowpass(freq) for obj in self._base_objs]
        return self

    def fadein(self, dur=0.1, shape=0):
        pyoArgsAssert(self, "NI", dur, shape)
        if shape < 0 or shape > 3:
            shape = 0
        [obj.fadein(dur, shape) for obj in self._base_objs]
        return self

    def fadeout(self, dur=0.1, shape=0):
        pyoArgsAssert(self, "NI", dur, shape)
        if shape < 0 or shape > 3:
            shape = 0
        [obj.fadeout(dur, shape) for obj in self._base_objs]
        return self

    def add(self, x):
        pyoArgsAssert(self, "T", x)
        if isinstance(x, list):
            if isinstance(x[0], list):
                [obj.add(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
            else:
                [obj.add(x) for obj in self._base_objs]
        else:
            x, _ = convertArgsToLists(x)
            [obj.add(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        return self

    def sub(self, x):
        pyoArgsAssert(self, "T", x)
        if isinstance(x, list):
            if isinstance(x[0], list):
                [obj.sub(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
            else:
                [obj.sub(x) for obj in self._base_objs]
        else:
            x, _ = convertArgsToLists(x)
            [obj.sub(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        return self

    def mul(self, x):
        pyoArgsAssert(self, "T", x)
        if isinstance(x, list):
            if isinstance(x[0], list):
                [obj.mul(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
            else:
                [obj.mul(x) for obj in self._base_objs]
        else:
            x, _ = convertArgsToLists(x)
            [obj.mul(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        return self

    def div(self, x):
        pyoArgsAssert(self, "T", x)
        if isinstance(x, list):
            if isinstance(x[0], list):
                [obj.div(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
            else:
                [obj.div(x) for obj in self._base_objs]
        else:
            x, _ = convertArgsToLists(x)
            [obj.div(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        return self

    def copyData(self, table, srcpos=0, destpos=0, length=-1):
        pyoArgsAssert(self, "tIII", table, srcpos, destpos, length)
        [obj.copyData(table[i], srcpos, destpos, length) for i, obj in enumerate(self._base_objs)]

    def rotate(self, pos):
        pyoArgsAssert(self, "I", pos)
        [obj.rotate(pos) for obj in self._base_objs]

    def copy(self):
        args = [getattr(self, att) for att in self.__dir__()]
        if self.__class__.__name__ == "SndTable":
            _size = self.getSize()
            if not isinstance(_size, list):
                _size = [_size]
            _chnls = len(self._base_objs)
            args[0] = None
            args.append(_chnls)
            newtable = getattr(current_pyo, self.__class__.__name__)(*args)
            baseobjs = newtable.getBaseObjects()
            [obj.setSize(_size[i % len(_size)]) for i, obj in enumerate(baseobjs)]
            [obj.copy(self[i]) for i, obj in enumerate(newtable.getBaseObjects())]
        else:
            newtable = getattr(current_pyo, self.__class__.__name__)(*args)
            [obj.copy(self[i]) for i, obj in enumerate(newtable.getBaseObjects())]
        return newtable

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)


######################################################################
### PyoMatrixObject -> base class for pyo matrix objects
######################################################################
class PyoMatrixObject(PyoObjectBase):

    _STREAM_TYPE = "matrix"

    def __init__(self):
        self._size = (0, 0)
        PyoObjectBase.__init__(self)

    def write(self, path):
        pyoArgsAssert(self, "S", path)
        f = open(path, "w")
        f.write(str([obj.getData() for obj in self._base_objs]))
        f.close()

    def read(self, path):
        pyoArgsAssert(self, "S", path)
        f = open(path, "r")
        f_list = eval(f.read())
        f_len = len(f_list)
        f.close()
        [obj.setData(f_list[i % f_len]) for i, obj in enumerate(self._base_objs)]

    def getSize(self):
        return self._size

    def normalize(self, level=0.99):
        [obj.normalize(level) for obj in self._base_objs]
        return self

    def blur(self):
        [obj.blur() for obj in self._base_objs]

    def boost(self, min=-1.0, max=1.0, boost=0.01):
        pyoArgsAssert(self, "NNN", min, max, boost)
        [obj.boost(min, max, boost) for obj in self._base_objs]

    def put(self, value, x=0, y=0):
        pyoArgsAssert(self, "NII", value, x, y)
        [obj.put(value, x, y) for obj in self._base_objs]

    def get(self, x=0, y=0):
        pyoArgsAssert(self, "II", x, y)
        values = [obj.get(x, y) for obj in self._base_objs]
        if len(values) == 1:
            return values[0]
        else:
            return values

    def getInterpolated(self, x=0.0, y=0.0):
        pyoArgsAssert(self, "FF", x, y)
        values = [obj.getInterpolated(x, y) for obj in self._base_objs]
        if len(values) == 1:
            return values[0]
        else:
            return values


######################################################################
### PyoObject -> base class for pyo phase vocoder objects
######################################################################
class PyoPVObject(PyoObjectBase):

    _STREAM_TYPE = "pvoc"

    def __init__(self):
        PyoObjectBase.__init__(self)
        self._target_dict = {}
        self._signal_dict = {}
        self._base_players = None

    def isPlaying(self, all=False):
        pyoArgsAssert(self, "B", all)
        if all:
            return [obj._getStream().isPlaying() for obj in self._base_objs]
        else:
            return self._base_objs[0]._getStream().isPlaying()

    def _init_play(self):
        temp = self._allow_auto_start
        self._allow_auto_start = False
        self.play()
        self._allow_auto_start = temp

    def play(self, dur=0, delay=0):
        pyoArgsAssert(self, "nn", dur, delay)
        dur, delay, _ = convertArgsToLists(dur, delay)
        if not self.isPlaying():
            self._autoplay(dur, delay)
        if self._trig_objs is not None:
            self._trig_objs.play(dur, delay)
        if self._base_players is not None:
            [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_players)]
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_objs)]
        return self

    def stop(self, wait=0):
        if self.isPlaying():
            self._autostop(wait)
        if self._stop_delay != -1:
            wait = self._stop_delay

        if self._trig_objs is not None:
            self._trig_objs.stop(wait)
        if self._base_players is not None:
            [obj.stop(wait) for obj in self._base_players]
        [obj.stop(wait) for obj in self._base_objs]
        return self

    def set(self, attr, value, port=0.025):
        pyoArgsAssert(self, "Snn", attr, value, port)
        self._target_dict[attr] = value
        init = getattr(self, attr)
        if attr in self._signal_dict:
            if isinstance(self._signal_dict[attr], VarPort):
                if self._signal_dict[attr].isPlaying():
                    init = self._signal_dict[attr].get(True)
                    self._signal_dict[attr].stop()
        self._signal_dict[attr] = VarPort(value, port, init, self._reset_from_set, attr)
        setattr(self, attr, self._signal_dict[attr])

    def _reset_from_set(self, attr=None):
        if isinstance(getattr(self, attr), VarPort):
            setattr(self, attr, self._target_dict[attr])
        self._signal_dict[attr].stop()


######################################################################
### Internal classes -> Used by pyo
######################################################################
class Mix(PyoObject):

    def __init__(self, input, voices=1, mul=1, add=0):
        pyoArgsAssert(self, "oIOO", input, voices, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        mul, add, lmax = convertArgsToLists(mul, add)
        if isinstance(input, list):
            input_objs = [obj for pyoObj in input for obj in pyoObj.getBaseObjects()]
            self._linked_objects = input
        else:
            input_objs = input.getBaseObjects()
            self._linked_objects = [input]
        input_len = len(input_objs)
        if voices < 1:
            voices = 1
            num = input_len
        elif voices > input_len and voices > lmax:
            num = voices
        elif lmax > input_len:
            num = lmax
        else:
            num = input_len
        sub_lists = []
        for i in range(voices):
            sub_lists.append([])
        for i in range(num):
            obj = input_objs[i % input_len]
            sub_lists[i % voices].append(obj)
        self._base_objs = [Mix_base(l, wrap(mul, i), wrap(add, i)) for i, l in enumerate(sub_lists)]
        self._init_play()


class Dummy(PyoObject):

    def __init__(self, objs_list):
        PyoObject.__init__(self)
        self._objs_list = objs_list
        tmp_list = []
        for x in objs_list:
            if isinstance(x, Dummy):
                tmp_list.extend(x.getBaseObjects())
            else:
                tmp_list.append(x)
        self._base_objs = tmp_list


class InputFader(PyoObject):

    def __init__(self, input):
        pyoArgsAssert(self, "o", input)
        PyoObject.__init__(self)
        self._input = input
        input, lmax = convertArgsToLists(input)
        self._base_objs = [InputFader_base(wrap(input, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        x, _ = convertArgsToLists(x)
        [obj.setInput(wrap(x, i), fadetime) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Sig(PyoObject):

    def __init__(self, value, mul=1, add=0):
        pyoArgsAssert(self, "OOO", value, mul, add)
        PyoObject.__init__(self, mul, add)
        self._value = value
        value, mul, add, lmax = convertArgsToLists(value, mul, add)
        self._base_objs = [Sig_base(wrap(value, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setValue(self, x):
        pyoArgsAssert(self, "O", x)
        self._value = x
        x, _ = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)


class VarPort(PyoObject):

    def __init__(self, value, time=0.025, init=0.0, function=None, arg=None, mul=1, add=0):
        pyoArgsAssert(self, "nnnczOO", value, time, init, function, arg, mul, add)
        PyoObject.__init__(self, mul, add)
        self._value = value
        self._time = time
        self._function = getWeakMethodRef(function)
        value, time, init, function, arg, mul, add, lmax = convertArgsToLists(
            value, time, init, function, arg, mul, add
        )
        self._base_objs = [
            VarPort_base(
                wrap(value, i),
                wrap(time, i),
                wrap(init, i),
                WeakMethod(wrap(function, i)),
                wrap(arg, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setValue(self, x):
        pyoArgsAssert(self, "n", x)
        self._value = x
        x, _ = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setTime(self, x):
        pyoArgsAssert(self, "n", x)
        self._time = x
        x, _ = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFunction(self, x):
        pyoArgsAssert(self, "c", x)
        self._function = getWeakMethodRef(x)
        x, _ = convertArgsToLists(x)
        [obj.setFunction(WeakMethod(wrap(x, i))) for i, obj in enumerate(self._base_objs)]

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)

    @property
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)

    @property
    def function(self):
        return self._function

    @function.setter
    def function(self, x):
        self.setFunction(x)


class Pow(PyoObject):

    def __init__(self, base=10, exponent=1, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", base, exponent, mul, add)
        PyoObject.__init__(self, mul, add)
        self._base = base
        self._exponent = exponent
        base, exponent, mul, add, lmax = convertArgsToLists(base, exponent, mul, add)
        self._base_objs = [
            M_Pow_base(wrap(base, i), wrap(exponent, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setBase(self, x):
        pyoArgsAssert(self, "O", x)
        self._base = x
        x, _ = convertArgsToLists(x)
        [obj.setBase(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExponent(self, x):
        pyoArgsAssert(self, "O", x)
        self._exponent = x
        x, _ = convertArgsToLists(x)
        [obj.setExponent(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def base(self):
        return self._base

    @base.setter
    def base(self, x):
        self.setBase(x)

    @property
    def exponent(self):
        return self._exponent

    @exponent.setter
    def exponent(self, x):
        self.setExponent(x)


class Wrap(PyoObject):

    def __init__(self, input, min=0.0, max=1.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, min, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [
            Wrap_base(wrap(in_fader, i), wrap(min, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, _ = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, _ = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class Compare(PyoObject):

    def __init__(self, input, comp, mode="<", mul=1, add=0):
        pyoArgsAssert(self, "oOsOO", input, comp, mode, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._comp = comp
        self._mode = mode
        self._in_fader = InputFader(input)
        self.comp_dict = {"<": 0, "<=": 1, ">": 2, ">=": 3, "==": 4, "!=": 5}
        in_fader, comp, mode, mul, add, lmax = convertArgsToLists(self._in_fader, comp, mode, mul, add)
        self._base_objs = [
            Compare_base(wrap(in_fader, i), wrap(comp, i), self.comp_dict[wrap(mode, i)], wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setComp(self, x):
        pyoArgsAssert(self, "O", x)
        self._comp = x
        x, _ = convertArgsToLists(x)
        [obj.setComp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMode(self, x):
        pyoArgsAssert(self, "s", x)
        self._mode = x
        x, _ = convertArgsToLists(x)
        [obj.setMode(self.comp_dict[wrap(x, i)]) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def comp(self):
        return self._comp

    @comp.setter
    def comp(self, x):
        self.setComp(x)

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, x):
        self.setMode(x)
