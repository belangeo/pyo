from _core import *
import aifc

class SfPlayer(PyoObject):
    def __init__(self, path, speed=1, loop=False, offset=0, interp=0, mul=1, add=0):
        self._speed = speed
        self._loop = loop
        self._offset = offset
        self._interp = interp
        self._mul = mul
        self._add = add
        path, speed, loop, offset, interp, mul, add, lmax = convertArgsToLists(path, speed, loop, offset, interp, mul, add)
        self._base_objs = []
        self._snd_size, self._snd_sr, self._snd_chnls = sndinfo(path[0])
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

    def setSound(self, x):
        x, lmax = convertArgsToLists(x)
        [obj.setSound(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setSpeed(self, x):
        self._speed = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpeed(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setLoop(self, x):
        self._loop = x
        x, lmax = convertArgsToLists(x)
        for i, obj in enumerate(self._base_players):
            if wrap(x,i): obj.setLoop(1)
            else: obj.setLoop(0)

    def setOffset(self, x):
        self._offset = x
        x, lmax = convertArgsToLists(x)
        [obj.setOffset(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setInterp(self, x):
        self.interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_players)]
          
    #def demo():
    #    execfile("demos/SfPlayer_demo.py")
    #demo = Call_example(demo)

    def args():
        print('SfPlayer(path, speed=1, loop=False, offset=0, interp=0, mul=1, add=0)')
    args = Print_args(args)
          
    @property
    def speed(self): return self._speed
    @property
    def loop(self): return self._loop
    @property
    def offset(self): return self._offset
    @property
    def interp(self): return self._interp
    @speed.setter
    def speed(self, x): self.setSpeed(x)
    @loop.setter
    def loop(self, x): self.setLoop(x)
    @offset.setter
    def offset(self, x): self.setOffset(x)
    @interp.setter
    def interp(self, x): self.setInterp(x)

class SfMarkerShuffler(PyoObject):
    def __init__(self, path, speed=1, interp=0, mul=1, add=0):
        self._speed = speed
        self._interp = interp
        self._mul = mul
        self._add = add
        path, speed, interp, mul, add, lmax = convertArgsToLists(path, speed, interp, mul, add)
        self._base_players = []
        self._base_objs = []
        self._snd_size, self._snd_sr, self._snd_chnls = sndinfo(path[0])
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

    def setSpeed(self, x):
        self._speed = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpeed(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setInterp(self, x):
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    #def demo():
    #    execfile("demos/SfMarkerShuffler_demo.py")
    #demo = Call_example(demo)

    def args():
        print('SfMarkerShuffler(path, speed=1, interp=0, mul=1, add=0)')
    args = Print_args(args)
                    
    @property
    def speed(self): return self._speed
    @property
    def interp(self): return self._interp
    @speed.setter
    def speed(self, x): self.setSpeed(x)
    @interp.setter
    def interp(self, x): self.setInterp(x)
