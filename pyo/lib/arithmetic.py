"""
Tools to perform arithmetic operations on audio signals.

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


class Sin(PyoObject):
    """
    Performs a sine function on audio signal.

    Returns the sine of audio signal as input.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, angle in radians.

    >>> s = Server().boot()
    >>> s.start()
    >>> import math
    >>> a = Phasor(500, mul=math.pi*2)
    >>> b = Sin(a, mul=.3).mix(2).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Sin_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Cos(PyoObject):
    """
    Performs a cosine function on audio signal.

    Returns the cosine of audio signal as input.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, angle in radians.

    >>> s = Server().boot()
    >>> s.start()
    >>> import math
    >>> a = Phasor(500, mul=math.pi*2)
    >>> b = Cos(a, mul=.3).mix(2).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Cos_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Tan(PyoObject):
    """
    Performs a tangent function on audio signal.

    Returns the tangent of audio signal as input.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, angle in radians.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Tangent panning function
    >>> import math
    >>> src = Sine(mul=.3)
    >>> a = Phasor(freq=1, mul=90, add=-45)
    >>> b = Tan(Abs(a*math.pi/180))
    >>> b1 = 1.0 - b
    >>> oL = src * b
    >>> oR = src * b1
    >>> oL.out()
    >>> oR.out(1)

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Tan_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Abs(PyoObject):
    """
    Performs an absolute function on audio signal.

    Returns the absolute value of audio signal as input.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Back-and-Forth playback
    >>> t = SndTable(SNDS_PATH + "/transparent.aif")
    >>> a = Phasor(freq=t.getRate()*0.5, mul=2, add=-1)
    >>> b = Pointer(table=t, index=Abs(a), mul=0.5).mix(2).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Abs_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Sqrt(PyoObject):
    """
    Performs a square-root function on audio signal.

    Returns the square-root value of audio signal as input.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Equal-power panning function
    >>> src = Sine(mul=.3)
    >>> a = Abs(Phasor(freq=1, mul=2, add=-1))
    >>> left = Sqrt(1.0 - a)
    >>> right = Sqrt(a)
    >>> oL = src * left
    >>> oR = src * right
    >>> oL.out()
    >>> oR.out(1)

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Sqrt_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Log(PyoObject):
    """
    Performs a natural log function on audio signal.

    Returns the natural log value of of audio signal as input.
    Values less than 0.0 return 0.0.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    # Logarithmic amplitude envelope
    >>> a = LFO(freq=1, type=3, mul=0.2, add=1.2) # triangle
    >>> b = Log(a)
    >>> c = SineLoop(freq=[300,301], feedback=0.05, mul=b).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Log_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Log2(PyoObject):
    """
    Performs a base 2 log function on audio signal.

    Returns the base 2 log value of audio signal as input.
    Values less than 0.0 return 0.0.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    # Logarithmic amplitude envelope
    >>> a = LFO(freq=1, type=3, mul=0.1, add=1.1) # triangle
    >>> b = Log2(a)
    >>> c = SineLoop(freq=[300,301], feedback=0.05, mul=b).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Log2_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Log10(PyoObject):
    """
    Performs a base 10 log function on audio signal.

    Returns the base 10 log value of audio signal as input.
    Values less than 0.0 return 0.0.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    # Logarithmic amplitude envelope
    >>> a = LFO(freq=1, type=3, mul=0.4, add=1.4) # triangle
    >>> b = Log10(a)
    >>> c = SineLoop(freq=[300,301], feedback=0.05, mul=b).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Log10_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Atan2(PyoObject):
    """
    Computes the principal value of the arc tangent of b/a.

    Computes the principal value of the arc tangent of b/a,
    using the signs of both arguments to determine the quadrant
    of the return value.

    :Parent: :py:class:`PyoObject`

    :Args:

        b: float or PyoObject, optional
            Numerator. Defaults to 1.
        a: float or PyoObject, optional
            Denominator. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Simple distortion
    >>> a = Sine(freq=[200,200.3])
    >>> lf = Sine(freq=1, mul=.2, add=.2)
    >>> dist = Atan2(a, lf)
    >>> lp = Tone(dist, freq=2000, mul=.1).out()

    """

    def __init__(self, b=1, a=1, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", b, a, mul, add)
        PyoObject.__init__(self, mul, add)
        self._b = b
        self._a = a
        b, a, mul, add, lmax = convertArgsToLists(b, a, mul, add)
        self._base_objs = [M_Atan2_base(wrap(b, i), wrap(a, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setB(self, x):
        """
        Replace the `b` attribute.

        :Args:

            x: float or PyoObject
                new `b` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._b = x
        x, lmax = convertArgsToLists(x)
        [obj.setB(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setA(self, x):
        """
        Replace the `a` attribute.

        :Args:

            x: float or PyoObject
                new `a` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._a = x
        x, lmax = convertArgsToLists(x)
        [obj.setA(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def b(self):
        """float or PyoObject. Numerator."""
        return self._b

    @b.setter
    def b(self, x):
        self.setB(x)

    @property
    def a(self):
        """float or PyoObject. Denominator."""
        return self._a

    @a.setter
    def a(self, x):
        self.setA(x)


class Floor(PyoObject):
    """
    Rounds to largest integral value not greater than audio signal.

    For each samples in the input signal, rounds to the largest integral
    value not greater than the sample value.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Clipping frequencies
    >>> sweep = Phasor(freq=[1,.67], mul=4)
    >>> flo = Floor(sweep, mul=50, add=200)
    >>> a = SineLoop(freq=flo, feedback=.1, mul=.3).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Floor_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Ceil(PyoObject):
    """
    Rounds to smallest integral value greater than or equal to the input signal.

    For each samples in the input signal, rounds to the smallest integral
    value greater than or equal to the sample value.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Clipping frequencies
    >>> sweep = Phasor(freq=[1,.67], mul=4)
    >>> flo = Ceil(sweep, mul=50, add=200)
    >>> a = SineLoop(freq=flo, feedback=.1, mul=.3).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Ceil_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Round(PyoObject):
    """
    Rounds to the nearest integer value in a floating-point format.

    For each samples in the input signal, rounds to the nearest integer
    value of the sample value.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Clipping frequencies
    >>> sweep = Phasor(freq=[1,.67], mul=4)
    >>> flo = Round(sweep, mul=50, add=200)
    >>> a = SineLoop(freq=flo, feedback=.1, mul=.3).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Round_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Tanh(PyoObject):
    """
    Performs a hyperbolic tangent function on audio signal.

    Returns the hyperbolic tangent of audio signal as input.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, angle in radians.

    >>> s = Server().boot()
    >>> s.start()
    >>> import math
    >>> a = Phasor(250, mul=math.pi*2)
    >>> b = Tanh(Sin(a, mul=10), mul=0.3).mix(2).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Tanh_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Exp(PyoObject):
    """
    Calculates the value of e to the power of x.

    Returns the value of e to the power of x, where e is the base of the
    natural logarithm, 2.718281828...

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, the exponent.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine(freq=200)
    >>> lf = Sine(freq=.5, mul=5, add=6)
    >>> # Tanh style distortion
    >>> t = Exp(2 * a * lf)
    >>> th = (t - 1) / (t + 1)
    >>> out = (th * 0.3).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Exp_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Div(PyoObject):
    """
    Divides a by b.

    :Parent: :py:class:`PyoObject`

    :Args:

        a: float or PyoObject, optional
            Numerator. Defaults to 1.
        b: float or PyoObject, optional
            Denominator. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine(freq=[400, 500])
    >>> b = Randi(min=1, max=10, freq=5.00)
    >>> c = Div(a, b, mul=0.3).out()

    """

    def __init__(self, a=1, b=1, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", a, b, mul, add)
        PyoObject.__init__(self, mul, add)
        self._a = a
        self._b = b
        a, b, mul, add, lmax = convertArgsToLists(a, b, mul, add)
        self._base_objs = [M_Div_base(wrap(a, i), wrap(b, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setA(self, x):
        """
        Replace the `a` attribute.

        :Args:

            x: float or PyoObject
                new `a` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._a = x
        x, lmax = convertArgsToLists(x)
        [obj.setA(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setB(self, x):
        """
        Replace the `b` attribute.

        :Args:

            x: float or PyoObject
                new `b` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._b = x
        x, lmax = convertArgsToLists(x)
        [obj.setB(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def a(self):
        """float or PyoObject. Numerator."""
        return self._a

    @a.setter
    def a(self, x):
        self.setA(x)

    @property
    def b(self):
        """float or PyoObject. Denominator."""
        return self._b

    @b.setter
    def b(self, x):
        self.setB(x)


class Sub(PyoObject):
    """
    Substracts b from a.

    :Parent: :py:class:`PyoObject`

    :Args:

        a: float or PyoObject, optional
            Left operand. Defaults to 1.
        b: float or PyoObject, optional
            Right operand. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sig([400, 500])
    >>> b = Randi(min=50, max=100, freq=5.00)
    >>> c = Sub(a, b)
    >>> d = SineLoop(freq=c, feedback=0.08, mul=0.3).out()

    """

    def __init__(self, a=1, b=1, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", a, b, mul, add)
        PyoObject.__init__(self, mul, add)
        self._a = a
        self._b = b
        a, b, mul, add, lmax = convertArgsToLists(a, b, mul, add)
        self._base_objs = [M_Sub_base(wrap(a, i), wrap(b, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setA(self, x):
        """
        Replace the `a` attribute.

        :Args:

            x: float or PyoObject
                new `a` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._a = x
        x, lmax = convertArgsToLists(x)
        [obj.setA(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setB(self, x):
        """
        Replace the `b` attribute.

        :Args:

            x: float or PyoObject
                new `b` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._b = x
        x, lmax = convertArgsToLists(x)
        [obj.setB(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def a(self):
        """float or PyoObject. Left operand."""
        return self._a

    @a.setter
    def a(self, x):
        self.setA(x)

    @property
    def b(self):
        """float or PyoObject. Right operand."""
        return self._b

    @b.setter
    def b(self, x):
        self.setB(x)
