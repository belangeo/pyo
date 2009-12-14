def ex1():
    import time
    print """
A simple white noise:

o_ex = Noise(mul=.5).out()
"""
    o_ex = Noise(mul=.5).out()

    time.sleep(4)
    o_ex.stop()
    del o_ex

def ex2():
    import time
    from pyo import Sine
    print """
Sine frequency controlled with white noise:

o_ex1 = Noise(mul=500, add=1000)
o_ex2 = Sine(o_ex1, 0, .5).out()
"""
    o_ex1 = Noise(mul=500, add=1000)
    o_ex2 = Sine(o_ex1, 0, .5).out()

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2
    
def ex3():
    import time
    from pyo import Biquad
    print """
Band limited noise:

o_ex1 = Noise(mul=.5)
o_ex2 = Biquad(o_ex1, 1000, 5).out()
"""
    o_ex1 = Noise(mul=.5)
    o_ex2 = Biquad(o_ex1, 1000, 5).out()

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2
    
funcs = {1: ex1, 2: ex2, 3: ex3}

print "\n** A Server must be started !"
while True:
    rep = input("""    
Choose a demo :

White noise : 1
Noisy sine : 2
Band limited noise : 3
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
