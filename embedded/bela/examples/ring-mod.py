"""
01-Simple FX - Stereo ring modulator.

This example shows how to build a ring modulation effect 
with modulator's frequency and brightness control with
analog inputs (use audio inputs after the stereo audio
channels, ie. Input(2) is analog-in 0, Input(3) is 
analog-in 1, etc.).

It also show how to send signal to analog outputs. Again,
use the outputs after the stereo audio channels, ie.
.out(2) writes to analog-out 0, .out(3) to analog-out 1,
etc.).

"""
# Set to True if you want to control the modulator 
# frequency and brightness with analog inputs.
WITH_ANALOG_INPUT = True
# If False, set frequency and brightness values.
FREQUENCY = 500     # Hz
BRIGHTNESS = 0.05   # 0 -> 0.2

# If True, a positive value is sent on analog-out 0 and 1
# whenever there is an output signal (can be used to build 
# a cheap vumeter with leds).
WITH_ANALOG_OUTPUT = True

# stereo input
src = Input([0,1])

# Don't know if the noise comes from my mic,
# but it sounds better with a gate on the input!
gate = Gate(src.mix(), thresh=-60, risetime=.005, 
            falltime=.02, lookahead=1, outputAmp=True)
srcg = src * gate

if WITH_ANALOG_INPUT:
    # analog-in 0 (modulator's frequency)
    i0 = Tone(Input(2), 8) 
    freq = Scale(i0, 0, 1, 1, 1000, 3)

    # analog-in 1 (modulator's brightness)
    i1 = Tone(Input(3), 8) 
    feed = i1 * 0.2
else:
    freq = FREQUENCY
    feed = BRIGHTNESS

# Modulation oscillator
mod = SineLoop(freq, feed)

# Ring modulation and stereo output
out = (srcg * mod).out()

if WITH_ANALOG_OUTPUT:
    # analog out 0-1 (stereo vumeter) 
    fol = Sqrt(Clip(Follower(out, mul=4))).out(2)

