from _core import *

######################################################################
### Proxy of Server object
######################################################################
class Server:
    def __init__(self, sr=44100, nchnls=1, buffersize=64, duplex=0):
        self._server = Server_base(sr, nchnls, buffersize, duplex)
        
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
