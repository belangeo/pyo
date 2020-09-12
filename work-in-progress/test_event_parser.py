import random
from pyo import *
from EventParser import EventParser

### Orchestra ###
class Instr1:
    def __init__(self, duration, *args):
        self.f = Fader(0.001, duration - 0.005, duration, args[1]).play()
        self.f.setExp(3)
        self.osc = SineLoop([args[0], args[0] * 1.01], feedback=0.12, mul=self.f)
        self.filt = ButHP(self.osc, args[0]).out()


class Rhythm:
    def __init__(self, duration, *args):
        self.env = CosTable([(0, 0), (32, 1), (1000, 0.25), (8191, 0)])
        rhythms = [
            [2, 2, 1, 1, 1, 1, 4],
            [1, 2, 1, 2, 2, 1, 2, 1, 2, 1, 1],
            [4, 2, 1, 1, 2, 2, 1.3, 1.3, 1.4],
        ]
        self.seq = Seq(0.125, random.choice(rhythms), poly=1, onlyonce=True).play()
        self.amp = TrigEnv(self.seq, self.env, 0.125, mul=args[1])
        self.osc = Lorenz([args[0], args[0] * 1.01], args[2], True, mul=self.amp)
        self.disto = Disto(self.osc, drive=0.9, slope=0.9, mul=0.4)
        self.bp = ButBP(self.disto, freq=1000, q=0.7).out()


class Chord:
    def __init__(self, duration, *args, **kwargs):
        self.f = Fader(1, 1, duration, args[0]).play()
        self.o1 = LFO(kwargs["f1"], sharp=0.95, type=2, mul=self.f).out()
        self.o2 = LFO(kwargs["f2"], sharp=0.95, type=2, mul=self.f).out(1)
        self.o3 = LFO(kwargs["f3"], sharp=0.95, type=2, mul=self.f).out()
        self.o4 = LFO(kwargs["f4"], sharp=0.95, type=2, mul=self.f).out(1)


### Little score generator ###
sc = ""
starttime = 8
for i in range(300):
    template = "Instr1 => %f %f %f %f\n"
    dur = random.choice([0.25, 0.75, 1])
    freq = random.choice(midiToHz([67, 70, 72, 75, 79, 84, 86, 87, 91]))
    amp = random.uniform(0.08, 0.15)
    sc = sc + template % (starttime, dur, freq, amp)
    starttime += random.choice([0.125, 0.125, 0.25, 0.25])
for i in range(65):
    template = "Rhythm => %f %f %f %f %f\n"
    pit = random.uniform(0.9, 1.0)
    amp = random.uniform(0.1, 0.2)
    chaos = random.uniform(0.8, 1.0)
    sc = sc + template % (i, 1, pit, amp, chaos)
for i in range(17):
    template = "Chord => %f 5 0.03 %s\n"
    notes = midiToHz([48.01, 55.02, 59.99, 63.01, 67, 69.98, 72.03, 75.04])
    cdict = {
        "f1": random.choice(notes),
        "f2": random.choice(notes),
        "f3": random.choice(notes),
        "f4": random.choice(notes),
    }
    sc = sc + template % (i * 4, str(cdict))

### Rendering ###
REALTIME = True

if REALTIME:
    s = Server().boot()
else:
    s = Server(buffersize=8, audio="offline").boot()
    s.recordOptions(dur=70, filename="rendered_score.wav")

reader = EventParser(s, sc, globals())
reader.play()

if REALTIME:
    s.gui(locals())
else:
    s.start()
