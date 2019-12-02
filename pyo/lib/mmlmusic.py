"""
Set of objects to manage triggers streams.

A trigger is an audio signal with a value of 1 surrounded by 0s.

TrigXXX objects use this kind of signal to generate different
processes with sampling rate time accuracy.

"""
from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

"""
Copyright 2009-2015 Olivier Belanger

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

### MML framework ###
#####################

VALID_NOTES = "abcdefgr?"
VALID_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?"

class MMLParser:
    def __init__(self, text, voices=1):
        self.text = text
        self.voices = voices

    def _remove_comments(self, text):
        pos = text.find(";")
        while pos != -1:
            pos2 = text.find("\n", pos+1)
            if pos2 == -1:
                text = text[:pos]
            else:
                text = text[:pos] + text[pos2+1:]
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
                if i == len(text)-1 and text[start:]:
                    l.append(text[start:].strip())
            elif i == len(text)-1:
                if text[start:]:
                    l.append(text[start:].strip())
            elif rescan and text[i] != " ":
                rescan = False
                start = i
        return l

    def _expand_tuplets(self, text):
        pos = text.find("(")
        while pos != -1:
            nextpos = text.find("(", pos+1)
            pos2 = text.find(")", pos+1)
            if pos2 == -1:
                # missing end brace, just ignore the tuplet.
                text = text[:pos] + text[pos+1:]
            elif nextpos != -1 and nextpos < pos2:
                # missing end brace, just ignore the tuplet.
                text = text[:pos] + text[pos+1:]
            else:
                duration = int(text[pos2+1])
                tmp_eles = self._split_tuplets(text[pos+1:pos2])
                elements = []
                for ele in tmp_eles:
                    if ele[0] in VALID_NOTES:
                        elements.append("%s%i" % (ele, duration))
                    else:
                        elements.append(ele)
                num_eles = len([e for e in elements if e[0] in VALID_NOTES])
                ele_text = " ".join(elements)
                ele_div = " /%i " % num_eles
                text = text[:pos] + ele_div + ele_text + " /1 " + text[pos2+2:]
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
            for macro in macros:
                pos = new.find(macro)
                while pos != -1:
                    if new[pos-1] not in VALID_CHARS and \
                       new[pos+len(macro)] not in VALID_CHARS:
                        new = new[:pos] + macros[macro] + new[pos+len(macro):]
                    pos = new.find(macro, pos+len(macro))

        return new

    def _remove_extra_spaces(self, text):
        while "  " in text:
            text = text.replace("  ", " ")
        return text

    def _add_space_around_loops(self, text):
        pos = text.find("|:")
        while pos != -1:
            if text[pos+2] != " ":
                text = text[:pos+2] + " " + text[pos+2:]
            if text[pos-1] != " ":
                text = text[:pos] + " " + text[pos:]
            pos = text.find("|:", pos+3)
        pos = text.find(":|")
        while pos != -1:
            if text[pos-1] != " ":
                text = text[:pos] + " " + text[pos:]
            pos = text.find(":|", pos+3)
        return text

    def _process_specific_group(self, text, inchar, outchar):
        pos = text.find(inchar)
        while pos != -1:
            nextpos = text.find(inchar, pos+1)
            pos2 = text.find(outchar, pos+1)
            if pos2 == -1:
                raise Exception("Missing %s symbol..." % outchar)
            elif nextpos != -1 and nextpos < pos2:
                raise Exception("Missing %s symbol..." % outchar)
            else:
                format = text[pos:pos2+1].replace(" ", ",")
                format = format.replace(" ", "")
                text = text[:pos] + format + text[pos2+1:]
            pos = text.find(inchar, pos2+1)
        return text

    def _process_groups(self, text):
        text = self._process_specific_group(text, "[", "]")
        text = self._process_specific_group(text, "{", "}")
        return text

    def _expand_line_continuation(self, text):
        text = text.replace("\r", "")
        pos = text.find("\\")
        while pos != -1:
            pos2 = text.find("\n", pos+1)
            text = text[:pos] + text[pos2+1:]
            pos = text.find("\\", pos+1)
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
    Generates algorithmic trigger patterns.

    :Parent: :py:class:`PyoObject`

    :Args:

        poly: int, optional
            Beat polyphony. Denotes how many independent streams are
            generated by the object, allowing overlapping processes.

            Available only at initialization. Defaults to 1.

    .. note::

        Beat outputs many signals identified with a string between brackets:

        |  obj['tap'] returns audio stream of the current tap of the measure.
        |  obj['amp'] returns audio stream of the current beat amplitude.
        |  obj['dur'] returns audio stream of the current beat duration in seconds.
        |  obj['end'] returns audio stream with a trigger just before the end of the measure.

        obj without brackets returns the generated trigger stream of the measure.

        The out() method is bypassed. Beat's signal can not be sent to audio outs.

        Beat has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = CosTable([(0,0), (100,1), (500,.3), (8191,0)])
    >>> beat = Beat(time=.125, taps=16, w1=[90,80], w2=50, w3=35, poly=1).play()
    >>> trmid = TrigXnoiseMidi(beat, dist=12, mrange=(60, 96))
    >>> trhz = Snap(trmid, choice=[0,2,3,5,7,8,10], scale=1)
    >>> tr2 = TrigEnv(beat, table=t, dur=beat['dur'], mul=beat['amp'])
    >>> a = Sine(freq=trhz, mul=tr2*0.3).out()

    """
    def __init__(self, music, voices=1, loop=False, poly=1, updateAtEnd=False):
        pyoArgsAssert(self, "SIBIB", music, voices, loop, poly, updateAtEnd)
        PyoObject.__init__(self)
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
        sequences = self.parser.getSequences()
        self._base_players = [MMLMain_base(loop, poly, updateAtEnd) for i in range(voices)]
        for i in range(voices):
            if sequences[i] is not None:
                self._base_players[i].setSequence(sequences[i])
        self._base_objs = [MML_base(wrap(self._base_players,j), i) for j in range(voices) for i in range(poly)]
        self._fre_objs = [MMLFreqStream_base(wrap(self._base_players,j), i) for j in range(voices) for i in range(poly)]
        self._amp_objs = [MMLAmpStream_base(wrap(self._base_players,j), i) for j in range(voices) for i in range(poly)]
        self._dur_objs = [MMLDurStream_base(wrap(self._base_players,j), i) for j in range(voices) for i in range(poly)]
        self._end_objs = [MMLEndStream_base(wrap(self._base_players,j), i) for j in range(voices) for i in range(poly)]
        self._x_objs = [MMLXStream_base(wrap(self._base_players,j), i) for j in range(voices) for i in range(poly)]
        self._y_objs = [MMLYStream_base(wrap(self._base_players,j), i) for j in range(voices) for i in range(poly)]
        self._z_objs = [MMLZStream_base(wrap(self._base_players,j), i) for j in range(voices) for i in range(poly)]

    def __getitem__(self, i):
        if i == 'freq':
            self._fre_dummy.append(Dummy([obj for obj in self._fre_objs]))
            return self._fre_dummy[-1]
        if i == 'amp':
            self._amp_dummy.append(Dummy([obj for obj in self._amp_objs]))
            return self._amp_dummy[-1]
        if i == 'dur':
            self._dur_dummy.append(Dummy([obj for obj in self._dur_objs]))
            return self._dur_dummy[-1]
        if i == 'end':
            self._end_dummy.append(Dummy([obj for obj in self._end_objs]))
            return self._end_dummy[-1]
        if i == 'x':
            self._x_dummy.append(Dummy([obj for obj in self._x_objs]))
            return self._x_dummy[-1]
        if i == 'y':
            self._y_dummy.append(Dummy([obj for obj in self._y_objs]))
            return self._y_dummy[-1]
        if i == 'z':
            self._z_dummy.append(Dummy([obj for obj in self._z_objs]))
            return self._z_dummy[-1]
        if type(i) == slice:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print("'i' too large!")

    def get(self, identifier="amp", all=False):
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        "tap", "amp" or "dur" must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier: string {"fre", "amp", "dur"}
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
        Replace the `time` attribute.

        :Args:

            x: float or PyoObject
                New `time` attribute.

        """
        pyoArgsAssert(self, "S", x)
        self._music = x
        self.parser.setText(self._music)
        sequences = self.parser.getSequences()
        for i in range(self._voices):
            if i == len(sequences):
                return
            if sequences[i] is not None:
                self._base_players[i].update(sequences[i])

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._fre_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._fre_objs)]
        self._amp_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._amp_objs)]
        self._dur_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._dur_objs)]
        self._end_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._end_objs)]
        self._x_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._x_objs)]
        self._y_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._y_objs)]
        self._z_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._z_objs)]
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

    @property
    def music(self):
        """string. ..."""
        return self._music
    @music.setter
    def music(self, x): self.setMusic(x)

