"""
Tutorial on creating a custom PyoObject.

There are few steps we need to take care of in order to create a class with all 
of the PyoObject behaviors.

Things to consider:

- The parent class must be PyoObject, that means the PyoObject's __init__ method
    must be called inside the object's __init__ method to properly initialize 
    PyoObject's basic attributes.
- When a PyoObject receives another PyoObject, it looks for a list of objects 
    called "self._base_objs". This list must contain the C implementation of 
    the audio objects generating the output sound of the process. 
- Adding "mul" and "add" arguments (they act on objects in self._base_objs).
- All PyoObjects support "list expansion".
- All PyoObjects with sound in input support cross-fading between old and new sources.
- We will probably want to override the play(), out() and stop() methods.
- There is an attribute for any function that modify a parameter.
- We should override the ctrl() method to allow a GUI to control parameters.

In this tutorial, we will define a RingMod object with this definition:

RingMod(input, freq=100, mul=1, add=0)

"""
from pyo import *

# Step 1 - Declaring the class

# We will create a new class called RingMod with PyoObject as its parent class.
# Another good habit is to put a __doc__ string at the beginning of our classes.
# Doing so will allow other users to retrieve the object's documentation with the
# standard python help() function.


class RingMod(PyoObject):
    """
    Ring modulator.

    Ring modulation is a signal-processing effect in electronics 
    performed by multiplying two signals, where one is typically 
    a sine-wave or another simple waveform.

    :Parent: :py:class:`PyoObject`

    :Args:

        input : PyoObject
            Input signal to process.
        freq : float or PyoObject, optional
            Frequency, in cycles per second, of the modulator. 
            Defaults to 100.

    >>> s = Server().boot()
    >>> s.start()
    >>> src = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, mul=.3)
    >>> lfo = Sine(.25, phase=[0,.5], mul=.5, add=.5)
    >>> ring = RingMod(src, freq=[800,1000], mul=lfo).out()

    """

    # Step 2 - The __init__ method

    # This is the place where we have to take care of some of pyo's generic behaviours.
    # The most important thing to remember is that when a PyoObject receives another
    # PyoObject in input, it looks for an attribute called self._base_objs. This attribute
    # is a list of the object's base classes and is considered the audio output signal
    # of the object (the Sine object uses internally an object called Sine_base). The
    # getBaseObjects() method returns the list of base classes for a given PyoObject. We
    # will call the getBaseObjects() method on the objects generating the output signal of
    # our process. .play(), .out(), .stop() and .mix() methods act on this list.

    # We also need to add two arguments to the definition of the object: "mul" and "add".
    # The attributes "self._mul" and "self._add" are handled by the parent class and are
    # automatically applied to the objects stored in the list "self._base_objs".

    # Finally, we have to consider the "multi-channel expansion" feature, allowing lists given as
    # arguments to create multiple instances of our object and managing multiple audio streams.
    # Two functions help us to accomplish this:

    # convertArgsToLists(*args) : Return arguments converted to lists and the maximum list size.
    # wrap(list,i) : Return value at position "i" in "list" with wrap around len(list).

    def __init__(self, input, freq=100, mul=1, add=0):
        # Properly initialize PyoObject's basic attributes
        PyoObject.__init__(self, mul, add)

        # Keep references of all raw arguments
        self._input = input
        self._freq = freq

        # Using InputFader to manage input sound allows cross-fade when changing sources
        self._in_fader = InputFader(input)

        # Convert all arguments to lists for "multi-channel expansion"
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)

        # Apply processing
        self._mod = Sine(freq=freq, mul=in_fader)

        # Use a Sig object as a through to prevent modifying the "mul" attribute of self._mod
        self._ring = Sig(self._mod, mul=mul, add=add)

        # self._base_objs is the audio output seen by the outside world!
        self._base_objs = self._ring.getBaseObjects()

    # Step 3 - setXXX methods and attributes

    # Now, we will add methods and attributes getter and setter for all controllable
    # parameters. It should be noted that we use the setInput() method of the
    # InputFader object to change an input source. This object implements a cross-fade
    # between the old source and the new one with a cross-fade duration argument.
    # Here, we need to keep references of raw argument in order to get the
    # real current state when we call the dump() method.

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x : PyoObject
                New signal to process.
            fadetime : float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x : float or PyoObject
                New `freq` attribute.

        """
        self._freq = x
        self._mod.freq = x

    @property  # getter
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter  # setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Frequency of the modulator."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    # Step 4 - The ctrl() method

    # The ctrl() method of a PyoObject is used to pop-up a GUI to control the parameters
    # of the object. The initialization of sliders is done with a list of SLMap objects
    # where we can set the range of the slider, the type of scaling, the name of the
    # attribute linked to the slider and the initial value. We will define a default
    # "self._map_list" that will be used if the user doesn't provide one to the parameter
    # "map_list". If the object doesn't have any parameter to control with a GUI, this
    # method can be undefined.

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(10, 2000, "log", "freq", self._freq),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    # Step 5 - Overriding the .play(), .stop() and .out() methods

    # Finally, we might want to override .play(), .stop() and .out() methods to be sure all
    # our internal PyoObjects are consequently managed instead of only objects in self._base_obj,
    # as it is in built-in objects. To handle properly the process for self._base_objs, we still
    # need to call the method that belongs to PyoObject. We return the returned value (self) of
    # these methods in order to possibly append the method to the object's creation. See the
    # definition of these methods in the PyoObject man page to understand the meaning of arguments.

    def play(self, dur=0, delay=0):
        self._mod.play(dur, delay)
        return PyoObject.play(self, dur, delay)

    def stop(self, wait=0):
        self._mod.stop(wait)
        return PyoObject.stop(self, wait)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        self._mod.play(dur, delay)
        return PyoObject.out(self, chnl, inc, dur, delay)


# Run the script to test the RingMod object.
if __name__ == "__main__":
    s = Server().boot()
    src = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=0.3)
    src2 = SfPlayer(SNDS_PATH + "/accord.aif", loop=True, mul=0.3)
    lfo = Sine(0.25, phase=[0, 0.5], mul=0.5, add=0.5)
    ring = RingMod(src, freq=[800, 1000], mul=lfo).out()
    s.gui(locals())
