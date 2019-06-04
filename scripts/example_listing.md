Exemples de scripts audio avec pyo
==================================

01-intro
--------

1. audio-server.py
2. sine-tone.py
3. parallel-proc.py
4. serial-proc.py
5. output-channels.py

02-controls
-----------

1. fixed-control.py
2. dynamic-control.py
3. output-range.py
4. building-lfo.py
5. math-ops.py
6. multichannel-expansion.py
7. multichannel-expansion-2.py
8. handling-channels.py
9. handling-channels-2.py
10. handling-channels-3.py
11. handling-channels-4.py

03-generators
-------------

1. complex-oscs.py
2. band-limited-oscs.py
3. fm-generators.py
4. noise-generators.py
5. strange-attractors.py
6. random-generators.py

04-soundfiles
-------------

1. read-from-disk.py
2. read-from-disk-2.py
3. read-from-ram.py
4. record-perf.py
5. record-streams.py
6. record-table.py

05-envelopes
------------
1. data-signal-conversion.py (Sig, get())
2. linear-ramp.py (SigTo, set())
3. exponential-ramp.py (Port)
4. simple-envelopes.py (Fader, Adsr)
5. breakpoints-function.py (Linseg, Expseg)

06-filtres
----------
1. lowpass-filters.py
2. bandpass-filters.py
3. complex-resonator.py
4. phasing.py
5. convolution-filters.py
6. vocoder.py
7. hilbert-transform.py

07-effects
----------

1. flanger.py
2. schroeder-reverb.py
3. fuzz-disto.py
4. ping-pong-delay.py
5. hand-made-chorus.py
6. hand-made-harmonizer.py

08-dynamics
-----------
1. dynamic-range.py
2. ducking.py
3. gated-verb.py
4. rms-tracing.py

09-callbacks
------------

1. periodic-calls.py
2. score-calls.py
3. delayed-calls.py

10-tables
---------

1. envelopes.py
2. scrubbing.py
3. looping.py
4. granulation.py
5. micro-montage.py
6. table-stutter.py
7. moving-points.py
8. table-lookup.py

**All good up to here**

? NewTable, PartialTable, PadSynthTable ?

11-triggers
-----------

1. periodic-triggers.py
2. sequence-triggers.py
3. random-triggers.py
4. step-sequencer.py

12-algorithmic
--------------

1. melodic-algo.py
2. harmonic-algo.py
3. drum-machine.py
4. complex-algo.py
5. table-algorithm.py
6. markov.py

13-multiband
------------

1. band-splitter.py
2. band-splitter-2.py
3. four-bands.py
4. four-bands-2.py
5. multi-bands.py

14-spectral
-----------

1. fft-filter.py 
2. fft-cross-synth.py
3. pv-stretch.py
4. pv-transpose.py
5. pv-playback-speed.py
6. pv-bin-modulation.py

15-matrices
-----------

1. matrix-record.py
2. matrix-from-image.py
3. wave-terrain-synthesis.py
4. record-loop-chunks.py
5. algo-with-matrix.py

16-midi
-------

1. midi-scan.py
2. notein-object.py
3. midi-envelope.py
4. simple-midi-synth.py
5. control-change.py
6. midi-out.py
7. midifile-with-mido.py

17-osc
------

1. osc-scan.py
2. osc-send-streams.py
3. osc-receive-streams.py
4. osc-receive-list.py
5. osc-send-data.py
6. osc-receive-data.py

18-synthesis
------------

1. voltage-controlled-oscillator.py
2. band-limited-oscillator.py
3. complex-fm.py
4. phase-aligned-formant.py
5. split-sideband-synthesis.py
6. bucket-brigade-device.py

19-multirate
------------
1. multi-rate-processing.py
2. multi-rate-synthesis.py

20-multicore
------------
1. processes-spawning.py
2. processes-sharing-audio.py
3. processes-synchronization.py
4. processes-data-communication.py

21-utilities
------------

1. save-audio-file.py
2. batch-processing.py
3. batch-synthesis.py
4. upsamp-processing.py
5. multiple-threads.py

22-wxgui
--------

misc
----

**handling objects life (start time and duration)**
**autoStartChildren**