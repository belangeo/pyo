def ex1():
    import time
    from pyo import SfPlayer
    print """
Soft distortion:

o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.25)
o_ex2 = Disto(o_ex1, drive=.25, slope=.7).out()
"""
    o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.25)
    o_ex2 = Disto(o_ex1, drive=.25, slope=.7).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex2():
    import time
    from pyo import SfPlayer
    print """
Hard distortion:

o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5)
o_ex2 = Disto(o_ex1, drive=.98, slope=.2, mul=.15).out()
"""
    o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5)
    o_ex2 = Disto(o_ex1, drive=.98, slope=.2, mul=.15).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex3():
    import time
    from pyo import Sine, SfPlayer
    print """
DIstortion with moving drive:

o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5)
o_ex2 = Sine(freq=0.5, mul=0.499, add=0.5)
o_ex3 = Disto(input=o_ex1, drive=o_ex2, slope=.5, mul=.25).out()
"""
    o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5)
    o_ex2 = Sine(freq=0.5, mul=0.499, add=0.5)
    o_ex3 = Disto(input=o_ex1, drive=o_ex2, slope=.5, mul=.25).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del o_ex1
    del o_ex2
    del o_ex3

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

