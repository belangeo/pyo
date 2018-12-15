"""
01-compress-expand.py - Adjust the dynamic range of the signal.


"""
from pyo import *

s = Server().boot()

src = SfPlayer("../snds/drumloop.wav", loop=True)
cmp = Compress(src, thresh=-18, ratio=3, risetime=0.005, falltime=0.05, knee=0.5)
exp = Expand(src, downthresh=-32, upthresh=-12, ratio=2.5, risetime=0.005, falltime=0.05)
gat = Gate(src, thresh=-40, risetime=0.005, falltime=0.05)

labels = ["Original", "Compressed", "Expanded", "Gated"]

signals = [src, cmp, exp, gat]

output = Selector(signals)

stout = output.mix(2).out()

num_of_sigs = len(signals)
c = 1
def endOfLoop():
    global c
    output.voice = c
    if sc.viewFrame is not None:
        sc.viewFrame.SetTitle("=== %s ===" % labels[c])
    c = (c + 1) % num_of_sigs

tf = TrigFunc(src["trig"], endOfLoop)

sc = Scope(output, wintitle="=== Original ===")




s.gui(locals())
