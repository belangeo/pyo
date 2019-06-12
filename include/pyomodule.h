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

#include "Python.h"
#include <math.h>

#define PYO_VERSION "1.0.0"

#ifndef __MYFLT_DEF
#define __MYFLT_DEF

#ifndef USE_DOUBLE
#define LIB_BASE_NAME "_pyo"
#define MYFLT float
#define FLOAT_VALUE f
#define TYPE_F "f"
#define TYPE_F_I "f|i"
#define TYPE__IF "|if"
#define TYPE__FF "|ff"
#define TYPE__IFF "|iff"
#define TYPE__FII "|fii"
#define TYPE_F_II "f|ii"
#define TYPE__FFF "|fff"
#define TYPE_F_FFF "f|fff"
#define TYPE_O_FIFFI "O|fiffi"
#define TYPE_O_F "O|f"
#define TYPE_O_FO "O|fO"
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
#define TYPE__OFII "|Ofii"
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
#define TYPE__IF "|id"
#define TYPE__FF "|dd"
#define TYPE__IFF "|idd"
#define TYPE__FII "|dii"
#define TYPE_F_II "d|ii"
#define TYPE__FFF "|ddd"
#define TYPE_F_FFF "d|ddd"
#define TYPE_O_FIFFI "O|diddi"
#define TYPE_O_F "O|d"
#define TYPE_O_FO "O|dO"
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
#define TYPE__OFII "|Odii"
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

#endif
#endif

#ifdef COMPILE_EXTERNALS
#include "externalmodule.h"
#endif

#ifdef USE_PORTMIDI
extern PyTypeObject MidiListenerType;
extern PyTypeObject MidiDispatcherType;
#endif
#ifdef USE_OSC
extern PyTypeObject OscListenerType;
extern PyTypeObject OscSendType;
extern PyTypeObject OscReceiveType;
extern PyTypeObject OscReceiverType;
extern PyTypeObject OscDataSendType;
extern PyTypeObject OscDataReceiveType;
extern PyTypeObject OscListReceiveType;
extern PyTypeObject OscListReceiverType;
#endif
extern PyTypeObject SineType;
extern PyTypeObject FastSineType;
extern PyTypeObject SineLoopType;
extern PyTypeObject FmType;
extern PyTypeObject CrossFmType;
extern PyTypeObject LFOType;
extern PyTypeObject BlitType;
extern PyTypeObject RosslerType;
extern PyTypeObject RosslerAltType;
extern PyTypeObject LorenzType;
extern PyTypeObject LorenzAltType;
extern PyTypeObject ChenLeeType;
extern PyTypeObject ChenLeeAltType;
extern PyTypeObject PhasorType;
extern PyTypeObject SuperSawType;
extern PyTypeObject PointerType;
extern PyTypeObject TableIndexType;
extern PyTypeObject LookupType;
extern PyTypeObject TableReadType;
extern PyTypeObject OscType;
extern PyTypeObject OscLoopType;
extern PyTypeObject OscTrigType;
extern PyTypeObject OscBankType;
extern PyTypeObject SumOscType;
extern PyTypeObject PulsarType;
extern PyTypeObject NoiseType;
extern PyTypeObject PinkNoiseType;
extern PyTypeObject BrownNoiseType;
extern PyTypeObject InputType;
extern PyTypeObject SfPlayerType;
extern PyTypeObject SfPlayType;
extern PyTypeObject SfMarkerShufflerType;
extern PyTypeObject SfMarkerShuffleType;
extern PyTypeObject SfMarkerLooperType;
extern PyTypeObject SfMarkerLoopType;
extern PyTypeObject TrigType;
extern PyTypeObject NextTrigType;
extern PyTypeObject MetroType;
extern PyTypeObject SeqerType;
extern PyTypeObject SeqType;
extern PyTypeObject ClouderType;
extern PyTypeObject CloudType;
extern PyTypeObject BeaterType;
extern PyTypeObject BeatType;
extern PyTypeObject BeatTapStreamType;
extern PyTypeObject BeatAmpStreamType;
extern PyTypeObject BeatDurStreamType;
extern PyTypeObject BeatEndStreamType;
extern PyTypeObject CounterType;
extern PyTypeObject CountType;
extern PyTypeObject SelectType;
extern PyTypeObject ChangeType;
extern PyTypeObject ThreshType;
extern PyTypeObject PercentType;
extern PyTypeObject TimerType;
extern PyTypeObject ScoreType;
extern PyTypeObject FaderType;
extern PyTypeObject AdsrType;
extern PyTypeObject LinsegType;
extern PyTypeObject ExpsegType;
extern PyTypeObject RandiType;
extern PyTypeObject RandhType;
extern PyTypeObject RandDurType;
extern PyTypeObject ChoiceType;
extern PyTypeObject RandIntType;
extern PyTypeObject XnoiseType;
extern PyTypeObject XnoiseMidiType;
extern PyTypeObject XnoiseDurType;
extern PyTypeObject UrnType;
extern PyTypeObject BiquadType;
extern PyTypeObject BiquadxType;
extern PyTypeObject BiquadaType;
extern PyTypeObject EQType;
extern PyTypeObject ToneType;
extern PyTypeObject AtoneType;
extern PyTypeObject DCBlockType;
extern PyTypeObject PortType;
extern PyTypeObject AllpassType;
extern PyTypeObject Allpass2Type;
extern PyTypeObject PhaserType;
extern PyTypeObject VocoderType;
extern PyTypeObject DenormType;
extern PyTypeObject DistoType;
extern PyTypeObject ClipType;
extern PyTypeObject MirrorType;
extern PyTypeObject WrapType;
extern PyTypeObject BetweenType;
extern PyTypeObject DegradeType;
extern PyTypeObject CompressType;
extern PyTypeObject GateType;
extern PyTypeObject BalanceType;
extern PyTypeObject DelayType;
extern PyTypeObject SDelayType;
extern PyTypeObject WaveguideType;
extern PyTypeObject AllpassWGType;
extern PyTypeObject FreeverbType;
extern PyTypeObject WGVerbType;
extern PyTypeObject ChorusType;
extern PyTypeObject ConvolveType;
extern PyTypeObject IRWinSincType;
extern PyTypeObject IRPulseType;
extern PyTypeObject IRAverageType;
extern PyTypeObject IRFMType;
extern PyTypeObject GranulatorType;
extern PyTypeObject LooperType;
extern PyTypeObject LooperTimeStreamType;
extern PyTypeObject HarmonizerType;
extern PyTypeObject MidictlType;
extern PyTypeObject CtlScanType;
extern PyTypeObject CtlScan2Type;
extern PyTypeObject MidiNoteType;
extern PyTypeObject NoteinType;
extern PyTypeObject NoteinTrigType;
extern PyTypeObject BendinType;
extern PyTypeObject TouchinType;
extern PyTypeObject PrograminType;
extern PyTypeObject MidiAdsrType;
extern PyTypeObject MidiDelAdsrType;
extern PyTypeObject DummyType;
extern PyTypeObject TriggerDummyType;
extern PyTypeObject RecordType;
extern PyTypeObject ControlRecType;
extern PyTypeObject ControlReadType;
extern PyTypeObject NoteinRecType;
extern PyTypeObject NoteinReadType;
extern PyTypeObject CompareType;
extern PyTypeObject MixType;
extern PyTypeObject SigType;
extern PyTypeObject SigToType;
extern PyTypeObject VarPortType;
extern PyTypeObject InputFaderType;
extern PyTypeObject HarmTableType;
extern PyTypeObject ChebyTableType;
extern PyTypeObject HannTableType;
extern PyTypeObject SincTableType;
extern PyTypeObject WinTableType;
extern PyTypeObject ParaTableType;
extern PyTypeObject LinTableType;
extern PyTypeObject LogTableType;
extern PyTypeObject CosLogTableType;
extern PyTypeObject CosTableType;
extern PyTypeObject CurveTableType;
extern PyTypeObject ExpTableType;
extern PyTypeObject SndTableType;
extern PyTypeObject DataTableType;
extern PyTypeObject NewTableType;
extern PyTypeObject TableRecType;
extern PyTypeObject TableWriteType;
extern PyTypeObject TableRecTimeStreamType;
extern PyTypeObject TableMorphType;
extern PyTypeObject TrigTableRecType;
extern PyTypeObject TrigTableRecTimeStreamType;
extern PyTypeObject TablePutType;
extern PyTypeObject NewMatrixType;
extern PyTypeObject MatrixPointerType;
extern PyTypeObject MatrixRecType;
extern PyTypeObject MatrixRecLoopType;
extern PyTypeObject MatrixMorphType;
extern PyTypeObject TrigRandIntType;
extern PyTypeObject TrigValType;
extern PyTypeObject TrigRandType;
extern PyTypeObject TrigChoiceType;
extern PyTypeObject IterType;
extern PyTypeObject TrigEnvType;
extern PyTypeObject TrigLinsegType;
extern PyTypeObject TrigExpsegType;
extern PyTypeObject TrigFuncType;
extern PyTypeObject TrigXnoiseType;
extern PyTypeObject TrigXnoiseMidiType;
extern PyTypeObject PatternType;
extern PyTypeObject CallAfterType;
extern PyTypeObject BandSplitterType;
extern PyTypeObject BandSplitType;
extern PyTypeObject FourBandMainType;
extern PyTypeObject FourBandType;
extern PyTypeObject HilbertMainType;
extern PyTypeObject HilbertType;
extern PyTypeObject FollowerType;
extern PyTypeObject Follower2Type;
extern PyTypeObject ZCrossType;
extern PyTypeObject SPannerType;
extern PyTypeObject SPanType;
extern PyTypeObject PannerType;
extern PyTypeObject PanType;
extern PyTypeObject SwitcherType;
extern PyTypeObject SwitchType;
extern PyTypeObject SelectorType;
extern PyTypeObject VoiceManagerType;
extern PyTypeObject MixerType;
extern PyTypeObject MixerVoiceType;
extern PyTypeObject PrintType;
extern PyTypeObject SnapType;
extern PyTypeObject InterpType;
extern PyTypeObject SampHoldType;
extern PyTypeObject DBToAType;
extern PyTypeObject AToDBType;
extern PyTypeObject ScaleType;
extern PyTypeObject CentsToTranspoType;
extern PyTypeObject TranspoToCentsType;
extern PyTypeObject MToFType;
extern PyTypeObject FToMType;
extern PyTypeObject MToTType;
extern PyTypeObject M_SinType;
extern PyTypeObject M_CosType;
extern PyTypeObject M_TanType;
extern PyTypeObject M_AbsType;
extern PyTypeObject M_SqrtType;
extern PyTypeObject M_LogType;
extern PyTypeObject M_Log2Type;
extern PyTypeObject M_Log10Type;
extern PyTypeObject M_PowType;
extern PyTypeObject M_Atan2Type;
extern PyTypeObject M_FloorType;
extern PyTypeObject M_CeilType;
extern PyTypeObject M_RoundType;
extern PyTypeObject M_TanhType;
extern PyTypeObject M_ExpType;
extern PyTypeObject FFTMainType;
extern PyTypeObject FFTType;
extern PyTypeObject IFFTType;
extern PyTypeObject CarToPolType;
extern PyTypeObject PolToCarType;
extern PyTypeObject FrameDeltaMainType;
extern PyTypeObject FrameDeltaType;
extern PyTypeObject FrameAccumMainType;
extern PyTypeObject FrameAccumType;
extern PyTypeObject VectralMainType;
extern PyTypeObject VectralType;
extern PyTypeObject MinType;
extern PyTypeObject MaxType;
extern PyTypeObject Delay1Type;
extern PyTypeObject RCOscType;
extern PyTypeObject YinType;
extern PyTypeObject SVFType;
extern PyTypeObject SVF2Type;
extern PyTypeObject AverageType;
extern PyTypeObject CvlVerbType;
extern PyTypeObject SpectrumType;
extern PyTypeObject ResonType;
extern PyTypeObject ResonxType;
extern PyTypeObject ButLPType;
extern PyTypeObject ButHPType;
extern PyTypeObject ButBPType;
extern PyTypeObject ButBRType;
extern PyTypeObject MoogLPType;
extern PyTypeObject PVAnalType;
extern PyTypeObject PVSynthType;
extern PyTypeObject PVTransposeType;
extern PyTypeObject PVVerbType;
extern PyTypeObject PVGateType;
extern PyTypeObject PVAddSynthType;
extern PyTypeObject PVCrossType;
extern PyTypeObject PVMultType;
extern PyTypeObject PVMorphType;
extern PyTypeObject PVFilterType;
extern PyTypeObject PVDelayType;
extern PyTypeObject PVBufferType;
extern PyTypeObject PVShiftType;
extern PyTypeObject PVAmpModType;
extern PyTypeObject PVFreqModType;
extern PyTypeObject PVBufLoopsType;
extern PyTypeObject PVBufTabLoopsType;
extern PyTypeObject PVMixType;
extern PyTypeObject GranuleType;
extern PyTypeObject TableScaleType;
extern PyTypeObject TrackHoldType;
extern PyTypeObject ComplexResType;
extern PyTypeObject STReverbType;
extern PyTypeObject STRevType;
extern PyTypeObject Pointer2Type;
extern PyTypeObject CentroidType;
extern PyTypeObject AttackDetectorType;
extern PyTypeObject SmoothDelayType;
extern PyTypeObject TrigBursterType;
extern PyTypeObject TrigBurstType;
extern PyTypeObject TrigBurstTapStreamType;
extern PyTypeObject TrigBurstAmpStreamType;
extern PyTypeObject TrigBurstDurStreamType;
extern PyTypeObject TrigBurstEndStreamType;
extern PyTypeObject ScopeType;
extern PyTypeObject PeakAmpType;
extern PyTypeObject MainParticleType;
extern PyTypeObject ParticleType;
extern PyTypeObject MainParticle2Type;
extern PyTypeObject Particle2Type;
extern PyTypeObject AtanTableType;
extern PyTypeObject RawMidiType;
extern PyTypeObject ResampleType;
extern PyTypeObject ExprType;
extern PyTypeObject PadSynthTableType;
extern PyTypeObject LogiMapType;
extern PyTypeObject SharedTableType;
extern PyTypeObject TableFillType;
extern PyTypeObject TableScanType;
extern PyTypeObject HRTFDataType;
extern PyTypeObject HRTFSpatterType;
extern PyTypeObject HRTFType;
extern PyTypeObject ExpandType;
extern PyTypeObject RMSType;
extern PyTypeObject MidiLinsegType;
extern PyTypeObject MultiBandMainType;
extern PyTypeObject MultiBandType;
extern PyTypeObject M_DivType;
extern PyTypeObject M_SubType;

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
#define pyo_audio_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    Stream *stream; \
    void (*mode_func_ptr)(); \
    void (*proc_func_ptr)(); \
    void (*muladd_func_ptr)(); \
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
    int size; \
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
    if (self->stream != NULL) \
        Py_VISIT(self->stream); \
    Py_VISIT(self->mul); \
    Py_VISIT(self->mul_stream); \
    Py_VISIT(self->add); \
    Py_VISIT(self->add_stream);

#define pyo_table_VISIT \
    if (self->server != NULL) \
        Py_VISIT(self->server); \
    if (self->tablestream != NULL) \
        Py_VISIT(self->tablestream); \

#define pyo_matrix_VISIT \
    if (self->server != NULL) \
        Py_VISIT(self->server); \
    if (self->matrixstream != NULL) \
        Py_VISIT(self->matrixstream); \

#define pyo_CLEAR \
    if (self->server != NULL) { \
        Py_DECREF(self->server); \
        self->server = NULL; \
    } \
    if (self->stream != NULL) \
        Py_CLEAR(self->stream); \
    Py_CLEAR(self->mul); \
    Py_CLEAR(self->mul_stream); \
    Py_CLEAR(self->add); \
    Py_CLEAR(self->add_stream); \

#define pyo_table_CLEAR \
    if (self->server != NULL) { \
        Py_DECREF(self->server); \
        self->server = NULL; \
    } \
    if (self->tablestream != NULL) \
        Py_CLEAR(self->tablestream); \

#define pyo_matrix_CLEAR \
    if (self->server != NULL) { \
        Py_DECREF(self->server); \
        self->server = NULL; \
    } \
    if (self->matrixstream != NULL) \
        Py_CLEAR(self->matrixstream); \

#define pyo_DEALLOC \
    if (self->server != NULL && self->stream != NULL) \
        Server_removeStream((Server *)self->server, Stream_getStreamId(self->stream)); \
    free(self->data); \

#define ASSERT_ARG_NOT_NULL \
	if (arg == NULL) { \
		Py_INCREF(Py_None); \
		return Py_None; \
	}

/* INIT INPUT STREAM */
#define INIT_INPUT_STREAM \
    if ( PyObject_HasAttrString((PyObject *)inputtmp, "server") == 0 ) { \
        PyErr_SetString(PyExc_TypeError, "\"input\" argument must be a PyoObject.\n"); \
        Py_RETURN_NONE; \
    } \
    Py_INCREF(inputtmp); \
    Py_XDECREF(self->input); \
    self->input = inputtmp; \
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL); \
    Py_INCREF(input_streamtmp); \
    Py_XDECREF(self->input_stream); \
    self->input_stream = (Stream *)input_streamtmp;

#define INIT_INPUT_TRIGGER_STREAM \
    Py_INCREF(inputtmp); \
    Py_XDECREF(self->input); \
    self->input = inputtmp; \
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getTriggerStream", NULL); \
    Py_INCREF(input_streamtmp); \
    Py_XDECREF(self->input_stream); \
    self->input_stream = (TriggerStream *)input_streamtmp;

/* Init Server & Stream */
#define INIT_OBJECT_COMMON \
    self->server = PyServer_get_server(); \
    Py_INCREF(self->server); \
    self->mul = PyFloat_FromDouble(1); \
    self->add = PyFloat_FromDouble(0); \
    self->bufsize = PyInt_AsLong(PyObject_CallMethod(self->server, "getBufferSize", NULL)); \
    self->sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    self->nchnls = PyInt_AsLong(PyObject_CallMethod(self->server, "getNchnls", NULL)); \
    self->ichnls = PyInt_AsLong(PyObject_CallMethod(self->server, "getIchnls", NULL)); \
    self->data = (MYFLT *)realloc(self->data, (self->bufsize) * sizeof(MYFLT)); \
    for (i=0; i<self->bufsize; i++) \
        self->data[i] = 0.0; \
    MAKE_NEW_STREAM(self->stream, &StreamType, NULL); \
    Stream_setStreamObject(self->stream, (PyObject *)self); \
    Stream_setStreamId(self->stream, Stream_getNewStreamId()); \
    Stream_setBufferSize(self->stream, self->bufsize); \
    Stream_setData(self->stream, self->data);

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
    int i; \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "The data must be a list of floats."); \
        return PyInt_FromLong(-1); \
    } \
    self->size = PyList_Size(arg); \
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT)); \
    TableStream_setSize(self->tablestream, self->size+1); \
 \
    for (i=0; i<(self->size); i++) { \
        self->data[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
    } \
    self->data[self->size] = self->data[0]; \
    TableStream_setData(self->tablestream, self->data); \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define SET_MATRIX_DATA \
    int i, j; \
    PyObject *innerlist; \
 \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "The data must be a list of list of floats."); \
        return PyInt_FromLong(-1); \
    } \
    self->height = PyList_Size(arg); \
    self->width = PyList_Size(PyList_GetItem(arg, 0)); \
    self->data = (MYFLT **)realloc(self->data, (self->height + 1) * sizeof(MYFLT)); \
    for (i=0; i<(self->height+1); i++) { \
        self->data[i] = (MYFLT *)realloc(self->data[i], (self->width + 1) * sizeof(MYFLT)); \
    } \
    MatrixStream_setWidth(self->matrixstream, self->width); \
    MatrixStream_setHeight(self->matrixstream, self->height); \
 \
    for(i=0; i<self->height; i++) { \
        innerlist = PyList_GetItem(arg, i); \
        for (j=0; j<self->width; j++) { \
            self->data[i][j] = PyFloat_AsDouble(PyList_GET_ITEM(innerlist, j)); \
        } \
    } \
 \
    MatrixStream_setData(self->matrixstream, self->data); \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define COPY \
    int i; \
    MYFLT *tab = TableStream_getData((TableStream *)PyObject_CallMethod((PyObject *)arg, "getTableStream", "")); \
    for (i=0; i<self->size; i++) { \
        self->data[i] = tab[i]; \
    } \
    self->data[self->size] = self->data[0]; \
    Py_RETURN_NONE; \

#define TABLE_ADD \
    int i, tabsize; \
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
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] += list[i]; \
        } \
    } \
    else if (PyList_Check(arg)) { \
        tabsize = PyList_Size(arg); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] += PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
        } \
    } \
 \
    self->data[self->size] = self->data[0]; \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define TABLE_SUB \
    int i, tabsize; \
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
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] -= list[i]; \
        } \
    } \
    else if (PyList_Check(arg)) { \
        tabsize = PyList_Size(arg); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] -= PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
        } \
    } \
 \
    self->data[self->size] = self->data[0]; \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define TABLE_MUL \
    int i, tabsize; \
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
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] *= list[i]; \
        } \
    } \
    else if (PyList_Check(arg)) { \
        tabsize = PyList_Size(arg); \
        if (self->size < tabsize) \
            tabsize = self->size; \
        for (i=0; i<tabsize; i++) { \
            self->data[i] *= PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
        } \
    } \
 \
    self->data[self->size] = self->data[0]; \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define SET_TABLE \
    int i; \
    if (arg == NULL) { \
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute."); \
        return PyInt_FromLong(-1); \
    } \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "arg must be a list."); \
        return PyInt_FromLong(-1); \
    } \
    int size = PyList_Size(arg); \
    if (size != self->size) { \
        PyErr_SetString(PyExc_TypeError, "New table must be of the same size as actual table."); \
        return PyInt_FromLong(-1); \
    } \
    for(i=0; i<self->size; i++) { \
        self->data[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i)); \
    } \
    self->data[self->size] = self->data[0]; \
    Py_RETURN_NONE; \

#define GET_TABLE \
    int i; \
    PyObject *samples; \
 \
    samples = PyList_New(self->size); \
    for(i=0; i<self->size; i++) { \
        PyList_SetItem(samples, i, PyFloat_FromDouble(self->data[i])); \
    } \
 \
    return samples;

#define GET_VIEW_TABLE \
    int i, y, w, h, h2, amp; \
    float step; \
    PyObject *samples, *tuple, *sizetmp = NULL; \
 \
    static char *kwlist[] = {"size", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &sizetmp)) \
        return PyInt_FromLong(-1); \
 \
    if (sizetmp) { \
        if (PyTuple_Check(sizetmp)) { \
            w = PyInt_AsLong(PyTuple_GET_ITEM(sizetmp, 0)); \
            h = PyInt_AsLong(PyTuple_GET_ITEM(sizetmp, 1)); \
        } \
        else if (PyList_Check(sizetmp)) { \
            w = PyInt_AsLong(PyList_GET_ITEM(sizetmp, 0)); \
            h = PyInt_AsLong(PyList_GET_ITEM(sizetmp, 1)); \
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
    h2 = h/2; \
    amp = h2 - 2; \
    step = (float)self->size / (float)(w); \
 \
    samples = PyList_New(w); \
    for(i=0; i<w; i++) { \
        y = self->data[(int)(i*step)] * amp + amp + 2; \
        tuple = PyTuple_New(2); \
        PyTuple_SetItem(tuple, 0, PyInt_FromLong(i)); \
        PyTuple_SetItem(tuple, 1, PyInt_FromLong(h-y)); \
        PyList_SetItem(samples, i, tuple); \
    } \
 \
    return samples;

/* Table reverse */
#define REVERSE \
    int i, j; \
    MYFLT tmp; \
    j = self->size; \
    for (i=0; i<--j; i++) { \
        tmp = self->data[i]; \
        self->data[i] = self->data[j]; \
        self->data[j] = tmp; \
    } \
    self->data[self->size] = self->data[0]; \
    Py_INCREF(Py_None); \
    return Py_None; \

/* Table reset */
#define TABLE_RESET \
    int i; \
    for (i=0; i<self->size; i++) { \
        self->data[i] = 0.0; \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \

/* Table remove DC */
#define REMOVE_DC \
    int i; \
    MYFLT x, y, x1, y1; \
    x1 = y1 = 0.0; \
    for (i=0; i<self->size+1; i++) { \
        x = self->data[i]; \
        y = x - x1 + 0.995 * y1; \
        x1 = x; \
        self->data[i] = y1 = y; \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \

/* Table amplitude reverse */
#define INVERT \
    int i; \
    for (i=0; i<self->size+1; i++) { \
        self->data[i] = -self->data[i]; \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \

/* Table positive rectify */
#define RECTIFY \
    int i; \
    MYFLT x; \
    for (i=0; i<self->size+1; i++) { \
        x = self->data[i]; \
        if (x < 0) \
            self->data[i] = -x; \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \

/* Table rotation */
#define TABLE_ROTATE \
    int i, j, pos; \
    MYFLT tmp; \
    static char *kwlist[] = {"pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &pos)) \
        return PyInt_FromLong(-1); \
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
    int i, tabsize, srcpos=0, destpos=0, length=-1; \
    PyObject *table = NULL; \
    MYFLT *list = NULL; \
    static char *kwlist[] = {"table", "srcpos", "destpos", "length", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iii", kwlist, &tabletmp, &srcpos, &destpos, &length)) \
        return PyInt_FromLong(-1); \
 \
    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 1 ) { \
        Py_XDECREF(table); \
        table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", ""); \
        tabsize = TableStream_getSize((TableStream *)table); \
        if (length < 0) \
            length = tabsize < self->size ? tabsize : self->size; \
        if ((srcpos + length) > tabsize) \
            length = tabsize - srcpos; \
        if ((destpos + length) > self->size) \
            length = self->size - destpos; \
        list = TableStream_getData((TableStream *)table); \
        for (i=0; i<length; i++) { \
            self->data[destpos+i] = list[srcpos+i]; \
        } \
    } \
 \
    Py_RETURN_NONE;

/* Table bipolar gain */
#define TABLE_BIPOLAR_GAIN \
    MYFLT gpos = 1.0, gneg = 1.0; \
    int i; \
    static char *kwlist[] = {"gpos", "gneg", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FF, kwlist, &gpos, &gneg)) \
        return PyInt_FromLong(-1); \
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
    int i, sign; \
    static char *kwlist[] = {"exp", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F, kwlist, &exp)) \
        return PyInt_FromLong(-1); \
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
    int i; \
    double sr = PyFloat_AsDouble(PyObject_CallMethod(PyServer_get_server(), "getSamplingRate", NULL)); \
    static char *kwlist[] = {"freq", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F, kwlist, &freq)) \
        return PyInt_FromLong(-1); \
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
    int i, samp; \
    double sr = PyFloat_AsDouble(PyObject_CallMethod(PyServer_get_server(), "getSamplingRate", NULL)); \
    static char *kwlist[] = {"dur", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F, kwlist, &dur)) \
        return PyInt_FromLong(-1); \
 \
    samp = (int)(dur * sr); \
    if (samp < 0 || samp >= self->size) \
        Py_RETURN_NONE; \
 \
    inc = 1.0 / samp; \
    for (i=0; i<samp; i++) { \
        self->data[i] = self->data[i] * MYSQRT(inc * i); \
    } \
 \
    Py_RETURN_NONE;

#define TABLE_FADEOUT \
    MYFLT dur, inc; \
    int i, samp; \
    double sr = PyFloat_AsDouble(PyObject_CallMethod(PyServer_get_server(), "getSamplingRate", NULL)); \
    static char *kwlist[] = {"dur", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F, kwlist, &dur)) \
        return PyInt_FromLong(-1); \
 \
    samp = (int)(dur * sr); \
    if (samp < 0 || samp >= self->size) \
        Py_RETURN_NONE; \
 \
    inc = 1.0 / samp; \
    for (i=self->size; i>(self->size-samp); i--) { \
        self->data[i] = self->data[i] * MYSQRT(inc * (self->size - i)); \
    } \
 \
    Py_RETURN_NONE;

/* Normalize */
#define NORMALIZE \
	int i; \
	MYFLT mi, ma, max, ratio; \
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
		ratio = 0.99 / max; \
		for (i=0; i<self->size+1; i++) { \
			self->data[i] *= ratio; \
		} \
	} \
	Py_INCREF(Py_None); \
	return Py_None; \

#define NORMALIZE_MATRIX \
    int i, j; \
    MYFLT mi, ma, max, ratio; \
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
        ratio = 0.99 / max; \
        for (i=0; i<self->height+1; i++) { \
            for (j=0; j<self->width+1; j++) { \
                self->data[i][j] *= ratio; \
            } \
        } \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \


#define TABLE_PUT \
    MYFLT val; \
    int pos = 0; \
    static char *kwlist[] = {"value", "pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &val, &pos)) \
        return PyInt_FromLong(-1); \
 \
    if (pos >= self->size) \
        pos = self->size - 1; \
    else if (pos < 0) \
        pos = 0; \
 \
    self->data[pos] = val; \
 \
    Py_RETURN_NONE;

#define TABLE_GET \
    int pos; \
    static char *kwlist[] = {"pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &pos)) \
        return PyInt_FromLong(-1); \
 \
    if (pos >= self->size) { \
        PyErr_SetString(PyExc_TypeError, "position outside of table boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(self->data[pos]);

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
    Py_INCREF(Py_None); \
    return Py_None;

#define MATRIX_BOOST \
    int i, j; \
    MYFLT min, max, boost, val; \
    min = -1.0; \
    max = 1.0; \
    boost = 0.01; \
    static char *kwlist[] = {"min", "max", "boost", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FFF, kwlist, &min, &max, &boost)) \
        return PyInt_FromLong(-1); \
 \
    float mid = (min + max) * 0.5; \
 \
    for (i=0; i<self->height; i++) { \
        for (j=0; j<self->width; j++) { \
            val = self->data[i][j]; \
            self->data[i][j] = NewMatrix_clip(val + (val-mid) * boost, min, max); \
        } \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \

#define MATRIX_PUT \
    MYFLT val; \
    int x, y; \
    x = y = 0; \
    static char *kwlist[] = {"value", "x", "y", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_II, kwlist, &val, &x, &y)) \
        return PyInt_FromLong(-1); \
 \
    if (x >= self->width) { \
        PyErr_SetString(PyExc_TypeError, "X position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    if (y >= self->height) { \
        PyErr_SetString(PyExc_TypeError, "Y position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    self->data[y][x] = val; \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define MATRIX_GET \
    int x, y; \
    static char *kwlist[] = {"x", "y", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &x, &y)) \
        return PyInt_FromLong(-1); \
 \
    if (x >= self->width) { \
        PyErr_SetString(PyExc_TypeError, "X position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    if (y >= self->height) { \
        PyErr_SetString(PyExc_TypeError, "Y position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(self->data[y][x]); \

#define MATRIX_GET_INTERPOLATED \
    MYFLT x = 0.0, y = 0.0; \
    static char *kwlist[] = {"x", "y", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FF, kwlist, &x, &y)) \
        return PyInt_FromLong(-1); \
 \
    if (x < 0.0 || x > 1.0) { \
        PyErr_SetString(PyExc_TypeError, "X position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    if (y < 0.0 || y > 1.0) { \
        PyErr_SetString(PyExc_TypeError, "Y position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(MatrixStream_getInterpPointFromPos(self->matrixstream, x, y)); \

/* GETS & SETS */
#define GET_SERVER \
    if (self->server == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No server founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->server); \
    return self->server;

#define GET_STREAM \
    if (self->stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->stream); \
    return (PyObject *)self->stream;

#define GET_TRIGGER_STREAM \
    if (self->trig_stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No trigger stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->trig_stream); \
    return (PyObject *)self->trig_stream;

#define GET_TABLE_STREAM \
    if (self->tablestream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No table stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->tablestream); \
    return (PyObject *)self->tablestream; \

#define GET_MATRIX_STREAM \
    if (self->matrixstream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No matrix stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->matrixstream); \
    return (PyObject *)self->matrixstream; \

#define GET_PV_STREAM \
    if (self->pv_stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No pv stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->pv_stream); \
    return (PyObject *)self->pv_stream;

#define SET_MUL \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->mul); \
    if (isNumber == 1) { \
        self->mul = PyNumber_Float(tmp); \
        self->modebuffer[0] = 0; \
    } \
    else { \
        self->mul = tmp; \
        if (! PyObject_HasAttrString((PyObject *)self->mul, "_getStream")) { \
            PyErr_SetString(PyExc_ArithmeticError, "Only number or audio internal object can be used in arithmetic with audio internal objects.\n"); \
            PyErr_Print(); \
        } \
        streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->mul_stream); \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 1; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None;

#define SET_ADD \
    PyObject *tmp, *streamtmp; \
\
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
\
    int isNumber = PyNumber_Check(arg); \
\
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->add); \
    if (isNumber == 1) { \
        self->add = PyNumber_Float(tmp); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = tmp; \
        if (! PyObject_HasAttrString((PyObject *)self->add, "_getStream")) { \
            PyErr_SetString(PyExc_ArithmeticError, "Only number or audio internal object can be used in arithmetic with audio internal objects.\n"); \
            PyErr_Print(); \
        } \
        streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->add_stream); \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 1; \
    } \
\
    (*self->mode_func_ptr)(self); \
\
    Py_INCREF(Py_None); \
    return Py_None;

#define SET_SUB \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->add); \
    if (isNumber == 1) { \
        self->add = PyNumber_Multiply(PyNumber_Float(tmp), PyFloat_FromDouble(-1)); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = tmp; \
        if (! PyObject_HasAttrString((PyObject *)self->add, "_getStream")) { \
            PyErr_SetString(PyExc_ArithmeticError, "Only number or audio internal object can be used in arithmetic with audio internal objects.\n"); \
            PyErr_Print(); \
        } \
        streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->add_stream); \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 2; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None;

#define SET_DIV \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    if (isNumber == 1) { \
        if (PyFloat_AsDouble(tmp) != 0.) { \
            Py_DECREF(self->mul); \
            self->mul = PyNumber_TrueDivide(PyFloat_FromDouble(1.), PyNumber_Float(tmp)); \
            self->modebuffer[0] = 0; \
        } \
    } \
    else { \
        Py_DECREF(self->mul); \
        self->mul = tmp; \
        if (! PyObject_HasAttrString((PyObject *)self->mul, "_getStream")) { \
            PyErr_SetString(PyExc_ArithmeticError, "Only number or audio internal object can be used in arithmetic with audio internal objects.\n"); \
            PyErr_Print(); \
        } \
        streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->mul_stream); \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 2; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None;

/* Multiply, Add, inplace_multiply & inplace_add */
#define MULTIPLY \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setMul", "O", arg); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    return (PyObject *)dummy;

#define INPLACE_MULTIPLY \
    PyObject_CallMethod((PyObject *)self, "setMul", "O", arg); \
    return (PyObject *)self;

#define ADD \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setAdd", "O", arg); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    return (PyObject *)dummy;

#define INPLACE_ADD \
    PyObject_CallMethod((PyObject *)self, "setAdd", "O", arg); \
    return (PyObject *)self;

#define SUB \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setSub", "O", arg); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    return (PyObject *)dummy;

#define INPLACE_SUB \
    PyObject_CallMethod((PyObject *)self, "setSub", "O", arg); \
    return (PyObject *)self;

#define DIV \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setDiv", "O", arg); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    return (PyObject *)dummy;

#define INPLACE_DIV \
    PyObject_CallMethod((PyObject *)self, "setDiv", "O", arg); \
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
        return PyInt_FromLong(-1); \
 \
    globdel = PyFloat_AsDouble(PyObject_CallMethod(PyServer_get_server(), "getGlobalDel", NULL)); \
    globdur = PyFloat_AsDouble(PyObject_CallMethod(PyServer_get_server(), "getGlobalDur", NULL)); \
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
        return PyInt_FromLong(-1); \
 \
    globdel = PyFloat_AsDouble(PyObject_CallMethod(PyServer_get_server(), "getGlobalDel", NULL)); \
    globdur = PyFloat_AsDouble(PyObject_CallMethod(PyServer_get_server(), "getGlobalDur", NULL)); \
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
        Stream_setStreamActive(self->stream, 0); \
        for (i=0; i<self->bufsize; i++) \
            self->data[i] = 0.0; \
        nearestBuf = (int)roundf((del * self->sr) / self->bufsize); \
        Stream_setBufferCountWait(self->stream, nearestBuf); \
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
        return PyInt_FromLong(-1); \
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
    Py_INCREF(Py_None); \
    return Py_None;

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
#if PY_MAJOR_VERSION < 3
#define TABLESTREAM_READ_WRITE_BUFFER \
    if ( index != 0 ) { \
        PySys_WriteStdout("Pyo error: Table buffer, accessing non-existent bytes segment."); \
        return -1; \
    } \
    *ptr = (void *)self->data; \
    return (Py_ssize_t)self->size * sizeof(MYFLT);

#define TABLESTREAM_SEG_COUNT \
    if ( lenp ) \
        *lenp = (Py_ssize_t)self->size * sizeof(MYFLT); \
    return 1;
#endif

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
