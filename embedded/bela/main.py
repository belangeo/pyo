src = Input()

lf = FastSine(freq=[.005,.008,.011,.015],
              quality=0,
              mul=[.04,.08,.12,.16],
              add=[100,200,300,400])
lf2 = FastSine(.005, quality=0, mul=.2, add=.7)

det_wg = AllpassWG(src, freq=lf, feed=.999, detune=lf2, mul=.15).out()

tPulse = HarmTable([1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1])
lfos = FastSine(freq=[.01, .009], initphase=[0, 0.5], quality=0).range(2, 10)
waves = OscBank(tPulse, freq=lfos, spread=1.4, num=10, mul=0.02).out()

amps = FastSine(freq=[.09,.13,.15,.18],
              quality=0,
              mul=.0025, add=.0025)
sines = FastSine(freq=[2500,4000,6000,8500],
                 initphase=[0, 0.25, 0.5, 0.75],
                 quality=0,
                 mul=amps).out()
