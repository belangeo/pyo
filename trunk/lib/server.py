from _core import *

######################################################################
### Proxy of Server object
######################################################################
class Server:
    def __init__(self, sr=44100, nchnls=1, buffersize=64, duplex=0):
        self._server = Server_base(sr, nchnls, buffersize, duplex)

    def setInputDevice(self, x):
        self._server.setInputDevice(x)

    def setOutputDevice(self, x):
        self._server.setOutputDevice(x)

    def setMidiInputDevice(self, x):
        self._server.setMidiInputDevice(x)
 
    def setSamplingRate(self, x):
        self._server.setSamplingRate(x)

    def setBufferSize(self, x):
        self._server.setBufferSize(x)
  
    def setNchnls(self, x):
        self._server.setNchnls(x)

    def setDuplex(self, x):
        self._server.setDuplex(x)
 
    def shutdown(self):
        self._server.shutdown()
        
    def boot(self):
        self._server.boot()
        
    def start(self):
        self._server.start()
    
    def stop(self):
        self._server.stop()
        
    def recstart(self):
        self._server.recstart()
        
    def recstop(self):
        self._server.recstop()
        
    def getStreams(self):
        return self._server.getStreams()
        
    def getSamplingRate(self):
        return self._server.getSamplingRate()
        
    def getNchnls(self):
        return self._server.getNchnls()
        
    def getBufferSize(self):
        return self._server.getBufferSize()
