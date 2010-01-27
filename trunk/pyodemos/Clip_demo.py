def ex1():
    from pyo import SfPlayer, Clean_objects, DEMOS_PATH
    print """
Simple clipping (cheap distortion):

o_ex1 = SfPlayer(DEMOS_PATH + '/transparent.aif', loop=True)
o_ex2 = Clip(o_ex1, min=-.15, max=.15).out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + '/transparent.aif', loop=True)
    o_ex2 = Clip(o_ex1, min=-.15, max=.15).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex2():
    from pyo import Sine, Clean_objects
    print """
Frequency clipping:

o_ex1 = Sine(freq=.75, mul=500, add=700)
o_ex2 = Clip(o_ex2, min=400, max=800)
o_ex3 = Sine(freq=o_ex2, mul=.5).out()
"""
    o_ex1 = Sine(freq=.5, mul=500, add=700)
    o_ex2 = Clip(o_ex1, min=400, max=800)
    o_ex3 = Sine(freq=o_ex2, mul=.5).out()
    
    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

def ex3():
    from pyo import Sine, Phasor, Clean_objects
    print """
Frequency clipping with moving boundary:

o_ex1 = Sine(freq=10, mul=500, add=700)
o_ex2 = Phasor(freq=.25, mul=500, add=500)
o_ex3 = Clip(o_ex1, min=400, max=o_ex2)
o_ex4 = Sine(freq=o_ex3, mul=.5).out()
"""
    o_ex1 = Sine(freq=10, mul=500, add=700)
    o_ex2 = Phasor(freq=.25, mul=500, add=500)
    o_ex3 = Clip(o_ex1, min=400, max=o_ex2)
    o_ex4 = Sine(freq=o_ex3, mul=.5).out()
    
    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, o_ex4)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3}

while True:
    rep = input("""    
Choose a demo :

Simple clipping : 1
Frequency clipping : 2
Moving boundary : 3
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break

