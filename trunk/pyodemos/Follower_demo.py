def ex1():
    from pyo import SfPlayer, Noise, Clean_objects, DEMOS_PATH
    print """
Smooth envelope follower:

o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
o_ex2 = Follower(o_ex1, freq=5)
o_ex3 = Noise(mul=o_ex2).out()
o_ex4 = o_ex1 * .5
o_ex4.out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
    o_ex2 = Follower(o_ex1, freq=5)
    o_ex3 = Noise(mul=o_ex2).out()
    o_ex4 = o_ex1 * .5
    o_ex4.out()
    
    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, o_ex4)
    c.start()

def ex2():
    from pyo import SfPlayer, Noise, Clean_objects, DEMOS_PATH
    print """
Sharp envelope follower:

o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
o_ex2 = Follower(o_ex1, freq=25)
o_ex3 = Noise(mul=o_ex2).out()
o_ex4 = o_ex1 * .5
o_ex4.out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
    o_ex2 = Follower(o_ex1, freq=25)
    o_ex3 = Noise(mul=o_ex2).out()
    o_ex4 = o_ex1 * .5
    o_ex4.out()
    
    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, o_ex4)
    c.start()
  
def ex3():
    from pyo import SfPlayer, Noise, Clean_objects, DEMOS_PATH
    print """
Very sharp envelope follower:

o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
o_ex2 = Follower(o_ex1, freq=50)
o_ex3 = Noise(mul=o_ex2).out()
o_ex4 = o_ex1 * .5
o_ex4.out()
"""
    o_ex1 = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
    o_ex2 = Follower(o_ex1, freq=50)
    o_ex3 = Noise(mul=o_ex2).out()
    o_ex4 = o_ex1 * .5
    o_ex4.out()
    
    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, o_ex4)
    c.start()
   
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
        
