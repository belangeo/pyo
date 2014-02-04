bal = SigTo(0.25, 0.005, 0.25)
ins = Input([0,1])
dist = CvlVerb(ins, size=512, bal=bal, mul=0.3).out()