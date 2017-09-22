To do ASAP!
===========

- [DOC] Compilation for anaconda (mail from Stanley Rosenbaum).

- [DOC] Live recording without xruns, use Server.recstart() to record to a
  file on a tmpfs, which is bsically a filesystem in RAM.
  mail: "Server recording to wav generates many xruns (Aug 26)".

- Better error message when trying to open an input device but there is
  none on the system. Automatic deactivation of duplex mode if no input.

- E-Pyo: Reading preferences file should handle utf-8 paths.

- E-Pyo: style preference with unicode characters in the font name.

- Unicode paths don't work with python 3.6 on Windows.

- Jack midi.

    Server(sr=44100, nchnls=2, buffersize=256, duplex=1, audio="portaudio",
           jackname="pyo", ichnls=None, winhost="wasapi", midi="portmidi")
    Server.setJackMidiAutoConnectInputPorts(ports)
    Server.setJackMidiAutoConnectOutputPorts(ports)
    Server.setJackMidiInputPortNames(name)
    Server.setJackMidiOutputPortNames(name)


This is a list of features/fixes to implement for future releases
=================================================================

Internal
--------

- Replace malloc/realloc with a custom memory pool allocator.

- Use inheritance (instead of a bunch of macros) at the C level. 
  PyoObject_base or Stream) should handle `mul`, `add` and 
  stream-related functions for every PyoObject. It should make the 
  code much simple and easier to maintain/extend, and also reduce 
  compiling time and binary size.


Server
------

- The Server should perform a check, especially on Windows, to find the
  most suitable audio driver to use.

- A method to retrieve a graph of the internal state of the server 
  (active objects, connections, attribute values, ...).

- Remove, if possible, PyGILState_Ensure/PyGILState_Release from 
  the process_buffers function.

Examples
--------

- A section about building fx and synth classes.


Objects
-------

- TrigMap(inputs, values, init=0, mul=1, add=0)

  Where `inputs` are a list of trigger objects and `values` (list of floats) 
  the corresponding values to output depending which trigger has been detected.
  A trigger from the second object will make the object output the second value
  from the list.

MIDI
----

- Jack on windows.

- Create a MidiLinseg object that act like MidiAdsr but with a breakpoints
  function as the envelope. The sustain point should be settable by the user.

- sysex support in MidiListener.


GUI
---

- Implement all GUI components with Tkinter and make it the default GUI
  toolkit (instead of wxpython). WxPython classes could be removed from
  pyo sources and built as an optional extra package (pyo-wxgui). The idea
  is to remove an extra dependency, as tk is generally builtin with python.

- MixerGUI, an interface to control the routing of a Mixer object.

- Keyboard, a virtual MIDI keyboard (adapted from Zyne's one).

- Ability to set channel "name" in the view of PyoGuiScope and PyoGuiSpectrum.

Tables
------

- Objects that can write to tables should accept any PyoTableObject,
  not just a NewTable or a DataTable.

Matrices
--------

- Implement the buffer protocol as in the PyoTableObject.

E-Pyo
-----

- Split the file name (shown in the notebook tab) with the last dot, not the first.

- Complete review of E-Pyo on Windows. Lot of features don't seem to work.

    - Project tree refresh function doesn't work.

- Window splitter to show more than one file at the time (multiple 
  views) ?

- We need A way to let the user interact with the script via input() 
  and raw_input() functions.
