def ex1():
    from pyo import Clean_objects
    print """
A simple white noise:

o_ex = Noise(mul=.5).out()
"""
    o_ex = Noise(mul=.5).out()

    c = Clean_objects(4, o_ex)
    c.start()

def ex2():
    from pyo import Sine, Clean_objects
    print """
Sine frequency controlled with white noise:

o_ex1 = Noise(mul=500, add=1000)
o_ex2 = Sine(o_ex1, 0, .5).out()
"""
    o_ex1 = Noise(mul=500, add=1000)
    o_ex2 = Sine(o_ex1, 0, .5).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()
    
def ex3():
    from pyo import Biquad, Clean_objects
    print """
Band limited noise:

o_ex1 = Noise(mul=.5)
o_ex2 = Biquad(o_ex1, 1000, 5).out()
"""
    o_ex1 = Noise(mul=.5)
    o_ex2 = Biquad(o_ex1, 1000, 5).out()

    c = Clean_objects(4, o_ex1, o_ex2)
    c.start()
    
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
        
