# minimal imports.
# _core contains PyoObject, Mix, Dummy and some utility functions.
# _maps contains all SLMap objects used to set up slider controls.
from pyo.lib._core import *
from pyo.lib._maps import *

# Objects generating vectors of samples must inherit from PyoObject.
# Always provide a __doc__ string with the syntax below.
class Gain(PyoObject):
    """
    Adjusts the gain of an input signal according to a value in decibels.
 
    Parentclass: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to process.
    db : float or PyoObject, optional
        Gain in decibels, 0 is nominal.
        Defaults to -3.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setDB(x) : Replace the `db` attribute.

    Attributes:

    input : PyoObject. Input signal to process.
    db : float or PyoObject. Gain in decibels.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SineLoop([249,250], feedback=0.07)
    >>> lfo = Sine(1, mul=30, add=-40)
    >>> b = Gain(a, db=lfo).out()

    """
    # Do not forget "mul" and "add" attributes.
    def __init__(self, input, db=-3, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        # Keep trace of arguments (same name preceded by an underscore)
        self._input = input
        self._db = db
        # Always use InputFader for the input sound. That allows crossfade on input changes.  
        self._in_fader = InputFader(input)
        # Converts every arguments to lists (for multi-channel expansion).
        in_fader, db, mul, add, lmax = convertArgsToLists(self._in_fader, db, mul, add)
        # self._base_objs contains the list of XXX_base objects. Use "wrap" function to prevent "out of range" errors.
        self._base_objs = [Gain_base(wrap(in_fader,i), wrap(db,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
    
    # # setInput uses the "setInput" method of InputFader to set the crossfade time.
    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        # No need to convert argument to list because InputFader will do it.
        self._input = x
        self._in_fader.setInput(x, fadetime)

    # Every attribute must have its setXXX method.
    def setDB(self, x):
        """
        Replace the `db` attribute.
        
        Parameters:

        x : float or PyoObject
            New `db` attribute.

        """
        # Keep trace of the new value and convert argument to 
        # list before the assignment to XXX_base objects.
        self._db = x
        x, lmax = convertArgsToLists(x)
        [obj.setDB(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    # ctrl method shows to slider window of the object.
    # Look the man page of SLMap for the object's configuration.
    # self._map_list is the default list of SLMap objects.
    # This method must be defined only if the object uses slider controls. 
    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(-120,18,"lin", "db", self._db), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    # setter and getter properties. Each modifiable attribute returns its internal
    # state (self._attr) and must be linked with its setXXX method.
    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def db(self):
        """float or PyoObject. Gain in decibels.""" 
        return self._db
    @db.setter
    def db(self, x): self.setDB(x)
