"""
05-micro-montage.py - Create table from random chunks of a soundfile.

This example creates a new sound table from random chunks of a soundfile.

The SndTable object has some methods to help mixing different sounds or
parts of sounds into a single table:

- setSound(path, start=0, stop=None)
    Replace the table content with the new sound.
- insert(path, pos=0, crossfade=0, start=0, stop=None)
    Insert samples at a given position in the table, with crossfades.
- append(path, crossfade=0, start=0, stop=None)
    Append samples at the end of the table, with crossfade.

To generate a new mix in the sound table, call the `gen()` function
in the Interpreter field of the Server GUI.

"""
from pyo import *
import random

s = Server().boot()

# Path and duration of the choosen soundfile.
path = "../snds/baseballmajeur_m.aif"
snddur = sndinfo(path)[1]

# Initialize an empty sound table.
table = SndTable()

# Before generating a new table mix, we activate the fadeout of the
# envelope to ensure that the table is modified in the silence. As
# soon as the table is generated, we call the envelope fadein.
fade = Fader(fadein=0.005, fadeout=0.005, dur=0, mul=0.7)

# Reads the table with forward looping.
loop = Looper(table, dur=table.getDur(), xfade=5, mul=fade)

# Adds some reverb and send the signal to the output.
rvrb = STRev(loop, inpos=0.50, revtime=1.5, bal=0.15).out()


def addsnd():
    # Randomly choose a new starting point and a new duration.
    start = random.uniform(0, snddur * 0.7)
    duration = random.uniform(0.1, 0.3)

    # Randomly choose an insert point in the sound table and a croosfade time.
    pos = random.uniform(0.05, table.getDur() - 0.5)
    cross = random.uniform(0.04, duration / 2)

    # Insert the new chunk in the current sound table.
    table.insert(path, pos=pos, crossfade=cross, start=start, stop=start + duration)


def delgen():
    # Randomly choose a new starting point and a new duration.
    start = random.uniform(0, snddur * 0.7)
    duration = random.uniform(0.1, 0.3)

    # Load the chosen segment in the sound table.
    table.setSound(path, start=start, stop=start + duration)

    # Add 10 more chunks.
    for i in range(10):
        addsnd()

    # Set the new table duration to the Looper and reset it.
    loop.dur = table.getDur()
    loop.reset()

    # Activate the envelope fadein.
    fade.play()


# CallAfter calls a function after a given delay time.
caller = CallAfter(function=delgen, time=0.005).stop()


def gen():
    "Create a new mix in the sound table."
    fade.stop()  # Launch the fadeout...
    caller.play()  # ... then call the delayed generation.


# Generate the intial table.
gen()

s.gui(locals())
