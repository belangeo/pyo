"""
Windows audio host inspector.

This script will check if pyo can run in duplex mode (both audio input and output)
and will test every host API to help the user in making his audio device choice.

"""
import sys, time
from pyo import *

if sys.platform == "win32":
    host_apis = ["mme", "directsound", "asio", "wasapi", "wdm-ks"]
else:
    print("This program must be used on a windows system! Ciao!")
    exit()


print("* Checking for any available audio input... *")

input_names, input_indexes  = pa_get_input_devices()

print("* Checking audio output hosts... *")

s = Server(duplex=0)
s.verbosity = 0

host_results = []
for host in host_apis:
    print("* Testing %s... *" % host)
    try:
        s.reinit(buffersize=1024, duplex=0, winhost=host)
        s.boot().start()
        a = Sine(freq=440, mul=0.2).out()
        time.sleep(2)
        s.stop()
        s.shutdown()
        host_results.append(True)
    except:
        host_results.append(False)

print("\nResults")
print("-------\n")

if len(input_names):
    print("Duplex mode OK!")
    print("Initialize the Server with duplex=1 as argument.\n")
else:
    print("No input available. Duplex mode should be turned off.")
    print("Initialize the Server with duplex=0 as argument.\n")

for i, host in enumerate(host_apis):
    if host_results[i]:
        print("Host: %s  ==>  OK!" % host)
    else:
        print("Host: %s  ==>  Failed..." % host)

print("Initialize the Server with the desired host given to winhost argument.")

print("\nFinished!")
