"""
01-envelopes.py - Using break-point function to control an FM synthesis.

This example shows how break-point tables can be used to control synthesis/effect
parameters at audio rate. Pyo offers many objects to generate break-point function:

- LinTable: Construct a table from segments of straight lines.
- CosTable: Construct a table from cosine interpolated segments.
- ExpTable: Construct a table from exponential interpolated segments.
- CurveTable: Construct a table from curve, with tension and bias, interpolated segments.
- LogTable: Construct a table from logarithmic segments.
- CosLogTable: Construct a table from logarithmic-cosine segments.

These objects implement a `graph()` method (as well as Linseg and Expseg) which show
a graph window with which the user can set the shape of the trajectory.

With the focus on the graph window, the copy menu item (Ctrl+C) saves to the
clipboard the list of points in a format well suited to be paste in the code.
Useful to experiment graphically and then copy/paste the result in the script.

To play more notes, in the Interpreter field of the Server GUI, call the `note(freq, dur)`
function with the desired frequency and duration.

Note: The wxPython Phoenix graphical library must be installed to use the graph. Infos at:

http://www.wxpython.org/

"""
from pyo import *

s = Server().boot()

# Defines tables for the amplitude, the ratio and the modulation index.
amp_table = CosTable([(0, 0), (100, 1), (1024, 0.5), (7000, 0.5), (8192, 0)])
rat_table = ExpTable(
    [(0, 0.5), (1500, 0.5), (2000, 0.25), (3500, 0.25), (4000, 1), (5500, 1), (6000, 0.5), (8192, 0.5),]
)
ind_table = LinTable([(0, 20), (512, 10), (8192, 0)])

# call their graph() method. Use the "yrange" argument to set the minimum
# and maximum bundaries of the graph (defaults to 0 and 1).
amp_table.graph(title="Amplitude envelope")
rat_table.graph(title="Ratio envelope")
ind_table.graph(yrange=(0, 20), title="Modulation index envelope")

# Initialize the table readers (TableRead.play() must be called explicitly).
amp = TableRead(table=amp_table, freq=1, loop=False, mul=0.3)
rat = TableRead(table=rat_table, freq=1, loop=False)
ind = TableRead(table=ind_table, freq=1, loop=False)

# Use the signals from the table readers to control an FM synthesis.
fm = FM(carrier=[100, 100], ratio=rat, index=ind, mul=amp).out()

# Call the "note" function to generate an event.
def note(freq=100, dur=1):
    fm.carrier = [freq, freq * 1.005]
    amp.freq = 1.0 / dur
    rat.freq = 1.0 / dur
    ind.freq = 1.0 / dur
    amp.play()
    rat.play()
    ind.play()


# Play one note, carrier = 100 Hz, duration = 2 seconds.
note(200, 2)

s.gui(locals())
