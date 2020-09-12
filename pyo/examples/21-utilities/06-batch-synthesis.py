"""
Offline batch generation of synthesis sounds  

**06-batch-synthesis.py**

This program demonstrates how to use pyo to do offline synthesis batch
generation.

"""
import os
from pyo import *

# Initialize the Server in offline mode.
s = Server(duplex=0, audio="offline")

# Path to the output sound folder (user's home directory/pyo_batch_synth/).
output_folder = os.path.join(os.path.expanduser("~"), "pyo_batch_synth")

# Create the folder if it does not exist.
if not os.path.isdir(output_folder):
    os.mkdir(output_folder)

# Output file duration.
dur = 2

# How many file to generate.
NUM = 12

# Enter the batch synthesis loop.
for i in range(NUM):
    # Boot the Server.
    s.boot()

    # Define the pitch of the note.
    note = 60 + i
    noteFreq = midiToHz(note)

    # Set recording parameters.
    s.recordOptions(
        dur=dur + 0.1, filename=os.path.join(output_folder, "file_%02d.wav" % note), fileformat=0, sampletype=0,
    )

    # Synthesis, an exponential envelope modulating a slightly out-of-tune stereo oscillator.
    env = Adsr(attack=0.005, decay=0.15, sustain=0.7, release=1.7, dur=dur).play()
    qenv = Pow(env, 4, mul=0.8)
    osc1 = SineLoop(freq=noteFreq, feedback=0.075, mul=qenv).out()
    osc2 = SineLoop(freq=noteFreq * 1.01, feedback=0.075, mul=qenv).out(1)

    # Start thw rendering.
    s.start()

    # Cleanup for the next pass.
    s.shutdown()

print("Batch processing done!")
