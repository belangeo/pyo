def ex1():
    import time
    from pyo import LinTable, TrigEnv, Noise
    print """
Simple periodic trigger:

o_ex1 = Metro(time=.5).play()
t_ex1 = LinTable([(0,0), (10,1), (8191,0)])
o_ex2 = TrigEnv(o_ex1, t_ex1, dur=.5, mul=.25)
o_ex3 = Noise(mul=o_ex2).out()
"""
    o_ex1 = Metro(time=.5).play()
    t_ex1 = LinTable([(0,0), (10,1), (8191,0)])
    o_ex2 = TrigEnv(o_ex1, t_ex1, dur=.5, mul=.25)
    o_ex3 = Noise(mul=o_ex2).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del t_ex1
    del o_ex1
    del o_ex2
    del o_ex3

def ex2():
    import time
    from pyo import Delay
    print """
Metro as impulse sequence:

o_ex1 = Metro(time=.125).play()
o_ex2 = Delay(o_ex1, delay=.005, feedback=.99, mul=.5).out()
"""
    o_ex1 = Metro(time=.125).play()
    o_ex2 = Delay(o_ex1, delay=.005, feedback=.99, mul=.5).out()

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex3():
    import time
    from pyo import Delay
    print """
Polyphonic impulse sequences:

o_ex1 = Metro(time=.125, poly=4).play()
o_ex2 = Delay(o_ex1, delay=[.001, .002, .004, .006], feedback=.99, mul=.5).out()
"""
    o_ex1 = Metro(time=.125, poly=4).play()
    o_ex2 = Delay(o_ex1, delay=[.001, .002, .004, .006], feedback=.99, mul=.5).out()

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex4():
    import time
    from pyo import Biquad, Sine
    print """
Impulse train:

o_ex1 = Metro(time=.005).play()
o_ex2 = Sine(freq=.5, mul=1500, add=2000)
o_ex3 = Biquad(input=o_ex1, freq=o_ex2, q=5, type=0, mul=.5).out()
"""
    o_ex1 = Metro(time=.005).play()
    o_ex2 = Sine(freq=.5, mul=1500, add=2000)
    o_ex3 = Biquad(input=o_ex1, freq=o_ex2, q=5, type=0, mul=.5).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del o_ex1
    del o_ex2
    del o_ex3

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4}

while True:
    rep = input("""    
Choose a demo :

Simple periodic trigger : 1
Impulse sequence : 2
Polyphonic impulse sequences : 3
Impulse train : 4
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
