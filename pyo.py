"""
Copyright 2009-2015 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
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
import pyolib.opensndctrl as opensndctrl
from pyolib.opensndctrl import *
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
import pyolib.fourier as fourier
from pyolib.fourier import *
import pyolib.phasevoc as phasevoc
from pyolib.phasevoc import *
from pyolib._core import *
if WITH_EXTERNALS:
    import pyolib.external as external
    from pyolib.external import *

OBJECTS_TREE = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 'pm_get_input_devices',
                                    'pa_list_devices', 'pa_count_host_apis', 'pa_list_host_apis', 'pa_get_default_host_api',
                                    'pm_count_devices', 'pm_list_devices', 'sndinfo', 'savefile', 'pa_get_output_devices',
                                    'pa_get_input_devices', 'midiToHz', 'hzToMidi', 'sampsToSec', 'secToSamps', 'example', 'class_args',
                                    'pm_get_default_input', 'pm_get_output_devices', 'pm_get_default_output', 'midiToTranspo',
                                     'getVersion', 'reducePoints', 'serverCreated', 'serverBooted', 'distanceToSegment', 'rescale',
                                     'upsamp', 'downsamp', 'linToCosCurve', 'convertStringToSysEncoding', 'savefileFromTable',
                                    'pa_get_input_max_channels', 'pa_get_output_max_channels', 'pa_get_devices_infos', 'pa_get_version',
                                    'pa_get_version_text', 'floatmap']),
                'PyoObjectBase': {
                    'PyoMatrixObject': sorted(['NewMatrix']),
                    'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable', 'SawTable', 'ParaTable',
                                              'LogTable', 'CosLogTable', 'SquareTable', 'ChebyTable', 'CosTable', 'CurveTable', 'ExpTable',
                                              'DataTable', 'WinTable', 'SincTable', 'PartialTable', 'AtanTable']),
                    'PyoPVObject' : sorted(['PVAnal', 'PVSynth', 'PVTranspose', 'PVVerb', 'PVGate', 'PVAddSynth', 'PVCross', 'PVMult',
                                            'PVMorph', 'PVFilter', 'PVDelay', 'PVBuffer', 'PVShift', 'PVAmpMod', 'PVFreqMod', 'PVBufLoops',
                                            'PVBufTabLoops', 'PVMix']),
                    'PyoObject': {'analysis': sorted(['Follower', 'Follower2', 'ZCross', 'Yin', 'Centroid', 'AttackDetector', 'Scope',
                                                      'Spectrum', 'PeakAmp']),
                                  'arithmetic': sorted(['Sin', 'Cos', 'Tan', 'Abs', 'Sqrt', 'Log', 'Log2', 'Log10', 'Pow', 'Atan2', 'Floor',
                                                        'Round', 'Ceil', 'Tanh']),
                                  'controls': sorted(['Fader', 'Sig', 'SigTo', 'Adsr', 'Linseg', 'Expseg']),
                                  'dynamics': sorted(['Clip', 'Compress', 'Degrade', 'Mirror', 'Wrap', 'Gate', 'Balance', 'Min', 'Max']),
                                  'effects': sorted(['Delay', 'SDelay', 'Disto', 'Freeverb', 'Waveguide', 'Convolve', 'WGVerb', 'SmoothDelay',
                                                     'Harmonizer', 'Chorus', 'AllpassWG', 'FreqShift', 'Vocoder', 'Delay1', 'STRev']),
                                  'filters': sorted(['Biquad', 'BandSplit', 'Port', 'Hilbert', 'Tone', 'DCBlock', 'EQ', 'Allpass',
                                                     'Allpass2', 'Phaser', 'Biquadx', 'IRWinSinc', 'IRAverage', 'IRPulse', 'IRFM',
                                                     'FourBand', 'Biquada', 'Atone', 'SVF', 'Average', 'Reson', 'Resonx', 'ButLP',
                                                     'ButHP', 'ButBP', 'ButBR', 'ComplexRes']),
                                  'generators': sorted(['Noise', 'Phasor', 'Sine', 'Input', 'FM', 'SineLoop', 'Blit', 'PinkNoise', 'CrossFM',
                                                        'BrownNoise', 'Rossler', 'Lorenz', 'LFO', 'SumOsc', 'SuperSaw', 'RCOsc']),
                                  'internals': sorted(['Dummy', 'InputFader', 'Mix', 'VarPort']),
                                  'midi': sorted(['Midictl', 'CtlScan', 'CtlScan2', 'Notein', 'MidiAdsr', 'MidiDelAdsr', 'Bendin',
                                                  'Touchin', 'Programin']),
                                  'opensndctrl': sorted(['OscReceive', 'OscSend', 'OscDataSend', 'OscDataReceive', 'OscListReceive']),
                                  'pan': sorted(['Pan', 'SPan', 'Switch', 'Selector', 'Mixer', 'VoiceManager']),
                                  'pattern': sorted(['Pattern', 'Score', 'CallAfter']),
                                  'randoms': sorted(['Randi', 'Randh', 'Choice', 'RandInt', 'Xnoise', 'XnoiseMidi', 'RandDur', 'XnoiseDur', 'Urn']),
                                  'players': sorted(['SfMarkerShuffler', 'SfPlayer', 'SfMarkerLooper']),
                                  'tableprocess': sorted(['TableRec', 'Osc', 'Pointer', 'Pointer2', 'Lookup', 'Granulator', 'Pulsar', 'OscLoop',
                                                          'Granule', 'TableRead', 'TableMorph', 'Looper', 'TableIndex', 'OscBank', 'OscTrig',
                                                          'TablePut', 'TableScale', 'Particle', 'TableWrite']),
                                  'matrixprocess': sorted(['MatrixRec', 'MatrixPointer', 'MatrixMorph', 'MatrixRecLoop']),
                                  'triggers': sorted(['Metro', 'Beat', 'TrigEnv', 'TrigRand', 'TrigRandInt', 'Select', 'Counter', 'TrigChoice',
                                                    'TrigFunc', 'Thresh', 'Cloud', 'Trig', 'TrigXnoise', 'TrigXnoiseMidi', 'Timer', 'Count',
                                                    'Change', 'TrigLinseg', 'TrigExpseg', 'Percent', 'Seq', 'TrigTableRec', 'Iter', 'NextTrig',
                                                    'TrigVal', 'Euclide', 'TrigBurst']),
                                  'utils': sorted(['Clean_objects', 'Print', 'Snap', 'Interp', 'SampHold', 'Compare', 'Record', 'Between', 'Denorm',
                                                    'ControlRec', 'ControlRead', 'NoteinRec', 'NoteinRead', 'DBToA', 'AToDB', 'Scale', 'CentsToTranspo',
                                                    'TranspoToCents', 'MToF', 'FToM', 'MToT', 'TrackHold']),
                                  'fourier': sorted(['FFT', 'IFFT', 'CarToPol', 'PolToCar', 'FrameDelta', 'FrameAccum', 'Vectral', 'CvlVerb'])}},
        'Map': {'SLMap': sorted(['SLMapFreq', 'SLMapMul', 'SLMapPhase', 'SLMapQ', 'SLMapDur', 'SLMapPan'])},
        'Server': [],
        'Stream': [],
        'TableStream': []}

DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details', 'See also', 'Parentclass']