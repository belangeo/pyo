"""
08-table-lookup.py - Table as transfert function.


"""
from pyo import *

s = Server().boot()

src = SfPlayer("../snds/flute.aif", loop=True)

table = AtanTable(slope=0.5, size=512)
table.view()

drive = Sig(0.5)
drive.ctrl([SLMap(0, 1, "lin", "value", 0.5, dataOnly=True)])

look = Lookup(table, index=src, mul=0.5).out()

def redraw():
    table.slope = drive.get()

trig = TrigFunc(Change(drive), function=redraw)
    
sc = Scope(look)

s.gui(locals())
 