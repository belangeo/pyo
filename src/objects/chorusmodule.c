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

#include <Python.h>
#include "py2to3.h"
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

static MYFLT LFO_ARRAY[513] = {0.0, 0.012271538285719925, 0.024541228522912288, 0.036807222941358832, 0.049067674327418015, 0.061320736302208578, 0.073564563599667426, 0.085797312344439894, 0.098017140329560604, 0.11022220729388306, 0.1224106751992162, 0.13458070850712617, 0.14673047445536175, 0.15885814333386145, 0.17096188876030122, 0.18303988795514095, 0.19509032201612825, 0.20711137619221856, 0.2191012401568698, 0.23105810828067111, 0.24298017990326387, 0.25486565960451457, 0.26671275747489837, 0.27851968938505306, 0.29028467725446233, 0.30200594931922808, 0.31368174039889152, 0.32531029216226293, 0.33688985339222005, 0.34841868024943456, 0.35989503653498811, 0.37131719395183754, 0.38268343236508978, 0.3939920400610481, 0.40524131400498986, 0.41642956009763715, 0.42755509343028208, 0.43861623853852766, 0.44961132965460654, 0.46053871095824001, 0.47139673682599764, 0.48218377207912272, 0.49289819222978404, 0.50353838372571758, 0.51410274419322166, 0.52458968267846895, 0.53499761988709715, 0.54532498842204646, 0.55557023301960218, 0.56573181078361312, 0.57580819141784534, 0.58579785745643886, 0.59569930449243336, 0.60551104140432555, 0.61523159058062682, 0.62485948814238634, 0.63439328416364549, 0.64383154288979139, 0.65317284295377676, 0.66241577759017178, 0.67155895484701833, 0.68060099779545302, 0.68954054473706683, 0.69837624940897292, 0.70710678118654746, 0.71573082528381859, 0.72424708295146689, 0.7326542716724127, 0.74095112535495899, 0.74913639452345926, 0.75720884650648446, 0.76516726562245885, 0.77301045336273688, 0.78073722857209438, 0.78834642762660623, 0.79583690460888346, 0.80320753148064483, 0.81045719825259477, 0.81758481315158371, 0.82458930278502529, 0.83146961230254512, 0.83822470555483797, 0.84485356524970701, 0.8513551931052652, 0.85772861000027212, 0.8639728561215867, 0.87008699110871135, 0.87607009419540649, 0.88192126434835494, 0.88763962040285393, 0.89322430119551532, 0.89867446569395382, 0.90398929312344334, 0.90916798309052238, 0.91420975570353069, 0.91911385169005777, 0.92387953251128674, 0.92850608047321548, 0.93299279883473885, 0.93733901191257496, 0.94154406518302081, 0.94560732538052128, 0.94952818059303667, 0.95330604035419375, 0.95694033573220894, 0.96043051941556579, 0.96377606579543984, 0.96697647104485207, 0.97003125319454397, 0.97293995220556007, 0.97570213003852857, 0.97831737071962765, 0.98078528040323043, 0.98310548743121629, 0.98527764238894122, 0.98730141815785843, 0.98917650996478101, 0.99090263542778001, 0.99247953459870997, 0.99390697000235606, 0.99518472667219682, 0.996312612182778, 0.99729045667869021, 0.99811811290014918, 0.99879545620517241, 0.99932238458834954, 0.99969881869620425, 0.9999247018391445, 1.0, 0.9999247018391445, 0.99969881869620425, 0.99932238458834954, 0.99879545620517241, 0.99811811290014918, 0.99729045667869021, 0.996312612182778, 0.99518472667219693, 0.99390697000235606, 0.99247953459870997, 0.99090263542778001, 0.98917650996478101, 0.98730141815785843, 0.98527764238894122, 0.98310548743121629, 0.98078528040323043, 0.97831737071962765, 0.97570213003852857, 0.97293995220556018, 0.97003125319454397, 0.96697647104485207, 0.96377606579543984, 0.9604305194155659, 0.95694033573220894, 0.95330604035419386, 0.94952818059303667, 0.94560732538052139, 0.94154406518302081, 0.93733901191257496, 0.93299279883473885, 0.92850608047321559, 0.92387953251128674, 0.91911385169005777, 0.91420975570353069, 0.90916798309052249, 0.90398929312344345, 0.89867446569395393, 0.89322430119551521, 0.88763962040285393, 0.88192126434835505, 0.8760700941954066, 0.87008699110871146, 0.86397285612158681, 0.85772861000027212, 0.8513551931052652, 0.84485356524970723, 0.83822470555483819, 0.83146961230254546, 0.82458930278502529, 0.81758481315158371, 0.81045719825259477, 0.80320753148064494, 0.79583690460888357, 0.78834642762660634, 0.7807372285720946, 0.7730104533627371, 0.76516726562245907, 0.75720884650648479, 0.74913639452345926, 0.74095112535495899, 0.73265427167241282, 0.724247082951467, 0.71573082528381871, 0.70710678118654757, 0.69837624940897292, 0.68954054473706705, 0.68060099779545324, 0.67155895484701855, 0.66241577759017201, 0.65317284295377664, 0.64383154288979139, 0.63439328416364549, 0.62485948814238634, 0.61523159058062693, 0.60551104140432555, 0.59569930449243347, 0.58579785745643898, 0.57580819141784545, 0.56573181078361345, 0.55557023301960218, 0.54532498842204635, 0.53499761988709715, 0.52458968267846895, 0.51410274419322177, 0.50353838372571758, 0.49289819222978415, 0.48218377207912289, 0.47139673682599781, 0.46053871095824023, 0.44961132965460687, 0.43861623853852755, 0.42755509343028203, 0.41642956009763715, 0.40524131400498986, 0.39399204006104815, 0.38268343236508984, 0.37131719395183765, 0.35989503653498833, 0.34841868024943479, 0.33688985339222027, 0.3253102921622632, 0.31368174039889141, 0.30200594931922803, 0.29028467725446233, 0.27851968938505312, 0.26671275747489848, 0.25486565960451468, 0.24298017990326404, 0.2310581082806713, 0.21910124015687002, 0.20711137619221884, 0.19509032201612858, 0.1830398879551409, 0.17096188876030119, 0.15885814333386145, 0.1467304744553618, 0.13458070850712628, 0.12241067519921635, 0.11022220729388325, 0.09801714032956084, 0.085797312344440158, 0.073564563599667745, 0.061320736302208495, 0.049067674327417973, 0.036807222941358832, 0.024541228522912326, 0.012271538285720007, 1.2246467991473532e-16, -0.012271538285719761, -0.024541228522912083, -0.036807222941358582, -0.049067674327417724, -0.061320736302208245, -0.073564563599667496, -0.085797312344439922, -0.09801714032956059, -0.110222207293883, -0.1224106751992161, -0.13458070850712606, -0.14673047445536158, -0.15885814333386122, -0.17096188876030097, -0.18303988795514067, -0.19509032201612836, -0.20711137619221862, -0.21910124015686983, -0.23105810828067111, -0.24298017990326382, -0.25486565960451446, -0.26671275747489825, -0.27851968938505289, -0.29028467725446216, -0.30200594931922781, -0.31368174039889118, -0.32531029216226304, -0.33688985339222011, -0.34841868024943456, -0.35989503653498811, -0.37131719395183749, -0.38268343236508967, -0.39399204006104793, -0.40524131400498969, -0.41642956009763693, -0.42755509343028181, -0.43861623853852733, -0.44961132965460665, -0.46053871095824006, -0.47139673682599764, -0.48218377207912272, -0.49289819222978393, -0.50353838372571746, -0.51410274419322155, -0.52458968267846873, -0.53499761988709693, -0.54532498842204613, -0.55557023301960196, -0.56573181078361323, -0.57580819141784534, -0.58579785745643886, -0.59569930449243325, -0.60551104140432543, -0.61523159058062671, -0.62485948814238623, -0.63439328416364527, -0.64383154288979128, -0.65317284295377653, -0.66241577759017178, -0.67155895484701844, -0.68060099779545302, -0.68954054473706683, -0.6983762494089728, -0.70710678118654746, -0.71573082528381848, -0.72424708295146667, -0.73265427167241259, -0.74095112535495877, -0.74913639452345904, -0.75720884650648423, -0.76516726562245885, -0.77301045336273666, -0.78073722857209438, -0.78834642762660589, -0.79583690460888334, -0.80320753148064505, -0.81045719825259466, -0.81758481315158371, -0.82458930278502507, -0.83146961230254524, -0.83822470555483775, -0.84485356524970712, -0.85135519310526486, -0.85772861000027201, -0.86397285612158647, -0.87008699110871135, -0.87607009419540671, -0.88192126434835494, -0.88763962040285405, -0.89322430119551521, -0.89867446569395382, -0.90398929312344312, -0.90916798309052238, -0.91420975570353047, -0.91911385169005766, -0.92387953251128652, -0.92850608047321548, -0.93299279883473896, -0.93733901191257485, -0.94154406518302081, -0.94560732538052117, -0.94952818059303667, -0.95330604035419375, -0.95694033573220882, -0.96043051941556568, -0.96377606579543984, -0.96697647104485218, -0.97003125319454397, -0.97293995220556018, -0.97570213003852846, -0.97831737071962765, -0.98078528040323032, -0.98310548743121629, -0.98527764238894111, -0.98730141815785832, -0.9891765099647809, -0.99090263542778001, -0.99247953459871008, -0.99390697000235606, -0.99518472667219693, -0.996312612182778, -0.99729045667869021, -0.99811811290014918, -0.99879545620517241, -0.99932238458834943, -0.99969881869620425, -0.9999247018391445, -1.0, -0.9999247018391445, -0.99969881869620425, -0.99932238458834954, -0.99879545620517241, -0.99811811290014918, -0.99729045667869021, -0.996312612182778, -0.99518472667219693, -0.99390697000235606, -0.99247953459871008, -0.99090263542778001, -0.9891765099647809, -0.98730141815785843, -0.98527764238894122, -0.9831054874312164, -0.98078528040323043, -0.97831737071962777, -0.97570213003852857, -0.97293995220556029, -0.97003125319454397, -0.96697647104485229, -0.96377606579543995, -0.96043051941556579, -0.95694033573220894, -0.95330604035419375, -0.94952818059303679, -0.94560732538052128, -0.94154406518302092, -0.93733901191257496, -0.93299279883473907, -0.92850608047321559, -0.92387953251128663, -0.91911385169005788, -0.91420975570353058, -0.90916798309052249, -0.90398929312344334, -0.89867446569395404, -0.89322430119551532, -0.88763962040285416, -0.88192126434835505, -0.87607009419540693, -0.87008699110871146, -0.8639728561215867, -0.85772861000027223, -0.85135519310526508, -0.84485356524970734, -0.83822470555483797, -0.83146961230254557, -0.82458930278502529, -0.81758481315158404, -0.81045719825259488, -0.80320753148064528, -0.79583690460888368, -0.78834642762660612, -0.78073722857209471, -0.77301045336273688, -0.76516726562245918, -0.75720884650648457, -0.7491363945234597, -0.74095112535495922, -0.73265427167241315, -0.72424708295146711, -0.71573082528381904, -0.70710678118654768, -0.69837624940897269, -0.68954054473706716, -0.68060099779545302, -0.67155895484701866, -0.66241577759017178, -0.65317284295377709, -0.6438315428897915, -0.63439328416364593, -0.62485948814238645, -0.61523159058062737, -0.60551104140432566, -0.59569930449243325, -0.58579785745643909, -0.57580819141784523, -0.56573181078361356, -0.55557023301960218, -0.5453249884220468, -0.53499761988709726, -0.52458968267846939, -0.51410274419322188, -0.50353838372571813, -0.49289819222978426, -0.48218377207912261, -0.47139673682599792, -0.46053871095823995, -0.44961132965460698, -0.43861623853852766, -0.42755509343028253, -0.41642956009763726, -0.40524131400499042, -0.39399204006104827, -0.38268343236509039, -0.37131719395183777, -0.359895036534988, -0.3484186802494349, -0.33688985339222, -0.32531029216226331, -0.31368174039889152, -0.30200594931922853, -0.29028467725446244, -0.27851968938505367, -0.26671275747489859, -0.25486565960451435, -0.24298017990326418, -0.23105810828067103, -0.21910124015687016, -0.20711137619221853, -0.19509032201612872, -0.18303988795514103, -0.17096188876030177, -0.15885814333386158, -0.14673047445536239, -0.13458070850712642, -0.12241067519921603, -0.11022220729388338, -0.09801714032956052, -0.085797312344440282, -0.073564563599667426, -0.06132073630220905, -0.049067674327418091, -0.036807222941359394, -0.024541228522912451, -0.012271538285720572, 0.0};

/* center delay time, delay time deviation, lfo frequency */
static const MYFLT chorusParams[8][3] = {
{ 384.0, 44.0, 1.879 },
{ 450.0, 53.0, 1.654 },
{ 489.0, 57.0, 1.342 },
{ 553.0, 62.0, 1.231 },
{ 591.0, 66.0, 0.879 },
{ 662.0, 71.0, 0.657 },
{ 753.0, 88.0, 0.465 },
{ 785.0, 101.0, 0.254 }
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    PyObject *depth;
    Stream *depth_stream;
    PyObject *mix;
    Stream *mix_stream;
    void (*mix_func_ptr)();
    int modebuffer[5];
    MYFLT total_signal;
    MYFLT delays[8];
    MYFLT delay_devs[8];
    long size[8];
    long in_count[8];
    MYFLT *buffer[8];
    // jitters
    MYFLT pointerPos[8];
    MYFLT inc[8];
} Chorus;

static void
Chorus_process_ii(Chorus *self) {
    MYFLT lfo, pos, val, fpart, inval;
    int i, j, ipart;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT dpth = PyFloat_AS_DOUBLE(self->depth);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    if (dpth < 0)
        dpth = 0;
    else if (dpth > 5)
        dpth = 5;

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];

        self->total_signal = 0.0;
        for (j=0; j<8; j++) {
            if (self->pointerPos[j] < 0.0)
                self->pointerPos[j] += 512.0;
            else if (self->pointerPos[j] >= 512.0)
                self->pointerPos[j] -= 512.0;
            ipart = (int)self->pointerPos[j];
            fpart = self->pointerPos[j] - ipart;
            lfo = self->delay_devs[j] * dpth * (LFO_ARRAY[ipart] + (LFO_ARRAY[ipart+1] - LFO_ARRAY[ipart]) * fpart) + self->delays[j];
            self->pointerPos[j] += self->inc[j];

            pos = self->in_count[j] - lfo;
            if (pos < 0)
                pos += self->size[j];
            ipart = (int)pos;
            fpart = pos - ipart;
            val = self->buffer[j][ipart] + (self->buffer[j][ipart+1] - self->buffer[j][ipart]) * fpart;
            self->total_signal += val;

            self->buffer[j][self->in_count[j]] = inval + val * feed;
            if (self->in_count[j] == 0)
                self->buffer[j][self->size[j]] = self->buffer[j][self->in_count[j]];
            self->in_count[j]++;
            if (self->in_count[j] >= self->size[j])
                self->in_count[j] = 0;
        }
        self->data[i] = self->total_signal * 0.25;
    }
}

static void
Chorus_process_ai(Chorus *self) {
    MYFLT lfo, pos, val, fpart, inval, dpth;
    int i, j, ipart;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *depth = Stream_getData((Stream *)self->depth_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        dpth = depth[i];
        if (dpth < 0)
            dpth = 0;
        else if (dpth > 5)
            dpth = 5;

        self->total_signal = 0.0;
        for (j=0; j<8; j++) {
            if (self->pointerPos[j] < 0.0)
                self->pointerPos[j] += 512.0;
            else if (self->pointerPos[j] >= 512.0)
                self->pointerPos[j] -= 512.0;
            ipart = (int)self->pointerPos[j];
            fpart = self->pointerPos[j] - ipart;
            lfo = self->delay_devs[j] * dpth * (LFO_ARRAY[ipart] + (LFO_ARRAY[ipart+1] - LFO_ARRAY[ipart]) * fpart) + self->delays[j];
            self->pointerPos[j] += self->inc[j];

            pos = self->in_count[j] - lfo;
            if (pos < 0)
                pos += self->size[j];
            ipart = (int)pos;
            fpart = pos - ipart;
            val = self->buffer[j][ipart] + (self->buffer[j][ipart+1] - self->buffer[j][ipart]) * fpart;
            self->total_signal += val;

            self->buffer[j][self->in_count[j]] = inval + val * feed;
            if (self->in_count[j] == 0)
                self->buffer[j][self->size[j]] = self->buffer[j][self->in_count[j]];
            self->in_count[j]++;
            if (self->in_count[j] >= self->size[j])
                self->in_count[j] = 0;
        }
        self->data[i] = self->total_signal * 0.25;
    }
}

static void
Chorus_process_ia(Chorus *self) {
    MYFLT lfo, pos, val, fpart, inval, feed;
    int i, j, ipart;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT dpth = PyFloat_AS_DOUBLE(self->depth);
    MYFLT *feedback = Stream_getData((Stream *)self->feedback_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        feed = feedback[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;

        self->total_signal = 0.0;
        for (j=0; j<8; j++) {
            if (self->pointerPos[j] < 0.0)
                self->pointerPos[j] += 512.0;
            else if (self->pointerPos[j] >= 512.0)
                self->pointerPos[j] -= 512.0;
            ipart = (int)self->pointerPos[j];
            fpart = self->pointerPos[j] - ipart;
            lfo = self->delay_devs[j] * dpth * (LFO_ARRAY[ipart] + (LFO_ARRAY[ipart+1] - LFO_ARRAY[ipart]) * fpart) + self->delays[j];
            self->pointerPos[j] += self->inc[j];

            pos = self->in_count[j] - lfo;
            if (pos < 0)
                pos += self->size[j];
            ipart = (int)pos;
            fpart = pos - ipart;
            val = self->buffer[j][ipart] + (self->buffer[j][ipart+1] - self->buffer[j][ipart]) * fpart;
            self->total_signal += val;

            self->buffer[j][self->in_count[j]] = inval + val * feed;
            if (self->in_count[j] == 0)
                self->buffer[j][self->size[j]] = self->buffer[j][self->in_count[j]];
            self->in_count[j]++;
            if (self->in_count[j] >= self->size[j])
                self->in_count[j] = 0;
        }
        self->data[i] = self->total_signal * 0.25;
    }
}

static void
Chorus_process_aa(Chorus *self) {
    MYFLT lfo, pos, val, fpart, inval, dpth, feed;
    int i, j, ipart;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *depth = Stream_getData((Stream *)self->depth_stream);
    MYFLT *feedback = Stream_getData((Stream *)self->feedback_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        dpth = depth[i];
        feed = feedback[i];
        if (dpth < 0)
            dpth = 0;
        else if (dpth > 5)
            dpth = 5;
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;

        self->total_signal = 0.0;
        for (j=0; j<8; j++) {
            if (self->pointerPos[j] < 0.0)
                self->pointerPos[j] += 512.0;
            else if (self->pointerPos[j] >= 512.0)
                self->pointerPos[j] -= 512.0;
            ipart = (int)self->pointerPos[j];
            fpart = self->pointerPos[j] - ipart;
            lfo = self->delay_devs[j] * dpth * (LFO_ARRAY[ipart] + (LFO_ARRAY[ipart+1] - LFO_ARRAY[ipart]) * fpart) + self->delays[j];
            self->pointerPos[j] += self->inc[j];

            pos = self->in_count[j] - lfo;
            if (pos < 0)
                pos += self->size[j];
            ipart = (int)pos;
            fpart = pos - ipart;
            val = self->buffer[j][ipart] + (self->buffer[j][ipart+1] - self->buffer[j][ipart]) * fpart;
            self->total_signal += val;

            self->buffer[j][self->in_count[j]] = inval + val * feed;
            if (self->in_count[j] == 0)
                self->buffer[j][self->size[j]] = self->buffer[j][self->in_count[j]];
            self->in_count[j]++;
            if (self->in_count[j] >= self->size[j])
                self->in_count[j] = 0;
        }
        self->data[i] = self->total_signal * 0.25;
    }
}

static void
Chorus_mix_i(Chorus *self) {
    int i;
    MYFLT val;

    MYFLT mix = PyFloat_AS_DOUBLE(self->mix);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (mix < 0.0)
        mix = 0.0;
    else if (mix > 1.0)
        mix = 1.0;

    for (i=0; i<self->bufsize; i++) {
        val = in[i] * (1.0 - mix) + self->data[i] * mix;
        self->data[i] = val;
    }
}

static void
Chorus_mix_a(Chorus *self) {
    int i;
    MYFLT mix, val;

    MYFLT *mi = Stream_getData((Stream *)self->mix_stream);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        mix = mi[i];
        if (mix < 0.0)
            mix = 0.0;
        else if (mix > 1.0)
            mix = 1.0;

        val = in[i] * (1.0 - mix) + self->data[i] * mix;
        self->data[i] = val;
    }
}

static void Chorus_postprocessing_ii(Chorus *self) { POST_PROCESSING_II };
static void Chorus_postprocessing_ai(Chorus *self) { POST_PROCESSING_AI };
static void Chorus_postprocessing_ia(Chorus *self) { POST_PROCESSING_IA };
static void Chorus_postprocessing_aa(Chorus *self) { POST_PROCESSING_AA };
static void Chorus_postprocessing_ireva(Chorus *self) { POST_PROCESSING_IREVA };
static void Chorus_postprocessing_areva(Chorus *self) { POST_PROCESSING_AREVA };
static void Chorus_postprocessing_revai(Chorus *self) { POST_PROCESSING_REVAI };
static void Chorus_postprocessing_revaa(Chorus *self) { POST_PROCESSING_REVAA };
static void Chorus_postprocessing_revareva(Chorus *self) { POST_PROCESSING_REVAREVA };

static void
Chorus_setProcMode(Chorus *self)
{
    int procmode, muladdmode, mixmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    mixmode = self->modebuffer[4];

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Chorus_process_ii;
            break;
        case 1:
            self->proc_func_ptr = Chorus_process_ai;
            break;
        case 10:
            self->proc_func_ptr = Chorus_process_ia;
            break;
        case 11:
            self->proc_func_ptr = Chorus_process_aa;
            break;
    }
    switch (mixmode) {
        case 0:
            self->mix_func_ptr = Chorus_mix_i;
            break;
        case 1:
            self->mix_func_ptr = Chorus_mix_a;
            break;
    }

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Chorus_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Chorus_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Chorus_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Chorus_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Chorus_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Chorus_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Chorus_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Chorus_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Chorus_postprocessing_revareva;
            break;
    }
}

static void
Chorus_compute_next_data_frame(Chorus *self)
{
    (*self->proc_func_ptr)(self);
    (*self->mix_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Chorus_traverse(Chorus *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->feedback);
    Py_VISIT(self->feedback_stream);
    Py_VISIT(self->depth);
    Py_VISIT(self->depth_stream);
    Py_VISIT(self->mix);
    Py_VISIT(self->mix_stream);
    return 0;
}

static int
Chorus_clear(Chorus *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->feedback);
    Py_CLEAR(self->feedback_stream);
    Py_CLEAR(self->depth);
    Py_CLEAR(self->depth_stream);
    Py_CLEAR(self->mix);
    Py_CLEAR(self->mix_stream);
    return 0;
}

static void
Chorus_dealloc(Chorus* self)
{
    int i;
    pyo_DEALLOC
    for (i=0; i<8; i++) {
        free(self->buffer[i]);
    }
    Chorus_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Chorus_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    long j;
    MYFLT srfac;
    PyObject *inputtmp, *input_streamtmp, *depthtmp=NULL, *feedbacktmp=NULL, *mixtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Chorus *self;
    self = (Chorus *)type->tp_alloc(type, 0);

    self->feedback = PyFloat_FromDouble(0.5);
    self->depth = PyFloat_FromDouble(1.0);
    self->mix = PyFloat_FromDouble(0.5);

    self->total_signal = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Chorus_compute_next_data_frame);
    self->mode_func_ptr = Chorus_setProcMode;

    srfac = self->sr / 44100.0;

    for (i=0; i<8; i++) {
        self->in_count[i] = 0;
        self->delays[i] = chorusParams[i][0] * srfac;
        self->delay_devs[i] = chorusParams[i][1] * srfac;
        self->inc[i] = chorusParams[i][2] * 512 / self->sr;
    }

    static char *kwlist[] = {"input", "depth", "feedback", "mix", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOO", kwlist, &inputtmp, &depthtmp, &feedbacktmp, &mixtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (depthtmp) {
        PyObject_CallMethod((PyObject *)self, "setDepth", "O", depthtmp);
    }

    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }

    if (mixtmp) {
        PyObject_CallMethod((PyObject *)self, "setMix", "O", mixtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    for (i=0; i<8; i++) {
        self->size[i] = (long)(chorusParams[i][0] * srfac * 2 + 0.5);
        self->buffer[i] = (MYFLT *)realloc(self->buffer[i], (self->size[i]+1) * sizeof(MYFLT));
        for (j=0; j<(self->size[i]+1); j++) {
            self->buffer[i][j] = 0.;
        }
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Chorus_getServer(Chorus* self) { GET_SERVER };
static PyObject * Chorus_getStream(Chorus* self) { GET_STREAM };
static PyObject * Chorus_setMul(Chorus *self, PyObject *arg) { SET_MUL };
static PyObject * Chorus_setAdd(Chorus *self, PyObject *arg) { SET_ADD };
static PyObject * Chorus_setSub(Chorus *self, PyObject *arg) { SET_SUB };
static PyObject * Chorus_setDiv(Chorus *self, PyObject *arg) { SET_DIV };

static PyObject * Chorus_play(Chorus *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Chorus_out(Chorus *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Chorus_stop(Chorus *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Chorus_multiply(Chorus *self, PyObject *arg) { MULTIPLY };
static PyObject * Chorus_inplace_multiply(Chorus *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Chorus_add(Chorus *self, PyObject *arg) { ADD };
static PyObject * Chorus_inplace_add(Chorus *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Chorus_sub(Chorus *self, PyObject *arg) { SUB };
static PyObject * Chorus_inplace_sub(Chorus *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Chorus_div(Chorus *self, PyObject *arg) { DIV };
static PyObject * Chorus_inplace_div(Chorus *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Chorus_reset(Chorus *self)
{
    int i, j;
    for (i=0; i<8; i++) {
        for (j=0; j<(self->size[i]+1); j++) {
            self->buffer[i][j] = 0.;
        }
    }
	Py_RETURN_NONE;
}

static PyObject *
Chorus_setDepth(Chorus *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->depth);
	if (isNumber == 1) {
		self->depth = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->depth = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->depth, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->depth_stream);
        self->depth_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Chorus_setFeedback(Chorus *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

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

static PyObject *
Chorus_setMix(Chorus *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->mix);
	if (isNumber == 1) {
		self->mix = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->mix = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->mix, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->mix_stream);
        self->mix_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Chorus_members[] = {
{"server", T_OBJECT_EX, offsetof(Chorus, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Chorus, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Chorus, input), 0, "Input sound object."},
{"feedback", T_OBJECT_EX, offsetof(Chorus, feedback), 0, "Feedback value."},
{"depth", T_OBJECT_EX, offsetof(Chorus, depth), 0, "Chorus depth."},
{"mix", T_OBJECT_EX, offsetof(Chorus, mix), 0, "Balance between dry and wet signals."},
{"mul", T_OBJECT_EX, offsetof(Chorus, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Chorus, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Chorus_methods[] = {
{"getServer", (PyCFunction)Chorus_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Chorus_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Chorus_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Chorus_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Chorus_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"reset", (PyCFunction)Chorus_reset, METH_NOARGS, "Reset the delay line."},
{"setFeedback", (PyCFunction)Chorus_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
{"setDepth", (PyCFunction)Chorus_setDepth, METH_O, "Sets chorus depth."},
{"setMix", (PyCFunction)Chorus_setMix, METH_O, "Sets balance between dry and wet signals."},
{"setMul", (PyCFunction)Chorus_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Chorus_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Chorus_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Chorus_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Chorus_as_number = {
(binaryfunc)Chorus_add,                      /*nb_add*/
(binaryfunc)Chorus_sub,                 /*nb_subtract*/
(binaryfunc)Chorus_multiply,                 /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
(binaryfunc)Chorus_inplace_add,              /*inplace_add*/
(binaryfunc)Chorus_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Chorus_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)Chorus_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)Chorus_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject ChorusType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Chorus_base",         /*tp_name*/
sizeof(Chorus),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Chorus_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Chorus_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Chorus objects. 8 delay lines chorus.",           /* tp_doc */
(traverseproc)Chorus_traverse,   /* tp_traverse */
(inquiry)Chorus_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Chorus_methods,             /* tp_methods */
Chorus_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Chorus_new,                 /* tp_new */
};