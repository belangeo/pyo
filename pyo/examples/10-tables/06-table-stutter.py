"""
06-table-stutter.py - Variable length table reading.

This little program use a function and a time-variable function caller
to stutter a sound loaded in a table. The starting point of the table
playback is set initially to near the end of the table, and move toward
the beginning each time a new playback is called.

For the playback, we use a Pointer object, which is a table reader with
control on the pointer position (normalized between 0 and 1).

"""
from pyo import *
import random

s = Server().boot()

STUTTER = 0.025  # Delta time added each time the playback restart.
FADETIME = 0.01  # Fadein-fadeout time to avoid clicks.

# Load a sound in the table and get its duration.
table = SndTable(SNDS_PATH + "/transparent.aif")
tabdur = table.getDur()

# Intialize the line used to read the table.
line = Linseg([(0, 0), (1, 1)])

# Amplitude envelope, to avoid clicks.
amp = Fader(FADETIME, FADETIME, dur=STUTTER, mul=0.5)
amp.setExp(2)

# Read the sound and mix it to stereo.
read = Pointer(table=table, index=line, mul=amp).mix(2).out()

# Global variables (start position and playback duration).
start = tabdur - STUTTER
dur = STUTTER


def go():
    "Read a segment of a sound table and set the duration before the next iteration."
    global start, dur

    # Create the pointer segment (from normalized start position to the end of the table)
    line.list = [(0, start / tabdur), (dur, 1)]

    # Assign duration to the envelope and the function caller (Pattern).
    amp.dur = pat.time = dur

    # Decrement start position and increment duration by STUTTER seconds.
    start -= STUTTER
    dur += STUTTER

    # Reset start and dur variables when reaching the beginning of the sound.
    if start < 0:
        start = tabdur - STUTTER
        dur = STUTTER

    # Activate the pointer's index line and the amplitude envelope.
    line.play()
    amp.play()


# Call go() each time the playback reaches the end of the file
pat = Pattern(function=go, time=STUTTER).play()

s.gui(locals())
