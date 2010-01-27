def ex1():
    from pyo import SfPlayer, Clean_objects
    print """
Simple delay:

o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5).out()
o_ex2 = Delay(o_ex1, .250, 0, 1).out()
"""
    o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5).out()
    o_ex2 = Delay(o_ex1, .250, 0, 1).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex2():
    from pyo import SfPlayer, Clean_objects
    print """
Delay with feedback:

o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5).out()
o_ex2 = Delay(o_ex1, .250, 0.75, 1).out()
"""
    o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5).out()
    o_ex2 = Delay(o_ex1, .250, 0.75, 1).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex3():
    from pyo import Sine, SfPlayer, Clean_objects
    print """
Basic chorus:

o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5).out()
o_ex2 = Sine(freq=[1,1.34], mul=0.005, add=0.025)
o_ex3 = Delay(input=o_ex1, delay=o_ex2, feedback=0.5, mul=.3).out()
"""
    o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5).out()
    o_ex2 = Sine(freq=[1,1.34], mul=0.005, add=0.025)
    o_ex3 = Delay(input=o_ex1, delay=o_ex2, feedback=0.5, mul=.3).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

def ex4():
    from pyo import Sine, SfPlayer, Clean_objects
    print """
Flanger:

o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5).out()
o_ex2 = Sine(freq=.3, mul=0.003, add=0.004)
o_ex3 = Delay(input=o_ex1, delay=o_ex2, feedback=0.65).out()
"""
    o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.5).out()
    o_ex2 = Sine(freq=.3, mul=0.003, add=0.004)
    o_ex3 = Delay(input=o_ex1, delay=o_ex2, feedback=0.65).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

def ex5():
    from pyo import SfPlayer, Clean_objects
    print """
Resonator bank:

o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.025)
o_ex2 = Delay(o_ex1, delay=[.004, .0079999, .012002], feedback=0.99).out()
"""
    o_ex1 = SfPlayer('demos/transparent.aif', loop=True, mul=.025)
    o_ex2 = Delay(o_ex1, delay=[.004, .0079999, .012002], feedback=0.99).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4, 5: ex5}

while True:
    rep = input("""    
Choose a demo :

Simple delay : 1
Delay with feedback : 2
Basic chorus : 3
Flanger : 4
Resonator bank : 5
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
