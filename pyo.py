from distutils.sysconfig import get_python_lib
import os

from pyolib.server import *
from pyolib.control import *
from pyolib.effects import *
from pyolib.input import *
from pyolib.midi import *
from pyolib.OSC import *
from pyolib.sfplayer import *
from pyolib.table import *
from pyolib.trigger import *
from pyolib.pattern import *
from pyolib.bandsplitter import *
from pyolib.hilbert import *
from pyolib.pan import *

OBJECTS_TREE = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 'pa_list_devices', 
                    'pm_count_devices', 'pm_list_devices', 'sndinfo']),
        'PyoObject': {'Filters': sorted(['Biquad', 'Port', 'Hilbert', 'Tone', 'DCBlock']),
                      'Open Sound Control': sorted(['OscReceive', 'OscSend']),
                      'Effects': sorted(['BandSplit',  'Clip', 'Compress', 'Delay', 'Disto', 'Freeverb','Pan', 'SPan', 'Waveguide']),
                      'Triggers': sorted(['Metro', 'TrigEnv', 'TrigRand', 'Select', 'Counter', 'TrigChoice', 'TrigFunc']),
                      'Generators': sorted(['Noise', 'Osc', 'Phasor', 'Sig', 'SigTo', 'Sine', 'Input', 'Fader']),
                      'MIDI': sorted(['Midictl', 'Notein']),
                      'Sound players': sorted(['SfMarkerShuffler', 'SfPlayer']),
                      'Patterns': sorted(['Pattern']),
                      'Table process': sorted(['TableRec', 'Pointer', 'Granulator']),
                      'Analysis': sorted(['Follower']),
                      'Internal objects': sorted(['Dummy', 'InputFader', 'Mix'])},
        'Map': {'SLMap': sorted(['SLMapFreq', 'SLMapMul', 'SLMapPhase', 'SLMapQ', 'SLMapDur', 'SLMapPan'])},
        'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable']),
        'Server': [], 
        'Stream': [], 
        'TableStream': [],
        'Clean_objects': []}

DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details', 'See also']

DEMOS_PATH = os.path.join(get_python_lib(), "pyodemos")
