/*************************************************************************
 * Copyright 2010 Olivier Belanger                                        *                  
 *                                                                        * 
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *  
 *                                                                        * 
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    * 
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with pyo.  If not, see <http://www.gnu.org/licenses/>.           *
 *************************************************************************/

#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "tablemodule.h"
#include "interpolation.h"

MYFLT SINE_ARRAY[513] = {0.0, 0.012271538285719925, 0.024541228522912288, 0.036807222941358832, 0.049067674327418015, 0.061320736302208578, 0.073564563599667426, 0.085797312344439894, 0.098017140329560604, 0.11022220729388306, 0.1224106751992162, 0.13458070850712617, 0.14673047445536175, 0.15885814333386145, 0.17096188876030122, 0.18303988795514095, 0.19509032201612825, 0.20711137619221856, 0.2191012401568698, 0.23105810828067111, 0.24298017990326387, 0.25486565960451457, 0.26671275747489837, 0.27851968938505306, 0.29028467725446233, 0.30200594931922808, 0.31368174039889152, 0.32531029216226293, 0.33688985339222005, 0.34841868024943456, 0.35989503653498811, 0.37131719395183754, 0.38268343236508978, 0.3939920400610481, 0.40524131400498986, 0.41642956009763715, 0.42755509343028208, 0.43861623853852766, 0.44961132965460654, 0.46053871095824001, 0.47139673682599764, 0.48218377207912272, 0.49289819222978404, 0.50353838372571758, 0.51410274419322166, 0.52458968267846895, 0.53499761988709715, 0.54532498842204646, 0.55557023301960218, 0.56573181078361312, 0.57580819141784534, 0.58579785745643886, 0.59569930449243336, 0.60551104140432555, 0.61523159058062682, 0.62485948814238634, 0.63439328416364549, 0.64383154288979139, 0.65317284295377676, 0.66241577759017178, 0.67155895484701833, 0.68060099779545302, 0.68954054473706683, 0.69837624940897292, 0.70710678118654746, 0.71573082528381859, 0.72424708295146689, 0.7326542716724127, 0.74095112535495899, 0.74913639452345926, 0.75720884650648446, 0.76516726562245885, 0.77301045336273688, 0.78073722857209438, 0.78834642762660623, 0.79583690460888346, 0.80320753148064483, 0.81045719825259477, 0.81758481315158371, 0.82458930278502529, 0.83146961230254512, 0.83822470555483797, 0.84485356524970701, 0.8513551931052652, 0.85772861000027212, 0.8639728561215867, 0.87008699110871135, 0.87607009419540649, 0.88192126434835494, 0.88763962040285393, 0.89322430119551532, 0.89867446569395382, 0.90398929312344334, 0.90916798309052238, 0.91420975570353069, 0.91911385169005777, 0.92387953251128674, 0.92850608047321548, 0.93299279883473885, 0.93733901191257496, 0.94154406518302081, 0.94560732538052128, 0.94952818059303667, 0.95330604035419375, 0.95694033573220894, 0.96043051941556579, 0.96377606579543984, 0.96697647104485207, 0.97003125319454397, 0.97293995220556007, 0.97570213003852857, 0.97831737071962765, 0.98078528040323043, 0.98310548743121629, 0.98527764238894122, 0.98730141815785843, 0.98917650996478101, 0.99090263542778001, 0.99247953459870997, 0.99390697000235606, 0.99518472667219682, 0.996312612182778, 0.99729045667869021, 0.99811811290014918, 0.99879545620517241, 0.99932238458834954, 0.99969881869620425, 0.9999247018391445, 1.0, 0.9999247018391445, 0.99969881869620425, 0.99932238458834954, 0.99879545620517241, 0.99811811290014918, 0.99729045667869021, 0.996312612182778, 0.99518472667219693, 0.99390697000235606, 0.99247953459870997, 0.99090263542778001, 0.98917650996478101, 0.98730141815785843, 0.98527764238894122, 0.98310548743121629, 0.98078528040323043, 0.97831737071962765, 0.97570213003852857, 0.97293995220556018, 0.97003125319454397, 0.96697647104485207, 0.96377606579543984, 0.9604305194155659, 0.95694033573220894, 0.95330604035419386, 0.94952818059303667, 0.94560732538052139, 0.94154406518302081, 0.93733901191257496, 0.93299279883473885, 0.92850608047321559, 0.92387953251128674, 0.91911385169005777, 0.91420975570353069, 0.90916798309052249, 0.90398929312344345, 0.89867446569395393, 0.89322430119551521, 0.88763962040285393, 0.88192126434835505, 0.8760700941954066, 0.87008699110871146, 0.86397285612158681, 0.85772861000027212, 0.8513551931052652, 0.84485356524970723, 0.83822470555483819, 0.83146961230254546, 0.82458930278502529, 0.81758481315158371, 0.81045719825259477, 0.80320753148064494, 0.79583690460888357, 0.78834642762660634, 0.7807372285720946, 0.7730104533627371, 0.76516726562245907, 0.75720884650648479, 0.74913639452345926, 0.74095112535495899, 0.73265427167241282, 0.724247082951467, 0.71573082528381871, 0.70710678118654757, 0.69837624940897292, 0.68954054473706705, 0.68060099779545324, 0.67155895484701855, 0.66241577759017201, 0.65317284295377664, 0.64383154288979139, 0.63439328416364549, 0.62485948814238634, 0.61523159058062693, 0.60551104140432555, 0.59569930449243347, 0.58579785745643898, 0.57580819141784545, 0.56573181078361345, 0.55557023301960218, 0.54532498842204635, 0.53499761988709715, 0.52458968267846895, 0.51410274419322177, 0.50353838372571758, 0.49289819222978415, 0.48218377207912289, 0.47139673682599781, 0.46053871095824023, 0.44961132965460687, 0.43861623853852755, 0.42755509343028203, 0.41642956009763715, 0.40524131400498986, 0.39399204006104815, 0.38268343236508984, 0.37131719395183765, 0.35989503653498833, 0.34841868024943479, 0.33688985339222027, 0.3253102921622632, 0.31368174039889141, 0.30200594931922803, 0.29028467725446233, 0.27851968938505312, 0.26671275747489848, 0.25486565960451468, 0.24298017990326404, 0.2310581082806713, 0.21910124015687002, 0.20711137619221884, 0.19509032201612858, 0.1830398879551409, 0.17096188876030119, 0.15885814333386145, 0.1467304744553618, 0.13458070850712628, 0.12241067519921635, 0.11022220729388325, 0.09801714032956084, 0.085797312344440158, 0.073564563599667745, 0.061320736302208495, 0.049067674327417973, 0.036807222941358832, 0.024541228522912326, 0.012271538285720007, 1.2246467991473532e-16, -0.012271538285719761, -0.024541228522912083, -0.036807222941358582, -0.049067674327417724, -0.061320736302208245, -0.073564563599667496, -0.085797312344439922, -0.09801714032956059, -0.110222207293883, -0.1224106751992161, -0.13458070850712606, -0.14673047445536158, -0.15885814333386122, -0.17096188876030097, -0.18303988795514067, -0.19509032201612836, -0.20711137619221862, -0.21910124015686983, -0.23105810828067111, -0.24298017990326382, -0.25486565960451446, -0.26671275747489825, -0.27851968938505289, -0.29028467725446216, -0.30200594931922781, -0.31368174039889118, -0.32531029216226304, -0.33688985339222011, -0.34841868024943456, -0.35989503653498811, -0.37131719395183749, -0.38268343236508967, -0.39399204006104793, -0.40524131400498969, -0.41642956009763693, -0.42755509343028181, -0.43861623853852733, -0.44961132965460665, -0.46053871095824006, -0.47139673682599764, -0.48218377207912272, -0.49289819222978393, -0.50353838372571746, -0.51410274419322155, -0.52458968267846873, -0.53499761988709693, -0.54532498842204613, -0.55557023301960196, -0.56573181078361323, -0.57580819141784534, -0.58579785745643886, -0.59569930449243325, -0.60551104140432543, -0.61523159058062671, -0.62485948814238623, -0.63439328416364527, -0.64383154288979128, -0.65317284295377653, -0.66241577759017178, -0.67155895484701844, -0.68060099779545302, -0.68954054473706683, -0.6983762494089728, -0.70710678118654746, -0.71573082528381848, -0.72424708295146667, -0.73265427167241259, -0.74095112535495877, -0.74913639452345904, -0.75720884650648423, -0.76516726562245885, -0.77301045336273666, -0.78073722857209438, -0.78834642762660589, -0.79583690460888334, -0.80320753148064505, -0.81045719825259466, -0.81758481315158371, -0.82458930278502507, -0.83146961230254524, -0.83822470555483775, -0.84485356524970712, -0.85135519310526486, -0.85772861000027201, -0.86397285612158647, -0.87008699110871135, -0.87607009419540671, -0.88192126434835494, -0.88763962040285405, -0.89322430119551521, -0.89867446569395382, -0.90398929312344312, -0.90916798309052238, -0.91420975570353047, -0.91911385169005766, -0.92387953251128652, -0.92850608047321548, -0.93299279883473896, -0.93733901191257485, -0.94154406518302081, -0.94560732538052117, -0.94952818059303667, -0.95330604035419375, -0.95694033573220882, -0.96043051941556568, -0.96377606579543984, -0.96697647104485218, -0.97003125319454397, -0.97293995220556018, -0.97570213003852846, -0.97831737071962765, -0.98078528040323032, -0.98310548743121629, -0.98527764238894111, -0.98730141815785832, -0.9891765099647809, -0.99090263542778001, -0.99247953459871008, -0.99390697000235606, -0.99518472667219693, -0.996312612182778, -0.99729045667869021, -0.99811811290014918, -0.99879545620517241, -0.99932238458834943, -0.99969881869620425, -0.9999247018391445, -1.0, -0.9999247018391445, -0.99969881869620425, -0.99932238458834954, -0.99879545620517241, -0.99811811290014918, -0.99729045667869021, -0.996312612182778, -0.99518472667219693, -0.99390697000235606, -0.99247953459871008, -0.99090263542778001, -0.9891765099647809, -0.98730141815785843, -0.98527764238894122, -0.9831054874312164, -0.98078528040323043, -0.97831737071962777, -0.97570213003852857, -0.97293995220556029, -0.97003125319454397, -0.96697647104485229, -0.96377606579543995, -0.96043051941556579, -0.95694033573220894, -0.95330604035419375, -0.94952818059303679, -0.94560732538052128, -0.94154406518302092, -0.93733901191257496, -0.93299279883473907, -0.92850608047321559, -0.92387953251128663, -0.91911385169005788, -0.91420975570353058, -0.90916798309052249, -0.90398929312344334, -0.89867446569395404, -0.89322430119551532, -0.88763962040285416, -0.88192126434835505, -0.87607009419540693, -0.87008699110871146, -0.8639728561215867, -0.85772861000027223, -0.85135519310526508, -0.84485356524970734, -0.83822470555483797, -0.83146961230254557, -0.82458930278502529, -0.81758481315158404, -0.81045719825259488, -0.80320753148064528, -0.79583690460888368, -0.78834642762660612, -0.78073722857209471, -0.77301045336273688, -0.76516726562245918, -0.75720884650648457, -0.7491363945234597, -0.74095112535495922, -0.73265427167241315, -0.72424708295146711, -0.71573082528381904, -0.70710678118654768, -0.69837624940897269, -0.68954054473706716, -0.68060099779545302, -0.67155895484701866, -0.66241577759017178, -0.65317284295377709, -0.6438315428897915, -0.63439328416364593, -0.62485948814238645, -0.61523159058062737, -0.60551104140432566, -0.59569930449243325, -0.58579785745643909, -0.57580819141784523, -0.56573181078361356, -0.55557023301960218, -0.5453249884220468, -0.53499761988709726, -0.52458968267846939, -0.51410274419322188, -0.50353838372571813, -0.49289819222978426, -0.48218377207912261, -0.47139673682599792, -0.46053871095823995, -0.44961132965460698, -0.43861623853852766, -0.42755509343028253, -0.41642956009763726, -0.40524131400499042, -0.39399204006104827, -0.38268343236509039, -0.37131719395183777, -0.359895036534988, -0.3484186802494349, -0.33688985339222, -0.32531029216226331, -0.31368174039889152, -0.30200594931922853, -0.29028467725446244, -0.27851968938505367, -0.26671275747489859, -0.25486565960451435, -0.24298017990326418, -0.23105810828067103, -0.21910124015687016, -0.20711137619221853, -0.19509032201612872, -0.18303988795514103, -0.17096188876030177, -0.15885814333386158, -0.14673047445536239, -0.13458070850712642, -0.12241067519921603, -0.11022220729388338, -0.09801714032956052, -0.085797312344440282, -0.073564563599667426, -0.06132073630220905, -0.049067674327418091, -0.036807222941359394, -0.024541228522912451, -0.012271538285720572, 0.0};
MYFLT ONE_OVER_512 = 1.0 / 512.0;

static MYFLT
_clip(MYFLT x) {
    if (x < 0.0)
        return 0.0;
    else if (x >= 1.0)
        return 1.0;
    else
        return x;
}

static MYFLT
Sine_clip(MYFLT x) {
    if (x < 0) {
        x += ((int)(-x * ONE_OVER_512) + 1) * 512;
    }
    else if (x >= 512) {
        x -= (int)(x * ONE_OVER_512) * 512;
    }
    return x;
}

/* Sine object */
typedef struct {
    pyo_audio_HEAD
    PyObject *freq;
    Stream *freq_stream;
    PyObject *phase;
    Stream *phase_stream;
    int modebuffer[4];
    MYFLT pointerPos;
} Sine;

static void
Sine_readframes_ii(Sine *self) {
    MYFLT inc, fr, ph, pos, fpart;
    int i, ipart;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase) * 512;
    inc = fr * 512 / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos = Sine_clip(self->pointerPos);
        pos = self->pointerPos + ph;
        if (pos >= 512)
            pos -= 512;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos += inc;
    }
}

static void
Sine_readframes_ai(Sine *self) {
    MYFLT inc, ph, pos, fpart, fac;
    int i, ipart;
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase) * 512;
    
    fac = 512 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * fac;
        self->pointerPos = Sine_clip(self->pointerPos);
        pos = self->pointerPos + ph;
        if (pos >= 512)
            pos -= 512;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos += inc;
    }
}

static void
Sine_readframes_ia(Sine *self) {
    MYFLT inc, fr, pos, fpart;
    int i, ipart;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);
    inc = fr * 512 / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos = Sine_clip(self->pointerPos);
        pos = self->pointerPos + ph[i] * 512;
        if (pos >= 512)
            pos -= 512;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos += inc;
    }
}

static void
Sine_readframes_aa(Sine *self) {
    MYFLT inc, pos, fpart, fac;
    int i, ipart;
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);
    
    fac = 512 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * fac;
        self->pointerPos = Sine_clip(self->pointerPos);
        pos = self->pointerPos + ph[i] * 512;
        if (pos >= 512)
            pos -= 512;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos += inc;
    }
}

static void Sine_postprocessing_ii(Sine *self) { POST_PROCESSING_II };
static void Sine_postprocessing_ai(Sine *self) { POST_PROCESSING_AI };
static void Sine_postprocessing_ia(Sine *self) { POST_PROCESSING_IA };
static void Sine_postprocessing_aa(Sine *self) { POST_PROCESSING_AA };
static void Sine_postprocessing_ireva(Sine *self) { POST_PROCESSING_IREVA };
static void Sine_postprocessing_areva(Sine *self) { POST_PROCESSING_AREVA };
static void Sine_postprocessing_revai(Sine *self) { POST_PROCESSING_REVAI };
static void Sine_postprocessing_revaa(Sine *self) { POST_PROCESSING_REVAA };
static void Sine_postprocessing_revareva(Sine *self) { POST_PROCESSING_REVAREVA };

static void
Sine_setProcMode(Sine *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Sine_readframes_ii;
            break;
        case 1:    
            self->proc_func_ptr = Sine_readframes_ai;
            break;
        case 10:    
            self->proc_func_ptr = Sine_readframes_ia;
            break;
        case 11:    
            self->proc_func_ptr = Sine_readframes_aa;
            break;
    } 
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Sine_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Sine_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Sine_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Sine_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Sine_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Sine_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Sine_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Sine_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Sine_postprocessing_revareva;
            break;
    }
}

static void
Sine_compute_next_data_frame(Sine *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Sine_traverse(Sine *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->phase);    
    Py_VISIT(self->phase_stream);    
    return 0;
}

static int 
Sine_clear(Sine *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->phase);    
    Py_CLEAR(self->phase_stream);    
    return 0;
}

static void
Sine_dealloc(Sine* self)
{
    free(self->data);
    Sine_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Sine_deleteStream(Sine *self) { DELETE_STREAM };

static PyObject *
Sine_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Sine *self;
    self = (Sine *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->phase = PyFloat_FromDouble(0.0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->pointerPos = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Sine_compute_next_data_frame);
    self->mode_func_ptr = Sine_setProcMode;

    return (PyObject *)self;
}

static int
Sine_init(Sine *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *freqtmp=NULL, *phasetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"freq", "phase", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &freqtmp, &phasetmp, &multmp, &addtmp))
        return -1; 
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (phasetmp) {
        PyObject_CallMethod((PyObject *)self, "setPhase", "O", phasetmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Sine_getServer(Sine* self) { GET_SERVER };
static PyObject * Sine_getStream(Sine* self) { GET_STREAM };
static PyObject * Sine_setMul(Sine *self, PyObject *arg) { SET_MUL };	
static PyObject * Sine_setAdd(Sine *self, PyObject *arg) { SET_ADD };	
static PyObject * Sine_setSub(Sine *self, PyObject *arg) { SET_SUB };	
static PyObject * Sine_setDiv(Sine *self, PyObject *arg) { SET_DIV };	

static PyObject * Sine_play(Sine *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Sine_out(Sine *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Sine_stop(Sine *self) { STOP };

static PyObject * Sine_multiply(Sine *self, PyObject *arg) { MULTIPLY };
static PyObject * Sine_inplace_multiply(Sine *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Sine_add(Sine *self, PyObject *arg) { ADD };
static PyObject * Sine_inplace_add(Sine *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Sine_sub(Sine *self, PyObject *arg) { SUB };
static PyObject * Sine_inplace_sub(Sine *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Sine_div(Sine *self, PyObject *arg) { DIV };
static PyObject * Sine_inplace_div(Sine *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Sine_setFreq(Sine *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Sine_setPhase(Sine *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->phase);
	if (isNumber == 1) {
		self->phase = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->phase = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->phase, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->phase_stream);
        self->phase_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Sine_members[] = {
{"server", T_OBJECT_EX, offsetof(Sine, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Sine, stream), 0, "Stream object."},
{"freq", T_OBJECT_EX, offsetof(Sine, freq), 0, "Frequency in cycle per second."},
{"phase", T_OBJECT_EX, offsetof(Sine, phase), 0, "Phase of signal (0 -> 1)"},
{"mul", T_OBJECT_EX, offsetof(Sine, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Sine, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Sine_methods[] = {
{"getServer", (PyCFunction)Sine_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Sine_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Sine_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Sine_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Sine_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Sine_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Sine_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
{"setPhase", (PyCFunction)Sine_setPhase, METH_O, "Sets oscillator phase between 0 and 1."},
{"setMul", (PyCFunction)Sine_setMul, METH_O, "Sets Sine mul factor."},
{"setAdd", (PyCFunction)Sine_setAdd, METH_O, "Sets Sine add factor."},
{"setSub", (PyCFunction)Sine_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Sine_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Sine_as_number = {
(binaryfunc)Sine_add,                      /*nb_add*/
(binaryfunc)Sine_sub,                 /*nb_subtract*/
(binaryfunc)Sine_multiply,                 /*nb_multiply*/
(binaryfunc)Sine_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)Sine_inplace_add,              /*inplace_add*/
(binaryfunc)Sine_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Sine_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Sine_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject SineType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Sine_base",         /*tp_name*/
sizeof(Sine),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Sine_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Sine_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"Sine objects. Generates a sinewave.",           /* tp_doc */
(traverseproc)Sine_traverse,   /* tp_traverse */
(inquiry)Sine_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Sine_methods,             /* tp_methods */
Sine_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Sine_init,      /* tp_init */
0,                         /* tp_alloc */
Sine_new,                 /* tp_new */
};

/*******************/
/* SineLoop object */
/*******************/
typedef struct {
    pyo_audio_HEAD
    PyObject *freq;
    Stream *freq_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    int modebuffer[4];
    MYFLT pointerPos;
    MYFLT lastValue;
} SineLoop;

static void
SineLoop_readframes_ii(SineLoop *self) {
    MYFLT inc, fr, feed, pos, fpart;
    int i, ipart;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    feed = _clip(PyFloat_AS_DOUBLE(self->feedback)) * 512;
    inc = fr * 512 / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos = Sine_clip(self->pointerPos);
        pos = Sine_clip(self->pointerPos + self->lastValue * feed);
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = self->lastValue = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos += inc;
    }
}

static void
SineLoop_readframes_ai(SineLoop *self) {
    MYFLT inc, feed, pos, fpart, fac;
    int i, ipart;
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    feed = _clip(PyFloat_AS_DOUBLE(self->feedback)) * 512;
    
    fac = 512 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * fac;
        self->pointerPos = Sine_clip(self->pointerPos);
        pos = Sine_clip(self->pointerPos + self->lastValue * feed);
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = self->lastValue = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos += inc;
    }
}

static void
SineLoop_readframes_ia(SineLoop *self) {
    MYFLT inc, fr, feed, pos, fpart;
    int i, ipart;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *fd = Stream_getData((Stream *)self->feedback_stream);
    inc = fr * 512 / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        feed = _clip(fd[i]) * 512;
        self->pointerPos = Sine_clip(self->pointerPos);
        pos = Sine_clip(self->pointerPos + self->lastValue * feed);
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = self->lastValue = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos += inc;
    }
}

static void
SineLoop_readframes_aa(SineLoop *self) {
    MYFLT inc, feed, pos, fpart, fac;
    int i, ipart;
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *fd = Stream_getData((Stream *)self->feedback_stream);
    
    fac = 512 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * fac;
        feed = _clip(fd[i]) * 512;
        self->pointerPos = Sine_clip(self->pointerPos);
        pos = Sine_clip(self->pointerPos + self->lastValue * feed);
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = self->lastValue = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos += inc;
    }
}

static void SineLoop_postprocessing_ii(SineLoop *self) { POST_PROCESSING_II };
static void SineLoop_postprocessing_ai(SineLoop *self) { POST_PROCESSING_AI };
static void SineLoop_postprocessing_ia(SineLoop *self) { POST_PROCESSING_IA };
static void SineLoop_postprocessing_aa(SineLoop *self) { POST_PROCESSING_AA };
static void SineLoop_postprocessing_ireva(SineLoop *self) { POST_PROCESSING_IREVA };
static void SineLoop_postprocessing_areva(SineLoop *self) { POST_PROCESSING_AREVA };
static void SineLoop_postprocessing_revai(SineLoop *self) { POST_PROCESSING_REVAI };
static void SineLoop_postprocessing_revaa(SineLoop *self) { POST_PROCESSING_REVAA };
static void SineLoop_postprocessing_revareva(SineLoop *self) { POST_PROCESSING_REVAREVA };

static void
SineLoop_setProcMode(SineLoop *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = SineLoop_readframes_ii;
            break;
        case 1:    
            self->proc_func_ptr = SineLoop_readframes_ai;
            break;
        case 10:    
            self->proc_func_ptr = SineLoop_readframes_ia;
            break;
        case 11:    
            self->proc_func_ptr = SineLoop_readframes_aa;
            break;
    } 
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = SineLoop_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = SineLoop_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = SineLoop_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = SineLoop_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = SineLoop_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = SineLoop_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = SineLoop_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = SineLoop_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = SineLoop_postprocessing_revareva;
            break;
    }
}

static void
SineLoop_compute_next_data_frame(SineLoop *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
SineLoop_traverse(SineLoop *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->feedback);    
    Py_VISIT(self->feedback_stream);    
    return 0;
}

static int 
SineLoop_clear(SineLoop *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->feedback);    
    Py_CLEAR(self->feedback_stream);    
    return 0;
}

static void
SineLoop_dealloc(SineLoop* self)
{
    free(self->data);
    SineLoop_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SineLoop_deleteStream(SineLoop *self) { DELETE_STREAM };

static PyObject *
SineLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    SineLoop *self;
    self = (SineLoop *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->feedback = PyFloat_FromDouble(0.0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->pointerPos = self->lastValue = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SineLoop_compute_next_data_frame);
    self->mode_func_ptr = SineLoop_setProcMode;
    
    return (PyObject *)self;
}

static int
SineLoop_init(SineLoop *self, PyObject *args, PyObject *kwds)
{
    PyObject *freqtmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"freq", "feedback", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &freqtmp, &feedbacktmp, &multmp, &addtmp))
        return -1; 
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * SineLoop_getServer(SineLoop* self) { GET_SERVER };
static PyObject * SineLoop_getStream(SineLoop* self) { GET_STREAM };
static PyObject * SineLoop_setMul(SineLoop *self, PyObject *arg) { SET_MUL };	
static PyObject * SineLoop_setAdd(SineLoop *self, PyObject *arg) { SET_ADD };	
static PyObject * SineLoop_setSub(SineLoop *self, PyObject *arg) { SET_SUB };	
static PyObject * SineLoop_setDiv(SineLoop *self, PyObject *arg) { SET_DIV };	

static PyObject * SineLoop_play(SineLoop *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SineLoop_out(SineLoop *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SineLoop_stop(SineLoop *self) { STOP };

static PyObject * SineLoop_multiply(SineLoop *self, PyObject *arg) { MULTIPLY };
static PyObject * SineLoop_inplace_multiply(SineLoop *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SineLoop_add(SineLoop *self, PyObject *arg) { ADD };
static PyObject * SineLoop_inplace_add(SineLoop *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SineLoop_sub(SineLoop *self, PyObject *arg) { SUB };
static PyObject * SineLoop_inplace_sub(SineLoop *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SineLoop_div(SineLoop *self, PyObject *arg) { DIV };
static PyObject * SineLoop_inplace_div(SineLoop *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
SineLoop_setFreq(SineLoop *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
SineLoop_setFeedback(SineLoop *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feedback);
	if (isNumber == 1) {
		self->feedback = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef SineLoop_members[] = {
	{"server", T_OBJECT_EX, offsetof(SineLoop, server), 0, "Pyo server."},
	{"stream", T_OBJECT_EX, offsetof(SineLoop, stream), 0, "Stream object."},
	{"freq", T_OBJECT_EX, offsetof(SineLoop, freq), 0, "Frequency in cycle per second."},
	{"feedback", T_OBJECT_EX, offsetof(SineLoop, feedback), 0, "Phase of signal (0 -> 1)"},
	{"mul", T_OBJECT_EX, offsetof(SineLoop, mul), 0, "Mul factor."},
	{"add", T_OBJECT_EX, offsetof(SineLoop, add), 0, "Add factor."},
	{NULL}  /* Sentinel */
};

static PyMethodDef SineLoop_methods[] = {
	{"getServer", (PyCFunction)SineLoop_getServer, METH_NOARGS, "Returns server object."},
	{"_getStream", (PyCFunction)SineLoop_getStream, METH_NOARGS, "Returns stream object."},
	{"deleteStream", (PyCFunction)SineLoop_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
	{"play", (PyCFunction)SineLoop_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
	{"out", (PyCFunction)SineLoop_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
	{"stop", (PyCFunction)SineLoop_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)SineLoop_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
	{"setFeedback", (PyCFunction)SineLoop_setFeedback, METH_O, "Sets oscillator feedback between 0 and 1."},
	{"setMul", (PyCFunction)SineLoop_setMul, METH_O, "Sets SineLoop mul factor."},
	{"setAdd", (PyCFunction)SineLoop_setAdd, METH_O, "Sets SineLoop add factor."},
	{"setSub", (PyCFunction)SineLoop_setSub, METH_O, "Sets inverse add factor."},
	{"setDiv", (PyCFunction)SineLoop_setDiv, METH_O, "Sets inverse mul factor."},
	{NULL}  /* Sentinel */
};

static PyNumberMethods SineLoop_as_number = {
	(binaryfunc)SineLoop_add,                      /*nb_add*/
	(binaryfunc)SineLoop_sub,                 /*nb_subtract*/
	(binaryfunc)SineLoop_multiply,                 /*nb_multiply*/
	(binaryfunc)SineLoop_div,                   /*nb_divide*/
	0,                /*nb_remainder*/
	0,                   /*nb_divmod*/
	0,                   /*nb_power*/
	0,                  /*nb_neg*/
	0,                /*nb_pos*/
	0,                  /*(unaryfunc)array_abs*/
	0,                    /*nb_nonzero*/
	0,                    /*nb_invert*/
	0,               /*nb_lshift*/
	0,              /*nb_rshift*/
	0,              /*nb_and*/
	0,              /*nb_xor*/
	0,               /*nb_or*/
	0,                                          /*nb_coerce*/
	0,                       /*nb_int*/
	0,                      /*nb_long*/
	0,                     /*nb_float*/
	0,                       /*nb_oct*/
	0,                       /*nb_hex*/
	(binaryfunc)SineLoop_inplace_add,              /*inplace_add*/
	(binaryfunc)SineLoop_inplace_sub,         /*inplace_subtract*/
	(binaryfunc)SineLoop_inplace_multiply,         /*inplace_multiply*/
	(binaryfunc)SineLoop_inplace_div,           /*inplace_divide*/
	0,        /*inplace_remainder*/
	0,           /*inplace_power*/
	0,       /*inplace_lshift*/
	0,      /*inplace_rshift*/
	0,      /*inplace_and*/
	0,      /*inplace_xor*/
	0,       /*inplace_or*/
	0,             /*nb_floor_divide*/
	0,              /*nb_true_divide*/
	0,     /*nb_inplace_floor_divide*/
	0,      /*nb_inplace_true_divide*/
	0,                     /* nb_index */
};

PyTypeObject SineLoopType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"_pyo.SineLoop_base",         /*tp_name*/
	sizeof(SineLoop),         /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)SineLoop_dealloc, /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	&SineLoop_as_number,             /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
	"SineLoop objects. Generates a looped sinewave.",           /* tp_doc */
	(traverseproc)SineLoop_traverse,   /* tp_traverse */
	(inquiry)SineLoop_clear,           /* tp_clear */
	0,		               /* tp_richcompare */
	0,		               /* tp_weaklistoffset */
	0,		               /* tp_iter */
	0,		               /* tp_iternext */
	SineLoop_methods,             /* tp_methods */
	SineLoop_members,             /* tp_members */
	0,                      /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)SineLoop_init,      /* tp_init */
	0,                         /* tp_alloc */
	SineLoop_new,                 /* tp_new */
};

/***************/
/* Osc objects */
/***************/
static MYFLT
Osc_clip(MYFLT x, int size) {
    if (x < 0) {
        x += ((int)(-x / size) + 1) * size;
    }
    else if (x >= size) {
        x -= (int)(x / size) * size;
    }
    return x;
}

typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *phase;
    Stream *phase_stream;
    int modebuffer[4];
    MYFLT pointerPos;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    MYFLT (*interp_func_ptr)(MYFLT *, int, MYFLT, int);
} Osc;

static void
Osc_readframes_ii(Osc *self) {
    MYFLT fr, ph, pos, inc, fpart;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);

    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase);
    inc = fr * size / self->sr;

    ph *= size;
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        self->pointerPos = Osc_clip(self->pointerPos, size);
        pos = self->pointerPos + ph;
        if (pos >= size)
            pos -= size;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
    }
}

static void
Osc_readframes_ai(Osc *self) {
    MYFLT inc, ph, pos, fpart, sizeOnSr;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase);
    ph *= size;
    
    sizeOnSr = size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * sizeOnSr;
        self->pointerPos += inc;
        self->pointerPos = Osc_clip(self->pointerPos, size);
        pos = self->pointerPos + ph;
        if (pos >= size)
            pos -= size;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
    }
}

static void
Osc_readframes_ia(Osc *self) {
    MYFLT fr, pha, pos, inc, fpart;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);
    inc = fr * size / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        pha = ph[i] * size;
        self->pointerPos += inc;
        self->pointerPos = Osc_clip(self->pointerPos, size);
        pos = self->pointerPos + pha;
        if (pos >= size)
            pos -= size;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
    }
}

static void
Osc_readframes_aa(Osc *self) {
    MYFLT inc, pha, pos, fpart, sizeOnSr;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);

    sizeOnSr = size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * sizeOnSr;
        pha = ph[i] * size;
        self->pointerPos += inc;
        self->pointerPos = Osc_clip(self->pointerPos, size);
        pos = self->pointerPos + pha;
        if (pos >= size)
            pos -= size;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
    }
}

static void Osc_postprocessing_ii(Osc *self) { POST_PROCESSING_II };
static void Osc_postprocessing_ai(Osc *self) { POST_PROCESSING_AI };
static void Osc_postprocessing_ia(Osc *self) { POST_PROCESSING_IA };
static void Osc_postprocessing_aa(Osc *self) { POST_PROCESSING_AA };
static void Osc_postprocessing_ireva(Osc *self) { POST_PROCESSING_IREVA };
static void Osc_postprocessing_areva(Osc *self) { POST_PROCESSING_AREVA };
static void Osc_postprocessing_revai(Osc *self) { POST_PROCESSING_REVAI };
static void Osc_postprocessing_revaa(Osc *self) { POST_PROCESSING_REVAA };
static void Osc_postprocessing_revareva(Osc *self) { POST_PROCESSING_REVAREVA };

static void
Osc_setProcMode(Osc *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Osc_readframes_ii;
            break;
        case 1:    
            self->proc_func_ptr = Osc_readframes_ai;
            break;
        case 10:        
            self->proc_func_ptr = Osc_readframes_ia;
            break;
        case 11:    
            self->proc_func_ptr = Osc_readframes_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Osc_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Osc_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Osc_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Osc_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Osc_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Osc_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Osc_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Osc_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Osc_postprocessing_revareva;
            break;
    } 
}

static void
Osc_compute_next_data_frame(Osc *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Osc_traverse(Osc *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->phase);    
    Py_VISIT(self->phase_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Osc_clear(Osc *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->phase);    
    Py_CLEAR(self->phase_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
Osc_dealloc(Osc* self)
{
    free(self->data);
    Osc_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Osc_deleteStream(Osc *self) { DELETE_STREAM };

static PyObject *
Osc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Osc *self;
    self = (Osc *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->phase = PyFloat_FromDouble(0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->pointerPos = 0.;
    self->interp = 2;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Osc_compute_next_data_frame);
    self->mode_func_ptr = Osc_setProcMode;

    return (PyObject *)self;
}

static int
Osc_init(Osc *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *freqtmp=NULL, *phasetmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"table", "freq", "phase", "interp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiOO", kwlist, &tabletmp, &freqtmp, &phasetmp, &self->interp, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (phasetmp) {
        PyObject_CallMethod((PyObject *)self, "setPhase", "O", phasetmp);
    }

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    SET_INTERP_POINTER

    Py_INCREF(self);
    return 0;
}

static PyObject * Osc_getServer(Osc* self) { GET_SERVER };
static PyObject * Osc_getStream(Osc* self) { GET_STREAM };
static PyObject * Osc_setMul(Osc *self, PyObject *arg) { SET_MUL };	
static PyObject * Osc_setAdd(Osc *self, PyObject *arg) { SET_ADD };	
static PyObject * Osc_setSub(Osc *self, PyObject *arg) { SET_SUB };	
static PyObject * Osc_setDiv(Osc *self, PyObject *arg) { SET_DIV };	

static PyObject * Osc_play(Osc *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Osc_out(Osc *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Osc_stop(Osc *self) { STOP };

static PyObject * Osc_multiply(Osc *self, PyObject *arg) { MULTIPLY };
static PyObject * Osc_inplace_multiply(Osc *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Osc_add(Osc *self, PyObject *arg) { ADD };
static PyObject * Osc_inplace_add(Osc *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Osc_sub(Osc *self, PyObject *arg) { SUB };
static PyObject * Osc_inplace_sub(Osc *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Osc_div(Osc *self, PyObject *arg) { DIV };
static PyObject * Osc_inplace_div(Osc *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Osc_getTable(Osc* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Osc_setTable(Osc *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Osc_setFreq(Osc *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Osc_setPhase(Osc *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->phase);
	if (isNumber == 1) {
		self->phase = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->phase = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->phase, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->phase_stream);
        self->phase_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Osc_setInterp(Osc *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isNumber = PyNumber_Check(arg);
    
	if (isNumber == 1) {
		self->interp = PyInt_AsLong(PyNumber_Int(arg));
    }  
    
    SET_INTERP_POINTER
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Osc_members[] = {
    {"server", T_OBJECT_EX, offsetof(Osc, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Osc, stream), 0, "Stream object."},
    {"table", T_OBJECT_EX, offsetof(Osc, table), 0, "Waveform table."},
    {"freq", T_OBJECT_EX, offsetof(Osc, freq), 0, "Frequency in cycle per second."},
    {"phase", T_OBJECT_EX, offsetof(Osc, phase), 0, "Oscillator phase."},
    {"mul", T_OBJECT_EX, offsetof(Osc, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Osc, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Osc_methods[] = {
    {"getTable", (PyCFunction)Osc_getTable, METH_NOARGS, "Returns waveform table object."},
    {"getServer", (PyCFunction)Osc_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Osc_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Osc_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Osc_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Osc_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Osc_stop, METH_NOARGS, "Stops computing."},
    {"setTable", (PyCFunction)Osc_setTable, METH_O, "Sets oscillator table."},
	{"setFreq", (PyCFunction)Osc_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
    {"setPhase", (PyCFunction)Osc_setPhase, METH_O, "Sets oscillator phase."},
    {"setInterp", (PyCFunction)Osc_setInterp, METH_O, "Sets oscillator interpolation mode."},
	{"setMul", (PyCFunction)Osc_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Osc_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Osc_setSub, METH_O, "Sets oscillator inverse add factor."},
    {"setDiv", (PyCFunction)Osc_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Osc_as_number = {
    (binaryfunc)Osc_add,                      /*nb_add*/
    (binaryfunc)Osc_sub,                 /*nb_subtract*/
    (binaryfunc)Osc_multiply,                 /*nb_multiply*/
    (binaryfunc)Osc_div,                   /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)Osc_inplace_add,              /*inplace_add*/
    (binaryfunc)Osc_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Osc_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Osc_inplace_div,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject OscType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Osc_base",         /*tp_name*/
    sizeof(Osc),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Osc_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Osc_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Osc objects. Generates an oscillatory waveform.",           /* tp_doc */
    (traverseproc)Osc_traverse,   /* tp_traverse */
    (inquiry)Osc_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Osc_methods,             /* tp_methods */
    Osc_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Osc_init,      /* tp_init */
    0,                         /* tp_alloc */
    Osc_new,                 /* tp_new */
};

/**************/
/* OscLoop object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    int modebuffer[4];
    MYFLT pointerPos;
    MYFLT lastValue;
} OscLoop;

static void
OscLoop_readframes_ii(OscLoop *self) {
    MYFLT fr, feed, pos, inc, fpart;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    feed = _clip(PyFloat_AS_DOUBLE(self->feedback)) * size;
    inc = fr * size / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        self->pointerPos = Osc_clip(self->pointerPos, size);
        pos = self->pointerPos + (self->lastValue * feed);
        if (pos >= size)
            pos -= size;
        else if (pos < 0)
            pos += size;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = self->lastValue = tablelist[ipart] * (1.0 - fpart) + tablelist[ipart+1] * fpart;
    }
}

static void
OscLoop_readframes_ai(OscLoop *self) {
    MYFLT inc, feed, pos, fpart, sizeOnSr;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    feed = _clip(PyFloat_AS_DOUBLE(self->feedback)) * size;
    
    sizeOnSr = size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * sizeOnSr;
        self->pointerPos += inc;
        self->pointerPos = Osc_clip(self->pointerPos, size);
        pos = self->pointerPos + (self->lastValue * feed);
        if (pos >= size)
            pos -= size;
        else if (pos < 0)
            pos += size;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = self->lastValue = tablelist[ipart] * (1.0 - fpart) + tablelist[ipart+1] * fpart;
    }
}

static void
OscLoop_readframes_ia(OscLoop *self) {
    MYFLT fr, feed, pos, inc, fpart;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *fd = Stream_getData((Stream *)self->feedback_stream);
    inc = fr * size / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        feed = _clip(fd[i]) * size;
        self->pointerPos += inc;
        self->pointerPos = Osc_clip(self->pointerPos, size);
        pos = self->pointerPos + (self->lastValue * feed);
        if (pos >= size)
            pos -= size;
        else if (pos < 0)
            pos += size;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = self->lastValue = tablelist[ipart] * (1.0 - fpart) + tablelist[ipart+1] * fpart;
    }
}

static void
OscLoop_readframes_aa(OscLoop *self) {
    MYFLT inc, feed, pos, fpart, sizeOnSr;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *fd = Stream_getData((Stream *)self->feedback_stream);
    
    sizeOnSr = size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * sizeOnSr;
        feed = _clip(fd[i]) * size;
        self->pointerPos += inc;
        self->pointerPos = Osc_clip(self->pointerPos, size);
        pos = self->pointerPos + (self->lastValue * feed);
        if (pos >= size)
            pos -= size;
        else if (pos < 0)
            pos += size;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->data[i] = self->lastValue = tablelist[ipart] * (1.0 - fpart) + tablelist[ipart+1] * fpart;
    }
}

static void OscLoop_postprocessing_ii(OscLoop *self) { POST_PROCESSING_II };
static void OscLoop_postprocessing_ai(OscLoop *self) { POST_PROCESSING_AI };
static void OscLoop_postprocessing_ia(OscLoop *self) { POST_PROCESSING_IA };
static void OscLoop_postprocessing_aa(OscLoop *self) { POST_PROCESSING_AA };
static void OscLoop_postprocessing_ireva(OscLoop *self) { POST_PROCESSING_IREVA };
static void OscLoop_postprocessing_areva(OscLoop *self) { POST_PROCESSING_AREVA };
static void OscLoop_postprocessing_revai(OscLoop *self) { POST_PROCESSING_REVAI };
static void OscLoop_postprocessing_revaa(OscLoop *self) { POST_PROCESSING_REVAA };
static void OscLoop_postprocessing_revareva(OscLoop *self) { POST_PROCESSING_REVAREVA };

static void
OscLoop_setProcMode(OscLoop *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = OscLoop_readframes_ii;
            break;
        case 1:    
            self->proc_func_ptr = OscLoop_readframes_ai;
            break;
        case 10:        
            self->proc_func_ptr = OscLoop_readframes_ia;
            break;
        case 11:    
            self->proc_func_ptr = OscLoop_readframes_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = OscLoop_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = OscLoop_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = OscLoop_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = OscLoop_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = OscLoop_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = OscLoop_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = OscLoop_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = OscLoop_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = OscLoop_postprocessing_revareva;
            break;
    } 
}

static void
OscLoop_compute_next_data_frame(OscLoop *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
OscLoop_traverse(OscLoop *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->feedback);    
    Py_VISIT(self->feedback_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
OscLoop_clear(OscLoop *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->feedback);    
    Py_CLEAR(self->feedback_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
OscLoop_dealloc(OscLoop* self)
{
    free(self->data);
    OscLoop_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * OscLoop_deleteStream(OscLoop *self) { DELETE_STREAM };

static PyObject *
OscLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    OscLoop *self;
    self = (OscLoop *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->feedback = PyFloat_FromDouble(0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->pointerPos = self->lastValue = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscLoop_compute_next_data_frame);
    self->mode_func_ptr = OscLoop_setProcMode;
    
    return (PyObject *)self;
}

static int
OscLoop_init(OscLoop *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *freqtmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "freq", "feedback", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &tabletmp, &freqtmp, &feedbacktmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * OscLoop_getServer(OscLoop* self) { GET_SERVER };
static PyObject * OscLoop_getStream(OscLoop* self) { GET_STREAM };
static PyObject * OscLoop_setMul(OscLoop *self, PyObject *arg) { SET_MUL };	
static PyObject * OscLoop_setAdd(OscLoop *self, PyObject *arg) { SET_ADD };	
static PyObject * OscLoop_setSub(OscLoop *self, PyObject *arg) { SET_SUB };	
static PyObject * OscLoop_setDiv(OscLoop *self, PyObject *arg) { SET_DIV };	

static PyObject * OscLoop_play(OscLoop *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscLoop_out(OscLoop *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * OscLoop_stop(OscLoop *self) { STOP };

static PyObject * OscLoop_multiply(OscLoop *self, PyObject *arg) { MULTIPLY };
static PyObject * OscLoop_inplace_multiply(OscLoop *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscLoop_add(OscLoop *self, PyObject *arg) { ADD };
static PyObject * OscLoop_inplace_add(OscLoop *self, PyObject *arg) { INPLACE_ADD };
static PyObject * OscLoop_sub(OscLoop *self, PyObject *arg) { SUB };
static PyObject * OscLoop_inplace_sub(OscLoop *self, PyObject *arg) { INPLACE_SUB };
static PyObject * OscLoop_div(OscLoop *self, PyObject *arg) { DIV };
static PyObject * OscLoop_inplace_div(OscLoop *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
OscLoop_getTable(OscLoop* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
OscLoop_setTable(OscLoop *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
OscLoop_setFreq(OscLoop *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
OscLoop_setFeedback(OscLoop *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feedback);
	if (isNumber == 1) {
		self->feedback = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef OscLoop_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscLoop, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscLoop, stream), 0, "Stream object."},
    {"table", T_OBJECT_EX, offsetof(OscLoop, table), 0, "Waveform table."},
    {"freq", T_OBJECT_EX, offsetof(OscLoop, freq), 0, "Frequency in cycle per second."},
    {"feedback", T_OBJECT_EX, offsetof(OscLoop, feedback), 0, "Oscillator feedback."},
    {"mul", T_OBJECT_EX, offsetof(OscLoop, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(OscLoop, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscLoop_methods[] = {
    {"getTable", (PyCFunction)OscLoop_getTable, METH_NOARGS, "Returns waveform table object."},
    {"getServer", (PyCFunction)OscLoop_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscLoop_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)OscLoop_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)OscLoop_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)OscLoop_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)OscLoop_stop, METH_NOARGS, "Stops computing."},
    {"setTable", (PyCFunction)OscLoop_setTable, METH_O, "Sets oscillator table."},
	{"setFreq", (PyCFunction)OscLoop_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
    {"setFeedback", (PyCFunction)OscLoop_setFeedback, METH_O, "Sets oscillator feedback."},
	{"setMul", (PyCFunction)OscLoop_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)OscLoop_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)OscLoop_setSub, METH_O, "Sets oscillator inverse add factor."},
    {"setDiv", (PyCFunction)OscLoop_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods OscLoop_as_number = {
    (binaryfunc)OscLoop_add,                      /*nb_add*/
    (binaryfunc)OscLoop_sub,                 /*nb_subtract*/
    (binaryfunc)OscLoop_multiply,                 /*nb_multiply*/
    (binaryfunc)OscLoop_div,                   /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)OscLoop_inplace_add,              /*inplace_add*/
    (binaryfunc)OscLoop_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)OscLoop_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)OscLoop_inplace_div,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject OscLoopType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.OscLoop_base",         /*tp_name*/
    sizeof(OscLoop),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)OscLoop_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &OscLoop_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "OscLoop objects. Generates an oscillatory waveform.",           /* tp_doc */
    (traverseproc)OscLoop_traverse,   /* tp_traverse */
    (inquiry)OscLoop_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    OscLoop_methods,             /* tp_methods */
    OscLoop_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)OscLoop_init,      /* tp_init */
    0,                         /* tp_alloc */
    OscLoop_new,                 /* tp_new */
};

/**************/
/* Phasor object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *freq;
    Stream *freq_stream;
    PyObject *phase;
    Stream *phase_stream;
    int modebuffer[4];
    MYFLT pointerPos;
} Phasor;

static void
Phasor_readframes_ii(Phasor *self) {
    MYFLT fr, ph, pos, inc;
    int i;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = _clip(PyFloat_AS_DOUBLE(self->phase));
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        pos = self->pointerPos + ph;
        if (pos > 1)
            pos -= 1.0;
        self->data[i] = pos;
        
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
}

static void
Phasor_readframes_ai(Phasor *self) {
    MYFLT inc, ph, pos, oneOnSr;
    int i;
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    ph = _clip(PyFloat_AS_DOUBLE(self->phase));
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        pos = self->pointerPos + ph;
        if (pos > 1)
            pos -= 1.0;
        self->data[i] = pos;
        
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
}

static void
Phasor_readframes_ia(Phasor *self) {
    MYFLT fr, pha, pos, inc;
    int i;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);
    
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        pha = _clip(ph[i]);

        pos = self->pointerPos + pha;
        if (pos > 1)
            pos -= 1.0;
        self->data[i] = pos;
        
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
}

static void
Phasor_readframes_aa(Phasor *self) {
    MYFLT pha, pos, inc, oneOnSr;
    int i;
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);

    oneOnSr = 1.0 / self->sr;

    for (i=0; i<self->bufsize; i++) {
        pha = _clip(ph[i]);
        
        pos = self->pointerPos + pha;
        if (pos > 1)
            pos -= 1.0;
        self->data[i] = pos;
        
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
}

static void Phasor_postprocessing_ii(Phasor *self) { POST_PROCESSING_II };
static void Phasor_postprocessing_ai(Phasor *self) { POST_PROCESSING_AI };
static void Phasor_postprocessing_ia(Phasor *self) { POST_PROCESSING_IA };
static void Phasor_postprocessing_aa(Phasor *self) { POST_PROCESSING_AA };
static void Phasor_postprocessing_ireva(Phasor *self) { POST_PROCESSING_IREVA };
static void Phasor_postprocessing_areva(Phasor *self) { POST_PROCESSING_AREVA };
static void Phasor_postprocessing_revai(Phasor *self) { POST_PROCESSING_REVAI };
static void Phasor_postprocessing_revaa(Phasor *self) { POST_PROCESSING_REVAA };
static void Phasor_postprocessing_revareva(Phasor *self) { POST_PROCESSING_REVAREVA };

static void
Phasor_setProcMode(Phasor *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Phasor_readframes_ii;
            break;
        case 1:    
            self->proc_func_ptr = Phasor_readframes_ai;
            break;
        case 10:        
            self->proc_func_ptr = Phasor_readframes_ia;
            break;
        case 11:    
            self->proc_func_ptr = Phasor_readframes_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Phasor_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Phasor_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Phasor_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Phasor_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Phasor_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Phasor_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Phasor_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Phasor_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Phasor_postprocessing_revareva;
            break;
    } 
}

static void
Phasor_compute_next_data_frame(Phasor *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Phasor_traverse(Phasor *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->phase);    
    Py_VISIT(self->phase_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Phasor_clear(Phasor *self)
{
    pyo_CLEAR
    Py_CLEAR(self->phase);    
    Py_CLEAR(self->phase_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
Phasor_dealloc(Phasor* self)
{
    free(self->data);
    Phasor_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Phasor_deleteStream(Phasor *self) { DELETE_STREAM };

static PyObject *
Phasor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Phasor *self;
    self = (Phasor *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->phase = PyFloat_FromDouble(0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->pointerPos = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Phasor_compute_next_data_frame);
    self->mode_func_ptr = Phasor_setProcMode;
    
    return (PyObject *)self;
}

static int
Phasor_init(Phasor *self, PyObject *args, PyObject *kwds)
{
    PyObject *freqtmp=NULL, *phasetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"freq", "phase", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &freqtmp, &phasetmp, &multmp, &addtmp))
        return -1; 
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (phasetmp) {
        PyObject_CallMethod((PyObject *)self, "setPhase", "O", phasetmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Phasor_getServer(Phasor* self) { GET_SERVER };
static PyObject * Phasor_getStream(Phasor* self) { GET_STREAM };
static PyObject * Phasor_setMul(Phasor *self, PyObject *arg) { SET_MUL };	
static PyObject * Phasor_setAdd(Phasor *self, PyObject *arg) { SET_ADD };	
static PyObject * Phasor_setSub(Phasor *self, PyObject *arg) { SET_SUB };	
static PyObject * Phasor_setDiv(Phasor *self, PyObject *arg) { SET_DIV };	

static PyObject * Phasor_play(Phasor *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Phasor_out(Phasor *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Phasor_stop(Phasor *self) { STOP };

static PyObject * Phasor_multiply(Phasor *self, PyObject *arg) { MULTIPLY };
static PyObject * Phasor_inplace_multiply(Phasor *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Phasor_add(Phasor *self, PyObject *arg) { ADD };
static PyObject * Phasor_inplace_add(Phasor *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Phasor_sub(Phasor *self, PyObject *arg) { SUB };
static PyObject * Phasor_inplace_sub(Phasor *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Phasor_div(Phasor *self, PyObject *arg) { DIV };
static PyObject * Phasor_inplace_div(Phasor *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Phasor_setFreq(Phasor *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Phasor_setPhase(Phasor *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->phase);
	if (isNumber == 1) {
		self->phase = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->phase = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->phase, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->phase_stream);
        self->phase_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Phasor_members[] = {
{"server", T_OBJECT_EX, offsetof(Phasor, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Phasor, stream), 0, "Stream object."},
{"freq", T_OBJECT_EX, offsetof(Phasor, freq), 0, "Frequency in cycle per second."},
{"phase", T_OBJECT_EX, offsetof(Phasor, phase), 0, "Phasorillator phase."},
{"mul", T_OBJECT_EX, offsetof(Phasor, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Phasor, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Phasor_methods[] = {
{"getServer", (PyCFunction)Phasor_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Phasor_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Phasor_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Phasor_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Phasor_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Phasor_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Phasor_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
{"setPhase", (PyCFunction)Phasor_setPhase, METH_O, "Sets oscillator phase."},
{"setMul", (PyCFunction)Phasor_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Phasor_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Phasor_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)Phasor_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Phasor_as_number = {
(binaryfunc)Phasor_add,                      /*nb_add*/
(binaryfunc)Phasor_sub,                 /*nb_subtract*/
(binaryfunc)Phasor_multiply,                 /*nb_multiply*/
(binaryfunc)Phasor_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs,*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)Phasor_inplace_add,              /*inplace_add*/
(binaryfunc)Phasor_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Phasor_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Phasor_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject PhasorType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Phasor_base",         /*tp_name*/
sizeof(Phasor),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Phasor_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Phasor_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Phasor objects. Phase incrementor from 0 to 1.",           /* tp_doc */
(traverseproc)Phasor_traverse,   /* tp_traverse */
(inquiry)Phasor_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Phasor_methods,             /* tp_methods */
Phasor_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Phasor_init,      /* tp_init */
0,                         /* tp_alloc */
Phasor_new,                 /* tp_new */
};

/**************/
/* Pointer object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *index;
    Stream *index_stream;
    int modebuffer[2];
} Pointer;

static void
Pointer_readframes_a(Pointer *self) {
    MYFLT ph, fpart;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *pha = Stream_getData((Stream *)self->index_stream);
    
    for (i=0; i<self->bufsize; i++) {
        ph = Osc_clip(pha[i] * size, size);
   
        ipart = (int)ph;
        fpart = ph - ipart;
        self->data[i] = tablelist[ipart] * (1.0 - fpart) + tablelist[ipart+1] * fpart;
    }
}

static void Pointer_postprocessing_ii(Pointer *self) { POST_PROCESSING_II };
static void Pointer_postprocessing_ai(Pointer *self) { POST_PROCESSING_AI };
static void Pointer_postprocessing_ia(Pointer *self) { POST_PROCESSING_IA };
static void Pointer_postprocessing_aa(Pointer *self) { POST_PROCESSING_AA };
static void Pointer_postprocessing_ireva(Pointer *self) { POST_PROCESSING_IREVA };
static void Pointer_postprocessing_areva(Pointer *self) { POST_PROCESSING_AREVA };
static void Pointer_postprocessing_revai(Pointer *self) { POST_PROCESSING_REVAI };
static void Pointer_postprocessing_revaa(Pointer *self) { POST_PROCESSING_REVAA };
static void Pointer_postprocessing_revareva(Pointer *self) { POST_PROCESSING_REVAREVA };

static void
Pointer_setProcMode(Pointer *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Pointer_readframes_a;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Pointer_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Pointer_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Pointer_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Pointer_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Pointer_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Pointer_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Pointer_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Pointer_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Pointer_postprocessing_revareva;
            break;
    } 
}

static void
Pointer_compute_next_data_frame(Pointer *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Pointer_traverse(Pointer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->index);    
    Py_VISIT(self->index_stream);    
    return 0;
}

static int 
Pointer_clear(Pointer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->index);    
    Py_CLEAR(self->index_stream);    
    return 0;
}

static void
Pointer_dealloc(Pointer* self)
{
    free(self->data);
    Pointer_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Pointer_deleteStream(Pointer *self) { DELETE_STREAM };

static PyObject *
Pointer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Pointer *self;
    self = (Pointer *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Pointer_compute_next_data_frame);
    self->mode_func_ptr = Pointer_setProcMode;
    
    return (PyObject *)self;
}

static int
Pointer_init(Pointer *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *indextmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "index", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &tabletmp, &indextmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (indextmp) {
        PyObject_CallMethod((PyObject *)self, "setIndex", "O", indextmp);
    }

    PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Pointer_getServer(Pointer* self) { GET_SERVER };
static PyObject * Pointer_getStream(Pointer* self) { GET_STREAM };
static PyObject * Pointer_setMul(Pointer *self, PyObject *arg) { SET_MUL };	
static PyObject * Pointer_setAdd(Pointer *self, PyObject *arg) { SET_ADD };	
static PyObject * Pointer_setSub(Pointer *self, PyObject *arg) { SET_SUB };	
static PyObject * Pointer_setDiv(Pointer *self, PyObject *arg) { SET_DIV };	

static PyObject * Pointer_play(Pointer *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Pointer_out(Pointer *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Pointer_stop(Pointer *self) { STOP };

static PyObject * Pointer_multiply(Pointer *self, PyObject *arg) { MULTIPLY };
static PyObject * Pointer_inplace_multiply(Pointer *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Pointer_add(Pointer *self, PyObject *arg) { ADD };
static PyObject * Pointer_inplace_add(Pointer *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Pointer_sub(Pointer *self, PyObject *arg) { SUB };
static PyObject * Pointer_inplace_sub(Pointer *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Pointer_div(Pointer *self, PyObject *arg) { DIV };
static PyObject * Pointer_inplace_div(Pointer *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Pointer_getTable(Pointer* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Pointer_setTable(Pointer *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pointer_setIndex(Pointer *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	if (isNumber == 1) {
		printf("Pointer index attributes must be a PyoObject.\n");
        Py_INCREF(Py_None);
        return Py_None;
	}
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_XDECREF(self->index);

    self->index = tmp;
    streamtmp = PyObject_CallMethod((PyObject *)self->index, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->index_stream);
    self->index_stream = (Stream *)streamtmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Pointer_members[] = {
{"server", T_OBJECT_EX, offsetof(Pointer, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Pointer, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(Pointer, table), 0, "Waveform table."},
{"index", T_OBJECT_EX, offsetof(Pointer, index), 0, "Reader index."},
{"mul", T_OBJECT_EX, offsetof(Pointer, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Pointer, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Pointer_methods[] = {
{"getTable", (PyCFunction)Pointer_getTable, METH_NOARGS, "Returns waveform table object."},
{"getServer", (PyCFunction)Pointer_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Pointer_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Pointer_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Pointer_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Pointer_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Pointer_stop, METH_NOARGS, "Stops computing."},
{"setTable", (PyCFunction)Pointer_setTable, METH_O, "Sets oscillator table."},
{"setIndex", (PyCFunction)Pointer_setIndex, METH_O, "Sets reader index."},
{"setMul", (PyCFunction)Pointer_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Pointer_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Pointer_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)Pointer_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Pointer_as_number = {
(binaryfunc)Pointer_add,                      /*nb_add*/
(binaryfunc)Pointer_sub,                 /*nb_subtract*/
(binaryfunc)Pointer_multiply,                 /*nb_multiply*/
(binaryfunc)Pointer_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs,*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)Pointer_inplace_add,              /*inplace_add*/
(binaryfunc)Pointer_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Pointer_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Pointer_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject PointerType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Pointer_base",         /*tp_name*/
sizeof(Pointer),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Pointer_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Pointer_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Pointer objects. Read a waveform table with a pointer index.",           /* tp_doc */
(traverseproc)Pointer_traverse,   /* tp_traverse */
(inquiry)Pointer_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Pointer_methods,             /* tp_methods */
Pointer_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Pointer_init,      /* tp_init */
0,                         /* tp_alloc */
Pointer_new,                 /* tp_new */
};

/**************/
/* Lookup object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *index;
    Stream *index_stream;
    int modebuffer[2];
} Lookup;

static MYFLT
Lookup_clip(MYFLT x) {
    if (x < -1.0)
        return -1.0;
    else if (x > 1.0)
        return 1.0;
    else
        return x;
}

static void
Lookup_readframes_a(Lookup *self) {
    MYFLT ph, fpart;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *pha = Stream_getData((Stream *)self->index_stream);
    
    for (i=0; i<self->bufsize; i++) {
        ph = (Lookup_clip(pha[i]) * 0.5 + 0.5) * size;   
        ipart = (int)ph;
        fpart = ph - ipart;
        self->data[i] = tablelist[ipart] * (1.0 - fpart) + tablelist[ipart+1] * fpart;
    }
}

static void Lookup_postprocessing_ii(Lookup *self) { POST_PROCESSING_II };
static void Lookup_postprocessing_ai(Lookup *self) { POST_PROCESSING_AI };
static void Lookup_postprocessing_ia(Lookup *self) { POST_PROCESSING_IA };
static void Lookup_postprocessing_aa(Lookup *self) { POST_PROCESSING_AA };
static void Lookup_postprocessing_ireva(Lookup *self) { POST_PROCESSING_IREVA };
static void Lookup_postprocessing_areva(Lookup *self) { POST_PROCESSING_AREVA };
static void Lookup_postprocessing_revai(Lookup *self) { POST_PROCESSING_REVAI };
static void Lookup_postprocessing_revaa(Lookup *self) { POST_PROCESSING_REVAA };
static void Lookup_postprocessing_revareva(Lookup *self) { POST_PROCESSING_REVAREVA };

static void
Lookup_setProcMode(Lookup *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = Lookup_readframes_a;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Lookup_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Lookup_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Lookup_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Lookup_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Lookup_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Lookup_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Lookup_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Lookup_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Lookup_postprocessing_revareva;
            break;
    } 
}

static void
Lookup_compute_next_data_frame(Lookup *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Lookup_traverse(Lookup *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->index);    
    Py_VISIT(self->index_stream);    
    return 0;
}

static int 
Lookup_clear(Lookup *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->index);    
    Py_CLEAR(self->index_stream);    
    return 0;
}

static void
Lookup_dealloc(Lookup* self)
{
    free(self->data);
    Lookup_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Lookup_deleteStream(Lookup *self) { DELETE_STREAM };

static PyObject *
Lookup_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Lookup *self;
    self = (Lookup *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Lookup_compute_next_data_frame);
    self->mode_func_ptr = Lookup_setProcMode;
    
    return (PyObject *)self;
}

static int
Lookup_init(Lookup *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *indextmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "index", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &tabletmp, &indextmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (indextmp) {
        PyObject_CallMethod((PyObject *)self, "setIndex", "O", indextmp);
    }
    
    PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Lookup_getServer(Lookup* self) { GET_SERVER };
static PyObject * Lookup_getStream(Lookup* self) { GET_STREAM };
static PyObject * Lookup_setMul(Lookup *self, PyObject *arg) { SET_MUL };	
static PyObject * Lookup_setAdd(Lookup *self, PyObject *arg) { SET_ADD };	
static PyObject * Lookup_setSub(Lookup *self, PyObject *arg) { SET_SUB };	
static PyObject * Lookup_setDiv(Lookup *self, PyObject *arg) { SET_DIV };	

static PyObject * Lookup_play(Lookup *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Lookup_out(Lookup *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Lookup_stop(Lookup *self) { STOP };

static PyObject * Lookup_multiply(Lookup *self, PyObject *arg) { MULTIPLY };
static PyObject * Lookup_inplace_multiply(Lookup *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Lookup_add(Lookup *self, PyObject *arg) { ADD };
static PyObject * Lookup_inplace_add(Lookup *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Lookup_sub(Lookup *self, PyObject *arg) { SUB };
static PyObject * Lookup_inplace_sub(Lookup *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Lookup_div(Lookup *self, PyObject *arg) { DIV };
static PyObject * Lookup_inplace_div(Lookup *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Lookup_getTable(Lookup* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Lookup_setTable(Lookup *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Lookup_setIndex(Lookup *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	if (isNumber == 1) {
		printf("Lookup index attributes must be a PyoObject.\n");
        Py_INCREF(Py_None);
        return Py_None;
	}
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_XDECREF(self->index);
    
    self->index = tmp;
    streamtmp = PyObject_CallMethod((PyObject *)self->index, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->index_stream);
    self->index_stream = (Stream *)streamtmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Lookup_members[] = {
{"server", T_OBJECT_EX, offsetof(Lookup, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Lookup, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(Lookup, table), 0, "Waveform table."},
{"index", T_OBJECT_EX, offsetof(Lookup, index), 0, "Reader index."},
{"mul", T_OBJECT_EX, offsetof(Lookup, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Lookup, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Lookup_methods[] = {
{"getTable", (PyCFunction)Lookup_getTable, METH_NOARGS, "Returns waveform table object."},
{"getServer", (PyCFunction)Lookup_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Lookup_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Lookup_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Lookup_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Lookup_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Lookup_stop, METH_NOARGS, "Stops computing."},
{"setTable", (PyCFunction)Lookup_setTable, METH_O, "Sets oscillator table."},
{"setIndex", (PyCFunction)Lookup_setIndex, METH_O, "Sets reader index."},
{"setMul", (PyCFunction)Lookup_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Lookup_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Lookup_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)Lookup_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Lookup_as_number = {
(binaryfunc)Lookup_add,                      /*nb_add*/
(binaryfunc)Lookup_sub,                 /*nb_subtract*/
(binaryfunc)Lookup_multiply,                 /*nb_multiply*/
(binaryfunc)Lookup_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs,*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)Lookup_inplace_add,              /*inplace_add*/
(binaryfunc)Lookup_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Lookup_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Lookup_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject LookupType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Lookup_base",         /*tp_name*/
sizeof(Lookup),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Lookup_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Lookup_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Lookup objects. Modify a signal by reading a table with the signal as the index.",           /* tp_doc */
(traverseproc)Lookup_traverse,   /* tp_traverse */
(inquiry)Lookup_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Lookup_methods,             /* tp_methods */
Lookup_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Lookup_init,      /* tp_init */
0,                         /* tp_alloc */
Lookup_new,                 /* tp_new */
};

/**************/
/* Pulsar object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *env;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *phase;
    Stream *phase_stream;
    PyObject *frac;
    Stream *frac_stream;
    int modebuffer[5];
    MYFLT pointerPos;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    MYFLT (*interp_func_ptr)(MYFLT *, int, MYFLT, int);
} Pulsar;

static void
Pulsar_readframes_iii(Pulsar *self) {
    MYFLT fr, ph, frac, invfrac, pos, scl_pos, t_pos, e_pos, fpart, tmp;
    double inc;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    MYFLT *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase);
    frac = _clip(PyFloat_AS_DOUBLE(self->frac));
    invfrac = 1.0 / frac;
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph;
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < frac) {
            scl_pos = pos * invfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            tmp = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            self->data[i] = tmp * (envlist[ipart] * (1.0 - fpart) + envlist[ipart+1] * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_aii(Pulsar *self) {
    MYFLT ph, frac, invfrac, pos, scl_pos, t_pos, e_pos, fpart, tmp, oneOnSr;
    double inc;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    MYFLT *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase);
    frac = _clip(PyFloat_AS_DOUBLE(self->frac));
    invfrac = 1.0 / frac;
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph;
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < frac) {
            scl_pos = pos * invfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            tmp = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            self->data[i] = tmp * (envlist[ipart] * (1.0 - fpart) + envlist[ipart+1] * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_iai(Pulsar *self) {
    MYFLT fr, frac, invfrac, pos, scl_pos, t_pos, e_pos, fpart, tmp;
    double inc;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    MYFLT *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);
    frac = _clip(PyFloat_AS_DOUBLE(self->frac));
    invfrac = 1.0 / frac;
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph[i];
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < frac) {
            scl_pos = pos * invfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            tmp = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            self->data[i] = tmp * (envlist[ipart] * (1.0 - fpart) + envlist[ipart+1] * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_aai(Pulsar *self) {
    MYFLT frac, invfrac, pos, scl_pos, t_pos, e_pos, fpart, tmp, oneOnSr;
    double inc;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    MYFLT *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);
    frac = _clip(PyFloat_AS_DOUBLE(self->frac));
    invfrac = 1.0 / frac;
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph[i];
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < frac) {
            scl_pos = pos * invfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            tmp = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            self->data[i] = tmp * (envlist[ipart] * (1.0 - fpart) + envlist[ipart+1] * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_iia(Pulsar *self) {
    MYFLT fr, ph, pos, curfrac, scl_pos, t_pos, e_pos, fpart, tmp;
    double inc;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    MYFLT *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase);
    MYFLT *frac = Stream_getData((Stream *)self->frac_stream);
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        curfrac = frac[i];
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph;
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < curfrac) {
            scl_pos = pos / curfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            tmp = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            self->data[i] = tmp * (envlist[ipart] * (1.0 - fpart) + envlist[ipart+1] * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_aia(Pulsar *self) {
    MYFLT ph, pos, curfrac, scl_pos, t_pos, e_pos, fpart, tmp, oneOnSr;
    double inc;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    MYFLT *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase);
    MYFLT *frac = Stream_getData((Stream *)self->frac_stream);
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        curfrac = frac[i];
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph;
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < curfrac) {
            scl_pos = pos / curfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            tmp = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            self->data[i] = tmp * (envlist[ipart] * (1.0 - fpart) + envlist[ipart+1] * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_iaa(Pulsar *self) {
    MYFLT fr, pos, curfrac, scl_pos, t_pos, e_pos, fpart, tmp;
    double inc;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    MYFLT *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);
    MYFLT *frac = Stream_getData((Stream *)self->frac_stream);
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        curfrac = frac[i];
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph[i];
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < curfrac) {
            scl_pos = pos / curfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            tmp = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            self->data[i] = tmp * (envlist[ipart] * (1.0 - fpart) + envlist[ipart+1] * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_aaa(Pulsar *self) {
    MYFLT pos, curfrac, scl_pos, t_pos, e_pos, fpart, tmp, oneOnSr;
    double inc;
    int i, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    MYFLT *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *ph = Stream_getData((Stream *)self->phase_stream);
    MYFLT *frac = Stream_getData((Stream *)self->frac_stream);
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        curfrac = frac[i];
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph[i];
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < curfrac) {
            scl_pos = pos / curfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            tmp = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            self->data[i] = tmp * (envlist[ipart] * (1.0 - fpart) + envlist[ipart+1] * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void Pulsar_postprocessing_ii(Pulsar *self) { POST_PROCESSING_II };
static void Pulsar_postprocessing_ai(Pulsar *self) { POST_PROCESSING_AI };
static void Pulsar_postprocessing_ia(Pulsar *self) { POST_PROCESSING_IA };
static void Pulsar_postprocessing_aa(Pulsar *self) { POST_PROCESSING_AA };
static void Pulsar_postprocessing_ireva(Pulsar *self) { POST_PROCESSING_IREVA };
static void Pulsar_postprocessing_areva(Pulsar *self) { POST_PROCESSING_AREVA };
static void Pulsar_postprocessing_revai(Pulsar *self) { POST_PROCESSING_REVAI };
static void Pulsar_postprocessing_revaa(Pulsar *self) { POST_PROCESSING_REVAA };
static void Pulsar_postprocessing_revareva(Pulsar *self) { POST_PROCESSING_REVAREVA };

static void
Pulsar_setProcMode(Pulsar *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Pulsar_readframes_iii;
            break;
        case 1:    
            self->proc_func_ptr = Pulsar_readframes_aii;
            break;
        case 10:        
            self->proc_func_ptr = Pulsar_readframes_iai;
            break;
        case 11:    
            self->proc_func_ptr = Pulsar_readframes_aai;
            break;
        case 100:        
            self->proc_func_ptr = Pulsar_readframes_iia;
            break;
        case 101:    
            self->proc_func_ptr = Pulsar_readframes_aia;
            break;
        case 110:        
            self->proc_func_ptr = Pulsar_readframes_iaa;
            break;
        case 111:    
            self->proc_func_ptr = Pulsar_readframes_aaa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Pulsar_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Pulsar_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Pulsar_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Pulsar_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Pulsar_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Pulsar_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Pulsar_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Pulsar_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Pulsar_postprocessing_revareva;
            break;
    } 
}

static void
Pulsar_compute_next_data_frame(Pulsar *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Pulsar_traverse(Pulsar *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->env);
    Py_VISIT(self->phase);    
    Py_VISIT(self->phase_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->frac);    
    Py_VISIT(self->frac_stream);
    return 0;
}

static int 
Pulsar_clear(Pulsar *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->env);
    Py_CLEAR(self->phase);    
    Py_CLEAR(self->phase_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->frac);    
    Py_CLEAR(self->frac_stream);
    return 0;
}

static void
Pulsar_dealloc(Pulsar* self)
{
    free(self->data);
    Pulsar_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Pulsar_deleteStream(Pulsar *self) { DELETE_STREAM };

static PyObject *
Pulsar_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Pulsar *self;
    self = (Pulsar *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->phase = PyFloat_FromDouble(0);
    self->frac = PyFloat_FromDouble(0.5);
    self->interp = 2;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    self->pointerPos = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Pulsar_compute_next_data_frame);
    self->mode_func_ptr = Pulsar_setProcMode;
    
    return (PyObject *)self;
}

static int
Pulsar_init(Pulsar *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *envtmp, *freqtmp=NULL, *phasetmp=NULL, *fractmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "env", "freq", "frac", "phase", "interp", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOOiOO", kwlist, &tabletmp, &envtmp, &freqtmp, &fractmp, &phasetmp, &self->interp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    Py_XDECREF(self->env);
    self->env = PyObject_CallMethod((PyObject *)envtmp, "getTableStream", "");

    if (phasetmp) {
        PyObject_CallMethod((PyObject *)self, "setPhase", "O", phasetmp);
    }
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (fractmp) {
        PyObject_CallMethod((PyObject *)self, "setFrac", "O", fractmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);

    SET_INTERP_POINTER
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Pulsar_getServer(Pulsar* self) { GET_SERVER };
static PyObject * Pulsar_getStream(Pulsar* self) { GET_STREAM };
static PyObject * Pulsar_setMul(Pulsar *self, PyObject *arg) { SET_MUL };	
static PyObject * Pulsar_setAdd(Pulsar *self, PyObject *arg) { SET_ADD };	
static PyObject * Pulsar_setSub(Pulsar *self, PyObject *arg) { SET_SUB };	
static PyObject * Pulsar_setDiv(Pulsar *self, PyObject *arg) { SET_DIV };	

static PyObject * Pulsar_play(Pulsar *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Pulsar_out(Pulsar *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Pulsar_stop(Pulsar *self) { STOP };

static PyObject * Pulsar_multiply(Pulsar *self, PyObject *arg) { MULTIPLY };
static PyObject * Pulsar_inplace_multiply(Pulsar *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Pulsar_add(Pulsar *self, PyObject *arg) { ADD };
static PyObject * Pulsar_inplace_add(Pulsar *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Pulsar_sub(Pulsar *self, PyObject *arg) { SUB };
static PyObject * Pulsar_inplace_sub(Pulsar *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Pulsar_div(Pulsar *self, PyObject *arg) { DIV };
static PyObject * Pulsar_inplace_div(Pulsar *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Pulsar_getTable(Pulsar* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Pulsar_getEnv(Pulsar* self)
{
    Py_INCREF(self->env);
    return self->env;
};

static PyObject *
Pulsar_setTable(Pulsar *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pulsar_setEnv(Pulsar *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->env);
    self->env = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pulsar_setFreq(Pulsar *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pulsar_setPhase(Pulsar *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->phase);
	if (isNumber == 1) {
		self->phase = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->phase = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->phase, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->phase_stream);
        self->phase_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pulsar_setFrac(Pulsar *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->frac);
	if (isNumber == 1) {
		self->frac = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->frac = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->frac, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->frac_stream);
        self->frac_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pulsar_setInterp(Pulsar *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isNumber = PyNumber_Check(arg);
    
	if (isNumber == 1) {
		self->interp = PyInt_AsLong(PyNumber_Int(arg));
    }  
    
    SET_INTERP_POINTER
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Pulsar_members[] = {
{"server", T_OBJECT_EX, offsetof(Pulsar, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Pulsar, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(Pulsar, table), 0, "Waveform table."},
{"freq", T_OBJECT_EX, offsetof(Pulsar, freq), 0, "Frequency in cycle per second."},
{"phase", T_OBJECT_EX, offsetof(Pulsar, phase), 0, "Oscillator phase."},
{"frac", T_OBJECT_EX, offsetof(Pulsar, frac), 0, "Table width inside whole length."},
{"mul", T_OBJECT_EX, offsetof(Pulsar, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Pulsar, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Pulsar_methods[] = {
{"getTable", (PyCFunction)Pulsar_getTable, METH_NOARGS, "Returns waveform table object."},
{"getEnv", (PyCFunction)Pulsar_getEnv, METH_NOARGS, "Returns object envelope."},
{"getServer", (PyCFunction)Pulsar_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Pulsar_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Pulsar_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Pulsar_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Pulsar_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Pulsar_stop, METH_NOARGS, "Stops computing."},
{"setTable", (PyCFunction)Pulsar_setTable, METH_O, "Sets oscillator table."},
{"setEnv", (PyCFunction)Pulsar_setEnv, METH_O, "Sets envelope table."},
{"setFreq", (PyCFunction)Pulsar_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
{"setPhase", (PyCFunction)Pulsar_setPhase, METH_O, "Sets oscillator phase."},
{"setFrac", (PyCFunction)Pulsar_setFrac, METH_O, "Sets waveform width inside whole period length."},
{"setInterp", (PyCFunction)Pulsar_setInterp, METH_O, "Sets Pulsar interpolation mode."},
{"setMul", (PyCFunction)Pulsar_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Pulsar_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Pulsar_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)Pulsar_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Pulsar_as_number = {
(binaryfunc)Pulsar_add,                      /*nb_add*/
(binaryfunc)Pulsar_sub,                 /*nb_subtract*/
(binaryfunc)Pulsar_multiply,                 /*nb_multiply*/
(binaryfunc)Pulsar_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs,*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)Pulsar_inplace_add,              /*inplace_add*/
(binaryfunc)Pulsar_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Pulsar_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Pulsar_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject PulsarType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Pulsar_base",         /*tp_name*/
sizeof(Pulsar),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Pulsar_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Pulsar_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Pulsar objects. Generates pulsar synthesis oscillator.",           /* tp_doc */
(traverseproc)Pulsar_traverse,   /* tp_traverse */
(inquiry)Pulsar_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Pulsar_methods,             /* tp_methods */
Pulsar_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Pulsar_init,      /* tp_init */
0,                         /* tp_alloc */
Pulsar_new,                 /* tp_new */
};

/**************/
/* TableRead object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *freq;
    Stream *freq_stream;
    int loop;
    int modebuffer[3];
    double pointerPos;
    MYFLT *trigsBuffer;
    MYFLT *tempTrigsBuffer;
    int init;    
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    MYFLT (*interp_func_ptr)(MYFLT *, int, MYFLT, int);
} TableRead;

static void
TableRead_readframes_i(TableRead *self) {
    MYFLT fr, inc, fpart;
    int i, ipart;
    int go = 1;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr * size / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        if (self->pointerPos < 0) {
            if (self->init == 0)
                self->trigsBuffer[i] = 1.0;
            else
                self->init = 0;
            self->pointerPos = size + self->pointerPos;
        }    
        else if (self->pointerPos >= size) {
            self->trigsBuffer[i] = 1.0;
            if (self->loop == 1)
                self->pointerPos -= size;
            else
                go = 0;
        }
        if (go == 1) {
            ipart = (int)self->pointerPos;
            fpart = self->pointerPos - ipart;
            self->data[i] = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
        }
        else
            self->data[i] = 0.0;
    }
    if (go == 0) {
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
    }    
}

static void
TableRead_readframes_a(TableRead *self) {
    MYFLT inc, fpart, sizeOnSr;
    int i, ipart;
    int go = 1;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    
    sizeOnSr = size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * sizeOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0) {
            if (self->init == 0)
                self->trigsBuffer[i] = 1.0;
            else
                self->init = 0;
            self->pointerPos = size + self->pointerPos;
        }    
        else if (self->pointerPos >= size) {
            self->trigsBuffer[i] = 1.0;
            if (self->loop == 1)
                self->pointerPos -= size;
            else
                go = 0;
        }
        if (go == 1) {
            ipart = (int)self->pointerPos;
            fpart = self->pointerPos - ipart;
            self->data[i] = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
        }    
        else
            self->data[i] = 0.0;
    }
    if (go == 0)
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
}

static void TableRead_postprocessing_ii(TableRead *self) { POST_PROCESSING_II };
static void TableRead_postprocessing_ai(TableRead *self) { POST_PROCESSING_AI };
static void TableRead_postprocessing_ia(TableRead *self) { POST_PROCESSING_IA };
static void TableRead_postprocessing_aa(TableRead *self) { POST_PROCESSING_AA };
static void TableRead_postprocessing_ireva(TableRead *self) { POST_PROCESSING_IREVA };
static void TableRead_postprocessing_areva(TableRead *self) { POST_PROCESSING_AREVA };
static void TableRead_postprocessing_revai(TableRead *self) { POST_PROCESSING_REVAI };
static void TableRead_postprocessing_revaa(TableRead *self) { POST_PROCESSING_REVAA };
static void TableRead_postprocessing_revareva(TableRead *self) { POST_PROCESSING_REVAREVA };

static void
TableRead_setProcMode(TableRead *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = TableRead_readframes_i;
            break;
        case 1:    
            self->proc_func_ptr = TableRead_readframes_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TableRead_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TableRead_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TableRead_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TableRead_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TableRead_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TableRead_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TableRead_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TableRead_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TableRead_postprocessing_revareva;
            break;
    } 
}

static void
TableRead_compute_next_data_frame(TableRead *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TableRead_traverse(TableRead *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
TableRead_clear(TableRead *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
TableRead_dealloc(TableRead* self)
{
    free(self->data);
    free(self->tempTrigsBuffer);
    free(self->trigsBuffer);
    TableRead_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TableRead_deleteStream(TableRead *self) { DELETE_STREAM };

static PyObject *
TableRead_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TableRead *self;
    self = (TableRead *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1);
    self->loop = 0;
    self->init = 1;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    self->pointerPos = 0.;
    self->interp = 2;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TableRead_compute_next_data_frame);
    self->mode_func_ptr = TableRead_setProcMode;
    
    return (PyObject *)self;
}

static int
TableRead_init(TableRead *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *tabletmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "freq", "loop", "interp", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OiiOO", kwlist, &tabletmp, &freqtmp, &self->loop, &self->interp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->tempTrigsBuffer = (MYFLT *)realloc(self->tempTrigsBuffer, self->bufsize * sizeof(MYFLT));
    
    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }    
    
    (*self->mode_func_ptr)(self);

    SET_INTERP_POINTER

    self->init = 1;
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TableRead_getServer(TableRead* self) { GET_SERVER };
static PyObject * TableRead_getStream(TableRead* self) { GET_STREAM };
static PyObject * TableRead_setMul(TableRead *self, PyObject *arg) { SET_MUL };	
static PyObject * TableRead_setAdd(TableRead *self, PyObject *arg) { SET_ADD };	
static PyObject * TableRead_setSub(TableRead *self, PyObject *arg) { SET_SUB };	
static PyObject * TableRead_setDiv(TableRead *self, PyObject *arg) { SET_DIV };	

static PyObject * TableRead_play(TableRead *self, PyObject *args, PyObject *kwds) 
{ 
    self->pointerPos = 0.0;
    self->init = 1;
    PLAY 
};

static PyObject * TableRead_out(TableRead *self, PyObject *args, PyObject *kwds) 
{ 
    self->pointerPos = 0.0;
    OUT 
};
static PyObject * TableRead_stop(TableRead *self) { STOP };

static PyObject * TableRead_multiply(TableRead *self, PyObject *arg) { MULTIPLY };
static PyObject * TableRead_inplace_multiply(TableRead *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TableRead_add(TableRead *self, PyObject *arg) { ADD };
static PyObject * TableRead_inplace_add(TableRead *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TableRead_sub(TableRead *self, PyObject *arg) { SUB };
static PyObject * TableRead_inplace_sub(TableRead *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TableRead_div(TableRead *self, PyObject *arg) { DIV };
static PyObject * TableRead_inplace_div(TableRead *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TableRead_getTable(TableRead* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
TableRead_setTable(TableRead *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TableRead_setFreq(TableRead *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TableRead_setLoop(TableRead *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->loop = PyInt_AsLong(arg);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
TableRead_setInterp(TableRead *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isNumber = PyNumber_Check(arg);
    
	if (isNumber == 1) {
		self->interp = PyInt_AsLong(PyNumber_Int(arg));
    }  
    
    SET_INTERP_POINTER
    
    Py_INCREF(Py_None);
    return Py_None;
}

MYFLT *
TableRead_getTrigsBuffer(TableRead *self)
{
    int i;
    for (i=0; i<self->bufsize; i++) {
        self->tempTrigsBuffer[i] = self->trigsBuffer[i];
        self->trigsBuffer[i] = 0.0;
    }    
    return (MYFLT *)self->tempTrigsBuffer;
}    

static PyMemberDef TableRead_members[] = {
{"server", T_OBJECT_EX, offsetof(TableRead, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableRead, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(TableRead, table), 0, "Waveform table."},
{"freq", T_OBJECT_EX, offsetof(TableRead, freq), 0, "Frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(TableRead, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(TableRead, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableRead_methods[] = {
{"getTable", (PyCFunction)TableRead_getTable, METH_NOARGS, "Returns waveform table object."},
{"getServer", (PyCFunction)TableRead_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableRead_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TableRead_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TableRead_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)TableRead_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)TableRead_stop, METH_NOARGS, "Stops computing."},
{"setTable", (PyCFunction)TableRead_setTable, METH_O, "Sets oscillator table."},
{"setFreq", (PyCFunction)TableRead_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
{"setLoop", (PyCFunction)TableRead_setLoop, METH_O, "Sets the looping mode."},
{"setInterp", (PyCFunction)TableRead_setInterp, METH_O, "Sets reader interpolation mode."},
{"setMul", (PyCFunction)TableRead_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)TableRead_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)TableRead_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)TableRead_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods TableRead_as_number = {
(binaryfunc)TableRead_add,                      /*nb_add*/
(binaryfunc)TableRead_sub,                 /*nb_subtract*/
(binaryfunc)TableRead_multiply,                 /*nb_multiply*/
(binaryfunc)TableRead_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs,*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)TableRead_inplace_add,              /*inplace_add*/
(binaryfunc)TableRead_inplace_sub,         /*inplace_subtract*/
(binaryfunc)TableRead_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)TableRead_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject TableReadType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TableRead_base",         /*tp_name*/
sizeof(TableRead),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TableRead_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&TableRead_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"TableRead objects. Generates an oscillatory waveform.",           /* tp_doc */
(traverseproc)TableRead_traverse,   /* tp_traverse */
(inquiry)TableRead_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TableRead_methods,             /* tp_methods */
TableRead_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TableRead_init,      /* tp_init */
0,                         /* tp_alloc */
TableRead_new,                 /* tp_new */
};

/************************************************************************************************/
/* TableRead trig streamer */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TableRead *mainReader;
} TableReadTrig;

static void
TableReadTrig_compute_next_data_frame(TableReadTrig *self)
{
    int i;
    MYFLT *tmp;
    tmp = TableRead_getTrigsBuffer((TableRead *)self->mainReader);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }    
    Stream_setData(self->stream, self->data);
}

static int
TableReadTrig_traverse(TableReadTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainReader);
    return 0;
}

static int 
TableReadTrig_clear(TableReadTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainReader);    
    return 0;
}

static void
TableReadTrig_dealloc(TableReadTrig* self)
{
    free(self->data);
    TableReadTrig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TableReadTrig_deleteStream(TableReadTrig *self) { DELETE_STREAM };

static PyObject *
TableReadTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TableReadTrig *self;
    self = (TableReadTrig *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TableReadTrig_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
TableReadTrig_init(TableReadTrig *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL;
    
    static char *kwlist[] = {"mainReader", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        return -1; 
    
    Py_XDECREF(self->mainReader);
    Py_INCREF(maintmp);
    self->mainReader = (TableRead *)maintmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * TableReadTrig_getServer(TableReadTrig* self) { GET_SERVER };
static PyObject * TableReadTrig_getStream(TableReadTrig* self) { GET_STREAM };

static PyObject * TableReadTrig_play(TableReadTrig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TableReadTrig_stop(TableReadTrig *self) { STOP };

static PyMemberDef TableReadTrig_members[] = {
{"server", T_OBJECT_EX, offsetof(TableReadTrig, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableReadTrig, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableReadTrig_methods[] = {
{"getServer", (PyCFunction)TableReadTrig_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableReadTrig_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TableReadTrig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TableReadTrig_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TableReadTrig_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TableReadTrigType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TableReadTrig_base",         /*tp_name*/
sizeof(TableReadTrig),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TableReadTrig_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"TableReadTrig objects. Sends trigger at the end of playback.",           /* tp_doc */
(traverseproc)TableReadTrig_traverse,   /* tp_traverse */
(inquiry)TableReadTrig_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TableReadTrig_methods,             /* tp_methods */
TableReadTrig_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TableReadTrig_init,      /* tp_init */
0,                         /* tp_alloc */
TableReadTrig_new,                 /* tp_new */
};

/*************/
/* Fm object */
/*************/
typedef struct {
    pyo_audio_HEAD
    PyObject *car;
    Stream *car_stream;
    PyObject *ratio;
    Stream *ratio_stream;
    PyObject *index;
    Stream *index_stream;
    int modebuffer[5];
    MYFLT pointerPos_car;
    MYFLT pointerPos_mod;
    MYFLT scaleFactor;
} Fm;

static void
Fm_readframes_iii(Fm *self) {
    MYFLT mod_freq, mod_amp, mod_delta, mod_val, car_freq, car_delta, fpart;
    int i, ipart;
    
    MYFLT car = PyFloat_AS_DOUBLE(self->car);
    MYFLT rat = PyFloat_AS_DOUBLE(self->ratio);
    MYFLT ind = PyFloat_AS_DOUBLE(self->index);
    
    mod_freq = car * rat;
    mod_amp = mod_freq * ind;
    mod_delta = mod_freq * self->scaleFactor;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos_mod = Sine_clip(self->pointerPos_mod);
        ipart = (int)self->pointerPos_mod;
        fpart = self->pointerPos_mod - ipart;
        mod_val = mod_amp * (SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart);
        self->pointerPos_mod += mod_delta;
        
        car_freq = car + mod_val;
        car_delta = car_freq * self->scaleFactor;
        self->pointerPos_car = Sine_clip(self->pointerPos_car);
        ipart = (int)self->pointerPos_car;
        fpart = self->pointerPos_car - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos_car += car_delta;
    }
}

static void
Fm_readframes_aii(Fm *self) {
    MYFLT mod_freq, mod_amp, mod_delta, mod_val, car_freq, car_delta, fpart;
    int i, ipart;
    
    MYFLT *car = Stream_getData((Stream *)self->car_stream);
    MYFLT rat = PyFloat_AS_DOUBLE(self->ratio);
    MYFLT ind = PyFloat_AS_DOUBLE(self->index);

    for (i=0; i<self->bufsize; i++) {
        mod_freq = car[i] * rat;
        mod_amp = mod_freq * ind;
        mod_delta = mod_freq * self->scaleFactor;
        self->pointerPos_mod = Sine_clip(self->pointerPos_mod);
        ipart = (int)self->pointerPos_mod;
        fpart = self->pointerPos_mod - ipart;
        mod_val = mod_amp * (SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart);
        self->pointerPos_mod += mod_delta;
        
        car_freq = car[i] + mod_val;
        car_delta = car_freq * self->scaleFactor;
        self->pointerPos_car = Sine_clip(self->pointerPos_car);
        ipart = (int)self->pointerPos_car;
        fpart = self->pointerPos_car - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos_car += car_delta;
    }
}

static void
Fm_readframes_iai(Fm *self) {
    MYFLT mod_freq, mod_amp, mod_delta, mod_val, car_freq, car_delta, fpart;
    int i, ipart;
    
    MYFLT car = PyFloat_AS_DOUBLE(self->car);
    MYFLT *rat = Stream_getData((Stream *)self->ratio_stream);
    MYFLT ind = PyFloat_AS_DOUBLE(self->index);

    for (i=0; i<self->bufsize; i++) {
        mod_freq = car * rat[i];
        mod_amp = mod_freq * ind;
        mod_delta = mod_freq * self->scaleFactor;
        self->pointerPos_mod = Sine_clip(self->pointerPos_mod);
        ipart = (int)self->pointerPos_mod;
        fpart = self->pointerPos_mod - ipart;
        mod_val = mod_amp * (SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart);
        self->pointerPos_mod += mod_delta;
        
        car_freq = car + mod_val;
        car_delta = car_freq * self->scaleFactor;
        self->pointerPos_car = Sine_clip(self->pointerPos_car);
        ipart = (int)self->pointerPos_car;
        fpart = self->pointerPos_car - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos_car += car_delta;
    }
}

static void
Fm_readframes_aai(Fm *self) {
    MYFLT mod_freq, mod_amp, mod_delta, mod_val, car_freq, car_delta, fpart;
    int i, ipart;
    
    MYFLT *car = Stream_getData((Stream *)self->car_stream);
    MYFLT *rat = Stream_getData((Stream *)self->ratio_stream);
    MYFLT ind = PyFloat_AS_DOUBLE(self->index);

    for (i=0; i<self->bufsize; i++) {
        mod_freq = car[i] * rat[i];
        mod_amp = mod_freq * ind;
        mod_delta = mod_freq * self->scaleFactor;
        self->pointerPos_mod = Sine_clip(self->pointerPos_mod);
        ipart = (int)self->pointerPos_mod;
        fpart = self->pointerPos_mod - ipart;
        mod_val = mod_amp * (SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart);
        self->pointerPos_mod += mod_delta;
        
        car_freq = car[i] + mod_val;
        car_delta = car_freq * self->scaleFactor;
        self->pointerPos_car = Sine_clip(self->pointerPos_car);
        ipart = (int)self->pointerPos_car;
        fpart = self->pointerPos_car - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos_car += car_delta;
    }}

static void
Fm_readframes_iia(Fm *self) {
    MYFLT mod_freq, mod_amp, mod_delta, mod_val, car_freq, car_delta, fpart;
    int i, ipart;
    
    MYFLT car = PyFloat_AS_DOUBLE(self->car);
    MYFLT rat = PyFloat_AS_DOUBLE(self->ratio);
    MYFLT *ind = Stream_getData((Stream *)self->index_stream);
    
    mod_freq = car * rat;
    mod_delta = mod_freq * self->scaleFactor;
    
    for (i=0; i<self->bufsize; i++) {
        mod_amp = mod_freq * ind[i];
        self->pointerPos_mod = Sine_clip(self->pointerPos_mod);
        ipart = (int)self->pointerPos_mod;
        fpart = self->pointerPos_mod - ipart;
        mod_val = mod_amp * (SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart);
        self->pointerPos_mod += mod_delta;
        
        car_freq = car + mod_val;
        car_delta = car_freq * self->scaleFactor;
        self->pointerPos_car = Sine_clip(self->pointerPos_car);
        ipart = (int)self->pointerPos_car;
        fpart = self->pointerPos_car - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos_car += car_delta;
    }
}

static void
Fm_readframes_aia(Fm *self) {
    MYFLT mod_freq, mod_amp, mod_delta, mod_val, car_freq, car_delta, fpart;
    int i, ipart;
    
    MYFLT *car = Stream_getData((Stream *)self->car_stream);
    MYFLT rat = PyFloat_AS_DOUBLE(self->ratio);
    MYFLT *ind = Stream_getData((Stream *)self->index_stream);
    
    for (i=0; i<self->bufsize; i++) {
        mod_freq = car[i] * rat;
        mod_amp = mod_freq * ind[i];
        mod_delta = mod_freq * self->scaleFactor;
        self->pointerPos_mod = Sine_clip(self->pointerPos_mod);
        ipart = (int)self->pointerPos_mod;
        fpart = self->pointerPos_mod - ipart;
        mod_val = mod_amp * (SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart);
        self->pointerPos_mod += mod_delta;
        
        car_freq = car[i] + mod_val;
        car_delta = car_freq * self->scaleFactor;
        self->pointerPos_car = Sine_clip(self->pointerPos_car);
        ipart = (int)self->pointerPos_car;
        fpart = self->pointerPos_car - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos_car += car_delta;
    }
}

static void
Fm_readframes_iaa(Fm *self) {
    MYFLT mod_freq, mod_amp, mod_delta, mod_val, car_freq, car_delta, fpart;
    int i, ipart;
    
    MYFLT car = PyFloat_AS_DOUBLE(self->car);
    MYFLT *rat = Stream_getData((Stream *)self->ratio_stream);
    MYFLT *ind = Stream_getData((Stream *)self->index_stream);
    
    for (i=0; i<self->bufsize; i++) {
        mod_freq = car * rat[i];
        mod_amp = mod_freq * ind[i];
        mod_delta = mod_freq * self->scaleFactor;
        self->pointerPos_mod = Sine_clip(self->pointerPos_mod);
        ipart = (int)self->pointerPos_mod;
        fpart = self->pointerPos_mod - ipart;
        mod_val = mod_amp * (SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart);
        self->pointerPos_mod += mod_delta;
        
        car_freq = car + mod_val;
        car_delta = car_freq * self->scaleFactor;
        self->pointerPos_car = Sine_clip(self->pointerPos_car);
        ipart = (int)self->pointerPos_car;
        fpart = self->pointerPos_car - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos_car += car_delta;
    }
}

static void
Fm_readframes_aaa(Fm *self) {
    MYFLT mod_freq, mod_amp, mod_delta, mod_val, car_freq, car_delta, fpart;
    int i, ipart;
    
    MYFLT *car = Stream_getData((Stream *)self->car_stream);
    MYFLT *rat = Stream_getData((Stream *)self->ratio_stream);
    MYFLT *ind = Stream_getData((Stream *)self->index_stream);
    
    for (i=0; i<self->bufsize; i++) {
        mod_freq = car[i] * rat[i];
        mod_amp = mod_freq * ind[i];
        mod_delta = mod_freq * self->scaleFactor;
        self->pointerPos_mod = Sine_clip(self->pointerPos_mod);
        ipart = (int)self->pointerPos_mod;
        fpart = self->pointerPos_mod - ipart;
        mod_val = mod_amp * (SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart);
        self->pointerPos_mod += mod_delta;
        
        car_freq = car[i] + mod_val;
        car_delta = car_freq * self->scaleFactor;
        self->pointerPos_car = Sine_clip(self->pointerPos_car);
        ipart = (int)self->pointerPos_car;
        fpart = self->pointerPos_car - ipart;
        self->data[i] = SINE_ARRAY[ipart] * (1.0 - fpart) + SINE_ARRAY[ipart+1] * fpart;
        self->pointerPos_car += car_delta;
    }
}

static void Fm_postprocessing_ii(Fm *self) { POST_PROCESSING_II };
static void Fm_postprocessing_ai(Fm *self) { POST_PROCESSING_AI };
static void Fm_postprocessing_ia(Fm *self) { POST_PROCESSING_IA };
static void Fm_postprocessing_aa(Fm *self) { POST_PROCESSING_AA };
static void Fm_postprocessing_ireva(Fm *self) { POST_PROCESSING_IREVA };
static void Fm_postprocessing_areva(Fm *self) { POST_PROCESSING_AREVA };
static void Fm_postprocessing_revai(Fm *self) { POST_PROCESSING_REVAI };
static void Fm_postprocessing_revaa(Fm *self) { POST_PROCESSING_REVAA };
static void Fm_postprocessing_revareva(Fm *self) { POST_PROCESSING_REVAREVA };

static void
Fm_setProcMode(Fm *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Fm_readframes_iii;
            break;
        case 1:    
            self->proc_func_ptr = Fm_readframes_aii;
            break;
        case 10:    
            self->proc_func_ptr = Fm_readframes_iai;
            break;
        case 11:    
            self->proc_func_ptr = Fm_readframes_aai;
            break;
        case 100:        
            self->proc_func_ptr = Fm_readframes_iia;
            break;
        case 101:    
            self->proc_func_ptr = Fm_readframes_aia;
            break;
        case 110:    
            self->proc_func_ptr = Fm_readframes_iaa;
            break;
        case 111:    
            self->proc_func_ptr = Fm_readframes_aaa;
            break;
    } 
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Fm_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Fm_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Fm_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Fm_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Fm_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Fm_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Fm_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Fm_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Fm_postprocessing_revareva;
            break;
    }
}

static void
Fm_compute_next_data_frame(Fm *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Fm_traverse(Fm *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->car);    
    Py_VISIT(self->car_stream);    
    Py_VISIT(self->ratio);    
    Py_VISIT(self->ratio_stream);    
    Py_VISIT(self->index);    
    Py_VISIT(self->index_stream);    
    return 0;
}

static int 
Fm_clear(Fm *self)
{
    pyo_CLEAR
    Py_CLEAR(self->car);    
    Py_CLEAR(self->car_stream);    
    Py_CLEAR(self->ratio);    
    Py_CLEAR(self->ratio_stream);    
    Py_CLEAR(self->index);    
    Py_CLEAR(self->index_stream);    
    return 0;
}

static void
Fm_dealloc(Fm* self)
{
    free(self->data);
    Fm_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Fm_deleteStream(Fm *self) { DELETE_STREAM };

static PyObject *
Fm_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Fm *self;
    self = (Fm *)type->tp_alloc(type, 0);
    
    self->car = PyFloat_FromDouble(100);
    self->ratio = PyFloat_FromDouble(0.5);
    self->index = PyFloat_FromDouble(5);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    self->pointerPos_car = self->pointerPos_mod = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Fm_compute_next_data_frame);
    self->mode_func_ptr = Fm_setProcMode;

    self->scaleFactor = 512.0 / self->sr;

    return (PyObject *)self;
}

static int
Fm_init(Fm *self, PyObject *args, PyObject *kwds)
{
    PyObject *cartmp=NULL, *ratiotmp=NULL, *indextmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"carrier", "ratio", "index", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOO", kwlist, &cartmp, &ratiotmp, &indextmp, &multmp, &addtmp))
        return -1; 
    
    if (cartmp) {
        PyObject_CallMethod((PyObject *)self, "setCarrier", "O", cartmp);
    }
    
    if (ratiotmp) {
        PyObject_CallMethod((PyObject *)self, "setRatio", "O", ratiotmp);
    }

    if (indextmp) {
        PyObject_CallMethod((PyObject *)self, "setIndex", "O", indextmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Fm_getServer(Fm* self) { GET_SERVER };
static PyObject * Fm_getStream(Fm* self) { GET_STREAM };
static PyObject * Fm_setMul(Fm *self, PyObject *arg) { SET_MUL };	
static PyObject * Fm_setAdd(Fm *self, PyObject *arg) { SET_ADD };	
static PyObject * Fm_setSub(Fm *self, PyObject *arg) { SET_SUB };	
static PyObject * Fm_setDiv(Fm *self, PyObject *arg) { SET_DIV };	

static PyObject * Fm_play(Fm *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Fm_out(Fm *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Fm_stop(Fm *self) { STOP };

static PyObject * Fm_multiply(Fm *self, PyObject *arg) { MULTIPLY };
static PyObject * Fm_inplace_multiply(Fm *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Fm_add(Fm *self, PyObject *arg) { ADD };
static PyObject * Fm_inplace_add(Fm *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Fm_sub(Fm *self, PyObject *arg) { SUB };
static PyObject * Fm_inplace_sub(Fm *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Fm_div(Fm *self, PyObject *arg) { DIV };
static PyObject * Fm_inplace_div(Fm *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Fm_setCarrier(Fm *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->car);
	if (isNumber == 1) {
		self->car = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->car = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->car, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->car_stream);
        self->car_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Fm_setRatio(Fm *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->ratio);
	if (isNumber == 1) {
		self->ratio = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->ratio = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->ratio, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->ratio_stream);
        self->ratio_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Fm_setIndex(Fm *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->index);
	if (isNumber == 1) {
		self->index = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->index = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->index, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->index_stream);
        self->index_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Fm_members[] = {
{"server", T_OBJECT_EX, offsetof(Fm, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Fm, stream), 0, "Stream object."},
{"carrier", T_OBJECT_EX, offsetof(Fm, car), 0, "Frequency in cycle per second."},
{"ratio", T_OBJECT_EX, offsetof(Fm, ratio), 0, "Ratio carrier:modulator (mod freq = car*mod)."},
{"index", T_OBJECT_EX, offsetof(Fm, index), 0, "Modulation index (mod amp = mod freq*index)."},
{"mul", T_OBJECT_EX, offsetof(Fm, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Fm, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Fm_methods[] = {
{"getServer", (PyCFunction)Fm_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Fm_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Fm_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Fm_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Fm_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Fm_stop, METH_NOARGS, "Stops computing."},
{"setCarrier", (PyCFunction)Fm_setCarrier, METH_O, "Sets carrier frequency in cycle per second."},
{"setRatio", (PyCFunction)Fm_setRatio, METH_O, "Sets car:mod ratio."},
{"setIndex", (PyCFunction)Fm_setIndex, METH_O, "Sets modulation index."},
{"setMul", (PyCFunction)Fm_setMul, METH_O, "Sets Fm mul factor."},
{"setAdd", (PyCFunction)Fm_setAdd, METH_O, "Sets Fm add factor."},
{"setSub", (PyCFunction)Fm_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Fm_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Fm_as_number = {
(binaryfunc)Fm_add,                      /*nb_add*/
(binaryfunc)Fm_sub,                 /*nb_subtract*/
(binaryfunc)Fm_multiply,                 /*nb_multiply*/
(binaryfunc)Fm_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)Fm_inplace_add,              /*inplace_add*/
(binaryfunc)Fm_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Fm_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Fm_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject FmType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Fm_base",         /*tp_name*/
sizeof(Fm),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Fm_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Fm_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"Fm objects. Generates a frequency modulation synthesis.",           /* tp_doc */
(traverseproc)Fm_traverse,   /* tp_traverse */
(inquiry)Fm_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Fm_methods,             /* tp_methods */
Fm_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Fm_init,      /* tp_init */
0,                         /* tp_alloc */
Fm_new,                 /* tp_new */
};
