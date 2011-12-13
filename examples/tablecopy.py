#!/usr/bin/env python
# encoding: utf-8
"""
Various tests with PyoTableObject.copy().
"""
from pyo import *
import copy
s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

ex = 0

if ex == 0:
    tt = HarmTable([1,0,.3,0,.2,0,.143,0,.111])
    tt.replace([1,.5,.3,.25,.2])
    tt.normalize()
    tt2 = tt.copy()
    tt.view(title="square")
    tt2.view(title="saw")
elif ex == 1:
    snd = SndTable([SNDS_PATH+"/transparent.aif"]).reverse()
    snd2 = snd.copy()
    a = Osc(snd2, freq=snd2.getRate(), mul=.3).out()
    def ch():
        snd.path = SNDS_PATH+"/accord.aif"
elif ex == 2:
    path = os.getcwd()
    snds = [x for x in os.listdir(path) if x.endswith("aif") or x.endswith("aiff") or x.endswith("wav")]

    t = SndTable("ounkmaster.aif", start=0, stop=1)
    print len(t)

    def addsnd():
        p = random.choice(snds)
        dur = sndinfo(p)[1]
        start = random.uniform(0, dur*0.5)
        duration = random.uniform(.1, (dur-start)*.1)
        pos = random.uniform(0, t.getDur())
        cross = random.uniform(0.05, duration/2)
        t.insert(p, pos=pos, crossfade=cross, start=start, stop=start+duration)

    def gen():
        p = random.choice(snds)
        dur = sndinfo(p)[1]
        start = random.uniform(0, dur*0.5)
        duration = random.uniform(.1, (dur-start)*.1)
        t.setSound(p, start=start, stop=start+duration)
        for i in range(10):
            addsnd()

    gen()
    t2 = t.copy()
    a = Osc(t2, t2.getRate(), mul=.5).out()
elif ex == 3:
    objs = []
    def cp():
        t2 = t.copy()
        f = Fader(fadein=1).play()
        pl = Osc(t2, freq=1, interp=4, mul=f).out()
        objs.extend([t2, f, pl])
    def mouse(pos):
        print pos
    i = Input([0,1])
    t = NewTable(1, chnls=2)
    r = TableRec(i, t, fadetime=0.1).play(delay=.1)
    tr = TrigFunc(r["trig"], function=cp)

s.gui(locals())