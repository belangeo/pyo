# Copyright 2009-2021 Olivier Belanger
# 
# This file is part of pyo, a python module to help digital signal
# processing script creation.
#
# pyo is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# pyo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with pyo.  If not, see <http://www.gnu.org/licenses/>.

from ._core import *


class FFT(PyoObject):

    def __init__(self, input, size=1024, overlaps=4, wintype=2):
        pyoArgsAssert(self, "oiIi", input, size, overlaps, wintype)
        PyoObject.__init__(self)
        self._real_dummy = []
        self._imag_dummy = []
        self._bin_dummy = []
        self._input = input
        self._size = size
        self._overlaps = overlaps
        self._wintype = wintype
        self._in_fader = InputFader(input)
        in_fader, size, wintype, lmax = convertArgsToLists(self._in_fader, size, wintype)
        self._base_players = []
        for j in range(overlaps):
            for i in range(lmax):
                hopsize = wrap(size, i) * j // overlaps
                self._base_players.append(FFTMain_base(wrap(in_fader, i), wrap(size, i), hopsize, wrap(wintype, i)))
        self._real_objs = []
        self._imag_objs = []
        self._bin_objs = []
        for j in range(len(self._base_players)):
            self._real_objs.append(FFT_base(wrap(self._base_players, j), 0, self._mul, self._add))
            self._imag_objs.append(FFT_base(wrap(self._base_players, j), 1, self._mul, self._add))
            self._bin_objs.append(FFT_base(wrap(self._base_players, j), 2, self._mul, self._add))
        self._base_objs = [Sig(0)]  # Dummy objs to prevent PyoObjectBase methods to fail.
        self._init_play()

    def __len__(self):
        return len(self._real_objs)

    def __getitem__(self, str):
        if str == "real":
            self._real_dummy.append(Dummy([obj for i, obj in enumerate(self._real_objs)]))
            return self._real_dummy[-1]
        if str == "imag":
            self._imag_dummy.append(Dummy([obj for i, obj in enumerate(self._imag_objs)]))
            return self._imag_dummy[-1]
        if str == "bin":
            self._bin_dummy.append(Dummy([obj for i, obj in enumerate(self._bin_objs)]))
            return self._bin_dummy[-1]

    def get(self, identifier="real", all=False):
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._autoplay(dur, delay)
        self._in_fader.play(dur, delay)
        self._base_players = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_players)]
        self._real_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._real_objs)]
        self._imag_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._imag_objs)]
        self._bin_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._bin_objs)]
        return self

    def stop(self, wait=0):
        self._autostop(wait)
        self._in_fader.stop(wait)
        [obj.stop(wait) for obj in self._base_players]
        [obj.stop(wait) for obj in self._real_objs]
        [obj.stop(wait) for obj in self._imag_objs]
        [obj.stop(wait) for obj in self._bin_objs]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setSize(self, x):
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        poly = len(self._base_players) // self._overlaps
        for j in range(self._overlaps):
            for i in range(poly):
                hopsize = wrap(x, i) * j // self._overlaps
                self._base_players[j * poly + i].setSize(wrap(x, i), hopsize)

    def setWinType(self, x):
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinType(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def wintype(self):
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)


class IFFT(PyoObject):

    def __init__(self, inreal, inimag, size=1024, overlaps=4, wintype=2, mul=1, add=0):
        pyoArgsAssert(self, "ooiIiOO", inreal, inimag, size, overlaps, wintype, mul, add)
        PyoObject.__init__(self, mul, add)
        self._inreal = inreal
        self._inimag = inimag
        self._size = size
        self._overlaps = overlaps
        self._wintype = wintype
        self._in_fader = InputFader(inreal)
        self._in_fader2 = InputFader(inimag)
        in_fader, in_fader2, size, wintype, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, size, wintype, mul, add
        )
        self._base_objs = []
        ratio = lmax // overlaps
        for i in range(lmax):
            hopsize = wrap(size, i) * ((i // ratio) % overlaps) // overlaps
            self._base_objs.append(
                IFFT_base(
                    wrap(in_fader, i),
                    wrap(in_fader2, i),
                    wrap(size, i),
                    hopsize,
                    wrap(wintype, i),
                    wrap(mul, i),
                    wrap(add, i),
                )
            )
        self._init_play()

    def __len__(self):
        return len(self._inreal)

    def setInReal(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inreal = x
        self._in_fader.setInput(x, fadetime)

    def setInImag(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inimag = x
        self._in_fader2.setInput(x, fadetime)

    def setSize(self, x):
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        ratio = len(self._base_objs) // self._overlaps
        for i, obj in enumerate(self._base_objs):
            hopsize = wrap(x, i) * ((i // ratio) % self._overlaps) // self._overlaps
            self._base_objs[i].setSize(wrap(x, i), hopsize)

    def setWinType(self, x):
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def inreal(self):
        return self._inreal

    @inreal.setter
    def inreal(self, x):
        self.setInReal(x)

    @property
    def inimag(self):
        return self._inimag

    @inimag.setter
    def inimag(self, x):
        self.setInImag(x)

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def wintype(self):
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)


class CarToPol(PyoObject):

    def __init__(self, inreal, inimag, mul=1, add=0):
        pyoArgsAssert(self, "ooOO", inreal, inimag, mul, add)
        PyoObject.__init__(self, mul, add)
        self._mag_dummy = []
        self._ang_dummy = []
        self._inreal = inreal
        self._inimag = inimag
        self._in_fader = InputFader(inreal)
        self._in_fader2 = InputFader(inimag)
        in_fader, in_fader2, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, mul, add)
        self._base_objs = []
        for i in range(lmax):
            self._base_objs.append(CarToPol_base(wrap(in_fader, i), wrap(in_fader2, i), 0, wrap(mul, i), wrap(add, i)))
            self._base_objs.append(CarToPol_base(wrap(in_fader, i), wrap(in_fader2, i), 1, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def __len__(self):
        return len(self._inreal)

    def __getitem__(self, str):
        if str == "mag":
            self._mag_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if i % 2 == 0]))
            return self._mag_dummy[-1]
        if str == "ang":
            self._ang_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if i % 2 == 1]))
            return self._ang_dummy[-1]

    def get(self, identifier="mag", all=False):
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def setInReal(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inreal = x
        self._in_fader.setInput(x, fadetime)

    def setInImag(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inimag = x
        self._in_fader2.setInput(x, fadetime)

    @property
    def inreal(self):
        return self._inreal

    @inreal.setter
    def inreal(self, x):
        self.setInReal(x)

    @property
    def inimag(self):
        return self._inimag

    @inimag.setter
    def inimag(self, x):
        self.setInImag(x)


class PolToCar(PyoObject):

    def __init__(self, inmag, inang, mul=1, add=0):
        pyoArgsAssert(self, "ooOO", inmag, inang, mul, add)
        PyoObject.__init__(self, mul, add)
        self._real_dummy = []
        self._imag_dummy = []
        self._inmag = inmag
        self._inang = inang
        self._in_fader = InputFader(inmag)
        self._in_fader2 = InputFader(inang)
        in_fader, in_fader2, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, mul, add)
        self._base_objs = []
        for i in range(lmax):
            self._base_objs.append(PolToCar_base(wrap(in_fader, i), wrap(in_fader2, i), 0, wrap(mul, i), wrap(add, i)))
            self._base_objs.append(PolToCar_base(wrap(in_fader, i), wrap(in_fader2, i), 1, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def __len__(self):
        return len(self._inmag)

    def __getitem__(self, str):
        if str == "real":
            self._real_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if i % 2 == 0]))
            return self._real_dummy[-1]
        if str == "imag":
            self._imag_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if i % 2 == 1]))
            return self._imag_dummy[-1]

    def get(self, identifier="real", all=False):
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def setInMag(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inmag = x
        self._in_fader.setInput(x, fadetime)

    def setInAng(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inang = x
        self._in_fader2.setInput(x, fadetime)

    @property
    def inmag(self):
        return self._inmag

    @inmag.setter
    def inmag(self, x):
        self.setInMag(x)

    @property
    def inang(self):
        return self._inang

    @inang.setter
    def inang(self, x):
        self.setInAng(x)


class FrameDelta(PyoObject):

    def __init__(self, input, framesize=1024, overlaps=4, mul=1, add=0):
        pyoArgsAssert(self, "oiiOO", input, framesize, overlaps, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._framesize = framesize
        self._overlaps = overlaps
        self._in_fader = InputFader(input)
        in_fader, framesize, overlaps, mul, add, lmax = convertArgsToLists(
            self._in_fader, framesize, overlaps, mul, add
        )
        num_of_mains = len(self._in_fader) // self._overlaps
        self._base_players = []
        for j in range(num_of_mains):
            objs_list = []
            for i in range(len(self._in_fader)):
                if (i % num_of_mains) == j:
                    objs_list.append(self._in_fader[i])
            self._base_players.append(FrameDeltaMain_base(objs_list, wrap(framesize, j), wrap(overlaps, j)))
        self._base_objs = []
        for i in range(lmax):
            base_player = i % num_of_mains
            overlap = i // num_of_mains
            self._base_objs.append(
                FrameDelta_base(self._base_players[base_player], overlap, wrap(mul, i), wrap(add, i))
            )
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFrameSize(self, x):
        pyoArgsAssert(self, "i", x)
        self._framesize = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrameSize(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def framesize(self):
        return self._framesize

    @framesize.setter
    def framesize(self, x):
        self.setFrameSize(x)


class FrameAccum(PyoObject):

    def __init__(self, input, framesize=1024, overlaps=4, mul=1, add=0):
        pyoArgsAssert(self, "oiiOO", input, framesize, overlaps, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._framesize = framesize
        self._overlaps = overlaps
        self._in_fader = InputFader(input)
        in_fader, framesize, overlaps, mul, add, lmax = convertArgsToLists(
            self._in_fader, framesize, overlaps, mul, add
        )
        num_of_mains = len(self._in_fader) // self._overlaps
        self._base_players = []
        for j in range(num_of_mains):
            objs_list = []
            for i in range(len(self._in_fader)):
                if (i % num_of_mains) == j:
                    objs_list.append(self._in_fader[i])
            self._base_players.append(FrameAccumMain_base(objs_list, wrap(framesize, j), wrap(overlaps, j)))
        self._base_objs = []
        for i in range(lmax):
            base_player = i % num_of_mains
            overlap = i // num_of_mains
            self._base_objs.append(
                FrameAccum_base(self._base_players[base_player], overlap, wrap(mul, i), wrap(add, i))
            )
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFrameSize(self, x):
        pyoArgsAssert(self, "i", x)
        self._framesize = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrameSize(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def framesize(self):
        return self._framesize

    @framesize.setter
    def framesize(self, x):
        self.setFrameSize(x)


class Vectral(PyoObject):

    def __init__(self, input, framesize=1024, overlaps=4, up=1.0, down=0.7, damp=0.9, mul=1, add=0):
        pyoArgsAssert(self, "oiiOOOOO", input, framesize, overlaps, up, down, damp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._framesize = framesize
        self._overlaps = overlaps
        self._up = up
        self._down = down
        self._damp = damp
        self._in_fader = InputFader(input)
        in_fader, framesize, overlaps, up, down, damp, mul, add, lmax = convertArgsToLists(
            self._in_fader, framesize, overlaps, up, down, damp, mul, add
        )
        num_of_mains = len(self._in_fader) // self._overlaps
        self._base_players = []
        for j in range(num_of_mains):
            objs_list = []
            for i in range(len(self._in_fader)):
                if (i % num_of_mains) == j:
                    objs_list.append(self._in_fader[i])
            self._base_players.append(
                VectralMain_base(
                    objs_list, wrap(framesize, j), wrap(overlaps, j), wrap(up, j), wrap(down, j), wrap(damp, j)
                )
            )
        self._base_objs = []
        for i in range(lmax):
            base_player = i % num_of_mains
            overlap = i // num_of_mains
            self._base_objs.append(Vectral_base(self._base_players[base_player], overlap, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFrameSize(self, x):
        pyoArgsAssert(self, "i", x)
        self._framesize = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrameSize(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setUp(self, x):
        pyoArgsAssert(self, "O", x)
        self._up = x
        x, lmax = convertArgsToLists(x)
        [obj.setUp(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDown(self, x):
        pyoArgsAssert(self, "O", x)
        self._down = x
        x, lmax = convertArgsToLists(x)
        [obj.setDown(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDamp(self, x):
        pyoArgsAssert(self, "O", x)
        self._damp = x
        x, lmax = convertArgsToLists(x)
        [obj.setDamp(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def framesize(self):
        return self._framesize

    @framesize.setter
    def framesize(self, x):
        self.setFrameSize(x)

    @property
    def up(self):
        return self._up

    @up.setter
    def up(self, x):
        self.setUp(x)

    @property
    def down(self):
        return self._down

    @down.setter
    def down(self, x):
        self.setDown(x)

    @property
    def damp(self):
        return self._damp

    @damp.setter
    def damp(self, x):
        self.setDamp(x)


class IFFTMatrix(PyoObject):

    def __init__(self, matrix, index, phase, size=1024, overlaps=4, wintype=2, mul=1, add=0):
        pyoArgsAssert(self, "mooiIiOO", matrix, index, phase, size, overlaps, wintype, mul, add)
        PyoObject.__init__(self, mul, add)
        self._matrix = matrix
        self._index = index
        self._phase = phase
        self._size = size
        self._overlaps = overlaps
        self._wintype = wintype
        matrix, index, phase, size, wintype, mul, add, self._lmax = convertArgsToLists(
            matrix, index, phase, size, wintype, mul, add
        )
        self._base_objs = []
        for j in range(overlaps):
            for i in range(self._lmax):
                hopsize = int(wrap(size, i) / overlaps) * j
                self._base_objs.append(
                    IFFTMatrix_base(
                        wrap(matrix, i),
                        wrap(index, i),
                        wrap(phase, i),
                        wrap(size, i),
                        hopsize,
                        wrap(wintype, i),
                        wrap(mul, i),
                        wrap(add, i),
                    )
                )
        self._init_play()

    def __len__(self):
        return int(len(self._base_objs) / self._overlaps)

    def setIndex(self, x):
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        for j in range(overlaps):
            for i in range(self._lmax):
                self._base_objs[j * self._overlaps + i].setIndex(wrap(x, i))

    def setPhase(self, x):
        pyoArgsAssert(self, "o", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        for j in range(overlaps):
            for i in range(self._lmax):
                self._base_objs[j * self._overlaps + i].setPhase(wrap(x, i))

    def setSize(self, x):
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        for j in range(overlaps):
            for i in range(self._lmax):
                hopsize = int(wrap(x, i) / self._overlaps) * j
                self._base_objs[j * self._overlaps + i].setSize(wrap(x, i), hopsize)

    def setWinType(self, x):
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        for j in range(overlaps):
            for i in range(self._lmax):
                self._base_objs[j * self._overlaps + i].setWinType(wrap(x, i))

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)

    @property
    def phase(self):
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def wintype(self):
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)
