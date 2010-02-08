from _core import *
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
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> snd = DEMOS_PATH + "/transparent.aif"
    >>> sf = SfPlayer(snd, speed=.75, loop=True).out()
    
    """
    def __init__(self, path, speed=1, loop=False, offset=0, interp=2, mul=1, add=0):
        self._sound = path
        self._speed = speed
        self._loop = loop
        self._offset = offset
        self._interp = interp
        self._mul = mul
        self._add = add
        path, speed, loop, offset, interp, mul, add, lmax = convertArgsToLists(path, speed, loop, offset, interp, mul, add)
        self._base_objs = []
        self._snd_size, self._dur, self._snd_sr, self._snd_chnls = sndinfo(path[0])
        self._base_players = [SfPlayer_base(wrap(path,i), wrap(speed,i), wrap(loop,i), wrap(offset,i), wrap(interp,i)) for i in range(lmax)]
        for i in range(lmax * self._snd_chnls):
            j = i / self._snd_chnls
            self._base_objs.append(SfPlay_base(wrap(self._base_players,j), i % self._snd_chnls, wrap(mul,j), wrap(add,j)))

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj
                        
    def play(self):
        self._base_players = [obj.play() for obj in self._base_players]
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1):
        self._base_players = [obj.play() for obj in self._base_players]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
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
        if map_list == None:
            map_list = [SLMap(-2., 2., 'lin', 'speed', self._speed),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)
          
    #def demo():
    #    execfile(DEMOS_PATH + "/SfPlayer_demo.py")
    #demo = Call_example(demo)

    def args():
        return('SfPlayer(path, speed=1, loop=False, offset=0, interp=2, mul=1, add=0)')
    args = Print_args(args)
          
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
    
    Attributes:
    
    speed : float or PyoObject, Transposition factor.
    interp : int {1, 2, 3, 4}, Interpolation method.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfMarkerShuffler(DEMOS_PATH + "/transparent.aif", speed=1).out()
    
    """
    def __init__(self, path, speed=1, interp=2, mul=1, add=0):
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
                markers = [m[1] for m in markerstmp]
            except:
                markers = []    
            self._base_players.append(SfMarkerShuffler_base(wrap(path,i), markers, wrap(speed,i), wrap(interp,i)))
        for i in range(lmax * self._snd_chnls):
            j = i / self._snd_chnls
            self._base_objs.append(SfMarkerShuffle_base(wrap(self._base_players,j), i % self._snd_chnls, wrap(mul,j), wrap(add,j)))

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj
                        
    def play(self):
        self._base_players = [obj.play() for obj in self._base_players]
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1):
        self._base_players = [obj.play() for obj in self._base_players]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc) for i, obj in enumerate(self._base_objs)]
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

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(0.01, 2., 'lin', 'speed', self._speed),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)
 
    #def demo():
    #    execfile(DEMOS_PATH + "/SfMarkerShuffler_demo.py")
    #demo = Call_example(demo)

    def args():
        return('SfMarkerShuffler(path, speed=1, interp=2, mul=1, add=0)')
    args = Print_args(args)
                    
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
