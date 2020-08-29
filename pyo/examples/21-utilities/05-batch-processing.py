"""
Offline processing of multiple audio files in batch  

**05-batch-processing.py**

This program demonstrates how to use pyo to do offline batch processing
given a folder of sounds.

"""
import os
from pyo import *

# Initialize the Server in offline mode.
s = Server(audio="offline")

# Path to your sound folder (SNDS_PATH is a folder containing a few sounds installed with pyo,
# you can change this path to try with your own sounds).
folder_path = SNDS_PATH

# Path to the processed sounds folder (user's home directory/pyo_batch_fx/).
output_folder = os.path.join(os.path.expanduser("~"), "pyo_batch_fx")

# create the folder if it does not exist.
if not os.path.isdir(output_folder):
    os.mkdir(output_folder)

# Get the list of audio files to process.
sounds = [f for f in os.listdir(folder_path) if sndinfo(os.path.join(folder_path, f)) is not None]

# Enter the batch processing loop.
for sound in sounds:
    # Retrieve info about the sound from its header.
    path = os.path.join(folder_path, sound)
    info = sndinfo(path)
    dur, sr, chnls = info[1], info[2], info[3]
    fformat = ["WAVE", "AIFF", "AU", "RAW", "SD2", "FLAC", "CAF", "OGG"].index(info[4])
    samptype = [
        "16 bit int",
        "24 bit int",
        "32 bit int",
        "32 bit float",
        "64 bits float",
        "U-Law encoded",
        "A-Law encoded",
    ].index(info[5])

    # Set server parameters according to the current sound info.
    s.setSamplingRate(sr)
    s.setNchnls(chnls)
    s.boot()
    s.recordOptions(
        dur=dur + 1,  # give some room for the reverb trail!
        filename=os.path.join(output_folder, sound),
        fileformat=fformat,
        sampletype=samptype,
    )

    # Simple processing applied to the sound.
    source = SfPlayer(path)
    bandpass = ButBP(source, 1000, 5)
    disto = Disto(bandpass, drive=0.9, slope=0.8)
    output = WGVerb(source + disto, feedback=0.8, cutoff=5000, bal=0.25, mul=0.5).out()

    # Start the rendering.
    s.start()

    # Cleanup for the next pass.
    s.shutdown()
