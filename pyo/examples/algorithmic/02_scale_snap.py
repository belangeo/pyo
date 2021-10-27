"""
Algorithmic melody patterns snapped on a specific scale...

"""

from pyo import *

s = Server(duplex=0).boot()

scls = [
    [0, 4, 7, 11, 13, 17, 21, 25, 29, 33],
    [0, 7, 12, 14, 17, 21, 24, 29, 31, 34],
    [0, 7, 10, 12, 15, 19, 20],
]

env = CosTable([(0, 0), (50, 1), (500, 0.25), (8191, 0)])
wav = SquareTable(5)

met = Metro(time=0.125, poly=8).play()
note = TrigXnoiseMidi(met, dist="loopseg", x1=1, x2=0.2, mrange=(48, 97))
snp = Snap(note, choice=scls[0], scale=1)

curscl = 0


def changeScl():
    # change the scale for snp.choice argument
    global curscl
    curscl = (curscl + 1) % len(scls)
    snp.choice = scls[curscl]
    print(snp.choice)


metscl = Metro(time=8).play()
tr = TrigFunc(metscl, function=changeScl)

c = TrigEnv(met, table=env, mul=0.07)
d = Osc(table=wav, freq=snp, mul=c).out()
d1 = Osc(table=wav, freq=snp * 0.999, mul=c).out()
d2 = Osc(table=wav, freq=snp * 1.002, mul=c).out()
d3 = Osc(table=wav, freq=snp * 0.997, mul=c).out()

s.gui(locals())
