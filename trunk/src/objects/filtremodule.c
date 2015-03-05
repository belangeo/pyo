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

static MYFLT HALF_COS_ARRAY[513] = {1.0, 0.99998110153278696, 0.99992440684545181, 0.99982991808087995, 0.99969763881045715, 0.99952757403393411, 0.99931973017923825, 0.99907411510222999, 0.99879073808640628, 0.99846960984254973, 0.99811074250832332, 0.99771414964781235, 0.99727984625101107, 0.99680784873325645, 0.99629817493460782, 0.99575084411917214, 0.99516587697437664, 0.99454329561018584, 0.99388312355826691, 0.9931853857710996, 0.99245010862103322, 0.99167731989928998, 0.99086704881491472, 0.99001932599367026, 0.98913418347688054, 0.98821165472021921, 0.9872517745924454, 0.98625457937408512, 0.98522010675606064, 0.98414839583826585, 0.98303948712808786, 0.98189342253887657, 0.98071024538836005, 0.97949000039700762, 0.97823273368633901, 0.9769384927771817, 0.97560732658787452, 0.97423928543241856, 0.97283442101857576, 0.97139278644591409, 0.96991443620380113, 0.96839942616934394, 0.96684781360527761, 0.96525965715780015, 0.96363501685435693, 0.96197395410137099, 0.96027653168192206, 0.95854281375337425, 0.95677286584495025, 0.95496675485525528, 0.95312454904974775, 0.95124631805815985, 0.94933213287186513, 0.94738206584119555, 0.94539619067270686, 0.9433745824263926, 0.94131731751284708, 0.9392244736903772, 0.93709613006206383, 0.9349323670727715, 0.93273326650610799, 0.93049891148133324, 0.92822938645021758, 0.92592477719384991, 0.92358517081939495, 0.92121065575680161, 0.91880132175545981, 0.91635725988080907, 0.91387856251089561, 0.91136532333288145, 0.90881763733950294, 0.9062356008254806, 0.90361931138387919, 0.90096886790241915, 0.89828437055973898, 0.89556592082160869, 0.89281362143709486, 0.89002757643467667, 0.88720789111831455, 0.8843546720634694, 0.88146802711307481, 0.87854806537346075, 0.87559489721022943, 0.8726086342440843, 0.86958938934661101, 0.86653727663601088, 0.86345241147278784, 0.86033491045538835, 0.85718489141579368, 0.85400247341506719, 0.8507877767388532, 0.84754092289283123, 0.8442620345981231, 0.84095123578665476, 0.8376086515964718, 0.83423440836700968, 0.83082863363431847, 0.82739145612624232, 0.82392300575755428, 0.82042341362504534, 0.81689281200256991, 0.81333133433604599, 0.80973911523841147, 0.80611629048453592, 0.80246299700608914, 0.79877937288636502, 0.7950655573550629, 0.79132169078302494, 0.78754791467693042, 0.78374437167394739, 0.77991120553634141, 0.77604856114604148, 0.77215658449916424, 0.76823542270049605, 0.76428522395793219, 0.7603061375768756, 0.75629831395459302, 0.75226190457453135, 0.74819706200059122, 0.7441039398713607, 0.73998269289430851, 0.73583347683993672, 0.73165644853589207, 0.72745176586103977, 0.72321958773949491, 0.71896007413461649, 0.71467338604296105, 0.71035968548819706, 0.70601913551498185, 0.70165190018279788, 0.69725814455975277, 0.69283803471633953, 0.68839173771916018, 0.68391942162461061, 0.6794212554725293, 0.67489740927980701, 0.67034805403396192, 0.66577336168667567, 0.66117350514729512, 0.65654865827629605, 0.65189899587871258, 0.64722469369752944, 0.6425259284070397, 0.63780287760616672, 0.63305571981175202, 0.62828463445180749, 0.62348980185873359, 0.61867140326250347, 0.61382962078381298, 0.60896463742719675, 0.60407663707411186, 0.59916580447598711, 0.59423232524724023, 0.58927638585826192, 0.58429817362836856, 0.57929787671872113, 0.57427568412521424, 0.56923178567133192, 0.56416637200097319, 0.55907963457124654, 0.55397176564523298, 0.5488429582847193, 0.5436934063429012, 0.53852330445705543, 0.53333284804118442, 0.52812223327862839, 0.52289165711465235, 0.51764131724900009, 0.51237141212842374, 0.50708214093918114, 0.50177370359950879, 0.49644630075206486, 0.49110013375634509, 0.48573540468107329, 0.48035231629656205, 0.47495107206705045, 0.46953187614301212, 0.46409493335344021, 0.45864044919810504, 0.45316862983978612, 0.44767968209648135, 0.44217381343358825, 0.43665123195606403, 0.43111214640055828, 0.42555676612752463, 0.41998530111330729, 0.41439796194220363, 0.40879495979850627, 0.40317650645851943, 0.39754281428255606, 0.3918940962069094, 0.38623056573580644, 0.38055243693333718, 0.3748599244153632, 0.36915324334140731, 0.36343260940651945, 0.35769823883312568, 0.35195034836285416, 0.34618915524834432, 0.34041487724503472, 0.33462773260293199, 0.32882794005836308, 0.32301571882570607, 0.31719128858910622, 0.31135486949417079, 0.30550668213964982, 0.29964694756909749, 0.29377588726251663, 0.28789372312798917, 0.28200067749328667, 0.27609697309746906, 0.27018283308246382, 0.26425848098463345, 0.25832414072632598, 0.25238003660741054, 0.24642639329680122, 0.24046343582396335, 0.23449138957040974, 0.22851048026118126, 0.22252093395631445, 0.21652297704229864, 0.21051683622351761, 0.20450273851368242, 0.19848091122724945, 0.19245158197082995, 0.18641497863458675, 0.1803713293836198, 0.17432086264934399, 0.16826380712085329, 0.16220039173627876, 0.15613084567413366, 0.1500553983446527, 0.14397427938112045, 0.13788771863119115, 0.13179594614820278, 0.12569919218247999, 0.11959768717263308, 0.11349166173684638, 0.10738134666416307, 0.10126697290576155, 0.095148771566225324, 0.089026973894809708, 0.082901811276699419, 0.076773515224264705, 0.070642317368309157, 0.064508449449316344, 0.058372143308689985, 0.052233630879990445, 0.046093144180169916, 0.039950915300801082, 0.033807176399306589, 0.027662159690182372, 0.021516097436222258, 0.01536922193973846, 0.0092217655337806046, 0.0030739605733557966, -0.0030739605733554522, -0.0092217655337804832, -0.015369221939738116, -0.021516097436222133, -0.027662159690182025, -0.033807176399306464, -0.039950915300800735, -0.046093144180169791, -0.052233630879990098, -0.05837214330868986, -0.064508449449316232, -0.07064231736830906, -0.076773515224264371, -0.082901811276699308, -0.089026973894809375, -0.095148771566225213, -0.10126697290576121, -0.10738134666416296, -0.11349166173684605, -0.11959768717263299, -0.12569919218247966, -0.13179594614820267, -0.13788771863119104, -0.14397427938112034, -0.15005539834465259, -0.15613084567413354, -0.16220039173627843, -0.16826380712085318, -0.17432086264934366, -0.18037132938361969, -0.18641497863458642, -0.19245158197082984, -0.19848091122724912, -0.20450273851368231, -0.21051683622351727, -0.21652297704229853, -0.22252093395631434, -0.22851048026118118, -0.23449138957040966, -0.24046343582396323, -0.24642639329680088, -0.25238003660741043, -0.25832414072632565, -0.26425848098463334, -0.27018283308246349, -0.27609697309746895, -0.28200067749328633, -0.28789372312798905, -0.2937758872625163, -0.29964694756909738, -0.30550668213964971, -0.31135486949417068, -0.31719128858910589, -0.32301571882570601, -0.32882794005836274, -0.33462773260293188, -0.34041487724503444, -0.3461891552483442, -0.35195034836285388, -0.35769823883312557, -0.36343260940651911, -0.3691532433414072, -0.37485992441536287, -0.38055243693333707, -0.38623056573580633, -0.39189409620690935, -0.39754281428255578, -0.40317650645851938, -0.408794959798506, -0.41439796194220352, -0.41998530111330723, -0.42555676612752458, -0.43111214640055795, -0.43665123195606392, -0.44217381343358819, -0.44767968209648107, -0.45316862983978584, -0.45864044919810493, -0.46409493335344015, -0.46953187614301223, -0.47495107206704995, -0.48035231629656183, -0.4857354046810729, -0.49110013375634509, -0.4964463007520647, -0.50177370359950857, -0.5070821409391808, -0.51237141212842352, -0.51764131724899998, -0.52289165711465191, -0.52812223327862795, -0.53333284804118419, -0.53852330445705532, -0.5436934063429012, -0.54884295828471885, -0.55397176564523276, -0.55907963457124621, -0.56416637200097308, -0.5692317856713317, -0.57427568412521401, -0.57929787671872079, -0.58429817362836844, -0.5892763858582617, -0.5942323252472399, -0.59916580447598666, -0.60407663707411174, -0.60896463742719653, -0.61382962078381298, -0.61867140326250303, -0.62348980185873337, -0.62828463445180716, -0.6330557198117519, -0.6378028776061665, -0.64252592840703937, -0.64722469369752911, -0.65189899587871247, -0.65654865827629583, -0.66117350514729478, -0.66577336168667522, -0.67034805403396169, -0.67489740927980679, -0.6794212554725293, -0.68391942162461028, -0.68839173771915996, -0.6928380347163392, -0.69725814455975266, -0.70165190018279777, -0.70601913551498163, -0.71035968548819683, -0.71467338604296105, -0.71896007413461638, -0.72321958773949468, -0.72745176586103955, -0.73165644853589207, -0.73583347683993661, -0.73998269289430874, -0.74410393987136036, -0.74819706200059111, -0.75226190457453113, -0.75629831395459302, -0.76030613757687548, -0.76428522395793208, -0.76823542270049594, -0.77215658449916424, -0.77604856114604126, -0.77991120553634119, -0.78374437167394717, -0.78754791467693031, -0.79132169078302472, -0.7950655573550629, -0.79877937288636469, -0.80246299700608903, -0.80611629048453581, -0.80973911523841147, -0.81333133433604599, -0.8168928120025698, -0.82042341362504512, -0.82392300575755417, -0.82739145612624221, -0.83082863363431825, -0.83423440836700946, -0.8376086515964718, -0.84095123578665465, -0.8442620345981231, -0.84754092289283089, -0.85078777673885309, -0.85400247341506696, -0.85718489141579368, -0.86033491045538824, -0.86345241147278773, -0.86653727663601066, -0.86958938934661101, -0.87260863424408419, -0.87559489721022921, -0.87854806537346053, -0.88146802711307481, -0.88435467206346929, -0.88720789111831455, -0.89002757643467667, -0.89281362143709475, -0.89556592082160857, -0.89828437055973898, -0.90096886790241903, -0.90361931138387908, -0.90623560082548038, -0.90881763733950294, -0.91136532333288134, -0.9138785625108955, -0.91635725988080885, -0.91880132175545981, -0.92121065575680139, -0.92358517081939495, -0.9259247771938498, -0.92822938645021758, -0.93049891148133312, -0.93273326650610799, -0.9349323670727715, -0.93709613006206383, -0.93922447369037709, -0.94131731751284708, -0.9433745824263926, -0.94539619067270697, -0.94738206584119544, -0.94933213287186502, -0.95124631805815973, -0.95312454904974775, -0.95496675485525517, -0.95677286584495025, -0.95854281375337413, -0.96027653168192206, -0.96197395410137099, -0.96363501685435693, -0.96525965715780004, -0.9668478136052775, -0.96839942616934394, -0.96991443620380113, -0.97139278644591398, -0.97283442101857565, -0.97423928543241844, -0.97560732658787452, -0.9769384927771817, -0.9782327336863389, -0.97949000039700751, -0.98071024538836005, -0.98189342253887657, -0.98303948712808775, -0.98414839583826574, -0.98522010675606064, -0.98625457937408501, -0.9872517745924454, -0.98821165472021921, -0.98913418347688054, -0.99001932599367015, -0.99086704881491472, -0.99167731989928998, -0.99245010862103311, -0.99318538577109949, -0.99388312355826691, -0.99454329561018584, -0.99516587697437653, -0.99575084411917214, -0.99629817493460782, -0.99680784873325645, -0.99727984625101107, -0.99771414964781235, -0.99811074250832332, -0.99846960984254973, -0.99879073808640628, -0.99907411510222999, -0.99931973017923825, -0.99952757403393411, -0.99969763881045715, -0.99982991808087995, -0.99992440684545181, -0.99998110153278685, -1.0, -1.0};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    void (*coeffs_func_ptr)();
    int init;
    int modebuffer[4]; // need at least 2 slots for mul & add
    int filtertype;
    MYFLT nyquist;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // variables
    MYFLT c;
    MYFLT w0;
    MYFLT alpha;
    // coefficients
    MYFLT b0;
    MYFLT b1;
    MYFLT b2;
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
} Biquad;

static void
Biquad_compute_coeffs_lp(Biquad *self)
{
    self->b0 = self->b2 = (1 - self->c) / 2;
    self->b1 = 1 - self->c;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void
Biquad_compute_coeffs_hp(Biquad *self)
{
    self->b0 = (1 + self->c) / 2;
    self->b1 = -(1 + self->c);
    self->b2 = self->b0;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void
Biquad_compute_coeffs_bp(Biquad *self)
{
    self->b0 = self->alpha;
    self->b1 = 0;
    self->b2 = -self->alpha;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void
Biquad_compute_coeffs_bs(Biquad *self)
{
    self->b0 = 1;
    self->b1 = self->a1 = -2 * self->c;
    self->b2 = 1;
    self->a0 = 1 + self->alpha;
    self->a2 = 1 - self->alpha;
}

static void
Biquad_compute_coeffs_ap(Biquad *self)
{
    self->b0 = self->a2 = 1 - self->alpha;
    self->b1 = self->a1 = -2 * self->c;
    self->b2 = self->a0 = 1 + self->alpha;
}

static void
Biquad_compute_variables(Biquad *self, MYFLT freq, MYFLT q)
{
    if (freq <= 1)
        freq = 1;
    else if (freq >= self->nyquist)
        freq = self->nyquist;
    if (q < 0.1)
        q = 0.1;

    self->w0 = TWOPI * freq / self->sr;
    self->c = MYCOS(self->w0);
    self->alpha = MYSIN(self->w0) / (2 * q);
    (*self->coeffs_func_ptr)(self);
}

static void
Biquad_filters_ii(Biquad *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    for (i=0; i<self->bufsize; i++) {
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_ai(Biquad *self) {
    MYFLT val, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);

    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr[i], q);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_ia(Biquad *self) {
    MYFLT val, fr;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr, q[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_aa(Biquad *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr[i], q[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void Biquad_postprocessing_ii(Biquad *self) { POST_PROCESSING_II };
static void Biquad_postprocessing_ai(Biquad *self) { POST_PROCESSING_AI };
static void Biquad_postprocessing_ia(Biquad *self) { POST_PROCESSING_IA };
static void Biquad_postprocessing_aa(Biquad *self) { POST_PROCESSING_AA };
static void Biquad_postprocessing_ireva(Biquad *self) { POST_PROCESSING_IREVA };
static void Biquad_postprocessing_areva(Biquad *self) { POST_PROCESSING_AREVA };
static void Biquad_postprocessing_revai(Biquad *self) { POST_PROCESSING_REVAI };
static void Biquad_postprocessing_revaa(Biquad *self) { POST_PROCESSING_REVAA };
static void Biquad_postprocessing_revareva(Biquad *self) { POST_PROCESSING_REVAREVA };

static void
Biquad_setProcMode(Biquad *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (self->filtertype) {
        case 0:
            self->coeffs_func_ptr = Biquad_compute_coeffs_lp;
            break;
        case 1:
            self->coeffs_func_ptr = Biquad_compute_coeffs_hp;
            break;
        case 2:
            self->coeffs_func_ptr = Biquad_compute_coeffs_bp;
            break;
        case 3:
            self->coeffs_func_ptr = Biquad_compute_coeffs_bs;
            break;
        case 4:
            self->coeffs_func_ptr = Biquad_compute_coeffs_ap;
            break;
    }

	switch (procmode) {
        case 0:
            Biquad_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->q));
            self->proc_func_ptr = Biquad_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = Biquad_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = Biquad_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = Biquad_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Biquad_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Biquad_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Biquad_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Biquad_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Biquad_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Biquad_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Biquad_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Biquad_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Biquad_postprocessing_revareva;
            break;
    }
}

static void
Biquad_compute_next_data_frame(Biquad *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Biquad_traverse(Biquad *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    return 0;
}

static int
Biquad_clear(Biquad *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    return 0;
}

static void
Biquad_dealloc(Biquad* self)
{
    pyo_DEALLOC
    Biquad_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Biquad_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Biquad *self;
    self = (Biquad *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->filtertype = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->init = 1;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;

    Stream_setFunctionPtr(self->stream, Biquad_compute_next_data_frame);
    self->mode_func_ptr = Biquad_setProcMode;

    static char *kwlist[] = {"input", "freq", "q", "type", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiOO", kwlist, &inputtmp, &freqtmp, &qtmp, &self->filtertype, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Biquad_getServer(Biquad* self) { GET_SERVER };
static PyObject * Biquad_getStream(Biquad* self) { GET_STREAM };
static PyObject * Biquad_setMul(Biquad *self, PyObject *arg) { SET_MUL };
static PyObject * Biquad_setAdd(Biquad *self, PyObject *arg) { SET_ADD };
static PyObject * Biquad_setSub(Biquad *self, PyObject *arg) { SET_SUB };
static PyObject * Biquad_setDiv(Biquad *self, PyObject *arg) { SET_DIV };

static PyObject * Biquad_play(Biquad *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Biquad_out(Biquad *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Biquad_stop(Biquad *self) { STOP };

static PyObject * Biquad_multiply(Biquad *self, PyObject *arg) { MULTIPLY };
static PyObject * Biquad_inplace_multiply(Biquad *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Biquad_add(Biquad *self, PyObject *arg) { ADD };
static PyObject * Biquad_inplace_add(Biquad *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Biquad_sub(Biquad *self, PyObject *arg) { SUB };
static PyObject * Biquad_inplace_sub(Biquad *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Biquad_div(Biquad *self, PyObject *arg) { DIV };
static PyObject * Biquad_inplace_div(Biquad *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Biquad_setFreq(Biquad *self, PyObject *arg)
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
Biquad_setQ(Biquad *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Biquad_setType(Biquad *self, PyObject *arg)
{

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Biquad_members[] = {
    {"server", T_OBJECT_EX, offsetof(Biquad, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Biquad, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Biquad, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Biquad, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(Biquad, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(Biquad, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Biquad, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Biquad_methods[] = {
    {"getServer", (PyCFunction)Biquad_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Biquad_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Biquad_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Biquad_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Biquad_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Biquad_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)Biquad_setQ, METH_O, "Sets filter Q factor."},
    {"setType", (PyCFunction)Biquad_setType, METH_O, "Sets filter type factor."},
	{"setMul", (PyCFunction)Biquad_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Biquad_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Biquad_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Biquad_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Biquad_as_number = {
    (binaryfunc)Biquad_add,                         /*nb_add*/
    (binaryfunc)Biquad_sub,                         /*nb_subtract*/
    (binaryfunc)Biquad_multiply,                    /*nb_multiply*/
    (binaryfunc)Biquad_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)Biquad_inplace_add,                 /*inplace_add*/
    (binaryfunc)Biquad_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Biquad_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Biquad_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject BiquadType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Biquad_base",                                   /*tp_name*/
    sizeof(Biquad),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Biquad_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Biquad_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Biquad objects. Generates a biquadratic filter.",           /* tp_doc */
    (traverseproc)Biquad_traverse,                  /* tp_traverse */
    (inquiry)Biquad_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Biquad_methods,                                 /* tp_methods */
    Biquad_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Biquad_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    void (*coeffs_func_ptr)();
    int init;
    int modebuffer[4]; // need at least 2 slots for mul & add
    int filtertype;
    int stages;
    MYFLT nyquist;
    // sample memories
    MYFLT *x1;
    MYFLT *x2;
    MYFLT *y1;
    MYFLT *y2;
    // variables
    MYFLT c;
    MYFLT w0;
    MYFLT alpha;
    // coefficients
    MYFLT b0;
    MYFLT b1;
    MYFLT b2;
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
} Biquadx;

static void
Biquadx_allocate_memories(Biquadx *self)
{
    self->x1 = (MYFLT *)realloc(self->x1, self->stages * sizeof(MYFLT));
    self->x2 = (MYFLT *)realloc(self->x2, self->stages * sizeof(MYFLT));
    self->y1 = (MYFLT *)realloc(self->y1, self->stages * sizeof(MYFLT));
    self->y2 = (MYFLT *)realloc(self->y2, self->stages * sizeof(MYFLT));
    self->init = 1;
}

static void
Biquadx_compute_coeffs_lp(Biquadx *self)
{
    self->b0 = self->b2 = (1 - self->c) / 2;
    self->b1 = 1 - self->c;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void
Biquadx_compute_coeffs_hp(Biquadx *self)
{
    self->b0 = (1 + self->c) / 2;
    self->b1 = -(1 + self->c);
    self->b2 = self->b0;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void
Biquadx_compute_coeffs_bp(Biquadx *self)
{
    self->b0 = self->alpha;
    self->b1 = 0;
    self->b2 = -self->alpha;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void
Biquadx_compute_coeffs_bs(Biquadx *self)
{
    self->b0 = 1;
    self->b1 = self->a1 = -2 * self->c;
    self->b2 = 1;
    self->a0 = 1 + self->alpha;
    self->a2 = 1 - self->alpha;
}

static void
Biquadx_compute_coeffs_ap(Biquadx *self)
{
    self->b0 = self->a2 = 1 - self->alpha;
    self->b1 = self->a1 = -2 * self->c;
    self->b2 = self->a0 = 1 + self->alpha;
}

static void
Biquadx_compute_variables(Biquadx *self, MYFLT freq, MYFLT q)
{
    if (freq <= 1)
        freq = 1;
    else if (freq >= self->nyquist)
        freq = self->nyquist;
    if (q < 0.1)
        q = 0.1;

    self->w0 = TWOPI * freq / self->sr;
    self->c = MYCOS(self->w0);
    self->alpha = MYSIN(self->w0) / (2 * q);
    (*self->coeffs_func_ptr)(self);
}

static void
Biquadx_filters_ii(Biquadx *self) {
    MYFLT vin, vout;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        for (i=0; i<self->stages; i++) {
            self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = in[0];
        }
        self->init = 0;
    }

    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        vin = in[i];
        for (j=0; j<self->stages; j++) {
            vout = ( (self->b0 * vin) + (self->b1 * self->x1[j]) + (self->b2 * self->x2[j]) - (self->a1 * self->y1[j]) - (self->a2 * self->y2[j]) ) / self->a0;
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Biquadx_filters_ai(Biquadx *self) {
    MYFLT vin, vout, q;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        for (i=0; i<self->stages; i++) {
            self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = in[0];
        }
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);

    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        Biquadx_compute_variables(self, fr[i], q);
        vin = in[i];
        for (j=0; j<self->stages; j++) {
            vout = ( (self->b0 * vin) + (self->b1 * self->x1[j]) + (self->b2 * self->x2[j]) - (self->a1 * self->y1[j]) - (self->a2 * self->y2[j]) ) / self->a0;
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Biquadx_filters_ia(Biquadx *self) {
    MYFLT vin, vout, fr;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        for (i=0; i<self->stages; i++) {
            self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = in[0];
        }
        self->init = 0;
    }

    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        Biquadx_compute_variables(self, fr, q[i]);
        vin = in[i];
        for (j=0; j<self->stages; j++) {
            vout = ( (self->b0 * vin) + (self->b1 * self->x1[j]) + (self->b2 * self->x2[j]) - (self->a1 * self->y1[j]) - (self->a2 * self->y2[j]) ) / self->a0;
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Biquadx_filters_aa(Biquadx *self) {
    MYFLT vin, vout;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        for (i=0; i<self->stages; i++) {
            self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = in[0];
        }
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        Biquadx_compute_variables(self, fr[i], q[i]);
        vin = in[i];
        for (j=0; j<self->stages; j++) {
            vout = ( (self->b0 * vin) + (self->b1 * self->x1[j]) + (self->b2 * self->x2[j]) - (self->a1 * self->y1[j]) - (self->a2 * self->y2[j]) ) / self->a0;
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void Biquadx_postprocessing_ii(Biquadx *self) { POST_PROCESSING_II };
static void Biquadx_postprocessing_ai(Biquadx *self) { POST_PROCESSING_AI };
static void Biquadx_postprocessing_ia(Biquadx *self) { POST_PROCESSING_IA };
static void Biquadx_postprocessing_aa(Biquadx *self) { POST_PROCESSING_AA };
static void Biquadx_postprocessing_ireva(Biquadx *self) { POST_PROCESSING_IREVA };
static void Biquadx_postprocessing_areva(Biquadx *self) { POST_PROCESSING_AREVA };
static void Biquadx_postprocessing_revai(Biquadx *self) { POST_PROCESSING_REVAI };
static void Biquadx_postprocessing_revaa(Biquadx *self) { POST_PROCESSING_REVAA };
static void Biquadx_postprocessing_revareva(Biquadx *self) { POST_PROCESSING_REVAREVA };

static void
Biquadx_setProcMode(Biquadx *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (self->filtertype) {
        case 0:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_lp;
            break;
        case 1:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_hp;
            break;
        case 2:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_bp;
            break;
        case 3:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_bs;
            break;
        case 4:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_ap;
            break;
    }

	switch (procmode) {
        case 0:
            Biquadx_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->q));
            self->proc_func_ptr = Biquadx_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = Biquadx_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = Biquadx_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = Biquadx_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Biquadx_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Biquadx_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Biquadx_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Biquadx_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Biquadx_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Biquadx_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Biquadx_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Biquadx_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Biquadx_postprocessing_revareva;
            break;
    }
}

static void
Biquadx_compute_next_data_frame(Biquadx *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Biquadx_traverse(Biquadx *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    return 0;
}

static int
Biquadx_clear(Biquadx *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    return 0;
}

static void
Biquadx_dealloc(Biquadx* self)
{
    pyo_DEALLOC
    free(self->x1);
    free(self->x2);
    free(self->y1);
    free(self->y2);
    Biquadx_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Biquadx_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Biquadx *self;
    self = (Biquadx *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->filtertype = 0;
    self->stages = 4;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->init = 1;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;

    Stream_setFunctionPtr(self->stream, Biquadx_compute_next_data_frame);
    self->mode_func_ptr = Biquadx_setProcMode;

    static char *kwlist[] = {"input", "freq", "q", "type", "stages", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiiOO", kwlist, &inputtmp, &freqtmp, &qtmp, &self->filtertype, &self->stages, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Biquadx_allocate_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Biquadx_getServer(Biquadx* self) { GET_SERVER };
static PyObject * Biquadx_getStream(Biquadx* self) { GET_STREAM };
static PyObject * Biquadx_setMul(Biquadx *self, PyObject *arg) { SET_MUL };
static PyObject * Biquadx_setAdd(Biquadx *self, PyObject *arg) { SET_ADD };
static PyObject * Biquadx_setSub(Biquadx *self, PyObject *arg) { SET_SUB };
static PyObject * Biquadx_setDiv(Biquadx *self, PyObject *arg) { SET_DIV };

static PyObject * Biquadx_play(Biquadx *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Biquadx_out(Biquadx *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Biquadx_stop(Biquadx *self) { STOP };

static PyObject * Biquadx_multiply(Biquadx *self, PyObject *arg) { MULTIPLY };
static PyObject * Biquadx_inplace_multiply(Biquadx *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Biquadx_add(Biquadx *self, PyObject *arg) { ADD };
static PyObject * Biquadx_inplace_add(Biquadx *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Biquadx_sub(Biquadx *self, PyObject *arg) { SUB };
static PyObject * Biquadx_inplace_sub(Biquadx *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Biquadx_div(Biquadx *self, PyObject *arg) { DIV };
static PyObject * Biquadx_inplace_div(Biquadx *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Biquadx_setFreq(Biquadx *self, PyObject *arg)
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
Biquadx_setQ(Biquadx *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Biquadx_setType(Biquadx *self, PyObject *arg)
{

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Biquadx_setStages(Biquadx *self, PyObject *arg)
{

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->stages = PyInt_AsLong(arg);
        Biquadx_allocate_memories(self);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Biquadx_members[] = {
    {"server", T_OBJECT_EX, offsetof(Biquadx, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Biquadx, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Biquadx, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Biquadx, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(Biquadx, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(Biquadx, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Biquadx, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Biquadx_methods[] = {
    {"getServer", (PyCFunction)Biquadx_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Biquadx_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Biquadx_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Biquadx_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Biquadx_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Biquadx_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)Biquadx_setQ, METH_O, "Sets filter Q factor."},
    {"setType", (PyCFunction)Biquadx_setType, METH_O, "Sets filter type factor."},
    {"setStages", (PyCFunction)Biquadx_setStages, METH_O, "Sets the number of filtering stages."},
	{"setMul", (PyCFunction)Biquadx_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Biquadx_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Biquadx_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Biquadx_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Biquadx_as_number = {
    (binaryfunc)Biquadx_add,                         /*nb_add*/
    (binaryfunc)Biquadx_sub,                         /*nb_subtract*/
    (binaryfunc)Biquadx_multiply,                    /*nb_multiply*/
    (binaryfunc)Biquadx_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)Biquadx_inplace_add,                 /*inplace_add*/
    (binaryfunc)Biquadx_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Biquadx_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Biquadx_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject BiquadxType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Biquadx_base",                                   /*tp_name*/
    sizeof(Biquadx),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Biquadx_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Biquadx_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Biquadx objects. Generates a biquadratic filter.",           /* tp_doc */
    (traverseproc)Biquadx_traverse,                  /* tp_traverse */
    (inquiry)Biquadx_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Biquadx_methods,                                 /* tp_methods */
    Biquadx_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Biquadx_new,                                     /* tp_new */
};

/*** Biquad filter with direct coefficient control ***/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    // coefficients
    Stream *b0_stream;
    Stream *b1_stream;
    Stream *b2_stream;
    Stream *a0_stream;
    Stream *a1_stream;
    Stream *a2_stream;
    int init;
    int modebuffer[2]; // need at least 2 slots for mul & add
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
} Biquada;

static void
Biquada_filters(Biquada *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *b0 = Stream_getData((Stream *)self->b0_stream);
    MYFLT *b1 = Stream_getData((Stream *)self->b1_stream);
    MYFLT *b2 = Stream_getData((Stream *)self->b2_stream);
    MYFLT *a0 = Stream_getData((Stream *)self->a0_stream);
    MYFLT *a1 = Stream_getData((Stream *)self->a1_stream);
    MYFLT *a2 = Stream_getData((Stream *)self->a2_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    for (i=0; i<self->bufsize; i++) {
        val = ( (b0[i] * in[i]) + (b1[i] * self->x1) + (b2[i] * self->x2) - (a1[i] * self->y1) - (a2[i] * self->y2) ) / a0[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
    }
}

static void Biquada_postprocessing_ii(Biquada *self) { POST_PROCESSING_II };
static void Biquada_postprocessing_ai(Biquada *self) { POST_PROCESSING_AI };
static void Biquada_postprocessing_ia(Biquada *self) { POST_PROCESSING_IA };
static void Biquada_postprocessing_aa(Biquada *self) { POST_PROCESSING_AA };
static void Biquada_postprocessing_ireva(Biquada *self) { POST_PROCESSING_IREVA };
static void Biquada_postprocessing_areva(Biquada *self) { POST_PROCESSING_AREVA };
static void Biquada_postprocessing_revai(Biquada *self) { POST_PROCESSING_REVAI };
static void Biquada_postprocessing_revaa(Biquada *self) { POST_PROCESSING_REVAA };
static void Biquada_postprocessing_revareva(Biquada *self) { POST_PROCESSING_REVAREVA };

static void
Biquada_setProcMode(Biquada *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Biquada_filters;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Biquada_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Biquada_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Biquada_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Biquada_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Biquada_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Biquada_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Biquada_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Biquada_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Biquada_postprocessing_revareva;
            break;
    }
}

static void
Biquada_compute_next_data_frame(Biquada *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Biquada_traverse(Biquada *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->b0_stream);
    Py_VISIT(self->b1_stream);
    Py_VISIT(self->b2_stream);
    Py_VISIT(self->a0_stream);
    Py_VISIT(self->a1_stream);
    Py_VISIT(self->a2_stream);
    return 0;
}

static int
Biquada_clear(Biquada *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->b0_stream);
    Py_CLEAR(self->b1_stream);
    Py_CLEAR(self->b2_stream);
    Py_CLEAR(self->a0_stream);
    Py_CLEAR(self->a1_stream);
    Py_CLEAR(self->a2_stream);
    return 0;
}

static void
Biquada_dealloc(Biquada* self)
{
    pyo_DEALLOC
    Biquada_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Biquada_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *b0tmp, *b1tmp, *b2tmp, *a0tmp, *a1tmp, *a2tmp, *multmp=NULL, *addtmp=NULL;
    Biquada *self;
    self = (Biquada *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    self->init = 1;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, Biquada_compute_next_data_frame);
    self->mode_func_ptr = Biquada_setProcMode;

    static char *kwlist[] = {"input", "b0", "b1", "b2", "a0", "a1", "a2", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOOOOOOOO", kwlist, &inputtmp, &b0tmp, &b1tmp, &b2tmp, &a0tmp, &a1tmp, &a2tmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (b0tmp)
        PyObject_CallMethod((PyObject *)self, "setB0", "O", b0tmp);
    if (b1tmp)
        PyObject_CallMethod((PyObject *)self, "setB1", "O", b1tmp);
    if (b2tmp)
        PyObject_CallMethod((PyObject *)self, "setB2", "O", b2tmp);
    if (a0tmp)
        PyObject_CallMethod((PyObject *)self, "setA0", "O", a0tmp);
    if (a1tmp)
        PyObject_CallMethod((PyObject *)self, "setA1", "O", a1tmp);
    if (a2tmp)
        PyObject_CallMethod((PyObject *)self, "setA2", "O", a2tmp);
    if (multmp)
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    if (addtmp)
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Biquada_getServer(Biquada* self) { GET_SERVER };
static PyObject * Biquada_getStream(Biquada* self) { GET_STREAM };
static PyObject * Biquada_setMul(Biquada *self, PyObject *arg) { SET_MUL };
static PyObject * Biquada_setAdd(Biquada *self, PyObject *arg) { SET_ADD };
static PyObject * Biquada_setSub(Biquada *self, PyObject *arg) { SET_SUB };
static PyObject * Biquada_setDiv(Biquada *self, PyObject *arg) { SET_DIV };

static PyObject * Biquada_play(Biquada *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Biquada_out(Biquada *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Biquada_stop(Biquada *self) { STOP };

static PyObject * Biquada_multiply(Biquada *self, PyObject *arg) { MULTIPLY };
static PyObject * Biquada_inplace_multiply(Biquada *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Biquada_add(Biquada *self, PyObject *arg) { ADD };
static PyObject * Biquada_inplace_add(Biquada *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Biquada_sub(Biquada *self, PyObject *arg) { SUB };
static PyObject * Biquada_inplace_sub(Biquada *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Biquada_div(Biquada *self, PyObject *arg) { DIV };
static PyObject * Biquada_inplace_div(Biquada *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Biquada_setB0(Biquada *self, PyObject *arg)
{
	PyObject *streamtmp;

	if (arg == NULL)
		Py_RETURN_NONE;

    streamtmp = PyObject_CallMethod((PyObject *)arg, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->b0_stream);
    self->b0_stream = (Stream *)streamtmp;

	Py_RETURN_NONE;
}

static PyObject *
Biquada_setB1(Biquada *self, PyObject *arg)
{
	PyObject *streamtmp;

	if (arg == NULL)
		Py_RETURN_NONE;

    streamtmp = PyObject_CallMethod((PyObject *)arg, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->b1_stream);
    self->b1_stream = (Stream *)streamtmp;

	Py_RETURN_NONE;
}

static PyObject *
Biquada_setB2(Biquada *self, PyObject *arg)
{
	PyObject *streamtmp;

	if (arg == NULL)
		Py_RETURN_NONE;

    streamtmp = PyObject_CallMethod((PyObject *)arg, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->b2_stream);
    self->b2_stream = (Stream *)streamtmp;

	Py_RETURN_NONE;
}

static PyObject *
Biquada_setA0(Biquada *self, PyObject *arg)
{
	PyObject *streamtmp;

	if (arg == NULL)
		Py_RETURN_NONE;

    streamtmp = PyObject_CallMethod((PyObject *)arg, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->a0_stream);
    self->a0_stream = (Stream *)streamtmp;

	Py_RETURN_NONE;
}

static PyObject *
Biquada_setA1(Biquada *self, PyObject *arg)
{
	PyObject *streamtmp;

	if (arg == NULL)
		Py_RETURN_NONE;

    streamtmp = PyObject_CallMethod((PyObject *)arg, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->a1_stream);
    self->a1_stream = (Stream *)streamtmp;

	Py_RETURN_NONE;
}

static PyObject *
Biquada_setA2(Biquada *self, PyObject *arg)
{
	PyObject *streamtmp;

	if (arg == NULL)
		Py_RETURN_NONE;

    streamtmp = PyObject_CallMethod((PyObject *)arg, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->a2_stream);
    self->a2_stream = (Stream *)streamtmp;

	Py_RETURN_NONE;
}

static PyMemberDef Biquada_members[] = {
    {"server", T_OBJECT_EX, offsetof(Biquada, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Biquada, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Biquada, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Biquada, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Biquada, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Biquada_methods[] = {
    {"getServer", (PyCFunction)Biquada_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Biquada_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Biquada_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Biquada_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Biquada_stop, METH_NOARGS, "Stops computing."},
	{"setB0", (PyCFunction)Biquada_setB0, METH_O, "Sets b0 coefficient."},
	{"setB1", (PyCFunction)Biquada_setB1, METH_O, "Sets b1 coefficient."},
	{"setB2", (PyCFunction)Biquada_setB2, METH_O, "Sets b2 coefficient."},
	{"setA0", (PyCFunction)Biquada_setA0, METH_O, "Sets a0 coefficient."},
	{"setA1", (PyCFunction)Biquada_setA1, METH_O, "Sets a1 coefficient."},
	{"setA2", (PyCFunction)Biquada_setA2, METH_O, "Sets a2 coefficient."},
	{"setMul", (PyCFunction)Biquada_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Biquada_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Biquada_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Biquada_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Biquada_as_number = {
    (binaryfunc)Biquada_add,                         /*nb_add*/
    (binaryfunc)Biquada_sub,                         /*nb_subtract*/
    (binaryfunc)Biquada_multiply,                    /*nb_multiply*/
    (binaryfunc)Biquada_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)Biquada_inplace_add,                 /*inplace_add*/
    (binaryfunc)Biquada_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Biquada_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Biquada_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject BiquadaType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Biquada_base",                                   /*tp_name*/
    sizeof(Biquada),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Biquada_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Biquada_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Biquada objects. Generates a biquadratic filter with direct coefficient control.",           /* tp_doc */
    (traverseproc)Biquada_traverse,                  /* tp_traverse */
    (inquiry)Biquada_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Biquada_methods,                                 /* tp_methods */
    Biquada_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Biquada_new,                                     /* tp_new */
};

/*** Typical EQ filter ***/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    PyObject *boost;
    Stream *boost_stream;
    void (*coeffs_func_ptr)();
    int init;
    int modebuffer[5]; // need at least 2 slots for mul & add
    int filtertype;
    MYFLT nyquist;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // variables
    MYFLT A;
    MYFLT c;
    MYFLT w0;
    MYFLT alpha;
    // coefficients
    MYFLT b0;
    MYFLT b1;
    MYFLT b2;
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
} EQ;

static void
EQ_compute_coeffs_peak(EQ *self)
{
    MYFLT alphaMul = self->alpha * self->A;
    MYFLT alphaDiv = self->alpha / self->A;

    self->b0 = 1.0 + alphaMul;
    self->b1 = self->a1 = -2.0 * self->c;
    self->b2 = 1.0 - alphaMul;
    self->a0 = 1.0 + alphaDiv;
    self->a2 = 1.0 - alphaDiv;
}

static void
EQ_compute_coeffs_lowshelf(EQ *self)
{
    MYFLT twoSqrtAAlpha = MYSQRT(self->A * 2.0)*self->alpha;
    MYFLT AminOneC = (self->A - 1.0) * self->c;
    MYFLT AAddOneC = (self->A + 1.0) * self->c;

    self->b0 = self->A * ((self->A + 1.0) - AminOneC + twoSqrtAAlpha);
    self->b1 = 2.0 * self->A * ((self->A - 1.0) - AAddOneC);
    self->b2 = self->A * ((self->A + 1.0) - AminOneC - twoSqrtAAlpha);
    self->a0 = (self->A + 1.0) + AminOneC + twoSqrtAAlpha;
    self->a1 = -2.0 * ((self->A - 1.0) + AAddOneC);
    self->a2 = (self->A + 1.0) + AminOneC - twoSqrtAAlpha;
}

static void
EQ_compute_coeffs_highshelf(EQ *self)
{
    MYFLT twoSqrtAAlpha = MYSQRT(self->A * 2.0)*self->alpha;
    MYFLT AminOneC = (self->A - 1.0) * self->c;
    MYFLT AAddOneC = (self->A + 1.0) * self->c;

    self->b0 = self->A * ((self->A + 1.0) + AminOneC + twoSqrtAAlpha);
    self->b1 = -2.0 * self->A * ((self->A - 1.0) + AAddOneC);
    self->b2 = self->A * ((self->A + 1.0) + AminOneC - twoSqrtAAlpha);
    self->a0 = (self->A + 1.0) - AminOneC + twoSqrtAAlpha;
    self->a1 = 2.0 * ((self->A - 1.0) - AAddOneC);
    self->a2 = (self->A + 1.0) - AminOneC - twoSqrtAAlpha;
}

static void
EQ_compute_variables(EQ *self, MYFLT freq, MYFLT q, MYFLT boost)
{
    if (freq <= 1)
        freq = 1;
    else if (freq >= self->nyquist)
        freq = self->nyquist;

    self->A = MYPOW(10.0, boost/40.0);
    self->w0 = TWOPI * freq / self->sr;
    self->c = MYCOS(self->w0);
    self->alpha = MYSIN(self->w0) / (2 * q);
    (*self->coeffs_func_ptr)(self);
}

static void
EQ_filters_iii(EQ *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    for (i=0; i<self->bufsize; i++) {
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_aii(EQ *self) {
    MYFLT val, q, boost;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    boost = PyFloat_AS_DOUBLE(self->boost);

    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr[i], q, boost);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_iai(EQ *self) {
    MYFLT val, fr, boost;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    boost = PyFloat_AS_DOUBLE(self->boost);

    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr, q[i], boost);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_aai(EQ *self) {
    MYFLT val, boost;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    boost = PyFloat_AS_DOUBLE(self->boost);

    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr[i], q[i], boost);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_iia(EQ *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    fr = PyFloat_AS_DOUBLE(self->freq);
    q = PyFloat_AS_DOUBLE(self->q);
    MYFLT *boost = Stream_getData((Stream *)self->boost_stream);

    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr, q, boost[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_aia(EQ *self) {
    MYFLT val, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    MYFLT *boost = Stream_getData((Stream *)self->boost_stream);

    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr[i], q, boost[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_iaa(EQ *self) {
    MYFLT val, fr;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    MYFLT *boost = Stream_getData((Stream *)self->boost_stream);

    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr, q[i], boost[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_aaa(EQ *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    MYFLT *boost = Stream_getData((Stream *)self->boost_stream);

    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr[i], q[i], boost[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void EQ_postprocessing_ii(EQ *self) { POST_PROCESSING_II };
static void EQ_postprocessing_ai(EQ *self) { POST_PROCESSING_AI };
static void EQ_postprocessing_ia(EQ *self) { POST_PROCESSING_IA };
static void EQ_postprocessing_aa(EQ *self) { POST_PROCESSING_AA };
static void EQ_postprocessing_ireva(EQ *self) { POST_PROCESSING_IREVA };
static void EQ_postprocessing_areva(EQ *self) { POST_PROCESSING_AREVA };
static void EQ_postprocessing_revai(EQ *self) { POST_PROCESSING_REVAI };
static void EQ_postprocessing_revaa(EQ *self) { POST_PROCESSING_REVAA };
static void EQ_postprocessing_revareva(EQ *self) { POST_PROCESSING_REVAREVA };

static void
EQ_setProcMode(EQ *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (self->filtertype) {
        case 0:
            self->coeffs_func_ptr = EQ_compute_coeffs_peak;
            break;
        case 1:
            self->coeffs_func_ptr = EQ_compute_coeffs_lowshelf;
            break;
        case 2:
            self->coeffs_func_ptr = EQ_compute_coeffs_highshelf;
            break;
    }

	switch (procmode) {
        case 0:
            EQ_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->q), PyFloat_AS_DOUBLE(self->boost));
            self->proc_func_ptr = EQ_filters_iii;
            break;
        case 1:
            self->proc_func_ptr = EQ_filters_aii;
            break;
        case 10:
            self->proc_func_ptr = EQ_filters_iai;
            break;
        case 11:
            self->proc_func_ptr = EQ_filters_aai;
            break;
        case 100:
            self->proc_func_ptr = EQ_filters_iia;
            break;
        case 101:
            self->proc_func_ptr = EQ_filters_aia;
            break;
        case 110:
            self->proc_func_ptr = EQ_filters_iaa;
            break;
        case 111:
            self->proc_func_ptr = EQ_filters_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = EQ_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = EQ_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = EQ_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = EQ_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = EQ_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = EQ_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = EQ_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = EQ_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = EQ_postprocessing_revareva;
            break;
    }
}

static void
EQ_compute_next_data_frame(EQ *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
EQ_traverse(EQ *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    Py_VISIT(self->boost);
    Py_VISIT(self->boost_stream);
    return 0;
}

static int
EQ_clear(EQ *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    Py_CLEAR(self->boost);
    Py_CLEAR(self->boost_stream);
    return 0;
}

static void
EQ_dealloc(EQ* self)
{
    pyo_DEALLOC
    EQ_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
EQ_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *boosttmp=NULL, *multmp=NULL, *addtmp=NULL;
    EQ *self;
    self = (EQ *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->boost = PyFloat_FromDouble(-3.0);
    self->filtertype = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    self->init = 1;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;

    Stream_setFunctionPtr(self->stream, EQ_compute_next_data_frame);
    self->mode_func_ptr = EQ_setProcMode;

    static char *kwlist[] = {"input", "freq", "q", "boost", "type", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOiOO", kwlist, &inputtmp, &freqtmp, &qtmp, &boosttmp, &self->filtertype, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (boosttmp) {
        PyObject_CallMethod((PyObject *)self, "setBoost", "O", boosttmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * EQ_getServer(EQ* self) { GET_SERVER };
static PyObject * EQ_getStream(EQ* self) { GET_STREAM };
static PyObject * EQ_setMul(EQ *self, PyObject *arg) { SET_MUL };
static PyObject * EQ_setAdd(EQ *self, PyObject *arg) { SET_ADD };
static PyObject * EQ_setSub(EQ *self, PyObject *arg) { SET_SUB };
static PyObject * EQ_setDiv(EQ *self, PyObject *arg) { SET_DIV };

static PyObject * EQ_play(EQ *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * EQ_out(EQ *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * EQ_stop(EQ *self) { STOP };

static PyObject * EQ_multiply(EQ *self, PyObject *arg) { MULTIPLY };
static PyObject * EQ_inplace_multiply(EQ *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * EQ_add(EQ *self, PyObject *arg) { ADD };
static PyObject * EQ_inplace_add(EQ *self, PyObject *arg) { INPLACE_ADD };
static PyObject * EQ_sub(EQ *self, PyObject *arg) { SUB };
static PyObject * EQ_inplace_sub(EQ *self, PyObject *arg) { INPLACE_SUB };
static PyObject * EQ_div(EQ *self, PyObject *arg) { DIV };
static PyObject * EQ_inplace_div(EQ *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
EQ_setFreq(EQ *self, PyObject *arg)
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
EQ_setQ(EQ *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
EQ_setBoost(EQ *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->boost);
	if (isNumber == 1) {
		self->boost = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->boost = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->boost, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->boost_stream);
        self->boost_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
EQ_setType(EQ *self, PyObject *arg)
{

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef EQ_members[] = {
{"server", T_OBJECT_EX, offsetof(EQ, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(EQ, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(EQ, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(EQ, freq), 0, "Cutoff frequency in cycle per second."},
{"q", T_OBJECT_EX, offsetof(EQ, q), 0, "Q factor."},
{"boost", T_OBJECT_EX, offsetof(EQ, boost), 0, "Boost factor."},
{"mul", T_OBJECT_EX, offsetof(EQ, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(EQ, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef EQ_methods[] = {
{"getServer", (PyCFunction)EQ_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)EQ_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)EQ_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)EQ_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)EQ_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)EQ_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setQ", (PyCFunction)EQ_setQ, METH_O, "Sets filter Q factor."},
{"setBoost", (PyCFunction)EQ_setBoost, METH_O, "Sets filter boost factor."},
{"setType", (PyCFunction)EQ_setType, METH_O, "Sets filter type factor."},
{"setMul", (PyCFunction)EQ_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)EQ_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)EQ_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)EQ_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods EQ_as_number = {
(binaryfunc)EQ_add,                         /*nb_add*/
(binaryfunc)EQ_sub,                         /*nb_subtract*/
(binaryfunc)EQ_multiply,                    /*nb_multiply*/
(binaryfunc)EQ_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)EQ_inplace_add,                 /*inplace_add*/
(binaryfunc)EQ_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)EQ_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)EQ_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject EQType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.EQ_base",                                   /*tp_name*/
sizeof(EQ),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)EQ_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&EQ_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"EQ objects. Generates a biquadratic filter.",           /* tp_doc */
(traverseproc)EQ_traverse,                  /* tp_traverse */
(inquiry)EQ_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
EQ_methods,                                 /* tp_methods */
EQ_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
EQ_new,                                     /* tp_new */
};

/* Performs portamento on audio signal */
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *risetime;
    PyObject *falltime;
    Stream *risetime_stream;
    Stream *falltime_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT y1; // sample memory
    MYFLT x1;
    int dir;
} Port;

static void
direction(Port *self, MYFLT val)
{
    if (val == self->x1)
        return;
    else if (val > self->x1)
        self->dir = 1;
    else
        self->dir = 0;
    self->x1 = val;
}

static void
Port_filters_ii(Port *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT risetime = PyFloat_AS_DOUBLE(self->risetime);
    MYFLT falltime = PyFloat_AS_DOUBLE(self->falltime);
    MYFLT risefactor = 1. / ((risetime + 0.001) * self->sr);
    MYFLT fallfactor = 1. / ((falltime + 0.001) * self->sr);
    MYFLT factors[2] = {fallfactor, risefactor};

    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        val = self->y1 + (in[i] - self->y1) * factors[self->dir];
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_ai(Port *self) {
    MYFLT val, risefactor;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *risetime = Stream_getData((Stream *)self->risetime_stream);
    MYFLT falltime = PyFloat_AS_DOUBLE(self->falltime);
    MYFLT fallfactor = 1. / ((falltime + 0.001) * self->sr);

    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        risefactor = (*risetime++ + 0.001) * self->sr;
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) / risefactor;
        else
            val = self->y1 + (*in++ - self->y1) * fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_ia(Port *self) {
    MYFLT val, fallfactor;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *falltime = Stream_getData((Stream *)self->falltime_stream);
    MYFLT risetime = PyFloat_AS_DOUBLE(self->risetime);
    MYFLT risefactor = 1. / ((risetime + 0.001) * self->sr);

    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        fallfactor = (*falltime++ + 0.001) * self->sr;
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) * risefactor;
        else
            val = self->y1 + (*in++ - self->y1) / fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_aa(Port *self) {
    MYFLT val, risefactor, fallfactor;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *risetime = Stream_getData((Stream *)self->risetime_stream);
    MYFLT *falltime = Stream_getData((Stream *)self->falltime_stream);

    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        risefactor = (*risetime++ + 0.001) * self->sr;
        fallfactor = (*falltime++ + 0.001) * self->sr;
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) / risefactor;
        else
            val = self->y1 + (*in++ - self->y1) / fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void Port_postprocessing_ii(Port *self) { POST_PROCESSING_II };
static void Port_postprocessing_ai(Port *self) { POST_PROCESSING_AI };
static void Port_postprocessing_ia(Port *self) { POST_PROCESSING_IA };
static void Port_postprocessing_aa(Port *self) { POST_PROCESSING_AA };
static void Port_postprocessing_ireva(Port *self) { POST_PROCESSING_IREVA };
static void Port_postprocessing_areva(Port *self) { POST_PROCESSING_AREVA };
static void Port_postprocessing_revai(Port *self) { POST_PROCESSING_REVAI };
static void Port_postprocessing_revaa(Port *self) { POST_PROCESSING_REVAA };
static void Port_postprocessing_revareva(Port *self) { POST_PROCESSING_REVAREVA };

static void
Port_setProcMode(Port *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Port_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = Port_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = Port_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = Port_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Port_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Port_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Port_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Port_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Port_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Port_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Port_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Port_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Port_postprocessing_revareva;
            break;
    }
}

static void
Port_compute_next_data_frame(Port *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Port_traverse(Port *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->risetime);
    Py_VISIT(self->risetime_stream);
    Py_VISIT(self->falltime);
    Py_VISIT(self->falltime_stream);
    return 0;
}

static int
Port_clear(Port *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->risetime);
    Py_CLEAR(self->risetime_stream);
    Py_CLEAR(self->falltime);
    Py_CLEAR(self->falltime_stream);
    return 0;
}

static void
Port_dealloc(Port* self)
{
    pyo_DEALLOC
    Port_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Port_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT inittmp = 0.0;
    PyObject *inputtmp, *input_streamtmp, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    Port *self;
    self = (Port *)type->tp_alloc(type, 0);

    self->risetime = PyFloat_FromDouble(0.05);
    self->falltime = PyFloat_FromDouble(0.05);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->y1 = 0.0;
    self->x1 = 0.0;
    self->dir = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Port_compute_next_data_frame);
    self->mode_func_ptr = Port_setProcMode;

    static char *kwlist[] = {"input", "risetime", "falltime", "init", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFOO, kwlist, &inputtmp, &risetimetmp, &falltimetmp, &inittmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (risetimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRiseTime", "O", risetimetmp);
    }

    if (falltimetmp) {
        PyObject_CallMethod((PyObject *)self, "setFallTime", "O", falltimetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    if (inittmp != 0.0)
        self->x1 = self->y1 = inittmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Port_getServer(Port* self) { GET_SERVER };
static PyObject * Port_getStream(Port* self) { GET_STREAM };
static PyObject * Port_setMul(Port *self, PyObject *arg) { SET_MUL };
static PyObject * Port_setAdd(Port *self, PyObject *arg) { SET_ADD };
static PyObject * Port_setSub(Port *self, PyObject *arg) { SET_SUB };
static PyObject * Port_setDiv(Port *self, PyObject *arg) { SET_DIV };

static PyObject * Port_play(Port *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Port_out(Port *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Port_stop(Port *self) { STOP };

static PyObject * Port_multiply(Port *self, PyObject *arg) { MULTIPLY };
static PyObject * Port_inplace_multiply(Port *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Port_add(Port *self, PyObject *arg) { ADD };
static PyObject * Port_inplace_add(Port *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Port_sub(Port *self, PyObject *arg) { SUB };
static PyObject * Port_inplace_sub(Port *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Port_div(Port *self, PyObject *arg) { DIV };
static PyObject * Port_inplace_div(Port *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Port_setRiseTime(Port *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->risetime);
	if (isNumber == 1) {
		self->risetime = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->risetime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->risetime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->risetime_stream);
        self->risetime_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Port_setFallTime(Port *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->falltime);
	if (isNumber == 1) {
		self->falltime = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->falltime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->falltime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->falltime_stream);
        self->falltime_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Port_members[] = {
{"server", T_OBJECT_EX, offsetof(Port, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Port, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Port, input), 0, "Input sound object."},
{"risetime", T_OBJECT_EX, offsetof(Port, risetime), 0, "Rising portamento time in seconds."},
{"falltime", T_OBJECT_EX, offsetof(Port, falltime), 0, "Falling portamento time in seconds."},
{"mul", T_OBJECT_EX, offsetof(Port, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Port, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Port_methods[] = {
{"getServer", (PyCFunction)Port_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Port_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Port_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Port_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Port_stop, METH_NOARGS, "Stops computing."},
{"setRiseTime", (PyCFunction)Port_setRiseTime, METH_O, "Sets rising portamento time in seconds."},
{"setFallTime", (PyCFunction)Port_setFallTime, METH_O, "Sets falling portamento time in seconds."},
{"setMul", (PyCFunction)Port_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Port_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Port_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Port_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Port_as_number = {
(binaryfunc)Port_add,                         /*nb_add*/
(binaryfunc)Port_sub,                         /*nb_subtract*/
(binaryfunc)Port_multiply,                    /*nb_multiply*/
(binaryfunc)Port_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Port_inplace_add,                 /*inplace_add*/
(binaryfunc)Port_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Port_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Port_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject PortType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Port_base",                                   /*tp_name*/
sizeof(Port),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Port_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Port_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Port objects. Generates a portamento filter.",           /* tp_doc */
(traverseproc)Port_traverse,                  /* tp_traverse */
(inquiry)Port_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Port_methods,                                 /* tp_methods */
Port_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Port_new,                                     /* tp_new */
};

/************/
/* Tone */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
    MYFLT lastFreq;
    MYFLT nyquist;
    // sample memories
    MYFLT y1;
    // variables
    MYFLT c1;
    MYFLT c2;
} Tone;

static void
Tone_filters_i(Tone *self) {
    MYFLT val, b;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);

    if (fr != self->lastFreq) {
        if (fr <= 1.0)
            fr = 1.0;
        else if (fr >= self->nyquist)
            fr = self->nyquist;
        self->lastFreq = fr;
        b = 2.0 - MYCOS(TWOPI * fr / self->sr);
        self->c2 = (b - MYSQRT(b * b - 1.0));
        self->c1 = 1.0 - self->c2;
    }

    for (i=0; i<self->bufsize; i++) {
        val = self->c1 * in[i] + self->c2 * self->y1;
        self->data[i] = val;
        self->y1 = val;
    }
}

static void
Tone_filters_a(Tone *self) {
    MYFLT val, freq, b;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq != self->lastFreq) {
            if (freq <= 1.0)
                freq = 1.0;
            else if (freq >= self->nyquist)
                freq = self->nyquist;
            self->lastFreq = freq;
            b = 2.0 - MYCOS(TWOPI * freq / self->sr);
            self->c2 = (b - MYSQRT(b * b - 1.0));
            self->c1 = 1.0 - self->c2;
        }
        val = self->c1 * in[i] + self->c2 * self->y1;
        self->data[i] = val;
        self->y1 = val;
    }
}

static void Tone_postprocessing_ii(Tone *self) { POST_PROCESSING_II };
static void Tone_postprocessing_ai(Tone *self) { POST_PROCESSING_AI };
static void Tone_postprocessing_ia(Tone *self) { POST_PROCESSING_IA };
static void Tone_postprocessing_aa(Tone *self) { POST_PROCESSING_AA };
static void Tone_postprocessing_ireva(Tone *self) { POST_PROCESSING_IREVA };
static void Tone_postprocessing_areva(Tone *self) { POST_PROCESSING_AREVA };
static void Tone_postprocessing_revai(Tone *self) { POST_PROCESSING_REVAI };
static void Tone_postprocessing_revaa(Tone *self) { POST_PROCESSING_REVAA };
static void Tone_postprocessing_revareva(Tone *self) { POST_PROCESSING_REVAREVA };

static void
Tone_setProcMode(Tone *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Tone_filters_i;
            break;
        case 1:
            self->proc_func_ptr = Tone_filters_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Tone_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Tone_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Tone_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Tone_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Tone_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Tone_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Tone_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Tone_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Tone_postprocessing_revareva;
            break;
    }
}

static void
Tone_compute_next_data_frame(Tone *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Tone_traverse(Tone *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    return 0;
}

static int
Tone_clear(Tone *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    return 0;
}

static void
Tone_dealloc(Tone* self)
{
    pyo_DEALLOC
    Tone_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Tone_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Tone *self;
    self = (Tone *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->lastFreq = -1.0;
    self->y1 = self->c1 = self->c2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;

    Stream_setFunctionPtr(self->stream, Tone_compute_next_data_frame);
    self->mode_func_ptr = Tone_setProcMode;

    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Tone_getServer(Tone* self) { GET_SERVER };
static PyObject * Tone_getStream(Tone* self) { GET_STREAM };
static PyObject * Tone_setMul(Tone *self, PyObject *arg) { SET_MUL };
static PyObject * Tone_setAdd(Tone *self, PyObject *arg) { SET_ADD };
static PyObject * Tone_setSub(Tone *self, PyObject *arg) { SET_SUB };
static PyObject * Tone_setDiv(Tone *self, PyObject *arg) { SET_DIV };

static PyObject * Tone_play(Tone *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Tone_out(Tone *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Tone_stop(Tone *self) { STOP };

static PyObject * Tone_multiply(Tone *self, PyObject *arg) { MULTIPLY };
static PyObject * Tone_inplace_multiply(Tone *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Tone_add(Tone *self, PyObject *arg) { ADD };
static PyObject * Tone_inplace_add(Tone *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Tone_sub(Tone *self, PyObject *arg) { SUB };
static PyObject * Tone_inplace_sub(Tone *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Tone_div(Tone *self, PyObject *arg) { DIV };
static PyObject * Tone_inplace_div(Tone *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Tone_setFreq(Tone *self, PyObject *arg)
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

static PyMemberDef Tone_members[] = {
{"server", T_OBJECT_EX, offsetof(Tone, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Tone, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Tone, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Tone, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(Tone, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Tone, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Tone_methods[] = {
{"getServer", (PyCFunction)Tone_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Tone_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Tone_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Tone_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Tone_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Tone_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)Tone_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Tone_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Tone_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Tone_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Tone_as_number = {
(binaryfunc)Tone_add,                         /*nb_add*/
(binaryfunc)Tone_sub,                         /*nb_subtract*/
(binaryfunc)Tone_multiply,                    /*nb_multiply*/
(binaryfunc)Tone_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Tone_inplace_add,                 /*inplace_add*/
(binaryfunc)Tone_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Tone_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Tone_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ToneType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Tone_base",                                   /*tp_name*/
sizeof(Tone),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Tone_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Tone_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Tone objects. One-pole recursive lowpass filter.",           /* tp_doc */
(traverseproc)Tone_traverse,                  /* tp_traverse */
(inquiry)Tone_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Tone_methods,                                 /* tp_methods */
Tone_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Tone_new,                                     /* tp_new */
};

/************/
/* Atone */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
    MYFLT lastFreq;
    MYFLT nyquist;
    // sample memories
    MYFLT y1;
    // variables
    MYFLT c1;
    MYFLT c2;
} Atone;

static void
Atone_filters_i(Atone *self) {
    MYFLT val, b;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);

    if (fr != self->lastFreq) {
        if (fr <= 1.0)
            fr = 1.0;
        else if (fr >= self->nyquist)
            fr = self->nyquist;
        self->lastFreq = fr;
        b = 2.0 - MYCOS(TWOPI * fr / self->sr);
        self->c2 = (b - MYSQRT(b * b - 1.0));
        self->c1 = 1.0 - self->c2;
    }

    for (i=0; i<self->bufsize; i++) {
        self->y1 = val = self->c1 * in[i] + self->c2 * self->y1;
        self->data[i] = in[i] - val;
    }
}

static void
Atone_filters_a(Atone *self) {
    MYFLT val, freq, b;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq != self->lastFreq) {
            if (freq <= 1.0)
                freq = 1.0;
            else if (freq >= self->nyquist)
                freq = self->nyquist;
            self->lastFreq = freq;
            b = 2.0 - MYCOS(TWOPI * freq / self->sr);
            self->c2 = (b - MYSQRT(b * b - 1.0));
            self->c1 = 1.0 - self->c2;
        }
        self->y1 = val = self->c1 * in[i] + self->c2 * self->y1;
        self->data[i] = in[i] - val;
    }
}

static void Atone_postprocessing_ii(Atone *self) { POST_PROCESSING_II };
static void Atone_postprocessing_ai(Atone *self) { POST_PROCESSING_AI };
static void Atone_postprocessing_ia(Atone *self) { POST_PROCESSING_IA };
static void Atone_postprocessing_aa(Atone *self) { POST_PROCESSING_AA };
static void Atone_postprocessing_ireva(Atone *self) { POST_PROCESSING_IREVA };
static void Atone_postprocessing_areva(Atone *self) { POST_PROCESSING_AREVA };
static void Atone_postprocessing_revai(Atone *self) { POST_PROCESSING_REVAI };
static void Atone_postprocessing_revaa(Atone *self) { POST_PROCESSING_REVAA };
static void Atone_postprocessing_revareva(Atone *self) { POST_PROCESSING_REVAREVA };

static void
Atone_setProcMode(Atone *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Atone_filters_i;
            break;
        case 1:
            self->proc_func_ptr = Atone_filters_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Atone_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Atone_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Atone_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Atone_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Atone_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Atone_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Atone_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Atone_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Atone_postprocessing_revareva;
            break;
    }
}

static void
Atone_compute_next_data_frame(Atone *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Atone_traverse(Atone *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    return 0;
}

static int
Atone_clear(Atone *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    return 0;
}

static void
Atone_dealloc(Atone* self)
{
    pyo_DEALLOC
    Atone_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Atone_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Atone *self;
    self = (Atone *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->lastFreq = -1.0;
    self->y1 = self->c1 = self->c2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;

    Stream_setFunctionPtr(self->stream, Atone_compute_next_data_frame);
    self->mode_func_ptr = Atone_setProcMode;

    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Atone_getServer(Atone* self) { GET_SERVER };
static PyObject * Atone_getStream(Atone* self) { GET_STREAM };
static PyObject * Atone_setMul(Atone *self, PyObject *arg) { SET_MUL };
static PyObject * Atone_setAdd(Atone *self, PyObject *arg) { SET_ADD };
static PyObject * Atone_setSub(Atone *self, PyObject *arg) { SET_SUB };
static PyObject * Atone_setDiv(Atone *self, PyObject *arg) { SET_DIV };

static PyObject * Atone_play(Atone *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Atone_out(Atone *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Atone_stop(Atone *self) { STOP };

static PyObject * Atone_multiply(Atone *self, PyObject *arg) { MULTIPLY };
static PyObject * Atone_inplace_multiply(Atone *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Atone_add(Atone *self, PyObject *arg) { ADD };
static PyObject * Atone_inplace_add(Atone *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Atone_sub(Atone *self, PyObject *arg) { SUB };
static PyObject * Atone_inplace_sub(Atone *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Atone_div(Atone *self, PyObject *arg) { DIV };
static PyObject * Atone_inplace_div(Atone *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Atone_setFreq(Atone *self, PyObject *arg)
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

static PyMemberDef Atone_members[] = {
{"server", T_OBJECT_EX, offsetof(Atone, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Atone, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Atone, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Atone, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(Atone, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Atone, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Atone_methods[] = {
{"getServer", (PyCFunction)Atone_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Atone_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Atone_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Atone_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Atone_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Atone_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)Atone_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Atone_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Atone_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Atone_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Atone_as_number = {
(binaryfunc)Atone_add,                         /*nb_add*/
(binaryfunc)Atone_sub,                         /*nb_subtract*/
(binaryfunc)Atone_multiply,                    /*nb_multiply*/
(binaryfunc)Atone_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Atone_inplace_add,                 /*inplace_add*/
(binaryfunc)Atone_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Atone_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Atone_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject AtoneType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Atone_base",                                   /*tp_name*/
sizeof(Atone),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Atone_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Atone_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Atone objects. One-pole recursive lowpass filter.",           /* tp_doc */
(traverseproc)Atone_traverse,                  /* tp_traverse */
(inquiry)Atone_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Atone_methods,                                 /* tp_methods */
Atone_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Atone_new,                                     /* tp_new */
};

/************/
/* DCBlock */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
    // sample memories
    MYFLT x1;
    MYFLT y1;
} DCBlock;

static void
DCBlock_filters(DCBlock *self) {
    MYFLT x, y;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        x = in[i];
        y = x - self->x1 + 0.995 * self->y1;
        self->x1 = x;
        self->data[i] = self->y1 = y;
    }
}

static void DCBlock_postprocessing_ii(DCBlock *self) { POST_PROCESSING_II };
static void DCBlock_postprocessing_ai(DCBlock *self) { POST_PROCESSING_AI };
static void DCBlock_postprocessing_ia(DCBlock *self) { POST_PROCESSING_IA };
static void DCBlock_postprocessing_aa(DCBlock *self) { POST_PROCESSING_AA };
static void DCBlock_postprocessing_ireva(DCBlock *self) { POST_PROCESSING_IREVA };
static void DCBlock_postprocessing_areva(DCBlock *self) { POST_PROCESSING_AREVA };
static void DCBlock_postprocessing_revai(DCBlock *self) { POST_PROCESSING_REVAI };
static void DCBlock_postprocessing_revaa(DCBlock *self) { POST_PROCESSING_REVAA };
static void DCBlock_postprocessing_revareva(DCBlock *self) { POST_PROCESSING_REVAREVA };

static void
DCBlock_setProcMode(DCBlock *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = DCBlock_filters;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = DCBlock_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = DCBlock_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = DCBlock_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = DCBlock_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = DCBlock_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = DCBlock_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = DCBlock_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = DCBlock_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = DCBlock_postprocessing_revareva;
            break;
    }
}

static void
DCBlock_compute_next_data_frame(DCBlock *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
DCBlock_traverse(DCBlock *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
DCBlock_clear(DCBlock *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
DCBlock_dealloc(DCBlock* self)
{
    pyo_DEALLOC
    DCBlock_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
DCBlock_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    DCBlock *self;
    self = (DCBlock *)type->tp_alloc(type, 0);

    self->x1 = self->y1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, DCBlock_compute_next_data_frame);
    self->mode_func_ptr = DCBlock_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * DCBlock_getServer(DCBlock* self) { GET_SERVER };
static PyObject * DCBlock_getStream(DCBlock* self) { GET_STREAM };
static PyObject * DCBlock_setMul(DCBlock *self, PyObject *arg) { SET_MUL };
static PyObject * DCBlock_setAdd(DCBlock *self, PyObject *arg) { SET_ADD };
static PyObject * DCBlock_setSub(DCBlock *self, PyObject *arg) { SET_SUB };
static PyObject * DCBlock_setDiv(DCBlock *self, PyObject *arg) { SET_DIV };

static PyObject * DCBlock_play(DCBlock *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * DCBlock_out(DCBlock *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * DCBlock_stop(DCBlock *self) { STOP };

static PyObject * DCBlock_multiply(DCBlock *self, PyObject *arg) { MULTIPLY };
static PyObject * DCBlock_inplace_multiply(DCBlock *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * DCBlock_add(DCBlock *self, PyObject *arg) { ADD };
static PyObject * DCBlock_inplace_add(DCBlock *self, PyObject *arg) { INPLACE_ADD };
static PyObject * DCBlock_sub(DCBlock *self, PyObject *arg) { SUB };
static PyObject * DCBlock_inplace_sub(DCBlock *self, PyObject *arg) { INPLACE_SUB };
static PyObject * DCBlock_div(DCBlock *self, PyObject *arg) { DIV };
static PyObject * DCBlock_inplace_div(DCBlock *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef DCBlock_members[] = {
{"server", T_OBJECT_EX, offsetof(DCBlock, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(DCBlock, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(DCBlock, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(DCBlock, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(DCBlock, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef DCBlock_methods[] = {
{"getServer", (PyCFunction)DCBlock_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)DCBlock_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)DCBlock_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)DCBlock_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)DCBlock_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)DCBlock_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)DCBlock_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)DCBlock_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)DCBlock_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods DCBlock_as_number = {
(binaryfunc)DCBlock_add,                         /*nb_add*/
(binaryfunc)DCBlock_sub,                         /*nb_subtract*/
(binaryfunc)DCBlock_multiply,                    /*nb_multiply*/
(binaryfunc)DCBlock_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)DCBlock_inplace_add,                 /*inplace_add*/
(binaryfunc)DCBlock_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)DCBlock_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)DCBlock_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject DCBlockType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.DCBlock_base",                                   /*tp_name*/
sizeof(DCBlock),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)DCBlock_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&DCBlock_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"DCBlock objects. Implements the DC blocking filter.",           /* tp_doc */
(traverseproc)DCBlock_traverse,                  /* tp_traverse */
(inquiry)DCBlock_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
DCBlock_methods,                                 /* tp_methods */
DCBlock_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
DCBlock_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *delay;
    Stream *delay_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    MYFLT maxDelay;
    long size;
    int in_count;
    int modebuffer[4];
    MYFLT *buffer; // samples memory
} Allpass;

static void
Allpass_process_ii(Allpass *self) {
    MYFLT val, xind, frac;
    int i, ind;

    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    if (del < 0.)
        del = 0.;
    else if (del > self->maxDelay)
        del = self->maxDelay;
    MYFLT sampdel = del * self->sr;

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val * (1.0 - (feed * feed)) + in[i] * -feed;

        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Allpass_process_ai(Allpass *self) {
    MYFLT val, xind, frac, sampdel, del;
    int i, ind;

    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxDelay)
            del = self->maxDelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val * (1.0 - (feed * feed)) + in[i] * -feed;

        self->buffer[self->in_count] = in[i]  + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Allpass_process_ia(Allpass *self) {
    MYFLT val, xind, frac, feed;
    int i, ind;

    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT *fdb = Stream_getData((Stream *)self->feedback_stream);

    if (del < 0.)
        del = 0.;
    else if (del > self->maxDelay)
        del = self->maxDelay;
    MYFLT sampdel = del * self->sr;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val * (1.0 - (feed * feed)) + in[i] * -feed;

        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Allpass_process_aa(Allpass *self) {
    MYFLT val, xind, frac, sampdel, feed, del;
    int i, ind;

    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);
    MYFLT *fdb = Stream_getData((Stream *)self->feedback_stream);

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        if (del < 0.)
            del = 0.;
        else if (del > self->maxDelay)
            del = self->maxDelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val * (1.0 - (feed * feed)) + in[i] * -feed;

        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Allpass_postprocessing_ii(Allpass *self) { POST_PROCESSING_II };
static void Allpass_postprocessing_ai(Allpass *self) { POST_PROCESSING_AI };
static void Allpass_postprocessing_ia(Allpass *self) { POST_PROCESSING_IA };
static void Allpass_postprocessing_aa(Allpass *self) { POST_PROCESSING_AA };
static void Allpass_postprocessing_ireva(Allpass *self) { POST_PROCESSING_IREVA };
static void Allpass_postprocessing_areva(Allpass *self) { POST_PROCESSING_AREVA };
static void Allpass_postprocessing_revai(Allpass *self) { POST_PROCESSING_REVAI };
static void Allpass_postprocessing_revaa(Allpass *self) { POST_PROCESSING_REVAA };
static void Allpass_postprocessing_revareva(Allpass *self) { POST_PROCESSING_REVAREVA };

static void
Allpass_setProcMode(Allpass *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Allpass_process_ii;
            break;
        case 1:
            self->proc_func_ptr = Allpass_process_ai;
            break;
        case 10:
            self->proc_func_ptr = Allpass_process_ia;
            break;
        case 11:
            self->proc_func_ptr = Allpass_process_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Allpass_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Allpass_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Allpass_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Allpass_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Allpass_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Allpass_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Allpass_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Allpass_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Allpass_postprocessing_revareva;
            break;
    }
}

static void
Allpass_compute_next_data_frame(Allpass *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Allpass_traverse(Allpass *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->delay);
    Py_VISIT(self->delay_stream);
    Py_VISIT(self->feedback);
    Py_VISIT(self->feedback_stream);
    return 0;
}

static int
Allpass_clear(Allpass *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->delay);
    Py_CLEAR(self->delay_stream);
    Py_CLEAR(self->feedback);
    Py_CLEAR(self->feedback_stream);
    return 0;
}

static void
Allpass_dealloc(Allpass* self)
{
    pyo_DEALLOC
    free(self->buffer);
    Allpass_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Allpass_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    Allpass *self;
    self = (Allpass *)type->tp_alloc(type, 0);

    self->delay = PyFloat_FromDouble(0);
    self->feedback = PyFloat_FromDouble(0);
    self->maxDelay = 1;
    self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Allpass_compute_next_data_frame);
    self->mode_func_ptr = Allpass_setProcMode;

    static char *kwlist[] = {"input", "delay", "feedback", "maxDelay", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFOO, kwlist, &inputtmp, &delaytmp, &feedbacktmp, &self->maxDelay, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (delaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDelay", "O", delaytmp);
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

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = self->maxDelay * self->sr + 0.5;

    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Allpass_getServer(Allpass* self) { GET_SERVER };
static PyObject * Allpass_getStream(Allpass* self) { GET_STREAM };
static PyObject * Allpass_setMul(Allpass *self, PyObject *arg) { SET_MUL };
static PyObject * Allpass_setAdd(Allpass *self, PyObject *arg) { SET_ADD };
static PyObject * Allpass_setSub(Allpass *self, PyObject *arg) { SET_SUB };
static PyObject * Allpass_setDiv(Allpass *self, PyObject *arg) { SET_DIV };

static PyObject * Allpass_play(Allpass *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Allpass_out(Allpass *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Allpass_stop(Allpass *self) { STOP };

static PyObject * Allpass_multiply(Allpass *self, PyObject *arg) { MULTIPLY };
static PyObject * Allpass_inplace_multiply(Allpass *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Allpass_add(Allpass *self, PyObject *arg) { ADD };
static PyObject * Allpass_inplace_add(Allpass *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Allpass_sub(Allpass *self, PyObject *arg) { SUB };
static PyObject * Allpass_inplace_sub(Allpass *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Allpass_div(Allpass *self, PyObject *arg) { DIV };
static PyObject * Allpass_inplace_div(Allpass *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Allpass_setDelay(Allpass *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->delay);
	if (isNumber == 1) {
		self->delay = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->delay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->delay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->delay_stream);
        self->delay_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Allpass_setFeedback(Allpass *self, PyObject *arg)
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

static PyMemberDef Allpass_members[] = {
    {"server", T_OBJECT_EX, offsetof(Allpass, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Allpass, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Allpass, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(Allpass, delay), 0, "delay time in seconds."},
    {"feedback", T_OBJECT_EX, offsetof(Allpass, feedback), 0, "Feedback value."},
    {"mul", T_OBJECT_EX, offsetof(Allpass, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Allpass, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Allpass_methods[] = {
    {"getServer", (PyCFunction)Allpass_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Allpass_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Allpass_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Allpass_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Allpass_stop, METH_NOARGS, "Stops computing."},
	{"setDelay", (PyCFunction)Allpass_setDelay, METH_O, "Sets delay time in seconds."},
    {"setFeedback", (PyCFunction)Allpass_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
	{"setMul", (PyCFunction)Allpass_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Allpass_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Allpass_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Allpass_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Allpass_as_number = {
    (binaryfunc)Allpass_add,                      /*nb_add*/
    (binaryfunc)Allpass_sub,                 /*nb_subtract*/
    (binaryfunc)Allpass_multiply,                 /*nb_multiply*/
    (binaryfunc)Allpass_div,                   /*nb_divide*/
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
    (binaryfunc)Allpass_inplace_add,              /*inplace_add*/
    (binaryfunc)Allpass_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Allpass_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Allpass_inplace_div,           /*inplace_divide*/
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

PyTypeObject AllpassType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Allpass_base",         /*tp_name*/
    sizeof(Allpass),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Allpass_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Allpass_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Allpass objects. Allpass signal by x samples.",           /* tp_doc */
    (traverseproc)Allpass_traverse,   /* tp_traverse */
    (inquiry)Allpass_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Allpass_methods,             /* tp_methods */
    Allpass_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Allpass_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *bw;
    Stream *bw_stream;
    int init;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT oneOnSr;
    MYFLT nyquist;
    // sample memories
    MYFLT y1;
    MYFLT y2;
    // coefficients
    MYFLT alpha;
    MYFLT beta;
} Allpass2;

static void
Allpass2_compute_variables(Allpass2 *self, MYFLT freq, MYFLT bw)
{
    MYFLT radius, angle;
    if (freq <= 1)
        freq = 1;
    else if (freq >= self->nyquist)
        freq = self->nyquist;

    radius = MYPOW(E, -PI * bw * self->oneOnSr);
    angle = TWOPI * freq * self->oneOnSr;

    self->alpha = radius * radius;
    self->beta = -2.0 * radius * MYCOS(angle);
}

static void
Allpass2_filters_ii(Allpass2 *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    for (i=0; i<self->bufsize; i++) {
        val = in[i] + (self->y1 * -self->beta) + (self->y2 * -self->alpha);
        self->data[i] = (val * self->alpha) + (self->y1 * self->beta) + self->y2;
        self->y2 = self->y1;
        self->y1 = val;
    }
}

static void
Allpass2_filters_ai(Allpass2 *self) {
    MYFLT val, bw;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    bw = PyFloat_AS_DOUBLE(self->bw);

    for (i=0; i<self->bufsize; i++) {
        Allpass2_compute_variables(self, fr[i], bw);
        val = in[i] + (self->y1 * -self->beta) + (self->y2 * -self->alpha);
        self->data[i] = (val * self->alpha) + (self->y1 * self->beta) + self->y2;
        self->y2 = self->y1;
        self->y1 = val;
    }
}

static void
Allpass2_filters_ia(Allpass2 *self) {
    MYFLT val, fr;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *bw = Stream_getData((Stream *)self->bw_stream);

    for (i=0; i<self->bufsize; i++) {
        Allpass2_compute_variables(self, fr, bw[i]);
        val = in[i] + (self->y1 * -self->beta) + (self->y2 * -self->alpha);
        self->data[i] = (val * self->alpha) + (self->y1 * self->beta) + self->y2;
        self->y2 = self->y1;
        self->y1 = val;
    }
}

static void
Allpass2_filters_aa(Allpass2 *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *bw = Stream_getData((Stream *)self->bw_stream);

    for (i=0; i<self->bufsize; i++) {
        Allpass2_compute_variables(self, fr[i], bw[i]);
        val = in[i] + (self->y1 * -self->beta) + (self->y2 * -self->alpha);
        self->data[i] = (val * self->alpha) + (self->y1 * self->beta) + self->y2;
        self->y2 = self->y1;
        self->y1 = val;
    }
}

static void Allpass2_postprocessing_ii(Allpass2 *self) { POST_PROCESSING_II };
static void Allpass2_postprocessing_ai(Allpass2 *self) { POST_PROCESSING_AI };
static void Allpass2_postprocessing_ia(Allpass2 *self) { POST_PROCESSING_IA };
static void Allpass2_postprocessing_aa(Allpass2 *self) { POST_PROCESSING_AA };
static void Allpass2_postprocessing_ireva(Allpass2 *self) { POST_PROCESSING_IREVA };
static void Allpass2_postprocessing_areva(Allpass2 *self) { POST_PROCESSING_AREVA };
static void Allpass2_postprocessing_revai(Allpass2 *self) { POST_PROCESSING_REVAI };
static void Allpass2_postprocessing_revaa(Allpass2 *self) { POST_PROCESSING_REVAA };
static void Allpass2_postprocessing_revareva(Allpass2 *self) { POST_PROCESSING_REVAREVA };

static void
Allpass2_setProcMode(Allpass2 *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            Allpass2_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->bw));
            self->proc_func_ptr = Allpass2_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = Allpass2_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = Allpass2_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = Allpass2_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Allpass2_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Allpass2_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Allpass2_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Allpass2_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Allpass2_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Allpass2_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Allpass2_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Allpass2_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Allpass2_postprocessing_revareva;
            break;
    }
}

static void
Allpass2_compute_next_data_frame(Allpass2 *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Allpass2_traverse(Allpass2 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->bw);
    Py_VISIT(self->bw_stream);
    return 0;
}

static int
Allpass2_clear(Allpass2 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->bw);
    Py_CLEAR(self->bw_stream);
    return 0;
}

static void
Allpass2_dealloc(Allpass2* self)
{
    pyo_DEALLOC
    Allpass2_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Allpass2_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *bwtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Allpass2 *self;
    self = (Allpass2 *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->bw = PyFloat_FromDouble(100);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->init = 1;

    INIT_OBJECT_COMMON

    self->oneOnSr = 1.0 / self->sr;
    self->nyquist = (MYFLT)self->sr * 0.49;

    Stream_setFunctionPtr(self->stream, Allpass2_compute_next_data_frame);
    self->mode_func_ptr = Allpass2_setProcMode;

    static char *kwlist[] = {"input", "freq", "bw", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &freqtmp, &bwtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (bwtmp) {
        PyObject_CallMethod((PyObject *)self, "setBw", "O", bwtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Allpass2_getServer(Allpass2* self) { GET_SERVER };
static PyObject * Allpass2_getStream(Allpass2* self) { GET_STREAM };
static PyObject * Allpass2_setMul(Allpass2 *self, PyObject *arg) { SET_MUL };
static PyObject * Allpass2_setAdd(Allpass2 *self, PyObject *arg) { SET_ADD };
static PyObject * Allpass2_setSub(Allpass2 *self, PyObject *arg) { SET_SUB };
static PyObject * Allpass2_setDiv(Allpass2 *self, PyObject *arg) { SET_DIV };

static PyObject * Allpass2_play(Allpass2 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Allpass2_out(Allpass2 *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Allpass2_stop(Allpass2 *self) { STOP };

static PyObject * Allpass2_multiply(Allpass2 *self, PyObject *arg) { MULTIPLY };
static PyObject * Allpass2_inplace_multiply(Allpass2 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Allpass2_add(Allpass2 *self, PyObject *arg) { ADD };
static PyObject * Allpass2_inplace_add(Allpass2 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Allpass2_sub(Allpass2 *self, PyObject *arg) { SUB };
static PyObject * Allpass2_inplace_sub(Allpass2 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Allpass2_div(Allpass2 *self, PyObject *arg) { DIV };
static PyObject * Allpass2_inplace_div(Allpass2 *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Allpass2_setFreq(Allpass2 *self, PyObject *arg)
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
Allpass2_setBw(Allpass2 *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->bw);
	if (isNumber == 1) {
		self->bw = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->bw = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->bw, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->bw_stream);
        self->bw_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Allpass2_members[] = {
{"server", T_OBJECT_EX, offsetof(Allpass2, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Allpass2, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Allpass2, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Allpass2, freq), 0, "Cutoff frequency in cycle per second."},
{"bw", T_OBJECT_EX, offsetof(Allpass2, bw), 0, "Bandwidth."},
{"mul", T_OBJECT_EX, offsetof(Allpass2, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Allpass2, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Allpass2_methods[] = {
{"getServer", (PyCFunction)Allpass2_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Allpass2_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Allpass2_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Allpass2_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Allpass2_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Allpass2_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setBw", (PyCFunction)Allpass2_setBw, METH_O, "Sets filter bandwidth."},
{"setMul", (PyCFunction)Allpass2_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Allpass2_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Allpass2_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Allpass2_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Allpass2_as_number = {
(binaryfunc)Allpass2_add,                         /*nb_add*/
(binaryfunc)Allpass2_sub,                         /*nb_subtract*/
(binaryfunc)Allpass2_multiply,                    /*nb_multiply*/
(binaryfunc)Allpass2_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Allpass2_inplace_add,                 /*inplace_add*/
(binaryfunc)Allpass2_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Allpass2_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Allpass2_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject Allpass2Type = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Allpass2_base",                                   /*tp_name*/
sizeof(Allpass2),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Allpass2_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Allpass2_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Allpass2 objects. Second order allpass filter.",           /* tp_doc */
(traverseproc)Allpass2_traverse,                  /* tp_traverse */
(inquiry)Allpass2_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Allpass2_methods,                                 /* tp_methods */
Allpass2_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Allpass2_new,                                     /* tp_new */
};

/*******************/
/***** Phaser ******/
/*******************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *spread;
    Stream *spread_stream;
    PyObject *q;
    Stream *q_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    int stages;
    int modebuffer[6]; // need at least 2 slots for mul & add
    MYFLT halfSr;
    MYFLT minusPiOnSr;
    MYFLT twoPiOnSr;
    MYFLT norm_arr_pos;
    MYFLT tmp;
    // sample memories
    MYFLT *y1;
    MYFLT *y2;
    // coefficients
    MYFLT *alpha;
    MYFLT *beta;
} Phaser;

static MYFLT
Phaser_clip(MYFLT x) {
    if (x < -1.0)
        return -1.0;
    else if (x > 1.0)
        return 1.0;
    else
        return x;
}

static void
Phaser_compute_variables(Phaser *self, MYFLT freq, MYFLT spread, MYFLT q)
{
    int i, ipart;
    MYFLT radius, angle, fr, qfactor, pos, fpart;

    qfactor = 1.0 / q * self->minusPiOnSr;
    fr = freq;
    for (i=0; i<self->stages; i++) {
        if (fr <= 20)
            fr = 20;
        else if (fr >= self->halfSr)
            fr = self->halfSr;

        radius = MYPOW(E, fr * qfactor);
        angle = fr * self->twoPiOnSr;

        self->alpha[i] = radius * radius;

        pos = angle * self->norm_arr_pos;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->beta[i] = -2.0 * radius * (HALF_COS_ARRAY[i] * (1.0 - fpart) + HALF_COS_ARRAY[i+1] * fpart);
        fr *= spread;
    }
}

static void
Phaser_filters_iii(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_aii(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT spread = PyFloat_AS_DOUBLE(self->spread);
    MYFLT q = PyFloat_AS_DOUBLE(self->q);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread, q);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread, q);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_iai(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *spread = Stream_getData((Stream *)self->spread_stream);
    MYFLT q = PyFloat_AS_DOUBLE(self->q);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread[i], q);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread[i], q);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_aai(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *spread = Stream_getData((Stream *)self->spread_stream);
    MYFLT q = PyFloat_AS_DOUBLE(self->q);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread[i], q);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread[i], q);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_iia(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT spread = PyFloat_AS_DOUBLE(self->spread);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread, q[i]);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread, q[i]);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_aia(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT spread = PyFloat_AS_DOUBLE(self->spread);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread, q[i]);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread, q[i]);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_iaa(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *spread = Stream_getData((Stream *)self->spread_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread[i], q[i]);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread[i], q[i]);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_aaa(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *spread = Stream_getData((Stream *)self->spread_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread[i], q[i]);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread[i], q[i]);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void Phaser_postprocessing_ii(Phaser *self) { POST_PROCESSING_II };
static void Phaser_postprocessing_ai(Phaser *self) { POST_PROCESSING_AI };
static void Phaser_postprocessing_ia(Phaser *self) { POST_PROCESSING_IA };
static void Phaser_postprocessing_aa(Phaser *self) { POST_PROCESSING_AA };
static void Phaser_postprocessing_ireva(Phaser *self) { POST_PROCESSING_IREVA };
static void Phaser_postprocessing_areva(Phaser *self) { POST_PROCESSING_AREVA };
static void Phaser_postprocessing_revai(Phaser *self) { POST_PROCESSING_REVAI };
static void Phaser_postprocessing_revaa(Phaser *self) { POST_PROCESSING_REVAA };
static void Phaser_postprocessing_revareva(Phaser *self) { POST_PROCESSING_REVAREVA };

static void
Phaser_setProcMode(Phaser *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            Phaser_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->spread), PyFloat_AS_DOUBLE(self->q));
            self->proc_func_ptr = Phaser_filters_iii;
            break;
        case 1:
            self->proc_func_ptr = Phaser_filters_aii;
            break;
        case 10:
            self->proc_func_ptr = Phaser_filters_iai;
            break;
        case 11:
            self->proc_func_ptr = Phaser_filters_aai;
            break;
        case 100:
            self->proc_func_ptr = Phaser_filters_iia;
            break;
        case 101:
            self->proc_func_ptr = Phaser_filters_aia;
            break;
        case 110:
            self->proc_func_ptr = Phaser_filters_iaa;
            break;
        case 111:
            self->proc_func_ptr = Phaser_filters_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Phaser_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Phaser_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Phaser_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Phaser_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Phaser_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Phaser_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Phaser_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Phaser_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Phaser_postprocessing_revareva;
            break;
    }
}

static void
Phaser_compute_next_data_frame(Phaser *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Phaser_traverse(Phaser *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->spread);
    Py_VISIT(self->spread_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    Py_VISIT(self->feedback);
    Py_VISIT(self->feedback_stream);
    return 0;
}

static int
Phaser_clear(Phaser *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->spread);
    Py_CLEAR(self->spread_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    Py_CLEAR(self->feedback);
    Py_CLEAR(self->feedback_stream);
    return 0;
}

static void
Phaser_dealloc(Phaser* self)
{
    pyo_DEALLOC
    free(self->y1);
    free(self->y2);
    free(self->alpha);
    free(self->beta);
    Phaser_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Phaser_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *spreadtmp=NULL, *qtmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    Phaser *self;
    self = (Phaser *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000.0);
    self->spread = PyFloat_FromDouble(1.0);
    self->q = PyFloat_FromDouble(10.0);
    self->feedback = PyFloat_FromDouble(0.0);
    self->tmp = 0.0;
    self->stages = 8;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
	self->modebuffer[5] = 0;

    INIT_OBJECT_COMMON

    self->halfSr = (MYFLT)self->sr * 0.49;
    self->minusPiOnSr = -PI / self->sr;
    self->twoPiOnSr = TWOPI / self->sr;
    self->norm_arr_pos = 1.0 / PI * 512.0;

    Stream_setFunctionPtr(self->stream, Phaser_compute_next_data_frame);
    self->mode_func_ptr = Phaser_setProcMode;

    static char *kwlist[] = {"input", "freq", "spread", "q", "feedback", "num", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOiOO", kwlist, &inputtmp, &freqtmp, &spreadtmp, &qtmp, &feedbacktmp, &self->stages, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    self->y1 = (MYFLT *)realloc(self->y1, self->stages * sizeof(MYFLT));
    self->y2 = (MYFLT *)realloc(self->y2, self->stages * sizeof(MYFLT));
    self->alpha = (MYFLT *)realloc(self->alpha, self->stages * sizeof(MYFLT));
    self->beta = (MYFLT *)realloc(self->beta, self->stages * sizeof(MYFLT));


    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (spreadtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpread", "O", spreadtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
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

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    for (i=0; i<self->stages; i++) {
        self->y1[i] = self->y2[i] = 0.0;
    }

    return (PyObject *)self;
}

static PyObject * Phaser_getServer(Phaser* self) { GET_SERVER };
static PyObject * Phaser_getStream(Phaser* self) { GET_STREAM };
static PyObject * Phaser_setMul(Phaser *self, PyObject *arg) { SET_MUL };
static PyObject * Phaser_setAdd(Phaser *self, PyObject *arg) { SET_ADD };
static PyObject * Phaser_setSub(Phaser *self, PyObject *arg) { SET_SUB };
static PyObject * Phaser_setDiv(Phaser *self, PyObject *arg) { SET_DIV };

static PyObject * Phaser_play(Phaser *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Phaser_out(Phaser *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Phaser_stop(Phaser *self) { STOP };

static PyObject * Phaser_multiply(Phaser *self, PyObject *arg) { MULTIPLY };
static PyObject * Phaser_inplace_multiply(Phaser *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Phaser_add(Phaser *self, PyObject *arg) { ADD };
static PyObject * Phaser_inplace_add(Phaser *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Phaser_sub(Phaser *self, PyObject *arg) { SUB };
static PyObject * Phaser_inplace_sub(Phaser *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Phaser_div(Phaser *self, PyObject *arg) { DIV };
static PyObject * Phaser_inplace_div(Phaser *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Phaser_setFreq(Phaser *self, PyObject *arg)
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
Phaser_setSpread(Phaser *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->spread);
	if (isNumber == 1) {
		self->spread = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->spread = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->spread, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->spread_stream);
        self->spread_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Phaser_setQ(Phaser *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Phaser_setFeedback(Phaser *self, PyObject *arg)
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
        self->modebuffer[5] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[5] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Phaser_members[] = {
    {"server", T_OBJECT_EX, offsetof(Phaser, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Phaser, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Phaser, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Phaser, freq), 0, "Base frequency in Hertz."},
    {"spread", T_OBJECT_EX, offsetof(Phaser, spread), 0, "Frequencies spreading factor."},
    {"q", T_OBJECT_EX, offsetof(Phaser, q), 0, "Q factor."},
    {"feedback", T_OBJECT_EX, offsetof(Phaser, feedback), 0, "Feedback factor."},
    {"mul", T_OBJECT_EX, offsetof(Phaser, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Phaser, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Phaser_methods[] = {
    {"getServer", (PyCFunction)Phaser_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Phaser_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Phaser_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Phaser_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Phaser_stop, METH_NOARGS, "Stops computing."},
    {"setFreq", (PyCFunction)Phaser_setFreq, METH_O, "Sets base frequency in Hertz."},
    {"setSpread", (PyCFunction)Phaser_setSpread, METH_O, "Sets spreading factor."},
    {"setQ", (PyCFunction)Phaser_setQ, METH_O, "Sets filter Q factor."},
    {"setFeedback", (PyCFunction)Phaser_setFeedback, METH_O, "Sets filter Feedback factor."},
    {"setMul", (PyCFunction)Phaser_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Phaser_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Phaser_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Phaser_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Phaser_as_number = {
    (binaryfunc)Phaser_add,                         /*nb_add*/
    (binaryfunc)Phaser_sub,                         /*nb_subtract*/
    (binaryfunc)Phaser_multiply,                    /*nb_multiply*/
    (binaryfunc)Phaser_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)Phaser_inplace_add,                 /*inplace_add*/
    (binaryfunc)Phaser_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Phaser_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Phaser_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject PhaserType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Phaser_base",                                   /*tp_name*/
    sizeof(Phaser),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Phaser_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Phaser_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Phaser objects. Multi-stages second order allpass filters.",           /* tp_doc */
    (traverseproc)Phaser_traverse,                  /* tp_traverse */
    (inquiry)Phaser_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Phaser_methods,                                 /* tp_methods */
    Phaser_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Phaser_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *input2;
    Stream *input2_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *spread;
    Stream *spread_stream;
    PyObject *q;
    Stream *q_stream;
    PyObject *slope;
    Stream *slope_stream;
    MYFLT last_freq;
    MYFLT last_spread;
    MYFLT last_q;
    MYFLT last_slope;
    MYFLT factor;
    int stages;
    int last_stages;
    int flag;
    MYFLT nyquist;
    MYFLT twoPiOnSr;
    int modebuffer[6]; // need at least 2 slots for mul & add
    // sample memories
    MYFLT *y1;
    MYFLT *y2;
    MYFLT *yy1;
    MYFLT *yy2;
    // follower memories
    MYFLT *follow;
    // coefficients
    MYFLT *b0;
    MYFLT *b2;
    MYFLT *a0;
    MYFLT *a1;
    MYFLT *a2;
} Vocoder;

static void
Vocoder_allocate_memories(Vocoder *self)
{
    int i, i2j, j;
    self->y1 = (MYFLT *)realloc(self->y1, self->stages * 2 *  sizeof(MYFLT));
    self->y2 = (MYFLT *)realloc(self->y2, self->stages * 2 *  sizeof(MYFLT));
    self->yy1 = (MYFLT *)realloc(self->yy1, self->stages * 2 *  sizeof(MYFLT));
    self->yy2 = (MYFLT *)realloc(self->yy2, self->stages * 2 *  sizeof(MYFLT));
    self->b0 = (MYFLT *)realloc(self->b0, self->stages *  sizeof(MYFLT));
    self->b2 = (MYFLT *)realloc(self->b2, self->stages *  sizeof(MYFLT));
    self->a0 = (MYFLT *)realloc(self->a0, self->stages *  sizeof(MYFLT));
    self->a1 = (MYFLT *)realloc(self->a1, self->stages *  sizeof(MYFLT));
    self->a2 = (MYFLT *)realloc(self->a2, self->stages *  sizeof(MYFLT));
    self->follow = (MYFLT *)realloc(self->follow, self->stages *  sizeof(MYFLT));
    for (i=0; i<self->stages; i++) {
        self->b0[i] = self->b2[i] = self->a0[i] = self->a1[i] = self->a2[i] = self->follow[i] = 0.0;
        for (j=0; j<2; j++) {
            i2j = i * 2 + j;
            self->yy1[i2j] = self->yy2[i2j] = self->y1[i2j] = self->y2[i2j] = 0.0;
        }
    }
    self->flag = 1;
}

static void
Vocoder_compute_variables(Vocoder *self, MYFLT base, MYFLT spread, MYFLT q)
{
    int i;
    MYFLT w0, c, alpha, freq, invqfac;

    invqfac = 1.0 / (2.0 * q);

    for (i=0; i<self->stages; i++) {
        freq = base * MYPOW(i+1, spread);
        if (freq <= 10)
            freq = 10.0;
        else if (freq >= self->nyquist)
            freq = self->nyquist;

        w0 = self->twoPiOnSr * freq;
        c = MYCOS(w0);
        alpha = MYSIN(w0) * invqfac;
        self->b0[i] = alpha;
        self->b2[i] = -alpha;
        self->a0[i] = 1.0 / (1.0 + alpha); /* Inversed to multiply */
        self->a1[i] = -2.0 * c;
        self->a2[i] = 1.0 - alpha;
    }
}

static void
Vocoder_filters_iii(Vocoder *self) {
    int i, j, j2;
    MYFLT vin, vout, vin2, vout2, w, w2, freq, spread, q, slope, output, amp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    freq = PyFloat_AS_DOUBLE(self->freq);
    spread = PyFloat_AS_DOUBLE(self->spread);
    q = PyFloat_AS_DOUBLE(self->q);
    if (q < 0.1)
        q = 0.1;
    amp = q * 10.0;

    if (self->modebuffer[5] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (slope < 0.0)
        slope = 0.0;
    else if (slope > 1.0)
        slope = 1.0;
    if (slope != self->last_slope) {
        self->last_slope = slope;
        self->factor = MYEXP(-1.0 / (self->sr / ((slope * 48.0) + 2.0)));
    }

    if (freq != self->last_freq || spread != self->last_spread || q != self->last_q || self->stages != self->last_stages || self->flag) {
        self->last_freq = freq;
        self->last_spread = spread;
        self->last_q = q;
        self->last_stages = self->stages;
        self->flag = 0;
        Vocoder_compute_variables(self, freq, spread, q);
    }

    for (i=0; i<self->bufsize; i++) {
        output = 0.0;
        vin = in[i];
        vin2 = in2[i];
        for (j=0; j<self->stages; j++) {
            j2 = j * 2;
            /* Analysis part filter 1 */
            w = ( vin - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) )  * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 1 */
            w2 = ( vin2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            j2++;
            /* Analysis part filter 2 */
            w = ( vout - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) ) * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 2 */
            w2 = ( vout2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            /* Follower */
            if (vout < 0.0)
                vout = -vout;
            self->follow[j] = vout + self->factor * (self->follow[j] - vout);
            output += vout2 * self->follow[j];
        }
        self->data[i] = output * amp;
    }
}

static void
Vocoder_filters_aii(Vocoder *self) {
    int i, j, j2;
    int count = 0, maxcount = self->bufsize / 4;
    MYFLT vin, vout, vin2, vout2, w, w2, spread, q, slope, output, amp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT freq = fr[0];
    spread = PyFloat_AS_DOUBLE(self->spread);
    q = PyFloat_AS_DOUBLE(self->q);
    if (q < 0.1)
        q = 0.1;
    amp = q * 10.0;

    if (self->modebuffer[5] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (slope < 0.0)
        slope = 0.0;
    else if (slope > 1.0)
        slope = 1.0;
    if (slope != self->last_slope) {
        self->last_slope = slope;
        self->factor = MYEXP(-1.0 / (self->sr / ((slope * 48.0) + 2.0)));
    }

    for (i=0; i<self->bufsize; i++) {
        if (count == 0)
            freq = fr[i];
        else if (count >= maxcount)
            count = 0;
        count++;
        if (freq != self->last_freq || spread != self->last_spread || q != self->last_q || self->stages != self->last_stages || self->flag) {
            self->last_freq = freq;
            self->last_spread = spread;
            self->last_q = q;
            self->last_stages = self->stages;
            self->flag = 0;
            Vocoder_compute_variables(self, freq, spread, q);
        }
        output = 0.0;
        vin = in[i];
        vin2 = in2[i];
        for (j=0; j<self->stages; j++) {
            j2 = j * 2;
            /* Analysis part filter 1 */
            w = ( vin - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) )  * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 1 */
            w2 = ( vin2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            j2++;
            /* Analysis part filter 2 */
            w = ( vout - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) ) * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 2 */
            w2 = ( vout2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            /* Follower */
            if (vout < 0.0)
                vout = -vout;
            self->follow[j] = vout + self->factor * (self->follow[j] - vout);
            output += vout2 * self->follow[j];
        }
        self->data[i] = output * amp;
    }
}

static void
Vocoder_filters_iai(Vocoder *self) {
    int i, j, j2;
    int count = 0, maxcount = self->bufsize / 4;
    MYFLT vin, vout, vin2, vout2, w, w2, freq, q, slope, output, amp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *sprd = Stream_getData((Stream *)self->spread_stream);
    MYFLT spread = sprd[0];
    q = PyFloat_AS_DOUBLE(self->q);
    if (q < 0.1)
        q = 0.1;
    amp = q * 10.0;

    if (self->modebuffer[5] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (slope < 0.0)
        slope = 0.0;
    else if (slope > 1.0)
        slope = 1.0;
    if (slope != self->last_slope) {
        self->last_slope = slope;
        self->factor = MYEXP(-1.0 / (self->sr / ((slope * 48.0) + 2.0)));
    }

    for (i=0; i<self->bufsize; i++) {
        if (count == 0)
            spread = sprd[i];
        else if (count >= maxcount)
            count = 0;
        count++;
        if (freq != self->last_freq || spread != self->last_spread || q != self->last_q || self->stages != self->last_stages || self->flag) {
            self->last_freq = freq;
            self->last_spread = spread;
            self->last_q = q;
            self->last_stages = self->stages;
            self->flag = 0;
            Vocoder_compute_variables(self, freq, spread, q);
        }
        output = 0.0;
        vin = in[i];
        vin2 = in2[i];
        for (j=0; j<self->stages; j++) {
            j2 = j * 2;
            /* Analysis part filter 1 */
            w = ( vin - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) )  * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 1 */
            w2 = ( vin2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            j2++;
            /* Analysis part filter 2 */
            w = ( vout - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) ) * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 2 */
            w2 = ( vout2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            /* Follower */
            if (vout < 0.0)
                vout = -vout;
            self->follow[j] = vout + self->factor * (self->follow[j] - vout);
            output += vout2 * self->follow[j];
        }
        self->data[i] = output * amp;
    }
}

static void
Vocoder_filters_aai(Vocoder *self) {
    int i, j, j2;
    int count = 0, maxcount = self->bufsize / 4;
    MYFLT vin, vout, vin2, vout2, w, w2, q, slope, output, amp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT freq = fr[0];
    MYFLT *sprd = Stream_getData((Stream *)self->spread_stream);
    MYFLT spread = sprd[0];
    q = PyFloat_AS_DOUBLE(self->q);
    if (q < 0.1)
        q = 0.1;
    amp = q * 10.0;

    if (self->modebuffer[5] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (slope < 0.0)
        slope = 0.0;
    else if (slope > 1.0)
        slope = 1.0;
    if (slope != self->last_slope) {
        self->last_slope = slope;
        self->factor = MYEXP(-1.0 / (self->sr / ((slope * 48.0) + 2.0)));
    }

    for (i=0; i<self->bufsize; i++) {
        if (count == 0) {
            freq = fr[i];
            spread = sprd[i];
        }
        else if (count >= maxcount)
            count = 0;
        count++;
        if (freq != self->last_freq || spread != self->last_spread || q != self->last_q || self->stages != self->last_stages || self->flag) {
            self->last_freq = freq;
            self->last_spread = spread;
            self->last_q = q;
            self->last_stages = self->stages;
            self->flag = 0;
            Vocoder_compute_variables(self, freq, spread, q);
        }
        output = 0.0;
        vin = in[i];
        vin2 = in2[i];
        for (j=0; j<self->stages; j++) {
            j2 = j * 2;
            /* Analysis part filter 1 */
            w = ( vin - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) )  * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 1 */
            w2 = ( vin2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            j2++;
            /* Analysis part filter 2 */
            w = ( vout - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) ) * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 2 */
            w2 = ( vout2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            /* Follower */
            if (vout < 0.0)
                vout = -vout;
            self->follow[j] = vout + self->factor * (self->follow[j] - vout);
            output += vout2 * self->follow[j];
        }
        self->data[i] = output * amp;
    }
}

static void
Vocoder_filters_iia(Vocoder *self) {
    int i, j, j2;
    int count = 0, maxcount = self->bufsize / 4;
    MYFLT vin, vout, vin2, vout2, w, w2, freq, spread, slope, output, amp = 1.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    freq = PyFloat_AS_DOUBLE(self->freq);
    spread = PyFloat_AS_DOUBLE(self->spread);
    MYFLT *qstr = Stream_getData((Stream *)self->q_stream);
    MYFLT q = qstr[0];
    if (self->modebuffer[5] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (slope < 0.0)
        slope = 0.0;
    else if (slope > 1.0)
        slope = 1.0;
    if (slope != self->last_slope) {
        self->last_slope = slope;
        self->factor = MYEXP(-1.0 / (self->sr / ((slope * 48.0) + 2.0)));
    }

    for (i=0; i<self->bufsize; i++) {
        if (count == 0) {
            q = qstr[i];
            if (q < 0.1)
                q = 0.1;
            amp = q * 10.0;
        }
        else if (count >= maxcount)
            count = 0;
        count++;
        if (freq != self->last_freq || spread != self->last_spread || q != self->last_q || self->stages != self->last_stages || self->flag) {
            self->last_freq = freq;
            self->last_spread = spread;
            self->last_q = q;
            self->last_stages = self->stages;
            self->flag = 0;
            Vocoder_compute_variables(self, freq, spread, q);
        }
        output = 0.0;
        vin = in[i];
        vin2 = in2[i];
        for (j=0; j<self->stages; j++) {
            j2 = j * 2;
            /* Analysis part filter 1 */
            w = ( vin - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) )  * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 1 */
            w2 = ( vin2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            j2++;
            /* Analysis part filter 2 */
            w = ( vout - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) ) * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 2 */
            w2 = ( vout2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            /* Follower */
            if (vout < 0.0)
                vout = -vout;
            self->follow[j] = vout + self->factor * (self->follow[j] - vout);
            output += vout2 * self->follow[j];
        }
        self->data[i] = output * amp;
    }
}

static void
Vocoder_filters_aia(Vocoder *self) {
    int i, j, j2;
    int count = 0, maxcount = self->bufsize / 4;
    MYFLT vin, vout, vin2, vout2, w, w2, spread, slope, output, amp = 1.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT freq = fr[0];
    spread = PyFloat_AS_DOUBLE(self->spread);
    MYFLT *qstr = Stream_getData((Stream *)self->q_stream);
    MYFLT q = qstr[0];
    if (self->modebuffer[5] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (slope < 0.0)
        slope = 0.0;
    else if (slope > 1.0)
        slope = 1.0;
    if (slope != self->last_slope) {
        self->last_slope = slope;
        self->factor = MYEXP(-1.0 / (self->sr / ((slope * 48.0) + 2.0)));
    }

    for (i=0; i<self->bufsize; i++) {
        if (count == 0) {
            freq = fr[i];
            q = qstr[i];
            if (q < 0.1)
                q = 0.1;
            amp = q * 10.0;
        }
        else if (count >= maxcount)
            count = 0;
        count++;
        if (freq != self->last_freq || spread != self->last_spread || q != self->last_q || self->stages != self->last_stages || self->flag) {
            self->last_freq = freq;
            self->last_spread = spread;
            self->last_q = q;
            self->last_stages = self->stages;
            self->flag = 0;
            Vocoder_compute_variables(self, freq, spread, q);
        }
        output = 0.0;
        vin = in[i];
        vin2 = in2[i];
        for (j=0; j<self->stages; j++) {
            j2 = j * 2;
            /* Analysis part filter 1 */
            w = ( vin - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) )  * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 1 */
            w2 = ( vin2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            j2++;
            /* Analysis part filter 2 */
            w = ( vout - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) ) * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 2 */
            w2 = ( vout2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            /* Follower */
            if (vout < 0.0)
                vout = -vout;
            self->follow[j] = vout + self->factor * (self->follow[j] - vout);
            output += vout2 * self->follow[j];
        }
        self->data[i] = output * amp;
    }
}

static void
Vocoder_filters_iaa(Vocoder *self) {
    int i, j, j2;
    int count = 0, maxcount = self->bufsize / 4;
    MYFLT vin, vout, vin2, vout2, w, w2, freq, slope, output, amp = 1.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *sprd = Stream_getData((Stream *)self->spread_stream);
    MYFLT spread = sprd[0];
    MYFLT *qstr = Stream_getData((Stream *)self->q_stream);
    MYFLT q = qstr[0];
    if (self->modebuffer[5] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (slope < 0.0)
        slope = 0.0;
    else if (slope > 1.0)
        slope = 1.0;
    if (slope != self->last_slope) {
        self->last_slope = slope;
        self->factor = MYEXP(-1.0 / (self->sr / ((slope * 48.0) + 2.0)));
    }

    for (i=0; i<self->bufsize; i++) {
        if (count == 0) {
            spread = sprd[i];
            q = qstr[i];
            if (q < 0.1)
                q = 0.1;
            amp = q * 10.0;
        }
        else if (count >= maxcount)
            count = 0;
        count++;
        if (freq != self->last_freq || spread != self->last_spread || q != self->last_q || self->stages != self->last_stages || self->flag) {
            self->last_freq = freq;
            self->last_spread = spread;
            self->last_q = q;
            self->last_stages = self->stages;
            self->flag = 0;
            Vocoder_compute_variables(self, freq, spread, q);
        }
        output = 0.0;
        vin = in[i];
        vin2 = in2[i];
        for (j=0; j<self->stages; j++) {
            j2 = j * 2;
            /* Analysis part filter 1 */
            w = ( vin - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) )  * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 1 */
            w2 = ( vin2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            j2++;
            /* Analysis part filter 2 */
            w = ( vout - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) ) * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 2 */
            w2 = ( vout2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            /* Follower */
            if (vout < 0.0)
                vout = -vout;
            self->follow[j] = vout + self->factor * (self->follow[j] - vout);
            output += vout2 * self->follow[j];
        }
        self->data[i] = output * amp;
    }
}

static void
Vocoder_filters_aaa(Vocoder *self) {
    int i, j, j2;
    int count = 0, maxcount = self->bufsize / 4;
    MYFLT vin, vout, vin2, vout2, w, w2, slope, output, amp = 1.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT freq = fr[0];
    MYFLT *sprd = Stream_getData((Stream *)self->spread_stream);
    MYFLT spread = sprd[0];
    MYFLT *qstr = Stream_getData((Stream *)self->q_stream);
    MYFLT q = qstr[0];
    if (self->modebuffer[5] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (slope < 0.0)
        slope = 0.0;
    else if (slope > 1.0)
        slope = 1.0;
    if (slope != self->last_slope) {
        self->last_slope = slope;
        self->factor = MYEXP(-1.0 / (self->sr / ((slope * 99.0) + 1.0)));
    }

    for (i=0; i<self->bufsize; i++) {
        if (count == 0) {
            freq = fr[i];
            spread = sprd[i];
            q = qstr[i];
            if (q < 0.1)
                q = 0.1;
            amp = q * 10.0;
        }
        else if (count >= maxcount)
            count = 0;
        count++;
        if (freq != self->last_freq || spread != self->last_spread || q != self->last_q || self->stages != self->last_stages || self->flag) {
            self->last_freq = freq;
            self->last_spread = spread;
            self->last_q = q;
            self->last_stages = self->stages;
            self->flag = 0;
            Vocoder_compute_variables(self, freq, spread, q);
        }
        output = 0.0;
        vin = in[i];
        vin2 = in2[i];
        for (j=0; j<self->stages; j++) {
            j2 = j * 2;
            /* Analysis part filter 1 */
            w = ( vin - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) )  * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 1 */
            w2 = ( vin2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            j2++;
            /* Analysis part filter 2 */
            w = ( vout - (self->a1[j] * self->y1[j2]) - (self->a2[j] * self->y2[j2]) ) * self->a0[j];
            vout = (self->b0[j] * w) + (self->b2[j] * self->y2[j2]);
            self->y2[j2] = self->y1[j2];
            self->y1[j2] = w;

            /* Exciter part filter 2 */
            w2 = ( vout2 - (self->a1[j] * self->yy1[j2]) - (self->a2[j] * self->yy2[j2]) ) * self->a0[j];
            vout2 = (self->b0[j] * w2) + (self->b2[j] * self->yy2[j2]);
            self->yy2[j2] = self->yy1[j2];
            self->yy1[j2] = w2;

            /* Follower */
            if (vout < 0.0)
                vout = -vout;
            self->follow[j] = vout + self->factor * (self->follow[j] - vout);
            output += vout2 * self->follow[j];
        }
        self->data[i] = output * amp;
    }
}

static void Vocoder_postprocessing_ii(Vocoder *self) { POST_PROCESSING_II };
static void Vocoder_postprocessing_ai(Vocoder *self) { POST_PROCESSING_AI };
static void Vocoder_postprocessing_ia(Vocoder *self) { POST_PROCESSING_IA };
static void Vocoder_postprocessing_aa(Vocoder *self) { POST_PROCESSING_AA };
static void Vocoder_postprocessing_ireva(Vocoder *self) { POST_PROCESSING_IREVA };
static void Vocoder_postprocessing_areva(Vocoder *self) { POST_PROCESSING_AREVA };
static void Vocoder_postprocessing_revai(Vocoder *self) { POST_PROCESSING_REVAI };
static void Vocoder_postprocessing_revaa(Vocoder *self) { POST_PROCESSING_REVAA };
static void Vocoder_postprocessing_revareva(Vocoder *self) { POST_PROCESSING_REVAREVA };

static void
Vocoder_setProcMode(Vocoder *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Vocoder_filters_iii;
            break;
        case 1:
            self->proc_func_ptr = Vocoder_filters_aii;
            break;
        case 10:
            self->proc_func_ptr = Vocoder_filters_iai;
            break;
        case 11:
            self->proc_func_ptr = Vocoder_filters_aai;
            break;
        case 100:
            self->proc_func_ptr = Vocoder_filters_iia;
            break;
        case 101:
            self->proc_func_ptr = Vocoder_filters_aia;
            break;
        case 110:
            self->proc_func_ptr = Vocoder_filters_iaa;
            break;
        case 111:
            self->proc_func_ptr = Vocoder_filters_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Vocoder_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Vocoder_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Vocoder_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Vocoder_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Vocoder_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Vocoder_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Vocoder_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Vocoder_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Vocoder_postprocessing_revareva;
            break;
    }
}

static void
Vocoder_compute_next_data_frame(Vocoder *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Vocoder_traverse(Vocoder *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->spread);
    Py_VISIT(self->spread_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    Py_VISIT(self->slope);
    Py_VISIT(self->slope_stream);
    return 0;
}

static int
Vocoder_clear(Vocoder *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->spread);
    Py_CLEAR(self->spread_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    Py_CLEAR(self->slope);
    Py_CLEAR(self->slope_stream);
    return 0;
}

static void
Vocoder_dealloc(Vocoder* self)
{
    pyo_DEALLOC
    free(self->y1);
    free(self->y2);
    free(self->yy1);
    free(self->yy2);
    free(self->b0);
    free(self->b2);
    free(self->a0);
    free(self->a1);
    free(self->a2);
    free(self->follow);
    Vocoder_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Vocoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *freqtmp=NULL, *spreadtmp=NULL, *qtmp=NULL, *slopetmp=NULL, *multmp=NULL, *addtmp=NULL;
    Vocoder *self;
    self = (Vocoder *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(60);
    self->spread = PyFloat_FromDouble(1.25);
    self->q = PyFloat_FromDouble(20);
    self->slope = PyFloat_FromDouble(0.5);
    self->last_freq = self->last_spread = self->last_q = self->last_slope = -1.0;
    self->factor = 0.99;
    self->stages = 24;
    self->last_stages = -1;
    self->flag = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
	self->modebuffer[5] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;
    self->twoPiOnSr = (MYFLT)(TWOPI / self->sr);

    Stream_setFunctionPtr(self->stream, Vocoder_compute_next_data_frame);
    self->mode_func_ptr = Vocoder_setProcMode;

    static char *kwlist[] = {"input", "input2", "freq", "spread", "q", "slope", "stages", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOOOiOO", kwlist, &inputtmp, &input2tmp, &freqtmp, &spreadtmp, &qtmp, &slopetmp, &self->stages, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if ( PyObject_HasAttrString((PyObject *)input2tmp, "server") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of Vocoder must be a PyoObject.\n");
        Py_RETURN_NONE;
    }
    Py_XDECREF(self->input2);
    self->input2 = input2tmp;
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
    Py_INCREF(input2_streamtmp);
    Py_XDECREF(self->input2_stream);
    self->input2_stream = (Stream *)input2_streamtmp;

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (spreadtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpread", "O", spreadtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (slopetmp) {
        PyObject_CallMethod((PyObject *)self, "setSlope", "O", slopetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Vocoder_allocate_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Vocoder_getServer(Vocoder* self) { GET_SERVER };
static PyObject * Vocoder_getStream(Vocoder* self) { GET_STREAM };
static PyObject * Vocoder_setMul(Vocoder *self, PyObject *arg) { SET_MUL };
static PyObject * Vocoder_setAdd(Vocoder *self, PyObject *arg) { SET_ADD };
static PyObject * Vocoder_setSub(Vocoder *self, PyObject *arg) { SET_SUB };
static PyObject * Vocoder_setDiv(Vocoder *self, PyObject *arg) { SET_DIV };

static PyObject * Vocoder_play(Vocoder *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Vocoder_out(Vocoder *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Vocoder_stop(Vocoder *self) { STOP };

static PyObject * Vocoder_multiply(Vocoder *self, PyObject *arg) { MULTIPLY };
static PyObject * Vocoder_inplace_multiply(Vocoder *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Vocoder_add(Vocoder *self, PyObject *arg) { ADD };
static PyObject * Vocoder_inplace_add(Vocoder *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Vocoder_sub(Vocoder *self, PyObject *arg) { SUB };
static PyObject * Vocoder_inplace_sub(Vocoder *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Vocoder_div(Vocoder *self, PyObject *arg) { DIV };
static PyObject * Vocoder_inplace_div(Vocoder *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Vocoder_setFreq(Vocoder *self, PyObject *arg)
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
Vocoder_setSpread(Vocoder *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->spread);
	if (isNumber == 1) {
		self->spread = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->spread = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->spread, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->spread_stream);
        self->spread_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Vocoder_setQ(Vocoder *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Vocoder_setSlope(Vocoder *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->slope);
	if (isNumber == 1) {
		self->slope = PyNumber_Float(tmp);
        self->modebuffer[5] = 0;
	}
	else {
		self->slope = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->slope, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->slope_stream);
        self->slope_stream = (Stream *)streamtmp;
		self->modebuffer[5] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Vocoder_setStages(Vocoder *self, PyObject *arg)
{

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->stages = PyInt_AsLong(arg);
        Vocoder_allocate_memories(self);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Vocoder_members[] = {
    {"server", T_OBJECT_EX, offsetof(Vocoder, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Vocoder, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Vocoder, input), 0, "Spectral envelope signal."},
    {"input2", T_OBJECT_EX, offsetof(Vocoder, input2), 0, "Exciter signal."},
    {"freq", T_OBJECT_EX, offsetof(Vocoder, freq), 0, "Base frequency in cycle per second."},
    {"spread", T_OBJECT_EX, offsetof(Vocoder, spread), 0, "Frequency expansion factor."},
    {"q", T_OBJECT_EX, offsetof(Vocoder, q), 0, "Q factor."},
    {"slope", T_OBJECT_EX, offsetof(Vocoder, slope), 0, "Responsiveness of the follower lowpass filter."},
    {"mul", T_OBJECT_EX, offsetof(Vocoder, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Vocoder, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Vocoder_methods[] = {
    {"getServer", (PyCFunction)Vocoder_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Vocoder_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Vocoder_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Vocoder_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Vocoder_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Vocoder_setFreq, METH_O, "Sets filter base frequency in cycle per second."},
	{"setSpread", (PyCFunction)Vocoder_setSpread, METH_O, "Sets frequency expansion factor."},
    {"setQ", (PyCFunction)Vocoder_setQ, METH_O, "Sets filter Q factor."},
    {"setSlope", (PyCFunction)Vocoder_setSlope, METH_O, "Sets responsiveness of the follower."},
    {"setStages", (PyCFunction)Vocoder_setStages, METH_O, "Sets the number of filtering stages."},
	{"setMul", (PyCFunction)Vocoder_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Vocoder_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Vocoder_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Vocoder_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Vocoder_as_number = {
    (binaryfunc)Vocoder_add,                         /*nb_add*/
    (binaryfunc)Vocoder_sub,                         /*nb_subtract*/
    (binaryfunc)Vocoder_multiply,                    /*nb_multiply*/
    (binaryfunc)Vocoder_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)Vocoder_inplace_add,                 /*inplace_add*/
    (binaryfunc)Vocoder_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Vocoder_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Vocoder_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject VocoderType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Vocoder_base",                                   /*tp_name*/
    sizeof(Vocoder),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Vocoder_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Vocoder_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Vocoder objects. Bank of bandpass filters implementing the vocoder effect.",           /* tp_doc */
    (traverseproc)Vocoder_traverse,                  /* tp_traverse */
    (inquiry)Vocoder_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Vocoder_methods,                                 /* tp_methods */
    Vocoder_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Vocoder_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    PyObject *type;
    Stream *type_stream;
    int modebuffer[5]; // need at least 2 slots for mul & add
    MYFLT srOverSix;
    MYFLT last_freq;
    MYFLT piOverSr;
    // sample memories
    MYFLT y1;
    MYFLT y2;
    MYFLT y3;
    MYFLT y4;
    // variables
    MYFLT w;
} SVF;

static void
SVF_filters_iii(SVF *self) {
    int i;
    MYFLT val, freq, q, type, q1, low, high, band, lowgain, highgain, bandgain;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    freq = PyFloat_AS_DOUBLE(self->freq);
    q = PyFloat_AS_DOUBLE(self->q);
    type = PyFloat_AS_DOUBLE(self->type);

    if (freq < 0.1)
        freq = 0.1;
    else if (freq > self->srOverSix)
        freq = self->srOverSix;

    if (freq != self->last_freq) {
        self->last_freq = freq;
        self->w = 2.0 * MYSIN(freq * self->piOverSr);
    }

    if (q < 0.5)
        q = 0.5;
    q1 = 1.0 / q;

    if (type < 0.0)
        type = 0.0;
    else if (type > 1.0)
        type = 1.0;

    lowgain = (type <= 0.5) ? (0.5 - type) : 0.0;
    highgain = (type >= 0.5) ? (type - 0.5) : 0.0;
    bandgain = (type <= 0.5) ? type : (1.0 - type);
    for (i=0; i<self->bufsize; i++) {
        low = self->y2 + self->w * self->y1;
        high = in[i] - low - q1 * self->y1;
        band = self->w * high + self->y1;
        self->y1 = band;
        self->y2 = low;
        val = low * lowgain + high * highgain + band * bandgain;
        low = self->y4 + self->w * self->y3;
        high = val - low - q1 * self->y3;
        band = self->w * high + self->y3;
        self->y3 = band;
        self->y4 = low;
        self->data[i] = low * lowgain + high * highgain + band * bandgain;
    }
}

static void
SVF_filters_aii(SVF *self) {
    int i;
    MYFLT val, freq, q, type, q1, low, high, band, lowgain, highgain, bandgain;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    type = PyFloat_AS_DOUBLE(self->type);

    if (q < 0.5)
        q = 0.5;
    q1 = 1.0 / q;

    if (type < 0.0)
        type = 0.0;
    else if (type > 1.0)
        type = 1.0;

    lowgain = (type <= 0.5) ? (0.5 - type) : 0.0;
    highgain = (type >= 0.5) ? (type - 0.5) : 0.0;
    bandgain = (type <= 0.5) ? type : (1.0 - type);
    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq < 0.1)
            freq = 0.1;
        else if (freq > self->srOverSix)
            freq = self->srOverSix;

        if (freq != self->last_freq) {
            self->last_freq = freq;
            self->w = 2.0 * MYSIN(freq * self->piOverSr);
        }
        low = self->y2 + self->w * self->y1;
        high = in[i] - low - q1 * self->y1;
        band = self->w * high + self->y1;
        self->y1 = band;
        self->y2 = low;
        val = low * lowgain + high * highgain + band * bandgain;
        low = self->y4 + self->w * self->y3;
        high = val - low - q1 * self->y3;
        band = self->w * high + self->y3;
        self->y3 = band;
        self->y4 = low;
        self->data[i] = low * lowgain + high * highgain + band * bandgain;
    }
}

static void
SVF_filters_iai(SVF *self) {
    int i;
    MYFLT val, freq, q, type, q1, low, high, band, lowgain, highgain, bandgain;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);
    type = PyFloat_AS_DOUBLE(self->type);

    if (freq < 0.1)
        freq = 0.1;
    else if (freq > self->srOverSix)
        freq = self->srOverSix;

    if (freq != self->last_freq) {
        self->last_freq = freq;
        self->w = 2.0 * MYSIN(freq * self->piOverSr);
    }

    if (type < 0.0)
        type = 0.0;
    else if (type > 1.0)
        type = 1.0;

    lowgain = (type <= 0.5) ? (0.5 - type) : 0.0;
    highgain = (type >= 0.5) ? (type - 0.5) : 0.0;
    bandgain = (type <= 0.5) ? type : (1.0 - type);
    for (i=0; i<self->bufsize; i++) {
        q = qst[i];
        if (q < 0.5)
            q = 0.5;
        q1 = 1.0 / q;
        low = self->y2 + self->w * self->y1;
        high = in[i] - low - q1 * self->y1;
        band = self->w * high + self->y1;
        self->y1 = band;
        self->y2 = low;
        val = low * lowgain + high * highgain + band * bandgain;
        low = self->y4 + self->w * self->y3;
        high = val - low - q1 * self->y3;
        band = self->w * high + self->y3;
        self->y3 = band;
        self->y4 = low;
        self->data[i] = low * lowgain + high * highgain + band * bandgain;
    }
}

static void
SVF_filters_aai(SVF *self) {
    int i;
    MYFLT val, freq, q, type, q1, low, high, band, lowgain, highgain, bandgain;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);
    type = PyFloat_AS_DOUBLE(self->type);

    if (type < 0.0)
        type = 0.0;
    else if (type > 1.0)
        type = 1.0;

    lowgain = (type <= 0.5) ? (0.5 - type) : 0.0;
    highgain = (type >= 0.5) ? (type - 0.5) : 0.0;
    bandgain = (type <= 0.5) ? type : (1.0 - type);
    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        q = qst[i];
        if (freq < 0.1)
            freq = 0.1;
        else if (freq > self->srOverSix)
            freq = self->srOverSix;

        if (freq != self->last_freq) {
            self->last_freq = freq;
            self->w = 2.0 * MYSIN(freq * self->piOverSr);
        }
        if (q < 0.5)
            q = 0.5;
        q1 = 1.0 / q;
        low = self->y2 + self->w * self->y1;
        high = in[i] - low - q1 * self->y1;
        band = self->w * high + self->y1;
        self->y1 = band;
        self->y2 = low;
        val = low * lowgain + high * highgain + band * bandgain;
        low = self->y4 + self->w * self->y3;
        high = val - low - q1 * self->y3;
        band = self->w * high + self->y3;
        self->y3 = band;
        self->y4 = low;
        self->data[i] = low * lowgain + high * highgain + band * bandgain;
    }
}

static void
SVF_filters_iia(SVF *self) {
    int i;
    MYFLT val, freq, q, type, q1, low, high, band, lowgain, highgain, bandgain;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    freq = PyFloat_AS_DOUBLE(self->freq);
    q = PyFloat_AS_DOUBLE(self->q);
    MYFLT *tp = Stream_getData((Stream *)self->type_stream);

    if (freq < 0.1)
        freq = 0.1;
    else if (freq > self->srOverSix)
        freq = self->srOverSix;

    if (freq != self->last_freq) {
        self->last_freq = freq;
        self->w = 2.0 * MYSIN(freq * self->piOverSr);
    }

    if (q < 0.5)
        q = 0.5;
    q1 = 1.0 / q;

    for (i=0; i<self->bufsize; i++) {
        type = tp[i];
        if (type < 0.0)
            type = 0.0;
        else if (type > 1.0)
            type = 1.0;
        lowgain = (type <= 0.5) ? (0.5 - type) : 0.0;
        highgain = (type >= 0.5) ? (type - 0.5) : 0.0;
        bandgain = (type <= 0.5) ? type : (1.0 - type);
        low = self->y2 + self->w * self->y1;
        high = in[i] - low - q1 * self->y1;
        band = self->w * high + self->y1;
        self->y1 = band;
        self->y2 = low;
        val = low * lowgain + high * highgain + band * bandgain;
        low = self->y4 + self->w * self->y3;
        high = val - low - q1 * self->y3;
        band = self->w * high + self->y3;
        self->y3 = band;
        self->y4 = low;
        self->data[i] = low * lowgain + high * highgain + band * bandgain;
    }
}

static void
SVF_filters_aia(SVF *self) {
    int i;
    MYFLT val, freq, q, type, q1, low, high, band, lowgain, highgain, bandgain;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    MYFLT *tp = Stream_getData((Stream *)self->type_stream);

    if (q < 0.5)
        q = 0.5;
    q1 = 1.0 / q;

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        type = tp[i];
        if (freq < 0.1)
            freq = 0.1;
        else if (freq > self->srOverSix)
            freq = self->srOverSix;

        if (freq != self->last_freq) {
            self->last_freq = freq;
            self->w = 2.0 * MYSIN(freq * self->piOverSr);
        }
        if (type < 0.0)
            type = 0.0;
        else if (type > 1.0)
            type = 1.0;
        lowgain = (type <= 0.5) ? (0.5 - type) : 0.0;
        highgain = (type >= 0.5) ? (type - 0.5) : 0.0;
        bandgain = (type <= 0.5) ? type : (1.0 - type);
        low = self->y2 + self->w * self->y1;
        high = in[i] - low - q1 * self->y1;
        band = self->w * high + self->y1;
        self->y1 = band;
        self->y2 = low;
        val = low * lowgain + high * highgain + band * bandgain;
        low = self->y4 + self->w * self->y3;
        high = val - low - q1 * self->y3;
        band = self->w * high + self->y3;
        self->y3 = band;
        self->y4 = low;
        self->data[i] = low * lowgain + high * highgain + band * bandgain;
    }
}

static void
SVF_filters_iaa(SVF *self) {
    int i;
    MYFLT val, freq, q, type, q1, low, high, band, lowgain, highgain, bandgain;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);
    MYFLT *tp = Stream_getData((Stream *)self->type_stream);

    if (freq < 0.1)
        freq = 0.1;
    else if (freq > self->srOverSix)
        freq = self->srOverSix;

    if (freq != self->last_freq) {
        self->last_freq = freq;
        self->w = 2.0 * MYSIN(freq * self->piOverSr);
    }

    for (i=0; i<self->bufsize; i++) {
        q = qst[i];
        type = tp[i];
        if (q < 0.5)
            q = 0.5;
        q1 = 1.0 / q;
        if (type < 0.0)
            type = 0.0;
        else if (type > 1.0)
            type = 1.0;
        lowgain = (type <= 0.5) ? (0.5 - type) : 0.0;
        highgain = (type >= 0.5) ? (type - 0.5) : 0.0;
        bandgain = (type <= 0.5) ? type : (1.0 - type);
        low = self->y2 + self->w * self->y1;
        high = in[i] - low - q1 * self->y1;
        band = self->w * high + self->y1;
        self->y1 = band;
        self->y2 = low;
        val = low * lowgain + high * highgain + band * bandgain;
        low = self->y4 + self->w * self->y3;
        high = val - low - q1 * self->y3;
        band = self->w * high + self->y3;
        self->y3 = band;
        self->y4 = low;
        self->data[i] = low * lowgain + high * highgain + band * bandgain;
    }
}

static void
SVF_filters_aaa(SVF *self) {
    int i;
    MYFLT val, freq, q, type, q1, low, high, band, lowgain, highgain, bandgain;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);
    MYFLT *tp = Stream_getData((Stream *)self->type_stream);

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        q = qst[i];
        type = tp[i];
        if (freq < 0.1)
            freq = 0.1;
        else if (freq > self->srOverSix)
            freq = self->srOverSix;

        if (freq != self->last_freq) {
            self->last_freq = freq;
            self->w = 2.0 * MYSIN(freq * self->piOverSr);
        }
        if (q < 0.5)
            q = 0.5;
        q1 = 1.0 / q;
        if (type < 0.0)
            type = 0.0;
        else if (type > 1.0)
            type = 1.0;
        lowgain = (type <= 0.5) ? (0.5 - type) : 0.0;
        highgain = (type >= 0.5) ? (type - 0.5) : 0.0;
        bandgain = (type <= 0.5) ? type : (1.0 - type);
        low = self->y2 + self->w * self->y1;
        high = in[i] - low - q1 * self->y1;
        band = self->w * high + self->y1;
        self->y1 = band;
        self->y2 = low;
        val = low * lowgain + high * highgain + band * bandgain;
        low = self->y4 + self->w * self->y3;
        high = val - low - q1 * self->y3;
        band = self->w * high + self->y3;
        self->y3 = band;
        self->y4 = low;
        self->data[i] = low * lowgain + high * highgain + band * bandgain;
    }
}

static void SVF_postprocessing_ii(SVF *self) { POST_PROCESSING_II };
static void SVF_postprocessing_ai(SVF *self) { POST_PROCESSING_AI };
static void SVF_postprocessing_ia(SVF *self) { POST_PROCESSING_IA };
static void SVF_postprocessing_aa(SVF *self) { POST_PROCESSING_AA };
static void SVF_postprocessing_ireva(SVF *self) { POST_PROCESSING_IREVA };
static void SVF_postprocessing_areva(SVF *self) { POST_PROCESSING_AREVA };
static void SVF_postprocessing_revai(SVF *self) { POST_PROCESSING_REVAI };
static void SVF_postprocessing_revaa(SVF *self) { POST_PROCESSING_REVAA };
static void SVF_postprocessing_revareva(SVF *self) { POST_PROCESSING_REVAREVA };

static void
SVF_setProcMode(SVF *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = SVF_filters_iii;
            break;
        case 1:
            self->proc_func_ptr = SVF_filters_aii;
            break;
        case 10:
            self->proc_func_ptr = SVF_filters_iai;
            break;
        case 11:
            self->proc_func_ptr = SVF_filters_aai;
            break;
        case 100:
            self->proc_func_ptr = SVF_filters_iia;
            break;
        case 101:
            self->proc_func_ptr = SVF_filters_aia;
            break;
        case 110:
            self->proc_func_ptr = SVF_filters_iaa;
            break;
        case 111:
            self->proc_func_ptr = SVF_filters_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = SVF_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = SVF_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = SVF_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = SVF_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = SVF_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = SVF_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = SVF_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = SVF_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = SVF_postprocessing_revareva;
            break;
    }
}

static void
SVF_compute_next_data_frame(SVF *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
SVF_traverse(SVF *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    Py_VISIT(self->type);
    Py_VISIT(self->type_stream);
    return 0;
}

static int
SVF_clear(SVF *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    Py_CLEAR(self->type);
    Py_CLEAR(self->type_stream);
    return 0;
}

static void
SVF_dealloc(SVF* self)
{
    pyo_DEALLOC
    SVF_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
SVF_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *typetmp=NULL, *multmp=NULL, *addtmp=NULL;
    SVF *self;
    self = (SVF *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->type = PyFloat_FromDouble(0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    self->y1 = self->y2 = self->y3 = self->y4 = self->w = 0.0;
    self->last_freq = -1.0;

    INIT_OBJECT_COMMON

    self->srOverSix = (MYFLT)self->sr / 6.0;
    self->piOverSr = PI / self->sr;

    Stream_setFunctionPtr(self->stream, SVF_compute_next_data_frame);
    self->mode_func_ptr = SVF_setProcMode;

    static char *kwlist[] = {"input", "freq", "q", "type", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOO", kwlist, &inputtmp, &freqtmp, &qtmp, &typetmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (typetmp) {
        PyObject_CallMethod((PyObject *)self, "setType", "O", typetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * SVF_getServer(SVF* self) { GET_SERVER };
static PyObject * SVF_getStream(SVF* self) { GET_STREAM };
static PyObject * SVF_setMul(SVF *self, PyObject *arg) { SET_MUL };
static PyObject * SVF_setAdd(SVF *self, PyObject *arg) { SET_ADD };
static PyObject * SVF_setSub(SVF *self, PyObject *arg) { SET_SUB };
static PyObject * SVF_setDiv(SVF *self, PyObject *arg) { SET_DIV };

static PyObject * SVF_play(SVF *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SVF_out(SVF *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SVF_stop(SVF *self) { STOP };

static PyObject * SVF_multiply(SVF *self, PyObject *arg) { MULTIPLY };
static PyObject * SVF_inplace_multiply(SVF *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SVF_add(SVF *self, PyObject *arg) { ADD };
static PyObject * SVF_inplace_add(SVF *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SVF_sub(SVF *self, PyObject *arg) { SUB };
static PyObject * SVF_inplace_sub(SVF *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SVF_div(SVF *self, PyObject *arg) { DIV };
static PyObject * SVF_inplace_div(SVF *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
SVF_setFreq(SVF *self, PyObject *arg)
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
SVF_setQ(SVF *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SVF_setType(SVF *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->type);
	if (isNumber == 1) {
		self->type = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->type = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->type, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->type_stream);
        self->type_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef SVF_members[] = {
    {"server", T_OBJECT_EX, offsetof(SVF, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(SVF, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(SVF, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(SVF, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(SVF, q), 0, "Q factor."},
    {"type", T_OBJECT_EX, offsetof(SVF, type), 0, "Filter type."},
    {"mul", T_OBJECT_EX, offsetof(SVF, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(SVF, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef SVF_methods[] = {
    {"getServer", (PyCFunction)SVF_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)SVF_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)SVF_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)SVF_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)SVF_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)SVF_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)SVF_setQ, METH_O, "Sets filter Q factor."},
    {"setType", (PyCFunction)SVF_setType, METH_O, "Sets filter type factor."},
	{"setMul", (PyCFunction)SVF_setMul, METH_O, "Sets mul factor."},
	{"setAdd", (PyCFunction)SVF_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)SVF_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)SVF_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods SVF_as_number = {
    (binaryfunc)SVF_add,                         /*nb_add*/
    (binaryfunc)SVF_sub,                         /*nb_subtract*/
    (binaryfunc)SVF_multiply,                    /*nb_multiply*/
    (binaryfunc)SVF_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)SVF_inplace_add,                 /*inplace_add*/
    (binaryfunc)SVF_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)SVF_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)SVF_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject SVFType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.SVF_base",                                   /*tp_name*/
    sizeof(SVF),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)SVF_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &SVF_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "SVF objects. Generates a state variable filter.",           /* tp_doc */
    (traverseproc)SVF_traverse,                  /* tp_traverse */
    (inquiry)SVF_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    SVF_methods,                                 /* tp_methods */
    SVF_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    SVF_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int size;
    int halfSize;
    int in_count;
    int init;
    double currentValue;
    double oneOnSize;
    int modebuffer[2];
    MYFLT *buffer; // samples memory
} Average;

static void
Average_process_i(Average *self) {
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init) {
        for (i=0; i<self->bufsize; i++) {
            if (self->init) {
                self->buffer[self->in_count] = in[i];
                self->currentValue += (double)in[i];
                self->in_count++;
                if (self->in_count < self->halfSize)
                    self->data[i] = 0.0;
                else
                    self->data[i] = (MYFLT)(self->currentValue * self->oneOnSize);
                if (self->in_count >= self->size) {
                    self->in_count = 0;
                    self->init = 0;
                }
            }
            else {
                self->buffer[self->in_count] = in[i];
                self->currentValue += (double)in[i];
                self->in_count++;
                if (self->in_count >= self->size)
                    self->in_count = 0;
                self->currentValue -= (double)self->buffer[self->in_count];
                self->data[i] = (MYFLT)(self->currentValue * self->oneOnSize);
            }
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            self->buffer[self->in_count] = in[i];
            self->currentValue += (double)in[i];
            self->in_count++;
            if (self->in_count >= self->size)
                self->in_count = 0;
            self->currentValue -= (double)self->buffer[self->in_count];
            self->data[i] = (MYFLT)(self->currentValue * self->oneOnSize);
        }
    }
}

static void Average_postprocessing_ii(Average *self) { POST_PROCESSING_II };
static void Average_postprocessing_ai(Average *self) { POST_PROCESSING_AI };
static void Average_postprocessing_ia(Average *self) { POST_PROCESSING_IA };
static void Average_postprocessing_aa(Average *self) { POST_PROCESSING_AA };
static void Average_postprocessing_ireva(Average *self) { POST_PROCESSING_IREVA };
static void Average_postprocessing_areva(Average *self) { POST_PROCESSING_AREVA };
static void Average_postprocessing_revai(Average *self) { POST_PROCESSING_REVAI };
static void Average_postprocessing_revaa(Average *self) { POST_PROCESSING_REVAA };
static void Average_postprocessing_revareva(Average *self) { POST_PROCESSING_REVAREVA };

static void
Average_setProcMode(Average *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Average_process_i;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Average_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Average_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Average_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Average_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Average_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Average_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Average_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Average_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Average_postprocessing_revareva;
            break;
    }
}

static void
Average_compute_next_data_frame(Average *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Average_traverse(Average *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
Average_clear(Average *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Average_dealloc(Average* self)
{
    pyo_DEALLOC
    free(self->buffer);
    Average_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Average_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    Average *self;
    self = (Average *)type->tp_alloc(type, 0);

    self->size = 10;
    self->init = 1;
    self->in_count = 0;
    self->currentValue = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Average_compute_next_data_frame);
    self->mode_func_ptr = Average_setProcMode;

    static char *kwlist[] = {"input", "size", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &inputtmp, &self->size, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->halfSize = (int)(self->size / 2);
    self->oneOnSize = 1.0 / (double)self->size;

    self->buffer = (MYFLT *)realloc(self->buffer, (self->size) * sizeof(MYFLT));
    for (i=0; i<(self->size); i++) {
        self->buffer[i] = 0.;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Average_getServer(Average* self) { GET_SERVER };
static PyObject * Average_getStream(Average* self) { GET_STREAM };
static PyObject * Average_setMul(Average *self, PyObject *arg) { SET_MUL };
static PyObject * Average_setAdd(Average *self, PyObject *arg) { SET_ADD };
static PyObject * Average_setSub(Average *self, PyObject *arg) { SET_SUB };
static PyObject * Average_setDiv(Average *self, PyObject *arg) { SET_DIV };

static PyObject * Average_play(Average *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Average_out(Average *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Average_stop(Average *self) { STOP };

static PyObject * Average_multiply(Average *self, PyObject *arg) { MULTIPLY };
static PyObject * Average_inplace_multiply(Average *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Average_add(Average *self, PyObject *arg) { ADD };
static PyObject * Average_inplace_add(Average *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Average_sub(Average *self, PyObject *arg) { SUB };
static PyObject * Average_inplace_sub(Average *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Average_div(Average *self, PyObject *arg) { DIV };
static PyObject * Average_inplace_div(Average *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Average_setSize(Average *self, PyObject *arg)
{
	int i;
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->size = PyInt_AsLong(arg);
        self->halfSize = (int)(self->size / 2);
        self->oneOnSize = 1.0 / (double)self->size;
        self->init = 1;
        self->in_count = 0;
        self->currentValue = 0.0;
        self->buffer = (MYFLT *)realloc(self->buffer, (self->size) * sizeof(MYFLT));
        for (i=0; i<(self->size); i++) {
            self->buffer[i] = 0.;
        }
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Average_members[] = {
    {"server", T_OBJECT_EX, offsetof(Average, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Average, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Average, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Average, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Average, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Average_methods[] = {
    {"getServer", (PyCFunction)Average_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Average_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Average_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Average_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Average_stop, METH_NOARGS, "Stops computing."},
	{"setSize", (PyCFunction)Average_setSize, METH_O, "Sets filter kernel size."},
	{"setMul", (PyCFunction)Average_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Average_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Average_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Average_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Average_as_number = {
    (binaryfunc)Average_add,                      /*nb_add*/
    (binaryfunc)Average_sub,                 /*nb_subtract*/
    (binaryfunc)Average_multiply,                 /*nb_multiply*/
    (binaryfunc)Average_div,                   /*nb_divide*/
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
    (binaryfunc)Average_inplace_add,              /*inplace_add*/
    (binaryfunc)Average_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Average_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Average_inplace_div,           /*inplace_divide*/
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

PyTypeObject AverageType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Average_base",         /*tp_name*/
    sizeof(Average),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Average_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Average_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Average objects. Moving average filter.",           /* tp_doc */
    (traverseproc)Average_traverse,   /* tp_traverse */
    (inquiry)Average_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Average_methods,             /* tp_methods */
    Average_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Average_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT nyquist;
    MYFLT last_freq;
    MYFLT last_q;
    MYFLT twopiOverSr;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // coefficients
    MYFLT b1;
    MYFLT b2;
    MYFLT a;
} Reson;

static void
Reson_compute_coeffs(Reson *self, MYFLT freq, MYFLT q)
{
    MYFLT bw;

    if (freq < 0.1)
        freq = 0.1;
    else if (freq > self->nyquist)
        freq = self->nyquist;
    if (q < 0.1)
        q = 0.1;

    bw = freq / q;

    self->b2 = MYEXP(-self->twopiOverSr * bw);
    self->b1 = (-4.0 * self->b2) / (1.0 + self->b2) * MYCOS(freq * self->twopiOverSr);
    self->a = 1.0 - MYSQRT(self->b2);
}

static void
Reson_filters_ii(Reson *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    fr = PyFloat_AS_DOUBLE(self->freq);
    q = PyFloat_AS_DOUBLE(self->q);

    if (fr != self->last_freq || q != self->last_q) {
        self->last_freq = fr;
        self->last_q = q;
        Reson_compute_coeffs(self, fr, q);
    }

    for (i=0; i<self->bufsize; i++) {
        val = (self->a * in[i]) - (self->a * self->x2) - (self->b1 * self->y1) - (self->b2 * self->y2);
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
    }
}

static void
Reson_filters_ai(Reson *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            Reson_compute_coeffs(self, fr, q);
        }
        val = (self->a * in[i]) - (self->a * self->x2) - (self->b1 * self->y1) - (self->b2 * self->y2);
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
    }
}

static void
Reson_filters_ia(Reson *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        q = qst[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            Reson_compute_coeffs(self, fr, q);
        }
        val = (self->a * in[i]) - (self->a * self->x2) - (self->b1 * self->y1) - (self->b2 * self->y2);
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
    }
}

static void
Reson_filters_aa(Reson *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        q = qst[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            Reson_compute_coeffs(self, fr, q);
        }
        val = (self->a * in[i]) - (self->a * self->x2) - (self->b1 * self->y1) - (self->b2 * self->y2);
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
    }
}

static void Reson_postprocessing_ii(Reson *self) { POST_PROCESSING_II };
static void Reson_postprocessing_ai(Reson *self) { POST_PROCESSING_AI };
static void Reson_postprocessing_ia(Reson *self) { POST_PROCESSING_IA };
static void Reson_postprocessing_aa(Reson *self) { POST_PROCESSING_AA };
static void Reson_postprocessing_ireva(Reson *self) { POST_PROCESSING_IREVA };
static void Reson_postprocessing_areva(Reson *self) { POST_PROCESSING_AREVA };
static void Reson_postprocessing_revai(Reson *self) { POST_PROCESSING_REVAI };
static void Reson_postprocessing_revaa(Reson *self) { POST_PROCESSING_REVAA };
static void Reson_postprocessing_revareva(Reson *self) { POST_PROCESSING_REVAREVA };

static void
Reson_setProcMode(Reson *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Reson_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = Reson_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = Reson_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = Reson_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Reson_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Reson_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Reson_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Reson_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Reson_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Reson_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Reson_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Reson_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Reson_postprocessing_revareva;
            break;
    }
}

static void
Reson_compute_next_data_frame(Reson *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Reson_traverse(Reson *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    return 0;
}

static int
Reson_clear(Reson *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    return 0;
}

static void
Reson_dealloc(Reson* self)
{
    pyo_DEALLOC
    Reson_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Reson_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Reson *self;
    self = (Reson *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->last_freq = self->last_q = -1.0;
    self->x1 = self->x2 = self->y1 = self->y2 = 0.0;
    self->a = self->b1 = self->b2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;
    self->twopiOverSr = TWOPI / (MYFLT)self->sr;

    Stream_setFunctionPtr(self->stream, Reson_compute_next_data_frame);
    self->mode_func_ptr = Reson_setProcMode;

    static char *kwlist[] = {"input", "freq", "q", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &freqtmp, &qtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Reson_getServer(Reson* self) { GET_SERVER };
static PyObject * Reson_getStream(Reson* self) { GET_STREAM };
static PyObject * Reson_setMul(Reson *self, PyObject *arg) { SET_MUL };
static PyObject * Reson_setAdd(Reson *self, PyObject *arg) { SET_ADD };
static PyObject * Reson_setSub(Reson *self, PyObject *arg) { SET_SUB };
static PyObject * Reson_setDiv(Reson *self, PyObject *arg) { SET_DIV };

static PyObject * Reson_play(Reson *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Reson_out(Reson *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Reson_stop(Reson *self) { STOP };

static PyObject * Reson_multiply(Reson *self, PyObject *arg) { MULTIPLY };
static PyObject * Reson_inplace_multiply(Reson *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Reson_add(Reson *self, PyObject *arg) { ADD };
static PyObject * Reson_inplace_add(Reson *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Reson_sub(Reson *self, PyObject *arg) { SUB };
static PyObject * Reson_inplace_sub(Reson *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Reson_div(Reson *self, PyObject *arg) { DIV };
static PyObject * Reson_inplace_div(Reson *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Reson_setFreq(Reson *self, PyObject *arg)
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
Reson_setQ(Reson *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Reson_members[] = {
    {"server", T_OBJECT_EX, offsetof(Reson, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Reson, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Reson, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Reson, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(Reson, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(Reson, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Reson, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Reson_methods[] = {
    {"getServer", (PyCFunction)Reson_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Reson_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Reson_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Reson_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Reson_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Reson_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)Reson_setQ, METH_O, "Sets filter Q factor."},
	{"setMul", (PyCFunction)Reson_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Reson_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Reson_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Reson_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Reson_as_number = {
    (binaryfunc)Reson_add,                         /*nb_add*/
    (binaryfunc)Reson_sub,                         /*nb_subtract*/
    (binaryfunc)Reson_multiply,                    /*nb_multiply*/
    (binaryfunc)Reson_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)Reson_inplace_add,                 /*inplace_add*/
    (binaryfunc)Reson_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Reson_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Reson_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject ResonType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Reson_base",                                   /*tp_name*/
    sizeof(Reson),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Reson_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Reson_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Reson objects. Second-order resonant bandpass filter.",           /* tp_doc */
    (traverseproc)Reson_traverse,                  /* tp_traverse */
    (inquiry)Reson_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Reson_methods,                                 /* tp_methods */
    Reson_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Reson_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    int stages;
    MYFLT nyquist;
    MYFLT last_freq;
    MYFLT last_q;
    MYFLT twopiOverSr;
    // sample memories
    MYFLT *x1;
    MYFLT *x2;
    MYFLT *y1;
    MYFLT *y2;
    // coefficients
    MYFLT b1;
    MYFLT b2;
    MYFLT a;
} Resonx;

static void
Resonx_allocate_memories(Resonx *self)
{
    int i;
    self->x1 = (MYFLT *)realloc(self->x1, self->stages * sizeof(MYFLT));
    self->x2 = (MYFLT *)realloc(self->x2, self->stages * sizeof(MYFLT));
    self->y1 = (MYFLT *)realloc(self->y1, self->stages * sizeof(MYFLT));
    self->y2 = (MYFLT *)realloc(self->y2, self->stages * sizeof(MYFLT));
    for (i=0; i < self->stages; i++) {
        self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = 0.0;
    }
}

static void
Resonx_compute_coeffs(Resonx *self, MYFLT freq, MYFLT q)
{
    MYFLT bw;

    if (freq < 0.1)
        freq = 0.1;
    else if (freq > self->nyquist)
        freq = self->nyquist;
    if (q < 0.1)
        q = 0.1;

    bw = freq / q;

    self->b2 = MYEXP(-self->twopiOverSr * bw);
    self->b1 = (-4.0 * self->b2) / (1.0 + self->b2) * MYCOS(freq * self->twopiOverSr);
    self->a = 1.0 - MYSQRT(self->b2);
}

static void
Resonx_filters_ii(Resonx *self) {
    MYFLT vin, vout, fr, q;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    fr = PyFloat_AS_DOUBLE(self->freq);
    q = PyFloat_AS_DOUBLE(self->q);

    if (fr != self->last_freq || q != self->last_q) {
        self->last_freq = fr;
        self->last_q = q;
        Resonx_compute_coeffs(self, fr, q);
    }

    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        vin = in[i];
        for (j=0; j<self->stages; j++) {
            vout = (self->a * vin) - (self->a * self->x2[j]) - (self->b1 * self->y1[j]) - (self->b2 * self->y2[j]);
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Resonx_filters_ai(Resonx *self) {
    MYFLT vin, vout, fr, q;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);

    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        vin = in[i];
        fr = freq[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            Resonx_compute_coeffs(self, fr, q);
        }
        for (j=0; j<self->stages; j++) {
            vout = (self->a * vin) - (self->a * self->x2[j]) - (self->b1 * self->y1[j]) - (self->b2 * self->y2[j]);
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Resonx_filters_ia(Resonx *self) {
    MYFLT vin, vout, fr, q;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);

    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        vin = in[i];
        q = qst[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            Resonx_compute_coeffs(self, fr, q);
        }
        for (j=0; j<self->stages; j++) {
            vout = (self->a * vin) - (self->a * self->x2[j]) - (self->b1 * self->y1[j]) - (self->b2 * self->y2[j]);
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Resonx_filters_aa(Resonx *self) {
    MYFLT vin, vout, fr, q;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);

    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        vin = in[i];
        fr = freq[i];
        q = qst[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            Resonx_compute_coeffs(self, fr, q);
        }
        for (j=0; j<self->stages; j++) {
            vout = (self->a * vin) - (self->a * self->x2[j]) - (self->b1 * self->y1[j]) - (self->b2 * self->y2[j]);
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void Resonx_postprocessing_ii(Resonx *self) { POST_PROCESSING_II };
static void Resonx_postprocessing_ai(Resonx *self) { POST_PROCESSING_AI };
static void Resonx_postprocessing_ia(Resonx *self) { POST_PROCESSING_IA };
static void Resonx_postprocessing_aa(Resonx *self) { POST_PROCESSING_AA };
static void Resonx_postprocessing_ireva(Resonx *self) { POST_PROCESSING_IREVA };
static void Resonx_postprocessing_areva(Resonx *self) { POST_PROCESSING_AREVA };
static void Resonx_postprocessing_revai(Resonx *self) { POST_PROCESSING_REVAI };
static void Resonx_postprocessing_revaa(Resonx *self) { POST_PROCESSING_REVAA };
static void Resonx_postprocessing_revareva(Resonx *self) { POST_PROCESSING_REVAREVA };

static void
Resonx_setProcMode(Resonx *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Resonx_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = Resonx_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = Resonx_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = Resonx_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Resonx_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Resonx_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Resonx_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Resonx_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Resonx_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Resonx_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Resonx_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Resonx_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Resonx_postprocessing_revareva;
            break;
    }
}

static void
Resonx_compute_next_data_frame(Resonx *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Resonx_traverse(Resonx *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    return 0;
}

static int
Resonx_clear(Resonx *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    return 0;
}

static void
Resonx_dealloc(Resonx* self)
{
    pyo_DEALLOC
    free(self->x1);
    free(self->x2);
    free(self->y1);
    free(self->y2);
    Resonx_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Resonx_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Resonx *self;
    self = (Resonx *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->last_freq = self->last_q = -1.0;
    self->stages = 4;
    self->a = self->b1 = self->b2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;
    self->twopiOverSr = TWOPI / (MYFLT)self->sr;

    Stream_setFunctionPtr(self->stream, Resonx_compute_next_data_frame);
    self->mode_func_ptr = Resonx_setProcMode;

    static char *kwlist[] = {"input", "freq", "q", "stages", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiOO", kwlist, &inputtmp, &freqtmp, &qtmp, &self->stages, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Resonx_allocate_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Resonx_getServer(Resonx* self) { GET_SERVER };
static PyObject * Resonx_getStream(Resonx* self) { GET_STREAM };
static PyObject * Resonx_setMul(Resonx *self, PyObject *arg) { SET_MUL };
static PyObject * Resonx_setAdd(Resonx *self, PyObject *arg) { SET_ADD };
static PyObject * Resonx_setSub(Resonx *self, PyObject *arg) { SET_SUB };
static PyObject * Resonx_setDiv(Resonx *self, PyObject *arg) { SET_DIV };

static PyObject * Resonx_play(Resonx *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Resonx_out(Resonx *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Resonx_stop(Resonx *self) { STOP };

static PyObject * Resonx_multiply(Resonx *self, PyObject *arg) { MULTIPLY };
static PyObject * Resonx_inplace_multiply(Resonx *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Resonx_add(Resonx *self, PyObject *arg) { ADD };
static PyObject * Resonx_inplace_add(Resonx *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Resonx_sub(Resonx *self, PyObject *arg) { SUB };
static PyObject * Resonx_inplace_sub(Resonx *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Resonx_div(Resonx *self, PyObject *arg) { DIV };
static PyObject * Resonx_inplace_div(Resonx *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Resonx_setFreq(Resonx *self, PyObject *arg)
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
Resonx_setQ(Resonx *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Resonx_setStages(Resonx *self, PyObject *arg)
{

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->stages = PyInt_AsLong(arg);
        Resonx_allocate_memories(self);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Resonx_members[] = {
    {"server", T_OBJECT_EX, offsetof(Resonx, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Resonx, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Resonx, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Resonx, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(Resonx, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(Resonx, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Resonx, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Resonx_methods[] = {
    {"getServer", (PyCFunction)Resonx_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Resonx_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Resonx_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Resonx_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Resonx_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Resonx_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)Resonx_setQ, METH_O, "Sets filter Q factor."},
    {"setStages", (PyCFunction)Resonx_setStages, METH_O, "Sets the number of stages of the filter."},
	{"setMul", (PyCFunction)Resonx_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Resonx_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Resonx_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Resonx_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Resonx_as_number = {
    (binaryfunc)Resonx_add,                         /*nb_add*/
    (binaryfunc)Resonx_sub,                         /*nb_subtract*/
    (binaryfunc)Resonx_multiply,                    /*nb_multiply*/
    (binaryfunc)Resonx_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)Resonx_inplace_add,                 /*inplace_add*/
    (binaryfunc)Resonx_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Resonx_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Resonx_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject ResonxType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Resonx_base",                                   /*tp_name*/
    sizeof(Resonx),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Resonx_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Resonx_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Resonx objects. Cascade of second-order Resonant bandpass filter.",           /* tp_doc */
    (traverseproc)Resonx_traverse,                  /* tp_traverse */
    (inquiry)Resonx_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Resonx_methods,                                 /* tp_methods */
    Resonx_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Resonx_new,                                     /* tp_new */
};

/************/
/* ButLP */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
    MYFLT lastFreq;
    MYFLT nyquist;
    MYFLT piOnSr;
    MYFLT sqrt2;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // variables
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
    MYFLT b1;
    MYFLT b2;
} ButLP;

static void
ButLP_filters_i(ButLP *self) {
    MYFLT val, c, c2;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);

    if (fr != self->lastFreq) {
        if (fr <= 1.0)
            fr = 1.0;
        else if (fr >= self->nyquist)
            fr = self->nyquist;
        self->lastFreq = fr;
        c = 1.0 / MYTAN(self->piOnSr * fr);
        c2 = c * c;
        self->a0 = self->a2 = 1.0 / (1.0 + self->sqrt2 * c + c2);
        self->a1 = 2.0 * self->a0;
        self->b1 = self->a1 * (1.0 - c2);
        self->b2 = self->a0 * (1.0 - self->sqrt2 * c + c2);
    }

    for (i=0; i<self->bufsize; i++) {
        val = self->a0 * in[i] + self->a1 * self->x1 + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void
ButLP_filters_a(ButLP *self) {
    MYFLT val, fr, c, c2;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr != self->lastFreq) {
            if (fr <= 1.0)
                fr = 1.0;
            else if (fr >= self->nyquist)
                fr = self->nyquist;
            self->lastFreq = fr;
            c = 1.0 / MYTAN(self->piOnSr * fr);
            c2 = c * c;
            self->a0 = self->a2 = 1.0 / (1.0 + self->sqrt2 * c + c2);
            self->a1 = 2.0 * self->a0;
            self->b1 = self->a1 * (1.0 - c2);
            self->b2 = self->a0 * (1.0 - self->sqrt2 * c + c2);
        }
        val = self->a0 * in[i] + self->a1 * self->x1 + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void ButLP_postprocessing_ii(ButLP *self) { POST_PROCESSING_II };
static void ButLP_postprocessing_ai(ButLP *self) { POST_PROCESSING_AI };
static void ButLP_postprocessing_ia(ButLP *self) { POST_PROCESSING_IA };
static void ButLP_postprocessing_aa(ButLP *self) { POST_PROCESSING_AA };
static void ButLP_postprocessing_ireva(ButLP *self) { POST_PROCESSING_IREVA };
static void ButLP_postprocessing_areva(ButLP *self) { POST_PROCESSING_AREVA };
static void ButLP_postprocessing_revai(ButLP *self) { POST_PROCESSING_REVAI };
static void ButLP_postprocessing_revaa(ButLP *self) { POST_PROCESSING_REVAA };
static void ButLP_postprocessing_revareva(ButLP *self) { POST_PROCESSING_REVAREVA };

static void
ButLP_setProcMode(ButLP *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = ButLP_filters_i;
            break;
        case 1:
            self->proc_func_ptr = ButLP_filters_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = ButLP_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = ButLP_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = ButLP_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = ButLP_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = ButLP_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = ButLP_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = ButLP_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = ButLP_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = ButLP_postprocessing_revareva;
            break;
    }
}

static void
ButLP_compute_next_data_frame(ButLP *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
ButLP_traverse(ButLP *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    return 0;
}

static int
ButLP_clear(ButLP *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    return 0;
}

static void
ButLP_dealloc(ButLP* self)
{
    pyo_DEALLOC
    ButLP_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ButLP_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    ButLP *self;
    self = (ButLP *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->lastFreq = -1.0;
    self->x1 = self->x2 = self->y1 = self->y2 = self->a0 = self->a1 = self->a2 = self->b1 = self->b2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;
    self->piOnSr = PI / (MYFLT)self->sr;
    self->sqrt2 = MYSQRT(2.0);

    Stream_setFunctionPtr(self->stream, ButLP_compute_next_data_frame);
    self->mode_func_ptr = ButLP_setProcMode;

    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * ButLP_getServer(ButLP* self) { GET_SERVER };
static PyObject * ButLP_getStream(ButLP* self) { GET_STREAM };
static PyObject * ButLP_setMul(ButLP *self, PyObject *arg) { SET_MUL };
static PyObject * ButLP_setAdd(ButLP *self, PyObject *arg) { SET_ADD };
static PyObject * ButLP_setSub(ButLP *self, PyObject *arg) { SET_SUB };
static PyObject * ButLP_setDiv(ButLP *self, PyObject *arg) { SET_DIV };

static PyObject * ButLP_play(ButLP *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * ButLP_out(ButLP *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * ButLP_stop(ButLP *self) { STOP };

static PyObject * ButLP_multiply(ButLP *self, PyObject *arg) { MULTIPLY };
static PyObject * ButLP_inplace_multiply(ButLP *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ButLP_add(ButLP *self, PyObject *arg) { ADD };
static PyObject * ButLP_inplace_add(ButLP *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ButLP_sub(ButLP *self, PyObject *arg) { SUB };
static PyObject * ButLP_inplace_sub(ButLP *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ButLP_div(ButLP *self, PyObject *arg) { DIV };
static PyObject * ButLP_inplace_div(ButLP *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ButLP_setFreq(ButLP *self, PyObject *arg)
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

static PyMemberDef ButLP_members[] = {
{"server", T_OBJECT_EX, offsetof(ButLP, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(ButLP, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(ButLP, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(ButLP, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(ButLP, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(ButLP, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef ButLP_methods[] = {
{"getServer", (PyCFunction)ButLP_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)ButLP_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)ButLP_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)ButLP_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)ButLP_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)ButLP_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)ButLP_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)ButLP_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)ButLP_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)ButLP_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods ButLP_as_number = {
(binaryfunc)ButLP_add,                         /*nb_add*/
(binaryfunc)ButLP_sub,                         /*nb_subtract*/
(binaryfunc)ButLP_multiply,                    /*nb_multiply*/
(binaryfunc)ButLP_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)ButLP_inplace_add,                 /*inplace_add*/
(binaryfunc)ButLP_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)ButLP_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)ButLP_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ButLPType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.ButLP_base",                                   /*tp_name*/
sizeof(ButLP),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)ButLP_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&ButLP_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"ButLP objects. Second-order Butterworth lowpass filter.",           /* tp_doc */
(traverseproc)ButLP_traverse,                  /* tp_traverse */
(inquiry)ButLP_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
ButLP_methods,                                 /* tp_methods */
ButLP_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
ButLP_new,                                     /* tp_new */
};

/************/
/* ButHP */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
    MYFLT lastFreq;
    MYFLT nyquist;
    MYFLT piOnSr;
    MYFLT sqrt2;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // variables
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
    MYFLT b1;
    MYFLT b2;
} ButHP;

static void
ButHP_filters_i(ButHP *self) {
    MYFLT val, c, c2;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);

    if (fr != self->lastFreq) {
        if (fr <= 1.0)
            fr = 1.0;
        else if (fr >= self->nyquist)
            fr = self->nyquist;
        self->lastFreq = fr;
        c = MYTAN(self->piOnSr * fr);
        c2 = c * c;
        self->a0 = self->a2 = 1.0 / (1.0 + self->sqrt2 * c + c2);
        self->a1 = -2.0 * self->a0;
        self->b1 = 2.0 * self->a0 * (c2 - 1.0);
        self->b2 = self->a0 * (1.0 - self->sqrt2 * c + c2);
    }

    for (i=0; i<self->bufsize; i++) {
        val = self->a0 * in[i] + self->a1 * self->x1 + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void
ButHP_filters_a(ButHP *self) {
    MYFLT val, fr, c, c2;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr != self->lastFreq) {
            if (fr <= 1.0)
                fr = 1.0;
            else if (fr >= self->nyquist)
                fr = self->nyquist;
            self->lastFreq = fr;
            c = MYTAN(self->piOnSr * fr);
            c2 = c * c;
            self->a0 = self->a2 = 1.0 / (1.0 + self->sqrt2 * c + c2);
            self->a1 = -2.0 * self->a0;
            self->b1 = 2.0 * self->a0 * (c2 - 1.0);
            self->b2 = self->a0 * (1.0 - self->sqrt2 * c + c2);
        }
        val = self->a0 * in[i] + self->a1 * self->x1 + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void ButHP_postprocessing_ii(ButHP *self) { POST_PROCESSING_II };
static void ButHP_postprocessing_ai(ButHP *self) { POST_PROCESSING_AI };
static void ButHP_postprocessing_ia(ButHP *self) { POST_PROCESSING_IA };
static void ButHP_postprocessing_aa(ButHP *self) { POST_PROCESSING_AA };
static void ButHP_postprocessing_ireva(ButHP *self) { POST_PROCESSING_IREVA };
static void ButHP_postprocessing_areva(ButHP *self) { POST_PROCESSING_AREVA };
static void ButHP_postprocessing_revai(ButHP *self) { POST_PROCESSING_REVAI };
static void ButHP_postprocessing_revaa(ButHP *self) { POST_PROCESSING_REVAA };
static void ButHP_postprocessing_revareva(ButHP *self) { POST_PROCESSING_REVAREVA };

static void
ButHP_setProcMode(ButHP *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = ButHP_filters_i;
            break;
        case 1:
            self->proc_func_ptr = ButHP_filters_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = ButHP_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = ButHP_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = ButHP_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = ButHP_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = ButHP_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = ButHP_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = ButHP_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = ButHP_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = ButHP_postprocessing_revareva;
            break;
    }
}

static void
ButHP_compute_next_data_frame(ButHP *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
ButHP_traverse(ButHP *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    return 0;
}

static int
ButHP_clear(ButHP *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    return 0;
}

static void
ButHP_dealloc(ButHP* self)
{
    pyo_DEALLOC
    ButHP_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ButHP_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    ButHP *self;
    self = (ButHP *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->lastFreq = -1.0;
    self->x1 = self->x2 = self->y1 = self->y2 = self->a0 = self->a1 = self->a2 = self->b1 = self->b2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;
    self->piOnSr = PI / (MYFLT)self->sr;
    self->sqrt2 = MYSQRT(2.0);

    Stream_setFunctionPtr(self->stream, ButHP_compute_next_data_frame);
    self->mode_func_ptr = ButHP_setProcMode;

    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * ButHP_getServer(ButHP* self) { GET_SERVER };
static PyObject * ButHP_getStream(ButHP* self) { GET_STREAM };
static PyObject * ButHP_setMul(ButHP *self, PyObject *arg) { SET_MUL };
static PyObject * ButHP_setAdd(ButHP *self, PyObject *arg) { SET_ADD };
static PyObject * ButHP_setSub(ButHP *self, PyObject *arg) { SET_SUB };
static PyObject * ButHP_setDiv(ButHP *self, PyObject *arg) { SET_DIV };

static PyObject * ButHP_play(ButHP *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * ButHP_out(ButHP *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * ButHP_stop(ButHP *self) { STOP };

static PyObject * ButHP_multiply(ButHP *self, PyObject *arg) { MULTIPLY };
static PyObject * ButHP_inplace_multiply(ButHP *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ButHP_add(ButHP *self, PyObject *arg) { ADD };
static PyObject * ButHP_inplace_add(ButHP *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ButHP_sub(ButHP *self, PyObject *arg) { SUB };
static PyObject * ButHP_inplace_sub(ButHP *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ButHP_div(ButHP *self, PyObject *arg) { DIV };
static PyObject * ButHP_inplace_div(ButHP *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ButHP_setFreq(ButHP *self, PyObject *arg)
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

static PyMemberDef ButHP_members[] = {
{"server", T_OBJECT_EX, offsetof(ButHP, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(ButHP, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(ButHP, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(ButHP, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(ButHP, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(ButHP, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef ButHP_methods[] = {
{"getServer", (PyCFunction)ButHP_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)ButHP_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)ButHP_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)ButHP_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)ButHP_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)ButHP_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)ButHP_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)ButHP_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)ButHP_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)ButHP_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods ButHP_as_number = {
(binaryfunc)ButHP_add,                         /*nb_add*/
(binaryfunc)ButHP_sub,                         /*nb_subtract*/
(binaryfunc)ButHP_multiply,                    /*nb_multiply*/
(binaryfunc)ButHP_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)ButHP_inplace_add,                 /*inplace_add*/
(binaryfunc)ButHP_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)ButHP_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)ButHP_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ButHPType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.ButHP_base",                                   /*tp_name*/
sizeof(ButHP),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)ButHP_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&ButHP_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"ButHP objects. Second-order Butterworth highpass filter.",           /* tp_doc */
(traverseproc)ButHP_traverse,                  /* tp_traverse */
(inquiry)ButHP_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
ButHP_methods,                                 /* tp_methods */
ButHP_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
ButHP_new,                                     /* tp_new */
};

/************/
/* ButBP */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT nyquist;
    MYFLT last_freq;
    MYFLT last_q;
    MYFLT piOnSr;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // coefficients
    MYFLT a0;
    MYFLT a2;
    MYFLT b1;
    MYFLT b2;
} ButBP;

static void
ButBP_compute_coeffs(ButBP *self, MYFLT freq, MYFLT q)
{
    MYFLT bw, c, d;

    if (freq < 1.0)
        freq = 1.0;
    else if (freq > self->nyquist)
        freq = self->nyquist;
    if (q < 1.0)
        q = 1.0;

    bw = freq / q;
    c = 1.0 / MYTAN(self->piOnSr * bw);
    d = 2.0 * MYCOS(2.0 * self->piOnSr * freq);

    self->a0 = 1.0 / (1.0 + c);
    self->a2 = -self->a0;
    self->b1 = self->a2 * c * d;
    self->b2 = self->a0 * (c - 1.0);
}

static void
ButBP_filters_ii(ButBP *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    fr = PyFloat_AS_DOUBLE(self->freq);
    q = PyFloat_AS_DOUBLE(self->q);

    if (fr != self->last_freq || q != self->last_q) {
        self->last_freq = fr;
        self->last_q = q;
        ButBP_compute_coeffs(self, fr, q);
    }

    for (i=0; i<self->bufsize; i++) {
        val = self->a0 * in[i] + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void
ButBP_filters_ai(ButBP *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            ButBP_compute_coeffs(self, fr, q);
        }
        val = self->a0 * in[i] + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void
ButBP_filters_ia(ButBP *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        q = qst[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            ButBP_compute_coeffs(self, fr, q);
        }
        val = self->a0 * in[i] + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void
ButBP_filters_aa(ButBP *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        q = qst[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            ButBP_compute_coeffs(self, fr, q);
        }
        val = self->a0 * in[i] + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void ButBP_postprocessing_ii(ButBP *self) { POST_PROCESSING_II };
static void ButBP_postprocessing_ai(ButBP *self) { POST_PROCESSING_AI };
static void ButBP_postprocessing_ia(ButBP *self) { POST_PROCESSING_IA };
static void ButBP_postprocessing_aa(ButBP *self) { POST_PROCESSING_AA };
static void ButBP_postprocessing_ireva(ButBP *self) { POST_PROCESSING_IREVA };
static void ButBP_postprocessing_areva(ButBP *self) { POST_PROCESSING_AREVA };
static void ButBP_postprocessing_revai(ButBP *self) { POST_PROCESSING_REVAI };
static void ButBP_postprocessing_revaa(ButBP *self) { POST_PROCESSING_REVAA };
static void ButBP_postprocessing_revareva(ButBP *self) { POST_PROCESSING_REVAREVA };

static void
ButBP_setProcMode(ButBP *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = ButBP_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = ButBP_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = ButBP_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = ButBP_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = ButBP_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = ButBP_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = ButBP_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = ButBP_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = ButBP_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = ButBP_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = ButBP_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = ButBP_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = ButBP_postprocessing_revareva;
            break;
    }
}

static void
ButBP_compute_next_data_frame(ButBP *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
ButBP_traverse(ButBP *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    return 0;
}

static int
ButBP_clear(ButBP *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    return 0;
}

static void
ButBP_dealloc(ButBP* self)
{
    pyo_DEALLOC
    ButBP_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ButBP_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;
    ButBP *self;
    self = (ButBP *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->last_freq = self->last_q = -1.0;
    self->x1 = self->x2 = self->y1 = self->y2 = 0.0;
    self->a0 = self->a2 = self->b1 = self->b2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;
    self->piOnSr = PI / (MYFLT)self->sr;

    Stream_setFunctionPtr(self->stream, ButBP_compute_next_data_frame);
    self->mode_func_ptr = ButBP_setProcMode;

    static char *kwlist[] = {"input", "freq", "q", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &freqtmp, &qtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * ButBP_getServer(ButBP* self) { GET_SERVER };
static PyObject * ButBP_getStream(ButBP* self) { GET_STREAM };
static PyObject * ButBP_setMul(ButBP *self, PyObject *arg) { SET_MUL };
static PyObject * ButBP_setAdd(ButBP *self, PyObject *arg) { SET_ADD };
static PyObject * ButBP_setSub(ButBP *self, PyObject *arg) { SET_SUB };
static PyObject * ButBP_setDiv(ButBP *self, PyObject *arg) { SET_DIV };

static PyObject * ButBP_play(ButBP *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * ButBP_out(ButBP *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * ButBP_stop(ButBP *self) { STOP };

static PyObject * ButBP_multiply(ButBP *self, PyObject *arg) { MULTIPLY };
static PyObject * ButBP_inplace_multiply(ButBP *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ButBP_add(ButBP *self, PyObject *arg) { ADD };
static PyObject * ButBP_inplace_add(ButBP *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ButBP_sub(ButBP *self, PyObject *arg) { SUB };
static PyObject * ButBP_inplace_sub(ButBP *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ButBP_div(ButBP *self, PyObject *arg) { DIV };
static PyObject * ButBP_inplace_div(ButBP *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ButBP_setFreq(ButBP *self, PyObject *arg)
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
ButBP_setQ(ButBP *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef ButBP_members[] = {
    {"server", T_OBJECT_EX, offsetof(ButBP, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(ButBP, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(ButBP, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(ButBP, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(ButBP, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(ButBP, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(ButBP, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef ButBP_methods[] = {
    {"getServer", (PyCFunction)ButBP_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)ButBP_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)ButBP_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)ButBP_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)ButBP_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)ButBP_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)ButBP_setQ, METH_O, "Sets filter Q factor."},
	{"setMul", (PyCFunction)ButBP_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)ButBP_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)ButBP_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)ButBP_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods ButBP_as_number = {
    (binaryfunc)ButBP_add,                         /*nb_add*/
    (binaryfunc)ButBP_sub,                         /*nb_subtract*/
    (binaryfunc)ButBP_multiply,                    /*nb_multiply*/
    (binaryfunc)ButBP_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)ButBP_inplace_add,                 /*inplace_add*/
    (binaryfunc)ButBP_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)ButBP_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)ButBP_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject ButBPType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.ButBP_base",                                   /*tp_name*/
    sizeof(ButBP),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)ButBP_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &ButBP_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "ButBP objects. Second-order Butterworth bandpass filter.",           /* tp_doc */
    (traverseproc)ButBP_traverse,                  /* tp_traverse */
    (inquiry)ButBP_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    ButBP_methods,                                 /* tp_methods */
    ButBP_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    ButBP_new,                                     /* tp_new */
};

/************/
/* ButBR */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT nyquist;
    MYFLT last_freq;
    MYFLT last_q;
    MYFLT piOnSr;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // coefficients
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
    MYFLT b1;
    MYFLT b2;
} ButBR;

static void
ButBR_compute_coeffs(ButBR *self, MYFLT freq, MYFLT q)
{
    MYFLT bw, c, d;

    if (freq < 1.0)
        freq = 1.0;
    else if (freq > self->nyquist)
        freq = self->nyquist;
    if (q < 1.0)
        q = 1.0;

    bw = freq / q;
    c = MYTAN(self->piOnSr * bw);
    d = 2.0 * MYCOS(2.0 * self->piOnSr * freq);

    self->a0 = self->a2 = 1.0 / (1.0 + c);
    self->a1 = self->b1 = -self->a0 * d;
    self->b2 = self->a0 * (1.0 - c);
}

static void
ButBR_filters_ii(ButBR *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    fr = PyFloat_AS_DOUBLE(self->freq);
    q = PyFloat_AS_DOUBLE(self->q);

    if (fr != self->last_freq || q != self->last_q) {
        self->last_freq = fr;
        self->last_q = q;
        ButBR_compute_coeffs(self, fr, q);
    }

    for (i=0; i<self->bufsize; i++) {
        val = self->a0 * in[i] + self->a1 * self->x1 + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void
ButBR_filters_ai(ButBR *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            ButBR_compute_coeffs(self, fr, q);
        }
        val = self->a0 * in[i] + self->a1 * self->x1 + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void
ButBR_filters_ia(ButBR *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        q = qst[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            ButBR_compute_coeffs(self, fr, q);
        }
        val = self->a0 * in[i] + self->a1 * self->x1 + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void
ButBR_filters_aa(ButBR *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *qst = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        q = qst[i];
        if (fr != self->last_freq || q != self->last_q) {
            self->last_freq = fr;
            self->last_q = q;
            ButBR_compute_coeffs(self, fr, q);
        }
        val = self->a0 * in[i] + self->a1 * self->x1 + self->a2 * self->x2 - self->b1 * self->y1 - self->b2 * self->y2;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->y2 = self->y1;
        self->data[i] = self->y1 = val;
    }
}

static void ButBR_postprocessing_ii(ButBR *self) { POST_PROCESSING_II };
static void ButBR_postprocessing_ai(ButBR *self) { POST_PROCESSING_AI };
static void ButBR_postprocessing_ia(ButBR *self) { POST_PROCESSING_IA };
static void ButBR_postprocessing_aa(ButBR *self) { POST_PROCESSING_AA };
static void ButBR_postprocessing_ireva(ButBR *self) { POST_PROCESSING_IREVA };
static void ButBR_postprocessing_areva(ButBR *self) { POST_PROCESSING_AREVA };
static void ButBR_postprocessing_revai(ButBR *self) { POST_PROCESSING_REVAI };
static void ButBR_postprocessing_revaa(ButBR *self) { POST_PROCESSING_REVAA };
static void ButBR_postprocessing_revareva(ButBR *self) { POST_PROCESSING_REVAREVA };

static void
ButBR_setProcMode(ButBR *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = ButBR_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = ButBR_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = ButBR_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = ButBR_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = ButBR_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = ButBR_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = ButBR_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = ButBR_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = ButBR_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = ButBR_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = ButBR_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = ButBR_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = ButBR_postprocessing_revareva;
            break;
    }
}

static void
ButBR_compute_next_data_frame(ButBR *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
ButBR_traverse(ButBR *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    return 0;
}

static int
ButBR_clear(ButBR *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    return 0;
}

static void
ButBR_dealloc(ButBR* self)
{
    pyo_DEALLOC
    ButBR_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ButBR_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;
    ButBR *self;
    self = (ButBR *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->last_freq = self->last_q = -1.0;
    self->x1 = self->x2 = self->y1 = self->y2 = 0.0;
    self->a0 = self->a1 = self->a2 = self->b1 = self->b2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.49;
    self->piOnSr = PI / (MYFLT)self->sr;

    Stream_setFunctionPtr(self->stream, ButBR_compute_next_data_frame);
    self->mode_func_ptr = ButBR_setProcMode;

    static char *kwlist[] = {"input", "freq", "q", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &freqtmp, &qtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * ButBR_getServer(ButBR* self) { GET_SERVER };
static PyObject * ButBR_getStream(ButBR* self) { GET_STREAM };
static PyObject * ButBR_setMul(ButBR *self, PyObject *arg) { SET_MUL };
static PyObject * ButBR_setAdd(ButBR *self, PyObject *arg) { SET_ADD };
static PyObject * ButBR_setSub(ButBR *self, PyObject *arg) { SET_SUB };
static PyObject * ButBR_setDiv(ButBR *self, PyObject *arg) { SET_DIV };

static PyObject * ButBR_play(ButBR *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * ButBR_out(ButBR *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * ButBR_stop(ButBR *self) { STOP };

static PyObject * ButBR_multiply(ButBR *self, PyObject *arg) { MULTIPLY };
static PyObject * ButBR_inplace_multiply(ButBR *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ButBR_add(ButBR *self, PyObject *arg) { ADD };
static PyObject * ButBR_inplace_add(ButBR *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ButBR_sub(ButBR *self, PyObject *arg) { SUB };
static PyObject * ButBR_inplace_sub(ButBR *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ButBR_div(ButBR *self, PyObject *arg) { DIV };
static PyObject * ButBR_inplace_div(ButBR *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ButBR_setFreq(ButBR *self, PyObject *arg)
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
ButBR_setQ(ButBR *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef ButBR_members[] = {
    {"server", T_OBJECT_EX, offsetof(ButBR, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(ButBR, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(ButBR, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(ButBR, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(ButBR, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(ButBR, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(ButBR, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef ButBR_methods[] = {
    {"getServer", (PyCFunction)ButBR_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)ButBR_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)ButBR_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)ButBR_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)ButBR_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)ButBR_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)ButBR_setQ, METH_O, "Sets filter Q factor."},
	{"setMul", (PyCFunction)ButBR_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)ButBR_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)ButBR_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)ButBR_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods ButBR_as_number = {
    (binaryfunc)ButBR_add,                         /*nb_add*/
    (binaryfunc)ButBR_sub,                         /*nb_subtract*/
    (binaryfunc)ButBR_multiply,                    /*nb_multiply*/
    (binaryfunc)ButBR_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)ButBR_inplace_add,                 /*inplace_add*/
    (binaryfunc)ButBR_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)ButBR_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)ButBR_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject ButBRType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.ButBR_base",                                   /*tp_name*/
    sizeof(ButBR),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)ButBR_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &ButBR_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "ButBR objects. Second-order Butterworth band-reject filter.",           /* tp_doc */
    (traverseproc)ButBR_traverse,                  /* tp_traverse */
    (inquiry)ButBR_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    ButBR_methods,                                 /* tp_methods */
    ButBR_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    ButBR_new,                                     /* tp_new */
};

/****************/
/** ComplexRes **/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *decay;
    Stream *decay_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT last_freq;
    MYFLT last_decay;
    MYFLT oneOnSr;
    // variables
    MYFLT res;
    MYFLT norm;
    MYFLT coeffx;
    MYFLT coeffy;
    // sample memories
    MYFLT x;
    MYFLT y;
} ComplexRes;

static void
ComplexRes_filters_ii(ComplexRes *self) {
    int i;
    MYFLT ang, x, y;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT decay = PyFloat_AS_DOUBLE(self->decay);

    if (decay <= 0.0001)
        decay = 0.0001;

    if (decay != self->last_decay || freq != self->last_freq) {
        self->res = MYEXP(-1.0/(decay*self->sr));
        //self->norm = (1.0-self->res*self->res)/self->res;
        self->last_decay = decay;
        ang = (freq*self->oneOnSr)*TWOPI;
        self->coeffx = self->res * MYCOS(ang);
        self->coeffy = self->res * MYSIN(ang);
        self->last_freq = freq;
    }

    for (i=0; i<self->bufsize; i++) {
        x = self->coeffx * self->x - self->coeffy * self->y + in[i];
        y = self->coeffy * self->x + self->coeffx * self->y;
        self->data[i] = y * self->norm;
        self->x = x;
        self->y = y;
    }
}

static void
ComplexRes_filters_ai(ComplexRes *self) {
    int i, check = 0;
    MYFLT freq, ang, x, y;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT decay = PyFloat_AS_DOUBLE(self->decay);

    if (decay <= 0.0001)
        decay = 0.0001;

    if (decay != self->last_decay) {
        self->res = MYEXP(-1.0/(decay*self->sr));
        //self->norm = (1.0-self->res*self->res)/self->res;
        self->last_decay = decay;
        check = 1;
    }

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq != self->last_freq || check) {
            ang = (freq*self->oneOnSr)*TWOPI;
            self->coeffx = self->res * MYCOS(ang);
            self->coeffy = self->res * MYSIN(ang);
            self->last_freq = freq;
            check = 0;
        }
        x = self->coeffx * self->x - self->coeffy * self->y + in[i];
        y = self->coeffy * self->x + self->coeffx * self->y;
        self->data[i] = y * self->norm;
        self->x = x;
        self->y = y;
    }
}

static void
ComplexRes_filters_ia(ComplexRes *self) {
    int i;
    MYFLT decay, ang, x, y;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *dec = Stream_getData((Stream *)self->decay_stream);

    for (i=0; i<self->bufsize; i++) {
        decay = dec[i];
        if (decay <= 0.0001)
            decay = 0.0001;
        if (freq != self->last_freq || decay != self->last_decay) {
            self->res = MYEXP(-1.0/(decay*self->sr));
            //self->norm = (1.0-self->res*self->res)/self->res;
            self->last_decay = decay;
            ang = (freq*self->oneOnSr)*TWOPI;
            self->coeffx = self->res * MYCOS(ang);
            self->coeffy = self->res * MYSIN(ang);
            self->last_freq = freq;
        }
        x = self->coeffx * self->x - self->coeffy * self->y + in[i];
        y = self->coeffy * self->x + self->coeffx * self->y;
        self->data[i] = y * self->norm;
        self->x = x;
        self->y = y;
    }
}

static void
ComplexRes_filters_aa(ComplexRes *self) {
    int i;
    MYFLT freq, decay, ang, x, y;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *dec = Stream_getData((Stream *)self->decay_stream);

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        decay = dec[i];
        if (decay <= 0.0001)
            decay = 0.0001;
        if (freq != self->last_freq || decay != self->last_decay) {
            self->res = MYEXP(-1.0/(decay*self->sr));
            //self->norm = (1.0-self->res*self->res)/self->res;
            self->last_decay = decay;
            ang = (freq*self->oneOnSr)*TWOPI;
            self->coeffx = self->res * MYCOS(ang);
            self->coeffy = self->res * MYSIN(ang);
            self->last_freq = freq;
        }
        x = self->coeffx * self->x - self->coeffy * self->y + in[i];
        y = self->coeffy * self->x + self->coeffx * self->y;
        self->data[i] = y * self->norm;
        self->x = x;
        self->y = y;
    }
}

static void ComplexRes_postprocessing_ii(ComplexRes *self) { POST_PROCESSING_II };
static void ComplexRes_postprocessing_ai(ComplexRes *self) { POST_PROCESSING_AI };
static void ComplexRes_postprocessing_ia(ComplexRes *self) { POST_PROCESSING_IA };
static void ComplexRes_postprocessing_aa(ComplexRes *self) { POST_PROCESSING_AA };
static void ComplexRes_postprocessing_ireva(ComplexRes *self) { POST_PROCESSING_IREVA };
static void ComplexRes_postprocessing_areva(ComplexRes *self) { POST_PROCESSING_AREVA };
static void ComplexRes_postprocessing_revai(ComplexRes *self) { POST_PROCESSING_REVAI };
static void ComplexRes_postprocessing_revaa(ComplexRes *self) { POST_PROCESSING_REVAA };
static void ComplexRes_postprocessing_revareva(ComplexRes *self) { POST_PROCESSING_REVAREVA };

static void
ComplexRes_setProcMode(ComplexRes *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = ComplexRes_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = ComplexRes_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = ComplexRes_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = ComplexRes_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = ComplexRes_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = ComplexRes_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = ComplexRes_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = ComplexRes_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = ComplexRes_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = ComplexRes_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = ComplexRes_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = ComplexRes_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = ComplexRes_postprocessing_revareva;
            break;
    }
}

static void
ComplexRes_compute_next_data_frame(ComplexRes *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
ComplexRes_traverse(ComplexRes *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->decay);
    Py_VISIT(self->decay_stream);
    return 0;
}

static int
ComplexRes_clear(ComplexRes *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->decay);
    Py_CLEAR(self->decay_stream);
    return 0;
}

static void
ComplexRes_dealloc(ComplexRes* self)
{
    pyo_DEALLOC
    ComplexRes_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ComplexRes_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *decaytmp=NULL, *multmp=NULL, *addtmp=NULL;
    ComplexRes *self;
    self = (ComplexRes *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
    self->decay = PyFloat_FromDouble(.25);
    self->last_freq = self->last_decay = -1.0;
    self->x = self->y = 0.0;
    self->res = 1.0;
    self->norm = 0.01; /* normalization factor fixed at -40 dB */
    self->coeffx = self->coeffy = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->oneOnSr = 1.0 / self->sr;

    Stream_setFunctionPtr(self->stream, ComplexRes_compute_next_data_frame);
    self->mode_func_ptr = ComplexRes_setProcMode;

    static char *kwlist[] = {"input", "freq", "decay", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &freqtmp, &decaytmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (decaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDecay", "O", decaytmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * ComplexRes_getServer(ComplexRes* self) { GET_SERVER };
static PyObject * ComplexRes_getStream(ComplexRes* self) { GET_STREAM };
static PyObject * ComplexRes_setMul(ComplexRes *self, PyObject *arg) { SET_MUL };
static PyObject * ComplexRes_setAdd(ComplexRes *self, PyObject *arg) { SET_ADD };
static PyObject * ComplexRes_setSub(ComplexRes *self, PyObject *arg) { SET_SUB };
static PyObject * ComplexRes_setDiv(ComplexRes *self, PyObject *arg) { SET_DIV };

static PyObject * ComplexRes_play(ComplexRes *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * ComplexRes_out(ComplexRes *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * ComplexRes_stop(ComplexRes *self) { STOP };

static PyObject * ComplexRes_multiply(ComplexRes *self, PyObject *arg) { MULTIPLY };
static PyObject * ComplexRes_inplace_multiply(ComplexRes *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ComplexRes_add(ComplexRes *self, PyObject *arg) { ADD };
static PyObject * ComplexRes_inplace_add(ComplexRes *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ComplexRes_sub(ComplexRes *self, PyObject *arg) { SUB };
static PyObject * ComplexRes_inplace_sub(ComplexRes *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ComplexRes_div(ComplexRes *self, PyObject *arg) { DIV };
static PyObject * ComplexRes_inplace_div(ComplexRes *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ComplexRes_setFreq(ComplexRes *self, PyObject *arg)
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
ComplexRes_setDecay(ComplexRes *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->decay);
	if (isNumber == 1) {
		self->decay = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->decay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->decay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->decay_stream);
        self->decay_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef ComplexRes_members[] = {
{"server", T_OBJECT_EX, offsetof(ComplexRes, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(ComplexRes, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(ComplexRes, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(ComplexRes, freq), 0, "Center frequency in cycle per second."},
{"decay", T_OBJECT_EX, offsetof(ComplexRes, decay), 0, "Decaying envelope time in seconds."},
{"mul", T_OBJECT_EX, offsetof(ComplexRes, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(ComplexRes, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef ComplexRes_methods[] = {
{"getServer", (PyCFunction)ComplexRes_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)ComplexRes_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)ComplexRes_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)ComplexRes_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)ComplexRes_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)ComplexRes_setFreq, METH_O, "Sets filter center frequency in cycle per second."},
{"setDecay", (PyCFunction)ComplexRes_setDecay, METH_O, "Sets filter decaying envelope time."},
{"setMul", (PyCFunction)ComplexRes_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)ComplexRes_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)ComplexRes_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)ComplexRes_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods ComplexRes_as_number = {
(binaryfunc)ComplexRes_add,                         /*nb_add*/
(binaryfunc)ComplexRes_sub,                         /*nb_subtract*/
(binaryfunc)ComplexRes_multiply,                    /*nb_multiply*/
(binaryfunc)ComplexRes_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)ComplexRes_inplace_add,                 /*inplace_add*/
(binaryfunc)ComplexRes_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)ComplexRes_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)ComplexRes_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ComplexResType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.ComplexRes_base",                                   /*tp_name*/
sizeof(ComplexRes),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)ComplexRes_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&ComplexRes_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"ComplexRes objects. Second order allpass filter.",           /* tp_doc */
(traverseproc)ComplexRes_traverse,                  /* tp_traverse */
(inquiry)ComplexRes_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
ComplexRes_methods,                                 /* tp_methods */
ComplexRes_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
ComplexRes_new,                                     /* tp_new */
};