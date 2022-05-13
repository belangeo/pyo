"""
Objects to manage values on an Open Sound Control port.

OscSend takes the first value of each buffersize and send it on an
OSC port.

OscReceive creates and returns audio streams from the value in its
input port.

The audio streams of these objects are essentially intended to be
controls and can't be sent to the output soundcard.

These objects are available only if pyo is built with OSC (Open Sound
Control) support.

"""

"""
Copyright 2009-2015 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from ._core import *
from ._maps import *


def assertOSCSupport(obj):
    if not withOSC():
        raise Exception("Pyo built without OSC support! '%s' objects is not available." % obj.__class__.__name__)


### TODO - Know bugs:
### OscListReceive.setValue(address, value) make the program segfault on quit (python 2.7 and 3.5).
### OscSend/OscReceive don't work with unicode on python 2.7 (ok on python 3.5)
###

######################################################################
### Open Sound Control
######################################################################
class OscSend(PyoObject):
    """
    Sends values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to share values to other softwares or other
    computers. Only the first value of each input buffersize will be
    sent on the OSC port.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal.
        port: int
            Port on which values are sent. Receiver should listen on the
            same port.
        address: string
            Address used on the port to identify values. Address is in
            the form of a Unix path (ex.: '/pitch').
        host: string, optional
            IP address of the target computer. The default, '127.0.0.1',
            is the localhost.

    .. note::

        The out() method is bypassed. OscSend's signal can not be sent
        to audio outs.

        OscSend has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine(freq=[1,1.5], mul=[100,.1], add=[600, .1])
    >>> b = OscSend(a, port=10001, address=['/pitch','/amp'])

    """

    def __init__(self, input, port, address, host="127.0.0.1"):
        assertOSCSupport(self)
        pyoArgsAssert(self, "oiss", input, port, address, host)
        PyoObject.__init__(self)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, port, address, host, lmax = convertArgsToLists(self._in_fader, port, address, host)
        self._base_objs = [
            OscSend_base(wrap(in_fader, i), wrap(port, i), wrap(address, i), wrap(host, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setBufferRate(self, x):
        """
        Sets how many buffers to wait before sending a new value.

        :Args:

            x: int
                Changes the data output frequency in multiples of the buffer size.
                Should be greater or equal to 1.

        """
        pyoArgsAssert(self, "I", x)
        [obj.setBufferRate(x) for obj in self._base_objs]

    @property
    def input(self):
        """PyoObject. Input signal."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class OscReceive(PyoObject):
    """
    Receives values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to receive values from other softwares or
    other computers. Get a value at the beginning of each buffersize
    and fill its buffer with it.

    :Parent: :py:class:`PyoObject`

    :Args:

        port: int
            Port on which values are received. Sender should output on
            the same port.

            Unlike OscSend object, there can be only one port per OscReceive
            object.

            Available at initialization time only.
        address: string
            Address used on the port to identify values. Address is in
            the form of a Unix path (ex.: '/pitch').

    .. note::

        Audio streams are accessed with the `address` string parameter.
        The user should call :

        OscReceive['/pitch'] to retreive stream named '/pitch'.

        The out() method is bypassed. OscReceive's signal can not be sent
        to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = OscReceive(port=10001, address=['/pitch', '/amp'])
    >>> b = Sine(freq=a['/pitch'], mul=a['/amp']).mix(2).out()

    """

    def __init__(self, port, address, mul=1, add=0):
        assertOSCSupport(self)
        pyoArgsAssert(self, "IsOO", port, address, mul, add)
        PyoObject.__init__(self, mul, add)
        address, mul, add, lmax = convertArgsToLists(address, mul, add)
        self._address = address
        self._mainReceiver = OscReceiver_base(port, address)
        self._base_objs = [
            OscReceive_base(self._mainReceiver, wrap(address, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def __getitem__(self, i):
        if type(i) in [bytes, str]:
            return self._base_objs[self._address.index(i)]
        elif i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print("'i' too large!")

    def getAddresses(self):
        """
        Returns the addresses managed by the object.

        """
        return self._address

    def addAddress(self, path, mul=1, add=0):
        """
        Adds new address(es) to the object's handler.

        :Args:

            path: string or list of strings
                New path(s) to receive from.
            mul: float or PyoObject
                Multiplication factor. Defaults to 1.
            add: float or PyoObject
                Addition factor. Defaults to 0.

        """
        pyoArgsAssert(self, "sOO", path, mul, add)
        path, lmax = convertArgsToLists(path)
        mul, add, lmax2 = convertArgsToLists(mul, add)
        for i, p in enumerate(path):
            if p not in self._address:
                self._mainReceiver.addAddress(p)
                self._address.append(p)
                self._base_objs.append(OscReceive_base(self._mainReceiver, p, wrap(mul, i), wrap(add, i)))
                self._base_objs[-1].play()

    def delAddress(self, path):
        """
        Removes address(es) from the object's handler.

        :Args:

            path: string or list of strings
                Path(s) to remove.

        """
        pyoArgsAssert(self, "s", path)
        path, lmax = convertArgsToLists(path)
        self._mainReceiver.delAddress(path)
        indexes = [self._address.index(p) for p in path]
        for ind in reversed(indexes):
            self._address.pop(ind)
            obj = self._base_objs.pop(ind)

    def setInterpolation(self, x):
        """
        Activate/Deactivate interpolation. Activated by default.

        :Args:

            x: boolean
                True activates the interpolation, False deactivates it.

        """
        pyoArgsAssert(self, "B", x)
        [obj.setInterpolation(x) for obj in self._base_objs]

    def setValue(self, path, value):
        """
        Sets value for a given address.

        :Args:

            path: string
                Address to which the value should be attributed.
            value: float
                Value to attribute to the given address.

        """
        pyoArgsAssert(self, "sn", path, value)
        path, value, lmax = convertArgsToLists(path, value)
        for i in range(lmax):
            p = wrap(path, i)
            if p in self._address:
                self._mainReceiver.setValue(p, wrap(value, i))
            else:
                print('Error: OscReceive.setValue, Illegal address "%s"' % p)

    def get(self, identifier=None, all=False):
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        Address as string must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier: string
                Address string parameter identifying audio stream.
                Defaults to None, useful when `all` is True to
                retreive all streams values.
            all: boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.

        """
        if not all:
            return self._base_objs[self._address.index(identifier)]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self._base_objs]

    def play(self, dur=0, delay=0):
        if type(dur) is list:
            maindur = dur[0]
        else:
            maindur = dur
        if type(delay) is list:
            maindelay = delay[0]
        else:
            maindelay = delay
        tmp = self._mainReceiver.play(maindur, maindelay)
        return PyoObject.play(self, dur, delay)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def stop(self, wait=0):
        self._mainReceiver.stop(wait)
        return PyoObject.stop(self, wait)


class OscDataSend(PyoObject):
    """
    Sends data values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to share values to other softwares or other
    computers. Values are sent on the form of a list containing `types`
    elements.

    :Parent: :py:class:`PyoObject`

    :Args:

        types: str
            String specifying the types sequence of the message to be sent.
            Possible values are:

            - "i": integer
            - "h": long integer
            - "f": float
            - "d": double
            - "s" ; string
            - "b": blob (list of chars)
            - "m": MIDI packet (list of 4 bytes: [midi port, status, data1, data2])
            - "c": char
            - "T": True
            - "F": False
            - "N": None (nil)

            The string "ssfi" indicates that the value to send will be a list
            containing two strings followed by a float and an integer.
        port: int
            Port on which values are sent. Receiver should listen on the
            same port.
        address: string
            Address used on the port to identify values. Address is in
            the form of a Unix path (ex.: '/pitch').
        host: string, optional
            IP address of the target computer. The default, '127.0.0.1',
            is the localhost.

    .. note::

        The out() method is bypassed. OscDataSend has no audio signal.

        OscDataSend has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> def pp(address, *args):
    ...     print(address)
    ...     print(args)
    >>> r = OscDataReceive(9900, "/data/test", pp)
    >>> # Send various types
    >>> a = OscDataSend("fissif", 9900, "/data/test")
    >>> msg = [3.14159, 1, "Hello", "world!", 2, 6.18]
    >>> a.send(msg)
    >>> # Send a blob
    >>> b = OscDataSend("b", 9900, "/data/test")
    >>> msg = [[chr(i) for i in range(10)]]
    >>> b.send(msg)
    >>> # Send a MIDI noteon on port 0
    >>> c = OscDataSend("m", 9900, "/data/test")
    >>> msg = [[0, 144, 60, 100]]
    >>> c.send(msg)

    """

    def __init__(self, types, port, address, host="127.0.0.1"):
        assertOSCSupport(self)
        pyoArgsAssert(self, "siss", types, port, address, host)
        PyoObject.__init__(self)
        types, port, address, host, lmax = convertArgsToLists(types, port, address, host)
        self._base_objs = [
            OscDataSend_base(wrap(types, i), wrap(port, i), wrap(address, i), wrap(host, i)) for i in range(lmax)
        ]
        self._addresses = {}
        for i, adr in enumerate(address):
            self._addresses[adr] = self._base_objs[i]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def getAddresses(self):
        """
        Returns the addresses managed by the object.

        """
        return list(self._addresses.keys())

    def addAddress(self, types, port, address, host="127.0.0.1"):
        """
        Adds new address(es) to the object's handler.

        :Args:

            types: str
                String specifying the types sequence of the message to be sent.
                Possible values are:
                - "i": integer
                - "h": long integer
                - "f": float
                - "d": double
                - "s" ; string
                - "b": blob (list of chars)
                - "m": MIDI packet (list of 4 bytes: [midi port, status, data1, data2])
                - "c": char
                - "T": True
                - "F": False
                - "N": None (nil)

                The string "ssfi" indicates that the value to send will be a list
                containing two strings followed by a float and an integer.
            port: int
                Port on which values are sent. Receiver should listen on the
                same port.
            address: string
                Address used on the port to identify values. Address is in
                the form of a Unix path (ex.: '/pitch').
            host: string, optional
                IP address of the target computer. The default, '127.0.0.1',
                is the localhost.

        """
        pyoArgsAssert(self, "siss", types, port, address, host)
        types, port, address, host, lmax = convertArgsToLists(types, port, address, host)
        objs = [OscDataSend_base(wrap(types, i), wrap(port, i), wrap(address, i), wrap(host, i)) for i in range(lmax)]
        self._base_objs.extend(objs)
        for i, adr in enumerate(address):
            self._addresses[adr] = objs[i]
        self.play()

    def delAddress(self, path):
        """
        Removes address(es) from the object's handler.

        :Args:

            path: string or list of strings
                Path(s) to remove.

        """
        pyoArgsAssert(self, "s", path)
        path, lmax = convertArgsToLists(path)
        for p in path:
            if p in self._addresses:
                self._base_objs.remove(self._addresses[p])
                del self._addresses[p]

    def send(self, msg, address=None):
        """
        Method used to send `msg` values as a list.

        :Args:

            msg: list
                List of values to send. Types of values in list
                must be of the kind defined of `types` argument
                given at the object's initialization.
            address: string, optional
                Address destination to send values. If None, values
                will be sent to all addresses managed by the object.

        """
        if address is None:
            pyoArgsAssert(self, "l", msg)
            [obj.send(msg) for obj in self._base_objs]
        else:
            pyoArgsAssert(self, "lS", msg, address)
            self._addresses[address].send(msg)


class OscDataReceive(PyoObject):
    """
    Receives data values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to receive data values from other softwares or
    other computers. When a message is received, the function given at the
    argument `function` is called with the current address destination in
    argument followed by a tuple of values.

    :Parent: :py:class:`PyoObject`

    :Args:

        port: int
            Port on which values are received. Sender should output on
            the same port. Unlike OscDataSend object, there can be only
            one port per OscDataReceive object. Available at initialization
            time only.
        address: string
            Address used on the port to identify values. Address is in
            the form of a Unix path (ex.: "/pitch"). There can be as many
            addresses as needed on a single port.
        function: callable (can't be a list)
            This function will be called whenever a message with a known
            address is received. there can be only one function per
            OscDataReceive object. Available at initialization time only.

    .. note::

        The header of the callable given at `function` argument must be in this form::

            def my_func(address, *args):
                ...

        The out() method is bypassed. OscDataReceive has no audio signal.

        OscDataReceive has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> def pp(address, *args):
    ...     print(address)
    ...     print(args)
    >>> r = OscDataReceive(9900, "/data/test", pp)
    >>> # Send various types
    >>> a = OscDataSend("fissif", 9900, "/data/test")
    >>> msg = [3.14159, 1, "Hello", "world!", 2, 6.18]
    >>> a.send(msg)
    >>> # Send a blob
    >>> b = OscDataSend("b", 9900, "/data/test")
    >>> msg = [[chr(i) for i in range(10)]]
    >>> b.send(msg)
    >>> # Send a MIDI noteon on port 0
    >>> c = OscDataSend("m", 9900, "/data/test")
    >>> msg = [[0, 144, 60, 100]]
    >>> c.send(msg)

    """

    def __init__(self, port, address, function):
        assertOSCSupport(self)
        pyoArgsAssert(self, "IsC", port, address, function)
        PyoObject.__init__(self)
        self._port = port
        self._function = WeakMethod(function)
        self._address, lmax = convertArgsToLists(address)
        # self._address is linked with list at C level
        self._base_objs = [OscDataReceive_base(port, self._address, self._function)]
        self._init_play()

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def getAddresses(self):
        """
        Returns the addresses managed by the object.

        """
        return self._address

    def addAddress(self, path):
        """
        Adds new address(es) to the object's handler.

        :Args:

            path: string or list of strings
                New path(s) to receive from.

        """
        pyoArgsAssert(self, "s", path)
        path, lmax = convertArgsToLists(path)
        for p in path:
            if p not in self._address:
                self._address.append(p)
                self._base_objs[0].addAddress(p)

    def delAddress(self, path):
        """
        Removes address(es) from the object's handler.

        :Args:

            path: string or list of strings
                Path(s) to remove.

        """
        pyoArgsAssert(self, "s", path)
        path, lmax = convertArgsToLists(path)
        for p in path:
            if p in self._address:
                index = self._address.index(p)
                self._base_objs[0].delAddress(index)
                self._address.remove(p)


class OscListReceive(PyoObject):
    """
    Receives list of values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to receive list of floating-point values from other
    softwares or other computers. The list are converted into audio streams.
    Get values at the beginning of each buffersize and fill buffers with them.

    :Parent: :py:class:`PyoObject`

    :Args:

        port: int
            Port on which values are received. Sender should output on
            the same port. Unlike OscSend object, there can be only one
            port per OscListReceive object. Available at initialization time
            only.
        address: string
            Address used on the port to identify values. Address is in
            the form of a Unix path (ex.: '/pitch').
        num: int, optional
            Length of the lists in input. The object will generate `num` audio
            streams per given address. Available at initialization time only.
            This value can't be a list. That means all addresses managed by an
            OscListReceive object are of the same length. Defaults to 8.

    .. note::

        Audio streams are accessed with the `address` string parameter.
        The user should call :

        OscReceive['/pitch'] to retreive list of streams named '/pitch'.

        The out() method is bypassed. OscReceive's signal can not be sent
        to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> # 8 oscillators
    >>> a = OscListReceive(port=10001, address=['/pitch', '/amp'], num=8)
    >>> b = Sine(freq=a['/pitch'], mul=a['/amp']).mix(2).out()

    """

    def __init__(self, port, address, num=8, mul=1, add=0):
        assertOSCSupport(self)
        pyoArgsAssert(self, "IsIOO", port, address, num, mul, add)
        PyoObject.__init__(self, mul, add)
        self._num = num
        self._op_duplicate = self._num
        address, mul, add, lmax = convertArgsToLists(address, mul, add)
        self._address = address
        self._mainReceiver = OscListReceiver_base(port, address, num)
        self._base_objs = [
            OscListReceive_base(self._mainReceiver, wrap(address, i), j, wrap(mul, i), wrap(add, i))
            for i in range(lmax)
            for j in range(self._num)
        ]
        self._init_play()

    def __getitem__(self, i):
        if type(i) in [bytes, str]:
            first = self._address.index(i) * self._num
            return self._base_objs[first : first + self._num]
        elif i < len(self._base_objs):
            first = i * self._num
            return self._base_objs[first : first + self._num]
        else:
            print("'i' too large!")

    def getAddresses(self):
        """
        Returns the addresses managed by the object.

        """
        return self._address

    def addAddress(self, path, mul=1, add=0):
        """
        Adds new address(es) to the object's handler.

        :Args:

            path: string or list of strings
                New path(s) to receive from.
            mul: float or PyoObject
                Multiplication factor. Defaults to 1.
            add: float or PyoObject
                Addition factor. Defaults to 0.

        """
        pyoArgsAssert(self, "sOO", path, mul, add)
        path, lmax = convertArgsToLists(path)
        mul, add, lmax2 = convertArgsToLists(mul, add)
        for i, p in enumerate(path):
            if p not in self._address:
                self._mainReceiver.addAddress(p)
                self._address.append(p)
                self._base_objs.extend(
                    [
                        OscListReceive_base(self._mainReceiver, p, j, wrap(mul, i), wrap(add, i))
                        for j in range(self._num)
                    ]
                )
        self.play()

    def delAddress(self, path):
        """
        Removes address(es) from the object's handler.

        :Args:

            path: string or list of strings
                Path(s) to remove.

        """
        pyoArgsAssert(self, "s", path)
        path, lmax = convertArgsToLists(path)
        self._mainReceiver.delAddress(path)
        indexes = [self._address.index(p) for p in path if p in self._address]
        for ind in reversed(indexes):
            self._address.pop(ind)
            first = ind * self._num
            for i in reversed(list(range(first, first + self._num))):
                obj = self._base_objs.pop(i)

    def setInterpolation(self, x):
        """
        Activate/Deactivate interpolation. Activated by default.

        :Args:

            x: boolean
                True activates the interpolation, False deactivates it.

        """
        pyoArgsAssert(self, "B", x)
        [obj.setInterpolation(x) for obj in self._base_objs]

    def setValue(self, path, value):
        """
        Sets value for a given address.

        :Args:

            path: string
                Address to which the value should be attributed.
            value: list of floats
                List of values to attribute to the given address.

        """
        pyoArgsAssert(self, "sl", path, value)
        path, lmax = convertArgsToLists(path)
        for i in range(lmax):
            p = wrap(path, i)
            if p in self._address:
                if type(value[0]) == list:
                    val = wrap(value, i)
                else:
                    val = value
                if len(val) == self._num:
                    self._mainReceiver.setValue(p, val)
                else:
                    print("Error: OscListReceive.setValue, value must be of the same length as the `num` attribute.")
            else:
                print('Error: OscListReceive.setValue, Illegal address "%s"' % p)

    def get(self, identifier=None, all=False):
        """
        Return the first list of samples of the current buffer as floats.

        Can be used to convert audio stream to usable Python data.

        Address as string must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier: string
                Address string parameter identifying audio stream.
                Defaults to None, useful when `all` is True to
                retreive all streams values.
            all: boolean, optional
                If True, the first list of values of each object's stream
                will be returned as a list of lists. Otherwise, only the
                the list of the object's identifier will be returned as a
                list of floats. Defaults to False.

        """
        if not all:
            first = self._address.index(identifier) * self._num
            return [obj._getStream().getValue() for obj in self._base_objs[first : first + self._num]]
        else:
            outlist = []
            for add in self._address:
                first = self._address.index(add) * self._num
                l = [obj._getStream().getValue() for obj in self._base_objs[first : first + self._num]]
                outlist.append(l)
            return outlist

    def play(self, dur=0, delay=0):
        self._mainReceiver.play(dur, delay)
        return PyoObject.play(self, dur, delay)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def stop(self, wait=0):
        self._mainReceiver.stop(wait)
        return PyoObject.stop(self, wait)
