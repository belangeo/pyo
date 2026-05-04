sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
st_delay = Delay(sf, delay=[.4, .5], feedback=0.7)
st_rev = WGVerb(st_delay, feedback=0.8, cutoff=4000, bal=0.25).out()
