"""
Hand-written Vocoder class.

This is an example on how to write a class acting the same way
as any other builtin PyoObjects.

"""
from pyo import *

class MyVocoder(PyoObject):
    """
    Vocoder effect.

    A vocoder is an analysis/synthesis system. In the encoder, the input is passed
    through a multiband filter, each band is passed through an envelope follower,
    and the control signals from the envelope followers are communicated to the
    decoder. The decoder applies these (amplitude) control signals to corresponding
    filters in the (re)synthesizer.


    Parent class: PyoObject

    Parameters:

    in1 : PyoObject
        Audio source generating the spectral envelope.
    in2 : PyoObject
        Audio source exciting the bank of filters.
    base : float or PyoObject, optional
        Base frequency used to compute filter notch frequencies.
        Defaults to 50.
    spread : float or PyoObject, optional
        Spreading of the filter notch frequencies. Defaults to 1.5.
    q : float or PyoObject, optional
        Q (inverse of the bandwidth) of the filters. Defaults to 5.
    num : int, optional
        Number of bands (filter notches) of the vocoder. Available only
        at initialization. Defaults to 20.

    Methods:

    setIn1(x) : Replace the `in1` attribute.
    setIn2(x) : Replace the `in2` attribute.
    setBase(x) : Replace the `base` attribute.
    setSpread(x) : Replace the `spread` attribute.
    setQ(x) : Replace the `q` attribute.

    Attributes:

    in1 : PyoObject. Audio source generating the spectral envelope.
    in2 : PyoObject. Audio source exciting the bank of filters.
    base : float or PyoObject, Base frequency.
    spread : float or PyoObject, Spreading of the filter notch frequencies.
    q : float or PyoObject, Q of the filters.

    See also: BandSplit, Phaser

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> b = PinkNoise(.1)
    >>> lfo = Sine(freq=.05, mul=50, add=100)
    >>> voc = MyVocoder(in1=a, in2=b, num=12, base=lfo, spread=[1.2,1.22]).out()

    """
    def __init__(self, in1, in2, base=50, spread=1.5, q=5, num=20, mul=1, add=0):
        PyoObject.__init__(self)
        # keep references of all raw arguments
        self._in1 = in1
        self._in2 = in2
        self._base = base
        self._spread = spread
        self._q = q
        self._num = num
        self._mul = mul
        self._add = add

        # list of filter's notch frequencies
        self._partials = [i+1 for i in range(self._num)]

        # Using InputFader for sound input allows crossfades when changing sources
        self._in1_fader = InputFader(in1)
        self._in2_fader = InputFader(in2)

        # Convert all arguments to lists for "list expansion"
        # convertArgsToLists function returns variables in argument as lists + maximum list size
        in1_fader, in2_fader, base, spread, q, mul, add, lmax = convertArgsToLists(self._in1_fader,
                                                            self._in2_fader, base, spread, q, mul, add)

        # Init some lists to keep track of created objects
        self._pows = []
        self._bases = []
        self._freqs = []
        self._srcs = []
        self._amps = []
        self._excs = []
        self._outs = []

        # self._base_objs is the audio output seen by the outside world!
        # .play(), .out(), .stop() and .mix() methods act on this list
        # "mul" and "add" attributes are also applied on this list's objects
        self._base_objs = []

        # Each cycle of the loop creates a mono stream of sound
        for i in range(lmax):
            self._pows.append(Pow(self._partials, wrap(spread,i)))
            self._bases.append(Sig(wrap(base,i)))
            self._freqs.append(Clip(self._pows[-1] * self._bases[-1], 20, 20000))
            self._srcs.append(Biquadx(wrap(in1_fader,i), freq=self._freqs[-1], q=wrap(q,i), type=2, stages=2))
            self._amps.append(Follower(self._srcs[-1], freq=20, mul=wrap(q,i)*10))
            self._excs.append(Biquadx(wrap(in2_fader,i), freq=self._freqs[-1], q=wrap(q,i), type=2, stages=2, mul=self._amps[-1]))
            # Here we mix in mono all sub streams created by "num" bands of vocoder
            self._outs.append(Mix(input=self._excs[-1], voices=1, mul=wrap(mul,i), add=wrap(add,i)))
            # getBaseObjects() method returns the list of Object_Base, needed in the self._base_objs list
            self._base_objs.extend(self._outs[-1].getBaseObjects())

    def __dir__(self):
        return ["in1", "in2", "base", "spread", "q", "mul", "add"]

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._pows)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._bases)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._freqs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._srcs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._amps)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._excs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._outs)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def stop(self, wait=0):
        [obj.stop(wait) for obj in self._pows]
        [obj.stop(wait) for obj in self._bases]
        [obj.stop(wait) for obj in self._freqs]
        [obj.stop(wait) for obj in self._srcs]
        [obj.stop(wait) for obj in self._amps]
        [obj.stop(wait) for obj in self._excs]
        [obj.stop(wait) for obj in self._outs]
        [obj.stop(wait) for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._pows)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._bases)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._freqs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._srcs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._amps)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._excs)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._outs)]
        if type(chnl) == list:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i))
                    for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def setIn1(self, x, fadetime=0.05):
        """
        Replace the `in1` attribute.

        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._in1 = x
        self._in1_fader.setInput(x, fadetime)

    def setIn2(self, x, fadetime=0.05):
        """
        Replace the `in2` attribute.

        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._in2 = x
        self._in2_fader.setInput(x, fadetime)

    def setBase(self, x):
        """
        Replace the `base` attribute.

        Parameters:

        x : float or PyoObject
            New `base` attribute.

        """
        self._base = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._bases)]

    def setSpread(self, x):
        """
        Replace the `spread` attribute.

        Parameters:

        x : float or PyoObject
            New `spread` attribute.

        """
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setExponent(wrap(x,i)) for i, obj in enumerate(self._pows)]

    def setQ(self, x):
        """
        Replace the `q` attribute.

        Parameters:

        x : float or PyoObject
            New `q` attribute.

        """
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setMul(wrap(x,i)*30) for i, obj in enumerate(self._amps)]
        [obj.setQ(wrap(x,i)) for i, obj in enumerate(self._srcs)]
        [obj.setQ(wrap(x,i)) for i, obj in enumerate(self._excs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        # Define the object's default map_list used if None is passed to PyoObject
        # map_list is a list of SLMap objects defined for each available attribute
        # in the controller window
        self._map_list = [SLMap(20., 250., "lin", "base", self._base),
                          SLMap(0.5, 2., "lin", "spread", self._spread),
                          SLMap(1., 50., "log", "q", self._q),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def in1(self): return self._in1
    @in1.setter
    def in1(self, x): self.setIn1(x)

    @property
    def in2(self): return self._in2
    @in2.setter
    def in2(self, x): self.setIn2(x)

    @property
    def base(self): return self._base
    @base.setter
    def base(self, x): self.setBase(x)

    @property
    def spread(self): return self._spread
    @spread.setter
    def spread(self, x): self.setSpread(x)

    @property
    def q(self): return self._q
    @q.setter
    def q(self, x): self.setQ(x)
