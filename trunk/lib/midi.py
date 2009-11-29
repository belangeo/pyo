from _core import *

######################################################################
### MIDI
######################################################################                                       
class Midictl(PyoObject):
    def __init__(self, ctlnumber, minscale=0, maxscale=1, mul=1, add=0):
        self._mul = mul
        self._add = add
        ctlnumber, minscale, maxscale, mul, add, lmax = convertArgsToLists(ctlnumber, minscale, maxscale, mul, add)
        self._base_objs = [Midictl_base(wrap(ctlnumber,i), wrap(minscale,i), wrap(maxscale,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0):
        pass

class Notein(PyoObject):
    def __init__(self, voices=10, scale=0, first=0, last=127, mul=1, add=0):
        self._pitch_dummy = None
        self._velocity_dummy = None
        self._voices = voices
        self._scale = scale
        self._first = first
        self._last = last
        self._mul = mul
        self._add = add
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_handler = MidiNote_base(self._voices, self._scale, self._first, self._last)
        self._base_objs = []
        for i in range(lmax * voices):
            self._base_objs.append(Notein_base(self._base_handler, i, 0, 1, 0))
            self._base_objs.append(Notein_base(self._base_handler, i, 1, wrap(mul,i), wrap(add,i)))

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        self._base_handler.deleteStream()
        del self._base_handler

    def __getitem__(self, str):
        if str == 'pitch':
            if self._pitch_dummy == None:
                self._pitch_dummy = Dummy([self._base_objs[i*2] for i in range(self._voices)])
            return self._pitch_dummy
        if str == 'velocity':
            if self._velocity_dummy == None:
                self._velocity_dummy = Dummy([self._base_objs[i*2+1] for i in range(self._voices)])
            return self._velocity_dummy
                        
    def play(self):
        self._base_handler.play()
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0):
        return self
    
    def stop(self):
        self._base_handler.stop()
        [obj.stop() for obj in self._base_objs]
