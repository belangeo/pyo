"""
04-process-calls.py - Calling python functions every processing frame

This example shows two things:

1. How to share memory from a PyoTableObject to a numpy array with the
   `getBuffer()` method of the PyoTableObject. Numpy functions can then
   be used to modify the table's content without copying every samples.

2. How to call a python function every processing frame with the
   CallAlways object.

.. note::

    The numpy module **must** be installed for this example to work.

"""

from pyo import *
import numpy as np

s = Server().boot()

# Get the length of an audio block.
bs = s.getBufferSize()

# Create a table of length `buffer size`.
t = DataTable(size=bs)

# Share the table's memory with a numpy array.
arr = np.asarray(t.getBuffer())

def proc1():
    "Fill the array (so the table) with white noise."
    arr[:] = np.random.normal(0.0, 0.5, size=bs)

# Fill the table.
call1 = CallAlways(proc1)

# You can do other pyo processes here (read the table, process the signal, feed into another shared table, etc.).

# numpy filter's kernel.
kernel_len = 32
window = np.ones(kernel_len) / kernel_len

def proc2():
    "Lowpass filter the array content."
    arr[:] = np.convolve(arr, window, mode='same')

# Lowpass filter the table content.
call2 = CallAlways(proc2) 

# Read the table in loop.
osc = TableRead(t, freq=t.getRate(), loop=True, mul=0.1).out()

s.gui(locals())
