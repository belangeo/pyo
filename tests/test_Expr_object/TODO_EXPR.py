# IMPLEMENTED:
# function definition       ===>    (define myfunc (body)) --> arguments are defined with $1, $2, ..., $9
# if then else              ===>    (if (condition) (then) (else))
# variable definition       ===>    (let #var (body)) --> process one time and use the result elsewhere.
#                                   private to definition if created inside a (define ) statement.
# external variables        ===>    (var #var (init)) --> process only on init pass, can be changed via method calls
# library importation       ===>    (load path/to/lib)
# function arguments        ===>    replace missing args with 0.0 and ignore extra args
# complex number            ===>    (complex x y) --> returns a complex number.
#                                   (real x) --> returns real value from complex number x.
#                                   (imag x) --> returns imaginary value from complex number x.
#                           ===>    (cpole signal coeff) --> complex one-pole recursive filter (returns a complex number).
#                           ===>    (czero signal coeff) --> complex one-pole non-recursive filter (returns a complex number).
#                           ===>    (let #cplx (complex re im)) --> let statement works with complex numbers.
# Multiple input signals to
# an expression             ===>    (* $x[0] $x1[0]) --> Ring-modulation between two input signals ($x is the same as $x0).
# Multiple output signals to - (need a new argument to the object, `outs=1`)
# an expression             ===>    (out 0 (sin (* twopi (~ 100)))) --> First output signal is a 100Hz sine wave.
#                                   (out 1 (sin (* twopi (~ 200)))) --> Second output signal is a 200Hz sine wave.
#                                   Output function must be used only when outs > 1.

# TODO:
# multi-stages function     ===>    (+ 1 2 3 4 5) --> (+ (+ (+ (+ 1 2) 3) 4) 5)
# recursive function        ===>    (loop (delay x) 10) --> the output of the first call is used as input for the second and so on...
# Optimization of the processing loop.

"""
(loop (delay x) 4) ===>

(delay (delay (delay (delay x))))
"""
