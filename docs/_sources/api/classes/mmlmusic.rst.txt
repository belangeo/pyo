Music Macro Language evaluator
=================================

.. currentmodule:: pyo

.. highlight:: none

The MML object implements acustom MML evaluator to allow simple and efficient
music composition within pyo. The language's rules are explained below.

The MML object generates triggers on new notes with additional streams to
handle frequency, amplitude, duration and custom parameters. See the object
documentation for more details.

**API documentation**
---------------------

- The space separates the tokens in a music sequence (a token can be a note value,
  an amplitude control, a tempo statement, etc.).

Pre-Processing on music text
----------------------------

- A comment starts with a semicolon ( **;** ) and ends at the end of the line. This
  text will be removed before starting to evaluate the sequences.

- A musical voice is represented by a single line of code.

- We can break a long line into multiple short lines with the backslash ( **\\** ).

- The symbol equal ( **=** ), preceded by a variable name in UPPER CASE, creates a
  macro. The remaining part of the line is the macro body. Anywhere the 
  pre-processor finds the variable name in the music, it will be replaced by the
  macro's body.

Realtime Processing of the music
--------------------------------

**Voice number**

- The symbol #, followed by a number indicates the voice number for the line. This should be
  the first token of a line. If missing, the line defaults to voice number 0.

**Notes**

- The letters **a** to **g** correspond to the musical pitches and cause the
  corresponding note to be played.

- Sharp notes are produced by appending a **+** to the pitch value, and flat notes
  by appending a **-** to the pitch value.

- The length of a note is specified by an integer following the note name. If a
  note doesn't have a duration, the last specified duration is used. Default
  duration is the Sixteenth note. Length values are:

  - 0 = Thirty-second note
  - 1 = Sixteenth note
  - 2 = Dotted sixteenth note
  - 3 = Eighth note
  - 4 = Dotted eighth note
  - 5 = Quarter note
  - 6 = Dotted quarter note
  - 7 = Half note
  - 8 = Dotted half note
  - 9 = Whole note

- The letter **r** corresponds to a rest. The length of the rest is specified in
  the same manner as the length of a note.

- The letter **o**, followed by a number, selects the octave the instrument will play in.
  If the letter **o** is followed by the symbol **+**, the octave steps up by one. If followed
  by the symbol **-**, the octave steps down by one. If a number follows the symbol **+** or **-**,
  the octave steps up or down by the given amount of octaves.

**Tuplets**

- Notes surrounded by brakets ( **(** and **)** ) act as tuplet. Tuplet length is specified
  just after the closing bracket using the same values as for a note duration. Length of
  each note in tuplet will evenly be <note length of tuplet> / <count of notes in tuplet>.
  If not specified, tuplet duration defaults to 5 (quarter note).

**Tempo**

- The letter **t**, followed by a number, sets the tempo in beats-per-minute.
  If the letter **t** is followed by the symbol **+**, the tempo increases by one. If followed
  by the symbol **-**, the tempo decreases by one. If a number follows the symbol **+** or **-**,
  the tempo increases or decreases by the given amount of BPM.

**Volume**

- The letter **v**, followed by a number between 0 and 100, sets the volume for the following
  notes. If the letter **v** is followed by the symbol **+**, the volume increases by one. If
  followed by the symbol **-**, the volume decreases by one. If a number follows the symbol 
  **+** or **-**, the volume increases or decreases by the given amount.

**User variables**

- The letters **x**, **y** an **z**, followed by a real number, are user-defined parameters. They
  can be used to control specific parameters of the synthesizer.
  If the letter is followed by the symbol **+**, the value increases by 0.01. If followed
  by the symbol **-**, the value decreases by 0.01. If a number follows the symbol **+** or **-**,
  the value increases or decreases by the given amount.

**Random selection**

- Random choice within a set of values can be done with the **?** symbol, followed by the
  possible values inside square brackets.
  Ex. ?[c e g b-] ; the note is a random choice between c e g and b-.

- Random choice between a range can be done with the **?** symbol, followed by the range inside
  curly brackets. If two values are presents, they are the minimum and maximum of the range.
  If there is only one value, the range is 0 to this value and if the brackets are empty, the
  range is 0 to 1.
  Ex. v?{40 70} ; volume is set randomly between 40 and 70.

**Looping segments**

- The symbol **|:** starts a looped segment and the symbol **:|** ends it. A number right after the
  last symbol indicates how many loops to perform. If missing, the number of loops is two (the
  first pass + one repetition). It is possible to use loops inside other loops. There is no
  limit to the number of levels of loop embedding.

.. highlight:: python

**Objects**
-----------

*MML*
----------

.. autoclass:: MML
   :members:

   .. autoclasstoc::
