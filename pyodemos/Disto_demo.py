def ex1():
    from pyo import SfPlayer, Clean_objects, DEMOS_PATH
    print """
Soft distortion:

o_ex1 = SfPlayer(DEMOS_PATH + '/transparent.aif', loop=True, mul=.25)
o_ex2 = Disto(o_ex1, drive=.25, slope=.7).out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + '/transparent.aif', loop=True, mul=.25)
    o_ex2 = Disto(o_ex1, drive=.25, slope=.7).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex2():
    from pyo import SfPlayer, Clean_objects, DEMOS_PATH
    print """
Hard distortion:

o_ex1 = SfPlayer(DEMOS_PATH + '/transparent.aif', loop=True, mul=.5)
o_ex2 = Disto(o_ex1, drive=.98, slope=.2, mul=.15).out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + '/transparent.aif', loop=True, mul=.5)
    o_ex2 = Disto(o_ex1, drive=.98, slope=.2, mul=.15).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex3():
    from pyo import Sine, SfPlayer, Clean_objects, DEMOS_PATH
    print """
DIstortion with moving drive:

o_ex1 = SfPlayer(DEMOS_PATH + '/transparent.aif', loop=True, mul=.5)
o_ex2 = Sine(freq=0.5, mul=0.499, add=0.5)
o_ex3 = Disto(input=o_ex1, drive=o_ex2, slope=.5, mul=.25).out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + '/transparent.aif', loop=True, mul=.5)
    o_ex2 = Sine(freq=0.5, mul=0.499, add=0.5)
    o_ex3 = Disto(input=o_ex1, drive=o_ex2, slope=.5, mul=.25).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3}

while True:
    rep = input("""    
Choose a demo :

Soft distortion : 1
Hard distortion : 2
Moving drive : 3
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break

