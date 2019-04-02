#!/usr/bin/env python
# encoding: utf-8
"""
Launch the audio processing in a separated thread and send events.
See play_snd.py for subprocess sound player.

"""
import subprocess, time, random, sys

PYTHON_EXE = "python%d" % sys.version_info[0]

pipe = subprocess.Popen(["%s -i play_snd.py" % PYTHON_EXE], shell=True,
                        universal_newlines=True, stdin=subprocess.PIPE).stdin

# Wait for the Server to be ready
time.sleep(2)

# send events to the sub process
for i in range(20):
    snd = "../snds/snd_%d.aif" % random.randrange(1,7)
    pipe.write("sf.path = '%s'\ndump = sf.play()\n" % snd)
    pipe.flush()
    time.sleep(random.uniform(.2, .5))

# Stop the audio Server before exiting
pipe.write("s.stop()\ntime.sleep(0.25)\n")
pipe.close()
