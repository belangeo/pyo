"""
Applying digital signal processing at a higher sampling rate

**07-upsamp-processing.py**

This script demonstrates how to use pyo to apply processing on a sound
at a very high sampling rate.

Pyo offers two functions to upsample and downsample an audio file::

    upsamp(path, outfile, up, order)
    downsamp(path, outfile, down, order)

Where:

    - `path` is the path to the file to process.
    - `outfile` is the path where to save the resampled file.
    - `up`, `down` is the resampling factor (new sampling rate will be
      this value times the current sampling rate).
    - `order` is the length, in samples, of the anti-aliasing lowpass filter.

"""
import os
from pyo import *

# Upsampling factor.
SR_FACTOR = 8

# Size, in samples, of the anti-aliasing lowpass filters.
SIZE = 512

# Sound to process.
IN_SND = "../snds/baseballmajeur_m.aif"

# Temporary audio files.
UP_SND = os.path.join(os.path.expanduser("~"), "Desktop", "temp_sound_up_%i.aif" % SR_FACTOR)
PROC_SND = os.path.join(os.path.expanduser("~"), "Desktop", "temp_sound_disto.aif")

# Output sound file.
DOWN_SND = os.path.join(os.path.expanduser("~"), "Desktop", "output_disto_down.aif")

# Retrieve info about the source file.
DUR = sndinfo(IN_SND)[1]
SR = sndinfo(IN_SND)[2]

print("Up sampling the source file %i times..." % SR_FACTOR)
upsamp(IN_SND, UP_SND, SR_FACTOR, SIZE)

print("Apply distortion at a sampling rate of %i Hz." % (SR * SR_FACTOR))

# Initialize the Server in offline mode, at the desired sampling rate.
s = Server(sr=SR * SR_FACTOR, nchnls=1, duplex=0, audio="offline").boot()

# Set the recording parameters.
s.recordOptions(dur=DUR + 0.1, filename=PROC_SND, fileformat=0, sampletype=0)

# Read the upsampled version of the source file.
sf = SfPlayer(UP_SND, loop=False, interp=4, mul=0.7)
# Apply distortion on it.
dist = Disto(sf, drive=0.75, slope=0.7, mul=0.3)
# Lowpass filtering.
filt = Biquad(dist, freq=8000, q=0.7, type=0).out()

# Start the offline processing.
s.start()

print("Down sampling the processed sound %i times..." % SR_FACTOR)
downsamp(PROC_SND, DOWN_SND, SR_FACTOR, SIZE)

# Cleanup the temporary files.
os.remove(UP_SND)
os.remove(PROC_SND)

print("Done")
