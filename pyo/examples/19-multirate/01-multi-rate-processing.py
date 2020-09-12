"""
01-multi-rate-processing.py - Doing processing at very high sampling rate.

In numerical audio computing, it is sometimes useful to be able to process
a signal with much more timing precision than what the usual sampling rates
offer. A typical case is when the effect applied to the sound adds a lot of 
harmonics. Higher harmonics will quickly wrap around the Nyquist frequency,
producing aliasing in the output signal. The solution is to increase the
sampling rate, so the nyquist frequency, and to use anti-aliasing filters
when converting from one rate to another.

Pyo allows to compute chunks of code at different sampling rates than the
one with which the server was started. You should do this only for the objects
you need to process with a higher sampling rate, without changing the server's
sampling rate, otherwise the program will be very CPU consuming.

You start a new resampling block with the method:

    Server.beginResamplingBlock(x)

where `x`, a power-of-two, is the resampling factor. A negative power-of-two
will start a downsampling block of code.

To close the block, simply call:

    Server.endResamplingBlock()

Everything between the two calls will be computed with the new sampling rate.

Audio signals must be resampled before used with a different sampling rate.
The Resample object does this. Inside a resampling block, it will convert the
signal to the new sampling rate, and outside the resampling block, it will
convert the signal back to the original sampling rate. Its `mode` argument lets
choose the quality of the interpolation/decimation filter used to resample the
signal.

"""
from pyo import *

# We create a new class for our upsampled distortion effect.
class UpSampDisto:
    """
    Upsampled distortion effect.

    :Args:

        input: PyoObject
            The signal to process.
        drive: float or PyoObject, optional
            Amount of distortion applied to the signal, between 0 and 1.
        upfactor: int (power-of-two), optional
            Resampling factor.
        filtmode: int, optional
            The interpolation/decimation mode. See Resample's man page
            for details.
        
    """

    def __init__(self, input, drive=0.5, upfactor=8, filtmode=32):
        # The InputFader object lets change its input signal without clicks.
        self.input = InputFader(input)

        # Convert the drive argument to audio signal.
        self.drive = Sig(drive)

        # Get a reference to the audio server.
        server = self.drive.getServer()

        # Start an upsampled block of code.
        server.beginResamplingBlock(upfactor)

        # Resample the audio signals. Because the drive signal is only a
        # control signal, a linear interpolation is enough. The input
        # signal uses a much better filter to eliminate aliasing artifacts.
        self.inputUp = Resample(self.input, mode=filtmode)
        self.driveUp = Resample(self.drive, mode=1)

        # Apply the distortion effect.
        self.disto = Disto(self.inputUp, drive=self.driveUp)

        # Close the upsampled block.
        server.endResamplingBlock()

        # Convert back the distorted signal to the current sampling rate.
        # Again, we use a good decimination filter to eliminate aliasing.
        self.output = Resample(self.disto, mode=filtmode, mul=0.5)

    # Define some useful methods.
    def setInput(self, x, fadetime=0.05):
        self.input.setInput(x, fadetime)

    def setDrive(self, x):
        self.drive.value = x

    def out(self, chnl=0):
        self.output.out(chnl)
        return self

    def sig(self):
        return self.output


### Usage example ###
s = Server().boot()

# Two different sources for testing, a sine wave and a flute melody.
src1 = Sine(freq=722, mul=0.7)
src1.ctrl([SLMapFreq(722)], title="Sine frequency")
src2 = SfPlayer("../snds/flute.aif", loop=True)
# Input source interpolation.
src = Interp(src1, src2, 0)
src.ctrl([SLMap(0, 1, "lin", "interp", 0)], title="Source: sine <=> flute")

# Control for the drive parameter of the distortion.
drv = Sig(0)
drv.ctrl(title="Drive")

# Distortion at current sampling rate.
dist = Disto(src, drive=drv, mul=0.5)

# Distortion with increased sampling rate.
updist = UpSampDisto(src, drv)

# Interpolator to compare the two processes.
output = Interp(dist, updist.sig(), 0, mul=0.5).out()
output.ctrl([SLMap(0, 1, "lin", "interp", 0)], title="Up Sampling: without <=> with")

sp = Spectrum(output)

s.gui(locals())
