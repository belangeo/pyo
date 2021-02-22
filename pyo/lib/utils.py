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
import threading, time


class Clean_objects(threading.Thread):

    def __init__(self, time, *args):
        threading.Thread.__init__(self)
        self.t = time
        self.args = args

    def run(self):
        time.sleep(self.t)
        for arg in self.args:
            try:
                arg.stop()
            except:
                pass
        for arg in self.args:
            del arg


class Print(PyoObject):

    def __init__(self, input, method=0, interval=0.25, message=""):
        pyoArgsAssert(self, "oins", input, method, interval, message)
        PyoObject.__init__(self)
        self._input = input
        self._method = method
        self._interval = interval
        self._message = message
        self._in_fader = InputFader(input)
        in_fader, method, interval, message, lmax = convertArgsToLists(self._in_fader, method, interval, message)
        self._base_objs = [
            Print_base(wrap(in_fader, i), wrap(method, i), wrap(interval, i), wrap(message, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMethod(self, x):
        pyoArgsAssert(self, "i", x)
        self._method = x
        x, lmax = convertArgsToLists(x)
        [obj.setMethod(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterval(self, x):
        pyoArgsAssert(self, "n", x)
        self._interval = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterval(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMessage(self, x):
        pyoArgsAssert(self, "s", x)
        self._message = x
        x, lmax = convertArgsToLists(x)
        [obj.setMessage(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def method(self):
        return self._method

    @method.setter
    def method(self, x):
        self.setMethod(x)

    @property
    def interval(self):
        return self._interval

    @interval.setter
    def interval(self, x):
        self.setInterval(x)

    @property
    def message(self):
        return self._message

    @message.setter
    def message(self, x):
        self.setMessage(x)


class Snap(PyoObject):

    def __init__(self, input, choice, scale=0, mul=1, add=0):
        pyoArgsAssert(self, "oliOO", input, choice, scale, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._choice = choice
        self._scale = scale
        self._in_fader = InputFader(input)
        in_fader, scale, mul, add, lmax = convertArgsToLists(self._in_fader, scale, mul, add)
        if type(choice[0]) != list:
            self._base_objs = [
                Snap_base(wrap(in_fader, i), choice, wrap(scale, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
            ]
        else:
            choicelen = len(choice)
            lmax = max(choicelen, lmax)
            self._base_objs = [
                Snap_base(wrap(in_fader, i), wrap(choice, i), wrap(scale, i), wrap(mul, i), wrap(add, i))
                for i in range(lmax)
            ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setChoice(self, x):
        pyoArgsAssert(self, "l", x)
        self._choice = x
        [obj.setChoice(x) for i, obj in enumerate(self._base_objs)]

    def setScale(self, x):
        pyoArgsAssert(self, "i", x)
        self._scale = x
        x, lmax = convertArgsToLists(x)
        [obj.setScale(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def choice(self):
        return self._choice

    @choice.setter
    def choice(self, x):
        self.setChoice(x)

    @property
    def scale(self):
        return self._scale

    @scale.setter
    def scale(self, x):
        self.setScale(x)


class Interp(PyoObject):

    def __init__(self, input, input2, interp=0.5, mul=1, add=0):
        pyoArgsAssert(self, "ooOOO", input, input2, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._input2 = input2
        self._interp = interp
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(input2)
        in_fader, in_fader2, interp, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, interp, mul, add
        )
        self._base_objs = [
            Interp_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(interp, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInput2(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)

    def setInterp(self, x):
        pyoArgsAssert(self, "O", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class SampHold(PyoObject):

    def __init__(self, input, controlsig, value=0.0, mul=1, add=0):
        pyoArgsAssert(self, "ooOOO", input, controlsig, value, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._controlsig = controlsig
        self._value = value
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(controlsig)
        in_fader, in_fader2, value, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, value, mul, add
        )
        self._base_objs = [
            SampHold_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(value, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setControlsig(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._controlsig = x
        self._in_fader2.setInput(x, fadetime)

    def setValue(self, x):
        pyoArgsAssert(self, "O", x)
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def controlsig(self):
        return self._controlsig

    @controlsig.setter
    def controlsig(self, x):
        self.setControlsig(x)

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)


class Denorm(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [Denorm_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class ControlRec(PyoObject):

    def __init__(self, input, filename, rate=1000, dur=0.0):
        pyoArgsAssert(self, "oSIN", input, filename, rate, dur)
        PyoObject.__init__(self)
        self._input = input
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._rate = rate
        self._dur = dur
        self._in_fader = InputFader(input).stop()
        in_fader, lmax = convertArgsToLists(self._in_fader)
        self._base_objs = [ControlRec_base(wrap(in_fader, i), rate, dur) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def write(self):
        for i, obj in enumerate(self._base_objs):
            f = open(os.path.join(self._path, "%s_%03d" % (self._name, i)), "w")
            [f.write("%f %f\n" % p) for p in obj.getData()]
            f.close()


class ControlRead(PyoObject):

    def __init__(self, filename, rate=1000, loop=False, interp=2, mul=1, add=0):
        pyoArgsAssert(self, "SIBIOO", filename, rate, loop, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._rate = rate
        self._loop = loop
        self._interp = interp
        files = sorted([f for f in os.listdir(self._path) if self._name + "_" in f])
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = []
        for i in range(len(files)):
            path = os.path.join(self._path, files[i])
            f = open(path, "r")
            values = [float(l.split()[1]) for l in f.readlines() if l.strip() != ""]
            f.close()
            self._base_objs.append(ControlRead_base(values, rate, loop, interp, wrap(mul, i), wrap(add, i)))
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setRate(self, x):
        pyoArgsAssert(self, "i", x)
        self._rate = x
        x, lmax = convertArgsToLists(x)
        [obj.setRate(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLoop(self, x):
        pyoArgsAssert(self, "b", x)
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def rate(self):
        return self._rate

    @rate.setter
    def rate(self, x):
        self.setRate(x)

    @property
    def loop(self):
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class NoteinRec(PyoObject):

    def __init__(self, input, filename):
        pyoArgsAssert(self, "oS", input, filename)
        PyoObject.__init__(self)
        self._input = input
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._in_pitch = self._input["pitch"]
        self.in_velocity = self._input["velocity"]
        in_pitch, in_velocity, lmax = convertArgsToLists(self._in_pitch, self.in_velocity)
        self._base_objs = [NoteinRec_base(wrap(in_pitch, i), wrap(in_velocity, i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def write(self):
        for i, obj in enumerate(self._base_objs):
            f = open(os.path.join(self._path, "%s_%03d" % (self._name, i)), "w")
            [f.write("%f %f %f\n" % p) for p in obj.getData()]
            f.close()


class NoteinRead(PyoObject):

    def __init__(self, filename, loop=False, mul=1, add=0):
        pyoArgsAssert(self, "SBOO", filename, loop, mul, add)
        PyoObject.__init__(self, mul, add)
        self._pitch_dummy = []
        self._velocity_dummy = []
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._loop = loop
        files = sorted([f for f in os.listdir(self._path) if self._name + "_" in f])
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = []
        _trig_objs_tmp = []
        self._poly = len(files)
        for i in range(self._poly):
            path = os.path.join(self._path, files[i])
            f = open(path, "r")
            vals = [l.split() for l in f.readlines() if l.strip() != ""]
            timestamps = [float(v[0]) for v in vals]
            pitches = [float(v[1]) for v in vals]
            amps = [float(v[2]) for v in vals]
            f.close()
            self._base_objs.append(NoteinRead_base(pitches, timestamps, loop))
            self._base_objs.append(NoteinRead_base(amps, timestamps, loop, wrap(mul, i), wrap(add, i)))
            _trig_objs_tmp.append(TriggerDummy_base(self._base_objs[-1]))
        self._trig_objs = Dummy(_trig_objs_tmp)
        self._init_play()

    def __getitem__(self, str):
        if str == "trig":
            return self._trig_objs
        if str == "pitch":
            self._pitch_dummy.append(Dummy([self._base_objs[i * 2] for i in range(self._poly)]))
            return self._pitch_dummy[-1]
        if str == "velocity":
            self._velocity_dummy.append(Dummy([self._base_objs[i * 2 + 1] for i in range(self._poly)]))
            return self._velocity_dummy[-1]

    def get(self, identifier="pitch", all=False):
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setLoop(self, x):
        pyoArgsAssert(self, "b", x)
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def loop(self):
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)


class DBToA(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [DBToA_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class AToDB(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [AToDB_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Scale(PyoObject):

    def __init__(self, input, inmin=0, inmax=1, outmin=0, outmax=1, exp=1, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOnOO", input, inmin, inmax, outmin, outmax, exp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._inmin = inmin
        self._inmax = inmax
        self._outmin = outmin
        self._outmax = outmax
        self._exp = exp
        self._in_fader = InputFader(input)
        in_fader, inmin, inmax, outmin, outmax, exp, mul, add, lmax = convertArgsToLists(
            self._in_fader, inmin, inmax, outmin, outmax, exp, mul, add
        )
        self._base_objs = [
            Scale_base(
                wrap(in_fader, i),
                wrap(inmin, i),
                wrap(inmax, i),
                wrap(outmin, i),
                wrap(outmax, i),
                wrap(exp, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInMin(self, x):
        pyoArgsAssert(self, "O", x)
        self._inmin = x
        x, lmax = convertArgsToLists(x)
        [obj.setInMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._inmax = x
        x, lmax = convertArgsToLists(x)
        [obj.setInMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOutMin(self, x):
        pyoArgsAssert(self, "O", x)
        self._outmin = x
        x, lmax = convertArgsToLists(x)
        [obj.setOutMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOutMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._outmax = x
        x, lmax = convertArgsToLists(x)
        [obj.setOutMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        pyoArgsAssert(self, "n", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def inmin(self):
        return self._inmin

    @inmin.setter
    def inmin(self, x):
        self.setInMin(x)

    @property
    def inmax(self):
        return self._inmax

    @inmax.setter
    def inmax(self, x):
        self.setInMax(x)

    @property
    def outmin(self):
        return self._outmin

    @outmin.setter
    def outmin(self, x):
        self.setOutMin(x)

    @property
    def outmax(self):
        return self._outmax

    @outmax.setter
    def outmax(self, x):
        self.setOutMax(x)

    @property
    def exp(self):
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)


class CentsToTranspo(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [CentsToTranspo_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class TranspoToCents(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [TranspoToCents_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class MToF(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [MToF_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class FToM(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [FToM_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class MToT(PyoObject):

    def __init__(self, input, centralkey=60.0, mul=1, add=0):
        pyoArgsAssert(self, "onOO", input, centralkey, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._centralkey = centralkey
        self._in_fader = InputFader(input)
        in_fader, centralkey, mul, add, lmax = convertArgsToLists(self._in_fader, centralkey, mul, add)
        self._base_objs = [
            MToT_base(wrap(in_fader, i), wrap(centralkey, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setCentralKey(self, x):
        pyoArgsAssert(self, "n", x)
        self._centralkey = x
        x, lmax = convertArgsToLists(x)
        [obj.setCentralKey(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def centralkey(self):
        return self._centralkey

    @centralkey.setter
    def centralkey(self, x):
        self.setCentralKey(x)


class Between(PyoObject):

    def __init__(self, input, min=-1.0, max=1.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, min, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [
            Between_base(wrap(in_fader, i), wrap(min, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class TrackHold(PyoObject):

    def __init__(self, input, controlsig, value=0.0, mul=1, add=0):
        pyoArgsAssert(self, "ooOOO", input, controlsig, value, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._controlsig = controlsig
        self._value = value
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(controlsig)
        in_fader, in_fader2, value, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, value, mul, add
        )
        self._base_objs = [
            TrackHold_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(value, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setControlsig(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._controlsig = x
        self._in_fader2.setInput(x, fadetime)

    def setValue(self, x):
        pyoArgsAssert(self, "O", x)
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def controlsig(self):
        return self._controlsig

    @controlsig.setter
    def controlsig(self, x):
        self.setControlsig(x)

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)


class Resample(PyoObject):

    def __init__(self, input, mode=1, mul=1, add=0):
        pyoArgsAssert(self, "oiOO", input, mode, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._mode = mode
        _input, mode, mul, add, lmax = convertArgsToLists(input, mode, mul, add)
        self._base_objs = [
            Resample_base(wrap(_input, i), wrap(mode, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setMode(self, x):
        pyoArgsAssert(self, "i", x)
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, x):
        self.setMode(x)
