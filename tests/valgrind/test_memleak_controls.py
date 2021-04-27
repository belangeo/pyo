# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_controls.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

a = Fader().play()
a.setFadein(0.01)

b = Adsr().play()
b.setAttack(0.01)

c = Linseg([(0,0), (0.1,1), (0.2,0)]).play()
c.setList([(0,1), (0.1,0), (0.2,1), (0.3, 0)])

d = Expseg([(0,0), (0.1,1), (0.2,0)]).play()
d.setList([(0,1), (0.1,0), (0.2,1), (0.3, 0)])

e = Metro().play()
e.time = Sig(0.01)

f = Seq(seq=[1,1,1,.5,.5], poly=4).play()
f.time = Sig(0.01)
f.seq = [.5, .5, .5, 1]

g = Cloud(poly=2).play()
g.density = Sig(1.5)

h = Trig().play()

i = Beat(poly=2).play()
i.time = Sig(0.25)
i.taps = 8
i.taps = 24
i.w1 = 90
i.w2 = 80
i.w3 = 70
i.fill()
i.new()

j = TrigBurst(Trig().play())

i1 = Sig(0)
i2 = Sig(0)

k = Pan(i1, outs=8)
k.input = i2
k.pan = Sig(0.5)
k.spread = Sig(0.5)

l = Pan(i1, outs=8)
l.input = i2
l.pan = Sig(0.5)

m = Switch(i1, outs=4)
m.input = i2
m.voice = Sig(0.5)

# VoiceManager
if False:
    env = CosTable([(0,0),(100,1),(500,.5),(8192,0)])
    met = Metro()
    vm = VoiceManager(met)
    sel = Select(vm, value=[0,1,2,3])
    osc = OscTrig(env, sel)
    amp = TableRead(env, freq=osc)
    vm.setTriggers(amp["trig"])

# Mixer
a0 = Sine(freq=100, mul=.2)
b0 = FM(carrier=200, ratio=[.5013,.4998], index=6, mul=.2)
mm = Mixer(outs=3, chnls=2, time=.025)
fx1 = Disto(mm[0], drive=.9, slope=.9, mul=.1).out()
fx2 = Freeverb(mm[1], size=.8, damp=.8, mul=.5).out()
fx3 = Harmonizer(mm[2], transpo=1, feedback=.75, mul=.5).out()
mm.addInput(0, a0)
mm.addInput(1, b0)
mm.setAmp(0,0,.5)
mm.setAmp(0,1,.5)
mm.setAmp(1,2,.5)
mm.setAmp(1,1,.5)

# Selector
a1 = Sine(freq=[100,101], mul=.3)
b1 = Noise(mul=.1)
c1 = SineLoop(freq=[101,102], mul=.5)
lf1 = Sine(freq=.1, add=1)
d1 = Selector(inputs=[a1,b1,c1], voice=lf1).out()

def call1():
    pass
def call2(x):
    pass

n = Pattern(call1, 4 / s.getSamplingRate()).play()
n.time = Sig(4 / s.getSamplingRate())
n.arg = 1
n.function = call2

def _sc1():
    pass
def _sc2():
    pass

o = Score(Sig(1), "_sc")

p1 = CallAfter(call1, 4 / s.getSamplingRate()).play()
p2 = CallAfter(call2, 4 / s.getSamplingRate(), 1).play()

rnds = Randi(freq=[1,2], min=200, max=400)
sines = SineLoop(freq=rnds, feedback=.05, mul=.2).out()
home = os.path.expanduser('~')
rec = ControlRec(rnds, home+"/test", rate=100, dur=0).play()


s.process()

o.input = Sig(2)

s.process()

rec.write()

rnds2 = ControlRead(home+"/test", rate=100, loop=True)
sines2 = SineLoop(freq=rnds, feedback=.05, mul=.15).out()

p = Select(Sig(0))

q = Change(Sig(0))

r = SigTo(0.5)
r.value = Sig(0.5)
r.time = Sig(0.5)

t = VarPort(0.5, time=4 / s.getSamplingRate(), function=call1).play()

t2 = VarPort(0.5, time=4 / s.getSamplingRate(), function=call2).play()
t2.arg = 1

s.process()
s.process()

a.stop()
b.stop()
e.stop()
f.stop()
g.stop()
i.stop()
n.stop()

s.process()
s.stop()
s.shutdown()
