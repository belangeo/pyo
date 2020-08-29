"""
03-looper.py - High quality crossfading multimode sound looper.

The Looper object reads audio from a PyoTableObject and plays it back in
a loop with user-defined pitch, start time, duration and crossfade time.

Looper will send a trigger signal every time a new playback starts, which
means at the object activation and at the beginning of the crossfade when
looping. User can retrieve the trigger streams with obj['trig'].

Looper also outputs a time stream, given the current position of the
reading pointer, normalized between 0.0 and 1.0 (1.0 means the beginning
of the crossfade), inside the loop. User can retrieve the trigger stream
with obj['time'].

Some methods let change the behaviour of the loop:

- Looper.appendFadeTime(boolean):
    If True, the crossfade starts after the loop duration.
- Looper.fadeInSeconds(boolean):
    If True, the crossfade duration (`xfade` attribute) is set in seconds.

"""
from pyo import *

s = Server().boot()

table = SndTable("../snds/baseballmajeur_m.aif")
table.view()

looper = Looper(
    table=table,  # The table to read.
    pitch=1.0,  # Speed factor, 0.5 means an octave lower,
    # 2 means an octave higher.
    start=0,  # Start time of the loop, in seconds.
    dur=table.getDur(),  # Duration of the loop, in seconds.
    xfade=25,  # Duration of the crossfade, in % of the loop length.
    mode=1,  # Looping mode: 0 = no loop, 1 = forward,
    #               2 = backward, 3 = back-and-forth.
    xfadeshape=0,  # Shape of the crossfade envelope: 0 = linear
    #   1 = equal power, 2 = sigmoid.
    startfromloop=False,  # If True, the playback starts from the loop start
    # point. If False, the playback starts from the
    # beginning and enters the loop mode when crossing
    # the loop start point.
    interp=4,  # Interpolation method (used when speed != 1):
    # 1 = none, 2 = linear, 3 = cosine, 4 = cubic.
    autosmooth=True,  # If True, a lowpass filter, following the pitch,
    # is applied on the output signal to reduce the
    # quantization noise produced by very low transpositions.
    # interp = 4 and autosmooth = True give a very high
    # quality reader for playing sound at low rates.
    mul=0.5,
)
looper.ctrl()

stlooper = looper.mix(2).out()

s.gui(locals())
