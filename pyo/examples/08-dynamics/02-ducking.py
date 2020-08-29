"""
02-ducking.py - Adjust the gain of a signal based on the presence of another one.

Ducking is an audio effect commonly used in radio. In ducking, the
level of one audio signal is reduced by the presence of another signal.

Here we use a Follower object to track the RMS envelope of the voice signal. Then
we use an audio conditional to create a switch, whose value is 1 when the voice is
talking and 0 when it is silent. This signal is finally used to change the amplitude
of the music whenever the voice is talking.

"""
from pyo import *

s = Server().boot()

# Alternate voice and silence.
table = SndTable(SNDS_PATH + "/transparent.aif")
metro = SDelay(Metro(3).play(), 1)
voice = TrigEnv(metro, table, dur=table.getDur(), mul=0.7)
stvoice = voice.mix(2).out()

# Play some music-box style tune!
freqs = midiToHz([60, 62, 64, 65, 67, 69, 71, 72])
choice = Choice(choice=freqs, freq=[1, 2, 3, 4])
port = Port(choice, risetime=0.001, falltime=0.001)
sines = SineLoop(port, feedback=0.05)
music = SPan(sines, pan=[0, 1, 0.2, 0.8, 0.5], mul=0.1).mix(2)

# Follow voice RMS amplitude.
follow = Follower(voice, freq=10)
# talk = 1 if voice is playing and 0 if not.
talk = follow > 0.005

# Smooth the on/off signal (rising is faster than falling)...
amp = Port(talk, risetime=0.05, falltime=0.1)
# ... then rescale it (1 when no voice and 0.1 when voice is playing).
ampscl = Scale(amp, outmin=1, outmax=0.1)

# Display the gain factor.
sc = Scope(ampscl)

# Apply gain factor and output music.
outsynth = (music * ampscl).out()

s.gui(locals())
