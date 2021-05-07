# Copyright 2009-2021 Olivier Belanger
# 
# This file is part of pyo, a python module to help digital signal
# processing script creation.
#
# pyo is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# pyo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with pyo.  If not, see <http://www.gnu.org/licenses/>.

import os, sys
from ._core import *

class Expr(PyoObject):

    def __init__(self, input, expr="", outs=1, initout=0.0, mul=1, add=0):
        pyoArgsAssert(self, "osIFOO", input, expr, outs, initout, mul, add)
        PyoObject.__init__(self, mul, add)
        self._editor = None
        self._input = input
        if isinstance(input, list):
            input_objs = [obj for pyoObj in input for obj in pyoObj.getBaseObjects()]
        else:
            input_objs = input.getBaseObjects()
        self._expr = expr
        self._outs = outs
        self._initout = initout
        expr = self._preproc(expr)
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_players = [Exprer_base(input_objs, expr, outs, initout)]
        self._base_objs = []
        for j in range(outs):
            self._base_objs.append(Expr_base(self._base_players[0], j, wrap(mul, j), wrap(add, j)))
        self._init_play()

    def setExpr(self, x):
        pyoArgsAssert(self, "s", x)
        self._expr = x
        if self._editor is not None:
            self._editor.update(x)
        x = self._preproc(x)
        x, lmax = convertArgsToLists(x)
        [obj.setExpr(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def printNodes(self):
        [obj.printNodes() for i, obj in enumerate(self._base_objs)]

    def setVar(self, varname, value):
        pyoArgsAssert(self, "sn", varname, value)
        varname, value, lmax = convertArgsToLists(varname, value)
        [obj.setVar(wrap(varname, j), wrap(value, j)) for i, obj in enumerate(self._base_players) for j in range(lmax)]

    def _get_matching_bracket_pos(self, x, p1):
        count = 1
        p2 = p1 + 1
        while True:
            if x[p2] == "(":
                count += 1
            elif x[p2] == ")":
                count -= 1
            if count == 0:
                break
            p2 += 1
            if p2 == len(x):
                break
        if count != 0:
            return -1
        else:
            return p2

    def _replace(self, x, lst):
        lst.reverse()
        for key, body in lst:
            # find how many args waiting in function's body
            numargs = 0
            doll = body.find("$")
            while doll != -1:
                arg = int(body[doll + 1])
                if arg > numargs:
                    numargs = arg
                doll = body.find("$", doll + 1)
            occurences = 0
            pos = x.find(key)
            while pos != -1:
                if x[pos - 1] in " \t\n()" and x[pos + len(key)] in " \t\n()":
                    # replace "#vars" with unique symbol
                    body2 = body.replace("-%s" % key, ".%s.%d" % (key, occurences))
                    occurences += 1
                    # find limits
                    is_inside_brackets = True
                    start = pos - 1
                    while x[start] != "(":
                        start -= 1
                        if start < 0:
                            start = 0
                            is_inside_brackets = False
                            break
                    if is_inside_brackets:
                        end = self._get_matching_bracket_pos(x, start)
                    else:
                        end = len(x)
                    # retrieve args #
                    # function arguments are computed only once at the beginning of the
                    # function. This fixes a bug wen an argument *must* be of the same
                    # value for every $x and the arg expression contains random function.
                    argvars = ""
                    howmany = 0
                    p1 = pos + len(key)
                    p2 = -1
                    for i in range(numargs):
                        while x[p1] in " \t\n":
                            p1 += 1
                        if x[p1] == "(":
                            p2 = self._get_matching_bracket_pos(x, p1)
                            if p2 == -1 or p2 >= end:
                                raise Exception("Mismatched brackets in function arguments.")
                            p2 += 1
                        else:
                            p2 = p1 + 1
                            if p2 < end:
                                while x[p2] not in " \t\n()":
                                    p2 += 1
                                    if p2 == end:
                                        break
                        if x[p1:p2] != ")":
                            # pattern for an arg is #FUNCNAMEargARGNUMBER.
                            argvars = argvars + "(let #%sarg%d %s)\n" % (key, i + 1, x[p1:p2])
                            howmany += 1
                        if p2 == end:
                            break
                        else:
                            p1 = p2
                    if howmany > 0:
                        body2 = body2[0] + "\n" + argvars + body2[1:]

                    # discard extra args
                    if p2 != end and p2 != -1:
                        x = x[:p2] + x[end:]

                    # replace args
                    if howmany > 0:
                        newbody = body2
                        for i in range(numargs):
                            if i < howmany:
                                arg = "#%sarg%d" % (key, (i + 1))
                            else:
                                arg = "0.0"
                            newbody = newbody.replace("$%d" % (i + 1), arg)
                        x = x[:pos] + newbody + x[p2:]
                    else:
                        x = x[:pos] + body2 + x[pos + len(key) :]
                pos = x.find(key, pos + 1)
        return x

    def _change_var_names(self, funcname, funcbody):
        d = {}
        letpos = funcbody.find("let ")
        while letpos != -1:
            pos = funcbody.find("#", letpos)
            if pos == -1:
                raise Exception("No #var defined inside a let function.")
            p1 = pos
            p2 = p1 + 1
            while funcbody[p2] not in " \t\n()":
                p2 += 1
            label = funcbody[p1:p2]
            d[label] = label + "-" + funcname
            letpos = funcbody.find("let ", letpos + 4)
        for label, newlabel in d.items():
            funcbody = funcbody.replace(label, newlabel)
        return funcbody

    def _preproc(self, x):
        # replace load functions with file body
        while "(load" in x:
            p1 = x.find("(load")
            p2 = self._get_matching_bracket_pos(x, p1)
            p2 += 1
            text = x[p1:p2]
            path = text.replace("(load", "").replace(")", "").strip()
            if os.path.isfile(path):
                with open(path, "r") as f:
                    text = f.read()
            x = x[:p1] + text + x[p2:]

        # remove comments
        while "//" in x:
            start = x.find("//")
            end = x.find("\n", start)
            x = x[:start] + x[end:]

        # make sure there are braces around constant keywords
        constants = ["pi", "twopi", "e", "sr"]
        for constant in constants:
            p1 = x.find(constant)
            while p1 != -1:
                if x[p1 - 1] == " ":
                    x = x[:p1] + "(" + constant + ")" + x[p1 + len(constant) :]
                p1 = x.find(constant, p1 + len(constant) + 2)

        # expand defined functions
        _defined = []
        while "define" in x:
            start = x.find("(define")
            p1 = start + 7
            # get function name
            while x[p1] in " \t\n":
                p1 += 1
            p2 = p1 + 1
            while x[p2] not in " \t\n":
                p2 += 1
            funcname = x[p1:p2]
            # get function body
            p1 = p2 + 1
            while x[p1] != "(":
                p1 += 1
            p2 = self._get_matching_bracket_pos(x, p1)
            if p2 == -1:
                raise Exception("Mismatched brackets in function body.")
            p2 += 1
            funcbody = x[p1:p2]
            # get end of the definition
            while x[p2] in " \t\n":
                p2 += 1
            if x[p2] != ")":
                raise Exception("Missing ending bracket in function definition.")
            stop = p2
            # save in dictionary and remove definition from the string
            funcbody = self._change_var_names(funcname, funcbody)
            _defined.append([funcname, funcbody])
            x = x[:start] + x[stop + 1 :]
        # replace calls to function with their function body
        x = self._replace(x, _defined)
        x = x.strip()
        return x

    @property
    def expr(self):
        return self._expr

    @expr.setter
    def expr(self, x):
        self.setExpr(x)
