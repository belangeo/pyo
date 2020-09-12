#!/usr/bin/env python
# encoding: utf-8
"""
The Melo class records 16 notes in a table and reads it at variable speed.

"""
from pyo import *
import random

s = Server(duplex=0).boot()

SCALES = [[0, 2, 5, 7, 9, 11], [0, 2, 3, 7, 8, 11], [0, 3, 5, 7, 8, 10]]


class Melo:
    def __init__(self, amp=0.1, speed=1, midirange=(48, 84)):
        # table to record new melody fragment
        self.table = NewTable(2)
        # loopseg generation
        self.base_mel = XnoiseMidi(dist=12, freq=8, x1=1, x2=0.25, scale=0, mrange=midirange)
        # snap on scale and convert to hertz
        self.base_melo = Snap(self.base_mel, choice=[0, 2, 4, 5, 7, 9, 11], scale=1)
        # record a new fragment every 10 seconds
        self.trig_rec = Metro(time=10).play()
        self.tab_rec = TrigTableRec(self.base_melo, self.trig_rec, self.table)
        # rise amp of the oscillators after the first recording
        self.amp = Counter(self.tab_rec["trig"], min=1, max=2, mul=amp)
        # random speed for the oscillator reading the melody table + portamento
        self.speed = Choice(choice=[0.0625, 0.125, 0.125, 0.125, 0.25, 0.5], freq=1.0 * speed)
        self.freq = Osc(self.table, self.speed * speed)
        self.freq_port = Port(self.freq, risetime=0.01, falltime=0.01)
        # 8 randis (freq and amp) to create a chorus of oscillators
        self.rnd_chorus = Randi(min=0.99, max=1.01, freq=[random.uniform(3, 6) for i in range(8)])
        self.rnd_amp = Randi(min=0, max=0.15, freq=[random.uniform(0.2, 0.5) for i in range(8)])
        # oscillators...
        self.osc = LFO(
            self.freq_port * self.rnd_chorus, type=3, sharp=0.75, mul=Port(self.amp, mul=self.rnd_amp),
        ).out()

    def setScale(self, scl):
        self.base_melo.choice = scl


def choose_scale():
    scl = random.choice(SCALES)
    for obj in objs:
        obj.setScale(scl)


a = Melo(amp=0.3, speed=1, midirange=(60, 84))
b = Melo(amp=0.6, speed=0.5, midirange=(48, 72))
c = Melo(amp=1, speed=0.25, midirange=(36, 60))
objs = [a, b, c]

pat = Pattern(time=20, function=choose_scale).play()

s.gui(locals())
