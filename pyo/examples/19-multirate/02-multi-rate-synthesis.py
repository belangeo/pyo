"""
01-multi-rate-synthesis.py - Doing synthesis at very high sampling rate.

In numerical audio computing, it is sometimes useful to be able to process
a signal with much more timing precision than what the usual sampling rates
offer. A typical case is when the synthesis algorithm generates aliasing in 
the output signal. The solution is to increase the sampling rate, so the 
nyquist frequency, and to use anti-aliasing filters when converting from one 
rate to another.

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

s = Server().boot()

# We create a new class for our upsampled Frequency modulation synthesis. We
# use only the modulation index as parameter in order to simplify the code.
class UpSampFM:
    """
    Frequency modulation synthesis that can be computed a higher sampling rate.

    :Args:

        fmindex: float or PyoObject, optional
            The modulation index of the FM synthesis.
        upfactor: int (power-of-two), optional
            Resampling factor.
        filtmode: int, optional
            The interpolation/decimation mode. See Resample's man page
            for details.
        
    """

    def __init__(self, fmindex=20, upfactor=8, filtmode=32):

        # Convert the modulation index argument to audio signal.
        self.fmindex = Sig(fmindex)

        # Get a reference to the audio server.
        server = self.fmindex.getServer()

        # Start an upsampled block of code.
        server.beginResamplingBlock(upfactor)

        # Resample the audio signals. Because the drive signal is only a
        # control signal, a linear interpolation is enough.
        self.fmindexUp = Resample(self.fmindex, mode=1)

        # Generate the FM synthesis.
        self.fmUp = FM(carrier=492, ratio=2, index=self.fmindexUp)

        # Close the upsampled block.
        server.endResamplingBlock()

        # Convert back the synthesized signal to the current sampling rate.
        # We use a good decimination filter to eliminate aliasing.
        self.output = Resample(self.fmUp, mode=filtmode)

    # Define some useful methods.
    def out(self):
        self.output.out()
        return self

    def sig(self):
        return self.output


# Control for the modulation index parameter of the synthesis.
index = Sig(20)
index.ctrl([SLMap(5, 50, "lin", "value", 20)], title="Modulation Index")

# FM synthesis at current sampling rate.
fm1 = FM(carrier=492, ratio=2, index=index)

# FM synthesis with increased sampling rate.
fm2 = UpSampFM(index)

# Interpolator to compare the two processes.
output = Interp(fm1, fm2.sig(), 0, mul=0.5).out()
output.ctrl([SLMap(0, 1, "lin", "interp", 0)], title="Up Sampling: without <=> with")

sp = Spectrum(output)

s.gui(locals())
