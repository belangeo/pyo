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

import pyolib.analysis as analysis
from pyolib.analysis import *
from pyolib.controls import *
from pyolib.dynamics import *
from pyolib.effects import *
from pyolib.filters import *
from pyolib.generators import *
from pyolib.arithmetic import *
from pyolib.midi import *
from pyolib.opensoundcontrol import *
from pyolib.pan import *
from pyolib.pattern import *
from pyolib.randoms import *
from pyolib.server import *
from pyolib.sfplayer import *
from pyolib.tableprocess import *
from pyolib.tables import *
from pyolib.triggers import *
from pyolib.utils import *

OBJECTS_TREE = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 
                                    'pa_list_devices', 'pm_count_devices', 'pm_list_devices', 'sndinfo', 
                                    'midiToHz', 'sampsToSec', 'secToSamps', 'example', 'class_args']),
        'PyoObject': {'analysis': sorted(['Follower', 'ZCross']),
                      'controls': sorted(['Fader', 'Sig', 'SigTo', 'Adsr', 'Linseg']),
                      'dynamics': sorted(['Clip', 'Compress', 'Degrade']),
                      'effects': sorted(['Delay', 'Disto', 'Freeverb', 'Waveguide', 'Convolve', 'WGVerb']),
                      'filters': sorted(['Biquad', 'BandSplit', 'Port', 'Hilbert', 'Tone', 'DCBlock']),
                      'generators': sorted(['Noise', 'Phasor', 'Sine', 'Input', 'FM', 'SineLoop']),
                      'internal objects': sorted(['Dummy', 'InputFader', 'Mix']),
                      'midi': sorted(['Midictl', 'Notein']),
                      'opensoundcontrol': sorted(['OscReceive', 'OscSend']),
                      'pan': sorted(['Pan', 'SPan']),
                      'patterns': sorted(['Pattern', 'Score']),
                      'randoms': sorted(['Randi', 'Randh', 'Choice', 'RandInt', 'Xnoise', 'XnoiseMidi']),
                      'sfplayer': sorted(['SfMarkerShuffler', 'SfPlayer']),
                      'tableprocess': sorted(['TableRec', 'Osc', 'Pointer', 'Lookup', 'Granulator', 'Pulsar', 
                                            'TableRead', 'TableMorph']),
                      'triggers': sorted(['Metro', 'TrigEnv', 'TrigRand', 'Select', 'Counter', 'TrigChoice', 
                                        'TrigFunc', 'Thresh', 'Cloud', 'Trig', 'TrigXnoise', 'TrigXnoiseMidi']),
                      'utils': sorted(['Print']),
                      'arithmetic': sorted(['Sin', 'Cos', 'Tan'])},
        'Map': {'SLMap': sorted(['SLMapFreq', 'SLMapMul', 'SLMapPhase', 'SLMapQ', 'SLMapDur', 'SLMapPan'])},
        'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable', 'SawTable', 
                                'SquareTable', 'ChebyTable', 'CosTable', 'CurveTable']),
        'Server': [], 
        'Stream': [], 
        'TableStream': [],
        'Clean_objects': []}

DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details', 'See also']

SNDS_PATH = os.path.join(get_python_lib(), "pyolib", "snds")
DEMOS_PATH = SNDS_PATH
