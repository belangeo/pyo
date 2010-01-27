def ex1():
    from pyo import Noise, Clean_objects
    print """
Amplitude fadein and fadeout:

o_ex1 = Fader(fadein=.75, fadeout=.75, dur=4, mul=.5).play()
o_ex2 = Noise(mul=o_ex1).out()
"""
    o_ex1 = Fader(fadein=.75, fadeout=.75, dur=4, mul=.5).play()
    o_ex2 = Noise(mul=o_ex1).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex2():
    from pyo import Noise, Clean_objects
    print """
Amplitude fadein and dur = 0
wait for the stop() method to start fadeout:

o_ex1 = Fader(fadein=.75, fadeout=.75, dur=0, mul=.5).play()
o_ex2 = Noise(mul=o_ex1).out()
"""
    o_ex1 = Fader(fadein=.75, fadeout=.75, dur=0, mul=.5).play()
    o_ex2 = Noise(mul=o_ex1).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()
    
def ex3():
    from pyo import Sine, Clean_objects
    print """
Frequency attack and decay:

o_ex1 = Fader(fadein=.15, fadeout=.15, dur=4, mul=100, add=900).play()
o_ex2 = Fader(fadein=.05, fadeout=.05, dur=4, mul=.5).play()
o_ex3 = Sine(freq=o_ex1, mul=o_ex2).out()
"""
    o_ex1 = Fader(fadein=.15, fadeout=.15, dur=4, mul=100, add=900).play()
    o_ex2 = Fader(fadein=.05, fadeout=.05, dur=4, mul=.5).play()
    o_ex3 = Sine(freq=o_ex1, mul=o_ex2).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()
   
funcs = {1: ex1, 2: ex2, 3: ex3}

print "\n** A Server must be started !"
while True:
    rep = input("""    
Choose a demo :

Fadein and fadeout : 1
Fadein only : 2
Frequency attack and decay : 3
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
