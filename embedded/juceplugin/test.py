import random

# Get the input sound and apply a stereo delay on it.
feed = SigTo(0, 0.05, 0)
freq = SigTo(0.1, 0.05, 0.1)
freq_sc = Scale(freq, 0, 1, 100, 10000, 4)
st_input = Input([0,1])
st_filter = ButBP(st_input, freq_sc)
st_delay = Delay(st_filter, delay=[random.uniform(.1, .9) for i in range(2)], feedback=feed).out()
