from pyo import *
s = Server().boot()
s.start()

a = SfPlayer('demos/transparent.aif', loop=True)
b = SPan(a)
lfo = Sine(.25, 0, .5, .5)
lfo2 = Sine(1, 0, .5, .5)
lfo3 = Sine(4, 0, .5, .5)
c = Freeverb(b, size=.7, damp=.9, bal=.5).out()

c.damp = 1
c.size = .7
c.size = .4
c.bal = 0
c.bal = .5
c.bal = .75
c.bal = 1
c.out()

b.pan = 0.75

a.speed = 1

s.stop()
