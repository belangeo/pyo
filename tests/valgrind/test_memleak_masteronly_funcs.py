# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_masteronly_funcs.py 
# There should not be any definitely lost bytes.

import os
os.environ["PYO_GUI_WX"] = "0"

from pyo import *

infos = sndinfo("../../pyo/lib/snds/transparent.aif")
print(infos)

savefile(samples=[0.0]*44100, path="savefile_test.wav", sr=44100, channels=1, fileformat=0, sampletype=0)
print(sndinfo("savefile_test.wav"))

upsamp("savefile_test.wav", "savefile_test_up.wav")
print(sndinfo("savefile_test_up.wav"))

downsamp("savefile_test_up.wav", "savefile_test_down.wav")
print(sndinfo("savefile_test_down.wav"))

pm_list_devices()
print("pm_count_devices: ", pm_count_devices())
print("pm_get_input_devices: ", pm_get_input_devices())
print("pm_get_output_devices: ", pm_get_output_devices())
print("pm_get_default_output: ", pm_get_default_output())
print("pm_get_default_input: ", pm_get_default_input())

print("pa_get_version: ", pa_get_version())
print("pa_get_version_text: ", pa_get_version_text())

s = Server(audio="manual")
s.deactivateMidi()
s.boot()

t = WinTable()
savefileFromTable(t, "savefileFromTable_test.wav")
print(sndinfo("savefileFromTable_test.wav"))

s.shutdown()

os.remove("savefile_test.wav")
os.remove("savefile_test_up.wav")
os.remove("savefile_test_down.wav")
os.remove("savefileFromTable_test.wav")
