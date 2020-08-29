"""
Launching the audio processing in a separated thread

**08-multiple-threads.py**

This program launches an audio process in a separated thread with the
subprocess module and sends parameters to its standard input to control
the sound playback.

"""
import os
import sys
import time
import random
import subprocess

# Get the python command according to the current operating system.
if sys.platform.startswith("win"):
    PYTHON_EXE = "py -%d.%d" % (sys.version_info[0], sys.version_info[1])
else:
    PYTHON_EXE = "python%d.%d" % (sys.version_info[0], sys.version_info[1])

# Path of the python file to run in the subprocess.
script_path = os.path.join(os.path.expanduser("~"), "08_multiple_threads_process.py")

# Thread's processing... Soundfile player + delay + reverb.
script = """
from pyo import *

s = Server(duplex=0).boot()

sf = SfPlayer('../snds/snd_1.aif', mul=0.7)
dd = SmoothDelay(sf, delay=0.25, feedback=0.5)
rv = STRev(sf+dd, inpos=0.50, revtime=1.5, cutoff=5000, bal=0.25).out()

s.start()
"""

# Create the python file to run in the subprocess.
f = open(script_path, "w")
f.write(script)
f.close()

# Launch an interactive python (-i flag) in a subprocess and store a
# reference to the standard input to pass message to the running process.
pipe = subprocess.Popen(
    ["%s -i %s" % (PYTHON_EXE, script_path)], shell=True, universal_newlines=True, stdin=subprocess.PIPE,
).stdin

# Wait for the audio server to be ready.
time.sleep(2)

# Send events to the subprocess.
for i in range(20):
    # Randomly choose a sound (snd_1.aif to snd_6.aif).
    snd = "../snds/snd_%d.aif" % random.randrange(1, 7)

    # Replace the sound and start the playback.
    pipe.write("sf.path = '%s'\ndump = sf.play()\n" % snd)
    pipe.write("dd.delay = %f\n" % random.uniform(0.1, 0.5))
    pipe.flush()

    # Wait some time before the next event.
    time.sleep(random.uniform(0.5, 1))

# Stop the audio Server before exiting.
pipe.write("s.stop()\ntime.sleep(0.25)\n")

# Close the subprocess.
pipe.close()

# Delete the python file.
os.remove(script_path)
