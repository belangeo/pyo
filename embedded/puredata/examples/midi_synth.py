notein = Notein(scale=1)
modwheel = Midictl(1, maxscale=0.2)

amp = MidiAdsr(notein["velocity"], 0.001, 0.01, 0.7, 0.05)

lfo = Sine(5, mul=modwheel, add=1)
synth1 = RCOsc(freq=notein["pitch"]*lfo, sharp=0.75, mul=amp)
synth2 = RCOsc(freq=notein["pitch"]*lfo*1.01, sharp=0.74, mul=amp)
stereo = Mix([synth1.mix(1), synth2.mix(1)], voices=2)
rev = STRev(stereo, inpos=[0,1], revtime=2, bal=0.25, mul=0.2).out()

