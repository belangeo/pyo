Getting Started
=================================

Here is quick introduction to Pyo. It assumes you already know Python and basics about Object-oriented programming.

The Pyo Server and GUI
-----------------------

The first thing you need to do to use Pyo is import the pyo python module and boot the server. This audio server will open audio and midi interfaces and will be ready to send to them the audio and MIDI produced by other pyo objects. You then need to make some sound:

>>> from pyo import *
>>> s = Server().boot()
>>> s.start()
>>> a = Sine(mul=0.01).out()

The `s` variable holds the Server instance, which has been booted, using the boot function. Booting the server includes opening audio and MIDI interfaces, and setting up the sample rate and number of channels, but the server will not be processing audio until its start() method is called. Then we create a Sine object, and store it in variable a, after calling its out method. The Sine class defines a Sine wave oscillator. The out method from this class connects the output of the oscillator to the server audio outputs. I have set the mul attribute of the Sine object to make sure you don't blow your ears when you play this, as the default amplitude multiplier is 1, i.e. a sine wave at the maximum amplitude before clipping! (But I'll talk about attributes later...) You can stop the server with:

>>> s.stop()

To interact or not to interact
--------------------------------

If you tried the above script from an interactive python shell you would have heard a sine tone, but if you ran it from a python script non-interactively, you are probably asking yourself why you haven't heard anything. The reason is that the script has finished before the server has sent any audio to the outputs! So if you are using python non-interactively, the way to hear this example is:

.. code-block:: python

    from pyo import *
    s = Server().boot()
    s.start()
    a = Sine(mul=0.01).out()
    s.gui(locals())

In the last line, you can see a very handy method from the Server class, which creates a small control GUI for the current instance. The gui method for the Server object, keeps a script running and allows you to start and stop the server, control the output volume and record to an audio file the sound generated in the server. A handy feature of the server GUI is the interpreter text box in the bottom. From it you can send commands interactively to the interpreter, to start and stop objects, create or destroy them, etc.

Changing Object Characteristics
----------------------------------

The Sine class constructor is defined as:

.. code-block:: python

    Sine(self, freq=1000, phase=0, mul=1, add=0)

So you can give it a frequency, starting phase, multiplier and DC offset value when you create it. Also, if you want to do without the server gui, you can use the server method start() from your script, but you might need to use the sleep function from the time module to have your script run the server for a while if you are running Python non-interactively:

.. code-block:: python

    from pyo import *
    import time
    s = Server().boot()
    a = Sine(440, 0, 0.1).out()
    s.start()
    time.sleep(1)
    s.stop()

Notice that you can set the parameters for Sine in the order in which they are defined, but you can also give the parameters a name if you want to leave the rest at their default:

.. code-block:: python

    a = Sine(mul=0.1).out()

Once the object has been created, you can modify its attributes using the access methods. For example, to modify the frequency of the a oscillator object after it has been created you can use:

.. code-block:: python

    a.setFreq(1000)

But you can also set the attributes directly:

.. code-block:: python

    a.freq = 1000

Chaining objects
-----------------

Oscillators like the Sine class can be used as inputs to other classes, for example for frequency modulation:

.. code-block:: python

    from pyo import *
    s = Server().boot()
    mod = Sine(freq=6, mul=50)
    a = Sine(freq=mod + 440, mul=0.1).out()
    s.gui(locals())

You can create an envelope for a sine wave like this:

.. code-block:: python

    from pyo import *
    s = Server().boot()
    f = Adsr(attack=.01, decay=.2, sustain=.5, release=.1, dur=5, mul=.5)
    a = Sine(mul=f).out()
    f.play()
    s.gui(locals())

Class examples
----------------

All Classes in Pyo come with an example which shows how it can be used. To execute the example you can do:

>>> from pyo import *
>>> example(Harmonizer)

This will show and execute the example for the Harmonizer class. 

