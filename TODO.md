This is a list of features/fixes to implement for future releases
=================================================================

Roadmap 1.0.5
-------------

- python 3.10 and 3.11 wheels

Roadmap 1.0.6
-------------

- HiDPI on Windows:

• since I run Windows 11 in HighDPI resolution, the default GUI looks a bit pixelated
-- this could be fixed later on by using
if IS_WINDOWS:
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(True)
    except Exception as e:
        pass

in conjunction with self.FromDIP(wx.Point, wx.Size, ...) in _wxwidgets.py

ref: https://mail.google.com/mail/u/0/?zx=5375dhfmshql#label/PYO/KtbxLxghkxTqDXhnSCDJxLQkCDHGxpMslq

- Safer version of realloc

- Add Midifile object

- Add an optional callback on the "stop" event of the PyoObject. 

- Add "loop points" to Linseg, Expseg, and MidiLinseg.
  ref: https://groups.google.com/g/pyo-discuss/c/Y3Eu-O7lYRM/m/vLoflgWvAQAJ?utm_medium=email&utm_source=footer&pli=1

- Install m_pyo.h configured for the target version in pythonlib/pyo/include/.

- PyoMatrixObject.get(x, y) inverts arguments. Add PyoMatrixObject.getValue(x, y) with x and y in the good order (also putValue(value, x, y))
- PyoMatrixObject.getMatrix()
- A new implementation, Pyo2DTableObject, doing the right things.

Internal
--------

- Use inheritance (instead of a bunch of macros) at the C level. 
  PyoObject_base or Stream) should handle `mul`, `add` and 
  stream-related functions for every PyoObject. It should make the 
  code much simple and easier to maintain/extend, and also reduce 
  compiling time and binary size.

Server
------

- Updating coreaudio support (lot of deprecated functions).

- A method to retrieve a graph of the internal state of the server 
  (active objects, connections, attribute values, ...).

Examples
--------

- finish the examples.

- A section about building fx and synth classes.

Objects
-------

- SigTo with independent rising and falling times.

- PVInterp

- PVGain with controlable gain value over time.

- Integrator and Differentiator

- TrigAdsr, to generate envelope from trigger with constant fadein time.

- Add VBAP and LBAP spat algorithms.

- Add a plate reverb.

MIDI
----

- MidiFile & MidiFileReader

- sysex support in MidiListener.

GUI
---

- PyoGuiRangeSlider (instead of importing pyo.lib._widgets.HRangeSlider

- MixerGUI, an interface to control the routing of a Mixer object.

- A graphical representation of a parametric EQ.

- Analysis display (cartesian plane, sonnagram, filter frequency response)


Matrices
--------

- Implement the buffer protocol as in the PyoTableObject.

Link
----

- Added Link integration to the Event Framework.
