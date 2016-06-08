"""
Music box - Four voices randomly choosing frequencies 
over a common scale.

"""
# Set to True to control the global gain with analog input 0.
WITH_ANALOG_INPUT = False

if WITH_ANALOG_INPUT:
    v = Tone(Input(2), 8) # knob 1 - global gain
else:
    v = 0.5

mid_freqs = [midiToHz(m+7) for m in [60,62,63.93,65,67.01,69,71,72]]
high_freqs = [midiToHz(m+7) for m in [72,74,75.93,77,79.01]]
freqs = [mid_freqs,mid_freqs,high_freqs,high_freqs]

chx = Choice(choice=freqs, freq=[2,3,3,4])
port = Port(chx, risetime=.001, falltime=.001)
sines = SineLoop(port, feedback=[.057,.033,.035,.016], 
		         mul=[.1,.1,.05,.05]*v).out()

