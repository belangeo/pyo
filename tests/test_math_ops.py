import time
from pyo import *

s = Server().boot()

# Math ops between PyoObject_base and float
a1 = Sig(2)
b1 = a1[0] * 0.5
c1 = a1[0] + 0.5
d1 = a1[0] - 0.5
e1 = a1[0] / 0.5
pa1 = Print(Sig(0), method=1, message="\n=== Math ops between PyoObject_base and float")
pb1 = Print(b1, method=1, message="mul (should be 1.0):")
pc1 = Print(c1, method=1, message="add (should be 2.5):")
pd1 = Print(d1, method=1, message="sub (should be 1.5):")
pe1 = Print(e1, method=1, message="div (should be 4.0):")

# Math ops between float and PyoObject_base
a2 = Sig(2)
b2 = 0.5 * a2[0]
c2 = 0.5 + a2[0]
d2 = 0.5 - a2[0]
e2 = 0.5 / a2[0]
pa2 = Print(Sig(0), method=1, message="\n=== Math ops between float and PyoObject_base")
pb2 = Print(b2, method=1, message="mul (should be 1.0):")
pc2 = Print(c2, method=1, message="add (should be 2.5):")
pd2 = Print(d2, method=1, message="sub (should be -1.5):")
pe2 = Print(e2, method=1, message="div (should be 0.25):")

# This one can work by calling MULTIPLY macro with -1 as mul argument.
# g = -a[0]

# Math ops between PyoObject and float
a3 = Sig(2)
b3 = a3 * 0.5
c3 = a3 + 0.5
d3 = a3 - 0.5
e3 = a3 / 0.5
pa3 = Print(Sig(0), method=1, message="\n=== Math ops between PyoObject and float")
pb3 = Print(b3, method=1, message="mul (should be 1.0):")
pc3 = Print(c3, method=1, message="add (should be 2.5):")
pd3 = Print(d3, method=1, message="sub (should be 1.5):")
pe3 = Print(e3, method=1, message="div (should be 4.0):")

# Math ops between float and PyoObject
a4 = Sig(2)
b4 = 0.5 * a4
c4 = 0.5 + a4
d4 = 0.5 - a4
e4 = 0.5 / a4
pa4 = Print(Sig(0), method=1, message="\n=== Math ops between float and PyoObject")
pb4 = Print(b4, method=1, message="mul (should be 1.0):")
pc4 = Print(c4, method=1, message="add (should be 2.5):")
pd4 = Print(d4, method=1, message="sub (should be -1.5):")
pe4 = Print(e4, method=1, message="div (should be 0.25):")

# Math ops between PyoObject and PyoObject
a5 = Sig(2)
aa5 = Sig(0.5)
b5 = a5 * aa5
c5 = a5 + aa5
d5 = a5 - aa5
e5 = a5 / aa5
pa5 = Print(Sig(0), method=1, message="\n=== Math ops between PyoObject and PyoObject")
pb5 = Print(b5, method=1, message="mul (should be 1.0):")
pc5 = Print(c5, method=1, message="add (should be 2.5):")
pd5 = Print(d5, method=1, message="sub (should be 1.5):")
pe5 = Print(e5, method=1, message="div (should be 4.0):")

# Math ops between PyoObject and list of floats
a6 = Sig(2)
b6 = a6 * [0.5, 0.1]
c6 = a6 + [0.5, 0.1]
d6 = a6 - [0.5, 0.1]
e6 = a6 / [0.5, 0.1]
pa6 = Print(Sig(0), method=1, message="\n=== Math ops between PyoObject and list of floats")
pb6 = Print(b6, method=1, message="mul (should be 1.0 and 0.2):")
pc6 = Print(c6, method=1, message="add (should be 2.5 and 2.1):")
pd6 = Print(d6, method=1, message="sub (should be 1.5 and 1.9):")
pe6 = Print(e6, method=1, message="div (should be 4.0 and 20.0):")

# Math ops between list of floats and PyoObject
a7 = Sig(2)
b7 = [0.5, 0.1] * a7
c7 = [0.5, 0.1] + a7
d7 = [0.5, 0.1] - a7
e7 = [0.5, 0.1] / a7
pa7 = Print(Sig(0), method=1, message="\n=== Math ops between list of floats and PyoObject")
pb7 = Print(b7, method=1, message="mul (should be 1.0 and 0.2):")
pc7 = Print(c7, method=1, message="add (should be 2.5 and 2.1):")
pd7 = Print(d7, method=1, message="sub (should be -1.5 and -1.9):")
pe7 = Print(e7, method=1, message="div (should be 0.25 and 0.05):")

# Math ops between PyoObject and multi-streams PyoObject
a8 = Sig(2)
aa8 = Sig([0.5, 0.1])
b8 = a8 * aa8
c8 = a8 + aa8
d8 = a8 - aa8
e8 = a8 / aa8
pa8 = Print(Sig(0), method=1, message="\n=== Math ops between PyoObject and multi-streams PyoObject",)
pb8 = Print(b8, method=1, message="mul (should be 1.0 and 0.2):")
pc8 = Print(c8, method=1, message="add (should be 2.5 and 2.1):")
pd8 = Print(d8, method=1, message="sub (should be 1.5 and 1.9):")
pe8 = Print(e8, method=1, message="div (should be 4.0 and 20.0):")

# Math ops between multi-streams PyoObject and PyoObject
a9 = Sig([0.5, 0.1])
aa9 = Sig(2)
b9 = a9 * aa9
c9 = a9 + aa9
d9 = a9 - aa9
e9 = a9 / aa9
pa9 = Print(Sig(0), method=1, message="\n=== Math ops between multi-streams PyoObject and PyoObject",)
pb9 = Print(b9, method=1, message="mul (should be 1.0 and 0.2):")
pc9 = Print(c9, method=1, message="add (should be 2.5 and 2.1):")
pd9 = Print(d9, method=1, message="sub (should be -1.5 and -1.9):")
pe9 = Print(e9, method=1, message="div (should be 0.25 and 0.05):")

s.start()
time.sleep(0.1)
