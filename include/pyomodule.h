/**************************************************************************
 * Copyright 2009-2015 Olivier Belanger                                   *
 *                                                                        *
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *
 *                                                                        *
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU Lesser General Public       *
 * License along with pyo.  If not, see <http://www.gnu.org/licenses/>.   *
 *************************************************************************/

#ifndef _PYOMODULE_H
#define _PYOMODULE_H

#include "Python.h"

#include <stdint.h>
#include <math.h>

#define PYO_VERSION "1.0.6"

#define MAX_NBR_SERVER 256
#define PYO_NUM_RND_OBJS 29

typedef struct Server Server;

static inline long
Pyo_CallMethod_AsLong(PyObject *obj, const char *method)
{
    PyObject *tmp = PyObject_CallMethod(obj, method, NULL);
    long value = -1;

    if (tmp == NULL)
    {
        PyErr_Print();
        return value;
    }

    value = PyLong_AsLong(tmp);
    Py_DECREF(tmp);
    return value;
}

static inline double
Pyo_CallMethod_AsDouble(PyObject *obj, const char *method)
{
    PyObject *tmp = PyObject_CallMethod(obj, method, NULL);
    double value = 0.0;

    if (tmp == NULL)
    {
        PyErr_Print();
        return value;
    }

    value = PyFloat_AsDouble(tmp);
    Py_DECREF(tmp);
    return value;
}

typedef enum
{
    PYO_RUNTIME_TYPE_SERVER = 0,
    PYO_RUNTIME_TYPE_STREAM,
    PYO_RUNTIME_TYPE_TRIGGER_STREAM,
    PYO_RUNTIME_TYPE_PV_STREAM,
    PYO_RUNTIME_TYPE_DUMMY,
    PYO_RUNTIME_TYPE_TRIGGER_DUMMY,
    PYO_RUNTIME_TYPE_TABLE_STREAM,
    PYO_RUNTIME_TYPE_MATRIX_STREAM,
    PYO_RUNTIME_TYPE_SIG,
    PYO_RUNTIME_TYPE_SIGTO,
    PYO_RUNTIME_TYPE_VARPORT,
    PYO_RUNTIME_TYPE_MIX,
    PYO_RUNTIME_TYPE_INPUTFADER,
    PYO_RUNTIME_TYPE_MIDI_LISTENER,
    PYO_RUNTIME_TYPE_MIDI_DISPATCHER,
    PYO_RUNTIME_TYPE_OSC_LISTENER,
    PYO_RUNTIME_TYPE_OSC_RECEIVE,
    PYO_RUNTIME_TYPE_OSC_RECEIVER,
    PYO_RUNTIME_TYPE_OSC_SEND,
    PYO_RUNTIME_TYPE_OSC_DATA_SEND,
    PYO_RUNTIME_TYPE_OSC_DATA_RECEIVE,
    PYO_RUNTIME_TYPE_OSC_LIST_RECEIVE,
    PYO_RUNTIME_TYPE_OSC_LIST_RECEIVER,
    PYO_RUNTIME_TYPE_PATTERN,
    PYO_RUNTIME_TYPE_SCORE,
    PYO_RUNTIME_TYPE_CALLAFTER,
    PYO_RUNTIME_TYPE_CALLALWAYS,
    PYO_RUNTIME_TYPE_M_SIN,
    PYO_RUNTIME_TYPE_M_COS,
    PYO_RUNTIME_TYPE_M_TAN,
    PYO_RUNTIME_TYPE_M_ABS,
    PYO_RUNTIME_TYPE_M_SQRT,
    PYO_RUNTIME_TYPE_M_LOG,
    PYO_RUNTIME_TYPE_M_LOG2,
    PYO_RUNTIME_TYPE_M_LOG10,
    PYO_RUNTIME_TYPE_M_POW,
    PYO_RUNTIME_TYPE_M_ATAN2,
    PYO_RUNTIME_TYPE_M_FLOOR,
    PYO_RUNTIME_TYPE_M_CEIL,
    PYO_RUNTIME_TYPE_M_ROUND,
    PYO_RUNTIME_TYPE_M_TANH,
    PYO_RUNTIME_TYPE_M_EXP,
    PYO_RUNTIME_TYPE_M_DIV,
    PYO_RUNTIME_TYPE_M_SUB,
    PYO_RUNTIME_TYPE_FOLLOWER,
    PYO_RUNTIME_TYPE_FOLLOWER2,
    PYO_RUNTIME_TYPE_ZCROSS,
    PYO_RUNTIME_TYPE_YIN,
    PYO_RUNTIME_TYPE_CENTROID,
    PYO_RUNTIME_TYPE_ATTACK_DETECTOR,
    PYO_RUNTIME_TYPE_SCOPE,
    PYO_RUNTIME_TYPE_PEAKAMP,
    PYO_RUNTIME_TYPE_RMS,
    PYO_RUNTIME_TYPE_CHORUS,
    PYO_RUNTIME_TYPE_BANDSPLITTER,
    PYO_RUNTIME_TYPE_BANDSPLIT,
    PYO_RUNTIME_TYPE_FOURBAND_MAIN,
    PYO_RUNTIME_TYPE_FOURBAND,
    PYO_RUNTIME_TYPE_MULTIBAND_MAIN,
    PYO_RUNTIME_TYPE_MULTIBAND,
    PYO_RUNTIME_TYPE_COMPRESS,
    PYO_RUNTIME_TYPE_GATE,
    PYO_RUNTIME_TYPE_BALANCE,
    PYO_RUNTIME_TYPE_EXPAND,
    PYO_RUNTIME_TYPE_CONVOLVE,
    PYO_RUNTIME_TYPE_IRWINSINC,
    PYO_RUNTIME_TYPE_IRPULSE,
    PYO_RUNTIME_TYPE_IRAVERAGE,
    PYO_RUNTIME_TYPE_IRFM,
    PYO_RUNTIME_TYPE_DISTO,
    PYO_RUNTIME_TYPE_CLIP,
    PYO_RUNTIME_TYPE_MIRROR,
    PYO_RUNTIME_TYPE_WRAP,
    PYO_RUNTIME_TYPE_DEGRADE,
    PYO_RUNTIME_TYPE_DELAY,
    PYO_RUNTIME_TYPE_SDELAY,
    PYO_RUNTIME_TYPE_WAVEGUIDE,
    PYO_RUNTIME_TYPE_ALLPASSWG,
    PYO_RUNTIME_TYPE_MIN,
    PYO_RUNTIME_TYPE_MAX,
    PYO_RUNTIME_TYPE_DELAY1,
    PYO_RUNTIME_TYPE_SMOOTHDELAY,
    PYO_RUNTIME_TYPE_EXPRER,
    PYO_RUNTIME_TYPE_EXPR,
    PYO_RUNTIME_TYPE_FADER,
    PYO_RUNTIME_TYPE_ADSR,
    PYO_RUNTIME_TYPE_LINSEG,
    PYO_RUNTIME_TYPE_EXPSEG,
    PYO_RUNTIME_TYPE_FFTMAIN,
    PYO_RUNTIME_TYPE_FFT,
    PYO_RUNTIME_TYPE_IFFT,
    PYO_RUNTIME_TYPE_CARTOPOL,
    PYO_RUNTIME_TYPE_POLTOCAR,
    PYO_RUNTIME_TYPE_FRAMEDELTAMAIN,
    PYO_RUNTIME_TYPE_FRAMEDELTA,
    PYO_RUNTIME_TYPE_FRAMEACCUMMAIN,
    PYO_RUNTIME_TYPE_FRAMEACCUM,
    PYO_RUNTIME_TYPE_VECTRALMAIN,
    PYO_RUNTIME_TYPE_VECTRAL,
    PYO_RUNTIME_TYPE_CVLVERB,
    PYO_RUNTIME_TYPE_SPECTRUM,
    PYO_RUNTIME_TYPE_IFFTMATRIX,
    PYO_RUNTIME_TYPE_BIQUAD,
    PYO_RUNTIME_TYPE_BIQUADX,
    PYO_RUNTIME_TYPE_BIQUADA,
    PYO_RUNTIME_TYPE_EQ,
    PYO_RUNTIME_TYPE_PORT,
    PYO_RUNTIME_TYPE_TONE,
    PYO_RUNTIME_TYPE_ATONE,
    PYO_RUNTIME_TYPE_DCBLOCK,
    PYO_RUNTIME_TYPE_ALLPASS,
    PYO_RUNTIME_TYPE_ALLPASS2,
    PYO_RUNTIME_TYPE_PHASER,
    PYO_RUNTIME_TYPE_VOCODER,
    PYO_RUNTIME_TYPE_SVF,
    PYO_RUNTIME_TYPE_SVF2,
    PYO_RUNTIME_TYPE_AVERAGE,
    PYO_RUNTIME_TYPE_RESON,
    PYO_RUNTIME_TYPE_RESONX,
    PYO_RUNTIME_TYPE_BUTLP,
    PYO_RUNTIME_TYPE_BUTHP,
    PYO_RUNTIME_TYPE_BUTBP,
    PYO_RUNTIME_TYPE_BUTBR,
    PYO_RUNTIME_TYPE_COMPLEXRES,
    PYO_RUNTIME_TYPE_MOOGLP,
    PYO_RUNTIME_TYPE_FREEVERB,
    PYO_RUNTIME_TYPE_GRANULATOR,
    PYO_RUNTIME_TYPE_LOOPER,
    PYO_RUNTIME_TYPE_LOOPERTIMESTREAM,
    PYO_RUNTIME_TYPE_GRANULE,
    PYO_RUNTIME_TYPE_MAINPARTICLE,
    PYO_RUNTIME_TYPE_PARTICLE,
    PYO_RUNTIME_TYPE_MAINPARTICLE2,
    PYO_RUNTIME_TYPE_PARTICLE2,
    PYO_RUNTIME_TYPE_HARMONIZER,
    PYO_RUNTIME_TYPE_HILBERTMAIN,
    PYO_RUNTIME_TYPE_HILBERT,
    PYO_RUNTIME_TYPE_INPUT,
    PYO_RUNTIME_TYPE_LFO,
    PYO_RUNTIME_TYPE_HRTFDATA,
    PYO_RUNTIME_TYPE_HRTFSPATTER,
    PYO_RUNTIME_TYPE_HRTF,
    PYO_RUNTIME_TYPE_BINAURALER,
    PYO_RUNTIME_TYPE_BINAURAL,
    PYO_RUNTIME_TYPE_MATRIXPOINTER,
    PYO_RUNTIME_TYPE_TRIG,
    PYO_RUNTIME_TYPE_METRO,
    PYO_RUNTIME_TYPE_SEQER,
    PYO_RUNTIME_TYPE_SEQ,
    PYO_RUNTIME_TYPE_CLOUDER,
    PYO_RUNTIME_TYPE_CLOUD,
    PYO_RUNTIME_TYPE_BEATER,
    PYO_RUNTIME_TYPE_BEAT,
    PYO_RUNTIME_TYPE_BEATTAPSTREAM,
    PYO_RUNTIME_TYPE_BEATAMPSTREAM,
    PYO_RUNTIME_TYPE_BEATDURSTREAM,
    PYO_RUNTIME_TYPE_BEATENDSTREAM,
    PYO_RUNTIME_TYPE_TRIGBURSTER,
    PYO_RUNTIME_TYPE_TRIGBURST,
    PYO_RUNTIME_TYPE_TRIGBURSTTAPSTREAM,
    PYO_RUNTIME_TYPE_TRIGBURSTAMPSTREAM,
    PYO_RUNTIME_TYPE_TRIGBURSTDURSTREAM,
    PYO_RUNTIME_TYPE_TRIGBURSTENDSTREAM,
    PYO_RUNTIME_TYPE_MIDICTL,
    PYO_RUNTIME_TYPE_CTLSCAN,
    PYO_RUNTIME_TYPE_CTLSCAN2,
    PYO_RUNTIME_TYPE_MIDINOTE,
    PYO_RUNTIME_TYPE_NOTEIN,
    PYO_RUNTIME_TYPE_NOTEINTRIG,
    PYO_RUNTIME_TYPE_BENDIN,
    PYO_RUNTIME_TYPE_TOUCHIN,
    PYO_RUNTIME_TYPE_PROGRAMIN,
    PYO_RUNTIME_TYPE_MIDIADSR,
    PYO_RUNTIME_TYPE_MIDIDELADSR,
    PYO_RUNTIME_TYPE_RAWMIDI,
    PYO_RUNTIME_TYPE_MIDILINSEG,
    PYO_RUNTIME_TYPE_MMLMAIN,
    PYO_RUNTIME_TYPE_MML,
    PYO_RUNTIME_TYPE_MMLFREQSTREAM,
    PYO_RUNTIME_TYPE_MMLAMPSTREAM,
    PYO_RUNTIME_TYPE_MMLDURSTREAM,
    PYO_RUNTIME_TYPE_MMLENDSTREAM,
    PYO_RUNTIME_TYPE_MMLXSTREAM,
    PYO_RUNTIME_TYPE_MMLYSTREAM,
    PYO_RUNTIME_TYPE_MMLZSTREAM,
    PYO_RUNTIME_TYPE_NOISE,
    PYO_RUNTIME_TYPE_PINKNOISE,
    PYO_RUNTIME_TYPE_BROWNNOISE,
    PYO_RUNTIME_TYPE_OSCBANK,
    PYO_RUNTIME_TYPE_SINE,
    PYO_RUNTIME_TYPE_FASTSINE,
    PYO_RUNTIME_TYPE_SINELOOP,
    PYO_RUNTIME_TYPE_FM,
    PYO_RUNTIME_TYPE_CROSSFM,
    PYO_RUNTIME_TYPE_BLIT,
    PYO_RUNTIME_TYPE_ROSSLER,
    PYO_RUNTIME_TYPE_ROSSLERALT,
    PYO_RUNTIME_TYPE_LORENZ,
    PYO_RUNTIME_TYPE_LORENZALT,
    PYO_RUNTIME_TYPE_CHENLEE,
    PYO_RUNTIME_TYPE_CHENLEEALT,
    PYO_RUNTIME_TYPE_PHASOR,
    PYO_RUNTIME_TYPE_POINTER,
    PYO_RUNTIME_TYPE_POINTER2,
    PYO_RUNTIME_TYPE_TABLEINDEX,
    PYO_RUNTIME_TYPE_LOOKUP,
    PYO_RUNTIME_TYPE_PULSAR,
    PYO_RUNTIME_TYPE_TABLEREAD,
    PYO_RUNTIME_TYPE_OSC,
    PYO_RUNTIME_TYPE_OSCLOOP,
    PYO_RUNTIME_TYPE_OSCTRIG,
    PYO_RUNTIME_TYPE_SUMOSC,
    PYO_RUNTIME_TYPE_SUPERSAW,
    PYO_RUNTIME_TYPE_RCOSC,
    PYO_RUNTIME_TYPE_TABLESCALE,
    PYO_RUNTIME_TYPE_TABLEFILL,
    PYO_RUNTIME_TYPE_TABLESCAN,
    PYO_RUNTIME_TYPE_PANNER,
    PYO_RUNTIME_TYPE_PAN,
    PYO_RUNTIME_TYPE_SPANNER,
    PYO_RUNTIME_TYPE_SPAN,
    PYO_RUNTIME_TYPE_SWITCHER,
    PYO_RUNTIME_TYPE_SWITCH,
    PYO_RUNTIME_TYPE_VOICEMANAGER,
    PYO_RUNTIME_TYPE_MIXER,
    PYO_RUNTIME_TYPE_MIXERVOICE,
    PYO_RUNTIME_TYPE_SELECTOR,
    PYO_RUNTIME_TYPE_RANDI,
    PYO_RUNTIME_TYPE_RANDH,
    PYO_RUNTIME_TYPE_RANDDUR,
    PYO_RUNTIME_TYPE_CHOICE,
    PYO_RUNTIME_TYPE_RANDINT,
    PYO_RUNTIME_TYPE_XNOISE,
    PYO_RUNTIME_TYPE_XNOISEMIDI,
    PYO_RUNTIME_TYPE_XNOISEDUR,
    PYO_RUNTIME_TYPE_URN,
    PYO_RUNTIME_TYPE_LOGIMAP,
    PYO_RUNTIME_TYPE_PVANAL,
    PYO_RUNTIME_TYPE_PVSYNTH,
    PYO_RUNTIME_TYPE_PVTRANSPOSE,
    PYO_RUNTIME_TYPE_PVVERB,
    PYO_RUNTIME_TYPE_PVGATE,
    PYO_RUNTIME_TYPE_PVADDSYNTH,
    PYO_RUNTIME_TYPE_PVCROSS,
    PYO_RUNTIME_TYPE_PVMULT,
    PYO_RUNTIME_TYPE_PVMORPH,
    PYO_RUNTIME_TYPE_PVFILTER,
    PYO_RUNTIME_TYPE_PVDELAY,
    PYO_RUNTIME_TYPE_PVBUFFER,
    PYO_RUNTIME_TYPE_PVSHIFT,
    PYO_RUNTIME_TYPE_PVAMPMOD,
    PYO_RUNTIME_TYPE_PVFREQMOD,
    PYO_RUNTIME_TYPE_PVBUFLOOPS,
    PYO_RUNTIME_TYPE_PVBUFTABLOOPS,
    PYO_RUNTIME_TYPE_PVMIX,
    PYO_RUNTIME_TYPE_RECORD,
    PYO_RUNTIME_TYPE_CONTROLREC,
    PYO_RUNTIME_TYPE_CONTROLREAD,
    PYO_RUNTIME_TYPE_NOTEINREC,
    PYO_RUNTIME_TYPE_NOTEINREAD,
    PYO_RUNTIME_TYPE_SELECT,
    PYO_RUNTIME_TYPE_CHANGE,
    PYO_RUNTIME_TYPE_SFPLAYER,
    PYO_RUNTIME_TYPE_SFPLAY,
    PYO_RUNTIME_TYPE_SFMARKERSHUFFLER,
    PYO_RUNTIME_TYPE_SFMARKERSHUFFLE,
    PYO_RUNTIME_TYPE_SFMARKERLOOPER,
    PYO_RUNTIME_TYPE_SFMARKERLOOP,
    PYO_RUNTIME_TYPE_NEXTTRIG,
    PYO_RUNTIME_TYPE_COUNTER,
    PYO_RUNTIME_TYPE_COUNTOBJ,
    PYO_RUNTIME_TYPE_THRESH,
    PYO_RUNTIME_TYPE_PERCENT,
    PYO_RUNTIME_TYPE_TIMER,
    PYO_RUNTIME_TYPE_DENORM,
    PYO_RUNTIME_TYPE_BETWEEN,
    PYO_RUNTIME_TYPE_WGVERB,
    PYO_RUNTIME_TYPE_COMPARE,
    PYO_RUNTIME_TYPE_TRIGRANDINT,
    PYO_RUNTIME_TYPE_TRIGVAL,
    PYO_RUNTIME_TYPE_TRIGRAND,
    PYO_RUNTIME_TYPE_TRIGCHOICE,
    PYO_RUNTIME_TYPE_ITER,
    PYO_RUNTIME_TYPE_TRIGENV,
    PYO_RUNTIME_TYPE_TRIGLINSEG,
    PYO_RUNTIME_TYPE_TRIGEXPSEG,
    PYO_RUNTIME_TYPE_TRIGFUNC,
    PYO_RUNTIME_TYPE_TRIGXNOISE,
    PYO_RUNTIME_TYPE_TRIGXNOISEMIDI,
    PYO_RUNTIME_TYPE_PRINT,
    PYO_RUNTIME_TYPE_SNAP,
    PYO_RUNTIME_TYPE_INTERP,
    PYO_RUNTIME_TYPE_SAMPHOLD,
    PYO_RUNTIME_TYPE_DBTOA,
    PYO_RUNTIME_TYPE_ATODB,
    PYO_RUNTIME_TYPE_SCALE,
    PYO_RUNTIME_TYPE_CENTSTOTRANSPO,
    PYO_RUNTIME_TYPE_TRANSPOTOCENTS,
    PYO_RUNTIME_TYPE_MTOF,
    PYO_RUNTIME_TYPE_FTOM,
    PYO_RUNTIME_TYPE_MTOT,
    PYO_RUNTIME_TYPE_TRACKHOLD,
    PYO_RUNTIME_TYPE_STREVERB,
    PYO_RUNTIME_TYPE_STREV,
    PYO_RUNTIME_TYPE_RESAMPLE,
    PYO_RUNTIME_TYPE_HARMTABLE,
    PYO_RUNTIME_TYPE_CHEBYTABLE,
    PYO_RUNTIME_TYPE_HANNTABLE,
    PYO_RUNTIME_TYPE_SINCTABLE,
    PYO_RUNTIME_TYPE_WINTABLE,
    PYO_RUNTIME_TYPE_PARATABLE,
    PYO_RUNTIME_TYPE_LINTABLE,
    PYO_RUNTIME_TYPE_LOGTABLE,
    PYO_RUNTIME_TYPE_COSLOGTABLE,
    PYO_RUNTIME_TYPE_COSTABLE,
    PYO_RUNTIME_TYPE_CURVETABLE,
    PYO_RUNTIME_TYPE_EXPTABLE,
    PYO_RUNTIME_TYPE_SNDTABLE,
    PYO_RUNTIME_TYPE_DATATABLE,
    PYO_RUNTIME_TYPE_NEWTABLE,
    PYO_RUNTIME_TYPE_TABLEREC,
    PYO_RUNTIME_TYPE_TABLEWRITE,
    PYO_RUNTIME_TYPE_TABLERECTIMESTREAM,
    PYO_RUNTIME_TYPE_TABLEMORPH,
    PYO_RUNTIME_TYPE_TRIGTABLEREC,
    PYO_RUNTIME_TYPE_TRIGTABLERECTIMESTREAM,
    PYO_RUNTIME_TYPE_TABLEPUT,
    PYO_RUNTIME_TYPE_NEWMATRIX,
    PYO_RUNTIME_TYPE_MATRIXREC,
    PYO_RUNTIME_TYPE_MATRIXRECLOOP,
    PYO_RUNTIME_TYPE_MATRIXMORPH,
    PYO_RUNTIME_TYPE_ATANTABLE,
    PYO_RUNTIME_TYPE_PADSYNTHTABLE,
    PYO_RUNTIME_TYPE_SHAREDTABLE,
    PYO_RUNTIME_TYPE_COUNT
} PyoRuntimeTypeId;

typedef struct
{
    Server *servers[MAX_NBR_SERVER];
    int current_server_id;
    int rnd_objs_count[PYO_NUM_RND_OBJS];
    unsigned int rand_seed;
    PyTypeObject *runtime_types[PYO_RUNTIME_TYPE_COUNT];
} PyoModuleState;

PyoModuleState * PyoState_Get(void);
void PyoState_Init(PyoModuleState *state);

PyTypeObject * PyoType_GetCurrent(PyoRuntimeTypeId id);
int PyoType_SetCurrent(PyoRuntimeTypeId id, PyTypeObject *type);
PyTypeObject * PyoCreateServerType(PyObject *module);
PyTypeObject * PyoCreateSigType(PyObject *module);
PyTypeObject * PyoCreateSigToType(PyObject *module);
PyTypeObject * PyoCreateVarPortType(PyObject *module);
PyTypeObject * PyoCreateMixType(PyObject *module);
PyTypeObject * PyoCreateInputFaderType(PyObject *module);
#ifdef USE_PORTMIDI
PyTypeObject * PyoCreateMidiListenerType(PyObject *module);
PyTypeObject * PyoCreateMidiDispatcherType(PyObject *module);
#endif
#ifdef USE_OSC
PyTypeObject * PyoCreateOscListenerType(PyObject *module);
PyTypeObject * PyoCreateOscReceiveType(PyObject *module);
PyTypeObject * PyoCreateOscReceiverType(PyObject *module);
PyTypeObject * PyoCreateOscSendType(PyObject *module);
PyTypeObject * PyoCreateOscDataSendType(PyObject *module);
PyTypeObject * PyoCreateOscDataReceiveType(PyObject *module);
PyTypeObject * PyoCreateOscListReceiveType(PyObject *module);
PyTypeObject * PyoCreateOscListReceiverType(PyObject *module);
#endif
PyTypeObject * PyoCreatePatternType(PyObject *module);
PyTypeObject * PyoCreateScoreType(PyObject *module);
PyTypeObject * PyoCreateCallAfterType(PyObject *module);
PyTypeObject * PyoCreateCallAlwaysType(PyObject *module);
PyTypeObject * PyoCreateMSinType(PyObject *module);
PyTypeObject * PyoCreateMCosType(PyObject *module);
PyTypeObject * PyoCreateMTanType(PyObject *module);
PyTypeObject * PyoCreateMAbsType(PyObject *module);
PyTypeObject * PyoCreateMSqrtType(PyObject *module);
PyTypeObject * PyoCreateMLogType(PyObject *module);
PyTypeObject * PyoCreateMLog2Type(PyObject *module);
PyTypeObject * PyoCreateMLog10Type(PyObject *module);
PyTypeObject * PyoCreateMPowType(PyObject *module);
PyTypeObject * PyoCreateMAtan2Type(PyObject *module);
PyTypeObject * PyoCreateMFloorType(PyObject *module);
PyTypeObject * PyoCreateMCeilType(PyObject *module);
PyTypeObject * PyoCreateMRoundType(PyObject *module);
PyTypeObject * PyoCreateMTanhType(PyObject *module);
PyTypeObject * PyoCreateMExpType(PyObject *module);
PyTypeObject * PyoCreateMDivType(PyObject *module);
PyTypeObject * PyoCreateMSubType(PyObject *module);
PyTypeObject * PyoCreateFollowerType(PyObject *module);
PyTypeObject * PyoCreateFollower2Type(PyObject *module);
PyTypeObject * PyoCreateZCrossType(PyObject *module);
PyTypeObject * PyoCreateYinType(PyObject *module);
PyTypeObject * PyoCreateCentroidType(PyObject *module);
PyTypeObject * PyoCreateAttackDetectorType(PyObject *module);
PyTypeObject * PyoCreateScopeType(PyObject *module);
PyTypeObject * PyoCreatePeakAmpType(PyObject *module);
PyTypeObject * PyoCreateRMSType(PyObject *module);
PyTypeObject * PyoCreateChorusType(PyObject *module);
PyTypeObject * PyoCreateBandSplitterType(PyObject *module);
PyTypeObject * PyoCreateBandSplitType(PyObject *module);
PyTypeObject * PyoCreateFourBandMainType(PyObject *module);
PyTypeObject * PyoCreateFourBandType(PyObject *module);
PyTypeObject * PyoCreateMultiBandMainType(PyObject *module);
PyTypeObject * PyoCreateMultiBandType(PyObject *module);
PyTypeObject * PyoCreateCompressType(PyObject *module);
PyTypeObject * PyoCreateGateType(PyObject *module);
PyTypeObject * PyoCreateBalanceType(PyObject *module);
PyTypeObject * PyoCreateExpandType(PyObject *module);
PyTypeObject * PyoCreateConvolveType(PyObject *module);
PyTypeObject * PyoCreateIRWinSincType(PyObject *module);
PyTypeObject * PyoCreateIRPulseType(PyObject *module);
PyTypeObject * PyoCreateIRAverageType(PyObject *module);
PyTypeObject * PyoCreateIRFMType(PyObject *module);
PyTypeObject * PyoCreateDistoType(PyObject *module);
PyTypeObject * PyoCreateClipType(PyObject *module);
PyTypeObject * PyoCreateMirrorType(PyObject *module);
PyTypeObject * PyoCreateWrapType(PyObject *module);
PyTypeObject * PyoCreateDegradeType(PyObject *module);
PyTypeObject * PyoCreateDelayType(PyObject *module);
PyTypeObject * PyoCreateSDelayType(PyObject *module);
PyTypeObject * PyoCreateWaveguideType(PyObject *module);
PyTypeObject * PyoCreateAllpassWGType(PyObject *module);
PyTypeObject * PyoCreateMinType(PyObject *module);
PyTypeObject * PyoCreateMaxType(PyObject *module);
PyTypeObject * PyoCreateDelay1Type(PyObject *module);
PyTypeObject * PyoCreateSmoothDelayType(PyObject *module);
PyTypeObject * PyoCreateExprerType(PyObject *module);
PyTypeObject * PyoCreateExprType(PyObject *module);
PyTypeObject * PyoCreateFaderType(PyObject *module);
PyTypeObject * PyoCreateAdsrType(PyObject *module);
PyTypeObject * PyoCreateLinsegType(PyObject *module);
PyTypeObject * PyoCreateExpsegType(PyObject *module);
PyTypeObject * PyoCreateFFTMainType(PyObject *module);
PyTypeObject * PyoCreateFFTType(PyObject *module);
PyTypeObject * PyoCreateIFFTType(PyObject *module);
PyTypeObject * PyoCreateCarToPolType(PyObject *module);
PyTypeObject * PyoCreatePolToCarType(PyObject *module);
PyTypeObject * PyoCreateFrameDeltaMainType(PyObject *module);
PyTypeObject * PyoCreateFrameDeltaType(PyObject *module);
PyTypeObject * PyoCreateFrameAccumMainType(PyObject *module);
PyTypeObject * PyoCreateFrameAccumType(PyObject *module);
PyTypeObject * PyoCreateVectralMainType(PyObject *module);
PyTypeObject * PyoCreateVectralType(PyObject *module);
PyTypeObject * PyoCreateCvlVerbType(PyObject *module);
PyTypeObject * PyoCreateSpectrumType(PyObject *module);
PyTypeObject * PyoCreateIFFTMatrixType(PyObject *module);
PyTypeObject * PyoCreateBiquadType(PyObject *module);
PyTypeObject * PyoCreateBiquadxType(PyObject *module);
PyTypeObject * PyoCreateBiquadaType(PyObject *module);
PyTypeObject * PyoCreateEQType(PyObject *module);
PyTypeObject * PyoCreatePortType(PyObject *module);
PyTypeObject * PyoCreateToneType(PyObject *module);
PyTypeObject * PyoCreateAtoneType(PyObject *module);
PyTypeObject * PyoCreateDCBlockType(PyObject *module);
PyTypeObject * PyoCreateAllpassType(PyObject *module);
PyTypeObject * PyoCreateAllpass2Type(PyObject *module);
PyTypeObject * PyoCreatePhaserType(PyObject *module);
PyTypeObject * PyoCreateVocoderType(PyObject *module);
PyTypeObject * PyoCreateSVFType(PyObject *module);
PyTypeObject * PyoCreateSVF2Type(PyObject *module);
PyTypeObject * PyoCreateAverageType(PyObject *module);
PyTypeObject * PyoCreateResonType(PyObject *module);
PyTypeObject * PyoCreateResonxType(PyObject *module);
PyTypeObject * PyoCreateButLPType(PyObject *module);
PyTypeObject * PyoCreateButHPType(PyObject *module);
PyTypeObject * PyoCreateButBPType(PyObject *module);
PyTypeObject * PyoCreateButBRType(PyObject *module);
PyTypeObject * PyoCreateComplexResType(PyObject *module);
PyTypeObject * PyoCreateMoogLPType(PyObject *module);
PyTypeObject * PyoCreateFreeverbType(PyObject *module);
PyTypeObject * PyoCreateGranulatorType(PyObject *module);
PyTypeObject * PyoCreateLooperType(PyObject *module);
PyTypeObject * PyoCreateLooperTimeStreamType(PyObject *module);
PyTypeObject * PyoCreateGranuleType(PyObject *module);
PyTypeObject * PyoCreateMainParticleType(PyObject *module);
PyTypeObject * PyoCreateParticleType(PyObject *module);
PyTypeObject * PyoCreateMainParticle2Type(PyObject *module);
PyTypeObject * PyoCreateParticle2Type(PyObject *module);
PyTypeObject * PyoCreateHarmonizerType(PyObject *module);
PyTypeObject * PyoCreateHilbertMainType(PyObject *module);
PyTypeObject * PyoCreateHilbertType(PyObject *module);
PyTypeObject * PyoCreateInputType(PyObject *module);
PyTypeObject * PyoCreateLFOType(PyObject *module);
PyTypeObject * PyoCreateHRTFDataType(PyObject *module);
PyTypeObject * PyoCreateHRTFSpatterType(PyObject *module);
PyTypeObject * PyoCreateHRTFType(PyObject *module);
PyTypeObject * PyoCreateBinauralerType(PyObject *module);
PyTypeObject * PyoCreateBinauralType(PyObject *module);
PyTypeObject * PyoCreateMatrixPointerType(PyObject *module);
PyTypeObject * PyoCreateTrigType(PyObject *module);
PyTypeObject * PyoCreateMetroType(PyObject *module);
PyTypeObject * PyoCreateSeqerType(PyObject *module);
PyTypeObject * PyoCreateSeqType(PyObject *module);
PyTypeObject * PyoCreateClouderType(PyObject *module);
PyTypeObject * PyoCreateCloudType(PyObject *module);
PyTypeObject * PyoCreateBeaterType(PyObject *module);
PyTypeObject * PyoCreateBeatType(PyObject *module);
PyTypeObject * PyoCreateBeatTapStreamType(PyObject *module);
PyTypeObject * PyoCreateBeatAmpStreamType(PyObject *module);
PyTypeObject * PyoCreateBeatDurStreamType(PyObject *module);
PyTypeObject * PyoCreateBeatEndStreamType(PyObject *module);
PyTypeObject * PyoCreateTrigBursterType(PyObject *module);
PyTypeObject * PyoCreateTrigBurstType(PyObject *module);
PyTypeObject * PyoCreateTrigBurstTapStreamType(PyObject *module);
PyTypeObject * PyoCreateTrigBurstAmpStreamType(PyObject *module);
PyTypeObject * PyoCreateTrigBurstDurStreamType(PyObject *module);
PyTypeObject * PyoCreateTrigBurstEndStreamType(PyObject *module);
PyTypeObject * PyoCreateMidictlType(PyObject *module);
PyTypeObject * PyoCreateCtlScanType(PyObject *module);
PyTypeObject * PyoCreateCtlScan2Type(PyObject *module);
PyTypeObject * PyoCreateMidiNoteType(PyObject *module);
PyTypeObject * PyoCreateNoteinType(PyObject *module);
PyTypeObject * PyoCreateNoteinTrigType(PyObject *module);
PyTypeObject * PyoCreateBendinType(PyObject *module);
PyTypeObject * PyoCreateTouchinType(PyObject *module);
PyTypeObject * PyoCreatePrograminType(PyObject *module);
PyTypeObject * PyoCreateMidiAdsrType(PyObject *module);
PyTypeObject * PyoCreateMidiDelAdsrType(PyObject *module);
PyTypeObject * PyoCreateRawMidiType(PyObject *module);
PyTypeObject * PyoCreateMidiLinsegType(PyObject *module);
PyTypeObject * PyoCreateMMLMainType(PyObject *module);
PyTypeObject * PyoCreateMMLType(PyObject *module);
PyTypeObject * PyoCreateMMLFreqStreamType(PyObject *module);
PyTypeObject * PyoCreateMMLAmpStreamType(PyObject *module);
PyTypeObject * PyoCreateMMLDurStreamType(PyObject *module);
PyTypeObject * PyoCreateMMLEndStreamType(PyObject *module);
PyTypeObject * PyoCreateMMLXStreamType(PyObject *module);
PyTypeObject * PyoCreateMMLYStreamType(PyObject *module);
PyTypeObject * PyoCreateMMLZStreamType(PyObject *module);
PyTypeObject * PyoCreateNoiseType(PyObject *module);
PyTypeObject * PyoCreatePinkNoiseType(PyObject *module);
PyTypeObject * PyoCreateBrownNoiseType(PyObject *module);
PyTypeObject * PyoCreateOscBankType(PyObject *module);
PyTypeObject * PyoCreateSineType(PyObject *module);
PyTypeObject * PyoCreateFastSineType(PyObject *module);
PyTypeObject * PyoCreateSineLoopType(PyObject *module);
PyTypeObject * PyoCreateFmType(PyObject *module);
PyTypeObject * PyoCreateCrossFmType(PyObject *module);
PyTypeObject * PyoCreateBlitType(PyObject *module);
PyTypeObject * PyoCreateRosslerType(PyObject *module);
PyTypeObject * PyoCreateRosslerAltType(PyObject *module);
PyTypeObject * PyoCreateLorenzType(PyObject *module);
PyTypeObject * PyoCreateLorenzAltType(PyObject *module);
PyTypeObject * PyoCreateChenLeeType(PyObject *module);
PyTypeObject * PyoCreateChenLeeAltType(PyObject *module);
PyTypeObject * PyoCreatePhasorType(PyObject *module);
PyTypeObject * PyoCreatePointerType(PyObject *module);
PyTypeObject * PyoCreatePointer2Type(PyObject *module);
PyTypeObject * PyoCreateTableIndexType(PyObject *module);
PyTypeObject * PyoCreateLookupType(PyObject *module);
PyTypeObject * PyoCreatePulsarType(PyObject *module);
PyTypeObject * PyoCreateTableReadType(PyObject *module);
PyTypeObject * PyoCreateOscType(PyObject *module);
PyTypeObject * PyoCreateOscLoopType(PyObject *module);
PyTypeObject * PyoCreateOscTrigType(PyObject *module);
PyTypeObject * PyoCreateSumOscType(PyObject *module);
PyTypeObject * PyoCreateSuperSawType(PyObject *module);
PyTypeObject * PyoCreateRCOscType(PyObject *module);
PyTypeObject * PyoCreateTableScaleType(PyObject *module);
PyTypeObject * PyoCreateTableFillType(PyObject *module);
PyTypeObject * PyoCreateTableScanType(PyObject *module);
PyTypeObject * PyoCreatePannerType(PyObject *module);
PyTypeObject * PyoCreatePanType(PyObject *module);
PyTypeObject * PyoCreateSPannerType(PyObject *module);
PyTypeObject * PyoCreateSPanType(PyObject *module);
PyTypeObject * PyoCreateSwitcherType(PyObject *module);
PyTypeObject * PyoCreateSwitchType(PyObject *module);
PyTypeObject * PyoCreateVoiceManagerType(PyObject *module);
PyTypeObject * PyoCreateMixerType(PyObject *module);
PyTypeObject * PyoCreateMixerVoiceType(PyObject *module);
PyTypeObject * PyoCreateSelectorType(PyObject *module);
PyTypeObject * PyoCreateRandiType(PyObject *module);
PyTypeObject * PyoCreateRandhType(PyObject *module);
PyTypeObject * PyoCreateRandDurType(PyObject *module);
PyTypeObject * PyoCreateChoiceType(PyObject *module);
PyTypeObject * PyoCreateRandIntType(PyObject *module);
PyTypeObject * PyoCreateXnoiseType(PyObject *module);
PyTypeObject * PyoCreateXnoiseMidiType(PyObject *module);
PyTypeObject * PyoCreateXnoiseDurType(PyObject *module);
PyTypeObject * PyoCreateUrnType(PyObject *module);
PyTypeObject * PyoCreateLogiMapType(PyObject *module);
PyTypeObject * PyoCreatePVAnalType(PyObject *module);
PyTypeObject * PyoCreatePVSynthType(PyObject *module);
PyTypeObject * PyoCreatePVTransposeType(PyObject *module);
PyTypeObject * PyoCreatePVVerbType(PyObject *module);
PyTypeObject * PyoCreatePVGateType(PyObject *module);
PyTypeObject * PyoCreatePVAddSynthType(PyObject *module);
PyTypeObject * PyoCreatePVCrossType(PyObject *module);
PyTypeObject * PyoCreatePVMultType(PyObject *module);
PyTypeObject * PyoCreatePVMorphType(PyObject *module);
PyTypeObject * PyoCreatePVFilterType(PyObject *module);
PyTypeObject * PyoCreatePVDelayType(PyObject *module);
PyTypeObject * PyoCreatePVBufferType(PyObject *module);
PyTypeObject * PyoCreatePVShiftType(PyObject *module);
PyTypeObject * PyoCreatePVAmpModType(PyObject *module);
PyTypeObject * PyoCreatePVFreqModType(PyObject *module);
PyTypeObject * PyoCreatePVBufLoopsType(PyObject *module);
PyTypeObject * PyoCreatePVBufTabLoopsType(PyObject *module);
PyTypeObject * PyoCreatePVMixType(PyObject *module);
PyTypeObject * PyoCreateRecordType(PyObject *module);
PyTypeObject * PyoCreateControlRecType(PyObject *module);
PyTypeObject * PyoCreateControlReadType(PyObject *module);
PyTypeObject * PyoCreateNoteinRecType(PyObject *module);
PyTypeObject * PyoCreateNoteinReadType(PyObject *module);
PyTypeObject * PyoCreateSelectType(PyObject *module);
PyTypeObject * PyoCreateChangeType(PyObject *module);
PyTypeObject * PyoCreateSfPlayerType(PyObject *module);
PyTypeObject * PyoCreateSfPlayType(PyObject *module);
PyTypeObject * PyoCreateSfMarkerShufflerType(PyObject *module);
PyTypeObject * PyoCreateSfMarkerShuffleType(PyObject *module);
PyTypeObject * PyoCreateSfMarkerLooperType(PyObject *module);
PyTypeObject * PyoCreateSfMarkerLoopType(PyObject *module);
PyTypeObject * PyoCreateNextTrigType(PyObject *module);
PyTypeObject * PyoCreateCounterType(PyObject *module);
PyTypeObject * PyoCreateCountType(PyObject *module);
PyTypeObject * PyoCreateThreshType(PyObject *module);
PyTypeObject * PyoCreatePercentType(PyObject *module);
PyTypeObject * PyoCreateTimerType(PyObject *module);
PyTypeObject * PyoCreateDenormType(PyObject *module);
PyTypeObject * PyoCreateBetweenType(PyObject *module);
PyTypeObject * PyoCreateWGVerbType(PyObject *module);
PyTypeObject * PyoCreateCompareType(PyObject *module);
PyTypeObject * PyoCreateTrigRandIntType(PyObject *module);
PyTypeObject * PyoCreateTrigValType(PyObject *module);
PyTypeObject * PyoCreateTrigRandType(PyObject *module);
PyTypeObject * PyoCreateTrigChoiceType(PyObject *module);
PyTypeObject * PyoCreateIterType(PyObject *module);
PyTypeObject * PyoCreateTrigEnvType(PyObject *module);
PyTypeObject * PyoCreateTrigLinsegType(PyObject *module);
PyTypeObject * PyoCreateTrigExpsegType(PyObject *module);
PyTypeObject * PyoCreateTrigFuncType(PyObject *module);
PyTypeObject * PyoCreateTrigXnoiseType(PyObject *module);
PyTypeObject * PyoCreateTrigXnoiseMidiType(PyObject *module);
PyTypeObject * PyoCreatePrintType(PyObject *module);
PyTypeObject * PyoCreateSnapType(PyObject *module);
PyTypeObject * PyoCreateInterpType(PyObject *module);
PyTypeObject * PyoCreateSampHoldType(PyObject *module);
PyTypeObject * PyoCreateDBToAType(PyObject *module);
PyTypeObject * PyoCreateAToDBType(PyObject *module);
PyTypeObject * PyoCreateScaleType(PyObject *module);
PyTypeObject * PyoCreateCentsToTranspoType(PyObject *module);
PyTypeObject * PyoCreateTranspoToCentsType(PyObject *module);
PyTypeObject * PyoCreateMToFType(PyObject *module);
PyTypeObject * PyoCreateFToMType(PyObject *module);
PyTypeObject * PyoCreateMToTType(PyObject *module);
PyTypeObject * PyoCreateTrackHoldType(PyObject *module);
PyTypeObject * PyoCreateSTReverbType(PyObject *module);
PyTypeObject * PyoCreateSTRevType(PyObject *module);
PyTypeObject * PyoCreateResampleType(PyObject *module);

PyTypeObject * PyoCreateHarmTableType(PyObject *module);
PyTypeObject * PyoCreateChebyTableType(PyObject *module);
PyTypeObject * PyoCreateHannTableType(PyObject *module);
PyTypeObject * PyoCreateSincTableType(PyObject *module);
PyTypeObject * PyoCreateWinTableType(PyObject *module);
PyTypeObject * PyoCreateParaTableType(PyObject *module);
PyTypeObject * PyoCreateLinTableType(PyObject *module);
PyTypeObject * PyoCreateLogTableType(PyObject *module);
PyTypeObject * PyoCreateCosLogTableType(PyObject *module);
PyTypeObject * PyoCreateCosTableType(PyObject *module);
PyTypeObject * PyoCreateCurveTableType(PyObject *module);
PyTypeObject * PyoCreateExpTableType(PyObject *module);
PyTypeObject * PyoCreateSndTableType(PyObject *module);
PyTypeObject * PyoCreateDataTableType(PyObject *module);
PyTypeObject * PyoCreateNewTableType(PyObject *module);
PyTypeObject * PyoCreateTableRecType(PyObject *module);
PyTypeObject * PyoCreateTableWriteType(PyObject *module);
PyTypeObject * PyoCreateTableRecTimeStreamType(PyObject *module);
PyTypeObject * PyoCreateTableMorphType(PyObject *module);
PyTypeObject * PyoCreateTrigTableRecType(PyObject *module);
PyTypeObject * PyoCreateTrigTableRecTimeStreamType(PyObject *module);
PyTypeObject * PyoCreateTablePutType(PyObject *module);
PyTypeObject * PyoCreateNewMatrixType(PyObject *module);
PyTypeObject * PyoCreateMatrixRecType(PyObject *module);
PyTypeObject * PyoCreateMatrixRecLoopType(PyObject *module);
PyTypeObject * PyoCreateMatrixMorphType(PyObject *module);
PyTypeObject * PyoCreateAtanTableType(PyObject *module);
PyTypeObject * PyoCreatePadSynthTableType(PyObject *module);
PyTypeObject * PyoCreateSharedTableType(PyObject *module);

typedef Py_ssize_t T_SIZE_T;

#ifndef __MYFLT_DEF
#define __MYFLT_DEF

#ifndef USE_DOUBLE
#define LIB_BASE_NAME "_pyo"
#define MYFLT float
#define FLOAT_VALUE f
#define TYPE_F "f"
#define TYPE_F_I "f|i"
#define TYPE_F_N "f|n"
#define TYPE__IF "|if"
#define TYPE__FF "|ff"
#define TYPE__IFF "|iff"
#define TYPE__FII "|fii"
#define TYPE__FIN "|fin"
#define TYPE_F_II "f|ii"
#define TYPE__FFF "|fff"
#define TYPE_F_FFF "f|fff"
#define TYPE_O_FIFFI "O|fiffi"
#define TYPE_O_F "O|f"
#define TYPE_O_O "O|O"
#define TYPE_O_FO "O|fO"
#define TYPE_O_OIF "O|Oif"
#define TYPE__OF "|Of"
#define TYPE_O_FOO "O|fOO"
#define TYPE_O_FIOO "O|fiOO"
#define TYPE_I_FFOO "i|ffOO"
#define TYPE_I_FFFOO "i|fffOO"
#define TYPE_I_FFFIOO "i|fffiOO"
#define TYPE_O_IF "O|if"
#define TYPE_O_IFS "O|ifs"
#define TYPE_S_IFF "s|iff"
#define TYPE_P_IFF "s#|iff"
#define TYPE_S_FIFF "s|fiff"
#define TYPE_P_FIFF "s#|fiff"
#define TYPE_S_FFIFF "s|ffiff"
#define TYPE_P_FFIFF "s#|ffiff"
#define TYPE_S__OIFI "s|Oifi"
#define TYPE_P__OIFI "s#|Oifi"
#define TYPE__FFFOO "|fffOO"
#define TYPE__FFFIOO "|fffiOO"
#define TYPE__FFFFFOO "|fffffOO"
#define TYPE__FFFFFF "|ffffff"
#define TYPE_O_FFFFOO "O|ffffOO"
#define TYPE_O_FFFFFOO "O|fffffOO"
#define TYPE_O_FFFFFFOO "O|ffffffOO"
#define TYPE_OO_F "OO|f"
#define TYPE_OO_FI "OO|fi"
#define TYPE_OO_IF "OO|if"
#define TYPE_OOO_F "OOO|f"
#define TYPE_OOO_FI "OOO|fi"
#define TYPE_OO_OF "OO|Of"
#define TYPE_OOO_FFFFII "OOO|ffffii"
#define TYPE_O_FFFFII "O|ffffii"
#define TYPE_F_O "f|O"
#define TYPE_F_OF "f|Of"
#define TYPE__OFFI "|Offi"
#define TYPE__OFFN "|Offn"
#define TYPE__OFII "|Ofii"
#define TYPE__OFIN "|Ofin"
#define TYPE__OFIOO "|OfiOO"
#define TYPE__OOFOO "|OOfOO"
#define TYPE__FIIOO "|fiiOO"
#define TYPE_O_OFOO "O|OfOO"
#define TYPE_O_OOOOFF "O|OOOOff"
#define TYPE_O_IFFO "O|iffO"
#define TYPE_O_OOIF "O|OOif"
#define TYPE_O_FFFFIOO "O|ffffiOO"
#define TYPE_OO_FOO "OO|fOO"
#define TYPE_OO_FFOO "OO|ffOO"
#define TYPE_O_IFIOO "O|ifiOO"
#define TYPE_O_OFOOOO "O|OfOOOO"
#define TYPE_O_OOFOO "O|OOfOO"
#define TYPE_O_OOFFOO "O|OOffOO"
#define TYPE_O_OOOFOO "O|OOOfOO"
#define TYPE_OO_OOOIFOO "OO|OOOifOO"
#define TYPE__FFFFIFI "|ffffifi"
#define TYPE__FFFFIFN "|ffffifn"

#define SF_WRITE sf_write_float
#define SF_READ sf_read_float

#define MYSQRT sqrtf
#define MYLOG logf
#define MYLOG2 log2f
#define MYLOG10 log10f
#define MYCOS cosf
#define MYSIN sinf
#define MYTAN tanf
#define MYPOW powf
#define MYFABS fabsf
#define MYFMOD fmodf
#define MYFLOOR floorf
#define MYCEIL ceilf
#define MYTANH tanhf
#define MYATAN atanf
#define MYATAN2 atan2f
#define MYEXP expf
#define MYROUND roundf

#else
#define LIB_BASE_NAME "_pyo64"
#define MYFLT double
#define FLOAT_VALUE d
#define TYPE_F "d"
#define TYPE_F_I "d|i"
#define TYPE_F_N "d|n"
#define TYPE__IF "|id"
#define TYPE__FF "|dd"
#define TYPE__IFF "|idd"
#define TYPE__FII "|dii"
#define TYPE__FIN "|din"
#define TYPE_F_II "d|ii"
#define TYPE__FFF "|ddd"
#define TYPE_F_FFF "d|ddd"
#define TYPE_O_FIFFI "O|diddi"
#define TYPE_O_F "O|d"
#define TYPE_O_O "O|O"
#define TYPE_O_FO "O|dO"
#define TYPE_O_OIF "O|Oid"
#define TYPE__OF "|Od"
#define TYPE_O_FOO "O|dOO"
#define TYPE_O_FIOO "O|diOO"
#define TYPE_I_FFOO "i|ddOO"
#define TYPE_I_FFFOO "i|dddOO"
#define TYPE_I_FFFIOO "i|dddiOO"
#define TYPE_O_IF "O|id"
#define TYPE_O_IFS "O|ids"
#define TYPE_S_IFF "s|idd"
#define TYPE_P_IFF "s#|idd"
#define TYPE_S_FIFF "s|didd"
#define TYPE_P_FIFF "s#|didd"
#define TYPE_S_FFIFF "s|ddidd"
#define TYPE_P_FFIFF "s#|ddidd"
#define TYPE_S__OIFI "s|Oidi"
#define TYPE_P__OIFI "s#|Oidi"
#define TYPE__FFFOO "|dddOO"
#define TYPE__FFFIOO "|dddiOO"
#define TYPE__FFFFFOO "|dddddOO"
#define TYPE__FFFFFF "|dddddd"
#define TYPE_O_FFFFOO "O|ddddOO"
#define TYPE_O_FFFFFOO "O|dddddOO"
#define TYPE_O_FFFFFFOO "O|ddddddOO"
#define TYPE_OO_F "OO|d"
#define TYPE_OO_FI "OO|di"
#define TYPE_OO_IF "OO|id"
#define TYPE_OOO_F "OOO|d"
#define TYPE_OOO_FI "OOO|di"
#define TYPE_OO_OF "OO|Od"
#define TYPE_OOO_FFFFII "OOO|ddddii"
#define TYPE_O_FFFFII "O|ddddii"
#define TYPE_F_O "d|O"
#define TYPE_F_OF "d|Od"
#define TYPE__OFFI "|Oddi"
#define TYPE__OFFN "|Oddn"
#define TYPE__OFII "|Odii"
#define TYPE__OFIN "|Odin"
#define TYPE__OFIOO "|OdiOO"
#define TYPE__OOFOO "|OOdOO"
#define TYPE__FIIOO "|diiOO"
#define TYPE_O_OFOO "O|OdOO"
#define TYPE_O_OOOOFF "O|OOOOdd"
#define TYPE_O_IFFO "O|iddO"
#define TYPE_O_OOIF "O|OOid"
#define TYPE_O_FFFFIOO "O|ddddiOO"
#define TYPE_OO_FOO "OO|dOO"
#define TYPE_OO_FFOO "OO|ddOO"
#define TYPE_O_IFIOO "O|idiOO"
#define TYPE_O_OFOOOO "O|OdOOOO"
#define TYPE_O_OOFOO "O|OOdOO"
#define TYPE_O_OOFFOO "O|OOddOO"
#define TYPE_O_OOOFOO "O|OOOdOO"
#define TYPE_OO_OOOIFOO "OO|OOOidOO"
#define TYPE__FFFFIFI "|ddddidi"
#define TYPE__FFFFIFN "|ddddidn"

#define SF_WRITE sf_write_double
#define SF_READ sf_read_double

#define MYSQRT sqrt
#define MYLOG log
#define MYLOG2 log2
#define MYLOG10 log10
#define MYCOS cos
#define MYSIN sin
#define MYTAN tan
#define MYPOW pow
#define MYFABS fabs
#define MYFMOD fmod
#define MYFLOOR floor
#define MYCEIL ceil
#define MYTANH tanh
#define MYATAN atan
#define MYATAN2 atan2
#define MYEXP exp
#define MYROUND round

#endif // USE_DOUBLE
#endif // __MYFLT_DEF

#ifdef COMPILE_EXTERNALS
#include "externalmodule.h"
#endif

/* Constants */
#define E M_E
#define PI M_PI
#define TWOPI (2 * M_PI)

#define PYO_RAND_MAX 4294967295u

/* random uniform (0.0 -> 1.0) */
#define RANDOM_UNIFORM (pyorand()/((MYFLT)(PYO_RAND_MAX)+1))

/* random objects identifier */
#define BEATER_ID 0
#define CLOUD_ID 1
#define RANDI_ID 2
#define RANDH_ID 3
#define CHOICE_ID 4
#define RANDINT_ID 5
#define RANDDUR_ID 6
#define XNOISE_ID 7
#define XNOISEMIDI_ID 8
#define TRIGRANDINT_ID 9
#define TRIGRAND_ID 10
#define TRIGCHOICE_ID 11
#define TRIGXNOISE_ID 12
#define TRIGXNOISEMIDI_ID 13
#define PERCENT_ID 14
#define DENORM_ID 15
#define NOISE_ID 16
#define PINKNOISE_ID 17
#define BROWNNOISE_ID 18
#define LFO_ID 19
#define OSCBANK_ID 20
#define SFMARKERSHUFFLER_ID 21
#define SFMARKERLOOPER_ID 22
#define GRANULATOR_ID 23
#define FREEVERB_ID 24
#define XNOISEDUR_ID 25
#define URN_ID 26
#define GRANULE_ID 27
#define MAINPARTICLE_ID 28
/* Do not forget to modify Server_generateSeed function */

/* object headers */
#define PYO_AUDIO_CALLBACK(func) ((void (*)(void *))(func))
#define PYO_MYFLT_CALLBACK(func) ((MYFLT (*)(void *))(func))

#define pyo_audio_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    Stream *stream; \
    void (*mode_func_ptr)(void *); \
    void (*proc_func_ptr)(void *); \
    void (*muladd_func_ptr)(void *); \
    PyObject *mul; \
    Stream *mul_stream; \
    PyObject *add; \
    Stream *add_stream; \
    int bufsize; \
    int nchnls; \
    int ichnls; \
    double sr; \
    MYFLT *data;

#define pyo_table_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    TableStream *tablestream; \
    T_SIZE_T size; \
    MYFLT *data;

#define pyo_matrix_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    MatrixStream *matrixstream; \
    int width; \
    int height; \
    MYFLT **data;

/* VISIT & CLEAR */
#define pyo_VISIT \
    if (self->server != NULL) \
        Py_VISIT(self->server); \
    Py_VISIT(self->mul); \
    Py_VISIT(self->add); \

#define pyo_table_VISIT \
    if (self->server != NULL) \
        Py_VISIT(self->server); \

#define pyo_matrix_VISIT \
    if (self->server != NULL) \
        Py_VISIT(self->server); \

#define pyo_CLEAR \
    if (self->server != NULL) { \
        Py_DECREF(self->server); \
        self->server = NULL; \
    } \
    Py_CLEAR(self->mul); \
    Py_CLEAR(self->add); \

#define pyo_table_CLEAR \
    if (self->server != NULL) { \
        Py_DECREF(self->server); \
        self->server = NULL; \
    } \

#define pyo_matrix_CLEAR \
    if (self->server != NULL) { \
        Py_DECREF(self->server); \
        self->server = NULL; \
    } \

#define pyo_GC_UNTRACK(self) \
    if (PyObject_GC_IsTracked((PyObject *)(self))) \
        PyObject_GC_UnTrack((PyObject *)(self)); \

#define pyo_DEALLOC \
    pyo_GC_UNTRACK(self); \
    if (self->server != NULL && self->stream != NULL) \
        Server_removeStream((Server *)self->server, Stream_getStreamId(self->stream)); \
    PyMem_RawFree(self->data); \

#define PYO_CALL_METHOD(obj, method, fmt, ...) \
    do { \
        PyObject *_tmp = PyObject_CallMethod((PyObject *)(obj), (method), (fmt), ##__VA_ARGS__); \
        if (_tmp == NULL) { \
            PyErr_Print(); \
        } \
        else { \
            Py_DECREF(_tmp); \
        } \
    } while (0)

#define PYO_CALL_METHOD_RET(obj, method, fmt, ...) \
    PyObject_CallMethod((PyObject *)(obj), (method), (fmt), ##__VA_ARGS__)

#define PYO_CALL_METHOD_OR_RETURN_NULL(self, obj, method, fmt, ...) \
    do { \
        PyObject *_tmp = PyObject_CallMethod((PyObject *)(obj), (method), (fmt), ##__VA_ARGS__); \
        if (_tmp == NULL) { \
            Py_DECREF(self); \
            return NULL; \
        } \
        Py_DECREF(_tmp); \
    } while (0)

#define PYO_CALL_METHOD_O_OR_RETURN_NULL(self, method, arg) \
    PYO_CALL_METHOD_OR_RETURN_NULL((self), (self), (method), "O", (arg))

#define PYO_ADD_STREAM_OR_RETURN_NULL(self) \
    PYO_CALL_METHOD_OR_RETURN_NULL((self), (self)->server, "addStream", "O", (self)->stream)

#define ASSERT_ARG_NOT_NULL \
    if (arg == NULL) { \
        Py_RETURN_NONE; \
    }

/* INIT INPUT STREAM */
/* self->input should never be a floating-point object. The assigment from PyFloat_FromDouble
   only serve to initialize the variable and increase its refcnt to 1, before replacing it with
   the actual audio object.
*/
#define INIT_INPUT_STREAM \
    if ( PyObject_HasAttrString((PyObject *)inputtmp, "server") == 0 ) { \
        PyErr_SetString(PyExc_TypeError, "\"input\" argument must be a PyoObject.\n"); \
        Py_DECREF(self); \
        return NULL; \
    } \
    self->input = inputtmp; \
    Py_INCREF(self->input); \
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL); \
    if (input_streamtmp == NULL) { \
        Py_DECREF(self); \
        return NULL; \
    } \
    self->input_stream = (Stream *)input_streamtmp;

#define INIT_INPUT_TRIGGER_STREAM \
    if ( PyObject_HasAttrString((PyObject *)inputtmp, "server") == 0 ) { \
        PyErr_SetString(PyExc_TypeError, "\"input\" argument must be a PyoObject.\n"); \
        Py_DECREF(self); \
        return NULL; \
    } \
    self->input = inputtmp; \
    Py_INCREF(self->input); \
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getTriggerStream", NULL); \
    if (input_streamtmp == NULL) { \
        Py_DECREF(self); \
        return NULL; \
    } \
    self->input_stream = (TriggerStream *)input_streamtmp;

#define INIT_INPUT_PV_STREAM \
    self->input = inputtmp; \
    Py_INCREF(self->input); \
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL); \
    if (input_streamtmp == NULL) { \
        Py_DECREF(self); \
        return NULL; \
    } \
    self->input_stream = (PVStream *)input_streamtmp;

#define INIT_INPUT2_PV_STREAM \
    self->input2 = input2tmp; \
    Py_INCREF(self->input2); \
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getPVStream", NULL); \
    if (input2_streamtmp == NULL) { \
        Py_DECREF(self); \
        return NULL; \
    } \
    self->input2_stream = (PVStream *)input2_streamtmp;

/* Init Server & Stream */
#define INIT_OBJECT_COMMON \
    self->server = PyServer_get_server(); \
    Py_INCREF(self->server); \
    self->mul = PyFloat_FromDouble(1); \
    self->add = PyFloat_FromDouble(0); \
    PyObject *bufobj = PyObject_CallMethod(self->server, "getBufferSize", NULL); \
    self->bufsize = PyLong_AsLong(bufobj); \
    Py_DECREF(bufobj); \
    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL); \
    self->sr = PyFloat_AsDouble(srobj); \
    Py_DECREF(srobj); \
    PyObject *nchobj = PyObject_CallMethod(self->server, "getNchnls", NULL); \
    self->nchnls = PyLong_AsLong(nchobj); \
    Py_DECREF(nchobj); \
    PyObject *ichobj = PyObject_CallMethod(self->server, "getIchnls", NULL); \
    self->ichnls = PyLong_AsLong(ichobj); \
    Py_DECREF(ichobj); \
    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->bufsize) * sizeof(MYFLT)); \
    for (i=0; i<self->bufsize; i++) \
        self->data[i] = 0.0; \
    MAKE_NEW_STREAM(self->stream, PyoType_GetCurrent(PYO_RUNTIME_TYPE_STREAM), NULL); \
    Stream_setStreamObject(self->stream, (PyObject *)self); \
    Stream_setStreamId(self->stream, Server_getNewStreamId((Server *)self->server)); \
    Stream_setBufferSize(self->stream, self->bufsize); \
    Stream_setData(self->stream, self->data); \
    Py_INCREF(self->stream);

#define SET_INTERP_POINTER \
    if (self->interp == 0) \
        self->interp = 2; \
    if (self->interp == 1) \
        self->interp_func_ptr = nointerp; \
    else if (self->interp == 2) \
        self->interp_func_ptr = linear; \
    else if (self->interp == 3) \
        self->interp_func_ptr = cosine; \
    else if (self->interp == 4) \
        self->interp_func_ptr = cubic;

/* Set data */
#define SET_TABLE_DATA \
    T_SIZE_T i; \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "The data must be a list of floats."); \
        return PyLong_FromLong(-1); \
    } \
    self->size = (T_SIZE_T)PyList_Size(arg); \
    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size+1) * sizeof(MYFLT)); \
    TableStream_setSize(self->tablestream, self->size+1); \
 \
    for (i=0; i<(self->size); i++) { \
        self->data[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
    } \
    self->data[self->size] = self->data[0]; \
    TableStream_setData(self->tablestream, self->data); \
 \
    Py_RETURN_NONE; \

#define SET_MATRIX_DATA \
    int i, j; \
    PyObject *innerlist; \
 \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "The data must be a list of list of floats."); \
        return PyLong_FromLong(-1); \
    } \
    for (i = 0; i < (self->height + 1); i++) { \
        PyMem_RawFree(self->data[i]); \
    } \
    self->height = PyList_Size(arg); \
    self->width = PyList_Size(PyList_GetItem(arg, 0)); \
    self->data = (MYFLT **)PyMem_RawRealloc(self->data, (self->height + 1) * sizeof(MYFLT *)); \
    for (i=0; i<(self->height+1); i++) { \
        self->data[i] = (MYFLT *)PyMem_RawMalloc((self->width + 1) * sizeof(MYFLT)); \
    } \
    MatrixStream_setWidth(self->matrixstream, self->width); \
    MatrixStream_setHeight(self->matrixstream, self->height); \
 \
    for (i=0; i<self->height; i++) { \
        innerlist = PyList_GetItem(arg, i); \
        for (j=0; j<self->width; j++) { \
            self->data[i][j] = PyFloat_AsDouble(PyList_GET_ITEM(innerlist, j)); \
        } \
    } \
 \
    MatrixStream_setData(self->matrixstream, self->data); \
 \
    Py_RETURN_NONE; \

#define COPY \
    T_SIZE_T i; \
    PyObject *table = PyObject_CallMethod((PyObject *)arg, "getTableStream", ""); \
    MYFLT *tab = TableStream_getData((TableStream *)table); \
    for (i=0; i<self->size; i++) { \
        self->data[i] = tab[i]; \
    } \
    self->data[self->size] = self->data[0]; \
    Py_DECREF(table); \
    Py_RETURN_NONE; \

#define TABLE_ADD \
    T_SIZE_T i, tabsize; \
    MYFLT x = 0.0; \
    MYFLT *list = NULL; \
    PyObject *table = NULL; \
    if (PyNumber_Check(arg)) { \
        x = PyFloat_AsDouble(arg); \
        for (i=0; i<self->size; i++) { \
            self->data[i] += x; \
        } \
    } \
    else if ( PyObject_HasAttrString((PyObject *)arg, "getTableStream") == 1 ) { \
        Py_XDECREF(table); \
        table = PyObject_CallMethod((PyObject *)arg, "getTableStream", ""); \
        list = TableStream_getData((TableStream *)table); \
        tabsize = TableStream_getSize((TableStream *)table); \
        Py_DECREF(table); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] += list[i]; \
        } \
    } \
    else if (PyList_Check(arg)) { \
        tabsize = (T_SIZE_T)PyList_Size(arg); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] += PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
        } \
    } \
 \
    self->data[self->size] = self->data[0]; \
 \
    Py_RETURN_NONE; \

#define TABLE_SUB \
    T_SIZE_T i, tabsize; \
    MYFLT x = 0.0; \
    MYFLT *list = NULL; \
    PyObject *table = NULL; \
    if (PyNumber_Check(arg)) { \
        x = PyFloat_AsDouble(arg); \
        for (i=0; i<self->size; i++) { \
            self->data[i] -= x; \
        } \
    } \
    else if ( PyObject_HasAttrString((PyObject *)arg, "getTableStream") == 1 ) { \
        Py_XDECREF(table); \
        table = PyObject_CallMethod((PyObject *)arg, "getTableStream", ""); \
        list = TableStream_getData((TableStream *)table); \
        tabsize = TableStream_getSize((TableStream *)table); \
        Py_DECREF(table); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] -= list[i]; \
        } \
    } \
    else if (PyList_Check(arg)) { \
        tabsize = (T_SIZE_T)PyList_Size(arg); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] -= PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
        } \
    } \
 \
    self->data[self->size] = self->data[0]; \
 \
    Py_RETURN_NONE; \

#define TABLE_MUL \
    T_SIZE_T i, tabsize; \
    MYFLT x = 0.0; \
    MYFLT *list = NULL; \
    PyObject *table = NULL; \
    if (PyNumber_Check(arg)) { \
        x = PyFloat_AsDouble(arg); \
        for (i=0; i<self->size; i++) { \
            self->data[i] *= x; \
        } \
    } \
    else if ( PyObject_HasAttrString((PyObject *)arg, "getTableStream") == 1 ) { \
        Py_XDECREF(table); \
        table = PyObject_CallMethod((PyObject *)arg, "getTableStream", ""); \
        list = TableStream_getData((TableStream *)table); \
        tabsize = TableStream_getSize((TableStream *)table); \
        Py_DECREF(table); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] *= list[i]; \
        } \
    } \
    else if (PyList_Check(arg)) { \
        tabsize = (T_SIZE_T)PyList_Size(arg); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] *= PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
        } \
    } \
 \
    self->data[self->size] = self->data[0]; \
 \
    Py_RETURN_NONE; \

#define TABLE_DIV \
    T_SIZE_T i, tabsize; \
    MYFLT x = 0.0; \
    MYFLT *list = NULL; \
    PyObject *table = NULL; \
    if (PyNumber_Check(arg)) { \
        x = PyFloat_AsDouble(arg); \
        if (x >= 0 && x < 1.0e-24) \
            x = 1.0e-24; \
        else if (x < 0 && x > -1.0e-24) \
            x = -1.0e-24; \
        for (i=0; i<self->size; i++) { \
            self->data[i] /= x; \
        } \
    } \
    else if ( PyObject_HasAttrString((PyObject *)arg, "getTableStream") == 1 ) { \
        Py_XDECREF(table); \
        table = PyObject_CallMethod((PyObject *)arg, "getTableStream", ""); \
        list = TableStream_getData((TableStream *)table); \
        tabsize = TableStream_getSize((TableStream *)table); \
        Py_DECREF(table); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            x = list[i]; \
            if (x >= 0 && x < 1.0e-24) \
                x = 1.0e-24; \
            else if (x < 0 && x > -1.0e-24) \
                x = -1.0e-24; \
            self->data[i] /= x; \
        } \
    } \
    else if (PyList_Check(arg)) { \
        tabsize = (T_SIZE_T)PyList_Size(arg); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
            if (x >= 0 && x < 1.0e-24) \
                x = 1.0e-24; \
            else if (x < 0 && x > -1.0e-24) \
                x = -1.0e-24; \
            self->data[i] /= x; \
        } \
    } \
 \
    self->data[self->size] = self->data[0]; \
 \
    Py_RETURN_NONE; \

#define SET_TABLE \
    T_SIZE_T i; \
    if (arg == NULL) { \
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute."); \
        return PyLong_FromLong(-1); \
    } \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "arg must be a list."); \
        return PyLong_FromLong(-1); \
    } \
    int size = PyList_Size(arg); \
    if (size != self->size) { \
        PyErr_SetString(PyExc_TypeError, "New table must be of the same size as actual table."); \
        return PyLong_FromLong(-1); \
    } \
    for (i=0; i<self->size; i++) { \
        self->data[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
    } \
    self->data[self->size] = self->data[0]; \
    Py_RETURN_NONE; \

#define GET_TABLE \
    T_SIZE_T i; \
    PyObject *samples; \
 \
    samples = PyList_New(self->size); \
    for (i=0; i<self->size; i++) { \
        PyList_SetItem(samples, i, PyFloat_FromDouble(self->data[i])); \
    } \
 \
    return samples;

// TODO: remove in stripped branch...
#define GET_VIEW_TABLE \
    int i, y, w, h, h2, amp; \
    float step; \
    PyObject *samples, *tuple, *sizetmp = NULL; \
 \
    static char *kwlist[] = {"size", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &sizetmp)) \
        return PyLong_FromLong(-1); \
 \
    if (sizetmp) { \
        if (PyTuple_Check(sizetmp)) { \
            w = PyLong_AsLong(PyTuple_GET_ITEM(sizetmp, 0)); \
            h = PyLong_AsLong(PyTuple_GET_ITEM(sizetmp, 1)); \
        } \
        else if (PyList_Check(sizetmp)) { \
            w = PyLong_AsLong(PyList_GET_ITEM(sizetmp, 0)); \
            h = PyLong_AsLong(PyList_GET_ITEM(sizetmp, 1)); \
        } \
        else { \
            w = 500; \
            h = 200; \
        } \
    } \
    else { \
        w = 500; \
        h = 200; \
    } \
    h2 = h / 2; \
    amp = h2 - 2; \
    step = (float)self->size / (float)(w); \
 \
    samples = PyList_New(w); \
    for (i=0; i<w; i++) { \
        y = self->data[(T_SIZE_T)(i * step)] * amp + amp + 2; \
        tuple = PyTuple_New(2); \
        PyTuple_SetItem(tuple, 0, PyLong_FromLong(i)); \
        PyTuple_SetItem(tuple, 1, PyLong_FromLong(h-y)); \
        PyList_SetItem(samples, i, tuple); \
    } \
 \
    return samples;

/* Table reverse */
#define REVERSE \
    T_SIZE_T i, j; \
    MYFLT tmp; \
    j = self->size; \
    for (i=0; i<--j; i++) { \
        tmp = self->data[i]; \
        self->data[i] = self->data[j]; \
        self->data[j] = tmp; \
    } \
    self->data[self->size] = self->data[0]; \
    Py_RETURN_NONE; \

/* Table reset */
#define TABLE_RESET \
    T_SIZE_T i; \
    for (i=0; i<self->size; i++) { \
        self->data[i] = 0.0; \
    } \
    Py_RETURN_NONE; \

/* Table remove DC */
#define REMOVE_DC \
    T_SIZE_T i; \
    MYFLT x, y, x1, y1; \
    x1 = y1 = 0.0; \
    for (i=0; i<self->size+1; i++) { \
        x = self->data[i]; \
        y = x - x1 + 0.995 * y1; \
        x1 = x; \
        self->data[i] = y1 = y; \
    } \
    Py_RETURN_NONE; \

/* Table amplitude reverse */
#define INVERT \
    T_SIZE_T i; \
    for (i=0; i<self->size+1; i++) { \
        self->data[i] = -self->data[i]; \
    } \
    Py_RETURN_NONE; \

/* Table positive rectify */
#define RECTIFY \
    T_SIZE_T i; \
    MYFLT x; \
    for (i=0; i<self->size+1; i++) { \
        x = self->data[i]; \
        if (x < 0) \
            self->data[i] = -x; \
    } \
    Py_RETURN_NONE; \

/* Table rotation */
#define TABLE_ROTATE \
    T_SIZE_T i, j, pos; \
    MYFLT tmp; \
    static char *kwlist[] = {"pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "n", kwlist, &pos)) \
        return PyLong_FromLong(-1); \
 \
    pos = -pos; \
    while (pos > self->size) pos -= self->size; \
    while (pos < 0) pos += self->size; \
 \
    j = self->size; \
    for (i=0; i<--j; i++) { \
        tmp = self->data[i]; \
        self->data[i] = self->data[j]; \
        self->data[j] = tmp; \
    } \
    j = pos; \
    for (i=0; i<--j; i++) { \
        tmp = self->data[i]; \
        self->data[i] = self->data[j]; \
        self->data[j] = tmp; \
    } \
    j = self->size; \
    for (i=pos; i<--j; i++) { \
        tmp = self->data[i]; \
        self->data[i] = self->data[j]; \
        self->data[j] = tmp; \
    } \
 \
    self->data[self->size] = self->data[0]; \
 \
    Py_RETURN_NONE;

/* Table copy from table */
#define TABLE_COPYDATA \
    PyObject *tabletmp; \
    T_SIZE_T i, tabsize, srcpos=0, destpos=0, length=-1; \
    PyObject *table = NULL; \
    MYFLT *list = NULL; \
    static char *kwlist[] = {"table", "srcpos", "destpos", "length", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|nnn", kwlist, &tabletmp, &srcpos, &destpos, &length)) \
        return PyLong_FromLong(-1); \
 \
    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 1 ) { \
        Py_XDECREF(table); \
        table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", ""); \
        tabsize = TableStream_getSize((TableStream *)table); \
 \
        if (srcpos < -tabsize || srcpos >= tabsize) { \
            PyErr_SetString(PyExc_IndexError, "PyoTableObject: Position outside of table boundaries!."); \
            Py_DECREF(table); \
            return PyLong_FromLong(-1); \
        } \
    \
        if (srcpos < 0) \
            srcpos = tabsize + srcpos; \
    \
 \
        if (destpos < -self->size || destpos >= self->size) { \
            PyErr_SetString(PyExc_IndexError, "PyoTableObject: Position outside of table boundaries!."); \
            Py_DECREF(table); \
            return PyLong_FromLong(-1); \
        } \
    \
        if (destpos < 0) \
            destpos = self->size + destpos; \
 \
        if (length < 0) \
            length = tabsize < self->size ? tabsize : self->size; \
        if ((srcpos + length) > tabsize) \
            length = tabsize - srcpos; \
        if ((destpos + length) > self->size) \
            length = self->size - destpos; \
        list = TableStream_getData((TableStream *)table); \
        Py_DECREF(table); \
        for (i=0; i<length; i++) { \
            self->data[destpos+i] = list[srcpos+i]; \
        } \
    } \
 \
    Py_RETURN_NONE;

/* Table bipolar gain */
#define TABLE_BIPOLAR_GAIN \
    MYFLT gpos = 1.0, gneg = 1.0; \
    T_SIZE_T i; \
    static char *kwlist[] = {"gpos", "gneg", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FF, kwlist, &gpos, &gneg)) \
        return PyLong_FromLong(-1); \
 \
    for (i=0; i<self->size+1; i++) { \
        if (self->data[i] < 0) \
            self->data[i] *= gneg; \
        else \
            self->data[i] *= gpos; \
    } \
 \
    Py_RETURN_NONE;

/* Table power function */
#define TABLE_POWER \
    MYFLT x, exp; \
    T_SIZE_T i; \
    int sign; \
    static char *kwlist[] = {"exp", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F, kwlist, &exp)) \
        return PyLong_FromLong(-1); \
 \
    for (i=0; i<self->size+1; i++) { \
        x = self->data[i]; \
        sign = 1; \
        if (x < 0) \
            sign = -1; \
        x = MYPOW(x, exp); \
        if (sign == -1 && x > 0) \
            x = -x; \
        self->data[i] = x; \
    } \
 \
    Py_RETURN_NONE;

/* Table one-pole lowpass filter */
#define TABLE_LOWPASS \
    MYFLT freq, b, c, x, y; \
    T_SIZE_T i; \
    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL); \
    double sr = PyFloat_AsDouble(srobj); \
    Py_DECREF(srobj); \
    static char *kwlist[] = {"freq", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F, kwlist, &freq)) \
        return PyLong_FromLong(-1); \
 \
    b = 2.0 - MYCOS(TWOPI * freq / sr); \
    c = b - MYSQRT(b * b - 1.0); \
    y = 0; \
    for (i=0; i<self->size+1; i++) { \
        x = self->data[i]; \
        self->data[i] = y = x + (y - x) * c; \
    } \
 \
    Py_RETURN_NONE;

/* FADE IN, FADE OUT */
#define TABLE_FADEIN \
    MYFLT dur, inc; \
    T_SIZE_T i, samp; \
    int shape = 0; \
    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL); \
    double sr = PyFloat_AsDouble(srobj); \
    Py_DECREF(srobj); \
    static char *kwlist[] = {"dur", "shape", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &dur, &shape)) \
        return PyLong_FromLong(-1); \
 \
    samp = (T_SIZE_T)(dur * sr + 0.5); \
    if (samp < 0 || samp >= self->size) \
        Py_RETURN_NONE; \
 \
    inc = 1.0 / samp; \
    switch (shape) \
    { \
        case 0: \
            for (i=0; i<samp; i++) \
                self->data[i] = self->data[i] * inc * i; \
            break; \
        case 1: \
            for (i=0; i<samp; i++) \
                self->data[i] = self->data[i] * MYSQRT(inc * i); \
            break; \
        case 2: \
            for (i=0; i<samp; i++) \
                self->data[i] = self->data[i] * MYSIN(inc * i * PI * 0.5); \
            break; \
        case 3: \
            for (i=0; i<samp; i++) \
                self->data[i] = self->data[i] * MYPOW(inc * i, 2.0); \
            break; \
        default: \
            for (i=0; i<samp; i++) \
                self->data[i] = self->data[i] * inc * i; \
    } \
 \
    Py_RETURN_NONE;

#define TABLE_FADEOUT \
    MYFLT dur, inc; \
    T_SIZE_T i, samp; \
    int shape = 0; \
    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL); \
    double sr = PyFloat_AsDouble(srobj); \
    Py_DECREF(srobj); \
    static char *kwlist[] = {"dur", "shape", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &dur, &shape)) \
        return PyLong_FromLong(-1); \
 \
    samp = (T_SIZE_T)(dur * sr + 0.5); \
    if (samp < 0 || samp >= self->size) \
        Py_RETURN_NONE; \
 \
    T_SIZE_T size1 = self->size - 1; \
    inc = 1.0 / samp; \
    switch (shape) \
    { \
        case 0: \
            for (i=size1; i>(size1-samp); i--) \
                self->data[i] = self->data[i] * inc * (size1 - i); \
            break; \
        case 1: \
            for (i=size1; i>(size1-samp); i--) \
                self->data[i] = self->data[i] * MYSQRT(inc * (size1 - i)); \
            break; \
        case 2: \
            for (i=size1; i>(size1-samp); i--) \
                self->data[i] = self->data[i] * MYSIN(inc * (size1 - i) * PI * 0.5); \
            break; \
        case 3: \
            for (i=size1; i>(size1-samp); i--) \
                self->data[i] = self->data[i] * MYPOW(inc * (size1 - i), 2.0); \
            break; \
        default: \
            for (i=size1; i>(size1-samp); i--) \
                self->data[i] = self->data[i] * inc * (size1 - i); \
    } \
 \
    Py_RETURN_NONE;

/* Normalize */
#define NORMALIZE \
    T_SIZE_T i; \
    MYFLT level = 0.99; \
    MYFLT mi, ma, max, ratio; \
 \
    static char *kwlist[] = {"level", NULL}; \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F, kwlist, &level)) \
        return PyLong_FromLong(-1); \
 \
    mi = ma = *self->data; \
    for (i=1; i<self->size; i++) { \
        if (mi > *(self->data+i)) \
            mi = *(self->data+i); \
        if (ma < *(self->data+i)) \
            ma = *(self->data+i); \
    } \
    if ((mi*mi) > (ma*ma)) \
        max = MYFABS(mi); \
    else \
        max = MYFABS(ma); \
 \
    if (max > 0.0) { \
        ratio = level / max; \
        for (i=0; i<self->size+1; i++) { \
            self->data[i] *= ratio; \
        } \
    } \
    Py_RETURN_NONE; \

#define NORMALIZE_MATRIX \
    int i, j; \
    MYFLT level = 0.99; \
    MYFLT mi, ma, max, ratio; \
 \
     static char *kwlist[] = {"level", NULL}; \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F, kwlist, &level)) \
        return PyLong_FromLong(-1); \
 \
    mi = ma = self->data[0][0]; \
    for (i=1; i<self->height; i++) { \
        for (j=1; j<self->width; j++) { \
            if (mi > self->data[i][j]) \
                mi = self->data[i][j]; \
            if (ma < self->data[i][j]) \
                ma = self->data[i][j]; \
        } \
    } \
    if ((mi*mi) > (ma*ma)) \
        max = MYFABS(mi); \
    else \
        max = MYFABS(ma); \
 \
    if (max > 0.0) { \
        ratio = level / max; \
        for (i=0; i<self->height+1; i++) { \
            for (j=0; j<self->width+1; j++) { \
                self->data[i][j] *= ratio; \
            } \
        } \
    } \
    Py_RETURN_NONE; \


#define TABLE_PUT \
    MYFLT val; \
    T_SIZE_T pos = 0; \
    static char *kwlist[] = {"value", "pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_N, kwlist, &val, &pos)) \
        return PyLong_FromLong(-1); \
 \
    if (pos < -self->size || pos >= self->size) { \
        PyErr_SetString(PyExc_IndexError, "PyoTableObject: Position outside of table boundaries!."); \
        return PyLong_FromLong(-1); \
    } \
 \
    if (pos < 0) \
        pos = self->size + pos; \
 \
    self->data[pos] = val; \
 \
    Py_RETURN_NONE;

#define TABLE_GET \
    T_SIZE_T pos; \
    static char *kwlist[] = {"pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "n", kwlist, &pos)) \
        return PyLong_FromLong(-1); \
 \
    if (pos < -self->size || pos >= self->size) { \
        PyErr_SetString(PyExc_IndexError, "PyoTableObject: Position outside of table boundaries!."); \
        return PyLong_FromLong(-1); \
    } \
 \
    if (pos < 0) \
        pos = self->size + pos; \
 \
    return PyFloat_FromDouble(self->data[pos]);

#define TABLE_GET_RATE \
    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL); \
    double sr = PyFloat_AsDouble(srobj); \
    Py_DECREF(srobj); \
    return PyFloat_FromDouble((MYFLT)sr / self->size);

#define TABLE_SET_SIZE \
    if (value == NULL) \
    { \
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute."); \
        return PyLong_FromLong(-1); \
    } \
 \
    if (! PyLong_Check(value)) \
    { \
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer."); \
        return PyLong_FromLong(-1); \
    } \
 \
    T_SIZE_T size = PyLong_AsLong(value); \
 \
    MYFLT *data = (MYFLT *)PyMem_RawRealloc(self->data, (size + 1) * sizeof(MYFLT)); \
    if (data != NULL) \
    { \
        self->data = data; \
        self->size = size; \
        TableStream_setData(self->tablestream, self->data); \
        TableStream_setSize(self->tablestream, self->size); \
    }

#define TABLE_SET_SIZE_WITH_POINT_LIST \
    T_SIZE_T i, old_size, x1; \
    MYFLT factor; \
    PyObject *tup, *x2; \
 \
    if (value == NULL) \
    { \
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute."); \
        return PyLong_FromLong(-1); \
    } \
 \
    if (! PyLong_Check(value)) \
    { \
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer."); \
        return PyLong_FromLong(-1); \
    } \
 \
    old_size = self->size; \
    self->size = PyLong_AsLong(value); \
 \
    factor = (MYFLT)(self->size) / old_size; \
 \
    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT)); \
    TableStream_setSize(self->tablestream, self->size); \
 \
    T_SIZE_T listsize = PyList_Size(self->pointslist); \
 \
    PyObject *listtemp = PyList_New(0); \
 \
    for (i = 0; i < (listsize); i++) \
    { \
        tup = PyList_GET_ITEM(self->pointslist, i); \
        PyObject *p1 = PyTuple_GET_ITEM(tup, 0); \
        x1 = PyLong_AsLong(PyNumber_Long(p1)); \
        PyObject *p2 = PyTuple_GET_ITEM(tup, 1); \
        x2 = PyNumber_Float(p2); \
        PyList_Append(listtemp, PyTuple_Pack(2, PyLong_FromLong((T_SIZE_T)(x1 * factor)), x2)); \
        Py_DECREF(p1); \
        Py_DECREF(p2); \
    } \
 \
    Py_INCREF(listtemp); \
    Py_DECREF(self->pointslist); \
    self->pointslist = listtemp;

/* Matrix macros */
#define MATRIX_BLUR \
    int i,j; \
    MYFLT tmp[self->height][self->width]; \
 \
    int lw = self->width - 1; \
    int lh = self->height - 1; \
    for (i=1; i<lw; i++) { \
        tmp[0][i] = (self->data[0][i-1] + self->data[0][i] + self->data[1][i] + self->data[0][i+1]) * 0.25; \
        tmp[lh][i] = (self->data[lh][i-1] + self->data[lh][i] + self->data[lh-1][i] + self->data[lh][i+1]) * 0.25; \
    } \
    for (i=1; i<lh; i++) { \
        tmp[i][0] = (self->data[i-1][0] + self->data[i][0] + self->data[i][1] + self->data[i+1][0]) * 0.25; \
        tmp[i][lw] = (self->data[i-1][lw] + self->data[i][lw] + self->data[i][lw-1] + self->data[i+1][lw]) * 0.25; \
    } \
 \
    for (i=1; i<lh; i++) { \
        for (j=1; j<lw; j++) { \
            tmp[i][j] = (self->data[i][j-1] + self->data[i][j] + self->data[i][j+1]) * 0.3333333; \
        } \
    } \
    for (j=1; j<lw; j++) { \
        for (i=1; i<lh; i++) { \
            self->data[i][j] = (tmp[i-1][j] + tmp[i][j] + tmp[i+1][j]) * 0.3333333; \
        } \
    } \
    Py_RETURN_NONE;

#define MATRIX_BOOST \
    int i, j; \
    MYFLT min, max, boost, val; \
    min = -1.0; \
    max = 1.0; \
    boost = 0.01; \
    static char *kwlist[] = {"min", "max", "boost", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FFF, kwlist, &min, &max, &boost)) \
        return PyLong_FromLong(-1); \
 \
    float mid = (min + max) * 0.5; \
 \
    for (i=0; i<self->height; i++) { \
        for (j=0; j<self->width; j++) { \
            val = self->data[i][j]; \
            self->data[i][j] = NewMatrix_clip(val + (val-mid) * boost, min, max); \
        } \
    } \
    Py_RETURN_NONE; \

#define MATRIX_PUT \
    MYFLT val; \
    int x, y; \
    x = y = 0; \
    static char *kwlist[] = {"value", "x", "y", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_II, kwlist, &val, &x, &y)) \
        return PyLong_FromLong(-1); \
 \
    if (x >= self->width) { \
        PyErr_SetString(PyExc_TypeError, "X position outside of matrix boundaries!."); \
        return PyLong_FromLong(-1); \
    } \
 \
    if (y >= self->height) { \
        PyErr_SetString(PyExc_TypeError, "Y position outside of matrix boundaries!."); \
        return PyLong_FromLong(-1); \
    } \
 \
    self->data[y][x] = val; \
 \
    if (x == 0 && y == 0) \
        self->data[self->height][self->width] = self->data[y][x]; \
    else if (x == 0) \
        self->data[y][self->width] = self->data[y][x]; \
    else if (y == 0) \
        self->data[self->height][x] = self->data[y][x]; \
 \
    Py_RETURN_NONE; \

#define MATRIX_GET \
    int x, y; \
    static char *kwlist[] = {"x", "y", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &x, &y)) \
        return PyLong_FromLong(-1); \
 \
    if (x >= self->width) { \
        PyErr_SetString(PyExc_TypeError, "X position outside of matrix boundaries!."); \
        return PyLong_FromLong(-1); \
    } \
 \
    if (y >= self->height) { \
        PyErr_SetString(PyExc_TypeError, "Y position outside of matrix boundaries!."); \
        return PyLong_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(self->data[y][x]); \

#define MATRIX_GET_INTERPOLATED \
    MYFLT x = 0.0, y = 0.0; \
    static char *kwlist[] = {"x", "y", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FF, kwlist, &x, &y)) \
        return PyLong_FromLong(-1); \
 \
    if (x < 0.0 || x > 1.0) { \
        PyErr_SetString(PyExc_TypeError, "X position outside of matrix boundaries!."); \
        return PyLong_FromLong(-1); \
    } \
 \
    if (y < 0.0 || y > 1.0) { \
        PyErr_SetString(PyExc_TypeError, "Y position outside of matrix boundaries!."); \
        return PyLong_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(MatrixStream_getInterpPointFromPos(self->matrixstream, x, y)); \

/* GETS & SETS */
#define GET_SERVER \
    if (self->server == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No server founded!"); \
        return PyLong_FromLong(-1); \
    } \
    Py_INCREF(self->server); \
    return self->server;

#define GET_STREAM \
    if (self->stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No stream founded!"); \
        return PyLong_FromLong(-1); \
    } \
    Py_INCREF(self->stream); \
    return (PyObject *)self->stream;

#define GET_TRIGGER_STREAM \
    if (self->trig_stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No trigger stream founded!"); \
        return PyLong_FromLong(-1); \
    } \
    Py_INCREF(self->trig_stream); \
    return (PyObject *)self->trig_stream;

#define GET_TABLE_STREAM \
    if (self->tablestream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No table stream founded!"); \
        return PyLong_FromLong(-1); \
    } \
    Py_INCREF(self->tablestream); \
    return (PyObject *)self->tablestream; \

#define GET_MATRIX_STREAM \
    if (self->matrixstream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No matrix stream founded!"); \
        return PyLong_FromLong(-1); \
    } \
    Py_INCREF(self->matrixstream); \
    return (PyObject *)self->matrixstream; \

#define GET_PV_STREAM \
    if (self->pv_stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No pv stream founded!"); \
        return PyLong_FromLong(-1); \
    } \
    Py_INCREF(self->pv_stream); \
    return (PyObject *)self->pv_stream;

#define SET_MUL \
    if (arg == NULL) { \
        Py_RETURN_NONE; \
    } \
 \
    Py_DECREF(self->mul); \
 \
    if (PyNumber_Check(arg)) { \
        self->mul = PyNumber_Float(arg); \
        self->modebuffer[0] = 0; \
    } \
    else { \
        self->mul = arg; \
        Py_INCREF(self->mul); \
        if (! PyObject_HasAttrString((PyObject *)self->mul, "_getStream")) { \
            PyErr_SetString(PyExc_ArithmeticError, "Only number or audio internal object can be used in arithmetic with audio internal objects.\n"); \
            PyErr_Print(); \
        } \
        PyObject *streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        if (streamtmp == NULL) { \
            return NULL; \
        } \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 1; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_RETURN_NONE;

#define SET_ADD \
    if (arg == NULL) { \
        Py_RETURN_NONE; \
    } \
\
    Py_DECREF(self->add); \
 \
    if (PyNumber_Check(arg)) { \
        self->add = PyNumber_Float(arg); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = arg; \
        Py_INCREF(self->add); \
        if (! PyObject_HasAttrString((PyObject *)self->add, "_getStream")) { \
            PyErr_SetString(PyExc_ArithmeticError, "Only number or audio internal object can be used in arithmetic with audio internal objects.\n"); \
            PyErr_Print(); \
        } \
        PyObject *streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        if (streamtmp == NULL) { \
            return NULL; \
        } \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 1; \
    } \
\
    (*self->mode_func_ptr)(self); \
\
    Py_RETURN_NONE;

#define SET_SUB \
    if (arg == NULL) { \
        Py_RETURN_NONE; \
    } \
 \
    Py_DECREF(self->add); \
 \
    if (PyNumber_Check(arg)) { \
        double tmp = PyFloat_AsDouble(arg); \
        self->add = PyFloat_FromDouble(tmp * -1.0); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = arg; \
        Py_INCREF(self->add); \
        if (! PyObject_HasAttrString((PyObject *)self->add, "_getStream")) { \
            PyErr_SetString(PyExc_ArithmeticError, "Only number or audio internal object can be used in arithmetic with audio internal objects.\n"); \
            PyErr_Print(); \
        } \
        PyObject *streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        if (streamtmp == NULL) { \
            return NULL; \
        } \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 2; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_RETURN_NONE;

#define SET_DIV \
    if (arg == NULL) { \
        Py_RETURN_NONE; \
    } \
 \
    if (PyNumber_Check(arg)) { \
        if (PyFloat_AsDouble(arg) != 0.) { \
            Py_DECREF(self->mul); \
            self->mul = PyFloat_FromDouble(1.0 / PyFloat_AsDouble(arg)); \
            self->modebuffer[0] = 0; \
        } \
    } \
    else { \
        Py_DECREF(self->mul); \
        self->mul = arg; \
        Py_INCREF(self->mul); \
        if (! PyObject_HasAttrString((PyObject *)self->mul, "_getStream")) { \
            PyErr_SetString(PyExc_ArithmeticError, "Only number or audio internal object can be used in arithmetic with audio internal objects.\n"); \
            PyErr_Print(); \
        } \
        PyObject *streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        if (streamtmp == NULL) { \
            return NULL; \
        } \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 2; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_RETURN_NONE;

#define SET_PARAM(param, paramstream, modebufpos) \
    if (arg == NULL) { \
        Py_RETURN_NONE; \
    } \
 \
    Py_DECREF(param); \
 \
    if (PyNumber_Check(arg)) { \
        param = PyNumber_Float(arg); \
        self->modebuffer[modebufpos] = 0; \
    } \
    else { \
        param = arg; \
        Py_INCREF(param); \
        PyObject *streamtmp = PyObject_CallMethod((PyObject *)param, "_getStream", NULL); \
        if (streamtmp == NULL) { \
            return NULL; \
        } \
        paramstream = (Stream *)streamtmp; \
        self->modebuffer[modebufpos] = 1; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_RETURN_NONE;

/* Multiply, Add, inplace_multiply & inplace_add */
#define MULTIPLY \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, PyoType_GetCurrent(PYO_RUNTIME_TYPE_DUMMY), NULL); \
    Dummy_initialize(dummy); \
    PYO_CALL_METHOD((PyObject *)dummy, "setMul", "O", arg); \
    PYO_CALL_METHOD((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_MULTIPLY \
    PYO_CALL_METHOD((PyObject *)self, "setMul", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define ADD \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, PyoType_GetCurrent(PYO_RUNTIME_TYPE_DUMMY), NULL); \
    Dummy_initialize(dummy); \
    PYO_CALL_METHOD((PyObject *)dummy, "setAdd", "O", arg); \
    PYO_CALL_METHOD((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_ADD \
    PYO_CALL_METHOD((PyObject *)self, "setAdd", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define SUB \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, PyoType_GetCurrent(PYO_RUNTIME_TYPE_DUMMY), NULL); \
    Dummy_initialize(dummy); \
    PYO_CALL_METHOD((PyObject *)dummy, "setSub", "O", arg); \
    PYO_CALL_METHOD((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_SUB \
    PYO_CALL_METHOD((PyObject *)self, "setSub", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define DIV \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, PyoType_GetCurrent(PYO_RUNTIME_TYPE_DUMMY), NULL); \
    Dummy_initialize(dummy); \
    PYO_CALL_METHOD((PyObject *)dummy, "setDiv", "O", arg); \
    PYO_CALL_METHOD((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_DIV \
    PYO_CALL_METHOD((PyObject *)self, "setDiv", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

/* PLAY, OUT, STOP */
#define PLAY \
    float del = 0; \
    float dur = 0; \
    float globdel = 0; \
    float globdur = 0; \
    int nearestBuf = 0; \
    int i; \
 \
    static char *kwlist[] = {"dur", "delay", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|ff", kwlist, &dur, &del)) \
        return PyLong_FromLong(-1); \
 \
    PyObject *delobj = PyObject_CallMethod(self->server, "getGlobalDel", NULL); \
    PyObject *durobj = PyObject_CallMethod(self->server, "getGlobalDur", NULL); \
    globdel = PyFloat_AsDouble(delobj); \
    globdur = PyFloat_AsDouble(durobj); \
    Py_DECREF(delobj); \
    Py_DECREF(durobj); \
 \
    if (globdel != 0) \
        del = globdel; \
    if (globdur != 0) \
        dur = globdur; \
 \
    Stream_setStreamToDac(self->stream, 0); \
    if (del == 0) { \
        Stream_setBufferCountWait(self->stream, 0); \
        Stream_setStreamActive(self->stream, 1); \
    } \
    else { \
        nearestBuf = (int)roundf((del * self->sr) / self->bufsize); \
        if (nearestBuf <= 0) { \
            Stream_setBufferCountWait(self->stream, 0); \
            Stream_setStreamActive(self->stream, 1); \
        } \
        else { \
            Stream_setStreamActive(self->stream, 0); \
            for (i=0; i<self->bufsize; i++) \
                self->data[i] = 0.0; \
            Stream_setBufferCountWait(self->stream, nearestBuf); \
        } \
    } \
    if (dur == 0) \
        Stream_setDuration(self->stream, 0); \
    else { \
        nearestBuf = (int)roundf((dur * self->sr) / self->bufsize + 0.5); \
        Stream_setDuration(self->stream, nearestBuf); \
    } \
    Py_INCREF(self); \
    return (PyObject *)self;

# define OUT \
    int chnltmp = 0; \
    float del = 0; \
    float dur = 0; \
    float globdel = 0; \
    float globdur = 0; \
    int nearestBuf = 0; \
    int i; \
\
    static char *kwlist[] = {"chnl", "dur", "delay", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iff", kwlist, &chnltmp, &dur, &del)) \
        return PyLong_FromLong(-1); \
 \
    PyObject *delobj = PyObject_CallMethod(self->server, "getGlobalDel", NULL); \
    PyObject *durobj = PyObject_CallMethod(self->server, "getGlobalDur", NULL); \
    globdel = PyFloat_AsDouble(delobj); \
    globdur = PyFloat_AsDouble(durobj); \
    Py_DECREF(delobj); \
    Py_DECREF(durobj); \
 \
    if (globdel != 0) \
        del = globdel; \
    if (globdur != 0) \
        dur = globdur; \
 \
    Stream_setStreamChnl(self->stream, chnltmp % self->nchnls); \
    Stream_setStreamToDac(self->stream, 1); \
    if (del == 0) { \
        Stream_setBufferCountWait(self->stream, 0); \
        Stream_setStreamActive(self->stream, 1); \
    } \
    else { \
        nearestBuf = (int)roundf((del * self->sr) / self->bufsize); \
        if (nearestBuf <= 0) { \
            Stream_setBufferCountWait(self->stream, 0); \
            Stream_setStreamActive(self->stream, 1); \
        } \
        else { \
            Stream_setStreamActive(self->stream, 0); \
            for (i=0; i<self->bufsize; i++) \
                self->data[i] = 0.0; \
            Stream_setBufferCountWait(self->stream, nearestBuf); \
        } \
    } \
    if (dur == 0) \
        Stream_setDuration(self->stream, 0); \
    else { \
        nearestBuf = (int)roundf((dur * self->sr) / self->bufsize + 0.5); \
        Stream_setDuration(self->stream, nearestBuf); \
    } \
    Py_INCREF(self); \
    return (PyObject *)self;

#define STOP \
    int i, nearestBuf = 0; \
    float wait = 0.0; \
 \
    static char *kwlist[] = {"wait", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|f", kwlist, &wait)) \
        return PyLong_FromLong(-1); \
 \
    if (wait == 0) { \
        Stream_setStreamActive(self->stream, 0); \
        Stream_setStreamChnl(self->stream, 0); \
        Stream_setStreamToDac(self->stream, 0); \
        for (i=0; i<self->bufsize; i++) { \
            self->data[i] = 0; \
        } \
    } \
    else { \
        Stream_resetBufferCount(self->stream); \
        nearestBuf = (int)roundf((wait * self->sr) / self->bufsize + 0.5); \
        Stream_setDuration(self->stream, nearestBuf); \
    } \
    Py_RETURN_NONE;

/* Post processing (mul & add) macros */
#define POST_PROCESSING_II \
    MYFLT mul, add, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    add = PyFloat_AS_DOUBLE(self->add); \
    if (mul != 1 || add != 0) { \
        for (i=0; i<self->bufsize; i++) { \
            old = self->data[i]; \
            val = mul * old + add; \
            self->data[i] = val; \
        } \
    }

#define POST_PROCESSING_AI \
    MYFLT add, old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    add = PyFloat_AS_DOUBLE(self->add); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old + add; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_IA \
    MYFLT mul, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul * old + add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_AA \
    MYFLT old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old + add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAI \
    MYFLT tmp, add, old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    add = PyFloat_AS_DOUBLE(self->add); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp + add; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAA \
    MYFLT tmp, old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp + add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_IREVA \
    MYFLT mul, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul * old - add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_AREVA \
    MYFLT old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old - add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAREVA \
    MYFLT tmp, old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp - add[i]; \
        self->data[i] = val; \
    }

/* Tables buffer protocol. */
#define TABLESTREAM_GET_BUFFER \
    if (view == NULL) { \
        PySys_WriteStdout("Pyo error: Table buffer, NULL view in getBuffer."); \
        return -1; \
    } \
    self->shape[0] = self->size; \
    view->obj = (PyObject *)self; \
    view->buf = (void *)self->data; \
    view->len = self->size * sizeof(MYFLT); \
    view->readonly = 0; \
    view->itemsize = sizeof(MYFLT); \
    view->format = TYPE_F; \
    view->ndim = 1; \
    view->shape = self->shape; \
    view->strides = NULL; \
    view->suboffsets = NULL; \
    view->internal = NULL; \
    Py_INCREF(self); \
    return 0;

#endif // _PYOMODULE_H
