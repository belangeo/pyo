"""
04-data-control.py - Multicore midi synthesizer.

Need at least 4 cores to be really effective.

Usage:
    python3 -i 04-data-control.py

"""
import sys, time, multiprocessing
from random import uniform
from pyo import *

VOICES_PER_CORE = 4

if sys.platform.startswith("linux"):
    audio = "jack"
elif sys.platform.startswith("darwin"):
    audio = "portaudio"
else:
    print("Multicore examples don't run under Windows... Sorry!")
    exit()


class Proc(multiprocessing.Process):
    def __init__(self, pipe):
        super(Proc, self).__init__()
        self.daemon = True
        self.pipe = pipe

    def run(self):
        self.server = Server(audio=audio)
        self.server.deactivateMidi()
        self.server.boot().start()

        self.mid = Notein(poly=VOICES_PER_CORE, scale=1, first=0, last=127)
        self.amp = MidiAdsr(self.mid["velocity"], 0.005, 0.1, 0.7, 0.5, mul=0.01)
        self.pit = self.mid["pitch"] * [uniform(0.99, 1.01) for i in range(40)]
        self.rc1 = RCOsc(self.pit, sharp=0.8, mul=self.amp).mix(1)
        self.rc2 = RCOsc(self.pit * 0.99, sharp=0.8, mul=self.amp).mix(1)
        self.mix = Mix([self.rc1, self.rc2], voices=2)
        self.rev = STRev(Denorm(self.mix), [0.1, 0.9], 2, bal=0.30).out()

        while True:
            if self.pipe.poll():
                data = self.pipe.recv()
                self.server.addMidiEvent(*data)
            time.sleep(0.001)

        self.server.stop()


if __name__ == "__main__":
    main1, child1 = multiprocessing.Pipe()
    main2, child2 = multiprocessing.Pipe()
    main3, child3 = multiprocessing.Pipe()
    main4, child4 = multiprocessing.Pipe()
    mains = [main1, main2, main3, main4]
    p1, p2, p3, p4 = Proc(child1), Proc(child2), Proc(child3), Proc(child4)
    p1.start()
    p2.start()
    p3.start()
    p4.start()

    playing = {0: [], 1: [], 2: [], 3: []}
    currentcore = 0

    def callback(status, data1, data2):
        global currentcore
        if status == 0x80 or status == 0x90 and data2 == 0:
            for i in range(4):
                if data1 in playing[i]:
                    playing[i].remove(data1)
                    mains[i].send([status, data1, data2])
                    break
        elif status == 0x90:
            for i in range(4):
                currentcore = (currentcore + 1) % 4
                if len(playing[currentcore]) < VOICES_PER_CORE:
                    playing[currentcore].append(data1)
                    mains[currentcore].send([status, data1, data2])
                    break

    s = Server()
    s.setMidiInputDevice(99)  # Open all devices.
    s.boot().start()
    raw = RawMidi(callback)
