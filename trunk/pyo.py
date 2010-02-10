from distutils.sysconfig import get_python_lib
import os

from pyolib.analysis import *
from pyolib.controls import *
from pyolib.dynamics import *
from pyolib.effects import *
from pyolib.filters import *
from pyolib.generators import *
from pyolib.midi import *
from pyolib.OSC import *
from pyolib.pan import *
from pyolib.pattern import *
from pyolib.server import *
from pyolib.sfplayer import *
from pyolib.tableprocess import *
from pyolib.tables import *
from pyolib.triggers import *

OBJECTS_TREE = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 'pa_list_devices', 
                    'pm_count_devices', 'pm_list_devices', 'sndinfo']),
        'PyoObject': {'Filters': sorted(['Biquad', 'BandSplit', 'Port', 'Hilbert', 'Tone', 'DCBlock']),
                      'Open Sound Control': sorted(['OscReceive', 'OscSend']),
                      'Effects': sorted(['Delay', 'Disto', 'Freeverb', 'Waveguide']),
                      'Triggers': sorted(['Metro', 'TrigEnv', 'TrigRand', 'Select', 'Counter', 'TrigChoice', 'TrigFunc']),
                      'Generators': sorted(['Noise', 'Phasor', 'Sig', 'SigTo', 'Sine', 'Input', 'Fader']),
                      'MIDI': sorted(['Midictl', 'Notein']),
                      'Sound players': sorted(['SfMarkerShuffler', 'SfPlayer']),
                      'Patterns': sorted(['Pattern']),
                      'Panning': sorted(['Pan', 'SPan']),
                      'Table process': sorted(['TableRec', 'Osc', 'Pointer', 'Granulator', 'Pulsar', 'TableRead']),
                      'Analysis': sorted(['Follower']),
                      'Dynamics': sorted(['Clip', 'Compress']),
                      'Internal objects': sorted(['Dummy', 'InputFader', 'Mix'])},
        'Map': {'SLMap': sorted(['SLMapFreq', 'SLMapMul', 'SLMapPhase', 'SLMapQ', 'SLMapDur', 'SLMapPan'])},
        'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable']),
        'Server': [], 
        'Stream': [], 
        'TableStream': [],
        'Clean_objects': []}

DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details', 'See also']

DEMOS_PATH = os.path.join(get_python_lib(), "pyodemos")
