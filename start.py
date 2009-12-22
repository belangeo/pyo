# -*- coding: utf-8 -*-

# start the python interpreter and enter :
# >>> execfile('start.py')
# >>> s.start()
# to listen the selected example

from pyo import *
import random

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0)
s.boot()

example = 30

if example == 1:
    t = HarmTable([1,0,0,.2,0,0,.1,0,0,.04])
    w = Osc(t, [random.uniform(196,204) for i in range(100)], 0, .01).out()
    x = Osc(t, [random.uniform(296,304) for i in range(100)], 0, .006).out(1)
    y = Osc(t, [random.uniform(396,404) for i in range(100)], 0, .003).out()
    z = Osc(t, [random.uniform(496,504) for i in range(100)], 0, .002).out(1)
elif example == 2:
    t = HarmTable([1-(i*.01) for i in range(100)])
    a = Osc(t, 30, 0, .001)
    b1 = Osc(HarmTable(), [.22, .23], 0, 500, 1000)
    f = Biquad(a, b1, 20, 0).out(0)
elif example == 3:
    a = Noise(.5)  
    b = Osc(HarmTable(), 9.98, 0, 500, 1000)
    c = Osc(HarmTable(), 20, 0, 18, 20)
    f = Biquad(a, b, c, 0).out()
elif example == 4:
    t = HarmTable()
    a = Osc(t, 300)
    b = Osc(t, 1, 0, .48, .5)
    d = Disto(a, b, mul=.5).out()
elif example == 5:
    a = Osc(HannTable(), .5)
    b = Noise(a).out()
elif example == 6:
    t = SndTable('demos/accord.aif')
    a = Osc(t, [t.getRate(), t.getRate()*.5], mul=.3).out()
elif example == 7:
    # on OS X, need a device that supports duplex mode (or an aggregate device!)
    a = Input(mul=.5)
    b = Osc(HarmTable(), 2, 0, .45, .5)
    c = Disto(a, b, .6, .1).out()
    d = Osc(HarmTable(), 1.5, 0, .45, .5)
    e = Disto(a, d, .6, .1).out(1)
elif example == 8:
    # Fader -> .play() starts fadein
    # dur=0 means wait for stop method to start fadeout
    # positive values will trigger fadeout automatically
    f1 = Fader(fadein=.02,fadeout=1, dur=0, mul=.3).play()
    a = Osc(HarmTable(), [250,500], 0, f1).out()
    f2 = Fader(fadein=1, fadeout=1, dur=5, mul=.2).play()
    b = Osc(HarmTable(), [375,625], 0, f2).out(1)
elif example == 9:
    # need a MIDI device available (and portmidi installed)
    t = HarmTable([1])
    m = Midictl(ctlnumber=[107,102], minscale=250, maxscale=1000)
    p = Port(m, .02)
    a = Osc(t, p, .5).out()
    a1 = Osc(t, p * 1.25, 0, .5).out()
    a2 = Osc(t, p * 1.5, 0, .5).out()
    # inplace_multiply and inplace_addition are the same as calling object.setMul or setAdd
    # object *= 1.5
    # multiply and add create a dummy object and call setMul or setAdd on Dummy, leaving original object unmodified
    # dummy = object * 1.5
    # In two case, right argument can be another pyo object
elif example == 10:
    # Sine phase shift
    t1 = HarmTable()
    a = Osc(t1, freq=1, phase=0, mul=.5, add=.5)
    b = Osc(t1, freq=1, phase=.5, mul=.5, add=.5)
    t = HarmTable([1,0,.3,0,.2])
    osc1 = Osc(t, 400, 0, a).out()
    osc2 = Osc(t, 500, 0, b).out()
elif example == 11:
    mod = Sine([random.uniform(.5, 3) for i in range(10)], 0, .1, .1)
    a = Sine([random.uniform(400,1000) for i in range(10)], 0, .1).out()
    a *= mod
elif example == 12:
    # Open Sound Control sending values
    a = Sine([1,1.5], 0, 100, [600, 1000])
    b = OscSend(a, 10000, ['/pit1','/pit2'])
elif example == 13:
    # Open Sound Control receiving values
    a = OscReceive(port=10001, address=['/pitch', '/amp'])
    b = Sine(a['/pitch'], 0, a['/amp']).out()
elif example == 14:  
    t = SndTable('demos/transparent.aif')
    a = Osc(t, t.getRate())
    si = Sine(.1, 0, .25, .5)
    fad = Fader(20, 20, mul=.9).play()
    d = Delay(a, delay=si, feedback=fad, maxdelay=1, mul=.1).out()
    f = Biquad(d, 2500, 2).out()
elif example == 15:
    a = Noise(.5)
    t = HarmTable([1-(i*.01) for i in range(100)])
    b = Osc(t, 30, 0, .01) 
    f = Biquad(a, 1000, 10).out()
    # call:
    # f.setInput(b, 10)
elif example == 16:
    t1 = SndTable('demos/transparent.aif')
    t2 = SndTable('demos/accord.aif')
    a = Osc(t1, t1.getRate(), 0, .7)                 
    b = Osc(t2, t2.getRate(), 0, .7)
    d = Delay(a, .25, .8, maxdelay=1).out()
    # call:
    # d.setInput(b, 10)
elif example == 17:
    t1 = NewTable(2, 1)
    a = Input(0)
    b = TableRec(a, t1, .01)
    c = Osc(t1, [t1.getRate(), t1.getRate()*.99]).out()
    # to record in the empty table, call:
    # b.play()
elif example == 18:
    a = Sine(.1, 0, .25, 1)
    sf = SfPlayer('demos/transparent.aif', speed=a, loop=True, offset=0, interp=2, mul=.5).out()
elif example == 19:
    a = Notein(poly=10, scale=1, mul=.5)
    p = Port(a['velocity'], .001, .5)
    b = Sine(a['pitch'], 0, p).out()
    c = Sine(a['pitch'] * 0.997, 0, p).out()
    d = Sine(a['pitch'] * 1.005, 0, p).out()
    e = Sine(a['pitch'] * 0.993, 0, p).out()
elif example == 20:
    a = Notein(poly=5, scale=2, first=0, last=11, mul=.7)
    p = Port(a['velocity'], .001, 1)
    b = SfPlayer('demos/transparent.aif', a['pitch'], loop=True, mul=p).out()
    a1 = Notein(poly=5, scale=2, first=12, last=23, mul=.7)
    p1 = Port(a1['velocity'], .001, 1)
    b1 = SfPlayer('demos/accord.aif', a1['pitch'], loop=True, mul=p1).out()
elif example == 21:
    a = Sine([.05,.1,.15], 0, .25, 1)
    b = SfMarkerShuffler('demos/transparent.aif', speed=a, interp=4, mul=.5).out()
elif example == 22:
    a = Sine([.05, .075], 0, .125, .25)
    b = Metro(a, poly=8)
    c = Delay(b, [(i+1)*.001 for i in range(8)], .99, 1, mul=.6).out()
elif example == 23:
    t = HarmTable([1,0,.3,0,.15,0,.1,0,0,.04,0,0,.02])
    a = Metro(.125, 8)
    b = TrigEnv(a, HannTable(), .45)
    w1 = Osc(t, [100,125,150,175,200,225,250,275], 0, b).out()
elif example == 24:
    t = HarmTable([1,0,0,.2,0,0,.1,0,0,.05])
    w1 = Osc(t, 250, 0, .1).out()
    
    def pat():
        l = t.list
        ll = len(l)
        newl = []
        for i, val in enumerate(l):
            off = random.uniform(-.01, .01)
            newval = val + off
            maxamp = ((ll - i) / float(ll))
            if newval < 0: newval = 0
            elif newval > maxamp: newval = maxamp
            newl.append(newval)
        t.list = newl
        
    p = PyPattern(pat, .125)
    
elif example == 25:
    t = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    a = Osc(t, 250, 0, .5).out()
    def pat():
        a.freq = random.randint(200, 400)
        
    p = Pattern(pat, .125)

elif example == 26:
    met = Metro(.125, 32)
    env = LinTable([(0,0), (5,1), (30,1), (75,.15), (200,.05), (8191,0)])
    trig = TrigEnv(met, env)
    snd = SndTable('demos/transparent.aif')
    outs = Osc(snd, [snd.getRate()*random.uniform(.95,1.1) for i in range(32)], 
                [i/8. for i in range(32)], trig).out()

elif example == 27:
    a = Noise(.3)
    b = Sine([random.uniform(.5,2) for i in range(10)], 0, .5, .5)
    c = Sine(.05, 0, 20, 21)
    d = BandSplit(a, 10, 100, 10000, c, b).out()

elif example == 28:
    a = SfPlayer("demos/accord.aif", loop=True).out()
    b = Hilbert(a)
    quad = Sine([250, 500], [0, .25])
    mod1 = b['real'] * quad[0]
    mod2 = b['imag'] * quad[1]
    up = mod1 - mod2
    down = mod1 + mod2
    up.out()
elif example == 29:
    a = SfPlayer("demos/transparent.aif", loop=True)
    a1 = a * .5
    a1.out()
    b = Follower(a)
    c = Noise(b).out()
elif example == 30:
    m = Metro(.25).play()
    c = Counter(m, 0, 11)
    sel = Select(c, [0, 3, 6, 9, 11])
    env = LinTable([(0,0), (5,1.5), (30,1.5), (75,.25), (200,.15), (8191,0)])
    trig = TrigEnv(sel, env)
    sine = Sine([random.randint(400, 1000) for i in range(5)], 0, trig).out()
               
class FreqMod:
    def __init__(self, carrier=250, ratio=.5, index=1, amplitude=1):
        self.carrierFrequency = carrier
        self.ratio = ratio
        self.index = index
        self.modulatorFrequency = carrier * ratio
        self.modulatorAmplitude = self.modulatorFrequency * index
        self.amplitude = amplitude
        
        self.table = HarmTable([1])
        self.modulator = Osc(self.table, self.modulatorFrequency, 0, self.modulatorAmplitude, self.carrierFrequency)
        self.carrier = Osc(self.table, self.modulator, 0, self.amplitude)
        
    def play(self):
        self.modulator.play()
        self.carrier.out()
        return self
        
    def stop(self):
        self.modulator.stop()
        self.carrier.stop()

    def setCarrier(self, carrier):
        self.carrierFrequency = carrier
        self.modulatorFrequency = carrier * self.ratio
        self.modulatorAmplitude = self.modulatorFrequency * self.index
        self.refresh()

    def setRatio(self, ratio):
        self.ratio = ratio
        self.modulatorFrequency = self.carrierFrequency * ratio
        self.modulatorAmplitude = self.modulatorFrequency * self.index
        self.refresh()

    def setIndex(self, index):
        self.index = index
        self.modulatorAmplitude = self.modulatorFrequency * index
        self.refresh()
          
    def refresh(self):    
        self.modulator.setFreq(self.modulatorFrequency) 
        self.modulator.setMul(self.modulatorAmplitude)
        self.modulator.setAdd(self.carrierFrequency)       
       