def ex1():
    from pyo import Noise, Clean_objects
    print """
Lowpass filter:

o_ex1 = Noise(mul=.5)
o_ex2 = Biquad(o_ex1, freq=1000, q=5, type=0).out()
"""
    o_ex1 = Noise(mul=.5)
    o_ex2 = Biquad(o_ex1, freq=1000, q=5, type=0).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()


def ex2():
    from pyo import Noise, Clean_objects
    print """
Highpass filter:

o_ex1 = Noise(mul=.25)
o_ex2 = Biquad(o_ex1, freq=3000, q=5, type=1).out()
"""
    o_ex1 = Noise(mul=.25)
    o_ex2 = Biquad(o_ex1, freq=3000, q=5, type=1).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex3():
    from pyo import Noise, Clean_objects
    print """
Bandpass filter bank:

o_ex1 = Noise(mul=.75)
o_ex2 = Biquad(o_ex1, freq=[250,500,1000,2000,4000], q=10, type=2).out()
"""
    o_ex1 = Noise(mul=.75)
    o_ex2 = Biquad(o_ex1, freq=[250,500,1000,2000,4000], q=10, type=2).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()

def ex4():
    from pyo import Noise, Sine, Clean_objects
    print """
Bandpass filter with moving frequency:

o_ex1 = Noise()
o_ex2 = Sine(freq=1, mul=1000, add=1000)
o_ex3 = Biquad(o_ex1, freq=o_ex2, q=5, type=2).out()
"""
    o_ex1 = Noise()
    o_ex2 = Sine(freq=1, mul=1000, add=1000)
    o_ex3 = Biquad(o_ex1, freq=o_ex2, q=5, type=2).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

def ex5():
    from pyo import Noise, Sine, Clean_objects
    print """
Bandpass filter with moving Q:

o_ex1 = Noise()
o_ex2 = Sine(freq=1, mul=10, add=11)
o_ex3 = Biquad(o_ex1, freq=1000, q=o_ex2, type=2).out()
"""
    o_ex1 = Noise()
    o_ex2 = Sine(freq=1, mul=10, add=11)
    o_ex3 = Biquad(o_ex1, freq=1000, q=o_ex2, type=2).out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4, 5: ex5}

while True:
    rep = input("""    
Choose a demo :

Lowpass filter : 1
Highpass filter : 2
Bandpass filter bank : 3
Moving center frequency : 4
Moving Q : 5
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
