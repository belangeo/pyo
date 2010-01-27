def ex1():
    from pyo import Clean_objects
    print """
A simple sine wave:

o_ex = Sine(freq=500, mul=.5).out()
"""
    o_ex = Sine(freq=500, mul=.5).out()

    c = Clean_objects(4, o_ex)
    c.start()

def ex2():
    from pyo import Clean_objects
    print """
Amplitude modulation:

o_ex1 = Sine(freq=10, phase=0, mul=.5, add=.5)
o_ex2 = Sine(500, mul=o_ex1).out()
"""
    o_ex1 = Sine(freq=10, phase=0, mul=.5, add=.5)
    o_ex2 = Sine(freq=500, mul=o_ex1).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()
    
def ex3():
    from pyo import Clean_objects
    print """
Frequency modulation:

o_ex1 = Sine(freq=150, phase=0, mul=600, add=900)
o_ex2 = Sine(freq=o_ex1, mul=.5).out()
"""
    o_ex1 = Sine(freq=150, phase=0, mul=600, add=900)
    o_ex2 = Sine(freq=o_ex1, mul=.5).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex4():
    from pyo import Clean_objects
    print """
Frequency, phase and ring modulation:

o_ex1 = Sine(freq=10, phase=0, mul=600, add=900)
o_ex2 = Sine(freq=101, phase=0, mul=.5, add=.5)
o_ex3 = Sine(freq=49, phase=0, mul=.5)
o_ex4 = Sine(freq=o_ex1, phase=o_ex2, mul=o_ex3).out()
"""
    o_ex1 = Sine(freq=10, phase=0, mul=600, add=900)
    o_ex2 = Sine(freq=101, phase=0, mul=.5, add=.5)
    o_ex3 = Sine(freq=49, phase=0, mul=.5)
    o_ex4 = Sine(freq=o_ex1, phase=o_ex2, mul=o_ex3).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, o_ex4)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4}

print "\n** A Server must be started !"
while True:
    rep = input("""    
Choose a demo :

Simple sine : 1
Amplitude modulation : 2
Frequency modulation : 3
Multi modulation : 4
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
