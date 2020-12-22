import math

class BaseStart(object):
    "Base class for context manager to start and stop the audio server automatically."

    def __init__(self, audio_server):
        self._audio_server = audio_server
        self._sr = audio_server.getSamplingRate()
        self._bs = audio_server.getBufferSize()

    def __enter__(self):
        self._audio_server.start()
        return self

    def __exit__(self, type, value, traceback):
        self._audio_server.stop()

    def advance(self, dur):
        numbuf = int(math.ceil(dur * self._sr / self._bs))
        for i in range(numbuf):
            self._audio_server.process()

    def advanceOneBuf(self):
        self._audio_server.process()

class Start(BaseStart):
    "Context manager to start and stop the audio server automatically."

    def __init__(self, audio_server):
        BaseStart.__init__(self, audio_server)


class StartAndAdvance(BaseStart):
    """
    Context manager to start and stop the audio server automatically.
    
    Advance `adv_time` seconds just after entering the context.

    """

    def __init__(self, audio_server, adv_time):
        BaseStart.__init__(self, audio_server)
        self._numbuf = int(math.ceil(adv_time * self._sr / self._bs))

    def __enter__(self):
        self._audio_server.start()
        for i in range(self._numbuf):
            self._audio_server.process()
        return self


class StartAndAdvanceOneBuf(BaseStart):
    """
    Context manager to start and stop the audio server automatically.
    
    Advance one buffer size just after entering the context.

    """

    def __init__(self, audio_server):
        BaseStart.__init__(self, audio_server)

    def __enter__(self):
        self._audio_server.start()
        self._audio_server.process()
        return self
