# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python3.9.supp python3 test_memleak_masteronly_objects.py 
# There should not be any definitely lost bytes.

# Errors still present:
# `Conditional jump or move depends on uninitialised value(s)` in vbap.c (choose_ls_triplets)
# `Invalid read of size 8` with OscListReceive

import os
os.environ["PYO_GUI_WX"] = "0"

from pyo import *

s = Server(audio="manual")
s.deactivateMidi()
s.boot().start()

i1 = Sig(0)
i2 = Sig(0)

a = HRTF(i1)
a.input = i2
a.azi = Sig(90)
a.ele = Sig(15)

b = Binaural(i1)
b.input = i2
b.azimuth = Sig(90)
b.elevation = Sig(15)
b.azispan = Sig(0.5)
b.elespan = Sig(0.5)

c = SfPlayer("../../pyo/lib/snds/transparent.aif")
c.setSound("../../pyo/lib/snds/accord.aif")
c.speed = Sig(0.5)

d = SfMarkerShuffler("../../pyo/lib/snds/transparent.aif")
d.speed = Sig(0.5)

e = SfMarkerLooper("../../pyo/lib/snds/transparent.aif")
e.speed = Sig(0.5)
e.mark = Sig(0)

f = CvlVerb(i1)
f.input = i2
f.setBal(Sig(0.5))

g = Record([i1, i2], "Record_test.wav")

a1 = Sine(freq=[1,1.5], mul=[100,.1], add=[600, .1])
h = OscSend(a1, port=10001, address=['/pitch','/amp'])

a2 = OscReceive(port=10001, address=['/pitch', '/amp'])
i = Sine(freq=a2['/pitch'], mul=a2['/amp']).mix(2).out()
a2.addAddress("/temp", 0.5, 0.01)
print(a2.getAddresses())
a2.delAddress("/temp")
print(a2.getAddresses())

def pp(address, *args):
     print(address)
     print(args)
r5 = OscDataReceive(9900, "/data/test", pp)
r5.addAddress("/data/test2")

# Send various types
a5 = OscDataSend("fissif", 9900, "/data/test")
msg = [3.14159, 1, "Hello", "world!", 2, 6.18]
a5.send(msg)
# Send a blob
b5 = OscDataSend("b", 9900, "/data/test")
msg = [[chr(i) for i in range(10)]]
b5.send(msg)
# Send a MIDI noteon on port 0
c5 = OscDataSend("m", 9900, "/data/test")
msg = [[0, 144, 60, 100]]
c5.send(msg, "/data/test")
c5.addAddress("fissif", 9900, "/data/test2")
msg = [3.14159, 1, "Hello", "world!", 2, 6.18]
c5.send(msg, "/data/test2")

j = OscListReceive(port=10002, address=['/pitch', '/amp'], num=8)
b3 = Sine(freq=j['/pitch'], mul=j['/amp']).mix(2).out()

s.process()
s.process()

g.stop()

s.stop()
s.shutdown()

os.remove("Record_test.wav")