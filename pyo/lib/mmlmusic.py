"""
Music Macro Language evaluator.

The MML object implements acustom MML evaluator to allow simple and efficient
music composition within pyo. The language's rules are explained below.

The MML object generates triggers on new notes with additional streams to
handle frequency, amplitude, duration and custom parameters. See the object
documentation for more details.

API documentation
=================

- The space separates the tokens in a music sequence (a token can be a note value,
  an amplitude control, a tempo statement, etc.).

Pre-Processing on music text
----------------------------

- A comment starts with a semicolon ( ; ) and ends at the end of the line. This
  text will be removed before starting to evaluate the sequences.

- A musical voice is represented by a single line of code.

- We can break a long line into multiple short lines with the backslash ( \ ).

- The symbol equal ( = ), preceded by a variable name in UPPER CASE, creates a
  macro. The remaining part of the line is the macro body. Anywhere the 
  pre-processor finds the variable name in the music, it will be replaced by the
  macro's body.

Realtime Processing of the music
--------------------------------

- The letters `a` to `g` correspond to the musical pitches and cause the
  corresponding note to be played.

- Sharp notes are produced by appending a `+` to the pitch value, and flat notes
  by appending a `-` to the pitch value.

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

- The letter `r` corresponds to a rest. The length of the rest is specified in
  the same manner as the length of a note.

- Notes surrounded by brakets ( `(` and `)` ) act as tuplet. Tuplet length is specified
  just after the closing bracket using the same values as for a note duration. Length of
  each note in tuplet will evenly be <note length of tuplet> / <count of notes in tuplet>.
  If not specified, tuplet duration defaults to 5 (quarter note).

- The letter `o`, followed by a number, selects the octave the instrument will play in.
  If the letter `o` is followed by the symbol `+`, the octave steps up by one. If followed
  by the symbol `-`, the octave steps down by one. If a number follows the symbol `+` or `-`,
  the octave steps up or down by the given amount of octaves.

- The letter `t`, followed by a number, sets the tempo in beats-per-minute.
  If the letter `t` is followed by the symbol `+`, the tempo increases by one. If followed
  by the symbol `-`, the tempo decreases by one. If a number follows the symbol `+` or `-`,
  the tempo increases or decreases by the given amount of BPM.

- The letter `v`, followed by a number between 0 and 100, sets the volume for the following
  notes. If the letter `v` is followed by the symbol `+`, the volume increases by one. If
  followed by the symbol `-`, the volume decreases by one. If a number follows the symbol 
  `+` or `-`, the volume increases or decreases by the given amount.

- The symbol #, followed by a number indicates the voice number for the line. This should be
  the first token of a line. If missing, the line defaults to voice number 0.

- The letters `x`, `y` an `z`, followed by a real number, are user-defined parameters. They
  can be used to control specific parameters of the synthesizer.
  If the letter is followed by the symbol `+`, the value increases by 0.01. If followed
  by the symbol `-`, the value decreases by 0.01. If a number follows the symbol `+` or `-`,
  the value increases or decreases by the given amount.

- Random choice within a set of values can be done with the ? symbol, followed by the
  possible values inside square brackets.
  Ex. ?[c e g b-] ; the note is a random choice between c e g and b-.

- Random choice between a range can be done with the ? symbol, followed by the range inside
  curly brackets. If two values are presents, they are the minimum and maximum of the range.
  If there is only one value, the range is 0 to this value and if the brackets are empty, the
  range is 0 to 1.
  Ex. v?{40 70} ; volume is set randomly between 40 and 70.

- The symbol |: starts a looped segment and the symbol :| ends it. A number right after the
  last symbol indicates how many loops to perform. If missing, the number of loops is two (the
  first pass + one repetition). It is possible to use loops inside other loops. There is no
  limit to the number of levels of loop embedding.

"""

"""
Copyright 2009-2020 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from ._core import *
from ._maps import *
from ._widgets import createMMLEditorWindow

### MML framework ###
#####################

VALID_NOTES = "abcdefgr?"
VALID_PARAMS = "xyz"
VALID_DIGITS = "0123456789"
VALID_SPACES = " \t\n"
VALID_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?"
MACRO_DELIMITERS = "0123456789abcdefgrxyz?()[]{}.|: \t\n"


class MMLParser:
    def __init__(self, text, voices=1):
        self.text = text
        self.voices = voices

    def _remove_comments(self, text):
        pos = text.find(";")
        while pos != -1:
            pos2 = text.find("\n", pos + 1)
            if pos2 == -1:
                text = text[:pos]
            else:
                text = text[:pos] + text[pos2 + 1 :]
            pos = text.find(";")
        return text

    def _split_tuplets(self, text):
        l = []
        text = text.strip()
        inside = False
        rescan = True
        start = 0
        for i in range(len(text)):
            if text[i] == " " and not inside:
                if text[start:i]:
                    l.append(text[start:i].strip())
                start = i
                rescan = True
            elif text[i] == "[":
                inside = True
            elif text[i] == "]":
                inside = False
                if i == len(text) - 1 and text[start:]:
                    l.append(text[start:].strip())
            elif i == len(text) - 1:
                if text[start:]:
                    l.append(text[start:].strip())
            elif rescan and text[i] != " ":
                rescan = False
                start = i
        return l

    def _expand_tuplets(self, text):
        pos = text.find("(")
        while pos != -1:
            nextpos = text.find("(", pos + 1)
            pos2 = text.find(")", pos + 1)
            if pos2 == -1:
                # missing end brace, just ignore the tuplet.
                text = text[:pos] + text[pos + 1 :]
            elif nextpos != -1 and nextpos < pos2:
                # missing end brace, just ignore the tuplet.
                text = text[:pos] + text[pos + 1 :]
            else:
                durchar = text[pos2 + 1]
                if durchar in VALID_DIGITS:
                    duration = int(durchar)
                else:
                    duration = 5  # default tuplet duration is the quarter note.
                tmp_eles = self._split_tuplets(text[pos + 1 : pos2])
                elements = []
                for ele in tmp_eles:
                    if ele[0] in VALID_NOTES:
                        elements.append("%s%i" % (ele, duration))
                    else:
                        elements.append(ele)
                # count only note elements.
                num_eles = len([e for e in elements if e[0] in VALID_NOTES])
                ele_text = " ".join(elements)
                ele_div = " /%i " % num_eles
                text = text[:pos] + ele_div + ele_text + " /1 " + text[pos2 + 2 :]
            pos = text.find("(")

        # Remove orphan closing braces.
        text = text.replace(")", " ")

        return text

    def _expand_macros(self, text):
        macros = {}
        new = ""
        for line in text.splitlines(True):
            if "=" in line:
                head, tail = line.split("=")
                tail = tail.replace("\n", "")
                macros[head.strip()] = tail.strip()
            else:
                new = new + line

        # Scan the text two times in case a macro uses another macro.
        for i in range(2):
            for macro in sorted(macros, key=len, reverse=True):
                pos = new.find(macro)
                while pos != -1:
                    if new[pos - 1] in MACRO_DELIMITERS and new[pos + len(macro)] in MACRO_DELIMITERS:
                        new = new[:pos] + macros[macro] + new[pos + len(macro) :]
                    pos = new.find(macro, pos + len(macro))
        return new

    def _remove_extra_spaces(self, text):
        while "  " in text:
            text = text.replace("  ", " ")
        return text

    def _add_space_around_loops(self, text):
        pos = text.find("|:")
        while pos != -1:
            if text[pos + 2] != " ":
                text = text[: pos + 2] + " " + text[pos + 2 :]
            if text[pos - 1] != " ":
                text = text[:pos] + " " + text[pos:]
            pos = text.find("|:", pos + 3)
        pos = text.find(":|")
        while pos != -1:
            if text[pos - 1] != " ":
                text = text[:pos] + " " + text[pos:]
            pos = text.find(":|", pos + 3)
        return text

    def _process_specific_group(self, text, inchar, outchar):
        pos = text.find(inchar)
        while pos != -1:
            nextpos = text.find(inchar, pos + 1)
            pos2 = text.find(outchar, pos + 1)
            if pos2 == -1:
                raise Exception("Missing %s symbol..." % outchar)
            elif nextpos != -1 and nextpos < pos2:
                raise Exception("Missing %s symbol..." % outchar)
            else:
                format = text[pos : pos2 + 1].replace(" ", ",")
                format = format.replace(" ", "")
                text = text[:pos] + format + text[pos2 + 1 :]
            pos = text.find(inchar, pos2 + 1)
        return text

    def _process_groups(self, text):
        text = self._process_specific_group(text, "[", "]")
        text = self._process_specific_group(text, "{", "}")
        return text

    def _expand_line_continuation(self, text):
        text = text.replace("\r", "")
        pos = text.find("\\")
        while pos != -1:
            pos2 = text.find("\n", pos + 1)
            text = text[:pos] + text[pos2 + 1 :]
            pos = text.find("\\", pos + 1)
        return text

    def _preproc(self, text):
        text = self._remove_comments(text)
        text = self._expand_line_continuation(text)
        text = self._remove_extra_spaces(text)
        text = self._expand_macros(text)
        text = self._expand_tuplets(text)
        text = self._add_space_around_loops(text)
        text = self._process_groups(text)
        return text

    def setText(self, text):
        self.text = text

    def getSequences(self):
        sequences = [None] * self.voices
        text = self._preproc(self.text)
        lines = [l.replace("\n", "").split() for l in text.splitlines() if l]
        for line in lines:
            if line[0].startswith("#"):
                voice = int(line[0][1:])
                if voice < self.voices:
                    sequences[voice] = line[1:]
            else:
                sequences[0] = line
        return sequences


class MML(PyoObject):
    """
    Generates music sequences based on a custom MML notation.

    Music Macro Language (MML) is a music description language used in
    sequencing music on computer and video game systems.


    :Parent: :py:class:`PyoObject`

    :Args:

        music: string
            The new music code to parse. If the string is a valid path
            to a text file, the file is opened and its content is taken
            as the music code.
        voices: int, optional
            The number of voices in the music code. This number is used
            to initialize the internal voices that will play the sequences.
            Defaults to 1.
        loop: bool, optional
            If True, the playback will start again when the music reaches
            its end, otherwise the object just stops to send triggers.
            Defaults to False.
        poly: int, optional
            Per voice polyphony. Denotes how many independent streams are
            generated per voice by the object, allowing overlapping
            processes.

            Available only at initialization. Defaults to 1.
        updateAtEnd: bool, optional
            If True, voices will update their internal sequence only when
            the current one reaches its end, no matter when the `music`
            argument is changed. If False, sequences are updated immediately.
            Defaults to False.

    .. note::

        MML outputs many signals identified with a string between brackets:

        |  obj['freq'] returns an audio stream of the current note frequency.
        |  obj['amp'] returns an audio stream of the current note amplitude.
        |  obj['dur'] returns an audio stream of the current note duration in seconds.
        |  obj['end'] returns an audio stream with a trigger at the end of the sequence.
        |  obj['x'] returns an audio stream with the current value of the `x` parameter.
        |  obj['y'] returns an audio stream with the current value of the `y` parameter.
        |  obj['z'] returns an audio stream with the current value of the `z` parameter.

        obj without brackets returns the generated trigger streams of the music.

        The out() method is bypassed. MML's signal can not be sent to audio outs.

        MML has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = '''
    >>> ; Title: La perdriole
    >>> ; Author: traditionnel
    >>> A = r6 o4 v40 g3 v50 o5 c d e f g5 o+ c o- b3 a g f e d c7
    >>> B = |: g3 g g4 f1 e3 d c5 :| g1 g g g g g g g b-3 o+ c d7 r7
    >>> C = |: o5 c4 d1 e3 f g4 o+ c1 o- b3 a g f e d e d c5 r5 :|
    >>> #0 t92 x.1 |: A A B C :|
    >>> A1 = |: r7 o4 c7 d7 e5 f g c7 :|
    >>> B1 = |: g7 o- b5 o+ c :| d5 d f g7 r7
    >>> C1 = |: c8 d7 g c5 r5 :|
    >>> #1 t92 x0.25 v50 |: A1 B1 C1 :|
    >>> '''
    >>> t = CosTable([(0,0), (64,1), (1024,1), (4096, 0.5), (8191,0)])
    >>> mml = MML(a, voices=2, loop=True, poly=4).play()
    >>> dur = Sig(mml.getVoice(0, "dur"), mul=2)
    >>> tr = TrigEnv(mml.getVoice(0), table=t, dur=dur, mul=mml.getVoice(0, "amp"))
    >>> a = SineLoop(freq=mml.getVoice(0, "freq"), feedback=mml.getVoice(0, "x"), mul=tr).mix()
    >>> dur2 = Sig(mml.getVoice(1, "dur"), mul=2)
    >>> tr2 = TrigEnv(mml.getVoice(1), table=t, dur=dur2, mul=mml.getVoice(1, "amp"))
    >>> a2 = LFO(freq=mml.getVoice(1, "freq"), sharp=mml.getVoice(1, "x"), type=2, mul=tr2).mix()
    >>> output = STRev([a, a2], inpos=[0.2, 0.8], bal=0.2, mul=1.5).out()

    """

    def __init__(self, music, voices=1, loop=False, poly=1, updateAtEnd=False):
        pyoArgsAssert(self, "SIBIB", music, voices, loop, poly, updateAtEnd)
        PyoObject.__init__(self)
        self._editor = None
        self._pitches = pitches = {
            0: "c",
            1: "c+",
            2: "d",
            3: "e-",
            4: "e",
            5: "f",
            6: "f+",
            7: "g",
            8: "a-",
            9: "a",
            10: "b-",
            11: "b",
        }
        self._fre_dummy = []
        self._amp_dummy = []
        self._dur_dummy = []
        self._end_dummy = []
        self._x_dummy = []
        self._y_dummy = []
        self._z_dummy = []
        self._music = music
        self._voices = voices
        self._loop = loop
        self._poly = poly
        self._updateAtEnd = updateAtEnd
        if os.path.isfile(music):
            with open(music, "r") as f:
                music = f.read()
        self.parser = MMLParser(music, voices)
        self._sequences = self.parser.getSequences()
        self._base_players = [MMLMain_base(loop, poly, updateAtEnd) for i in range(voices)]
        for i in range(voices):
            if self._sequences[i] is not None:
                self._base_players[i].setSequence(self._sequences[i])
        self._base_objs = [MML_base(wrap(self._base_players, j), i) for j in range(voices) for i in range(poly)]
        self._fre_objs = [
            MMLFreqStream_base(wrap(self._base_players, j), i) for j in range(voices) for i in range(poly)
        ]
        self._amp_objs = [MMLAmpStream_base(wrap(self._base_players, j), i) for j in range(voices) for i in range(poly)]
        self._dur_objs = [MMLDurStream_base(wrap(self._base_players, j), i) for j in range(voices) for i in range(poly)]
        self._end_objs = [MMLEndStream_base(wrap(self._base_players, j), i) for j in range(voices) for i in range(poly)]
        self._x_objs = [MMLXStream_base(wrap(self._base_players, j), i) for j in range(voices) for i in range(poly)]
        self._y_objs = [MMLYStream_base(wrap(self._base_players, j), i) for j in range(voices) for i in range(poly)]
        self._z_objs = [MMLZStream_base(wrap(self._base_players, j), i) for j in range(voices) for i in range(poly)]

    def __getitem__(self, i):
        if i == "freq":
            self._fre_dummy.append(Dummy([obj for obj in self._fre_objs]))
            return self._fre_dummy[-1]
        if i == "amp":
            self._amp_dummy.append(Dummy([obj for obj in self._amp_objs]))
            return self._amp_dummy[-1]
        if i == "dur":
            self._dur_dummy.append(Dummy([obj for obj in self._dur_objs]))
            return self._dur_dummy[-1]
        if i == "end":
            self._end_dummy.append(Dummy([obj for obj in self._end_objs]))
            return self._end_dummy[-1]
        if i == "x":
            self._x_dummy.append(Dummy([obj for obj in self._x_objs]))
            return self._x_dummy[-1]
        if i == "y":
            self._y_dummy.append(Dummy([obj for obj in self._y_objs]))
            return self._y_dummy[-1]
        if i == "z":
            self._z_dummy.append(Dummy([obj for obj in self._z_objs]))
            return self._z_dummy[-1]
        if type(i) == slice:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print("'i' too large!")

    def getVoice(self, voice, stream=None):
        sl = slice(voice * self._poly, (voice + 1) * self._poly)
        if stream is None:
            return self[sl]
        else:
            if stream in ["freq", "amp", "dur", "end", "x", "y", "z"]:
                return self[stream][sl]
            else:
                print("MML has no stream named %s!" % stream)
                return None

    def get(self, identifier="amp", all=False):
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        "freq", "amp", "dur", "end", "x", "y" or "z" can be given
        to `identifier` to retrieve a specific stream to get the
        value from.

        :Args:

            identifier: string {"freq", "amp", "dur", "end", "x", "y", "z"}
                Address string parameter identifying audio stream.
                Defaults to "amp".
            all: boolean, optional
                If True, the first value of each object's stream
                will be returned as a list.

                If False, only the value of the first object's
                stream will be returned as a float.

        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def setMusic(self, x):
        """
        Replace the `music` attribute.

        :Args:

            x: string
                The new music code to parse. If the string is a valid path
                to a text file, the file is opened and its content is taken
                as the music code.

        """
        pyoArgsAssert(self, "S", x)
        self._music = x
        if os.path.isfile(x):
            with open(x, "r") as f:
                x = f.read()
        if self._editor is not None:
            self._editor.update(x)
        self.parser.setText(x)
        self._sequences = self.parser.getSequences()
        for i in range(self._voices):
            if i == len(self._sequences):
                return
            if self._sequences[i] is not None:
                self._base_players[i].update(self._sequences[i])

    def getSequences(self):
        """
        Returns the sequences parsed from the music text.

        """
        return self._sequences

    def getNoteFromPitch(self, x):
        """
        Converts a MIDI note to MML notation and returns the result as a string.

        :Args:

            x: int
                The MIDI note to convert to MML notation. This will return two tokens,
                the octave followed by the note name.

        """
        oct = int(x / 12)
        pit = self._pitches[x % 12]
        return "o%d %s" % (oct, pit)

    def getVolumeFromVelocity(self, x):
        """
        Converts a MIDI velocity to MML volume notation and returns the result as a string.

        :Args:

            x: int
                The MIDI velocity to convert to MML volume notation. This will return a
                volume token.

        """
        vol = int(x / 127)
        return "v%d" % vol

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._fre_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._fre_objs)]
        self._amp_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._amp_objs)]
        self._dur_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._dur_objs)]
        self._end_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._end_objs)]
        self._x_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._x_objs)]
        self._y_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._y_objs)]
        self._z_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._z_objs)]
        return PyoObject.play(self, dur, delay)

    def stop(self, wait=0):
        [obj.stop(wait) for obj in self._fre_objs]
        [obj.stop(wait) for obj in self._amp_objs]
        [obj.stop(wait) for obj in self._dur_objs]
        [obj.stop(wait) for obj in self._end_objs]
        [obj.stop(wait) for obj in self._x_objs]
        [obj.stop(wait) for obj in self._y_objs]
        [obj.stop(wait) for obj in self._z_objs]
        return PyoObject.stop(self, wait)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    def editor(self, title="MML Editor", wxnoserver=False):
        """
        Opens the text editor for this object.

        :Args:

            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createMMLEditorWindow(self, title, wxnoserver)

    @property
    def music(self):
        """string. The music code to parse."""
        return self._music

    @music.setter
    def music(self, x):
        self.setMusic(x)
