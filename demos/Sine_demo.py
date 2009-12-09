def ex1():
    import time
    print """
A simple sine wave:

o_ex = Sine(freq=500, mul=.5).out()
"""
    o_ex = Sine(freq=500, mul=.5).out()

    time.sleep(4)
    o_ex.stop()
    del o_ex

def ex2():
    import time
    print """
Amplitude modulation:

o_ex1 = Sine(freq=10, phase=0, mul=.5, add=.5)
o_ex2 = Sine(500, mul=o_ex1).out()
"""
    o_ex1 = Sine(freq=10, phase=0, mul=.5, add=.5)
    o_ex2 = Sine(freq=500, mul=o_ex1).out()

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2
    
def ex3():
    import time
    print """
Frequency modulation:

o_ex1 = Sine(freq=150, phase=0, mul=600, add=900)
o_ex2 = Sine(freq=o_ex1, mul=.5).out()
"""
    o_ex1 = Sine(freq=150, phase=0, mul=600, add=900)
    o_ex2 = Sine(freq=o_ex1, mul=.5).out()

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex4():
    import time
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

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    o_ex4.stop()
    del o_ex1
    del o_ex2
    del o_ex3
    del o_ex4

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4}

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
        
