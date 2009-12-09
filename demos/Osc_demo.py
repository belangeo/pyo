def ex1():
    from pyo import HarmTable
    import time
    print """
A simple square wave:

t_ex = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
o_ex = Osc(table=t_ex, freq=500, mul=.5).out()
"""
    t_ex = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    o_ex = Osc(table=t_ex, freq=500, mul=.5).out()

    time.sleep(4)
    o_ex.stop()
    del t_ex
    del o_ex

def ex2():
    from pyo import HannTable, HarmTable
    import time
    print """
Envelope looping:

t_ex1 = HannTable()
t_ex2 = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
o_ex1 = Osc(table=t_ex1, freq=4, mul=.5)
o_ex2 = Osc(table=t_ex2, freq=500, mul=o_ex1).out()
"""
    t_ex1 = HannTable()
    t_ex2 = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    o_ex1 = Osc(table=t_ex1, freq=4, mul=.5)
    o_ex2 = Osc(table=t_ex2, freq=500, mul=o_ex1).out()

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    del t_ex1
    del t_ex2    
    del o_ex1
    del o_ex2
    
def ex3():
    from pyo import HarmTable
    import time
    print """
Complex ring modulation:

t_ex1 = HarmTable([1,0,0,.2,0,0,.1,0,0,.05])
t_ex2 = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
o_ex1 = Osc(table=t_ex1, freq=100, mul=.5)
o_ex2 = Osc(table=t_ex2, freq=500, mul=o_ex1).out()
"""
    t_ex1 = HarmTable([1,0,0,.2,0,0,.1,0,0,.05])
    t_ex2 = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    o_ex1 = Osc(table=t_ex1, freq=100, mul=.5)
    o_ex2 = Osc(table=t_ex2, freq=500, mul=o_ex1).out()

    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    del t_ex1
    del t_ex2    
    del o_ex1
    del o_ex2

def ex4():
    from pyo import HannTable, HarmTable
    import time
    print """
Overlapping envelopes:

t_ex = HannTable()
t_ex1 = HarmTable([1,0,0,.2,0,0,.1,0,0,.05])
env_ex1 = Osc(table=t_ex, freq=2, phase=0)
o_ex1 = Osc(table=t_ex1, freq=300, mul=env_ex1).out()
env_ex2 = Osc(table=t_ex, freq=2, phase=0.5)
o_ex2 = Osc(table=t_ex1, freq=400, mul=env_ex2).out()
"""
    t_ex = HannTable()
    t_ex1 = HarmTable([1,0,0,.2,0,0,.1,0,0,.05])
    env_ex1 = Osc(table=t_ex, freq=2, phase=0)
    o_ex1 = Osc(table=t_ex1, freq=300, mul=env_ex1).out()
    env_ex2 = Osc(table=t_ex, freq=2, phase=0.5)
    o_ex2 = Osc(table=t_ex1, freq=400, mul=env_ex2).out()

    time.sleep(5)
    o_ex1.stop()
    o_ex2.stop()
    del t_ex
    del t_ex1    
    del env_ex1
    del env_ex2
    del o_ex1
    del o_ex2

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4}

while True:
    rep = input("""
Choose a demo :

Simple square wave : 1
Envelope looping : 2
Complex ring modulation : 3
Overlapping envelopes : 4
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
