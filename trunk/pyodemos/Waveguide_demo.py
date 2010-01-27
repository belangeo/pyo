def ex1():
    from pyo import LinTable, Metro, TrigEnv, Clean_objects
    print """
Simple plucked string:

t_ex1 = LinTable([(0,0), (2,1), (5,0), (8191,0)])
o_ex1 = Metro().play()
o_ex2 = TrigEnv(o_ex1, t_ex1, 1)
o_ex3 = Waveguide(o_ex2, freq=[200,200.5], dur=20, minfreq=20, mul=.5).out()
"""
    t_ex1 = LinTable([(0,0), (2,1), (5,0), (8191,0)])
    o_ex1 = Metro().play()
    o_ex2 = TrigEnv(o_ex1, t_ex1, 1)
    o_ex3 = Waveguide(o_ex2, freq=[200,200.5], dur=20, minfreq=20, mul=.5).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, t_ex1)
    c.start()

def ex2():
    from pyo import LinTable, Metro, TrigEnv, Sine, Clean_objects
    print """
Simple plucked string, with vibrato:

t_ex1 = LinTable([(0,0), (2,1), (5,0), (8191,0)])
o_ex1 = Metro().play()
o_ex2 = TrigEnv(o_ex1, t_ex1, 1)
o_ex3 = Sine(3, 0, 3, [200, 200.5])
o_ex4 = Waveguide(o_ex2, freq=o_ex3, dur=20, minfreq=20, mul=.5).out()
"""
    t_ex1 = LinTable([(0,0), (2,1), (5,0), (8191,0)])
    o_ex1 = Metro().play()
    o_ex2 = TrigEnv(o_ex1, t_ex1, 1)
    o_ex3 = Sine(3, 0, 3, [200, 200.5])
    o_ex4 = Waveguide(o_ex2, freq=o_ex3, dur=20, minfreq=20, mul=.5).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, o_ex4, t_ex1)
    c.start()

def ex3():
    from pyo import LinTable, Metro, TrigEnv, Clean_objects
    print """
Arpeggio:

t_ex1 = LinTable([(0,0), (2,1), (5,0), (8191,0)])
o_ex1 = Metro(time=.125, poly=8).play()
o_ex2 = TrigEnv(o_ex1, t_ex1, 1)
o_ex3 = Waveguide(o_ex2, freq=range(100, 500, 50), dur=20, minfreq=20, mul=.5).out()
"""
    t_ex1 = LinTable([(0,0), (2,1), (5,0), (8191,0)])
    o_ex1 = Metro(time=.125, poly=8).play()
    o_ex2 = TrigEnv(o_ex1, t_ex1, 1)
    o_ex3 = Waveguide(o_ex2, freq=range(100, 500, 50), dur=20, minfreq=20, mul=.5).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, t_ex1)
    c.start()

def ex4():
    from pyo import SfPlayer, Clean_objects
    print """
Resonator bank:

o_ex1 = SfPlayer('pyodemos/transparent.aif', loop=True, mul=.025)
o_ex2 = Waveguide(o_ex1, freq=range(100, 500, 50), dur=60).out()
"""
    o_ex1 = SfPlayer('pyodemos/transparent.aif', loop=True, mul=.025)
    o_ex2 = Waveguide(o_ex1, freq=range(100, 500, 50), dur=60).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4}

while True:
    rep = input("""    
Choose a demo :

Simple plucked string : 1
Plucked string with vibrato : 2
Arpeggio : 3
Resonator bank : 4
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
