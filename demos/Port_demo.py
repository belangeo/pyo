def ex1():
    from pyo import Sine, Metro, TrigRand, Clean_objects
    print """
Frequency portamento:

o_ex1 = Metro(time=.25).play()
o_ex2 = TrigRand(o_ex1, min=400, max=800)
o_ex3 = Port(o_ex2, risetime=.125, falltime=.125)
o_ex4 = Sine(freq=o_ex3, mul=.25).out()
"""
    o_ex1 = Metro(time=.25).play()
    o_ex2 = TrigRand(o_ex1, min=400, max=800)
    o_ex3 = Port(o_ex2, risetime=.125, falltime=.125)
    o_ex4 = Sine(freq=o_ex3, mul=.25).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, o_ex4)
    c.start()

def ex2():
    from pyo import Metro, Noise, Clean_objects
    print """
Amplitude envelope:

o_ex1 = Metro(time=.25).play()
o_ex2 = Port(o_ex1, risetime=.002, falltime=.2, mul=5)
o_ex3 = Noise(mul=o_ex2).out()
"""
    o_ex1 = Metro(time=.25).play()
    o_ex2 = Port(o_ex1, risetime=.002, falltime=.2, mul=5)
    o_ex3 = Noise(mul=o_ex2).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()
    
def ex3():
    from pyo import Sine, Clean_objects
    print """
Distortion filter:

o_ex1 = Sine(freq=500, mul=.5)
o_ex2 = Port(o_ex1, risetime=.001, falltime=.01).out()
"""
    o_ex1 = Sine(freq=500, mul=.5)
    o_ex2 = Port(o_ex1, risetime=.001, falltime=.01).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()
  
funcs = {1: ex1, 2: ex2, 3: ex3}

print "\n** A Server must be started !"
while True:
    rep = input("""    
Choose a demo :

Frequency portamento : 1
Amplitude envelope : 2
Distortion filter : 3
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
