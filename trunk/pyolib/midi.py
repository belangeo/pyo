"""
Objects to retreive Midi informations for a specific Midi port.

Objects create and return audio streams from the value in input.

The audio streams of these objects are essentially intended to be
controls and can't be sent to the output soundcard.

"""

"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from _core import *
from _maps import *

######################################################################
### MIDI
######################################################################                                       
class Midictl(PyoObject):
    """
    Get the current value of a MIDI channel controller.
    
    Get the current value of a controller and optionally map it 
    inside a specified range.

    Parent class: PyoObject
    
    Parameters:
    
    ctlnumber : int
        Midi channel. Available at initialization time only.
    minscale : float, optional
        Low range value for mapping. Available at initialization 
        time only.
    maxscale : float, optional
        High range value for mapping. Available at initialization 
        time only.

    Notes:

    The out() method is bypassed. Midictl's signal can not be sent 
    to audio outs.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> m = Midictl(ctlnumber=[107,102], minscale=250, maxscale=1000)
    >>> p = Port(m, .02)
    >>> a = Sine(freq=p, mul=.3).out()
    >>> a1 = Sine(freq=p*1.25, mul=.3).out()
    >>> a2 = Sine(freq=p*1.5, mul=.3).out()
        
    """
    def __init__(self, ctlnumber, minscale=0, maxscale=1, mul=1, add=0):
        PyoObject.__init__(self)
        self._mul = mul
        self._add = add
        ctlnumber, minscale, maxscale, mul, add, lmax = convertArgsToLists(ctlnumber, minscale, maxscale, mul, add)
        self._base_objs = [Midictl_base(wrap(ctlnumber,i), wrap(minscale,i), wrap(maxscale,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['mul', 'add']

    def out(self, chnl=0, inc=1):
        pass

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)

class Notein(PyoObject):
    """
    Generates MIDI note messages.
    
    From a MIDI device, takes the notes in the range defined with 
    `first` and `last` parameters, and outputs `poly` noteon - noteoff 
    streams in the `scale` format (MIDI, hertz or transpo).
    
    Parent class: PyoObject

    Parameters:
    
    poly : int, optional
        Number of streams of polyphony generated. Defaults to 10.
    scale : int, optional
        Pitch output format. 0 = MIDI, 1 = Hertz, 2 = transpo. 
        In the transpo mode, the central key (the key where there 
        is no transposition) is (`first` + `last`) / 2.
    first : int, optional
        Lowest MIDI value. Defaults to 0.
    last : int, optional
        Highest MIDI value. Defaults to 127.

    Methods:

    get(identifier, all) : Return the first sample of the current 
        buffer as a float.

    Notes:
    
    Pitch and velocity are two separated set of streams. 
    The user should call :
    
    Notein['pitch'] to retrieve pitch streams.
    Notein['velocity'] to retrieve velocity streams.    

    Velocity is automatically scaled between 0 and 1.
    
    The out() method is bypassed. Notein's signal can not be sent 
    to audio outs.
    
    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = Notein(poly=10, scale=1, mul=.5)
    >>> p = Port(notes['velocity'], .001, .5)
    >>> b = Sine(freq=notes['pitch'], mul=p).out()
    >>> c = Sine(freq=notes['pitch'] * 0.997, mul=p).out()
    >>> d = Sine(freq=notes['pitch'] * 1.005, mul=p).out()
    
    """
    def __init__(self, poly=10, scale=0, first=0, last=127, mul=1, add=0):
        PyoObject.__init__(self)
        self._pitch_dummy = None
        self._velocity_dummy = None
        self._poly = poly
        self._scale = scale
        self._first = first
        self._last = last
        self._mul = mul
        self._add = add
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_handler = MidiNote_base(self._poly, self._scale, self._first, self._last)
        self._base_objs = []
        for i in range(lmax * poly):
            self._base_objs.append(Notein_base(self._base_handler, i, 0, 1, 0))
            self._base_objs.append(Notein_base(self._base_handler, i, 1, wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        self._base_handler.deleteStream()
        del self._base_handler

    def __getitem__(self, str):
        if str == 'pitch':
            if self._pitch_dummy == None:
                self._pitch_dummy = Dummy([self._base_objs[i*2] for i in range(self._poly)])
            return self._pitch_dummy
        if str == 'velocity':
            if self._velocity_dummy == None:
                self._velocity_dummy = Dummy([self._base_objs[i*2+1] for i in range(self._poly)])
            return self._velocity_dummy

    def get(self, identifier="pitch", all=False):
        """
        Return the first sample of the current buffer as a float.
        
        Can be used to convert audio stream to usable Python data.
        
        "pitch" or "velocity" must be given to `identifier` to specify
        which stream to get value from.
        
        Parameters:

            identifier : string {"pitch", "velocity"}
                Address string parameter identifying audio stream.
                Defaults to "pitch".
            all : boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.
                 
        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]
                        
    def play(self):
        self._base_handler.play()
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1):
        return self
    
    def stop(self):
        self._base_handler.stop()
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)
