import time

class BaseStart(object):
    "Base class for context manager to start and stop the audio server automatically."

    def __init__(self, audio_server):
        self._audio_server = audio_server

    def __enter__(self):
        self._audio_server.start()
        return self

    def __exit__(self, type, value, traceback):
        self._audio_server.stop()

    def wait(self, dur):
        time.sleep(dur)

    def waitOneBuf(self):
        time.sleep(self._audio_server.getBufferSize() / self._audio_server.getSamplingRate() + 0.005)

class Start(BaseStart):
    "Context manager to start and stop the audio server automatically."

    def __init__(self, audio_server):
        BaseStart.__init__(self, audio_server)


class StartAndWait(BaseStart):
    """
    Context manager to start and stop the audio server automatically.
    
    Use os.sleep() to wait `wait_time` just after entering the context.

    """

    def __init__(self, audio_server, wait_time):
        BaseStart.__init__(self, audio_server)
        self._wait_time = wait_time

    def __enter__(self):
        self._audio_server.start()
        time.sleep(self._wait_time)
        return self


class StartAndWaitOneBuf(BaseStart):
    """
    Context manager to start and stop the audio server automatically.
    
    Use os.sleep() to wait at least one buffer size just after entering the context.

    """

    def __init__(self, audio_server):
        BaseStart.__init__(self, audio_server)

    def __enter__(self):
        self._audio_server.start()
        time.sleep(self._audio_server.getBufferSize() / self._audio_server.getSamplingRate() + 0.005)
        return self
