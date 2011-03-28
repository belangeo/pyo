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

MYFLT ENV_ARRAY[513] = {0.0, 0.0625, 0.088388347648318447, 0.10825317547305482, 0.125, 0.13975424859373686, 0.15309310892394862, 0.16535945694153692, 0.17677669529663689, 0.1875, 0.19764235376052372, 0.20728904939721249, 0.21650635094610965, 0.22534695471649932, 0.23385358667337133, 0.24206145913796356, 0.25, 0.25769410160110379, 0.2651650429449553, 0.27243118397129212, 0.27950849718747373, 0.28641098093473999, 0.29315098498896436, 0.29973947020704494, 0.30618621784789724, 0.3125, 0.31868871959954903, 0.3247595264191645, 0.33071891388307384, 0.33657280044590648, 0.34232659844072882, 0.34798527267687634, 0.35355339059327379, 0.35903516540862679, 0.3644344934278313, 0.36975498644372601, 0.375, 0.38017265814363871, 0.385275875185561, 0.39031237489989989, 0.39528470752104744, 0.40019526483955303, 0.40504629365049127, 0.40983990776887502, 0.41457809879442498, 0.41926274578121059, 0.42389562394532926, 0.42847841252506524, 0.4330127018922193, 0.4375, 0.44194173824159222, 0.44633927678392815, 0.45069390943299864, 0.45500686808003238, 0.45927932677184591, 0.46351240544347894, 0.46770717334674267, 0.47186465220442186, 0.47598581911649429, 0.48007160924178799, 0.48412291827592713, 0.48814060474416587, 0.4921254921257382, 0.49607837082461076, 0.5, 0.50389110926865932, 0.50775240028974755, 0.51158454824202815, 0.51538820320220757, 0.51916399143237968, 0.52291251658379723, 0.52663436082352244, 0.5303300858899106, 0.53400023408234565, 0.53764532919016417, 0.54126587736527421, 0.54486236794258425, 0.54843527421200766, 0.55198505414549048, 0.55551215108222429, 0.55901699437494745, 0.5625, 0.56596157113358858, 0.5694020986965187, 0.57282196186947998, 0.57622152858080544, 0.5796011559684815, 0.58296119081805098, 0.58630196997792872, 0.5896238207535377, 0.59292706128157113, 0.59621200088559101, 0.59947894041408989, 0.60272817256205968, 0.60595998217704117, 0.60917464655056019, 0.61237243569579447, 0.6155536126122565, 0.61871843353822908, 0.62186714819163746, 0.625, 0.62811722632005562, 0.63121905864762984, 0.6343057228182637, 0.63737743919909806, 0.64043442287247487, 0.64347688381168755, 0.64650502704928758, 0.649519052838329, 0.65251915680690942, 0.65550553010634471, 0.65847835955329614, 0.66143782776614768, 0.66438411329591562, 0.66731739075195695, 0.67023783092272549, 0.67314560089181297, 0.67604086414949804, 0.67892378070001347, 0.68179450716473211, 0.68465319688145765, 0.6875, 0.69033506357420382, 0.69315853165058861, 0.69597054535375269, 0.69877124296868431, 0.70156076002011403, 0.70433922934904025, 0.70710678118654757, 0.7098635432250342, 0.7126096406869612, 0.71534519639122485, 0.71807033081725358, 0.72078516216692479, 0.72348980642438909, 0.72618437741389064, 0.72886898685566259, 0.7315437444199766, 0.73420875777942063, 0.7368641326594747, 0.73950997288745202, 0.7421463804398698, 0.74477345548831153, 0.74739129644383739, 0.75, 0.75259966117451849, 0.75519037334966077, 0.7577722283113838, 0.76034531628727742, 0.76290972598335638, 0.76546554461974314, 0.76801285796528174, 0.770551750371122, 0.7730823048033113, 0.77560460287442856, 0.77811872487429579, 0.78062474979979979, 0.78312275538385423, 0.78561281812353345, 0.78809501330740572, 0.79056941504209488, 0.79303609627809502, 0.79549512883486595, 0.7979465834252315, 0.80039052967910607, 0.80282703616657058, 0.80525617042032038, 0.80767799895750536, 0.81009258730098255, 0.8125, 0.81490030065033114, 0.81729355191387631, 0.81967981553775004, 0.82205915237286908, 0.82443162239205747, 0.82679728470768454, 0.82915619758884995, 0.8315084184781294, 0.83385400400789589, 0.83619301001622826, 0.83852549156242118, 0.84085150294210687, 0.8431710977020026, 0.84548432865429268, 0.84779124789065852, 0.85009190679596525, 0.85238635606161595, 0.85467464569858398, 0.85695682505013049, 0.85923294280422002, 0.86150304700563884, 0.86376718506782835, 0.8660254037844386, 0.86827774934061275, 0.87052426732400745, 0.87276500273555879, 0.875, 0.87722930297613744, 0.87945295496689302, 0.88167099872911781, 0.88388347648318444, 0.88609042992236409, 0.88829190022199345, 0.8904879280484379, 0.8926785535678563, 0.89486381645477209, 0.89704375590045771, 0.89921841062113494, 0.90138781886599728, 0.90355201842506006, 0.90571104663683988, 0.90786494039587184, 0.91001373616006476, 0.91215746995790148, 0.91429617739548708, 0.91642989366344874, 0.91855865354369182, 0.92068249141601466, 0.92280144126458752, 0.92491553668429638, 0.92702481088695787, 0.92912929670740663, 0.93122902660945872, 0.9333240326917549, 0.93541434669348533, 0.9375, 0.93958102364830676, 0.94165744833246023, 0.94372930440884373, 0.94579662190134728, 0.94785943050644383, 0.94991775959816649, 0.95197163823298858, 0.9540210951546092, 0.9560661587986472, 0.95810685729724321, 0.96014321848357598, 0.96217526989629076, 0.9642030387838445, 0.96622655210876918, 0.96824583655185426, 0.97026091851625151, 0.97227182413150282, 0.9742785792574935, 0.97628120948833175, 0.97827974015615804, 0.98027419633488266, 0.98226460284385697, 0.98425098425147639, 0.98623336487871871, 0.98821176880261852, 0.9901862198596787, 0.99215674164922152, 0.99412335753667913, 0.99608609065682674, 0.99804496391695696, 1.0, 0.99804496391695696, 0.99608609065682674, 0.99412335753667913, 0.99215674164922152, 0.9901862198596787, 0.98821176880261852, 0.98623336487871871, 0.98425098425147639, 0.98226460284385697, 0.98027419633488266, 0.97827974015615804, 0.97628120948833175, 0.9742785792574935, 0.97227182413150282, 0.97026091851625151, 0.96824583655185426, 0.96622655210876918, 0.9642030387838445, 0.96217526989629076, 0.96014321848357598, 0.95810685729724321, 0.9560661587986472, 0.9540210951546092, 0.95197163823298858, 0.94991775959816649, 0.94785943050644383, 0.94579662190134728, 0.94372930440884373, 0.94165744833246023, 0.93958102364830676, 0.9375, 0.93541434669348533, 0.9333240326917549, 0.93122902660945872, 0.92912929670740663, 0.92702481088695787, 0.92491553668429638, 0.92280144126458752, 0.92068249141601466, 0.91855865354369182, 0.91642989366344874, 0.91429617739548708, 0.91215746995790148, 0.91001373616006476, 0.90786494039587184, 0.90571104663683988, 0.90355201842506006, 0.90138781886599728, 0.89921841062113494, 0.89704375590045771, 0.89486381645477209, 0.8926785535678563, 0.8904879280484379, 0.88829190022199345, 0.88609042992236409, 0.88388347648318444, 0.88167099872911781, 0.87945295496689302, 0.87722930297613744, 0.875, 0.87276500273555879, 0.87052426732400745, 0.86827774934061275, 0.8660254037844386, 0.86376718506782835, 0.86150304700563884, 0.85923294280422002, 0.85695682505013049, 0.85467464569858398, 0.85238635606161595, 0.85009190679596525, 0.84779124789065852, 0.84548432865429268, 0.8431710977020026, 0.84085150294210687, 0.83852549156242118, 0.83619301001622826, 0.83385400400789589, 0.8315084184781294, 0.82915619758884995, 0.82679728470768454, 0.82443162239205747, 0.82205915237286908, 0.81967981553775004, 0.81729355191387631, 0.81490030065033114, 0.8125, 0.81009258730098255, 0.80767799895750536, 0.80525617042032038, 0.80282703616657058, 0.80039052967910607, 0.7979465834252315, 0.79549512883486595, 0.79303609627809502, 0.79056941504209488, 0.78809501330740572, 0.78561281812353345, 0.78312275538385423, 0.78062474979979979, 0.77811872487429579, 0.77560460287442856, 0.7730823048033113, 0.770551750371122, 0.76801285796528174, 0.76546554461974314, 0.76290972598335638, 0.76034531628727742, 0.7577722283113838, 0.75519037334966077, 0.75259966117451849, 0.75, 0.74739129644383739, 0.74477345548831153, 0.7421463804398698, 0.73950997288745202, 0.7368641326594747, 0.73420875777942063, 0.7315437444199766, 0.72886898685566259, 0.72618437741389064, 0.72348980642438909, 0.72078516216692479, 0.71807033081725358, 0.71534519639122485, 0.7126096406869612, 0.7098635432250342, 0.70710678118654757, 0.70433922934904025, 0.70156076002011403, 0.69877124296868431, 0.69597054535375269, 0.69315853165058861, 0.69033506357420382, 0.6875, 0.68465319688145765, 0.68179450716473211, 0.67892378070001347, 0.67604086414949804, 0.67314560089181297, 0.67023783092272549, 0.66731739075195695, 0.66438411329591562, 0.66143782776614768, 0.65847835955329614, 0.65550553010634471, 0.65251915680690942, 0.649519052838329, 0.64650502704928758, 0.64347688381168755, 0.64043442287247487, 0.63737743919909806, 0.6343057228182637, 0.63121905864762984, 0.62811722632005562, 0.625, 0.62186714819163746, 0.61871843353822908, 0.6155536126122565, 0.61237243569579447, 0.60917464655056019, 0.60595998217704117, 0.60272817256205968, 0.59947894041408989, 0.59621200088559101, 0.59292706128157113, 0.5896238207535377, 0.58630196997792872, 0.58296119081805098, 0.5796011559684815, 0.57622152858080544, 0.57282196186947998, 0.5694020986965187, 0.56596157113358858, 0.5625, 0.55901699437494745, 0.55551215108222429, 0.55198505414549048, 0.54843527421200766, 0.54486236794258425, 0.54126587736527421, 0.53764532919016417, 0.53400023408234565, 0.5303300858899106, 0.52663436082352244, 0.52291251658379723, 0.51916399143237968, 0.51538820320220757, 0.51158454824202815, 0.50775240028974755, 0.50389110926865932, 0.5, 0.49607837082461076, 0.4921254921257382, 0.48814060474416587, 0.48412291827592713, 0.48007160924178799, 0.47598581911649429, 0.47186465220442186, 0.46770717334674267, 0.46351240544347894, 0.45927932677184591, 0.45500686808003238, 0.45069390943299864, 0.44633927678392815, 0.44194173824159222, 0.4375, 0.4330127018922193, 0.42847841252506524, 0.42389562394532926, 0.41926274578121059, 0.41457809879442498, 0.40983990776887502, 0.40504629365049127, 0.40019526483955303, 0.39528470752104744, 0.39031237489989989, 0.385275875185561, 0.38017265814363871, 0.375, 0.36975498644372601, 0.3644344934278313, 0.35903516540862679, 0.35355339059327379, 0.34798527267687634, 0.34232659844072882, 0.33657280044590648, 0.33071891388307384, 0.3247595264191645, 0.31868871959954903, 0.3125, 0.30618621784789724, 0.29973947020704494, 0.29315098498896436, 0.28641098093473999, 0.27950849718747373, 0.27243118397129212, 0.2651650429449553, 0.25769410160110379, 0.25, 0.24206145913796356, 0.23385358667337133, 0.22534695471649932, 0.21650635094610965, 0.20728904939721249, 0.19764235376052372, 0.1875, 0.17677669529663689, 0.16535945694153692, 0.15309310892394862, 0.13975424859373686, 0.125, 0.10825317547305482, 0.088388347648318447, 0.0625, 0.0};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *transpo;
    Stream *transpo_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    MYFLT winsize;
	MYFLT pointerPos;
    int in_count;
    MYFLT *buffer; // samples memory
    int modebuffer[4];
} Harmonizer;

static void
Harmonizer_transform_ii(Harmonizer *self) {
    MYFLT val, amp, inc, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT trans = PyFloat_AS_DOUBLE(self->transpo);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    ratio = MYPOW(2.0, trans/12.0);
	rate = (ratio-1.0) / self->winsize;	
    inc = -rate / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
		/* first overlap */
		pos = self->pointerPos;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = ENV_ARRAY[ipart] * (1.0-fpart) + ENV_ARRAY[ipart+1] * fpart;
		
		del = pos * self->winsize;
        if (del > 1.0)
            del = 1.0;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += (self->sr-1);
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] * (1.0 - fpart) + self->buffer[ipart+1] * fpart;
        self->data[i] = val * amp;

		/* second overlap */
		pos = self->pointerPos + 0.5;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = ENV_ARRAY[ipart] * (1.0-fpart) + ENV_ARRAY[ipart+1] * fpart;
		
		del = pos * self->winsize;
        if (del > 1.0)
            del = 1.0;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += (self->sr-1);
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] * (1.0 - fpart) + self->buffer[ipart+1] * fpart;
        self->data[i] += (val * amp);
		
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
		
		self->buffer[self->in_count++] = in[i]  + (self->data[i] * feed);
        if (self->in_count >= self->sr)
            self->in_count = 0;
    }    
}

static void
Harmonizer_transform_ai(Harmonizer *self) {
    MYFLT val, amp, inc, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trans = Stream_getData((Stream *)self->transpo_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);
	    
	MYFLT oneOnWinsize = 1.0 / self->winsize;
	MYFLT oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
		ratio = MYPOW(2.0, trans[i]/12.0);
		rate = (ratio-1.0) * oneOnWinsize;	
		inc = -rate * oneOnSr;;
		
		/* first overlap */
		pos = self->pointerPos;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = ENV_ARRAY[ipart] * (1.0-fpart) + ENV_ARRAY[ipart+1] * fpart;
		
		del = pos * self->winsize;
        if (del > 1.0)
            del = 1.0;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += (self->sr-1);
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] * (1.0 - fpart) + self->buffer[ipart+1] * fpart;
        self->data[i] = val * amp;
		
		/* second overlap */
		pos = self->pointerPos + 0.5;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = ENV_ARRAY[ipart] * (1.0-fpart) + ENV_ARRAY[ipart+1] * fpart;
		
		del = pos * self->winsize;
        if (del > 1.0)
            del = 1.0;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += (self->sr-1);
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] * (1.0 - fpart) + self->buffer[ipart+1] * fpart;
        self->data[i] += (val * amp);
		
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
		
		self->buffer[self->in_count++] = in[i]  + (self->data[i] * feed);
        if (self->in_count >= self->sr)
            self->in_count = 0;
    }  
}

static void
Harmonizer_transform_ia(Harmonizer *self) {
    MYFLT val, amp, inc, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT trans = PyFloat_AS_DOUBLE(self->transpo);
    MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
	
	ratio = MYPOW(2.0, trans/12.0);
	rate = (ratio-1.0) / self->winsize;	
    inc = -rate / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
		/* first overlap */
		pos = self->pointerPos;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = ENV_ARRAY[ipart] * (1.0-fpart) + ENV_ARRAY[ipart+1] * fpart;
		
		del = pos * self->winsize;
        if (del > 1.0)
            del = 1.0;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += (self->sr-1);
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] * (1.0 - fpart) + self->buffer[ipart+1] * fpart;
        self->data[i] = val * amp;
		
		/* second overlap */
		pos = self->pointerPos + 0.5;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = ENV_ARRAY[ipart] * (1.0-fpart) + ENV_ARRAY[ipart+1] * fpart;
		
		del = pos * self->winsize;
        if (del > 1.0)
            del = 1.0;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += (self->sr-1);
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] * (1.0 - fpart) + self->buffer[ipart+1] * fpart;
        self->data[i] += (val * amp);
		
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
		
		self->buffer[self->in_count++] = in[i]  + (self->data[i] * feed[i]);
        if (self->in_count >= self->sr)
            self->in_count = 0;
    }  
}

static void
Harmonizer_transform_aa(Harmonizer *self) {
    MYFLT val, amp, inc, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trans = Stream_getData((Stream *)self->transpo_stream);
    MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
	
	MYFLT oneOnWinsize = 1.0 / self->winsize;
	MYFLT oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
		ratio = MYPOW(2.0, trans[i]/12.0);
		rate = (ratio-1.0) * oneOnWinsize;	
		inc = -rate * oneOnSr;;
		
		/* first overlap */
		pos = self->pointerPos;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = ENV_ARRAY[ipart] * (1.0-fpart) + ENV_ARRAY[ipart+1] * fpart;
		
		del = pos * self->winsize;
        if (del > 1.0)
            del = 1.0;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += (self->sr-1);
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] * (1.0 - fpart) + self->buffer[ipart+1] * fpart;
        self->data[i] = val * amp;
		
		/* second overlap */
		pos = self->pointerPos + 0.5;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = ENV_ARRAY[ipart] * (1.0-fpart) + ENV_ARRAY[ipart+1] * fpart;
		
		del = pos * self->winsize;
        if (del > 1.0)
            del = 1.0;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += (self->sr-1);
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] * (1.0 - fpart) + self->buffer[ipart+1] * fpart;
        self->data[i] += (val * amp);
		
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
		
		self->buffer[self->in_count++] = in[i]  + (self->data[i] * feed[i]);
        if (self->in_count >= self->sr)
            self->in_count = 0;
    }
} 

static void Harmonizer_feedbacktprocessing_ii(Harmonizer *self) { POST_PROCESSING_II };
static void Harmonizer_feedbacktprocessing_ai(Harmonizer *self) { POST_PROCESSING_AI };
static void Harmonizer_feedbacktprocessing_ia(Harmonizer *self) { POST_PROCESSING_IA };
static void Harmonizer_feedbacktprocessing_aa(Harmonizer *self) { POST_PROCESSING_AA };
static void Harmonizer_feedbacktprocessing_ireva(Harmonizer *self) { POST_PROCESSING_IREVA };
static void Harmonizer_feedbacktprocessing_areva(Harmonizer *self) { POST_PROCESSING_AREVA };
static void Harmonizer_feedbacktprocessing_revai(Harmonizer *self) { POST_PROCESSING_REVAI };
static void Harmonizer_feedbacktprocessing_revaa(Harmonizer *self) { POST_PROCESSING_REVAA };
static void Harmonizer_feedbacktprocessing_revareva(Harmonizer *self) { POST_PROCESSING_REVAREVA };

static void
Harmonizer_setProcMode(Harmonizer *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Harmonizer_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Harmonizer_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Harmonizer_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Harmonizer_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_revareva;
            break;
    }   
}

static void
Harmonizer_compute_next_data_frame(Harmonizer *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Harmonizer_traverse(Harmonizer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->transpo);    
    Py_VISIT(self->transpo_stream);    
    Py_VISIT(self->feedback);    
    Py_VISIT(self->feedback_stream);    
    return 0;
}

static int 
Harmonizer_clear(Harmonizer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->transpo);    
    Py_CLEAR(self->transpo_stream);    
    Py_CLEAR(self->feedback);    
    Py_CLEAR(self->feedback_stream);    
    return 0;
}

static void
Harmonizer_dealloc(Harmonizer* self)
{
    free(self->data);   
    free(self->buffer);   
    Harmonizer_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Harmonizer_deleteStream(Harmonizer *self) { DELETE_STREAM };

static PyObject *
Harmonizer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Harmonizer *self;
    self = (Harmonizer *)type->tp_alloc(type, 0);

    self->transpo = PyFloat_FromDouble(-7.0);
    self->feedback = PyFloat_FromDouble(0.0);
    self->winsize = 0.05;
    self->pointerPos = 1.0;
	self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Harmonizer_compute_next_data_frame);
    self->mode_func_ptr = Harmonizer_setProcMode;

    return (PyObject *)self;
}

static int
Harmonizer_init(Harmonizer *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *transpotmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"input", "transpo", "feedback", "winsize", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFOO, kwlist, &inputtmp, &transpotmp, &feedbacktmp, &self->winsize, &multmp, &addtmp))
        return -1; 

	INIT_INPUT_STREAM

    if (transpotmp) {
        PyObject_CallMethod((PyObject *)self, "setTranspo", "O", transpotmp);
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

    self->buffer = (MYFLT *)realloc(self->buffer, (self->sr+1) * sizeof(MYFLT));
    for (i=0; i<(self->sr+1); i++) {
        self->buffer[i] = 0.;
    }    
	
    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Harmonizer_getServer(Harmonizer* self) { GET_SERVER };
static PyObject * Harmonizer_getStream(Harmonizer* self) { GET_STREAM };
static PyObject * Harmonizer_setMul(Harmonizer *self, PyObject *arg) { SET_MUL };	
static PyObject * Harmonizer_setAdd(Harmonizer *self, PyObject *arg) { SET_ADD };	
static PyObject * Harmonizer_setSub(Harmonizer *self, PyObject *arg) { SET_SUB };	
static PyObject * Harmonizer_setDiv(Harmonizer *self, PyObject *arg) { SET_DIV };	

static PyObject * Harmonizer_play(Harmonizer *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Harmonizer_out(Harmonizer *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Harmonizer_stop(Harmonizer *self) { STOP };

static PyObject * Harmonizer_multiply(Harmonizer *self, PyObject *arg) { MULTIPLY };
static PyObject * Harmonizer_inplace_multiply(Harmonizer *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Harmonizer_add(Harmonizer *self, PyObject *arg) { ADD };
static PyObject * Harmonizer_inplace_add(Harmonizer *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Harmonizer_sub(Harmonizer *self, PyObject *arg) { SUB };
static PyObject * Harmonizer_inplace_sub(Harmonizer *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Harmonizer_div(Harmonizer *self, PyObject *arg) { DIV };
static PyObject * Harmonizer_inplace_div(Harmonizer *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Harmonizer_setTranspo(Harmonizer *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->transpo);
	if (isNumber == 1) {
		self->transpo = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->transpo = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->transpo, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->transpo_stream);
        self->transpo_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Harmonizer_setFeedback(Harmonizer *self, PyObject *arg)
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

static PyObject *
Harmonizer_setWinsize(Harmonizer *self, PyObject *arg)
{
	MYFLT wintmp;
	if (arg != NULL) {
        wintmp = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
		if (wintmp <= 1.0)
			self->winsize = wintmp;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Harmonizer_members[] = {
    {"server", T_OBJECT_EX, offsetof(Harmonizer, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Harmonizer, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Harmonizer, input), 0, "Input sound object."},
    {"transpo", T_OBJECT_EX, offsetof(Harmonizer, transpo), 0, "Transposition factor."},
    {"feedback", T_OBJECT_EX, offsetof(Harmonizer, feedback), 0, "Feedback factor."},
    {"mul", T_OBJECT_EX, offsetof(Harmonizer, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Harmonizer, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Harmonizer_methods[] = {
    {"getServer", (PyCFunction)Harmonizer_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Harmonizer_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Harmonizer_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Harmonizer_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Harmonizer_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Harmonizer_stop, METH_NOARGS, "Stops computing."},
	{"setTranspo", (PyCFunction)Harmonizer_setTranspo, METH_O, "Sets global transpo factor."},
    {"setFeedback", (PyCFunction)Harmonizer_setFeedback, METH_O, "Sets feedback factor."},
    {"setWinsize", (PyCFunction)Harmonizer_setWinsize, METH_O, "Sets the window size."},
	{"setMul", (PyCFunction)Harmonizer_setMul, METH_O, "Sets granulator mul factor."},
	{"setAdd", (PyCFunction)Harmonizer_setAdd, METH_O, "Sets granulator add factor."},
    {"setSub", (PyCFunction)Harmonizer_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Harmonizer_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Harmonizer_as_number = {
    (binaryfunc)Harmonizer_add,                      /*nb_add*/
    (binaryfunc)Harmonizer_sub,                 /*nb_subtract*/
    (binaryfunc)Harmonizer_multiply,                 /*nb_multiply*/
    (binaryfunc)Harmonizer_div,                   /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_feedback*/
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
    (binaryfunc)Harmonizer_inplace_add,              /*inplace_add*/
    (binaryfunc)Harmonizer_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Harmonizer_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Harmonizer_inplace_div,           /*inplace_divide*/
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

PyTypeObject HarmonizerType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_transpo*/
    "_pyo.Harmonizer_base",         /*tp_name*/
    sizeof(Harmonizer),         /*tp_basictranspo*/
    0,                         /*tp_itemtranspo*/
    (destructor)Harmonizer_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Harmonizer_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Harmonizer objects. Harmonize an input sound.",           /* tp_doc */
    (traverseproc)Harmonizer_traverse,   /* tp_traverse */
    (inquiry)Harmonizer_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Harmonizer_methods,             /* tp_methods */
    Harmonizer_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Harmonizer_init,      /* tp_init */
    0,                         /* tp_alloc */
    Harmonizer_new,                 /* tp_new */
};

