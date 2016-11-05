#!/usr/bin/env python
# encoding: utf-8
"""
Calling Python function from an audio stream with the Score object.

"""
from pyo import *
import random

s = Server(sr=44100, nchnls=2, buffersize=256, duplex=0).boot()

# Frequency of event generation in Hz
GEN_FREQ = .25

aamp = Linseg([(0,0),(1./GEN_FREQ,1),(2./GEN_FREQ,0)], mul=.5).stop()
asyn = Rossler(pitch=0.25, chaos=0.5, stereo=True, mul=aamp).out()

bamp = Linseg([(0,0),(1./GEN_FREQ,1),(2./GEN_FREQ,0)], mul=.25).stop()
bsyn = SineLoop(freq=[100,100], feedback=0.05, mul=bamp).out()

camp = Linseg([(0,0),(1./GEN_FREQ,1),(2./GEN_FREQ,0)], mul=.25).stop()
csyn = FM(carrier=[100,100], ratio=[.49,.5], index=6, mul=camp).out()

def event_0():
    print "playing chaotic attractor"
    asyn.pitch = random.uniform(0.25,0.75)
    asyn.chaos = random.uniform(0.2,0.8)
    aamp.play()

def event_1():
    print "playing looped oscillator"
    bsyn.freq = [random.choice(range(50,501,50))*random.uniform(0.99,1.01) for i in range(2)]
    bsyn.feedback = random.uniform(0.01, 0.1)
    bamp.play()

def event_2():
    print "playing frequency modulation"
    csyn.carrier = [random.choice([50,100,150,200,250])*random.uniform(0.99,1.01) for i in range(2)]
    csyn.ratio = [random.choice([.1251,.249,.502,.7501,1.003]) for i in range(2)]
    camp.play()

tr = RandInt(max=3, freq=GEN_FREQ)
sc = Score(input=tr, fname='event_')

s.gui(locals())
