from _core import *

######################################################################
### Tables
######################################################################                                       
class HarmTable(PyoTableObject):
    """
    Generate composite waveforms made up of weighted sums of simple sinusoids.
    
    **Parameters**
    
    list : list, optional
        Relative strengths of the fixed harmonic partial numbers 1,2,3, etc. Defaults to [1].
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    **Methods**
    
    setSize(size) : Change the size of the table. This will erase the previously drawn waveform.
    replace(list) : Redraw the waveform according to the new `list` parameter.
    
    **Attributes**
    
    size : int, optional
        Table size in samples.
    list : list, optional
        Relative strengths of the fixed harmonic partial numbers.

    """
    def __init__(self, list=[1.], size=8192):
        self._list = list
        self._size = size
        self._base_objs = [HarmTable_base(list, size)]
        
    def setSize(self, size):
        """
        Change the size of the table. This will erase the previously drawn waveform.
        
        **Parameters**
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics relative strengths.
        
        **Parameters**
        
        list : list
            Relative strengths of the fixed harmonic partial numbers 1,2,3, etc.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]

    #def demo():
    #    execfile("demos/HarmTable_demo.py")
    #demo = Call_example(demo)

    def args():
        print('HarmTable(list=[1.], size=8192)')
    args = Print_args(args)

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
    Generate Hanning window. 
    
    **Parameters**
    
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    **Methods**
    
    setSize(size) : Change the size of the table. This will redraw the envelope.
    
    **Attributes**
    
    size : int
        Table size in samples.

    """
    def __init__(self, size=8192):
        self._size = size
        self._base_objs = [HannTable_base(size)]

    def setSize(self, size):
        """
        Change the size of the table. This will redraw the envelope.
        
        **Parameters**
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]

    #def demo():
    #    execfile("demos/HannTable_demo.py")
    #demo = Call_example(demo)

    def args():
        print('HannTable(size=8192)')
    args = Print_args(args)

    @property
    def size(self): 
        """int. Table size in samples."""
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

class LinTable(PyoTableObject):
    """
    Construct a table from segments of straight lines in breakpoint fashion.
    
    **Parameters**
    
    list : list, optional
        List of tuples indicating location and value of each points in the table. 
        The default, [(0,0.), (8191, 1.)], creates a straight line from 0.0 at location 0
        to 1.0 at the end of the table (size - 1). Location must be an integer.
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    **Methods**
    
    setSize(size) : Change the size of the table and rescale the envelope.
    replace(list) : Draw a new envelope according to the new `list` parameter.

    **note**
    
    Locations in the list must be in increasing order. If the last value is less 
    than size, then the rest will be set to zero. 
    
    **Attributes**
    
    list : list
        List of tuples [(location, value), ...].
    size : int, optional
        Table size in samples.

    """
    def __init__(self, list=[(0, 0.), (8191, 1.)], size=8192):
        self._size = size
        self._base_objs = [LinTable_base(list, size)]
        
    def setSize(self, size):
        """
        Change the size of the table and rescale the envelope.
        
        **Parameters**
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.
        
        **Parameters**
        
        list : list
            List of tuples indicating location and value of each points in the table. 
            Location must be integer.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]

    def getPoints(self):
        return self._base_objs[0].getPoints()

    #def demo():
    #    execfile("demos/LinTable_demo.py")
    #demo = Call_example(demo)

    def args():
        print('LinTable(list=[(0, 0.), (8191, 1.)], size=8192)')
    args = Print_args(args)
        
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

class SndTable(PyoTableObject):
    """
    Load data from a soundfile into a function table.
    
    If `chnl` is None, the table will contain as many sub tables as necessary 
    to read all channels of the loaded sound.
    
    **Parameters**
    
    path : string
        Full path name of the sound.
    chnl : int, optional
        Channel number to read in. The default (None) reads all channels.

    **Methods**

    setSound(path) : Load a new sound in the table.
    getRate() : Return the frequency in cps at which the sound will be read 
    at its original pitch.

    """
    def __init__(self, path, chnl=None):
        self._size, self._snd_sr, self._snd_chnls = sndinfo(path)
        self._path = path
        if chnl == None:
            self._base_objs = [SndTable_base(path, i) for i in range(self._snd_chnls)]
        else:
            self._base_objs = [SndTable_base(path, chnl)]

    def setSound(self, path):
        """
        Load a new sound in the table.
        
        Keeps the number of channels of the sound loaded at initialization.
        If the new sound has less channels, it will wrap around and load the 
        same channels many times. If the new sound has more channels, the extra 
        channels will be skipped.
        
        **Parameters**
        
        path : string
            Full path of the new sound.

        """
        _size, _snd_sr, _snd_chnls = sndinfo(path)
        self._path = path
        [obj.setSound(path, (i%_snd_chnls)) for i, obj in enumerate(self._base_objs)]
        
    def getRate(self):
        return self._base_objs[0].getRate()

    #def demo():
    #    execfile("demos/SndTable_demo.py")
    #demo = Call_example(demo)

    def args():
        print('SndTable(path, chnl=None)')
    args = Print_args(args)

    @property
    def sound(self):
        """string. Full path of the sound.""" 
        return self._path
    @sound.setter
    def sound(self, x): self.setSound(x)

class NewTable(PyoTableObject):
    """
    Create an empty table ready for recording. See `TableRec` to write samples in 
    the table.
    
    **Parameters**
    
    length : float
        Length of the table in seconds.
    chnls : int, optional
        Number of channels that will be handled by the table. Defaults to 1.
        
    **Methods**    
    
    getSize() : Return the length of the table in samples.
    getLength() : Return the length of the table in seconds.
    getRate() : Return the frequency (cycle per second) to give an oscillator to read the sound 
        at its original pitch.

    """
    def __init__(self, length, chnls=1):
        self._base_objs = [NewTable_base(length) for i in range(chnls)]
                
    def getSize(self):
        return self._base_objs[0].getSize()

    def getLength(self):
        return self._base_objs[0].getLength()
             
    def getRate(self):
        return self._base_objs[0].getRate()

    #def demo():
    #    execfile("demos/NewTable_demo.py")
    #demo = Call_example(demo)

    def args():
        print('NewTable(length, chnls=1)')
    args = Print_args(args)

class TableRec(PyoObject):
    """
    TableRec is for writing samples into a previously created NewTable. See `NewTable` to create
    an empty table.

    **Parameters**

    input : PyoObject
        Audio signal to write in the table.
    table : PyoTableObject
        The table where to write samples.
    fadetime : float, optional
        Fade time at the beginning and the end of the recording in seconds. Defaults to 0.
    
    **Methods**

    setInput(x, fadetime) : Replace the `input` attribute.
    play() : Start the recording at the beginning of the table.
    stop() : Stop the recording. Otherwise, record through the end of the table.

    **Notes**

    The out() method is bypassed. TableRec returns no signal.
    
    TableRec has no `mul` and `add` attributes.
    
    """
    def __init__(self, input, table, fadetime=0):
        self._input = input
        self._table = table
        self._in_fader = InputFader(input)
        in_fader, table, fadetime, lmax = convertArgsToLists(self._in_fader, table, fadetime)
        self._base_objs = [TableRec_base(wrap(in_fader,i), wrap(table,i), wrap(fadetime,i)) for i in range(len(table))]

    def out(self, chnl=0, inc=1):
        pass

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        **Parameters**

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTable(self, x):
        """Replace the `table` attribute.
        
        **Parameters**

        x : NewTable
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    #def demo():
    #    execfile("demos/TableRec_demo.py")
    #demo = Call_example(demo)

    def args():
        print('TableRec(input, table, fadetime=0)')
    args = Print_args(args)
      
    @property
    def input(self):
        """PyoObject. Audio signal to write in the table.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def table(self):
        """PyoTableObject. The table where to write samples."""
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

