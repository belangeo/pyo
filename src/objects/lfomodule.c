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
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *freq;
    Stream *freq_stream;
    PyObject *sharp;
    Stream *sharp_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    int wavetype;
    MYFLT oneOverSr;
    MYFLT oneOverPiOverTwo;
    MYFLT srOverFour;
    MYFLT srOverEight;
    MYFLT pointerPos;
    MYFLT sahPointerPos;
    MYFLT sahCurrentValue;
    MYFLT sahLastValue;
    MYFLT modPointerPos;
} LFO;

static void
LFO_generates_ii(LFO *self) {
    MYFLT val, inc, freq, sharp, pointer, numh;
    MYFLT v1, v2, inc2, fade;
    MYFLT sharp2 = 0.0;
    int i, maxHarms;

    freq = PyFloat_AS_DOUBLE(self->freq);
    if (freq < 0.00001)
        freq = 0.00001;
    else if (freq > self->srOverFour)
        freq = self->srOverFour;

    sharp = PyFloat_AS_DOUBLE(self->sharp);
    if (sharp < 0.0)
        sharp = 0.0;
    else if (sharp > 1.0)
        sharp = 1.0;

    inc = freq * self->oneOverSr;

    switch (self->wavetype) {
        case 0: /* Saw up */
            maxHarms = (int)(self->srOverFour/freq);
            numh = sharp * 46.0 + 4.0;
            if (numh > maxHarms)
                numh = maxHarms;
            for (i=0; i<self->bufsize; i++) {
                pointer = self->pointerPos + 0.5;
                if (pointer >= 1.0)
                    pointer -= 1.0;
                pointer = pointer * 2.0 - 1.0;
                val = pointer - MYTANH(numh * pointer) / MYTANH(numh);
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 1: /* Saw down */
            maxHarms = (int)(self->srOverFour/freq);
            numh = sharp * 46.0 + 4.0;
            if (numh > maxHarms)
                numh = maxHarms;
            for (i=0; i<self->bufsize; i++) {
                pointer = self->pointerPos + 0.5;
                if (pointer >= 1.0)
                    pointer -= 1.0;
                pointer = pointer * 2.0 - 1.0;
                val = -(pointer - MYTANH(numh * pointer) / MYTANH(numh));
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 2: /* Square */
            maxHarms = (int)(self->srOverEight/freq);
            numh = sharp * 46.0 + 4.0;
            if (numh > maxHarms)
                numh = maxHarms;
            for (i=0; i<self->bufsize; i++) {
                val = MYATAN(numh * MYSIN(TWOPI*self->pointerPos));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 3: /* Triangle */
            maxHarms = (int)(self->srOverFour/freq);
            if ((sharp * 36.0) > maxHarms)
                numh = (MYFLT)(maxHarms / 36.0);
            else
                numh = sharp;
            for (i=0; i<self->bufsize; i++) {
                v1 = MYTAN(MYSIN(TWOPI*self->pointerPos)) * self->oneOverPiOverTwo;
                pointer = self->pointerPos + 0.25;
                if (pointer > 1.0)
                    pointer -= 1.0;
                v2 = 4.0 * (0.5 - MYFABS(pointer - 0.5)) - 1.0;
                val = v1 * (1 - numh) + v2 * numh;
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 4: /* Pulse */
            maxHarms = (int)(self->srOverEight/freq);
            numh = MYFLOOR(sharp * 46.0 + 4.0);
            if (numh > maxHarms)
                numh = maxHarms;
            if (MYFMOD(numh, 2.0) == 0.0)
                numh += 1.0;
            for (i=0; i<self->bufsize; i++) {
                val = MYTAN(MYPOW(MYFABS(MYSIN(TWOPI*self->pointerPos)), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 5: /* Bi-Pulse */
            maxHarms = (int)(self->srOverEight/freq);
            numh = MYFLOOR(sharp * 46.0 + 4.0);
            if (numh > maxHarms)
                numh = maxHarms;
            if (MYFMOD(numh, 2.0) == 0.0)
                numh += 1.0;
            for (i=0; i<self->bufsize; i++) {
                val = MYTAN(MYPOW(MYSIN(TWOPI*self->pointerPos), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 6: /* SAH */
            numh = 1.0 - sharp;
            inc2 = 1.0 / (int)(1.0 / inc * numh);
            for (i=0; i<self->bufsize; i++) {
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1) {
                    self->pointerPos -= 1.0;
                    self->sahPointerPos = 0.0;
                    self->sahLastValue = self->sahCurrentValue;
                    self->sahCurrentValue = RANDOM_UNIFORM * 2.0 - 1.0;
                }
                if (self->sahPointerPos < 1.0) {
                    fade = 0.5 * MYSIN(PI * (self->sahPointerPos+0.5)) + 0.5;
                    val = self->sahCurrentValue * (1.0 - fade) + self->sahLastValue * fade;
                    self->sahPointerPos += inc2;
                }
                else {
                    val = self->sahCurrentValue;
                }
                self->data[i] = val;
            }
            break;
        case 7: /* Sine-mod */
            inc2 = inc * sharp * 0.99;
            sharp2 = sharp * 0.5;
            for (i=0; i<self->bufsize; i++) {
                self->modPointerPos += inc2;
                if (self->modPointerPos < 0)
                    self->modPointerPos += 1.0;
                else if (self->modPointerPos >= 1)
                    self->modPointerPos -= 1.0;
                val = ((sharp2 * MYCOS(TWOPI*self->modPointerPos) + sharp2) + (1.0 - sharp)) * MYSIN(TWOPI*self->pointerPos);
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        default:
            break;
    }
}

static void
LFO_generates_ai(LFO *self) {
    MYFLT val, inc, freq, sharp, pointer, numh;
    MYFLT v1, v2, inc2, fade;
    MYFLT sharp2 = 0.0;
    int i, maxHarms;

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    sharp = PyFloat_AS_DOUBLE(self->sharp);
    if (sharp < 0.0)
        sharp = 0.0;
    else if (sharp > 1.0)
        sharp = 1.0;

    switch (self->wavetype) {
        case 0: /* Saw up */
            for (i=0; i<self->bufsize; i++) {
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour/freq);
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                pointer = self->pointerPos + 0.5;
                if (pointer >= 1.0)
                    pointer -= 1.0;
                pointer = pointer * 2.0 - 1.0;
                val = pointer - MYTANH(numh * pointer) / MYTANH(numh);
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 1: /* Saw down */
            for (i=0; i<self->bufsize; i++) {
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour/freq);
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                pointer = self->pointerPos + 0.5;
                if (pointer >= 1.0)
                    pointer -= 1.0;
                pointer = pointer * 2.0 - 1.0;
                val = -(pointer - MYTANH(numh * pointer) / MYTANH(numh));
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 2: /* Square */
            for (i=0; i<self->bufsize; i++) {
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight/freq);
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                val = MYATAN(numh * MYSIN(TWOPI*self->pointerPos));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 3: /* Triangle */
            for (i=0; i<self->bufsize; i++) {
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour/freq);
                if ((sharp * 36.0) > maxHarms)
                    numh = (MYFLT)(maxHarms / 36.0);
                else
                    numh = sharp;
                v1 = MYTAN(MYSIN(TWOPI*self->pointerPos)) * self->oneOverPiOverTwo;
                pointer = self->pointerPos + 0.25;
                if (pointer > 1.0)
                    pointer -= 1.0;
                v2 = 4.0 * (0.5 - MYFABS(pointer - 0.5)) - 1.0;
                val = v1 * (1 - numh) + v2 * numh;
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 4: /* Pulse */
            for (i=0; i<self->bufsize; i++) {
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight/freq);
                numh = MYFLOOR(sharp * 46.0 + 4.0);
                if (numh > maxHarms)
                    numh = maxHarms;
                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;
                val = MYTAN(MYPOW(MYFABS(MYSIN(TWOPI*self->pointerPos)), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 5: /* Bi-Pulse */
            for (i=0; i<self->bufsize; i++) {
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight/freq);
                numh = MYFLOOR(sharp * 46.0 + 4.0);
                if (numh > maxHarms)
                    numh = maxHarms;
                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;
                val = MYTAN(MYPOW(MYSIN(TWOPI*self->pointerPos), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 6: /* SAH */
            numh = 1.0 - sharp;
            for (i=0; i<self->bufsize; i++) {
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                inc2 = 1.0 / (int)(1.0 / inc * numh);
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1) {
                    self->pointerPos -= 1.0;
                    self->sahPointerPos = 0.0;
                    self->sahLastValue = self->sahCurrentValue;
                    self->sahCurrentValue = RANDOM_UNIFORM * 2.0 - 1.0;
                }
                if (self->sahPointerPos < 1.0) {
                    fade = 0.5 * MYSIN(PI * (self->sahPointerPos+0.5)) + 0.5;
                    val = self->sahCurrentValue * (1.0 - fade) + self->sahLastValue * fade;
                    self->sahPointerPos += inc2;
                }
                else {
                    val = self->sahCurrentValue;
                }
                self->data[i] = val;
            }
            break;
        case 7: /* Sine-mod */
            sharp2 = sharp * 0.5;
            for (i=0; i<self->bufsize; i++) {
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                inc2 = inc * sharp * 0.99;
                self->modPointerPos += inc2;
                if (self->modPointerPos < 0)
                    self->modPointerPos += 1.0;
                else if (self->modPointerPos >= 1)
                    self->modPointerPos -= 1.0;
                val = ((sharp2 * MYCOS(TWOPI*self->modPointerPos) + sharp2) + (1.0 - sharp)) * MYSIN(TWOPI*self->pointerPos);
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        default:
            break;
    }
}

static void
LFO_generates_ia(LFO *self) {
    MYFLT val, inc, freq, sharp, pointer, numh;
    MYFLT v1, v2, inc2, fade;
    MYFLT sharp2 = 0.0;
    int i, maxHarms;

    freq = PyFloat_AS_DOUBLE(self->freq);
    if (freq < 0.00001)
        freq = 0.00001;
    else if (freq > self->srOverFour)
        freq = self->srOverFour;
    inc = freq * self->oneOverSr;

    MYFLT *sh = Stream_getData((Stream *)self->sharp_stream);

    switch (self->wavetype) {
        case 0: /* Saw up */
            maxHarms = (int)(self->srOverFour/freq);
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                pointer = self->pointerPos + 0.5;
                if (pointer >= 1.0)
                    pointer -= 1.0;
                pointer = pointer * 2.0 - 1.0;
                val = pointer - MYTANH(numh * pointer) / MYTANH(numh);
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 1: /* Saw down */
            maxHarms = (int)(self->srOverFour/freq);
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                pointer = self->pointerPos + 0.5;
                if (pointer >= 1.0)
                    pointer -= 1.0;
                pointer = pointer * 2.0 - 1.0;
                val = -(pointer - MYTANH(numh * pointer) / MYTANH(numh));
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 2: /* Square */
            maxHarms = (int)(self->srOverEight/freq);
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                val = MYATAN(numh * MYSIN(TWOPI*self->pointerPos));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 3: /* Triangle */
            maxHarms = (int)(self->srOverFour/freq);
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                if ((sharp * 36.0) > maxHarms)
                    numh = (MYFLT)(maxHarms / 36.0);
                else
                    numh = sharp;
                v1 = MYTAN(MYSIN(TWOPI*self->pointerPos)) * self->oneOverPiOverTwo;
                pointer = self->pointerPos + 0.25;
                if (pointer > 1.0)
                    pointer -= 1.0;
                v2 = 4.0 * (0.5 - MYFABS(pointer - 0.5)) - 1.0;
                val = v1 * (1 - numh) + v2 * numh;
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 4: /* Pulse */
            maxHarms = (int)(self->srOverEight/freq);
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                numh = MYFLOOR(sharp * 46.0 + 4.0);
                if (numh > maxHarms)
                    numh = maxHarms;
                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;
                val = MYTAN(MYPOW(MYFABS(MYSIN(TWOPI*self->pointerPos)), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 5: /* Bi-Pulse */
            maxHarms = (int)(self->srOverEight/freq);
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                numh = MYFLOOR(sharp * 46.0 + 4.0);
                if (numh > maxHarms)
                    numh = maxHarms;
                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;
                val = MYTAN(MYPOW(MYSIN(TWOPI*self->pointerPos), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 6: /* SAH */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                numh = 1.0 - sharp;
                inc2 = 1.0 / (int)(1.0 / inc * numh);
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1) {
                    self->pointerPos -= 1.0;
                    self->sahPointerPos = 0.0;
                    self->sahLastValue = self->sahCurrentValue;
                    self->sahCurrentValue = RANDOM_UNIFORM * 2.0 - 1.0;
                }
                if (self->sahPointerPos < 1.0) {
                    fade = 0.5 * MYSIN(PI * (self->sahPointerPos+0.5)) + 0.5;
                    val = self->sahCurrentValue * (1.0 - fade) + self->sahLastValue * fade;
                    self->sahPointerPos += inc2;
                }
                else {
                    val = self->sahCurrentValue;
                }
                self->data[i] = val;
            }
            break;
        case 7: /* Sine-mod */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                inc2 = inc * sharp * 0.99;
                sharp2 = sharp * 0.5;
                self->modPointerPos += inc2;
                if (self->modPointerPos < 0)
                    self->modPointerPos += 1.0;
                else if (self->modPointerPos >= 1)
                    self->modPointerPos -= 1.0;
                val = ((sharp2 * MYCOS(TWOPI*self->modPointerPos) + sharp2) + (1.0 - sharp)) * MYSIN(TWOPI*self->pointerPos);
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        default:
            break;
    }
}

static void
LFO_generates_aa(LFO *self) {
    MYFLT val, inc, freq, sharp, pointer, numh;
    MYFLT v1, v2, inc2, fade;
    MYFLT sharp2 = 0.0;
    int i, maxHarms;

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *sh = Stream_getData((Stream *)self->sharp_stream);

    switch (self->wavetype) {
        case 0: /* Saw up */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour/freq);
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                pointer = self->pointerPos + 0.5;
                if (pointer >= 1.0)
                    pointer -= 1.0;
                pointer = pointer * 2.0 - 1.0;
                val = pointer - MYTANH(numh * pointer) / MYTANH(numh);
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 1: /* Saw down */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour/freq);
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                pointer = self->pointerPos + 0.5;
                if (pointer >= 1.0)
                    pointer -= 1.0;
                pointer = pointer * 2.0 - 1.0;
                val = -(pointer - MYTANH(numh * pointer) / MYTANH(numh));
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 2: /* Square */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight/freq);
                numh = sharp * 46.0 + 4.0;
                if (numh > maxHarms)
                    numh = maxHarms;
                val = MYATAN(numh * MYSIN(TWOPI*self->pointerPos));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 3: /* Triangle */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour/freq);
                if ((sharp * 36.0) > maxHarms)
                    numh = (MYFLT)(maxHarms / 36.0);
                else
                    numh = sharp;
                v1 = MYTAN(MYSIN(TWOPI*self->pointerPos)) * self->oneOverPiOverTwo;
                pointer = self->pointerPos + 0.25;
                if (pointer > 1.0)
                    pointer -= 1.0;
                v2 = 4.0 * (0.5 - MYFABS(pointer - 0.5)) - 1.0;
                val = v1 * (1 - numh) + v2 * numh;
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 4: /* Pulse */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight/freq);
                numh = MYFLOOR(sharp * 46.0 + 4.0);
                if (numh > maxHarms)
                    numh = maxHarms;
                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;
                val = MYTAN(MYPOW(MYFABS(MYSIN(TWOPI*self->pointerPos)), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 5: /* Bi-Pulse */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight/freq);
                numh = MYFLOOR(sharp * 46.0 + 4.0);
                if (numh > maxHarms)
                    numh = maxHarms;
                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;
                val = MYTAN(MYPOW(MYSIN(TWOPI*self->pointerPos), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        case 6: /* SAH */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                numh = 1.0 - sharp;
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                inc2 = 1.0 / (int)(1.0 / inc * numh);
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1) {
                    self->pointerPos -= 1.0;
                    self->sahPointerPos = 0.0;
                    self->sahLastValue = self->sahCurrentValue;
                    self->sahCurrentValue = RANDOM_UNIFORM * 2.0 - 1.0;
                }
                if (self->sahPointerPos < 1.0) {
                    fade = 0.5 * MYSIN(PI * (self->sahPointerPos+0.5)) + 0.5;
                    val = self->sahCurrentValue * (1.0 - fade) + self->sahLastValue * fade;
                    self->sahPointerPos += inc2;
                }
                else {
                    val = self->sahCurrentValue;
                }
                self->data[i] = val;
            }
            break;
        case 7: /* Sine-mod */
            for (i=0; i<self->bufsize; i++) {
                sharp = sh[i];
                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;
                freq = fr[i];
                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;
                inc = freq * self->oneOverSr;
                inc2 = inc * sharp * 0.99;
                sharp2 = sharp * 0.5;
                self->modPointerPos += inc2;
                if (self->modPointerPos < 0)
                    self->modPointerPos += 1.0;
                else if (self->modPointerPos >= 1)
                    self->modPointerPos -= 1.0;
                val = ((sharp2 * MYCOS(TWOPI*self->modPointerPos) + sharp2) + (1.0 - sharp)) * MYSIN(TWOPI*self->pointerPos);
                self->data[i] = val;
                self->pointerPos += inc;
                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }
            break;
        default:
            break;
    }
}

static void LFO_postprocessing_ii(LFO *self) { POST_PROCESSING_II };
static void LFO_postprocessing_ai(LFO *self) { POST_PROCESSING_AI };
static void LFO_postprocessing_ia(LFO *self) { POST_PROCESSING_IA };
static void LFO_postprocessing_aa(LFO *self) { POST_PROCESSING_AA };
static void LFO_postprocessing_ireva(LFO *self) { POST_PROCESSING_IREVA };
static void LFO_postprocessing_areva(LFO *self) { POST_PROCESSING_AREVA };
static void LFO_postprocessing_revai(LFO *self) { POST_PROCESSING_REVAI };
static void LFO_postprocessing_revaa(LFO *self) { POST_PROCESSING_REVAA };
static void LFO_postprocessing_revareva(LFO *self) { POST_PROCESSING_REVAREVA };

static void
LFO_setProcMode(LFO *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = LFO_generates_ii;
            break;
        case 1:
            self->proc_func_ptr = LFO_generates_ai;
            break;
        case 10:
            self->proc_func_ptr = LFO_generates_ia;
            break;
        case 11:
            self->proc_func_ptr = LFO_generates_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = LFO_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = LFO_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = LFO_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = LFO_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = LFO_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = LFO_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = LFO_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = LFO_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = LFO_postprocessing_revareva;
            break;
    }
}

static void
LFO_compute_next_data_frame(LFO *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
LFO_traverse(LFO *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->sharp);
    Py_VISIT(self->sharp_stream);
    return 0;
}

static int
LFO_clear(LFO *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->sharp);
    Py_CLEAR(self->sharp_stream);
    return 0;
}

static void
LFO_dealloc(LFO* self)
{
    pyo_DEALLOC
    LFO_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
LFO_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *freqtmp=NULL, *sharptmp=NULL, *multmp=NULL, *addtmp=NULL;
    LFO *self;
    self = (LFO *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(100);
    self->sharp = PyFloat_FromDouble(0.5);
    self->oneOverPiOverTwo = 1.0 / (PI / 2.0);
    self->wavetype = 0;
    self->pointerPos = 0.0;
    self->sahPointerPos = 0.0;
    self->modPointerPos = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->oneOverSr = 1.0 / (MYFLT)self->sr;
    self->srOverFour = (MYFLT)self->sr * 0.25;
    self->srOverEight = (MYFLT)self->sr * 0.125;
    Stream_setFunctionPtr(self->stream, LFO_compute_next_data_frame);
    self->mode_func_ptr = LFO_setProcMode;

    static char *kwlist[] = {"freq", "sharp", "type", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOiOO", kwlist, &freqtmp, &sharptmp, &self->wavetype, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (sharptmp) {
        PyObject_CallMethod((PyObject *)self, "setSharp", "O", sharptmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Server_generateSeed((Server *)self->server, LFO_ID);

    self->sahCurrentValue = self->sahLastValue = RANDOM_UNIFORM * 2.0 - 1.0;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * LFO_getServer(LFO* self) { GET_SERVER };
static PyObject * LFO_getStream(LFO* self) { GET_STREAM };
static PyObject * LFO_setMul(LFO *self, PyObject *arg) { SET_MUL };
static PyObject * LFO_setAdd(LFO *self, PyObject *arg) { SET_ADD };
static PyObject * LFO_setSub(LFO *self, PyObject *arg) { SET_SUB };
static PyObject * LFO_setDiv(LFO *self, PyObject *arg) { SET_DIV };

static PyObject * LFO_play(LFO *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * LFO_out(LFO *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * LFO_stop(LFO *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * LFO_multiply(LFO *self, PyObject *arg) { MULTIPLY };
static PyObject * LFO_inplace_multiply(LFO *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * LFO_add(LFO *self, PyObject *arg) { ADD };
static PyObject * LFO_inplace_add(LFO *self, PyObject *arg) { INPLACE_ADD };
static PyObject * LFO_sub(LFO *self, PyObject *arg) { SUB };
static PyObject * LFO_inplace_sub(LFO *self, PyObject *arg) { INPLACE_SUB };
static PyObject * LFO_div(LFO *self, PyObject *arg) { DIV };
static PyObject * LFO_inplace_div(LFO *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
LFO_setFreq(LFO *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

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
LFO_setSharp(LFO *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->sharp);
	if (isNumber == 1) {
		self->sharp = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->sharp = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->sharp, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->sharp_stream);
        self->sharp_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
LFO_setType(LFO *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		tmp = PyInt_AsLong(arg);
        if (tmp >= 0 && tmp < 8)
            self->wavetype = tmp;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
LFO_reset(LFO *self)
{
    self->pointerPos = 0.0;
    self->sahPointerPos = 0.0;
    self->modPointerPos = 0.0;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef LFO_members[] = {
    {"server", T_OBJECT_EX, offsetof(LFO, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(LFO, stream), 0, "Stream object."},
    {"freq", T_OBJECT_EX, offsetof(LFO, freq), 0, "Cutoff frequency in cycle per second."},
    {"sharp", T_OBJECT_EX, offsetof(LFO, sharp), 0, "Sharpness factor."},
    {"mul", T_OBJECT_EX, offsetof(LFO, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(LFO, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef LFO_methods[] = {
    {"getServer", (PyCFunction)LFO_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)LFO_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)LFO_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)LFO_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)LFO_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
	{"setFreq", (PyCFunction)LFO_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
    {"setSharp", (PyCFunction)LFO_setSharp, METH_O, "Sets the sharpness factor."},
    {"setType", (PyCFunction)LFO_setType, METH_O, "Sets waveform type."},
    {"reset", (PyCFunction)LFO_reset, METH_NOARGS, "Resets pointer position to 0."},
	{"setMul", (PyCFunction)LFO_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)LFO_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)LFO_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)LFO_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods LFO_as_number = {
    (binaryfunc)LFO_add,                         /*nb_add*/
    (binaryfunc)LFO_sub,                         /*nb_subtract*/
    (binaryfunc)LFO_multiply,                    /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
    (binaryfunc)LFO_inplace_add,                 /*inplace_add*/
    (binaryfunc)LFO_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)LFO_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)LFO_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)LFO_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject LFOType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.LFO_base",                                   /*tp_name*/
    sizeof(LFO),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)LFO_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &LFO_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "LFO objects. Generates a Low Frequency Oscillator with different waveshapes.",           /* tp_doc */
    (traverseproc)LFO_traverse,                  /* tp_traverse */
    (inquiry)LFO_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    LFO_methods,                                 /* tp_methods */
    LFO_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    LFO_new,                                     /* tp_new */
};