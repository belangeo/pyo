"""
01-audio-server.py - Booting the audio server.

A Server object needs to be created before any other audio object.
It is the one that handles the communication with the audio and midi
drivers and also the one that keeps track of the processing chain.

"""
from pyo import *

# Creates a Server object with default arguments.
# See the manual about how to change the sampling rate, the buffer
# size, the number of channels or one of the other global settings.
s = Server()

# Boots the server. This step initializes audio and midi streams.
# Audio and midi configurations (if any) must be done before that call.
s.boot()

# Starts the server. This step activates the server processing loop.
s.start()

# Here comes the processing chain...

# The Server object provides a Graphical User Interface with the
# gui() method. One of its purpose is to keep the program alive
# while computing samples over time. If the locals dictionary is
# given as argument, the user can continue to send commands to the
# python interpreter from the GUI.
s.gui(locals())
