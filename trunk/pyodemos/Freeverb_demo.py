def ex1():
    from pyo import SfPlayer, Clean_objects
    print """
Small romm:

o_ex1 = SfPlayer('pyodemos/transparent.aif', loop=True, mul=.5)
o_ex2 = Freeverb(o_ex1, size=.25, damp=.7, bal=.4).out()
"""
    o_ex1 = SfPlayer('pyodemos/transparent.aif', loop=True, mul=.5)
    o_ex2 = Freeverb(o_ex1, size=.25, damp=.7, bal=.4).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex2():
    from pyo import SfPlayer, Clean_objects
    print """
Big room:

o_ex1 = SfPlayer('pyodemos/transparent.aif', loop=True, mul=.5)
o_ex2 = Freeverb(o_ex1, size=.95, damp=1, bal=.5).out()
"""
    o_ex1 = SfPlayer('pyodemos/transparent.aif', loop=True, mul=.5)
    o_ex2 = Freeverb(o_ex1, size=.95, damp=1, bal=.5).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex3():
    from pyo import Sine, SfPlayer, Clean_objects
    print """
Reverb with changing room size:

o_ex1 = SfPlayer('pyodemos/transparent.aif', loop=True, mul=.5)
o_ex2 = Sine(freq=0.75, mul=0.5, add=0.5)
o_ex3 = Freeverb(o_ex1, size=o_ex2, damp=1, bal=.5).out()
"""
    o_ex1 = SfPlayer('pyodemos/transparent.aif', loop=True, mul=.5)
    o_ex2 = Sine(freq=0.75, mul=0.5, add=0.5)
    o_ex3 = Freeverb(o_ex1, size=o_ex2, damp=1, bal=.5).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3}

while True:
    rep = input("""    
Choose a demo :

Small room : 1
Big room : 2
Changing room size : 3
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break

