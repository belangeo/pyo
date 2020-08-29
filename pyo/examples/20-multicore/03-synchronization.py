"""
03-synchronization.py - Synchronizing multiple processes.

Usage:
    python3 -i 03-synchronization.py

"""
import sys, time, random, multiprocessing
from pyo import *

RECORD = False

if sys.platform.startswith("linux"):
    audio = "jack"
elif sys.platform.startswith("darwin"):
    audio = "portaudio"
    print("SharedTable does not behave correctly under MacOS... This example doesn't work.")
else:
    print("Multicore examples don't run under Windows... Sorry!")
    exit()


class Main(multiprocessing.Process):
    def __init__(self):
        super(Main, self).__init__()
        self.daemon = True

    def run(self):
        self.server = Server(audio=audio)
        self.server.deactivateMidi()
        self.server.boot().start()
        bufsize = self.server.getBufferSize()
        if RECORD:
            self.server.recstart("synchronization.wav")

        self.tab1 = SharedTable("/audio-1", create=False, size=bufsize)
        self.tab2 = SharedTable("/audio-2", create=False, size=bufsize)
        self.out1 = TableScan(self.tab1).out()
        self.out2 = TableScan(self.tab2).out(1)

        time.sleep(30)
        self.server.stop()


class Proc(multiprocessing.Process):
    def __init__(self, voice, conn):
        super(Proc, self).__init__()
        self.voice = voice
        self.connection = conn
        self.daemon = True

    def run(self):
        self.server = Server(audio=audio)
        self.server.deactivateMidi()
        self.server.boot()
        bufsize = self.server.getBufferSize()

        name = "/audio-%d" % self.voice
        self.audiotable = SharedTable(name, create=True, size=bufsize)

        onsets = random.sample([5, 6, 7, 8, 9], 2)
        self.tab = CosTable([(0, 0), (32, 1), (512, 0.5), (4096, 0.5), (8191, 0)])
        self.ryt = Euclide(time=0.125, taps=16, onsets=onsets, poly=1).play()
        self.mid = TrigXnoiseMidi(self.ryt, dist=12, mrange=(60, 96))
        self.frq = Snap(self.mid, choice=[0, 2, 3, 5, 7, 8, 10], scale=1)
        self.amp = TrigEnv(self.ryt, table=self.tab, dur=self.ryt["dur"], mul=self.ryt["amp"])
        self.sig = SineLoop(freq=self.frq, feedback=0.08, mul=self.amp * 0.3)
        self.fil = TableFill(self.sig, self.audiotable)

        # Wait for an incoming signal before starting the server.
        while not self.connection.poll():
            pass
        self.server.start()

        time.sleep(30)
        self.server.stop()


if __name__ == "__main__":
    signal, child = multiprocessing.Pipe()
    p1, p2 = Proc(1, child), Proc(2, child)
    main = Main()
    p1.start()
    p2.start()
    time.sleep(0.1)
    main.start()
    time.sleep(0.1)
    signal.send(1)
