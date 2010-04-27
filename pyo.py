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
import pyolib.sfplayer as sfplayer
from pyolib.sfplayer import *
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

OBJECTS_TREE = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 
                                    'pa_list_devices', 'pa_count_host_apis', 'pa_list_host_apis', 'pa_get_default_host_api', 
                                    'pm_count_devices', 'pm_list_devices', 'sndinfo', 
                                    'midiToHz', 'sampsToSec', 'secToSamps', 'example', 'class_args']),
        'PyoObject': {'analysis': sorted(['Follower', 'ZCross']),
                      'controls': sorted(['Fader', 'Sig', 'SigTo', 'Adsr', 'Linseg']),
                      'dynamics': sorted(['Clip', 'Compress', 'Degrade']),
                      'effects': sorted(['Delay', 'Disto', 'Freeverb', 'Waveguide', 'Convolve', 'WGVerb']),
                      'filters': sorted(['Biquad', 'BandSplit', 'Port', 'Hilbert', 'Tone', 'DCBlock', 'EQ']),
                      'generators': sorted(['Noise', 'Phasor', 'Sine', 'Input', 'FM', 'SineLoop']),
                      'internal objects': sorted(['Dummy', 'InputFader', 'Mix', 'VarPort']),
                      'midi': sorted(['Midictl', 'Notein']),
                      'opensoundcontrol': sorted(['OscReceive', 'OscSend']),
                      'pan': sorted(['Pan', 'SPan', 'Switch']),
                      'patterns': sorted(['Pattern', 'Score', 'CallAfter']),
                      'randoms': sorted(['Randi', 'Randh', 'Choice', 'RandInt', 'Xnoise', 'XnoiseMidi']),
                      'sfplayer': sorted(['SfMarkerShuffler', 'SfPlayer']),
                      'tableprocess': sorted(['TableRec', 'Osc', 'Pointer', 'Lookup', 'Granulator', 'Pulsar', 
                                            'TableRead', 'TableMorph']),
                      'matrixprocess': sorted(['MatrixRec', 'MatrixPointer']), 
                      'triggers': sorted(['Metro', 'TrigEnv', 'TrigRand', 'Select', 'Counter', 'TrigChoice', 
                                        'TrigFunc', 'Thresh', 'Cloud', 'Trig', 'TrigXnoise', 'TrigXnoiseMidi',
                                        'Change', 'TrigLinseg']),
                      'utils': sorted(['Clean_objects', 'Print', 'Snap', 'Interp', 'SampHold']),
                      'arithmetic': sorted(['Sin', 'Cos', 'Tan'])},
        'Map': {'SLMap': sorted(['SLMapFreq', 'SLMapMul', 'SLMapPhase', 'SLMapQ', 'SLMapDur', 'SLMapPan'])},
        'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable', 'SawTable', 
                                'SquareTable', 'ChebyTable', 'CosTable', 'CurveTable']),
        'PyoMatrixObject': sorted(['NewMatrix']),                        
        'Server': [], 
        'Stream': [], 
        'TableStream': []}

DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details', 'See also']

SNDS_PATH = os.path.join(get_python_lib(), "pyolib", "snds")
DEMOS_PATH = SNDS_PATH
