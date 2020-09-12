"""
02-read-from-disk-2.py - Catching the `end-of-file` signal from the SfPlayer object.

This example demonstrates how to use the `end-of-file` signal
of the SfPlayer object to trigger another playback (possibly
with another sound, another speed, etc.).

When a SfPlayer reaches the end of the file, it sends a trigger
(more on trigger later) that the user can retrieve with the
syntax :

variable_name["trig"]

"""
from pyo import *
import random

s = Server().boot()

# Sound bank
folder = "../snds/"
sounds = ["alum1.wav", "alum2.wav", "alum3.wav", "alum4.wav"]

# Creates the left and right players
sfL = SfPlayer(folder + sounds[0], speed=1, mul=0.5).out()
sfR = SfPlayer(folder + sounds[0], speed=1, mul=0.5).out(1)

# Function to choose a new sound and a new speed for the left player
def newL():
    sfL.path = folder + sounds[random.randint(0, 3)]
    sfL.speed = random.uniform(0.75, 1.5)
    sfL.out()


# The "end-of-file" signal triggers the function "newL"
tfL = TrigFunc(sfL["trig"], newL)

# Function to choose a new sound and a new speed for the right player
def newR():
    sfR.path = folder + sounds[random.randint(0, 3)]
    sfR.speed = random.uniform(0.75, 1.5)
    sfR.out(1)


# The "end-of-file" signal triggers the function "newR"
tfR = TrigFunc(sfR["trig"], newR)

s.gui(locals())
