"""
Prefix expression evaluators.

API documentation
=================

* This API is in alpha stage and subject to future changes!

Builtin functions
-----------------

Arithmetic operators:

(+ x y) : returns the sum of two values.
(- x y) : substracts the second value to the first and returns the result.
(* x y) : returns the multiplication of two values.
(/ x y) : returns the quotient of x/y.
(^ x y) : returns x to the power y.
(% x y) : returns the floating-point remainder of x/y.
(neg x) : returns the negative of x.

Moving phase operators:

(++ x y) : increments its internal state by x and wrap around 0.0 and y.
(-- x y) : decrements its internal state by x and wrap around 0.0 and y.
(~ x y) : generates a periodic ramp from 0 to 1 with frequency x and phase y.

Conditional operators:

(< x y) : returns 1 if x is less than y, otherwise returns 0.
(<= x y) : returns 1 if x is less than or equal to y, otherwise returns 0.
(> x y) : returns 1 if x is greater than y, otherwise returns 0.
(>= x y) : returns 1 if x is greater than or equal to y, otherwise returns 0.
(== x y) : returns 1 if x is equal to y, otherwise returns 0.
(!= x y) : returns 1 if x is not equal to y, otherwise returns 0.
(if (cond) (then) (else)) : returns then for any non-zero value of cond, otherwise returns else.
(and x y) : returns 1 if both x and y are not 0, otherwise returns 0.
(or x y) : returns 1 if one of x or y are not 0, otherwise returns 0.

Trigonometric functions:

(sin x) : returns the sine of an angle of x radians.
(cos x) : returns the cosine of an angle of x radians.
(tan x) : returns the tangent of x radians.
(tanh x) : returns the hyperbolic tangent of x radians.
(atan x) : returns the principal value of the arc tangent of x, expressed in radians.
(atan2 x y) : returns the principal value of the arc tangent of y/x, expressed in radians.

Power and logarithmic functions:

(sqrt x) : returns the square root of x.
(log x) : returns the natural logarithm of x.
(log2 x) : returns the binary (base-2) logarithm of x.
(log10 x) : returns the common (base-10) logarithm of x.
(pow x y) : returns x to the power y.

Clipping functions:

(abs x) : returns the absolute value of x.
(floor x) : rounds x downward, returning the largest integral value that is not greater than x.
(ceil x) : rounds x upward, returning the smallest integral value that is not less than x.
(exp  x) : returns the constant e to the power x.
(round x) : returns the integral value that is nearest to x.
(min x y) : returns the smaller of its arguments: either x or y.
(max x y) : returns the larger of its arguments: either x or y.
(wrap x) : wraps x between 0 and 1.

Random fuctions:

(randf x y) : returns a pseudo-random floating-point number in the range between x and y.
(randi x y) : returns a pseudo-random integral number in the range between x and y.

Complex numbers:

(complex x y) : returns a complex number where x is the real part and y the imaginary part.
(real x) : returns the real part of the complex number x.
(imag x) : returns the imaginary part of the complex number x.

Filter functions:

(delay x) : one sample delay.
(sah x y) : samples and holds x value whenever y is smaller than its previous state.
(rpole x y) : real one-pole recursive filter. returns x + last_out * y.
(rzero x y) : real one-zero non-recursive filter. returns x - last_x * y.
(cpole x y) : complex one-pole recursive filter. x is the complex signal to filter, y is a complex coefficient, it returns a complex signal.
(czero x y) : complex one-zero non-recursive filter. x is the complex signal to filter, y is a complex coefficient, it returns a complex signal.

Multi-output function:

(out x y) : If using multiple outputs, creates an audio stream where x is the output channel and y the signal to write to the channel.

Constants:

(const x) : returns x.
pi : returns an approximated value of pi.
twopi : returns a constant with value pi*2.
e : returns an approximated value of e.
sr : returns the current sampling rate.

Comments
--------

A comment starts with two slashs ( // ) and ends at the end of the line:

    // This is a comment!

Input and Output signals
------------------------

User has access to the last buffer size of input and output samples.

To use samples from past inputs, use $x0[n] notation, where 0 is the first
audio stream ($x1[0] would retrieve the second audio stream) and n is the
position from the current time. $x0[0] is the current input sample, $x0[-1]
is the previous one and $x0[-buffersize] is the last available input sample.

The first input audio stream can also simply be named $x[0] ($x[0] is the
same as $x0[0]).

To use samples from past output, use $y0[n] notation, where 0 is the first
output audio stream ($y1[0] would retrieve the second output audio stream)
and n is the position from the current time. $y0[-1] is the previous output
and $y0[-buffersize] is the last available output sample of the stream.

The first output audio stream can also simply be named $y[-1] ($y[-1] is the
same as $y0[-1]).

If the object generates only one channel output (the default), the last
expression in the script is the output signal. Otherwise, output signals must
be created with the out function.

Here an example of a first-order IIR lowpass filter expression:

    // A first-order IIR lowpass filter
    + $x[0] (* (- $y[-1] $x[0]) 0.99)

A simple ring-modulation expression:

    // ring-modulation between the first two input signals
    * $x[0] $x1[0]

Two output channels:

    // First channel
    (out 0 (sin (* (twopi) (~ 400))))
    // Second channel
    (out 1 (sin (* (twopi) (~ 500))))

Defining custom functions
-------------------------

The define keyword starts the definition of a custom function.

(define funcname (body))

funcname is the name used to call the function in the expression and
body is the sequence of functions to execute. Arguments of the function
are extracted directly from the body. They must be named $1, $2, $3, ..., $9.

Example of a sine wave function:

    (define osc (
        sin (* (twopi) (~ $1))
        )
    )
    // play a sine wave
    * (osc 440) 0.3

State variables
---------------

User can create state variable with the keyword "let". This is useful
to set an intermediate state to be used in multiple places in the
processing chain. The syntax is:

(let #var (body))

The variable name must begin with a "#".

    (let #sr 44100)
    (let #freq 1000)
    (let #coeff (
        ^ (e) (/ (* (* -2 (pi)) #freq) #sr)
        )
    )
    + $x[0] (* (- $y[-1] $x[0]) #coeff)

The variable is private to a function if created inside a custom function.

    (let #freq 250) // global #freq variable
    (define osc (
        (let #freq (* $1 $2)) // local #freq variable
        sin (* (twopi) (~ #freq))
        )
    )
    * (+ (osc 1 #freq) (osc 2 #freq)) 0.2

State variables can be used to do 1 sample feedback if used before created.
Undefined variables are initialized to 0.

    (define oscloop (
            (let #xsin
                (sin (+ (* (~ $1) (twopi)) (* #xsin $2))) // #xsin used before...
            ) // ... "let" statement finished!
            #xsin // oscloop function outputs #xsin variable
        )
    )
    * (oscloop 200 0.7) 0.3

A state variable can also contain a complex number:

    (let #v (complex 0.2 0.7)) // #v = (0.2 0.7)

User variables
--------------

User variables are created with the keyword "var".

(var #var (init))

The variable name must begin with a "#".

They are computed only at initialization, but can be changed from the python
script with method calls (varname is a string and value is a float):

obj.setVar(varname, value)

The following example shows how to control the cutoff fequency of a lowpass
filter with a user variable:

    expression = '''
    (var #freq 1000)
    (let #coeff (
        ^ e (/ (* (neg twopi) #freq) 44100)
        )
    )
    + $x[0] (* (- $y[-1] $x[0]) #coeff)
    '''
    ex = Expr(Noise(0.5), expression).out()
    freq = Sine(0.25).range(250, 10000)

    def update():
        ex.setVar("#freq", freq.get())

    pat = Pattern(update, 0.02).play()

Library importation
-------------------

Custom functions can be defined in an external file and imported with the
"load" function:

(load path/to/the/file)

The content of the file will be inserted where the load function is called
and all functions defined inside the file will then be accessible. The path
can be absolute or relative to the current working directory.

Complex numbers
---------------

A complex number is created with the `complex` function:

(complex x y)

We can retrieve one part of a complex number with `real` and `imag` functions:

    // get the real part (x) of a number
    (real (complex x y))

If a complex number is used somewhere not waiting for a complex, real value
will be used.

If a real number is used somewhere waiting for a complex, the imaginary part
is set to 0.0.

Examples
--------

A first-order IIR lowpass filter:

    (var #sr 44100)
    (var #cutoff 1000)
    (let #coeff (exp (/ (* (* -2 (pi)) #cutoff) #sr)))
    + $x[0] (* (- $y[-1] $x[0]) #coeff)

A LFO'ed hyperbolic tangent distortion:

    // $1 = lfo frequency, $2 = lfo depth
    (define lfo (
            (+ (* (sin (* (twopi) (~ $1))) (- $2 1)) $2)
        )
    )
    tanh (* $x[0] (lfo .25 10))

A triangle waveform generator (use Sig(0) as input argument to bypass input):

    (var #freq 440)
    // $1 = oscillator frequency
    (define triangle (
            (let #ph (~ $1))
            (- (* (min #ph (- 1 #ph)) 4) 1)
        )
    )
    (triangle #freq)

"""

"""
Copyright 2015-19 Olivier Belanger

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

import os
from ._core import *
from ._maps import *
from ._widgets import createExprEditorWindow

class Expr(PyoObject):
    """
    Prefix audio expression evaluator.

    Expr implements a tiny functional programming language that can be
    used to write synthesis or signal processing algorithms.

    For documentation about the language, see the module's documentation.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject or list of PyoObjects
            Input signal(s) to process. If given a multi-stream PyoObject
            or a list of PyoObjects, each stream ca be retrieved with the
            syntax $x0[0], $x1[0], $x2[0], etc. $x[0] is the same as $x0[0].
        expr: str, optional
            Expression to evaluate as a string.
        outs: int, optional
            Number of output signals generated by the object. If 1 (the
            default), the last expression in the script is the output signal.
            Otherwise, output signals must be created with the `out` function.
            Available at initialization time only.
        initout: float, optional
            Value used to initialize the output buffers. Use with caution!
            Defaults to 0.0.

    >>> s = Server().boot()
    >>> s.start()
    >>> proc = '''
    >>> (var #boost 1)
    >>> (tanh (* $x[0] #boost))
    >>> '''
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> ex = Expr(sf, proc, mul=0.4).out()
    >>> lfo = Sine(freq=1).range(1, 20)
    >>> def change():
    ...     ex.setVar("#boost", lfo.get())
    >>> pat = Pattern(change, 0.02).play()

    """

    def __init__(self, input, expr="", outs=1, initout=0.0, mul=1, add=0):
        pyoArgsAssert(self, "osIFOO", input, expr, outs, initout, mul, add)
        PyoObject.__init__(self, mul, add)
        self._editor = None
        self._input = input
        if isinstance(input, list):
            input_objs = [obj for pyoObj in input for obj in pyoObj.getBaseObjects()]
        else:
            input_objs = input.getBaseObjects()
        self._expr = expr
        self._outs = outs
        self._initout = initout
        expr = self._preproc(expr)
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_players = [Exprer_base(input_objs, expr, outs, initout)]
        self._base_objs = []
        for j in range(outs):
            self._base_objs.append(Expr_base(self._base_players[0], j, wrap(mul, j), wrap(add, j)))
        self._init_play()

    def setExpr(self, x):
        """
        Replace the `expr` attribute.

        :Args:

            x: string
                New expression to process.

        """
        pyoArgsAssert(self, "s", x)
        self._expr = x
        if self._editor is not None:
            self._editor.update(x)
        x = self._preproc(x)
        x, lmax = convertArgsToLists(x)
        [obj.setExpr(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def printNodes(self):
        """
        Print the list of current nodes.

        """
        [obj.printNodes() for i, obj in enumerate(self._base_objs)]

    def setVar(self, varname, value):
        pyoArgsAssert(self, "sn", varname, value)
        varname, value, lmax = convertArgsToLists(varname, value)
        [obj.setVar(wrap(varname, j), wrap(value, j)) for i, obj in enumerate(self._base_players) for j in range(lmax)]

    def _get_matching_bracket_pos(self, x, p1):
        count = 1
        p2 = p1 + 1
        while True:
            if x[p2] == "(":
                count += 1
            elif x[p2] == ")":
                count -= 1
            if count == 0:
                break
            p2 += 1
            if p2 == len(x):
                break
        if count != 0:
            return -1
        else:
            return p2

    def _replace(self, x, lst):
        lst.reverse()
        for key, body in lst:
            # find how many args waiting in function's body
            numargs = 0
            doll = body.find("$")
            while doll != -1:
                arg = int(body[doll + 1])
                if arg > numargs:
                    numargs = arg
                doll = body.find("$", doll + 1)
            occurences = 0
            pos = x.find(key)
            while pos != -1:
                if x[pos - 1] in " \t\n()" and x[pos + len(key)] in " \t\n()":
                    # replace "#vars" with unique symbol
                    body2 = body.replace("-%s" % key, ".%s.%d" % (key, occurences))
                    occurences += 1
                    # find limits
                    is_inside_brackets = True
                    start = pos - 1
                    while x[start] != "(":
                        start -= 1
                        if start < 0:
                            start = 0
                            is_inside_brackets = False
                            break
                    if is_inside_brackets:
                        end = self._get_matching_bracket_pos(x, start)
                    else:
                        end = len(x)
                    # retrieve args #
                    # function arguments are computed only once at the beginning of the
                    # function. This fixes a bug wen an argument *must* be of the same
                    # value for every $x and the arg expression contains random function.
                    argvars = ""
                    howmany = 0
                    p1 = pos + len(key)
                    p2 = -1
                    for i in range(numargs):
                        while x[p1] in " \t\n":
                            p1 += 1
                        if x[p1] == "(":
                            p2 = self._get_matching_bracket_pos(x, p1)
                            if p2 == -1 or p2 >= end:
                                raise Exception("Mismatched brackets in function arguments.")
                            p2 += 1
                        else:
                            p2 = p1 + 1
                            if p2 < end:
                                while x[p2] not in " \t\n()":
                                    p2 += 1
                                    if p2 == end:
                                        break
                        if x[p1:p2] != ")":
                            # pattern for an arg is #FUNCNAMEargARGNUMBER.
                            argvars = argvars + "(let #%sarg%d %s)\n" % (key, i + 1, x[p1:p2])
                            howmany += 1
                        if p2 == end:
                            break
                        else:
                            p1 = p2
                    if howmany > 0:
                        body2 = body2[0] + "\n" + argvars + body2[1:]

                    # discard extra args
                    if p2 != end and p2 != -1:
                        x = x[:p2] + x[end:]

                    # replace args
                    if howmany > 0:
                        newbody = body2
                        for i in range(numargs):
                            if i < howmany:
                                arg = "#%sarg%d" % (key, (i + 1))
                            else:
                                arg = "0.0"
                            newbody = newbody.replace("$%d" % (i + 1), arg)
                        x = x[:pos] + newbody + x[p2:]
                    else:
                        x = x[:pos] + body2 + x[pos + len(key) :]
                pos = x.find(key, pos + 1)
        return x

    def _change_var_names(self, funcname, funcbody):
        d = {}
        letpos = funcbody.find("let ")
        while letpos != -1:
            pos = funcbody.find("#", letpos)
            if pos == -1:
                raise Exception("No #var defined inside a let function.")
            p1 = pos
            p2 = p1 + 1
            while funcbody[p2] not in " \t\n()":
                p2 += 1
            label = funcbody[p1:p2]
            d[label] = label + "-" + funcname
            letpos = funcbody.find("let ", letpos + 4)
        for label, newlabel in d.items():
            funcbody = funcbody.replace(label, newlabel)
        return funcbody

    def _preproc(self, x):
        # replace load functions with file body
        while "(load" in x:
            p1 = x.find("(load")
            p2 = self._get_matching_bracket_pos(x, p1)
            p2 += 1
            text = x[p1:p2]
            path = text.replace("(load", "").replace(")", "").strip()
            if os.path.isfile(path):
                with open(path, "r") as f:
                    text = f.read()
            x = x[:p1] + text + x[p2:]

        # remove comments
        while "//" in x:
            start = x.find("//")
            end = x.find("\n", start)
            x = x[:start] + x[end:]

        # make sure there are braces around constant keywords
        constants = ["pi", "twopi", "e", "sr"]
        for constant in constants:
            p1 = x.find(constant)
            while p1 != -1:
                if x[p1 - 1] == " ":
                    x = x[:p1] + "(" + constant + ")" + x[p1 + len(constant) :]
                p1 = x.find(constant, p1 + len(constant) + 2)

        # expand defined functions
        _defined = []
        while "define" in x:
            start = x.find("(define")
            p1 = start + 7
            # get function name
            while x[p1] in " \t\n":
                p1 += 1
            p2 = p1 + 1
            while x[p2] not in " \t\n":
                p2 += 1
            funcname = x[p1:p2]
            # get function body
            p1 = p2 + 1
            while x[p1] != "(":
                p1 += 1
            p2 = self._get_matching_bracket_pos(x, p1)
            if p2 == -1:
                raise Exception("Mismatched brackets in function body.")
            p2 += 1
            funcbody = x[p1:p2]
            # get end of the definition
            while x[p2] in " \t\n":
                p2 += 1
            if x[p2] != ")":
                raise Exception("Missing ending bracket in function definition.")
            stop = p2
            # save in dictionary and remove definition from the string
            funcbody = self._change_var_names(funcname, funcbody)
            _defined.append([funcname, funcbody])
            x = x[:start] + x[stop + 1 :]
        # replace calls to function with their function body
        x = self._replace(x, _defined)
        x = x.strip()
        return x

    def editor(self, title="Expr Editor", wxnoserver=False):
        """
        Opens the text editor for this object.

        :Args:

            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createExprEditorWindow(self, title, wxnoserver)

    @property
    def expr(self):
        """string. New expression to process."""
        return self._expr

    @expr.setter
    def expr(self, x):
        self.setExpr(x)
