"""
Setting audio object parameter's value with portamento

**03-set-method.py**

The set() method of the PyoObject lets the user define a new
value for a given parameter with a portamento time to reach
the target. Optionally, a function to call at the end of the
ramp can be given to the `callback` argument.

The signature of the method is::

    PyoObject.set(attr, value, port=0.03, callback=None)

Where:

    - `attr` is the name of the attribute as a string.
    - `value` is the target value (can be a list if object has
      multiple streams).
    - `port` is the portamento time in seconds.
    - `callback` is a function to call when the ramp is done.
    
"""

from pyo import *
from random import uniform

s = Server(duplex=0).boot()

# 10 frequency modulations with random parameters.
a = FM(
    carrier=[uniform(197, 203) for i in range(10)],
    ratio=[uniform(0.99, 1.01) for i in range(10)],
    index=[uniform(5, 10) for i in range(10)],
    mul=0.05,
).out()


def go():
    "Linear ramps between the current values and the new ones in 9.5 sec."
    # Choose new targets and start the ramps.
    a.set("carrier", [uniform(395, 405) for i in range(10)], 9.5)
    a.set("ratio", [uniform(0.49, 0.51) for i in range(10)], 9.5)
    # Call the reset() function when portamentos are done.
    a.set("index", [uniform(10, 15) for i in range(10)], 9.5, callback=reset)


def reset():
    "Instantaneous jumps to new values."
    a.carrier = [uniform(197, 203) for i in range(10)]
    a.ratio = [uniform(0.99, 1.01) for i in range(10)]
    a.index = [uniform(5, 10) for i in range(10)]


# Call the go() function every 10 seconds.
pat = Pattern(go, time=10).play()

s.gui(locals())
