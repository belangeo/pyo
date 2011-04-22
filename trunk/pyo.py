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
from distutils.sysconfig import get_python_lib
import os

import sys
import __builtin__
from types import IntType, FloatType

# For Python 2.5-, this will enable the simliar property mechanism as in
# Python 2.6+/3.0+. The code is based on
# http://bruynooghe.blogspot.com/2008/04/xsetter-syntax-in-python-25.html
if sys.version_info[:2] <= (2, 5):
    class property(property):
        def __init__(self, fget, *args, **kwargs):
            self.__doc__ = fget.__doc__
            super(property, self).__init__(fget, *args, **kwargs)

        def setter(self, fset):
            cls_ns = sys._getframe(1).f_locals
            for k, v in cls_ns.iteritems():
                if v == self:
                    propname = k
                    break
            cls_ns[propname] = property(self.fget, fset, self.fdel, self.__doc__)
            return cls_ns[propname]

        def deleter(self, fdel):
            cls_ns = sys._getframe(1).f_locals
            for k, v in cls_ns.iteritems():
                if v == self:
                    propname = k
                    break
            cls_ns[propname] = property(self.fget, self.fset, fdel, self.__doc__)
            return cls_ns[propname]

    __builtin__.property = property

from pyolib._maps import *
import pyolib.analysis as analysis
from pyolib.analysis import *
import pyolib.controls as controls
from pyolib.controls import *
import pyolib.dynamics as dynamics
from pyolib.dynamics import *
import pyolib.effects as effects
from pyolib.effects import *
import pyolib.filters as filters
from pyolib.filters import *
import pyolib.generators as generators
from pyolib.generators import *
import pyolib.arithmetic as arithmetic
from pyolib.arithmetic import *
import pyolib.midi as midi
from pyolib.midi import *
import pyolib.opensoundcontrol as opensoundcontrol
from pyolib.opensoundcontrol import *
import pyolib.pan as pan
from pyolib.pan import *
import pyolib.pattern as pattern
from pyolib.pattern import *
import pyolib.randoms as randoms
from pyolib.randoms import *
from pyolib.server import *
import pyolib.players as players
from pyolib.players import *
import pyolib.tableprocess as tableprocess
from pyolib.tableprocess import *
import pyolib.matrixprocess as matrixprocess
from pyolib.matrixprocess import *
from pyolib.tables import *
from pyolib.matrix import *
import pyolib.triggers as triggers
from pyolib.triggers import *
import pyolib.utils as utils
from pyolib.utils import *
from pyolib._core import *

# Temporary objects, need to be coded in C
class Looper(PyoObject):
    """
    Crossfading looper.

    Looper reads audio from a PyoTableObject and plays it back in a loop with 
    user-defined pitch, start time, duration and crossfade time. It allows
    negative pitch value for reading backward.
    
    
    Parent class: PyoObject

    Parameters:

    table : PyoTableObject
        Table containing the waveform samples.
    pitch : float or PyoObject, optional
        Transposition factor. 1 is normal pitch, 0.5 is one octave lower, 
        2 is one octave higher and negative pitch signifies backwards playback.
        Defaults to 1.
    start : float or PyoObject, optional
        Starting point, in seconds, of the loop. Defaults to 0.
    dur : float or PyoObject, optional
        Duration, in seconds, of the loop. Defaults to 1.
    xfade : float {0 -> 100}, optional
        Pourcentage of the loop time used to crossfade readers.
        Available at intialization time only. Defaults to 20.
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setPitch(x) : Replace the `pitch` attribute.
    setStart(x) : Replace the `start` attribute.
    setDur(x) : Replace the `dur` attribute.
    
    Attributes:

    table : PyoTableObject. Table containing the waveform samples.
    pitch : float or PyoObject, Transposition factor.
    start : float or PyoObject, Loop start position in seconds.
    dur : float or PyoObject, Loop duration in seconds.

    See also: TableRead, Pointer

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> tab = SndTable(SNDS_PATH + '/transparent.aif')
    >>> pit = Choice(choice=[.5,.75,1,1.25,1.5], freq=[3,4])
    >>> start = Phasor(freq=.05, mul=tab.getDur())
    >>> dur = Choice(choice=[.0625,.125,.125,.25,.33], freq=4)
    >>> a = Looper(table=tab, pitch=pit, start=start, dur=dur, xfade=20).out()

    """
    def __init__(self, table, pitch=1, start=0, dur=1, xfade=20, mul=1, add=0):
        PyoObject.__init__(self)
        self._0to1Line = LinTable()
        self._table = table
        self._pitch = pitch
        self._start = start
        self._dur = dur
        self._xfade = xfade
        self._mul = mul
        self._add = add
        self._x1 = self._xfade * 49
        self._x2 = 10000 - self._x1
        self._invtabdur = 1. / self._table.getDur()
        self._env = LinTable([(0,0), (self._x1,1),(self._x2,1),(10000,0)], size=10000)

        dump1, dump2, dump3, dump4, dump5, dump6, lmax = convertArgsToLists(table, pitch, start, dur, mul, add)
        if type(pitch) in [IntType, FloatType] and lmax > 1:
            self._pitchStream = Sig([pitch for i in range(lmax)])
        elif isinstance(pitch, PyoObject) and len(pitch) < lmax:
            self._pitchStream = pitch.mix(lmax)
        else:
            self._pitchStream = Sig(pitch)

        if type(start) in [IntType, FloatType] and lmax > 1:
            self._startStream = Sig([start for i in range(lmax)])
        elif isinstance(start, PyoObject) and len(start) < lmax:
            self._startStream = start.mix(lmax)
        else:    
            self._startStream = Sig(start)
            
        self._durStream = Sig(self._pitchStream, dur)
        self._absDurStream = Abs(self._durStream)
        self._absPitchStream = Abs(self._pitchStream)
        self._durOnPitch = self._absDurStream / self._absPitchStream
        self._normStart = Sig(self._startStream, self._invtabdur)
        self._normDur = Sig(self._durStream, self._invtabdur)
        
        self._t1 = Trig().play()

        self._normstart1 = SampHold(self._normStart, self._t1, 1.0)
        self._normdur1 = SampHold(self._normDur, self._t1, 1.0)
        self._sampDur1 = SampHold(self._durOnPitch, self._t1, 1.0)
        self._ph1 = TrigEnv(self._t1, self._0to1Line, self._sampDur1)
        self._amp1 = Pointer(self._env, self._ph1)
        self._out1 = Pointer(self._table, (self._ph1*self._normdur1+self._normstart1), mul=self._amp1)
        self._comp1 = Thresh(self._amp1, 1.0, 1)

        self._normstart2 = SampHold(self._normStart, self._comp1, 1.0)
        self._normdur2 = SampHold(self._normDur, self._comp1, 1.0)
        self._sampDur2 = SampHold(self._durOnPitch, self._comp1, 1.0)
        self._ph2 = TrigEnv(self._comp1, self._0to1Line, self._sampDur2)
        self._amp2 = Pointer(self._env, self._ph2)
        self._out2 = Pointer(self._table, (self._ph2*self._normdur2+self._normstart2), mul=self._amp2)
        self._comp2 = Thresh(self._amp2, 1.0, 1)

        self._ph1.input = self._comp2
        self._normstart1.controlsig = self._comp2
        self._normdur1.controlsig = self._comp2
        self._sampDur1.controlsig = self._comp2
        
        self._out = Interp(self._out1, self._out2, mul=self._mul, add=self._add)
        self._base_objs = self._out.getBaseObjects()

    def __dir__(self):
        return ['table', 'pitch', 'start', 'dur', 'mul', 'add']

    def setTable(self, x):
        """
        Replace the `table` attribute.

        Parameters:

        x : PyoTableObject
            new `table` attribute.

        """
        self._table = x
        self._invtabdur = 1. / self._table.getDur()
        self._out1.table = self._table
        self._out2.table = self._table
        self._normstart.mul = self._invtabdur
        self._normdur.mul = self._invtabdur

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        Parameters:

        x : float or PyoObject
            new `pitch` attribute.

        """
        self._pitch = x
        if isinstance(x, PyoObject):
            l = len(self._pitchStream)
            if len(x) == l:
                self._pitchStream = x
            else:    
                self._pitchStream = x.mix(l)
            self._durStream.value = self._pitchStream
            self._absPitchStream.input = self._pitchStream
        else:
            self._pitchStream.value = x

    def setStart(self, x):
        """
        Replace the `start` attribute.

        Parameters:

        x : float or PyoObject
            new `start` attribute.

        """
        self._start = x
        if isinstance(x, PyoObject):
            l = len(self._pitchStream)
            if len(x) == l:
                self._startStream = x
            else:    
                self._startStream = x.mix(l)
            self._normStart.value = self._startStream
        else:
            self._startStream.value = x

    def setDur(self, x):
        """
        Replace the `dur` attribute.

        Parameters:

        x : float or PyoObject
            new `dur` attribute.

        """
        self._dur = x
        self._durStream.mul = x

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(-2., 2., "lin", "pitch", self._pitch),
                          SLMap(0, self._table.getDur(), "lin", "start", self._start),
                          SLMap(0.001, 2, "lin", "dur", self._dur),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)
            
    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples.""" 
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

    @property
    def pitch(self):
        """float or PyoObject. Transposition factor.""" 
        return self._pitch
    @pitch.setter
    def pitch(self, x): self.setPitch(x)

    @property
    def start(self):
        """float or PyoObject. Loop start position in seconds.""" 
        return self._start
    @start.setter
    def start(self, x): self.setStart(x)

    @property
    def dur(self):
        """float or PyoObject. Loop duration in seconds.""" 
        return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)

class PitchShift(PyoObject):
    """
    Frequency shifting using single sideband amplitude modulation.

    Shifting frequencies means that the input signal can be "detuned," 
    where the harmonic components of the signal are shifted out of 
    harmonic alignment with each other, e.g. a signal with harmonics at 
    100, 200, 300, 400 and 500 Hz, shifted up by 50 Hz, will have harmonics 
    at 150, 250, 350, 450, and 550 Hz.

    Parent class : PyoObject

    Parameters:

    input : PyoObject
        Input signal to process.
    shift : float or PyoObject, optional
        Amount of shifting in Hertz. Defaults to 100.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setShift(x) : Replace the `shift` attribute.

    Attributes:

    input : PyoObject. Input signal to process.
    shift : float or PyoObject. Amount of shifting in Hertz.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SineLoop(freq=300, feedback=.1, mul=.5)
    >>> lf1 = Sine(freq=.04, mul=10)
    >>> lf2 = Sine(freq=.05, mul=10)
    >>> b = PitchShift(a, shift=lf1, mul=.5).out()
    >>> c = PitchShift(a, shift=lf2, mul=.5).out(1)

    """
    def __init__(self, input, shift=100, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._shift = shift
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, shift, mul, add, lmax = convertArgsToLists(self._in_fader, shift, mul, add)

        self._hilb_objs = []
        self._sin_objs = []
        self._cos_objs = []
        self._mod_objs = []
        self._base_objs = []
        for i in range(lmax):
            self._hilb_objs.append(Hilbert(wrap(in_fader,i)))
            self._sin_objs.append(Sine(freq=wrap(shift,i), mul=.707))
            self._cos_objs.append(Sine(freq=wrap(shift,i), phase=0.25, mul=.707))
            self._mod_objs.append(Mix(self._hilb_objs[-1]['real'] * self._sin_objs[-1] + self._hilb_objs[-1]['imag'] * self._cos_objs[-1], 
                                      mul=wrap(mul,i), add=wrap(add,i)))
            self._base_objs.extend(self._mod_objs[-1].getBaseObjects())

    def __dir__(self):
        return ["input", "shift", "mul", "add"]    

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._hilb_objs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._sin_objs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._cos_objs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._mod_objs)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def stop(self):
        [obj.stop() for obj in self._hilb_objs]
        [obj.stop() for obj in self._sin_objs]
        [obj.stop() for obj in self._cos_objs]
        [obj.stop() for obj in self._mod_objs]
        [obj.stop() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._hilb_objs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._sin_objs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._cos_objs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._mod_objs)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setShift(self, x):
        """
        Replace the `shift` attribute.

        Parameters:

        x : float or PyoObject
            New `shift` attribute.

        """
        self._shift = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._sin_objs)]
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._cos_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(-2000., 2000., "lin", "shift", self._shift), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self): 
        """PyoObject. Input signal to pitch shift.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def shift(self): 
        """float or PyoObject. Amount of pitch shift in Hertz.""" 
        return self._shift
    @shift.setter
    def shift(self, x): self.setShift(x)


OBJECTS_TREE = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 'pm_get_input_devices',
                                    'pa_list_devices', 'pa_count_host_apis', 'pa_list_host_apis', 'pa_get_default_host_api', 
                                    'pm_count_devices', 'pm_list_devices', 'sndinfo', 'savefile', 'pa_get_output_devices', 
                                    'pa_get_input_devices', 'midiToHz', 'sampsToSec', 'secToSamps', 'example', 'class_args', 
                                    'pm_get_default_input', 'midiToTranspo']),
        'PyoObject': {'analysis': sorted(['Follower', 'ZCross']),
                      'controls': sorted(['Fader', 'Sig', 'SigTo', 'Adsr', 'Linseg', 'Expseg']),
                      'dynamics': sorted(['Clip', 'Compress', 'Degrade', 'Mirror', 'Wrap']),
                      'effects': sorted(['Delay', 'SDelay', 'Disto', 'Freeverb', 'Waveguide', 'Convolve', 'WGVerb', 
                                         'Harmonizer', 'Chorus', 'AllpassWG', 'PitchShift']),
                      'filters': sorted(['Biquad', 'BandSplit', 'Port', 'Hilbert', 'Tone', 'DCBlock', 'EQ', 'Allpass',
                                         'Allpass2', 'Phaser', 'Biquadx', 'IRWinSinc', 'IRAverage', 'IRPulse', 'IRFM']),
                      'generators': sorted(['Noise', 'Phasor', 'Sine', 'Input', 'FM', 'SineLoop', 'Blit', 'PinkNoise', 'CrossFM',
                                            'BrownNoise', 'Rossler', 'Lorenz', 'LFO']),
                      'internal objects': sorted(['Dummy', 'InputFader', 'Mix', 'VarPort']),
                      'midi': sorted(['Midictl', 'Notein', 'MidiAdsr']),
                      'opensoundcontrol': sorted(['OscReceive', 'OscSend']),
                      'pan': sorted(['Pan', 'SPan', 'Switch', 'Selector', 'Mixer']),
                      'patterns': sorted(['Pattern', 'Score', 'CallAfter']),
                      'randoms': sorted(['Randi', 'Randh', 'Choice', 'RandInt', 'Xnoise', 'XnoiseMidi']),
                      'players': sorted(['SfMarkerShuffler', 'SfPlayer', 'SfMarkerLooper']),
                      'tableprocess': sorted(['TableRec', 'Osc', 'Pointer', 'Lookup', 'Granulator', 'Pulsar', 
                                            'TableRead', 'TableMorph', 'Looper', 'TableIndex']),
                      'matrixprocess': sorted(['MatrixRec', 'MatrixPointer', 'MatrixMorph']), 
                      'triggers': sorted(['Metro', 'Beat', 'TrigEnv', 'TrigRand', 'TrigRandInt', 'Select', 'Counter', 'TrigChoice', 
                                        'TrigFunc', 'Thresh', 'Cloud', 'Trig', 'TrigXnoise', 'TrigXnoiseMidi',
                                        'Change', 'TrigLinseg', 'TrigExpseg', 'Percent', 'Seq']),
                      'utils': sorted(['Clean_objects', 'Print', 'Snap', 'Interp', 'SampHold', 'Compare', 'Record', 'Between', 'Denorm']),
                      'arithmetic': sorted(['Sin', 'Cos', 'Tan', 'Abs', 'Sqrt', 'Log', 'Log2', 'Log10', 'Pow'])},
        'Map': {'SLMap': sorted(['SLMapFreq', 'SLMapMul', 'SLMapPhase', 'SLMapQ', 'SLMapDur', 'SLMapPan'])},
        'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable', 'SawTable', 
                                'SquareTable', 'ChebyTable', 'CosTable', 'CurveTable', 'ExpTable', 'DataTable']),
        'PyoMatrixObject': sorted(['NewMatrix']),                        
        'Server': [], 
        'Stream': [], 
        'TableStream': []}

DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details', 'See also']

SNDS_PATH = os.path.join(get_python_lib(), "pyolib", "snds")
DEMOS_PATH = SNDS_PATH
