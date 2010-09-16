"""
Play soundfiles from the disk.

SfMarker's objects use markers features (store in the header) from 
an AIFF file to create more specific reading patterns.

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
import aifc

class SfPlayer(PyoObject):
    """
    Soundfile player.
    
    Reads audio data from a file, and can alter its pitch using one of 
    several available interpolation types, as well as convert the sample 
    rate to match the Server sampling rate setting.
    
    Parent class: PyoObject
    
    Parameters:
    
    path : string
        Full path name of the sound to read.
    speed : float or PyoObject, optional
        Transpose the pitch of input sound by this factor. 1 is the 
        original pitch, lower values play sound slower, and higher 
        values play sound faster. Negative values results in playing 
        sound backward. At audio rate, fast changes between positive and
        negative values can result in strong DC components in the output 
        sound. Defaults to 1.
    loop : bool, optional
        If set to True, sound will play in loop. Defaults to False.
    offset : float, optional 
        Time in seconds of input sound to be skipped, assuming speed=1. 
        Defaults to 0.
    interp : int, optional
        Choice of the interpolation method. Defaults to 2.
            1 : no interpolation
            2 : linear
            3 : cosinus
            4 : cubic
        
    Methods:
    
    setSound(path) : Replace the `sound` attribute.
    setSpeed(x) : Replace the `speed` attribute.
    setLoop(x) : Replace the `loop` attribute.
    setOffset(x) : Replace the `offset` attribute.
    setInterp(x) : Replace the `interp` attribute.
    
    Attributes:
    
    sound : path, Full path of the sound.
    speed : float or PyoObject, Transposition factor.
    loop : bool, Looping mode.
    offset : float, Time, in seconds, of the first sample to read.
    interp : int {1, 2, 3, 4}, Interpolation method.
    
    Notes:
    
    SfPlayer will sends a trigger signal at the end of the playback if 
    loop is off or any time it wraps around if loop is on. User can 
    retreive the trigger streams by calling obj['trig']:
    
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif").out()
    >>> trig = TrigRand(sf['trig'])
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SNDS_PATH + "/transparent.aif"
    >>> sf = SfPlayer(snd, speed=.75, loop=True).out()
    
    """
    def __init__(self, path, speed=1, loop=False, offset=0, interp=2, mul=1, add=0):
        PyoObject.__init__(self)
        self._sound = path
        self._speed = speed
        self._loop = loop
        self._offset = offset
        self._interp = interp
        self._mul = mul
        self._add = add
        path, speed, loop, offset, interp, mul, add, lmax = convertArgsToLists(path, speed, loop, offset, interp, mul, add)
        self._base_players = []
        self._base_objs = []
        self._trig_objs = []
        for i in range(lmax):
            _snd_size, _dur, _snd_sr, _snd_chnls = sndinfo(path[0])
            self._base_players.append(SfPlayer_base(wrap(path,i), wrap(speed,i), wrap(loop,i), wrap(offset,i), wrap(interp,i)))
            for j in range(_snd_chnls):
                self._base_objs.append(SfPlay_base(self._base_players[-1], j, wrap(mul,i), wrap(add,i)))
                self._trig_objs.append(SfPlayTrig_base(self._base_players[-1], j))

    def __dir__(self):
        return ['sound', 'speed', 'loop', 'offset', 'interp', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj

    def __getitem__(self, i):
        if i == 'trig':
            return self._trig_objs
        
        if type(i) == SliceType:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        self._trig_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._trig_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._trig_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._trig_objs)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        [obj.stop() for obj in self._trig_objs]
        return self
        
    def setSound(self, path):
        """
        Sets a new sound to read.
        
        If the number of channels of the new sound doesn't match those 
        of the sound loaded at initialization time, erratic behaviors 
        can occur.
        
        Parameters:
        
        path : string
            Full path of the new sound.

        """
        self._sound = path
        path, lmax = convertArgsToLists(path)
        [obj.setSound(wrap(path,i)) for i, obj in enumerate(self._base_players)]

    def setSpeed(self, x):
        """
        Replace the `speed` attribute.
        
        Parameters:

        x : float or PyoObject
            new `speed` attribute.
        
        """
        self._speed = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpeed(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setLoop(self, x):
        """
        Replace the `loop` attribute.
        
        Parameters:

        x : bool {True, False}
            new `loop` attribute.
        
        """
        self._loop = x
        x, lmax = convertArgsToLists(x)
        for i, obj in enumerate(self._base_players):
            if wrap(x,i): obj.setLoop(1)
            else: obj.setLoop(0)

    def setOffset(self, x):
        """
        Replace the `offset` attribute.
        
        Parameters:

        x : float
            new `offset` attribute.
        
        """
        self._offset = x
        x, lmax = convertArgsToLists(x)
        [obj.setOffset(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setInterp(self, x):
        """
        Replace the `interp` attribute.
        
        Parameters:

        x : int {1, 2, 3, 4}
            new `interp` attribute.
        
        """
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(-2., 2., 'lin', 'speed', self._speed), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)
          
    @property
    def sound(self): 
        """string. Full path of the sound."""
        return self._sound
    @sound.setter
    def sound(self, x): self.setSound(x)
    
    @property
    def speed(self): 
        """float or PyoObject. Transposition factor."""
        return self._speed
    @speed.setter
    def speed(self, x): self.setSpeed(x)

    @property
    def loop(self): 
        """bool. Looping mode."""
        return self._loop
    @loop.setter
    def loop(self, x): self.setLoop(x)

    @property
    def offset(self): 
        """float. Time, in seconds, of the first sample to read."""
        return self._offset
    @offset.setter
    def offset(self, x): self.setOffset(x)

    @property
    def interp(self): 
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp
    @interp.setter
    def interp(self, x): self.setInterp(x)

class SfMarkerShuffler(PyoObject):
    """
    AIFF with markers soundfile shuffler.
    
    Reads audio data from a AIFF file, and can alter its pitch using 
    one of several available interpolation types, as well as convert 
    the sample rate to match the Server sampling rate setting. The 
    reading pointer choose randomly a marker (set in the header of the 
    file) as its starting point and reads the samples until it reaches 
    the following marker. Then, it choose another marker and so on...
    
    Parent class: PyoObject
    
    Parameters:
    
    path : string
        Full path name of the sound to read.
    speed : float or PyoObject, optional
        Transpose the pitch of input sound by this factor. 1 is the 
        original pitch, lower values play sound slower, and higher 
        values play sound faster. Negative values results in playing 
        sound backward. At audio rate, fast changes between positive and
        negative values can result in strong DC components in the output 
        sound. Defaults to 1.
    interp : int, optional
        Choice of the interpolation method. Defaults to 2.
            1 : no interpolation
            2 : linear
            3 : cosinus
            4 : cubic
        
    Methods:
    
    setSpeed(x) : Replace the `speed` attribute.
    setInterp(x) : Replace the `interp` attribute.
    getMarkers() : Returns a list of marker time values in samples.
    
    Attributes:
    
    speed : float or PyoObject, Transposition factor.
    interp : int {1, 2, 3, 4}, Interpolation method.
 
    Notes:
    
    Reading backward with fast changes at audio rate can generates strong 
    DC in the resulting sound.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfMarkerShuffler(SNDS_PATH + "/transparent.aif", speed=1).out()
    
    """
    def __init__(self, path, speed=1, interp=2, mul=1, add=0):
        PyoObject.__init__(self)
        self._speed = speed
        self._interp = interp
        self._mul = mul
        self._add = add
        path, speed, interp, mul, add, lmax = convertArgsToLists(path, speed, interp, mul, add)
        self._base_players = []
        self._base_objs = []
        self._snd_size, self._dur, self._snd_sr, self._snd_chnls = sndinfo(path[0])
        for i in range(lmax):
            try:
                sf = aifc.open(wrap(path,i))
                markerstmp = sf.getmarkers()
                sf.close()
                self._markers = [m[1] for m in markerstmp]
            except:
                self._markers = []    
            self._base_players.append(SfMarkerShuffler_base(wrap(path,i), self._markers, wrap(speed,i), wrap(interp,i)))
        for i in range(lmax * self._snd_chnls):
            j = i / self._snd_chnls
            self._base_objs.append(SfMarkerShuffle_base(wrap(self._base_players,j), i % self._snd_chnls, wrap(mul,j), wrap(add,j)))

    def __dir__(self):
        return ['speed', 'interp', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj
                        
    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self
        
    def setSpeed(self, x):
        """
        Replace the `speed` attribute.
        
        Parameters:

        x : float or PyoObject
            new `speed` attribute.
        
        """
        self._speed = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpeed(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setInterp(self, x):
        """
        Replace the `interp` attribute.
        
        Parameters:

        x : int {1, 2, 3, 4}
            new `interp` attribute.
        
        """
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def getMarkers(self):
        """
        Returns a list of marker time values in samples.
        
        """
        return self._markers
        
    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(0.01, 2., 'lin', 'speed', self._speed), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)
                    
    @property
    def speed(self): 
        """float or PyoObject. Transposition factor."""
        return self._speed
    @speed.setter
    def speed(self, x): self.setSpeed(x)

    @property
    def interp(self): 
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp
    @interp.setter
    def interp(self, x): self.setInterp(x)

class SfMarkerLooper(PyoObject):
    """
    AIFF with markers soundfile looper.

    Reads audio data from a AIFF file, and can alter its pitch using 
    one of several available interpolation types, as well as convert 
    the sample rate to match the Server sampling rate setting. The 
    reading pointer loops a specific marker (set in the header of the 
    file) until it received a new integer in the `mark` parameter.

    Parent class: PyoObject

    Parameters:

    path : string
        Full path name of the sound to read.
    speed : float or PyoObject, optional
        Transpose the pitch of input sound by this factor. 1 is the 
        original pitch, lower values play sound slower, and higher 
        values play sound faster. Negative values results in playing 
        sound backward. At audio rate, fast changes between positive and
        negative values can result in strong DC components in the output 
        sound. Defaults to 1.
    mark : float or PyoObject, optional
        Integer denoting the marker to loop, in the range 0 -> len(getMarkers()).
        Defaults to 0.
    interp : int, optional
        Choice of the interpolation method. Defaults to 2.
            1 : no interpolation
            2 : linear
            3 : cosinus
            4 : cubic

    Methods:

    setSpeed(x) : Replace the `speed` attribute.
    setInterp(x) : Replace the `interp` attribute.
    setMark(x) : Replace the `mark` attribute.
    getMarkers() : Returns a list of marker time values in samples.

    Attributes:

    speed : float or PyoObject, Transposition factor.
    mark : float or PyoObject, Marker to loop.
    interp : int {1, 2, 3, 4}, Interpolation method.

    Notes:

    Reading backward with fast changes at audio rate can generates strong 
    DC in the resulting sound.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfMarkerLooper(SNDS_PATH + '/transparent.aif').out()
    >>> rnd = RandInt(len(a.getMarkers()), 2)
    >>> a.mark = rnd

    """
    def __init__(self, path, speed=1, mark=0, interp=2, mul=1, add=0):
        PyoObject.__init__(self)
        self._speed = speed
        self._mark = mark
        self._interp = interp
        self._mul = mul
        self._add = add
        path, speed, mark, interp, mul, add, lmax = convertArgsToLists(path, speed, mark, interp, mul, add)
        self._base_players = []
        self._base_objs = []
        self._snd_size, self._dur, self._snd_sr, self._snd_chnls = sndinfo(path[0])
        for i in range(lmax):
            try:
                sf = aifc.open(wrap(path,i))
                markerstmp = sf.getmarkers()
                sf.close()
                self._markers = [m[1] for m in markerstmp]
            except:
                self._markers = []    
            self._base_players.append(SfMarkerLooper_base(wrap(path,i), self._markers, wrap(speed,i), wrap(mark,i), wrap(interp,i)))
        for i in range(lmax * self._snd_chnls):
            j = i / self._snd_chnls
            self._base_objs.append(SfMarkerLoop_base(wrap(self._base_players,j), i % self._snd_chnls, wrap(mul,j), wrap(add,j)))

    def __dir__(self):
        return ['speed', 'mark', 'interp', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def setSpeed(self, x):
        """
        Replace the `speed` attribute.

        Parameters:

        x : float or PyoObject
            new `speed` attribute.

        """
        self._speed = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpeed(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setMark(self, x):
        """
        Replace the `mark` attribute.

        Parameters:

        x : float or PyoObject
            new `mark` attribute.

        """
        self._mark = x
        x, lmax = convertArgsToLists(x)
        [obj.setMark(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        Parameters:

        x : int {1, 2, 3, 4}
            new `interp` attribute.

        """
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def getMarkers(self):
        """
        Returns a list of marker time values in samples.

        """
        return self._markers

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(0.01, 2., 'lin', 'speed', self._speed), 
                          SLMap(0, len(self._markers)-1, 'lin', 'mark', self._mark, 'int'),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

    @property
    def speed(self): 
        """float or PyoObject. Transposition factor."""
        return self._speed
    @speed.setter
    def speed(self, x): self.setSpeed(x)

    @property
    def mark(self): 
        """float or PyoObject. Marker to loop."""
        return self._marker
    @mark.setter
    def mark(self, x): self.setMark(x)

    @property
    def interp(self): 
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp
    @interp.setter
    def interp(self, x): self.setInterp(x)
