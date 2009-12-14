def ex1():
    import time
    from pyo import Delay
    print """
Delayed input:

o_ex1 = Input(chnl=0, mul=.5)
o_ex2 = Delay(o_ex1, .250, 0, 1).out()
"""
    o_ex1 = Input(chnl=0, mul=.5)
    o_ex2 = Delay(o_ex1, .250, 0, 1).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

def ex2():
    import time
    from pyo import Delay
    print """
Delayed input with feedback:

o_ex1 = Input(chnl=0, mul=.5)
o_ex2 = Delay(o_ex1, .250, 0.75, 1).out()
"""
    o_ex1 = Input(chnl=0, mul=.5)
    o_ex2 = Delay(o_ex1, .250, 0.75, 1).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2
    
def ex3():
    import time
    from pyo import Sine
    print """
Ring modulation on input signal:

o_ex1 = Sine([150, 300])
o_ex2 = Input(chnl=0, mul=o_ex1).out()
"""
    o_ex1 = Sine([150, 300])
    o_ex2 = Input(chnl=0, mul=o_ex1).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del o_ex1
    del o_ex2

funcs = {1: ex1, 2: ex2, 3: ex3}

print "\n** A Server must be started with duplex mode on !"
while True:
    rep = input("""    
Choose a demo :

Delayed input : 1
Delayed input with feedback : 2
Ring modulation on input : 3
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
