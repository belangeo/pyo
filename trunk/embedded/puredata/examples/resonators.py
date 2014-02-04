reson = SigTo(2, 0.005, 2)
freqs = SigTo([50,100,150,200,250,300,350,400], time=0.2)

ins = Input([0,1])
wgs = Waveguide(ins, freq=freqs, dur=reson, mul=0.1)
wgss = wgs.mix(2)
wgss.out()

