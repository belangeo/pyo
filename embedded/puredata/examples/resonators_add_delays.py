deltime = SigTo(.25, 0.005, 0.25)
delfeed = SigTo(.25, 0.005, 0.25)
dd = Delay(wgs, delay=deltime, feedback=delfeed).out(0)
