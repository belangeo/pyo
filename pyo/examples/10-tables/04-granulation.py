"""
04-granulation.py - Full control granular synthesis.

The output of a Granular Synthesis is composed of many individual
grains of sound. A grain is a short chunk of sound, typically between
10 and 100 ms (but can also vary outside this range), with an amplitude
envelope in the shape of a bell curve. The sonic quality of a granular
texture is a result of the distribution of grains in time and of the
parameters selected for the synthesis of each grain. 

This example shows the usage of the most featured granulation object
of the library!

Available granulation objects, in order of complexity, are:

- Granulator
- Granule
- Particle
- Particle2

"""
from pyo import *

s = Server().boot()

snd = SndTable("../snds/baseballmajeur_m.aif")
snd.view()

# Remove sr/4 samples to the size of the table, just to be sure
# that the reading pointer never exceeds the end of the table.
end = snd.getSize() - s.getSamplingRate() * 0.25

# A Tuckey envelope for the grains (also known as flat-top envelope).
env = WinTable(7)
env.view(title="Grain envelope")

# The grain pitch has a default value of 1, to which we can add a
# randomness factor by raising the "mul" value of the Noise.
pit = Noise(0, add=1)
pit.ctrl([SLMap(0, 1, "lin", "mul", 0)], title="Pitch Randomness")

# The grain position oscillates slowly between the beginning and
# the end of the table. We add a little jitter to the position to
# attenuate phasing artifacts when overlapping the grains.
pososc = Sine(0.05).range(0, end)
posrnd = Noise(mul=0.01, add=1)
pos = pososc * posrnd

# The grain panoramisation is completely random.
pan = Noise(mul=0.5, add=0.5)

# The grain filter center frequency choices are the first 40
# harmonics of a base frequency oscillating between 75 and 125 Hz.
cf = Sine(freq=0.07).range(75, 125)
fcf = Choice(list(range(1, 40)), freq=150, mul=cf)

grn = Particle2(
    table=snd,  # The table to read.
    env=env,  # The grain envelope.
    dens=128,  # The density of grains per second.
    # The next arguments are sampled at the beginning of the grain
    # and hold their until the end of the grain.
    pitch=pit,  # The pitch of the grain.
    pos=pos,  # The position of the grain in the table.
    dur=0.2,  # The duration of the grain in seconds.
    dev=0.005,  # The maximum deviation of the start time,
    # synchronous versus asynchronous granulation.
    pan=pan,  # The pan value of the grain.
    filterfreq=fcf,  # The filter frequency of the grain.
    filterq=20,  # The filter Q of the grain.
    filtertype=2,  # The filter type of the grain.
    # End of sampled arguments.
    chnls=2,  # The output number of streams of the granulator.
    mul=0.2,
)
grn.ctrl()

# Some grains can be surprisingly loud so we compress the output of the granulator.
comp = Compress(grn, thresh=-20, ratio=4, risetime=0.005, falltime=0.10, knee=0.5, mul=2).out()

s.gui(locals())
