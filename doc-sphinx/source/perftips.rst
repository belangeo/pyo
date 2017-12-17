How to improve performance of your pyo programs
===============================================

This document lists various tips that help to improve the performance of your
pyo programs.

Python tips
-----------

There is not much you can do at the Python level because once the script has
finished its execution run, almost all computations are done in the C level
of pyo. Nevertheless, there is these two tricks to consider:

Adjust the interpreter's "check interval"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can change how often the interpreter checks for periodic things with
`sys.setcheckinterval(interval)`. The defaults is 100, which means the check
is performed every 100 Python virtual instructions. Setting it to a larger
value may increase performance for programs using threads.

Use the subprocess or multiprocessing modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use the subprocess or multiprocessing modules to spawn your processes
on multiple processors. From the python docs:

  The multiprocessing package offers both local and remote concurrency,
  effectively side-stepping the Global Interpreter Lock by using subprocesses
  instead of threads. Due to this, the multiprocessing module allows the
  programmer to fully leverage multiple processors on a given machine.
  It runs on both Unix and Windows.

Here is a little example of using the multiprocessing module to spawn a lot of
sine wave computations to multiple processors.

.. code-block:: python

    #!/usr/bin/env python
    # encoding: utf-8
    """
    Spawning lot of sine waves to multiple processes.
    From the command line, run the script with -i flag.

    Call quit() to stop the workers and quit the program.

    """
    import time
    import multiprocessing
    from random import uniform
    from pyo import Server, SineLoop

    class Group(multiprocessing.Process):
        def __init__(self, num_of_sines):
            super(Group, self).__init__()
            self.daemon = True
            self._terminated = False
            self.num_of_sines = num_of_sines

        def run(self):
            # All code that should run on a separated
            # core must be created in the run() method.
            self.server = Server()
            self.server.deactivateMidi()
            self.server.boot().start()

            freqs = [uniform(400,800) for i in range(self.num_of_sines)]
            self.oscs = SineLoop(freq=freqs, feedback=0.1, mul=.005).out()

            # Keeps the process alive...
            while not self._terminated:
                time.sleep(0.001)

            self.server.stop()

        def stop(self):
            self._terminated = True

    if __name__ == '__main__':
        # Starts four processes playing 500 oscillators each.
        jobs = [Group(500) for i in range(4)]
        [job.start() for job in jobs]

        def quit():
            "Stops the workers and quit the program."
            [job.stop() for job in jobs]
            exit()

Avoid memory allocation after initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Dynamic memory allocation (malloc/calloc/realloc) tends to be
nondeterministic; the time taken to allocate memory may not be predictable,
making it inappropriate for real time systems. To be sure that the audio
callback will run smoothly all the time, it is better to create all audio
objects at the program's initialization and call their `stop()`, `play()`,
`out()` methods when needed.

Be aware that a simple arithmetic operation involving an audio object will
create a `Dummy` object (to hold the modified signal), thus will allocate
memory for its audio stream AND add a processing task on the CPU. Run this
simple example and watch the process's CPU growing:

.. code-block:: python

    from pyo import *
    import random

    s = Server().boot()

    env = Fader(0.005, 0.09, 0.1, mul=0.2)
    jit = Randi(min=1.0, max=1.02, freq=3)
    sig = RCOsc(freq=[100,100], mul=env).out()

    def change():
        freq = midiToHz(random.randrange(60, 72, 2))
        # Because `jit` is a PyoObject, both `freq+jit` and `freq-jit` will
        # create a `Dummy` object, for which a reference will be created and
        # saved in the `sig` object. The result is both memory and CPU
        # increase until something bad happens!
        sig.freq = [freq+jit, freq-jit]
        env.play()

    pat = Pattern(change, time=0.125).play()

    s.gui(locals())

An efficient version of this program should look like this:

.. code-block:: python

    from pyo import *
    import random

    s = Server().boot()

    env = Fader(0.005, 0.09, 0.1, mul=0.2)
    jit = Randi(min=1.0, max=1.02, freq=3)
    # Create a `Sig` object to hold the frequency value.
    frq = Sig(100)
    # Create the `Dummy` objects only once at initialization.
    sig = RCOsc(freq=[frq+jit, frq-jit], mul=env).out()

    def change():
        freq = midiToHz(random.randrange(60, 72, 2))
        # Only change the `value` attribute of the Sig object.
        frq.value = freq
        env.play()

    pat = Pattern(change, time=0.125).play()

    s.gui(locals())

Don't do anything that can trigger the garbage collector
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The garbage collector of python is another nondeterministic process. You
should avoid doing anything that can trigger it. So, instead of deleting
an audio object, which can turn out to delete many stream objects, you
should just call its `stop()` method to remove it from the server's
processing loop.

Pyo tips
--------

Here is a list of tips specific to pyo that you should consider when trying to
reduce the CPU consumption of your audio program.

Mix down before applying effects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is very easy to over-saturate the CPU with pyo, especially if you use the
multi-channel expansion feature. If your final output uses less channels than
the number of audio streams in an object, don't forget to mix it down (call
its `mix()` method) before applying effects on the sum of the signals.

Consider the following snippet, which create a chorus of 50 oscillators and
apply a phasing effect on the resulting sound:

.. code-block:: python

    src = SineLoop(freq=[random.uniform(190,210) for i in range(50)],
                   feedback=0.1, mul=0.01)
    lfo = Sine(.25).range(200, 400)
    phs = Phaser(src, freq=lfo, q=20, feedback=0.95).out()


This version uses around 47% of the CPU on my Thinkpad T430, i5 3320M @ 2.6GHz.
The problem is that the 50 oscillators given in input of the Phaser object
creates 50 identical Phaser objects, one for each oscillator. That is a big
waste of CPU. The next version mixes the oscillators into a stereo stream
before applying the effect and the CPU consumption drops to ~7% !

.. code-block:: python

    src = SineLoop(freq=[random.uniform(190,210) for i in range(50)],
                   feedback=0.1, mul=0.01)
    lfo = Sine(.25).range(200, 400)
    phs = Phaser(src.mix(2), freq=lfo, q=20, feedback=0.95).out()


When costly effects are involved, this can have a very drastic impact on the
CPU usage.

Stop your unused audio objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Whenever you don't use an audio object (but you want to keep it for future
uses), call its `stop()` method. This will inform the server to remove it from
the computation loop. Setting the volume to 0 does not save CPU (everything is
computed then multiplied by 0), the `stop()` method does. My own synth classes
often looks like something like this:

.. code-block:: python

    class Glitchy:
        def __init__(self):
            self.feed = Lorenz(0.002, 0.8, True, 0.49, 0.5)
            self.amp = Sine(0.2).range(0.01, 0.3)
            self.src = SineLoop(1, self.feed, mul=self.amp)
            self.filt = ButLP(self.src, 10000)

        def play(self, chnl=0):
            self.feed.play()
            self.amp.play()
            self.src.play()
            self.filt.out(chnl)
            return self

        def stop(self):
            self.feed.stop()
            self.amp.stop()
            self.src.stop()
            self.filt.stop()
            return self

Control attribute with numbers instead of PyoObjects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Objects internal processing functions are optimized when plain numbers are
given to their attributes. Unless you really need audio control over some
parameters, don't waste CPU cycles and give fixed numbers to every attribute
that don't need to change over time. See this comparison:

.. code-block:: python

    n = Noise(.2)

    # ~5% CPU
    p1 = Phaser(n, freq=[100,105], spread=1.2, q=10,
                feedback=0.9, num=48).out()

    # ~14% CPU
    p2 = Phaser(n, freq=[100,105], spread=Sig(1.2), q=10,
                feedback=0.9, num=48).out()

Making the `spread` attribute of `p2` an audio signal causes the frequency of
the 48 notches to be recalculated every sample, which can be a very costly
process.

Check for denormal numbers
^^^^^^^^^^^^^^^^^^^^^^^^^^

From wikipedia:

  In computer science, denormal numbers or denormalized numbers (now
  often called subnormal numbers) fill the underflow gap around zero in
  floating-point arithmetic. Any non-zero number with magnitude smaller
  than the smallest normal number is 'subnormal'.

The problem is that some processors compute denormal numbers very
slowly, which makes grow the CPU consumption very quickly. The solution is to
wrap the objects that are subject to denormals (any object with an internal
recursive delay line, ie. filters, delays, reverbs, harmonizers, etc.) in a
`Denorm` object. `Denorm` adds a little amount of noise, with a magnitude
just above the smallest normal number, to its input. Of course, you can use
the same noise for multiple denormalizations:

.. code-block:: python

    n = Noise(1e-24) # low-level noise for denormals

    src = SfPlayer(SNDS_PATH+"/transparent.aif")
    dly = Delay(src+n, delay=.1, feedback=0.8, mul=0.2).out()
    rev = WGVerb(src+n).out()

Use a PyoObject when available
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Always look first if a PyoObject does what you want, it will always be more
efficient than the same process written from scratch.

This construct, although pedagogically valid, will never be more efficient, in
term of CPU and memory usage, than a native PyoObject (Phaser) written in C.

.. code-block:: python

    a = BrownNoise(.02).mix(2).out()

    lfo = Sine(.25).range(.75, 1.25)
    filters = []
    for i in range(24):
        freq = rescale(i, xmin=0, xmax=24, ymin=100, ymax=10000)
        filter = Allpass2(a, freq=lfo*freq, bw=freq/2, mul=0.2).out()
        filters.append(filter)

It is also more efficient to use `Biquadx(stages=4)` than a cascade of four
`Biquad` objects with identical arguments.

Avoid trigonometric computation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Avoid trigonometric functions computed at audio rate (`Sin`, `Cos`, `Tan`,
`Atan2`, etc.), use simple approximations instead. For example, you can
replace a clean `Sin/Cos` panning function with a cheaper one based on `Sqrt`:

.. code-block:: python

    # Heavier
    pan = Linseg([(0,0), (2, 1)]).play()
    left = Cos(pan * math.pi * 0.5, mul=0.5)
    right = Sin(pan * math.pi * 0.5, mul=0.5)
    a = Noise([left, right]).out()

    # Cheaper
    pan2 = Linseg([(0,0), (2, 1)]).play()
    left2 = Sqrt(1 - pan2, mul=0.5)
    right2 = Sqrt(pan2, mul=0.5)
    a2 = Noise([left2, right2]).out()

Use approximations if absolute precision is not needed
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When absolute precision is not really important, you can save precious CPU
cycles by using approximations instead of the real function. `FastSine` is an
approximation of the `sin` function that can be almost twice cheaper than a
lookup table (Sine). I plan to add more approximations like this one in the
future.

Re-use your generators
^^^^^^^^^^^^^^^^^^^^^^

Some times it possible to use the same signal for parallel purposes. Let's
study the next process:

.. code-block:: python

    # single white noise
    noise = Noise()

    # denormal signal
    denorm = noise * 1e-24
    # little jitter around 1 used to modulate frequency
    jitter = noise * 0.0007 + 1.0
    # excitation signal of the waveguide
    source = noise * 0.7

    env = Fader(fadein=0.001, fadeout=0.01, dur=0.015).play()
    src = ButLP(source, freq=1000, mul=env)
    wg = Waveguide(src+denorm, freq=100*jitter, dur=30).out()

Here the same white noise is used for three purposes at the same time. First,
it is used to generate a denormal signal. Then, it is used to generate a
little jitter applied to the frequency of the waveguide (that adds a little
buzz to the string sound) and finally, we use it as the excitation of the
waveguide. This is surely cheaper than generating three different white noises
without noticeable difference in the sound.

Leave 'mul' and 'add' attributes to their defaults when possible
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is an internal condition that bypass the object "post-processing"
function when `mul=1` and `add=0`. It is a good practice to apply amplitude
control in one place instead of messing with the `mul` attribute of each
objects.

.. code-block:: python

    # wrong
    n = Noise(mul=0.7)
    bp1 = ButBP(n, freq=500, q=10, mul=0.5)
    bp2 = ButBP(n, freq=1500, q=10, mul=0.5)
    bp3 = ButBP(n, freq=2500, q=10, mul=0.5)
    rev = Freeverb(bp1+bp2+bp3, size=0.9, bal=0.3, mul=0.7).out()

    # good
    n = Noise(mul=0.25)
    bp1 = ButBP(n, freq=500, q=10)
    bp2 = ButBP(n, freq=1500, q=10)
    bp3 = ButBP(n, freq=2500, q=10)
    rev = Freeverb(bp1+bp2+bp3, size=0.9, bal=0.3).out()

Avoid graphical updates
^^^^^^^^^^^^^^^^^^^^^^^

Even if they run in different threads, with different priorities, the audio
callback and the graphical interface of a python program are parts of a unique
process, sharing the same CPU. Don't use the Server's GUI if you don't need to
see the meters or use the volume slider. Instead, you could start the script
from command line with `-i` flag to leave the interpreter alive.

.. code-block:: bash

    $ python -i myscript.py

List of CPU intensive objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here is a non-exhaustive list of the most CPU intensive objects of the library.

- Analysis
    - Yin
    - Centroid
    - Spectrum
    - Scope
- Arithmetic
    - Sin
    - Cos
    - Tan
    - Tanh
    - Atan2
- Dynamic
    - Compress
    - Gate
- Special Effects
    - Convolve
- Prefix Expression Evaluator
    - Expr
- Filters
    - Phaser
    - Vocoder
    - IRWinSinc
    - IRAverage
    - IRPulse
    - IRFM
- Fast Fourier Transform
    - CvlVerb
- Phase Vocoder
    - Almost every objects!
- Signal Generators
    - LFO
- Matrix Processing
    - MatrixMorph
- Table Processing
    - Granulator
    - Granule
    - Particule
    - OscBank
- Utilities
    - Resample
