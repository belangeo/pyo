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
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct
{
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
LFO_generates_ii(LFO *self)
{
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

    switch (self->wavetype)
    {
        case 0: /* Saw up */
            maxHarms = (int)(self->srOverFour / freq);
            numh = sharp * 46.0 + 4.0;

            if (numh > maxHarms)
                numh = maxHarms;

            for (i = 0; i < self->bufsize; i++)
            {
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
            maxHarms = (int)(self->srOverFour / freq);
            numh = sharp * 46.0 + 4.0;

            if (numh > maxHarms)
                numh = maxHarms;

            for (i = 0; i < self->bufsize; i++)
            {
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
            maxHarms = (int)(self->srOverEight / freq);
            numh = sharp * 46.0 + 4.0;

            if (numh > maxHarms)
                numh = maxHarms;

            for (i = 0; i < self->bufsize; i++)
            {
                val = MYATAN(numh * MYSIN(TWOPI * self->pointerPos));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 3: /* Triangle */
            maxHarms = (int)(self->srOverFour / freq);

            if ((sharp * 36.0) > maxHarms)
                numh = (MYFLT)(maxHarms / 36.0);
            else
                numh = sharp;

            for (i = 0; i < self->bufsize; i++)
            {
                v1 = MYTAN(MYSIN(TWOPI * self->pointerPos)) * self->oneOverPiOverTwo;
                pointer = self->pointerPos + 0.25;

                if (pointer > 1.0)
                    pointer -= 1.0;

                v2 = 4.0 * (0.5 - MYFABS(pointer - (MYFLT)0.5)) - 1.0;
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
            maxHarms = (int)(self->srOverEight / freq);
            numh = MYFLOOR(sharp * 46.0 + 4.0);

            if (numh > maxHarms)
                numh = maxHarms;

            if (MYFMOD(numh, 2.0) == 0.0)
                numh += 1.0;

            for (i = 0; i < self->bufsize; i++)
            {
                val = MYTAN(MYPOW(MYFABS(MYSIN(TWOPI * self->pointerPos)), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 5: /* Bi-Pulse */
            maxHarms = (int)(self->srOverEight / freq);
            numh = MYFLOOR(sharp * 46.0 + 4.0);

            if (numh > maxHarms)
                numh = maxHarms;

            if (MYFMOD(numh, 2.0) == 0.0)
                numh += 1.0;

            for (i = 0; i < self->bufsize; i++)
            {
                val = MYTAN(MYPOW(MYSIN(TWOPI * self->pointerPos), numh));
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

            for (i = 0; i < self->bufsize; i++)
            {
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                {
                    self->pointerPos -= 1.0;
                    self->sahPointerPos = 0.0;
                    self->sahLastValue = self->sahCurrentValue;
                    self->sahCurrentValue = RANDOM_UNIFORM * 2.0 - 1.0;
                }

                if (self->sahPointerPos < 1.0)
                {
                    fade = 0.5 * MYSIN(PI * (self->sahPointerPos + 0.5)) + 0.5;
                    val = self->sahCurrentValue * (1.0 - fade) + self->sahLastValue * fade;
                    self->sahPointerPos += inc2;
                }
                else
                {
                    val = self->sahCurrentValue;
                }

                self->data[i] = val;
            }

            break;

        case 7: /* Sine-mod */
            inc2 = inc * sharp * 0.99;
            sharp2 = sharp * 0.5;

            for (i = 0; i < self->bufsize; i++)
            {
                self->modPointerPos += inc2;

                if (self->modPointerPos < 0)
                    self->modPointerPos += 1.0;
                else if (self->modPointerPos >= 1)
                    self->modPointerPos -= 1.0;

                val = ((sharp2 * MYCOS(TWOPI * self->modPointerPos) + sharp2) + (1.0 - sharp)) * MYSIN(TWOPI * self->pointerPos);
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
LFO_generates_ai(LFO *self)
{
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

    switch (self->wavetype)
    {
        case 0: /* Saw up */
            for (i = 0; i < self->bufsize; i++)
            {
                freq = fr[i];

                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;

                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour / freq);
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
            for (i = 0; i < self->bufsize; i++)
            {
                freq = fr[i];

                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;

                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour / freq);
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
            for (i = 0; i < self->bufsize; i++)
            {
                freq = fr[i];

                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;

                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight / freq);
                numh = sharp * 46.0 + 4.0;

                if (numh > maxHarms)
                    numh = maxHarms;

                val = MYATAN(numh * MYSIN(TWOPI * self->pointerPos));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 3: /* Triangle */
            for (i = 0; i < self->bufsize; i++)
            {
                freq = fr[i];

                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;

                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverFour / freq);

                if ((sharp * 36.0) > maxHarms)
                    numh = (MYFLT)(maxHarms / 36.0);
                else
                    numh = sharp;

                v1 = MYTAN(MYSIN(TWOPI * self->pointerPos)) * self->oneOverPiOverTwo;
                pointer = self->pointerPos + 0.25;

                if (pointer > 1.0)
                    pointer -= 1.0;

                v2 = 4.0 * (0.5 - MYFABS(pointer - (MYFLT)0.5)) - 1.0;
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
            for (i = 0; i < self->bufsize; i++)
            {
                freq = fr[i];

                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;

                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight / freq);
                numh = MYFLOOR(sharp * 46.0 + 4.0);

                if (numh > maxHarms)
                    numh = maxHarms;

                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;

                val = MYTAN(MYPOW(MYFABS(MYSIN(TWOPI * self->pointerPos)), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 5: /* Bi-Pulse */
            for (i = 0; i < self->bufsize; i++)
            {
                freq = fr[i];

                if (freq < 0.00001)
                    freq = 0.00001;
                else if (freq > self->srOverFour)
                    freq = self->srOverFour;

                inc = freq * self->oneOverSr;
                maxHarms = (int)(self->srOverEight / freq);
                numh = MYFLOOR(sharp * 46.0 + 4.0);

                if (numh > maxHarms)
                    numh = maxHarms;

                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;

                val = MYTAN(MYPOW(MYSIN(TWOPI * self->pointerPos), numh));
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

            for (i = 0; i < self->bufsize; i++)
            {
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
                else if (self->pointerPos >= 1)
                {
                    self->pointerPos -= 1.0;
                    self->sahPointerPos = 0.0;
                    self->sahLastValue = self->sahCurrentValue;
                    self->sahCurrentValue = RANDOM_UNIFORM * 2.0 - 1.0;
                }

                if (self->sahPointerPos < 1.0)
                {
                    fade = 0.5 * MYSIN(PI * (self->sahPointerPos + 0.5)) + 0.5;
                    val = self->sahCurrentValue * (1.0 - fade) + self->sahLastValue * fade;
                    self->sahPointerPos += inc2;
                }
                else
                {
                    val = self->sahCurrentValue;
                }

                self->data[i] = val;
            }

            break;

        case 7: /* Sine-mod */
            sharp2 = sharp * 0.5;

            for (i = 0; i < self->bufsize; i++)
            {
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

                val = ((sharp2 * MYCOS(TWOPI * self->modPointerPos) + sharp2) + (1.0 - sharp)) * MYSIN(TWOPI * self->pointerPos);
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
LFO_generates_ia(LFO *self)
{
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

    switch (self->wavetype)
    {
        case 0: /* Saw up */
            maxHarms = (int)(self->srOverFour / freq);

            for (i = 0; i < self->bufsize; i++)
            {
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
            maxHarms = (int)(self->srOverFour / freq);

            for (i = 0; i < self->bufsize; i++)
            {
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
            maxHarms = (int)(self->srOverEight / freq);

            for (i = 0; i < self->bufsize; i++)
            {
                sharp = sh[i];

                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;

                numh = sharp * 46.0 + 4.0;

                if (numh > maxHarms)
                    numh = maxHarms;

                val = MYATAN(numh * MYSIN(TWOPI * self->pointerPos));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 3: /* Triangle */
            maxHarms = (int)(self->srOverFour / freq);

            for (i = 0; i < self->bufsize; i++)
            {
                sharp = sh[i];

                if (sharp < 0.0)
                    sharp = 0.0;
                else if (sharp > 1.0)
                    sharp = 1.0;

                if ((sharp * 36.0) > maxHarms)
                    numh = (MYFLT)(maxHarms / 36.0);
                else
                    numh = sharp;

                v1 = MYTAN(MYSIN(TWOPI * self->pointerPos)) * self->oneOverPiOverTwo;
                pointer = self->pointerPos + 0.25;

                if (pointer > 1.0)
                    pointer -= 1.0;

                v2 = 4.0 * (0.5 - MYFABS(pointer - (MYFLT)0.5)) - 1.0;
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
            maxHarms = (int)(self->srOverEight / freq);

            for (i = 0; i < self->bufsize; i++)
            {
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

                val = MYTAN(MYPOW(MYFABS(MYSIN(TWOPI * self->pointerPos)), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 5: /* Bi-Pulse */
            maxHarms = (int)(self->srOverEight / freq);

            for (i = 0; i < self->bufsize; i++)
            {
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

                val = MYTAN(MYPOW(MYSIN(TWOPI * self->pointerPos), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 6: /* SAH */
            for (i = 0; i < self->bufsize; i++)
            {
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
                else if (self->pointerPos >= 1)
                {
                    self->pointerPos -= 1.0;
                    self->sahPointerPos = 0.0;
                    self->sahLastValue = self->sahCurrentValue;
                    self->sahCurrentValue = RANDOM_UNIFORM * 2.0 - 1.0;
                }

                if (self->sahPointerPos < 1.0)
                {
                    fade = 0.5 * MYSIN(PI * (self->sahPointerPos + 0.5)) + 0.5;
                    val = self->sahCurrentValue * (1.0 - fade) + self->sahLastValue * fade;
                    self->sahPointerPos += inc2;
                }
                else
                {
                    val = self->sahCurrentValue;
                }

                self->data[i] = val;
            }

            break;

        case 7: /* Sine-mod */
            for (i = 0; i < self->bufsize; i++)
            {
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

                val = ((sharp2 * MYCOS(TWOPI * self->modPointerPos) + sharp2) + (1.0 - sharp)) * MYSIN(TWOPI * self->pointerPos);
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
LFO_generates_aa(LFO *self)
{
    MYFLT val, inc, freq, sharp, pointer, numh;
    MYFLT v1, v2, inc2, fade;
    MYFLT sharp2 = 0.0;
    int i, maxHarms;

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *sh = Stream_getData((Stream *)self->sharp_stream);

    switch (self->wavetype)
    {
        case 0: /* Saw up */
            for (i = 0; i < self->bufsize; i++)
            {
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
                maxHarms = (int)(self->srOverFour / freq);
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
            for (i = 0; i < self->bufsize; i++)
            {
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
                maxHarms = (int)(self->srOverFour / freq);
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
            for (i = 0; i < self->bufsize; i++)
            {
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
                maxHarms = (int)(self->srOverEight / freq);
                numh = sharp * 46.0 + 4.0;

                if (numh > maxHarms)
                    numh = maxHarms;

                val = MYATAN(numh * MYSIN(TWOPI * self->pointerPos));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 3: /* Triangle */
            for (i = 0; i < self->bufsize; i++)
            {
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
                maxHarms = (int)(self->srOverFour / freq);

                if ((sharp * 36.0) > maxHarms)
                    numh = (MYFLT)(maxHarms / 36.0);
                else
                    numh = sharp;

                v1 = MYTAN(MYSIN(TWOPI * self->pointerPos)) * self->oneOverPiOverTwo;
                pointer = self->pointerPos + 0.25;

                if (pointer > 1.0)
                    pointer -= 1.0;

                v2 = 4.0 * (0.5 - MYFABS(pointer - (MYFLT)0.5)) - 1.0;
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
            for (i = 0; i < self->bufsize; i++)
            {
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
                maxHarms = (int)(self->srOverEight / freq);
                numh = MYFLOOR(sharp * 46.0 + 4.0);

                if (numh > maxHarms)
                    numh = maxHarms;

                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;

                val = MYTAN(MYPOW(MYFABS(MYSIN(TWOPI * self->pointerPos)), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 5: /* Bi-Pulse */
            for (i = 0; i < self->bufsize; i++)
            {
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
                maxHarms = (int)(self->srOverEight / freq);
                numh = MYFLOOR(sharp * 46.0 + 4.0);

                if (numh > maxHarms)
                    numh = maxHarms;

                if (MYFMOD(numh, 2.0) == 0.0)
                    numh += 1.0;

                val = MYTAN(MYPOW(MYSIN(TWOPI * self->pointerPos), numh));
                self->data[i] = val * self->oneOverPiOverTwo;
                self->pointerPos += inc;

                if (self->pointerPos < 0)
                    self->pointerPos += 1.0;
                else if (self->pointerPos >= 1)
                    self->pointerPos -= 1.0;
            }

            break;

        case 6: /* SAH */
            for (i = 0; i < self->bufsize; i++)
            {
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
                else if (self->pointerPos >= 1)
                {
                    self->pointerPos -= 1.0;
                    self->sahPointerPos = 0.0;
                    self->sahLastValue = self->sahCurrentValue;
                    self->sahCurrentValue = RANDOM_UNIFORM * 2.0 - 1.0;
                }

                if (self->sahPointerPos < 1.0)
                {
                    fade = 0.5 * MYSIN(PI * (self->sahPointerPos + 0.5)) + 0.5;
                    val = self->sahCurrentValue * (1.0 - fade) + self->sahLastValue * fade;
                    self->sahPointerPos += inc2;
                }
                else
                {
                    val = self->sahCurrentValue;
                }

                self->data[i] = val;
            }

            break;

        case 7: /* Sine-mod */
            for (i = 0; i < self->bufsize; i++)
            {
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

                val = ((sharp2 * MYCOS(TWOPI * self->modPointerPos) + sharp2) + (1.0 - sharp)) * MYSIN(TWOPI * self->pointerPos);
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

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(LFO_generates_ii);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(LFO_generates_ai);
            break;

        case 10:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(LFO_generates_ia);
            break;

        case 11:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(LFO_generates_aa);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(LFO_postprocessing_revareva);
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
    Py_VISIT(self->sharp);
    return 0;
}

static int
LFO_clear(LFO *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->sharp);
    return 0;
}

static void
LFO_dealloc(LFO* self)
{
    pyo_DEALLOC
    LFO_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
LFO_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *freqtmp = NULL, *sharptmp = NULL, *multmp = NULL, *addtmp = NULL;
    LFO *self;
    self = (LFO *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

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
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(LFO_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(LFO_setProcMode);

    static char *kwlist[] = {"freq", "sharp", "type", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOiOO", kwlist, &freqtmp, &sharptmp, &self->wavetype, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    if (freqtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setFreq", freqtmp);
    }

    if (sharptmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setSharp", sharptmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

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

static PyObject * LFO_setFreq(LFO *self, PyObject *arg) { SET_PARAM(self->freq, self->freq_stream, 2); }
static PyObject * LFO_setSharp(LFO *self, PyObject *arg) { SET_PARAM(self->sharp, self->sharp_stream, 3); }

static PyObject *
LFO_setType(LFO *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 8)
            self->wavetype = tmp;
    }

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyObject *
LFO_reset(LFO *self)
{
    self->pointerPos = 0.0;
    self->sahPointerPos = 0.0;
    self->modPointerPos = 0.0;
    Py_RETURN_NONE;
}

static PyMemberDef LFO_members[] =
{
    {"server", T_OBJECT_EX, offsetof(LFO, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(LFO, stream), 0, "Stream object."},
    {"freq", T_OBJECT_EX, offsetof(LFO, freq), 0, "Cutoff frequency in cycle per second."},
    {"sharp", T_OBJECT_EX, offsetof(LFO, sharp), 0, "Sharpness factor."},
    {"mul", T_OBJECT_EX, offsetof(LFO, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(LFO, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef LFO_methods[] =
{
    {"getServer", (PyCFunction)LFO_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)LFO_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)LFO_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)LFO_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)LFO_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
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

static PyType_Slot LFOType_slots[] = {
    {Py_tp_dealloc, LFO_dealloc},
    {Py_tp_doc, "LFO objects. Generates a Low Frequency Oscillator with different waveshapes."},
    {Py_tp_traverse, LFO_traverse},
    {Py_tp_clear, LFO_clear},
    {Py_tp_methods, LFO_methods},
    {Py_tp_members, LFO_members},
    {Py_tp_new, LFO_new},
    {Py_nb_add, LFO_add},
    {Py_nb_subtract, LFO_sub},
    {Py_nb_multiply, LFO_multiply},
    {Py_nb_true_divide, LFO_div},
    {Py_nb_inplace_add, LFO_inplace_add},
    {Py_nb_inplace_subtract, LFO_inplace_sub},
    {Py_nb_inplace_multiply, LFO_inplace_multiply},
    {Py_nb_inplace_true_divide, LFO_inplace_div},
    {0, NULL}
};

static PyType_Spec LFOType_spec =
{
    "_pyo.LFO_base",
    sizeof(LFO),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    LFOType_slots
};

PyTypeObject *
PyoCreateLFOType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &LFOType_spec, NULL);
}
