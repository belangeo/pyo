def ex1():
    import time
    from pyo import SfPlayer, Noise
    print """
Smooth envelope follower:

o_ex1 = SfPlayer("demos/transparent.aif", loop=True)
o_ex2 = Follower(o_ex1, freq=5)
o_ex3 = Noise(mul=o_ex2).out()
o_ex4 = o_ex1 * .5
o_ex4.out()
"""
    o_ex1 = SfPlayer("demos/transparent.aif", loop=True)
    o_ex2 = Follower(o_ex1, freq=5)
    o_ex3 = Noise(mul=o_ex2).out()
    o_ex4 = o_ex1 * .5
    o_ex4.out()
    
    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    o_ex4.stop()
    del o_ex1
    del o_ex2
    del o_ex3
    del o_ex4

def ex2():
    import time
    from pyo import SfPlayer, Noise
    print """
Sharp envelope follower:

o_ex1 = SfPlayer("demos/transparent.aif", loop=True)
o_ex2 = Follower(o_ex1, freq=25)
o_ex3 = Noise(mul=o_ex2).out()
o_ex4 = o_ex1 * .5
o_ex4.out()
"""
    o_ex1 = SfPlayer("demos/transparent.aif", loop=True)
    o_ex2 = Follower(o_ex1, freq=25)
    o_ex3 = Noise(mul=o_ex2).out()
    o_ex4 = o_ex1 * .5
    o_ex4.out()
    
    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    o_ex4.stop()
    del o_ex1
    del o_ex2
    del o_ex3
    del o_ex4
  
def ex3():
    import time
    from pyo import SfPlayer, Noise
    print """
Very sharp envelope follower:

o_ex1 = SfPlayer("demos/transparent.aif", loop=True)
o_ex2 = Follower(o_ex1, freq=50)
o_ex3 = Noise(mul=o_ex2).out()
o_ex4 = o_ex1 * .5
o_ex4.out()
"""
    o_ex1 = SfPlayer("demos/transparent.aif", loop=True)
    o_ex2 = Follower(o_ex1, freq=50)
    o_ex3 = Noise(mul=o_ex2).out()
    o_ex4 = o_ex1 * .5
    o_ex4.out()
    
    time.sleep(4)
    o_ex1.stop()
    o_ex2.stop()
    o_ex3.stop()
    o_ex4.stop()
    del o_ex1
    del o_ex2
    del o_ex3
    del o_ex4
   
funcs = {1: ex1, 2: ex2, 3: ex3}

print "\n** A Server must be started !"
while True:
    rep = input("""    
Choose a demo :

Smooth envelope follower : 1
Sharp envelope follower : 2
Very sharp envelope follower : 3
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break
        
