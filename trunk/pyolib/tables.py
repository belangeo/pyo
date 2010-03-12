from _core import *

######################################################################
### Tables
######################################################################                                       
class HarmTable(PyoTableObject):
    """
    Harmonic waveform generator.
    
    Generates composite waveforms made up of weighted sums 
    of simple sinusoids.
    
    Parent class: PyoTableObject
    
    Parameters:
    
    list : list, optional
        Relative strengths of the fixed harmonic partial numbers 1,2,3, etc. 
        Defaults to [1].
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table. This will erase the 
        previously drawn waveform.
    replace(list) : Redraw the waveform according to the new `list` 
        parameter.
    
    Attributes:
    
    list : list, optional
        Relative strengths of the fixed harmonic partial numbers.
    size : int, optional
        Table size in samples.
        
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> # Square wave up to 9th harmonic
    >>> t = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> a = Osc(table=t, freq=200, mul=.5).out()

    """
    def __init__(self, list=[1., 0.], size=8192):
        self._list = list
        self._size = size
        self._base_objs = [HarmTable_base(list, size)]

    def __dir__(self):
        return ['list', 'size']
        
    def setSize(self, size):
        """
        Change the size of the table. This will erase the previously 
        drawn waveform.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics 
        relative strengths.
        
        Parameters:
        
        list : list
            Relative strengths of the fixed harmonic partial 
            numbers 1,2,3, etc.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]

    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def list(self): 
        """list. Relative strengths of the fixed harmonic partial numbers."""
        return self._list
    @list.setter
    def list(self, x): self.replace(x)

class SawTable(PyoTableObject):
    """
    Sawtooth waveform generator.
    
    Generates sawtooth waveforms made up of fixed number of harmonics.
    
    Parent class: PyoTableObject
    
    Parameters:
    
    order : int, optional
        Number of harmonics sawtooth is made of. 
        Defaults to 10.
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setOrder(x) : Change the `order` attribute and redraw the waveform.
    setSize(size) : Change the size of the table. This will erase the 
        previously drawn waveform.
    
    Attributes:
    
    order : int, optional
        Number of harmonics sawtooth is made of.
    size : int, optional
        Table size in samples.
        
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = SawTable()
    >>> a = Osc(table=t, freq=200, mul=.5).out()

    """
    def __init__(self, order=10, size=8192):
        self._order = order
        self._size = size
        list = [1./i for i in range(1,(order+1))]
        self._base_objs = [HarmTable_base(list, size)]

    def __dir__(self):
        return ['order', 'size']
        
    def setSize(self, size):
        """
        Change the size of the table. This will erase the previously 
        drawn waveform.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def setOrder(self, x):
        """
        Change the `order` attribute and redraw the waveform.
        
        Parameters:
        
        x : int
            New number of harmonics

        """      
        self._order = x
        list = [1./i for i in range(1,(self._order+1))]
        [obj.replace(list) for obj in self._base_objs]

    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def order(self): 
        """int. Number of harmonics sawtooth is made of."""
        return self._order
    @order.setter
    def order(self, x): self.setOrder(x)

class SquareTable(PyoTableObject):
    """
    Square waveform generator.
    
    Generates square waveforms made up of fixed number of harmonics.
    
    Parent class: PyoTableObject
    
    Parameters:
    
    order : int, optional
        Number of harmonics square waveform is made of. The waveform will 
        contains `order` odd harmonics. Defaults to 10.
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setOrder(x) : Change the `order` attribute and redraw the waveform.
    setSize(size) : Change the size of the table. This will erase the 
        previously drawn waveform.
    
    Attributes:
    
    order : int, optional
        Number of harmonics square waveform is made of.
    size : int, optional
        Table size in samples.
        
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = SquareTable()
    >>> a = Osc(table=t, freq=200, mul=.5).out()

    """
    def __init__(self, order=10, size=8192):
        self._order = order
        self._size = size
        list = []
        for i in range(1,(order*2)):
            if i%2 == 1:
                list.append(1./i)
            else:
                list.append(0.)    
        self._base_objs = [HarmTable_base(list, size)]

    def __dir__(self):
        return ['order', 'size']
        
    def setSize(self, size):
        """
        Change the size of the table. This will erase the previously 
        drawn waveform.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def setOrder(self, x):
        """
        Change the `order` attribute and redraw the waveform.
        
        Parameters:
        
        x : int
            New number of harmonics

        """      
        self._order = x
        list = []
        for i in range(1,(self._order*2)):
            if i%2 == 1:
                list.append(1./i)
            else:
                list.append(0.)    
        [obj.replace(list) for obj in self._base_objs]

    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def order(self): 
        """int. Number of harmonics square waveform is made of."""
        return self._order
    @order.setter
    def order(self, x): self.setOrder(x)

class ChebyTable(PyoTableObject):
    """
    Chebyshev polynomials of the first kind. 
    
    Uses Chebyshev coefficients to generate stored polynomial functions 
    which, under waveshaping, can be used to split a sinusoid into 
    harmonic partials having a pre-definable spectrum. 
    
    Parent class: PyoTableObject
    
    Parameters:
    
    list : list, optional
        Relative strengths of partials numbers 1,2,3, ..., 12 that will 
        result when a sinusoid of amplitude 1 is waveshaped using this 
        function table. Up to 12 partials can be specified. Defaults to [1].
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table. This will erase the 
        previously drawn waveform.
    replace(list) : Redraw the waveform according to the new `list` 
        parameter.
    
    Attributes:
    
    list : list, optional
        Relative strengths of the fixed harmonic partial numbers.
    size : int, optional
        Table size in samples.
        
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = ChebyTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> a = Sine(freq=100)
    >>> b = Lookup(table=t, index=a, mul=.5).out()

    """
    def __init__(self, list=[1., 0.], size=8192):
        self._list = list
        self._size = size
        self._base_objs = [ChebyTable_base(list, size)]

    def __dir__(self):
        return ['list', 'size']
        
    def setSize(self, size):
        """
        Change the size of the table. This will erase the previously 
        drawn waveform.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics 
        relative strengths that will result when a sinusoid of 
        amplitude 1 is waveshaped using this function table.
        
        Parameters:
        
        list : list
            Relative strengths of the fixed harmonic partial 
            numbers 1,2,3, ..., 12. Up to 12 partials can be specified.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]

    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def list(self): 
        """list. Relative strengths of the fixed harmonic partial numbers."""
        return self._list
    @list.setter
    def list(self, x): self.replace(x)
        
class HannTable(PyoTableObject):
    """
    Generates Hanning window. 
    
    Parent class: PyoTableObject
    
    Parameters:
    
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table. This will redraw 
        the envelope.
    
    Attributes:
    
    size : int
        Table size in samples.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> # Hanning envelope
    >>> t = HannTable()
    >>> a = Osc(table=t, freq=2, mul=.5)
    >>> b = Sine(freq=500, mul=a).out()

    """
    def __init__(self, size=8192):
        self._size = size
        self._base_objs = [HannTable_base(size)]

    def __dir__(self):
        return ['size']

    def setSize(self, size):
        """
        Change the size of the table. This will redraw the envelope.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]

    @property
    def size(self): 
        """int. Table size in samples."""
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

class LinTable(PyoTableObject):
    """
    Construct a table from segments of straight lines in breakpoint fashion.

    Parent class: PyoTableObject
    
    Parameters:
    
    list : list, optional
        List of tuples indicating location and value of each points 
        in the table. The default, [(0,0.), (8191, 1.)], creates a 
        straight line from 0.0 at location 0 to 1.0 at the end of the 
        table (size - 1). Location must be an integer.
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table and rescale the envelope.
    replace(list) : Draw a new envelope according to the `list` parameter.

    Notes:
    
    Locations in the list must be in increasing order. If the last value 
    is less than size, the rest of the table will be filled with zeros. 
    
    Attributes:
    
    list : list
        List of tuples [(location, value), ...].
    size : int, optional
        Table size in samples.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> # Sharp attack envelope
    >>> t = LinTable([(0,0), (100,1), (1000,.25), (8191,0)])
    >>> a = Osc(table=t, freq=2, mul=.5)
    >>> b = Sine(freq=500, mul=a).out()

    """
    def __init__(self, list=[(0, 0.), (8191, 1.)], size=8192):
        self._size = size
        self._base_objs = [LinTable_base(list, size)]

    def __dir__(self):
        return ['list', 'size']
        
    def setSize(self, size):
        """
        Change the size of the table and rescale the envelope.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.
        
        Parameters:
        
        list : list
            List of tuples indicating location and value of each points 
            in the table. Location must be integer.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]

    def getPoints(self):
        return self._base_objs[0].getPoints()
        
    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table.""" 
        return self.getPoints()
    @list.setter
    def list(self, x): self.replace(x)

class CosTable(PyoTableObject):
    """
    Construct a table from cosine interpolated segments.

    Parent class: PyoTableObject
    
    Parameters:
    
    list : list, optional
        List of tuples indicating location and value of each points 
        in the table. The default, [(0,0.), (8191, 1.)], creates a 
        cosine line from 0.0 at location 0 to 1.0 at the end of the 
        table (size - 1). Location must be an integer.
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table and rescale the envelope.
    replace(list) : Draw a new envelope according to the `list` parameter.

    Notes:
    
    Locations in the list must be in increasing order. If the last value 
    is less than size, the rest of the table will be filled with zeros. 
    
    Attributes:
    
    list : list
        List of tuples [(location, value), ...].
    size : int, optional
        Table size in samples.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> # Sharp attack envelope
    >>> t = CosTable([(0,0), (100,1), (1000,.25), (8191,0)])
    >>> a = Osc(table=t, freq=2, mul=.5)
    >>> b = Sine(freq=500, mul=a).out()

    """
    def __init__(self, list=[(0, 0.), (8191, 1.)], size=8192):
        self._size = size
        self._base_objs = [CosTable_base(list, size)]

    def __dir__(self):
        return ['list', 'size']
        
    def setSize(self, size):
        """
        Change the size of the table and rescale the envelope.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.
        
        Parameters:
        
        list : list
            List of tuples indicating location and value of each points 
            in the table. Location must be integer.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]

    def getPoints(self):
        return self._base_objs[0].getPoints()
        
    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table.""" 
        return self.getPoints()
    @list.setter
    def list(self, x): self.replace(x)

class CurveTable(PyoTableObject):
    """
    Construct a table from curve interpolated segments.
    
    CurveTable uses Hermite interpolation (sort of cubic interpolation)
    to calculate each points of the curve. This algorithm allow tension
    and biasing controls. Tension can be used to tighten up the curvature 
    at the known points. The bias is used to twist the curve about the 
    known points.

    Parent class: PyoTableObject
    
    Parameters:
    
    list : list, optional
        List of tuples indicating location and value of each points 
        in the table. The default, [(0,0.), (8191, 1.)], creates a 
        curved line from 0.0 at location 0 to 1.0 at the end of the 
        table (size - 1). Location must be an integer.
    tension : float, optional
        Curvature at the known points. 1 is high, 0 normal, -1 is low.
        Defaults to 0.
    bias : float, optional
        Curve attraction (for each segments) toward bundary points.
        0 is even, positive is towards first point, negative is towards 
        the second point. Defaults to 0.
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table and rescale the envelope.
    setTension(x) : Replace the `tension` attribute.
    setTension(x) : Replace the `bias` attribute.
    replace(list) : Draw a new envelope according to the `list` parameter.
    
    Notes:
    
    Locations in the list must be in increasing order. If the last value 
    is less than size, the rest of the table will be filled with zeros.
    
    High tension or bias values can create unstable or very loud table,
    use normalize method to keep the curve between -1 and 1.
    
    Attributes:
    
    list : list
        List of tuples [(location, value), ...].
    tension : float
        Curvature tension.    
    bias : float
        Curve attraction.
    size : int, optional
        Table size in samples.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = CurveTable([(0,0),(2048,.5),(4096,.2),(6144,.5),(8192,0)], 0, 20)
    >>> t.normalize()
    >>> a = Osc(table=t, freq=2, mul=.5)
    >>> b = Sine(freq=500, mul=a).out()

    """
    def __init__(self, list=[(0, 0.), (8191, 1.)], tension=0, bias=0, size=8192):
        self._size = size
        self._tension = tension
        self._bias = bias
        self._base_objs = [CurveTable_base(list, tension, bias, size)]

    def __dir__(self):
        return ['list', 'tension', 'bias', 'size']
        
    def setSize(self, size):
        """
        Change the size of the table and rescale the envelope.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]

    def setTension(self, x):
        """
        Replace the `tension` attribute.
        
        1 is high, 0 normal, -1 is low.
        
        Parameters:
        
        x : float
            New `tension` attribute.
        
        """
        self._tension = x
        [obj.setTension(x) for obj in self._base_objs]

    def setBias(self, x):
        """
        Replace the `bias` attribute.
        
        0 is even, positive is towards first point, negative is towards 
        the second point.
        
        Parameters:
        
        x : float
            New `bias` attribute.
        
        """
        self._bias = x
        [obj.setBias(x) for obj in self._base_objs]
     
    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.
        
        Parameters:
        
        list : list
            List of tuples indicating location and value of each points 
            in the table. Location must be integer.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        
    def getPoints(self):
        return self._base_objs[0].getPoints()
        
    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def tension(self):
        """float. Curvature tension.""" 
        return self._tension
    @tension.setter
    def tension(self, x): self.setTension(x)

    @property
    def bias(self):
        """float. Curve Attraction.""" 
        return self._bias
    @bias.setter
    def bias(self, x): self.setBias(x)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table.""" 
        return self.getPoints()
    @list.setter
    def list(self, x): self.replace(x)
        
class SndTable(PyoTableObject):
    """
    Load data from a soundfile into a function table.
    
    If `chnl` is None, the table will contain as many table streams as 
    necessary to read all channels of the loaded sound.

    Parent class: PyoTableObject
    
    Parameters:
    
    path : string
        Full path name of the sound.
    chnl : int, optional
        Channel number to read in. The default (None) reads all channels.

    Methods:

    setSound(path) : Load a new sound in the table.
    getRate() : Return the frequency in cps at which the sound will be 
    read at its original pitch.
    
    Attributes:
    
    sound : Sound path loaded in the table.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> # Sharp attack envelope
    >>> snd_path = SNDS_PATH + '/transparent.aif'
    >>> t = SndTable(snd_path)
    >>> a = Osc(table=t, freq=t.getRate(), mul=.5).out()

    """
    def __init__(self, path, chnl=None):
        self._size, self._dur, self._snd_sr, self._snd_chnls = sndinfo(path)
        self._path = path
        if chnl == None:
            self._base_objs = [SndTable_base(path, i) for i in range(self._snd_chnls)]
        else:
            self._base_objs = [SndTable_base(path, chnl)]

    def __dir__(self):
        return ['sound']

    def setSound(self, path):
        """
        Load a new sound in the table.
        
        Keeps the number of channels of the sound loaded at initialization.
        If the new sound has less channels, it will wrap around and load 
        the same channels many times. If the new sound has more channels, 
        the extra channels will be skipped.
        
        Parameters:
        
        path : string
            Full path of the new sound.

        """
        _size, _dur, _snd_sr, _snd_chnls = sndinfo(path)
        self._path = path
        [obj.setSound(path, (i%_snd_chnls)) for i, obj in enumerate(self._base_objs)]
        
    def getRate(self):
        return self._base_objs[0].getRate()

    @property
    def sound(self):
        """string. Full path of the sound.""" 
        return self._path
    @sound.setter
    def sound(self, x): self.setSound(x)

class NewTable(PyoTableObject):
    """
    Create an empty table ready for recording. 
    
    See `TableRec` to write samples in the table.
    
    Parent class: PyoTableObject
    
    Parameters:
    
    length : float
        Length of the table in seconds.
    chnls : int, optional
        Number of channels that will be handled by the table. 
        Defaults to 1.
        
    Methods:    
    
    getSize() : Returns the length of the table in samples.
    getLength() : Returns the length of the table in seconds.
    getRate() : Returns the frequency (cycle per second) to give 
        to an oscillator to read the sound at its original pitch.

    See also: TableRec

    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t = NewTable(length=2, chnls=1)
    >>> a = Input(0)
    >>> b = TableRec(a, t, .01)
    >>> c = Osc(t, [t.getRate(), t.getRate()*.99]).out()
    >>> # to record in the empty table, call:
    >>> # b.play()

    """
    def __init__(self, length, chnls=1):
        self._base_objs = [NewTable_base(length) for i in range(chnls)]

    def __dir__(self):
        return []
     
    def getSize(self):
        """
        Returns the length of the table in samples.
        
        """
        return self._base_objs[0].getSize()

    def getLength(self):
        """
        Returns the length of the table in seconds.
        
        """
        return self._base_objs[0].getLength()
             
    def getRate(self):
        """
        Returns the frequency (cycle per second) to give to an 
        oscillator to read the sound at its original pitch.
        
        """
        return self._base_objs[0].getRate()

