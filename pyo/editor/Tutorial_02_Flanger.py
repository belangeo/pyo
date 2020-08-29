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

In this tutorial, we will define a Flanger object with this definition:

Flanger(input, depth=0.75, lfofreq=0.2, feedback=0.25, mul=1, add=0)

"""
from pyo import *

# Step 1 - Declaring the class

# We will create a new class called RingMod with PyoObject as its parent class.
# Another good habit is to put a __doc__ string at the beginning of our classes.
# Doing so will allow other users to retrieve the object's documentation with the
# standard python help() function.


class Flanger(PyoObject):
    """
    Flanging effect.

    A flanging is an audio effect produced by mixing two identical signals together, 
    with one signal delayed by a small and gradually changing period, usually smaller 
    than 20 milliseconds. This produces a swept comb filter effect: peaks and notches 
    are produced in the resultant frequency spectrum, related to each other in a linear 
    harmonic series. Varying the time delay causes these to sweep up and down the 
    frequency spectrum.
    
    
    :Parent: :py:class:`PyoObject`

    :Args:

        input : PyoObject
            Input signal to process.
        depth : float or PyoObject, optional
            Amplitude of the delay line modulation, between 0 and 1. 
            Defaults to 0.75.
        lfofreq : float or PyoObject, optional
            Frequency of the delay line modulation, in Hertz. 
            Defaults to 0.2.
        feedback : float or PyoObject, optional
            Amount of output signal reinjected into the delay line. 
            Defaults to 0.25.

    >>> s = Server().boot()
    >>> s.start()
    >>> inp = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> lf = Sine(0.005, mul=0.25, add=0.5)
    >>> flg = Flanger(input=inp, depth=0.9, lfofreq=0.1, feedback=lf).out()

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

    def __init__(self, input, depth=0.75, lfofreq=0.2, feedback=0.5, mul=1, add=0):
        # Properly initialize PyoObject's basic attributes
        PyoObject.__init__(self)

        # Keep references of all raw arguments
        self._input = input
        self._depth = depth
        self._lfofreq = lfofreq
        self._feedback = feedback

        # Using InputFader to manage input sound allows cross-fade when changing sources
        self._in_fader = InputFader(input)

        # Convert all arguments to lists for "multi-channel expansion"
        in_fader, depth, lfofreq, feedback, mul, add, lmax = convertArgsToLists(
            self._in_fader, depth, lfofreq, feedback, mul, add
        )

        # Apply processing
        self._modamp = Sig(depth, mul=0.005)
        self._mod = Sine(freq=lfofreq, mul=self._modamp, add=0.005)
        self._dls = Delay(in_fader, delay=self._mod, feedback=feedback)
        self._flange = Interp(in_fader, self._dls, mul=mul, add=add)

        # self._base_objs is the audio output seen by the outside world!
        self._base_objs = self._flange.getBaseObjects()

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

    def setDepth(self, x):
        """
        Replace the `depth` attribute.

        :Args:

            x : float or PyoObject
                New `depth` attribute.

        """
        self._depth = x
        self._modamp.value = x

    def setLfoFreq(self, x):
        """
        Replace the `lfofreq` attribute.

        :Args:

            x : float or PyoObject
                New `lfofreq` attribute.

        """
        self._lfofreq = x
        self._mod.freq = x

    def setFeedback(self, x):
        """
        Replace the `feedback` attribute.

        :Args:

            x : float or PyoObject
                New `feedback` attribute.

        """
        self._feedback = x
        self._dls.feedback = x

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def depth(self):
        """float or PyoObject. Amplitude of the delay line modulation."""
        return self._depth

    @depth.setter
    def depth(self, x):
        self.setDepth(x)

    @property
    def lfofreq(self):
        """float or PyoObject. Frequency of the delay line modulation."""
        return self._lfofreq

    @lfofreq.setter
    def lfofreq(self, x):
        self.setLfoFreq(x)

    @property
    def feedback(self):
        """float or PyoObject. Amount of output signal sent back into the delay line."""
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)

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
            SLMap(0.0, 1.0, "lin", "depth", self._depth),
            SLMap(0.001, 20.0, "log", "lfofreq", self._lfofreq),
            SLMap(0.0, 1.0, "lin", "feedback", self._feedback),
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
        self._modamp.play(dur, delay)
        self._mod.play(dur, delay)
        self._dls.play(dur, delay)
        return PyoObject.play(self, dur, delay)

    def stop(self, wait=0):
        self._modamp.stop(wait)
        self._mod.stop(wait)
        self._dls.stop(wait)
        return PyoObject.stop(self, wait)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        self._modamp.play(dur, delay)
        self._mod.play(dur, delay)
        self._dls.play(dur, delay)
        return PyoObject.out(self, chnl, inc, dur, delay)


# Run the script to test the Flanger object.
if __name__ == "__main__":
    s = Server().boot()
    src = BrownNoise([0.2, 0.2]).out()
    fl = Flanger(src, depth=0.9, lfofreq=0.1, feedback=0.5, mul=0.5).out()
    s.gui(locals())
