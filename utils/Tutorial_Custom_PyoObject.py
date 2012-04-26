"""
Tutorials on creating a custom PyoObject:

There is a few steps we need to take care of in order to create a class with all 
of pyo's functionalities.

Things to consider:

- The parent class must be PyoObject, that means the PyoObject's __init__ method
    must be called inside the object's __init__ method in order to properly
    initialize PyoObject's basic attributes.
- When a PyoObject receives another PyoObject, it looks for a list of objects 
    called "self._base_objs".
- Adding "mul" and "add" arguments (they act on objects in self._base_objs).
- All PyoObject support "list expansion".
- All PyoObject with sound in input support crossfading between old and new sources.
- We will probably want to override the play(), out() and stop() methods.
- There is an attribute for any function that modifies a parameter.
- The __dir__ method should return a list of the available attributes as strings.
- We should override the ctrl() method to popup a GUI to control parameters.

In this tutorial, we will define a Flanger object with this definition:

Flanger(input, depth=0.75, lfofreq=0.2, feedback=0.25, mul=1, add=0)

"""
from pyo import *

# Step 1 - Declaring the class

# We will create a new class called Flanger with PyoObject as it's parent class. 
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
    
    
    Parent class: PyoObject

    Parameters:

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
        
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setDepth(x) : Replace the `depth` attribute.
    setLfoFreq(x) : Replace the `lfofreq` attribute.
    setFeedback(x) : Replace the `feedback` attribute.
    
    Attributes:

    input : PyoObject. Input signal to delayed.
    depth : float or PyoObject. Amplitude of the delay line modulation.
    lfofreq : float or PyoObject. Frequency of the delay line modulation.
    feedback : float or PyoObject. Amount of output signal sent back
        into the delay line.

    See also: Delay, Chorus, Phaser

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> inp = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> lf = Sine(0.005, mul=0.25, add=0.5)
    >>> flg = Flanger(input=inp, depth=0.9, lfofreq=0.1, feedback=lf).out()

    """

# Step 2 - The __init__ method

# This is the place where we have to take care of some of pyo's generic behaviours. 
# The most important thing to remember is that when a PyoObject receives another 
# PyoObject in input, it looks for an attribute called self._base_objs. That attribute 
# is a list of the object's base classes and is considered as the audio output signal 
# of the object (the Sine object uses internally an object called Sine_base). The 
# getBaseObjects() method returns the list of base classes for a given PyoObject. We 
# will call the getBaseObjects() method on the objects generating the output signal of 
# our process.

# We also need to add two arguments to the definition of the object: "mul" and "add". 
# The attributes "self._mul" and "self._add" are handled by the parent class and are 
# automatically applied to the objects in the list "self._base_objs".

# Finally, we have to consider the "list expansion" feature, allowing lists given as 
# arguments to create multiple instances of our object and managing multiple audio streams. 
# Two functions help us to accomplish this:

# convertArgsToLists(*args) : Returns arguments converted to lists and the maximum list size.
# wrap(list,i) : Returns value at position "i" in "list" with wrap around len(list).

    def __init__(self, input, depth=0.75, lfofreq=0.2, feedback=0.5, mul=1, add=0):
        # Properly initialize PyoObject's basic attributes
        PyoObject.__init__(self)
        # Keep references of all raw arguments
        self._input = input
        self._depth = depth
        self._lfofreq = lfofreq
        self._feedback = feedback
        self._mul = mul
        self._add = add

        # Using InputFader for sound input allows crossfade when changing sources
        self._in_fader = InputFader(input)

        # Convert all arguments to lists for "list expansion"
        # convertArgsToLists function returns variables in argument as lists + maximum list size
        in_fader, depth, lfofreq, feedback, mul, add, lmax = convertArgsToLists(self._in_fader, depth, lfofreq, feedback, mul, add)
    
        # Init some lists to keep track of created objects
        self._modulators = []
        self._delays = []
        self._outs = []
        # self._base_objs is the audio output seen by the outside world!
        # .play(), .out(), .stop() and .mix() methods act on this list
        # "mul" and "add" attributes are also applied on this list's objects 
        self._base_objs = []
    
        # Each cycle of the loop creates a mono stream of sound
        for i in range(lmax):
            self._modulators.append(Sine(freq=wrap(lfofreq,i), mul=wrap(depth,i)*0.005, add=0.005))
            self._delays.append(Delay(wrap(in_fader,i), delay=self._modulators[-1], feedback=wrap(feedback,i)))
            self._outs.append(Interp(self._modulators[-1], self._delays[-1], mul=wrap(mul,i), add=wrap(add,i)))
            # getBaseObjects() method returns the list of Object_Base, needed in the self._base_objs list
            self._base_objs.extend(self._outs[-1].getBaseObjects())

# Step 3 - setXXX methods and attributes

# Now, we will add methods and attributes for all controllable parameters. 
# It should be noted that we use the setInput() method of the InputFader 
# object to change an input source (setInput()). This object implements a 
# crossfade between the old source and the new one with a crossfade duration 
# argument:

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        Parameters:

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

        Parameters:

        x : float or PyoObject
            New `depth` attribute.

        """
        self._depth = x
        x, lmax = convertArgsToLists(x)
        [obj.setMul(wrap(x,i)*0.005) for i, obj in enumerate(self._modulators)]

    def setLfoFreq(self, x):
        """
        Replace the `lfofreq` attribute.

        Parameters:

        x : float or PyoObject
            New `lfofreq` attribute.

        """
        self._lfofreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._modulators)]

    def setFeedback(self, x):
        """
        Replace the `feedback` attribute.

        Parameters:

        x : float or PyoObject
            New `feedback` attribute.

        """
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x,i)) for i, obj in enumerate(self._delays)]

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def depth(self): return self._depth
    @depth.setter
    def depth(self, x): self.setDepth(x)

    @property
    def lfofreq(self): return self._lfofreq
    @lfofreq.setter
    def lfofreq(self, x): self.setLfoFreq(x)

    @property
    def feedback(self): return self._feedback
    @feedback.setter
    def feedback(self, x): self.setFeedback(x)

# Step 4 - The __dir__ method

# We will override the __dir__ method to return a list of all controllable attributes 
# for our object. User can retrieve this value by calling dir(obj)

    def __dir__(self):
        return ["input", "depth", "lfofreq", "feedback", "mul", "add"]

# Step 5 - The ctrl() method

# The ctrl() method of a PyoObject is used to popup a GUI to control the parameters 
# of the object. The initialization of sliders is done with a list of SLMap objects 
# where we can set the range of the slider, the type of scaling, the name of the 
# attribute linked to the slider and the initial value. We will define a default 
# "map_list" that will be used if the user doesn't provide one.

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        # Define the object's default map_list used if None is passed to PyoObject
        # map_list is a list of SLMap objects defined for each available attribute
        # in the controller window
        self._map_list = [SLMap(0., 1., "lin", "depth", self._depth),
                          SLMap(0.001, 20., "log", "lfofreq", self._lfofreq),
                          SLMap(0., 1., "lin", "feedback", self._feedback),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

# Step 6 - Overriding the .play(), .stop() and .out() methods

# Finally, we might want to override .play(), .stop() and .out() methods to be sure all 
# our internal PyoObjects are consequently managed instead of only objects in self._base_obj, 
# as it is in current objects. To handle properly the process for self._base_objs, we call
# the method that belong to PyoObject. We return the returned value (self) of these methods
# in order to append the method to the object creation. See the definition of these methods 
# in the PyoObject man page to understand the meaning of arguments.

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._modulators)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._delays)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._outs)]
        return PyoObject.play(self, dur, delay)

    def stop(self):
        [obj.stop() for obj in self._modulators]
        [obj.stop() for obj in self._delays]
        [obj.stop() for obj in self._outs]
        return PyoObject.stop(self)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._modulators)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._delays)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._outs)]
        return PyoObject.out(self, chnl, inc, dur, delay)

# Run the script to test the Flanger object.
if __name__ == "__main__":
    s = Server().boot()
    src = BrownNoise([.2,.2]).out()
    fl = Flanger(src, depth=.9, lfofreq=.1, feedback=.5, mul=.5).out()
    s.gui(locals())