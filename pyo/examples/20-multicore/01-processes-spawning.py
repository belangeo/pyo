"""
01-processes-spawning.py - Simple processes spawning, no synchronization. 

Need at least 4 cores to be really effective.

Usage:
    python3 -i 01-processes-spawning.py

"""
import sys, time, random, multiprocessing
from pyo import *

if sys.platform.startswith("linux"):
    audio = "jack"
elif sys.platform.startswith("darwin"):
    audio = "portaudio"
else:
    print("Multicore examples don't run under Windows... Sorry!")
    exit()


class Proc(multiprocessing.Process):
    def __init__(self, pitch):
        super(Proc, self).__init__()
        self.daemon = True
        self.pitch = pitch

    def run(self):
        self.server = Server(audio=audio)
        self.server.deactivateMidi()
        self.server.boot().start()

        # 200 randomized band-limited square wave oscillators.
        self.amp = Fader(fadein=5, mul=0.01).play()
        lo, hi = midiToHz((self.pitch - 0.1, self.pitch + 0.1))
        self.fr = Randi(lo, hi, [random.uniform(0.2, 0.4) for i in range(50)])
        self.sh = Randi(0.1, 0.9, [random.uniform(0.2, 0.4) for i in range(50)])
        self.osc = LFO(self.fr, sharp=self.sh, type=2, mul=self.amp).out()

        time.sleep(30)  # Play for 30 seconds.
        self.server.stop()


if __name__ == "__main__":
    # C major chord (one note per process).
    p1, p2, p3, p4 = Proc(48), Proc(52), Proc(55), Proc(60)
    p1.start()
    p2.start()
    p3.start()
    p4.start()
    time.sleep(35)
