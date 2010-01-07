def ex1():
    from pyo import Sine, Noise
    import time
    print """
Multiband envelope:

o_ex1 = Noise(mul=.5)
o_ex2 = Sine(freq=[1.5,2,2.5,3], mul=.5, add=.5)
o_ex3 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2).out()
"""
    o_ex1 = Noise(mul=.5)
    o_ex2 = Sine(freq=[1.5,2,2.5,3], mul=.5, add=.5)
    o_ex3 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2, mul=o_ex2).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del o_ex1
    del o_ex2
    del o_ex3

def ex2():
    from pyo import Sine, SfPlayer
    import time
    print """
Multiband ring modulation:

o_ex1 = SfPlayer("demos/transparent.aif", loop=True, mul=.75)
o_ex2 = Sine(freq=[50,210,600.370])
o_ex3 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2, mul=o_ex2).out()
"""
    o_ex1 = SfPlayer("demos/transparent.aif", loop=True, mul=.75)
    o_ex2 = Sine(freq=[50,210,600.370])
    o_ex3 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2, mul=o_ex2).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del o_ex1
    del o_ex2
    del o_ex3

def ex3():
    from pyo import Disto, SfPlayer
    import time
    print """
Multiband distortion:

o_ex1 = SfPlayer("demos/transparent.aif", loop=True, mul=.5)
o_ex2 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2)
o_ex3 = Disto(o_ex2, drive=[0.98, 0, 0, 0.99], mul=.06125).out()
"""
    o_ex1 = SfPlayer("demos/transparent.aif", loop=True, mul=.5)
    o_ex2 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2)
    o_ex3 = Disto(o_ex2, drive=[0.98, 0, 0, 0.99], mul=.06125).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del o_ex1
    del o_ex2
    del o_ex3

def ex4():
    from pyo import Delay, SfPlayer
    import time
    print """
Multiband delay:

o_ex1 = SfPlayer("demos/transparent.aif", loop=True, mul=.75).out()
o_ex2 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2)
o_ex3 = Delay(o_ex2, delay=[.25, .125, .134, .5], feedback=.75).out()
"""
    o_ex1 = SfPlayer("demos/transparent.aif", loop=True, mul=.75).out()
    o_ex2 = BandSplit(o_ex1, num=4, min=250, max=5000, q=2)
    o_ex3 = Delay(o_ex2, delay=[.25,.125,.134,.5], feedback=.75).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del o_ex1
    del o_ex2
    del o_ex3

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
