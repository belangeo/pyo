def ex1():
    from pyo import SfPlayer, Sine, Clean_objects
    print """
Frequency shift:

o_ex1 = SfPlayer("pyodemos/accord.aif", loop=True).out()
o_ex2 = Hilbert(o_ex1)
o_ex3 = Sine(1000, [0, .25])
o_ex4 = b['real'] * o_ex3[0]
o_ex5 = b['imag'] * o_ex3[1]
o_up = o_ex4 - o_ex5
o_down = o_ex4 + o_ex5
o_up.out()
"""
    o_ex1 = SfPlayer("pyodemos/accord.aif", loop=True).out()
    o_ex2 = Hilbert(o_ex1)
    o_ex3 = Sine(1000, [0, .25])
    o_ex4 = o_ex2['real'] * o_ex3[0]
    o_ex5 = o_ex2['imag'] * o_ex3[1]
    o_up = o_ex4 - o_ex5
    o_down = o_ex4 + o_ex5
    o_up.out()

    c = Clean_objects(4, o_ex1, o_ex2, o_ex3, o_ex4, o_ex5, o_up, o_down)
    c.start()

funcs = {1: ex1}

while True:
    rep = input("""    
Choose a demo :

Frequency shift : 1
Quit demo : 0

-----> : """)
    if rep in funcs.keys():
        funcs[rep]()
    else:
        break

