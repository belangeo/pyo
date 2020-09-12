"""
02-sharing-audio.py - Sharing audio signals between processes.

Usage:
    python3 -i 02-sharing-audio.py

"""
import time, random, multiprocessing
from pyo import *


class Proc(multiprocessing.Process):
    def __init__(self, create):
        super(Proc, self).__init__()
        self.daemon = True
        self.create = create

    def run(self):
        self.server = Server(audio="jack")
        self.server.deactivateMidi()
        self.server.boot().start()
        bufsize = self.server.getBufferSize()

        nbands = 50
        names = ["/f%02d" % i for i in range(nbands)]

        if self.create:  # 50 bands frequency splitter.
            freq = [20 * 1.1487 ** i for i in range(nbands)]
            amp = [pow(10, (i * -1) * 0.05) * 8 for i in range(nbands)]
            self.input = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
            self.filts = IRWinSinc(self.input, freq, freq, 3, 128, amp)
            self.table = SharedTable(names, create=True, size=bufsize)
            self.recrd = TableFill(self.filts, self.table)
        else:  # Independent transposition per band.
            self.table = SharedTable(names, create=False, size=bufsize)
            self.tscan = TableScan(self.table)
            transpofac = [random.uniform(0.98, 1.02) for i in range(nbands)]
            self.pvana = PVAnal(self.tscan, size=1024, overlaps=4)
            self.pvtra = PVTranspose(self.pvana, transpo=transpofac)
            self.pvsyn = PVSynth(self.pvtra).out()

        time.sleep(30)
        self.server.stop()


if __name__ == "__main__":
    analysis = Proc(create=True)
    synthesis = Proc(create=False)
    analysis.start()
    synthesis.start()
