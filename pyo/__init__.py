"""
Copyright 2009-2019 Olivier Belanger

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
import os
import sys

if sys.platform.startswith("linux") and os.path.isdir("{}{}".format(os.path.dirname(__file__), ".libs")):
    from . import _linux_wheel_fix_symlinks

from .lib._maps import *
from .lib import analysis as analysis
from .lib.analysis import *
from .lib import controls as controls
from .lib.controls import *
from .lib import dynamics as dynamics
from .lib.dynamics import *
from .lib import effects as effects
from .lib.effects import *
from .lib import filters as filters
from .lib.filters import *
from .lib import generators as generators
from .lib.generators import *
from .lib import arithmetic as arithmetic
from .lib.arithmetic import *
from .lib import midi as midi
from .lib.midi import *
from .lib import opensndctrl as opensndctrl
from .lib.opensndctrl import *
from .lib import pan as pan
from .lib.pan import *
from .lib import pattern as pattern
from .lib.pattern import *
from .lib import randoms as randoms
from .lib.randoms import *
from .lib.server import *
from .lib.listener import *
from .lib import players as players
from .lib.players import *
from .lib import tableprocess as tableprocess
from .lib.tableprocess import *
from .lib import matrixprocess as matrixprocess
from .lib.matrixprocess import *
from .lib.tables import *
from .lib.matrix import *
from .lib import triggers as triggers
from .lib.triggers import *
from .lib import utils as utils
from .lib.utils import *
from .lib import expression as expression
from .lib.expression import *
from .lib import fourier as fourier
from .lib.fourier import *
from .lib import phasevoc as phasevoc
from .lib.phasevoc import *
from .lib._core import *
from .lib.wxgui import *
from .lib import wxgui as wxgui
from .lib.hrtf import *
from .lib import hrtf as hrtf
from .lib.events import *
from .lib import events as events
from .lib.mmlmusic import *
from .lib import mmlmusic as mmlmusic

if WITH_EXTERNALS:
    from .lib import external as external
    from .lib.external import *

OBJECTS_TREE = {
    "functions": sorted(
        [
            "pa_count_devices",
            "pa_get_default_input",
            "pa_get_default_output",
            "pm_get_input_devices",
            "pa_list_devices",
            "pa_count_host_apis",
            "pa_list_host_apis",
            "pa_get_default_host_api",
            "pa_get_default_devices_from_host",
            "pm_count_devices",
            "pm_list_devices",
            "sndinfo",
            "savefile",
            "pa_get_output_devices",
            "pa_get_input_devices",
            "midiToHz",
            "hzToMidi",
            "sampsToSec",
            "secToSamps",
            "example",
            "class_args",
            "pm_get_default_input",
            "pm_get_output_devices",
            "pm_get_default_output",
            "midiToTranspo",
            "getVersion",
            "reducePoints",
            "serverCreated",
            "serverBooted",
            "distanceToSegment",
            "rescale",
            "upsamp",
            "downsamp",
            "linToCosCurve",
            "convertStringToSysEncoding",
            "savefileFromTable",
            "pa_get_input_max_channels",
            "pa_get_output_max_channels",
            "pa_get_devices_infos",
            "pa_get_version",
            "pa_get_version_text",
            "floatmap",
            "getPrecision",
            "beatToDur",
        ]
    ),
    "PyoObjectBase": {
        "PyoMatrixObject": sorted(["NewMatrix"]),
        "PyoTableObject": sorted(
            [
                "LinTable",
                "NewTable",
                "SndTable",
                "HannTable",
                "HarmTable",
                "SawTable",
                "ParaTable",
                "LogTable",
                "CosLogTable",
                "SquareTable",
                "TriangleTable",
                "ChebyTable",
                "CosTable",
                "CurveTable",
                "ExpTable",
                "DataTable",
                "WinTable",
                "SincTable",
                "PartialTable",
                "AtanTable",
                "PadSynthTable",
                "SharedTable",
            ]
        ),
        "PyoPVObject": sorted(
            [
                "PVAnal",
                "PVSynth",
                "PVTranspose",
                "PVVerb",
                "PVGate",
                "PVAddSynth",
                "PVCross",
                "PVMult",
                "PVMorph",
                "PVFilter",
                "PVDelay",
                "PVBuffer",
                "PVShift",
                "PVAmpMod",
                "PVFreqMod",
                "PVBufLoops",
                "PVBufTabLoops",
                "PVMix",
            ]
        ),
        "PyoObject": {
            "analysis": sorted(
                [
                    "Follower",
                    "Follower2",
                    "ZCross",
                    "Yin",
                    "Centroid",
                    "AttackDetector",
                    "Scope",
                    "Spectrum",
                    "PeakAmp",
                    "RMS",
                ]
            ),
            "arithmetic": sorted(
                [
                    "Sin",
                    "Cos",
                    "Tan",
                    "Abs",
                    "Sqrt",
                    "Log",
                    "Log2",
                    "Log10",
                    "Pow",
                    "Atan2",
                    "Floor",
                    "Round",
                    "Ceil",
                    "Tanh",
                    "Exp",
                    "Div",
                    "Sub",
                ]
            ),
            "controls": sorted(["Fader", "Sig", "SigTo", "Adsr", "Linseg", "Expseg"]),
            "dynamics": sorted(
                ["Clip", "Compress", "Degrade", "Mirror", "Wrap", "Gate", "Balance", "Min", "Max", "Expand",]
            ),
            "effects": sorted(
                [
                    "Delay",
                    "SDelay",
                    "Disto",
                    "Freeverb",
                    "Waveguide",
                    "Convolve",
                    "WGVerb",
                    "SmoothDelay",
                    "Harmonizer",
                    "Chorus",
                    "AllpassWG",
                    "FreqShift",
                    "Vocoder",
                    "Delay1",
                    "STRev",
                ]
            ),
            "filters": sorted(
                [
                    "Biquad",
                    "BandSplit",
                    "Port",
                    "Hilbert",
                    "Tone",
                    "DCBlock",
                    "EQ",
                    "Allpass",
                    "Allpass2",
                    "Phaser",
                    "Biquadx",
                    "IRWinSinc",
                    "IRAverage",
                    "IRPulse",
                    "IRFM",
                    "FourBand",
                    "Biquada",
                    "Atone",
                    "SVF",
                    "SVF2",
                    "Average",
                    "Reson",
                    "Resonx",
                    "ButLP",
                    "ButHP",
                    "ButBP",
                    "ButBR",
                    "ComplexRes",
                    "MoogLP",
                    "MultiBand",
                ]
            ),
            "generators": sorted(
                [
                    "Noise",
                    "Phasor",
                    "Sine",
                    "Input",
                    "CrossFM",
                    "SineLoop",
                    "Blit",
                    "PinkNoise",
                    "FM",
                    "LFO",
                    "BrownNoise",
                    "Rossler",
                    "Lorenz",
                    "ChenLee",
                    "SumOsc",
                    "SuperSaw",
                    "RCOsc",
                    "FastSine",
                ]
            ),
            "internals": sorted(["Dummy", "InputFader", "Mix", "VarPort"]),
            "midi": sorted(
                [
                    "Midictl",
                    "CtlScan",
                    "CtlScan2",
                    "Notein",
                    "MidiAdsr",
                    "MidiDelAdsr",
                    "Bendin",
                    "Touchin",
                    "Programin",
                    "RawMidi",
                    "MidiLinseg",
                ]
            ),
            "opensndctrl": sorted(["OscReceive", "OscSend", "OscDataSend", "OscDataReceive", "OscListReceive",]),
            "pan": sorted(["Pan", "SPan", "Switch", "Selector", "Mixer", "VoiceManager", "HRTF", "Binaural",]),
            "pattern": sorted(["Pattern", "Score", "CallAfter"]),
            "randoms": sorted(
                [
                    "Randi",
                    "Randh",
                    "Choice",
                    "RandInt",
                    "Xnoise",
                    "XnoiseMidi",
                    "RandDur",
                    "XnoiseDur",
                    "Urn",
                    "LogiMap",
                ]
            ),
            "players": sorted(["SfMarkerShuffler", "SfPlayer", "SfMarkerLooper"]),
            "tableprocess": sorted(
                [
                    "TableRec",
                    "Osc",
                    "Pointer",
                    "Pointer2",
                    "Lookup",
                    "Granulator",
                    "Pulsar",
                    "OscLoop",
                    "Granule",
                    "TableRead",
                    "TableMorph",
                    "Looper",
                    "TableIndex",
                    "OscBank",
                    "OscTrig",
                    "TablePut",
                    "TableScale",
                    "Particle",
                    "Particle2",
                    "TableWrite",
                    "TableFill",
                    "TableScan",
                ]
            ),
            "matrixprocess": sorted(["MatrixRec", "MatrixPointer", "MatrixMorph", "MatrixRecLoop"]),
            "triggers": sorted(
                [
                    "Metro",
                    "Beat",
                    "TrigEnv",
                    "TrigRand",
                    "Trig",
                    "TrigRandInt",
                    "Select",
                    "Counter",
                    "TrigChoice",
                    "TrigFunc",
                    "Thresh",
                    "Cloud",
                    "TrigXnoise",
                    "TrigXnoiseMidi",
                    "Timer",
                    "Count",
                    "Change",
                    "TrigLinseg",
                    "TrigExpseg",
                    "Percent",
                    "Seq",
                    "TrigTableRec",
                    "Iter",
                    "NextTrig",
                    "TrigVal",
                    "Euclide",
                    "TrigBurst",
                ]
            ),
            "utils": sorted(
                [
                    "Clean_objects",
                    "Print",
                    "Snap",
                    "Interp",
                    "SampHold",
                    "Compare",
                    "Record",
                    "DBToA",
                    "AToDB",
                    "Between",
                    "Denorm",
                    "ControlRec",
                    "ControlRead",
                    "NoteinRec",
                    "NoteinRead",
                    "Scale",
                    "TrackHold",
                    "CentsToTranspo",
                    "TranspoToCents",
                    "MToF",
                    "FToM",
                    "MToT",
                    "Resample",
                ]
            ),
            "expression": sorted(["Expr"]),
            "mmlmusic": sorted(["MML"]),
            "fourier": sorted(
                ["FFT", "IFFT", "CarToPol", "PolToCar", "IFFTMatrix", "FrameDelta", "FrameAccum", "Vectral", "CvlVerb",]
            ),
            "events": sorted(
                [
                    "EventInstrument",
                    "DefaultInstrument",
                    "EventScale",
                    "EventGenerator",
                    "EventDummy",
                    "EventFilter",
                    "EventKey",
                    "EventSeq",
                    "EventSlide",
                    "EventIndex",
                    "EventMarkov",
                    "EventChoice",
                    "EventDrunk",
                    "EventNoise",
                    "EventCall",
                    "EventConditional",
                    "Events",
                ]
            ),
        },
    },
    "Map": {"SLMap": sorted(["SLMapFreq", "SLMapMul", "SLMapPhase", "SLMapQ", "SLMapDur", "SLMapPan"])},
    "Server": [],
    "MidiListener": [],
    "MidiDispatcher": [],
    "OscListener": [],
    "Stream": [],
    "TableStream": [],
    "PyoGui": [
        "PyoGuiControlSlider",
        "PyoGuiVuMeter",
        "PyoGuiGrapher",
        "PyoGuiMultiSlider",
        "PyoGuiSpectrum",
        "PyoGuiScope",
        "PyoGuiSndView",
        "PyoGuiKeyboard",
    ],
}

DOC_KEYWORDS = [
    "Attributes",
    "Examples",
    "Parameters",
    "Methods",
    "Notes",
    "Methods details",
    "See also",
    "Parentclass",
]


def getPyoKeywords():
    """
    Returns a list of every keywords (classes and functions) of pyo.

    >>> keywords = getPyoKeywords()

    """
    tree = OBJECTS_TREE
    _list = []
    for k1 in tree.keys():
        if type(tree[k1]) == type({}):
            for k2 in tree[k1].keys():
                if type(tree[k1][k2]) == type({}):
                    for k3 in tree[k1][k2].keys():
                        for val in tree[k1][k2][k3]:
                            _list.append(val)
                else:
                    for val in tree[k1][k2]:
                        _list.append(val)
        else:
            for val in tree[k1]:
                _list.append(val)
    _list.extend(
        ["PyoObjectBase", "PyoObject", "PyoTableObject", "PyoMatrixObject", "PyoPVObject",]
    )
    _list.extend(
        ["Server", "Map", "SLMap", "MidiListener", "MidiDispatcher", "OscListener", "Stream", "TableStream",]
    )
    return _list


def getPyoExamples(fullpath=False):
    """
    Returns a listing of the examples, as a dictionary, installed with pyo.

    :Args:

        fullpath: boolean
            If True, the full path of each file is returned. Otherwise, only the
            filenames are listed.

    >>> examples = getPyoExamples()

    """
    folder = "examples"
    filedir = os.path.dirname(os.path.abspath(__file__))
    subfolders = [f for f in os.listdir(os.path.join(filedir, folder)) if not f.startswith("__") and not f == "snds"]
    examples = {}
    for subfolder in sorted(subfolders):
        path = os.path.join(filedir, folder, subfolder)
        if fullpath:
            files = [os.path.join(path, f) for f in os.listdir(path) if not f.startswith("__")]
        else:
            files = [f for f in os.listdir(path) if not f.startswith("__")]
        examples[subfolder] = files
    return examples


OBJECTS_TREE["functions"] = sorted(OBJECTS_TREE["functions"] + ["getPyoKeywords", "getPyoExamples"])
