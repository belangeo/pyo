# -*- coding: utf-8 -*-

# Copyright 2009-2021 Olivier Belanger
# 
# This file is part of pyo, a python module to help digital signal
# processing script creation.
#
# pyo is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# pyo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with pyo.  If not, see <http://www.gnu.org/licenses/>.

from ._core import *

######################################################################
### Proxy of Server object
######################################################################
class Server(object):

    def __init__(
        self,
        sr=44100,
        nchnls=2,
        buffersize=256,
        duplex=1,
        ichnls=None
    ):

        self._nchnls = nchnls
        if ichnls is None:
            self._ichnls = nchnls
        else:
            self._ichnls = ichnls
        self._verbosity = 7
        self._dur = -1
        self._filename = None
        self._fileformat = 0
        self._sampletype = 0
        self._globalseed = 0
        self._resampling = 1
        self._server = Server_base(sr, nchnls, buffersize, duplex, self._ichnls)

    def __del__(self):
        self.setTime = None
        self.setRms = None
        if self.getIsBooted():
            if self.getIsStarted():
                self.stop()
            self.shutdown()

    def reinit(
        self,
        sr=44100,
        nchnls=2,
        buffersize=256,
        duplex=1,
        ichnls=None
    ):

        self._gui_frame = None
        self._nchnls = nchnls
        if ichnls is None:
            self._ichnls = nchnls
        else:
            self._ichnls = ichnls
        self._verbosity = 7
        self._dur = -1
        self._filename = None
        self._fileformat = 0
        self._sampletype = 0
        self._globalseed = 0
        self._resampling = 1
        self._server.__init__(sr, nchnls, buffersize, duplex, self._ichnls)

    def setCallback(self, callback):
        if callable(callback):
            self._server.setCallback(callback)

    def setSamplingRate(self, x):
        self._server.setSamplingRate(x)

    def setBufferSize(self, x):
        self._server.setBufferSize(x)

    def setNchnls(self, x):
        self._nchnls = x
        self._server.setNchnls(x)

    def setIchnls(self, x):
        self._ichnls = x
        self._server.setIchnls(x)

    def setDuplex(self, x):
        self._server.setDuplex(x)

    def setVerbosity(self, x):
        self._verbosity = x
        self._server.setVerbosity(x)

    def setGlobalDur(self, x):
        self._server.setGlobalDur(x)

    def setGlobalDel(self, x):
        self._server.setGlobalDel(x)

    def setGlobalSeed(self, x):
        self._globalseed = x
        self._server.setGlobalSeed(x)

    def beginResamplingBlock(self, x):
        realx = x
        x = abs(x)
        if ((x & (x - 1)) == 0) and x != 0:
            self._resampling = realx
            self._server.beginResamplingBlock(realx)
        else:
            print("Resampling factor must be a power-of-two (positive or negative).")

    def endResamplingBlock(self):
        self._resampling = 1
        self._server.endResamplingBlock()

    def shutdown(self):
        self._server.shutdown()

    def boot(self, newBuffer=True):
        self._server.boot(newBuffer)
        return self

    def start(self):
        self._server.start()
        return self

    def stop(self):
        self._server.stop()

    def getStreams(self):
        return self._server.getStreams()

    def getSamplingRate(self):
        return self._server.getSamplingRate()

    def getNchnls(self):
        return self._server.getNchnls()

    def getBufferSize(self):
        return self._server.getBufferSize()

    def getGlobalDur(self):
        return self._server.getGlobalDur()

    def getGlobalDel(self):
        return self._server.getGlobalDel()

    def getGlobalSeed(self):
        return self._server.getGlobalSeed()

    def getIsStarted(self):
        return self._server.getIsStarted()

    def getIsBooted(self):
        return self._server.getIsBooted()

    def getStreams(self):
        return self._server.getStreams()

    def getNumberOfStreams(self):
        return len(self._server.getStreams())

    def setServer(self):
        return self._server.setServer()

    def getInputAddr(self):
        return self._server.getInputAddr()

    def getOutputAddr(self):
        return self._server.getOutputAddr()

    def getServerID(self):
        return self._server.getServerID()

    def getServerAddr(self):
        return self._server.getServerAddr()

    def getEmbedICallbackAddr(self):
        return self._server.getEmbedICallbackAddr()

    def setAutoStartChildren(self, state):
        self._server.setAutoStartChildren(state)

    def process(self):
        self._server.process()

    @property
    def verbosity(self):
        return self._verbosity

    @verbosity.setter
    def verbosity(self, x):
        if type(x) == int:
            self.setVerbosity(x)
        else:
            raise Exception("verbosity must be an integer")

    @property
    def globalseed(self):
        return self._globalseed

    @globalseed.setter
    def globalseed(self, x):
        if type(x) == int:
            self.setGlobalSeed(x)
        else:
            raise Exception("global seed must be an integer")
