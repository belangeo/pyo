import random

feed = SigTo(0.05, 0.05, 0.05)
amp = Fader(fadein=0.005, fadeout=0.12, dur=0.125)
syn = SineLoop(freq=[500,510], feedback=feed, mul=amp*amp)
rev = WGVerb(syn, feedback=0.85, cutoff=4500, bal=0.1).out()

def choose(a, b):
    x = random.randint(a, b)
    deg = [0,2,3,5,7,8,11][x%7]
    hz = midiToHz(deg + (x / 7 * 12 + 36))
    syn.freq = [hz,hz*1.005]
    amp.play()
