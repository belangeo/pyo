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

MYFLT HANN_ARRAY[513] = {0.0, 3.7649080427748505e-05, 0.00015059065189787502, 0.00033880770582522812, 0.0006022718974137975, 0.00094094354992541041, 0.0013547716606548965, 0.0018436939086109994, 0.0024076366639015911, 0.0030465149988219697, 0.0037602327006450165, 0.0045486822861099951, 0.0054117450176094928, 0.0063492909210707826, 0.0073611788055293892, 0.008447256284391802, 0.0096073597983847847, 0.010841314640186228, 0.01214893498073577, 0.013530023897219967, 0.014984373402728013, 0.016511764477573965, 0.01811196710228008, 0.019784740292217107, 0.021529832133895588, 0.023346979822903069, 0.025235909703481663, 0.02719633730973936, 0.029227967408489652, 0.031330494043712576, 0.033503600582630577, 0.035746959763392261, 0.038060233744356631, 0.040443074154971115, 0.042895122148234655, 0.04541600845473881, 0.048005353438278331, 0.050662767153023036, 0.053387849402242393, 0.056180189798573088, 0.059039367825822531, 0.061964952902296755, 0.064956504445644325, 0.068013571939206652, 0.071135694999863941, 0.074322403447367402, 0.077573217375146442, 0.080887647222580961, 0.084265193848727327, 0.087705348607487466, 0.0912075934242082, 0.094771400873702671, 0.098396234259677584, 0.10208154769555827, 0.10582678618669689, 0.10963138571395276, 0.1134947733186315, 0.11741636718877052, 0.12139557674675766, 0.12543180273827048, 0.12952443732252056, 0.13367286416379365, 0.13787645852426655, 0.1421345873580907, 0.14644660940672627, 0.1508118752955136, 0.15522972763146659, 0.15969950110227349, 0.16422052257649078, 0.16879211120491411, 0.17341357852311173, 0.17808422855510436, 0.18280335791817737, 0.18757025592880688, 0.19238420470968665, 0.19724447929783728, 0.20215034775378338, 0.20710107127178057, 0.21209590429107733, 0.21713409460819338, 0.22221488349019902, 0.22733750578897693, 0.23250119005645153, 0.23770515866076564, 0.24294862790338922, 0.24823080813714127, 0.25355090388510804, 0.25890811396043867, 0.26430163158700115, 0.26973064452087991, 0.27519433517269665, 0.28069188073073631, 0.28622245328485907, 0.29178521995118151, 0.29737934299750513, 0.30300397996947603, 0.30865828381745514, 0.3143414030240812, 0.32005248173250589, 0.32579065987528277, 0.33155507330389, 0.33734485391886859, 0.3431591298005543, 0.34899702534038596, 0.35485766137276886, 0.3607401553074735, 0.36664362126255085, 0.37256717019774277, 0.37850991004836809, 0.3844709458596644, 0.39044937992156514, 0.39644431190389079, 0.4024548389919359, 0.40848005602242954, 0.41451905561984936, 0.42057092833306936, 0.42663476277231915, 0.43270964574643694, 0.43879466240039189, 0.44488889635305845, 0.45099142983521967, 0.45710134382778012, 0.46321771820016633, 0.46933963184889571, 0.47546616283629101, 0.48159638852932057, 0.48772938573854391, 0.4938642308571401, 0.5, 0.50613576914285996, 0.51227061426145615, 0.51840361147067948, 0.52453383716370905, 0.53066036815110429, 0.53678228179983378, 0.54289865617222, 0.54900857016478044, 0.55511110364694161, 0.56120533759960811, 0.56729035425356311, 0.57336523722768085, 0.5794290716669307, 0.58548094438015064, 0.59151994397757046, 0.59754516100806421, 0.60355568809610927, 0.60955062007843486, 0.61552905414033565, 0.62149008995163202, 0.62743282980225734, 0.63335637873744921, 0.6392598446925265, 0.64514233862723114, 0.6510029746596141, 0.6568408701994457, 0.66265514608113152, 0.66844492669611, 0.67420934012471723, 0.67994751826749411, 0.6856585969759188, 0.69134171618254492, 0.69699602003052408, 0.70262065700249487, 0.70821478004881866, 0.71377754671514104, 0.71930811926926386, 0.72480566482730335, 0.73026935547911997, 0.73569836841299896, 0.74109188603956144, 0.74644909611489207, 0.75176919186285884, 0.75705137209661089, 0.76229484133923453, 0.76749880994354869, 0.77266249421102329, 0.77778511650980109, 0.78286590539180656, 0.78790409570892272, 0.79289892872821943, 0.79784965224621673, 0.80275552070216283, 0.80761579529031347, 0.81242974407119317, 0.81719664208182285, 0.82191577144489569, 0.82658642147688843, 0.83120788879508589, 0.83577947742350922, 0.84030049889772651, 0.84477027236853353, 0.84918812470448646, 0.85355339059327373, 0.8578654126419093, 0.86212354147573356, 0.86632713583620635, 0.87047556267747961, 0.87456819726172963, 0.87860442325324228, 0.88258363281122953, 0.88650522668136844, 0.89036861428604719, 0.89417321381330317, 0.89791845230444178, 0.90160376574032242, 0.90522859912629738, 0.90879240657579186, 0.91229465139251265, 0.91573480615127267, 0.91911235277741898, 0.92242678262485356, 0.9256775965526326, 0.92886430500013606, 0.93198642806079335, 0.93504349555435573, 0.93803504709770325, 0.94096063217417747, 0.94381981020142702, 0.94661215059775761, 0.94933723284697691, 0.95199464656172172, 0.95458399154526119, 0.95710487785176535, 0.95955692584502894, 0.96193976625564337, 0.96425304023660785, 0.96649639941736942, 0.96866950595628754, 0.97077203259151035, 0.97280366269026064, 0.97476409029651834, 0.97665302017709688, 0.97847016786610441, 0.98021525970778289, 0.98188803289771998, 0.98348823552242604, 0.98501562659727204, 0.98646997610278009, 0.98785106501926423, 0.98915868535981377, 0.99039264020161522, 0.9915527437156082, 0.99263882119447056, 0.99365070907892927, 0.99458825498239056, 0.99545131771388995, 0.99623976729935504, 0.99695348500117809, 0.99759236333609846, 0.99815630609138895, 0.9986452283393451, 0.99905905645007453, 0.9993977281025862, 0.99966119229417472, 0.99984940934810207, 0.99996235091957231, 1.0, 0.99996235091957231, 0.99984940934810207, 0.99966119229417472, 0.9993977281025862, 0.99905905645007453, 0.9986452283393451, 0.99815630609138895, 0.99759236333609846, 0.99695348500117809, 0.99623976729935504, 0.99545131771388995, 0.99458825498239056, 0.99365070907892927, 0.99263882119447056, 0.9915527437156082, 0.99039264020161522, 0.98915868535981377, 0.98785106501926423, 0.98646997610278009, 0.98501562659727204, 0.98348823552242604, 0.98188803289771998, 0.98021525970778289, 0.97847016786610441, 0.97665302017709688, 0.97476409029651834, 0.97280366269026064, 0.97077203259151035, 0.96866950595628754, 0.96649639941736942, 0.96425304023660785, 0.96193976625564337, 0.95955692584502894, 0.95710487785176535, 0.95458399154526119, 0.95199464656172172, 0.94933723284697691, 0.94661215059775761, 0.94381981020142702, 0.94096063217417747, 0.93803504709770325, 0.93504349555435573, 0.93198642806079335, 0.92886430500013606, 0.9256775965526326, 0.92242678262485356, 0.91911235277741898, 0.91573480615127267, 0.91229465139251265, 0.90879240657579186, 0.90522859912629738, 0.90160376574032242, 0.89791845230444178, 0.89417321381330317, 0.89036861428604719, 0.88650522668136844, 0.88258363281122953, 0.87860442325324228, 0.87456819726172963, 0.87047556267747961, 0.86632713583620635, 0.86212354147573356, 0.8578654126419093, 0.85355339059327373, 0.84918812470448646, 0.84477027236853353, 0.84030049889772651, 0.83577947742350922, 0.83120788879508589, 0.82658642147688843, 0.82191577144489569, 0.81719664208182285, 0.81242974407119317, 0.80761579529031347, 0.80275552070216283, 0.79784965224621673, 0.79289892872821943, 0.78790409570892272, 0.78286590539180656, 0.77778511650980109, 0.77266249421102329, 0.76749880994354869, 0.76229484133923453, 0.75705137209661089, 0.75176919186285884, 0.74644909611489207, 0.74109188603956144, 0.73569836841299896, 0.73026935547911997, 0.72480566482730335, 0.71930811926926386, 0.71377754671514104, 0.70821478004881866, 0.70262065700249487, 0.69699602003052408, 0.69134171618254492, 0.6856585969759188, 0.67994751826749411, 0.67420934012471723, 0.66844492669611, 0.66265514608113152, 0.6568408701994457, 0.6510029746596141, 0.64514233862723114, 0.6392598446925265, 0.63335637873744921, 0.62743282980225734, 0.62149008995163202, 0.61552905414033565, 0.60955062007843486, 0.60355568809610927, 0.59754516100806421, 0.59151994397757046, 0.58548094438015064, 0.5794290716669307, 0.57336523722768085, 0.56729035425356311, 0.56120533759960811, 0.55511110364694161, 0.54900857016478044, 0.54289865617222, 0.53678228179983378, 0.53066036815110429, 0.52453383716370905, 0.51840361147067948, 0.51227061426145615, 0.50613576914285996, 0.5, 0.4938642308571401, 0.48772938573854391, 0.48159638852932057, 0.47546616283629101, 0.46933963184889571, 0.46321771820016633, 0.45710134382778012, 0.45099142983521967, 0.44488889635305845, 0.43879466240039189, 0.43270964574643694, 0.42663476277231915, 0.42057092833306936, 0.41451905561984936, 0.40848005602242954, 0.4024548389919359, 0.39644431190389079, 0.39044937992156514, 0.3844709458596644, 0.37850991004836809, 0.37256717019774277, 0.36664362126255085, 0.3607401553074735, 0.35485766137276886, 0.34899702534038596, 0.3431591298005543, 0.33734485391886859, 0.33155507330389, 0.32579065987528277, 0.32005248173250589, 0.3143414030240812, 0.30865828381745514, 0.30300397996947603, 0.29737934299750513, 0.29178521995118151, 0.28622245328485907, 0.28069188073073631, 0.27519433517269665, 0.26973064452087991, 0.26430163158700115, 0.25890811396043867, 0.25355090388510804, 0.24823080813714127, 0.24294862790338922, 0.23770515866076564, 0.23250119005645153, 0.22733750578897693, 0.22221488349019902, 0.21713409460819338, 0.21209590429107733, 0.20710107127178057, 0.20215034775378338, 0.19724447929783728, 0.19238420470968665, 0.18757025592880688, 0.18280335791817737, 0.17808422855510436, 0.17341357852311173, 0.16879211120491411, 0.16422052257649078, 0.15969950110227349, 0.15522972763146659, 0.1508118752955136, 0.14644660940672627, 0.1421345873580907, 0.13787645852426655, 0.13367286416379365, 0.12952443732252056, 0.12543180273827048, 0.12139557674675766, 0.11741636718877052, 0.1134947733186315, 0.10963138571395276, 0.10582678618669689, 0.10208154769555827, 0.098396234259677584, 0.094771400873702671, 0.0912075934242082, 0.087705348607487466, 0.084265193848727327, 0.080887647222580961, 0.077573217375146442, 0.074322403447367402, 0.071135694999863941, 0.068013571939206652, 0.064956504445644325, 0.061964952902296755, 0.059039367825822531, 0.056180189798573088, 0.053387849402242393, 0.050662767153023036, 0.048005353438278331, 0.04541600845473881, 0.042895122148234655, 0.040443074154971115, 0.038060233744356631, 0.035746959763392261, 0.033503600582630577, 0.031330494043712576, 0.029227967408489652, 0.02719633730973936, 0.025235909703481663, 0.023346979822903069, 0.021529832133895588, 0.019784740292217107, 0.01811196710228008, 0.016511764477573965, 0.014984373402728013, 0.013530023897219967, 0.01214893498073577, 0.010841314640186228, 0.0096073597983847847, 0.008447256284391802, 0.0073611788055293892, 0.0063492909210707826, 0.0054117450176094928, 0.0045486822861099951, 0.0037602327006450165, 0.0030465149988219697, 0.0024076366639015911, 0.0018436939086109994, 0.0013547716606548965, 0.00094094354992541041, 0.0006022718974137975, 0.00033880770582522812, 0.00015059065189787502, 3.7649080427748505e-05, 0.0}; 
MYFLT INV_BASE_HERTZ = 1.0 / 261.62556530066814;

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
    MYFLT val, amp, inc, hertz, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT trans = PyFloat_AS_DOUBLE(self->transpo);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    hertz = 8.1757989156437 * MYPOW(1.0594630943593, 60+trans);
	ratio = hertz * INV_BASE_HERTZ;
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
		amp = HANN_ARRAY[ipart] * (1.0-fpart) + HANN_ARRAY[ipart+1] * fpart;
		
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
		amp = HANN_ARRAY[ipart] * (1.0-fpart) + HANN_ARRAY[ipart+1] * fpart;
		
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
    MYFLT val, amp, inc, hertz, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trans = Stream_getData((Stream *)self->transpo_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);
	    
	MYFLT oneOnWinsize = 1.0 / self->winsize;
	MYFLT oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
		hertz = 8.1757989156437 * MYPOW(1.0594630943593, 60+trans[i]);
		ratio = hertz * INV_BASE_HERTZ;
		rate = (ratio-1.0) * oneOnWinsize;	
		inc = -rate * oneOnSr;;
		
		/* first overlap */
		pos = self->pointerPos;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = HANN_ARRAY[ipart] * (1.0-fpart) + HANN_ARRAY[ipart+1] * fpart;
		
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
		amp = HANN_ARRAY[ipart] * (1.0-fpart) + HANN_ARRAY[ipart+1] * fpart;
		
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
    MYFLT val, amp, inc, hertz, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT trans = PyFloat_AS_DOUBLE(self->transpo);
    MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
	
    hertz = 8.1757989156437 * MYPOW(1.0594630943593, 60+trans);
	ratio = hertz * INV_BASE_HERTZ;
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
		amp = HANN_ARRAY[ipart] * (1.0-fpart) + HANN_ARRAY[ipart+1] * fpart;
		
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
		amp = HANN_ARRAY[ipart] * (1.0-fpart) + HANN_ARRAY[ipart+1] * fpart;
		
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
    MYFLT val, amp, inc, hertz, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trans = Stream_getData((Stream *)self->transpo_stream);
    MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
	
	MYFLT oneOnWinsize = 1.0 / self->winsize;
	MYFLT oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
		hertz = 8.1757989156437 * MYPOW(1.0594630943593, 60+trans[i]);
		ratio = hertz * INV_BASE_HERTZ;
		rate = (ratio-1.0) * oneOnWinsize;	
		inc = -rate * oneOnSr;;
		
		/* first overlap */
		pos = self->pointerPos;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 512;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = HANN_ARRAY[ipart] * (1.0-fpart) + HANN_ARRAY[ipart+1] * fpart;
		
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
		amp = HANN_ARRAY[ipart] * (1.0-fpart) + HANN_ARRAY[ipart+1] * fpart;
		
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

