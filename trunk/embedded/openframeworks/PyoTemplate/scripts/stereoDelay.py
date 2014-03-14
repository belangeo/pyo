# Get the input sound and apply a stereo delay + reverb on it.
st_input = Input([0,1])
st_delay = Delay(st_input, delay=[.4, .5], feedback=0.7)
st_rev = WGVerb(st_delay, feedback=0.8, cutoff=4000, bal=0.25).out()

