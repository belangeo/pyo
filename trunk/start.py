# -*- coding: utf-8 -*-

# start the python interpreter and enter :
# >>> execfile('start.py')
# >>> s.start()
# to listen the selected example

from pyo import *
import random

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0)

example = 9

if example == 1:
    t = HarmTable([1,0,0,.2,0,0,.1,0,0,.04])
    w = Osc(t, [random.uniform(196,204) for i in range(100)], .01).out()
    x = Osc(t, [random.uniform(296,304) for i in range(100)], .006).out(1)
    y = Osc(t, [random.uniform(396,404) for i in range(100)], .003).out()
    z = Osc(t, [random.uniform(496,504) for i in range(100)], .002).out(1)
elif example == 2:
    t = HarmTable([1-(i*.01) for i in range(100)])
    a = Osc(t, 30, .001)
    b1 = Osc(HarmTable(), [.22, .23], 500, 1000)
    f = Biquad(a, b1, 20, 0).out(0)
elif example == 3:
    a = Noise(.5)  
    b = Osc(HarmTable(), 9.98, 500, 1000)
    c = Osc(HarmTable(), 20, 18, 20)
    f = Biquad(a, b, c, 0).out()
elif example == 4:
    t = HarmTable()
    a = Osc(t, 300)
    b = Osc(t, 1, .48, .5)
    d = Disto(a, b).out()
elif example == 5:
    a = Osc(HannTable(), .5)
    b = Noise(a).out()
elif example == 6:
    # load stereo sound into buffers and play them
    # set the Server to use 2 channels
    t = SndTable('/Users/olipet/Desktop/sons/cacanne4.aiff')
    a = Osc(t, t.getRate()).out()
elif example == 7:
    # on OS X, need a device that supports duplex mode (or an aggregate device!)
    a = Input(mul=.5)
    b = Osc(HarmTable(), 2, .45, .5)
    c = Disto(a, b, .6, .1).out()
    d = Osc(HarmTable(), 1.5, .45, .5)
    e = Disto(a, d, .6, .1).out(1)
elif example == 8:
    # Fader -> .play() starts fadein
    # dur=0 means wait for stop method to start fadeout
    # positive values will trigger fadeout automatically
    f1 = Fader(fadein=.02,fadeout=1, dur=0, mul=.3).play()
    a = Osc(HarmTable(), [250,500], f1).out()
    f2 = Fader(fadein=1, fadeout=1, dur=5, mul=.2).play()
    b = Osc(HarmTable(), [375,625], f2).out(1)
elif example == 9:
    # need a MIDI device available (and portmidi installed)
    t = HarmTable([1])
    m = Midictl(ctlnumber=74, minscale=250, maxscale=1000)
    p = Port(m, .1)
    a = Osc(t, p, .5).out()
    a1 = Osc(t, p * 1.25, .5).out()
    a2 = Osc(t, p * 1.5, .5).out()
    # inplace_multiply and inplace_addition are the same as calling object.setMul or setAdd
    # object *= 1.5
    # multiply and add create a dummy object and call setMul or setAdd on Dummy, leaving original object unmodified
    # dummy = object * 1.5
    # In two case, right argument can be another pyo object
elif example == 10:
    # Sine phase shift
    a = Sine(freq=1, phase=0, mul=.5, add=.5)
    b = Sine(freq=1, phase=.5, mul=.5, add=.5)
    t = HarmTable([1,0,.3,0,.2])
    osc1 = Osc(t, 200, a).out()
    osc2 = Osc(t, 300, b).out()
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
    t = SndTable('/Users/olipet/Desktop/sons/cacanne4.aiff')
    a = Osc(t, t.getRate())
    si = Sine(.1, 0, .25, .5)
    fad = Fader(20, 20, mul=.9).play()
    d = Delay(a, delay=si, feedback=fad, maxdelay=1, mul=.1).out()
    f = Biquad(d.mix(1), 2500, 2).out()
elif example == 15:
    a = Noise(.5)
    t = HarmTable([1-(i*.01) for i in range(100)])
    b = Osc(t, 30, .01) 
    f = Biquad(a, 1000, 10).out()
    # call:
    # f.setInput(b, 10)
elif example == 16:
    t1 = SndTable('/Users/olipet/Desktop/sons/baseballmajeur_s.aif')
    t2 = SndTable('/Users/olipet/Desktop/sons/cacanne4.aiff')
    a = Osc(t1, t1.getRate(), .7)                 
    b = Osc(t2, t2.getRate(), .7)
    d = Delay(a, .25, .8, maxdelay=1).out()
elif example == 17:
    t1 = NewTable(2, 2)
    #a = Noise(.3)
    #t2 = HarmTable([1-(i*.01) for i in range(100)])
    #aa = Osc(t2, 30, .01) 
    #a = Input(0)
    a = Sine(750, 0, .4)
    b = TableRec(a, t1, .01)
    c = Osc(t1, t1.getRate()).out()
    # call:
    # b.play()
elif example == 18:
    a = Sine(.1, 0, .25, -1)
    sf = SfPlayer('/Users/olipet/Desktop/sons/cacanne4.aiff', speed=a, loop=True, offset=0, interp=4, mul=.5).out()
elif example == 19:
    a = Notein(mul=.5)
    b = Sine(a['pitch'], 0, a['velocity']).out()
            
class FreqMod:
    def __init__(self, carrier=250, ratio=.5, index=1, amplitude=1):
        self.carrierFrequency = carrier
        self.ratio = ratio
        self.index = index
        self.modulatorFrequency = carrier * ratio
        self.modulatorAmplitude = self.modulatorFrequency * index
        self.amplitude = amplitude
        
        self.table = HarmTable([1])
        self.modulator = Osc(self.table, self.modulatorFrequency, self.modulatorAmplitude, self.carrierFrequency)
        self.carrier = Osc(self.table, self.modulator, self.amplitude)
        
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
       