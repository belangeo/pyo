def ex1():
    from pyo import Sine, Noise, Clean_objects
    print """
Multiband envelope:

o_ex1 = Noise(mul=.5)
o_ex2 = Sine(freq=[1.5,2,2.5,3], mul=.5, add=.5)
o_ex3 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2).out()
"""
    o_ex1 = Noise(mul=.5)
    o_ex2 = Sine(freq=[1.5,2,2.5,3], mul=.5, add=.5)
    o_ex3 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2, mul=o_ex2).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

def ex2():
    from pyo import Sine, SfPlayer, Clean_objects, DEMOS_PATH
    print """
Multiband ring modulation:

o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True, mul=.75)
o_ex2 = Sine(freq=[50,210,600.370])
o_ex3 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2, mul=o_ex2).out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True, mul=.75)
    o_ex2 = Sine(freq=[50,210,600.370])
    o_ex3 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2, mul=o_ex2).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

def ex3():
    from pyo import Disto, SfPlayer, Clean_objects, DEMOS_PATH
    print """
Multiband distortion:

o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True, mul=.5)
o_ex2 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2)
o_ex3 = Disto(o_ex2, drive=[0.98, 0, 0, 0.99], mul=.06125).out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True, mul=.5)
    o_ex2 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2)
    o_ex3 = Disto(o_ex2, drive=[0.98, 0, 0, 0.99], mul=.06125).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

def ex4():
    from pyo import Delay, SfPlayer, Clean_objects, DEMOS_PATH
    print """
Multiband delay:

o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True, mul=.75).out()
o_ex2 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2)
o_ex3 = Delay(o_ex2, delay=[.25, .125, .134, .5], feedback=.75).out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True, mul=.75).out()
    o_ex2 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2)
    o_ex3 = Delay(o_ex2, delay=[.25,.125,.134,.5], feedback=.75).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4}

print "\n** A Server must be started !"
while True:
    rep = input("""
Choose a demo :

Multiband envelope : 1
Multiband ring mod : 2
Multiband distortion : 3
Multiband delay : 4
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
