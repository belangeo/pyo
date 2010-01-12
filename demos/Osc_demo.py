def ex1():
    from pyo import HarmTable, Clean_objects
    print """
A simple square wave:

t_ex = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
o_ex = Osc(table=t_ex, freq=500, mul=.5).out()
"""
    t_ex = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    o_ex = Osc(table=t_ex, freq=500, mul=.5).out()

    c = Clean_objects(4, o_ex, t_ex)
    c.start()

def ex2():
    from pyo import HannTable, HarmTable, Clean_objects
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

    c = Clean_objects(4, o_ex1, o_ex2, t_ex1, t_ex2)
    c.start()
    
def ex3():
    from pyo import HarmTable, Clean_objects
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

    c = Clean_objects(4, o_ex1, o_ex2, t_ex1, t_ex2)
    c.start()


def ex4():
    from pyo import HannTable, HarmTable, Clean_objects
    print """
Overlapping envelopes:

t_ex = HannTable()
t_ex1 = HarmTable([1,0,0,.2,0,0,.1,0,0,.05])
env_ex1 = Osc(table=t_ex, freq=2, phase=0)
o_ex1 = Osc(table=t_ex1, freq=300, mul=env_ex1).out()
env_ex2 = Osc(table=t_ex, freq=2, phase=0.5)
o_ex2 = Osc(table=t_ex1, freq=400, mul=env_ex2).out()
"""
    t_ex1 = HannTable()
    t_ex2 = HarmTable([1,0,0,.2,0,0,.1,0,0,.05])
    env_ex1 = Osc(table=t_ex1, freq=2, phase=0)
    o_ex1 = Osc(table=t_ex2, freq=300, mul=env_ex1).out()
    env_ex2 = Osc(table=t_ex1, freq=2, phase=0.5)
    o_ex2 = Osc(table=t_ex2, freq=400, mul=env_ex2).out()

    c = Clean_objects(4, o_ex1, o_ex2, env_ex1, env_ex2, t_ex1, t_ex2)
    c.start()

funcs = {1: ex1, 2: ex2, 3: ex3, 4: ex4}

print "\n** A Server must be started !"
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
