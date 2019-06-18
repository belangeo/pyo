# Pyo - Python DSP module #

pyo is a Python module written in C to help digital signal processing script 
creation.

pyo is a Python module containing classes for a wide variety of audio signal 
processing types. With pyo, user will be able to include signal processing 
chains directly in Python scripts or projects, and to manipulate them in real 
time through the interpreter. Tools in pyo module offer primitives, like 
mathematical operations on audio signal, basic signal processing (filters, 
delays, synthesis generators, etc.), but also complex algorithms to create 
sound granulation and others creative audio manipulations. pyo supports OSC 
protocol (Open Sound Control), to ease communications between softwares, and 
MIDI protocol, for generating sound events and controlling process parameters. 
pyo allows creation of sophisticated signal processing chains with all the 
benefits of a mature, and widely used, general programming language.

Systems : macOS (10.10+), linux, Windows (XP, Vista, 7, 8, 10)

Python versions : 2.7, 3.5+

**For more resources, informations and documentation**, visit the 
[PYO OFFICAL WEB SITE](http://ajaxsoundstudio.com/pyo/).

How to install pre-built packages on any platform using **pip**: 
[INSTALL Instructions](http://ajaxsoundstudio.com/pyodoc/download.html).
    
How to get pyo running from sources on macOS and linux:
[COMPILE Instructions](http://ajaxsoundstudio.com/pyodoc/compiling.html).

pyo was awarded **second prize** in the 
[Lomus 2012](http://concours.afim-asso.org/2012/) Free Software Competition.

**You want to help the development of pyo ?** Go to the
[pyo features market](https://github.com/belangeo/pyo/wiki/Pyo-features-market) 
and make a donation for the feature you want to promote. You can also submit new
features on the mailing-list ( pyo-discuss@googlegroups.com ).

## Radio Pyo ##

If you want to listen to scripts rendered in real-time, just connect to 
[Radio Pyo](http://radiopyo.acaia.ca/) !

You want to have your script played on the radio ? Download the template
[radiopyo_template.py](https://github.com/tiagovaz/radiopyo/blob/master/utils/radiopyo_template.py),
follow the rules and post it on the mailing-list !

## Softwares using pyo as audio engine ##

[Cecilia 5](http://ajaxsoundstudio.com/software/cecilia/) : An audio signal 
processing environment.

[PsychoPy](http://www.psychopy.org/) : An open-source application to allow the 
presentation of stimuli and collection of data for a wide range of neuroscience, 
psychology and psychophysics experiments.

[Soundgrain](http://ajaxsoundstudio.com/software/soundgrain/) : 
A graphical interface where users can draw and edit trajectories to control 
granular sound synthesis.

[Zyne](https://github.com/belangeo/zyne) : A modular soft synthesizer.

[Pyo Synth](https://github.com/alexandrepoirier/PyoSynth) : Pyo Synth is an open
source application that makes the manipulation of pyo scripts easier by letting
you control it with a midi keyboard. 

## Examples ##

pyo is fully integrated to Python and very simple to use.

Play a sound:

```
>>> from pyo import *
>>> s = Server().boot()
>>> s.start()
>>> sf = SfPlayer("path/to/your/sound.aif", speed=1, loop=True).out()
```

Granulate an audio buffer:

```
>>> s = Server().boot()
>>> s.start()
>>> snd = SndTable("path/to/your/sound.aif")
>>> env = HannTable()
>>> pos = Phasor(freq=snd.getRate()*.25, mul=snd.getSize())
>>> dur = Noise(mul=.001, add=.1)
>>> g = Granulator(snd, env, [1, 1.001], pos, dur, 24, mul=.1).out()
```

Generate melodies:

```
>>> s = Server().boot()
>>> s.start()
>>> wav = SquareTable()
>>> env = CosTable([(0,0), (100,1), (500,.3), (8191,0)])
>>> met = Metro(.125, 12).play()
>>> amp = TrigEnv(met, table=env, mul=.1)
>>> pit = TrigXnoiseMidi(met, dist='loopseg', x1=20, scale=1, mrange=(48,84))
>>> out = Osc(table=wav, freq=pit, mul=amp).out()
```

## Donation ##

This project is developed by Olivier Bélanger on his free time to provide a 
fully integrated Python dsp module for sound exploration and music composition. 
If you feel this project is useful to you and want to support it and it's 
future development please consider donating money. I only ask for a small 
donation, but of course I appreciate any amount.

[![](https://www.paypal.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=9CA99DH6ES3HA)
