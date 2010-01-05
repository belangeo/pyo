def ex1():
    import time
    from pyo import Noise
    print """
Lowpass filter:

o_ex1 = Noise(mul=.5)
o_ex2 = Biquad(o_ex1, freq=1000, q=5, type=0).out()
"""
    o_ex1 = Noise(mul=.5)
    o_ex2 = Biquad(o_ex1, freq=1000, q=5, type=0).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex2():
    import time
    from pyo import Noise
    print """
Highpass filter:

o_ex1 = Noise(mul=.25)
o_ex2 = Biquad(o_ex1, freq=3000, q=5, type=1).out()
"""
    o_ex1 = Noise(mul=.25)
    o_ex2 = Biquad(o_ex1, freq=3000, q=5, type=1).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex3():
    import time
    from pyo import Noise
    print """
Bandpass filter bank:

o_ex1 = Noise(mul=.75)
o_ex2 = Biquad(o_ex1, freq=[250,500,1000,2000,4000], q=10, type=2).out()
"""
    o_ex1 = Noise(mul=.75)
    o_ex2 = Biquad(o_ex1, freq=[250,500,1000,2000,4000], q=10, type=2).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex4():
    import time
    from pyo import Noise, Sine
    print """
Bandpass filter with moving frequency:

o_ex1 = Noise()
o_ex2 = Sine(freq=1, mul=1000, add=1000)
o_ex3 = Biquad(o_ex1, freq=o_ex2, q=5, type=2).out()
"""
    o_ex1 = Noise()
    o_ex2 = Sine(freq=1, mul=1000, add=1000)
    o_ex3 = Biquad(o_ex1, freq=o_ex2, q=5, type=2).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del o_ex1
    del o_ex2
    del o_ex3

def ex5():
    import time
    from pyo import Noise, Sine
    print """
Bandpass filter with moving Q:

o_ex1 = Noise()
o_ex2 = Sine(freq=1, mul=10, add=11)
o_ex3 = Biquad(o_ex1, freq=1000, q=o_ex2, type=2).out()
"""
    o_ex1 = Noise()
    o_ex2 = Sine(freq=1, mul=10, add=11)
    o_ex3 = Biquad(o_ex1, freq=1000, q=o_ex2, type=2).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    del o_ex1
    del o_ex2
    del o_ex3

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
        
