"""
Sending and receiving data with OSC protocol

**05-send-receive-data.py**

OscDataSend and OscDataReceive are objects that should be used to
send and receive Open Sound Control messages when message type is
not simple float or when the user want to process values before 
sending using them to control an audio process.

**OscDataSend** sends messages in the form of a list containing elements
according to its `types` argument. See the documentation of the object
for the various available types.

**OscDataReceive** calls a function with the address as the first argument,
followed by a tuple containing every values of the data message.

"""
import random
from pyo import *

s = Server().boot()

# Little audio process to control with OSC messages.
source = SfPlayer("../snds/snd_1.aif")
freqs = SigTo([100, 200, 300, 400], time=2)
rezon = Resonx(source.mix(2), freq=freqs, q=2, stages=2).out()

# ----- Receiver -----

# Handle all messages coming from addresses starting with "/data/"
def getDataMessage(address, *args):
    if address == "/data/play":
        # Arguments are the sound to play as a string,
        # speed and gain as floats.
        source.path = args[0]
        source.speed = args[1]
        source.mul = args[2]
        source.play()
    elif address == "/data/rezo":
        # Arguments are 4 frequencies as floats.
        freqs.value = list(args)


# OSC data receiver.
rec = OscDataReceive(port=9900, address="/data/*", function=getDataMessage)

# ----- Sender 1 -----

# Builds and sends messages to load an play a new sound.
sender1 = OscDataSend(types="sff", port=9900, address="/data/play", host="127.0.0.1")


def choose():
    "Chooses new soundfile values and starts playback."
    soundfile = "../snds/snd_%i.aif" % random.randint(1, 6)
    speed = random.choice([0.5, 0.75, 1.0, 1.25])
    gain = random.uniform(0.5, 0.7)
    # Sends the message (message must be a list of values whose
    # types respect the `types` argument of the OscDataSend object.
    sender1.send([soundfile, speed, gain])
    # Adjusts the timing of the Pattern according to the playback duration.
    pat.time = sndinfo(soundfile)[1] / speed


# Call the choose() function to set a new sound playback.
pat = Pattern(choose).play()

# ----- Sender 2 -----

# Sends new frequencies to the resonator filters.
sender2 = OscDataSend(types="ffff", port=9900, address="/data/rezo", host="127.0.0.1")


def change():
    "Randomly chooses new frequencies and sends them to the filters."
    sender2.send([random.uniform(250, 4000) for i in range(4)])


# Call the change() function every 2 seconds.
pat2 = Pattern(change, time=2).play()

s.gui(locals())
