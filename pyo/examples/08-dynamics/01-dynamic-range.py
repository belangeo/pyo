"""
01-dynamic-range.py - Adjust the dynamic range of the signal.

Comparison of three objects used to adjust the dynamic range of the signal.

- Compress : Reduces the dynamic range of an audio signal.
- Expand : Increases the dynamic range of an audio signal.
- Gate : Allows a signal to pass only when its amplitude is above a threshold.

These three objects, by default, process independently each audio stream
relatively to its own RMS value. This can be a problem if they are passed
a stereo signal (or any multiphonic signals) where both channels should be
processed in the same way.

An alternative usage to the one illustrated below is to mix a multi-channel
signal prior to the dynamic processor and to tell the object to output the
amplitude value that should be applied to all streams at once. Something
like this:

>>> sf = SfPlayer("your/stereo.sound.wav")
>>> cmp = Compress(sf.mix(1), thresh=-18, ratio=3, outputAmp=True)
>>> # Compress both signal with the common amplitude curve.
>>> compressed = sf * cmp

"""
from pyo import *

s = Server().boot()

# The original source.
src = SfPlayer("../snds/drumloop.wav", loop=True)

# The three dynamic processing.
cmp = Compress(src, thresh=-18, ratio=3, risetime=0.005, falltime=0.05, knee=0.5)
exp = Expand(src, downthresh=-32, upthresh=-12, ratio=3, risetime=0.005, falltime=0.05)
gat = Gate(src, thresh=-40, risetime=0.005, falltime=0.05)

# These are labels that are shown as the scope window title.
labels = ["Original", "Compressed", "Expanded", "Gated"]

# List of signals to choose from.
signals = [src, cmp, exp, gat]

# Selector is used here to choose which signal to listen to.
output = Selector(signals)

# Converts the signal from mono to stereo.
stout = output.mix(2).out()

# Live oscilloscope of the alternated signals.
sc = Scope(output, wintitle="=== Original ===")

# The endOfLoop function cycles through the different signals
# and change the selection of the Selector object.
num_of_sigs = len(signals)
c = 1


def endOfLoop():
    global c
    output.voice = c
    if sc.viewFrame is not None:
        sc.viewFrame.SetTitle("=== %s ===" % labels[c])
    c = (c + 1) % num_of_sigs


# endOfLoop is called every time the SfPlayer reaches the end of the sound.
tf = TrigFunc(src["trig"], endOfLoop)

s.gui(locals())
