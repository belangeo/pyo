/**************************************************************************
 * Copyright 2009-2015 Olivier Belanger                                   *
 *                                                                        *
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *
 *                                                                        *
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU Lesser General Public       *
 * License along with pyo.  If not, see <http://www.gnu.org/licenses/>.   *
 *************************************************************************/

#include <Python.h>
#include "py2to3.h"
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

// arithmetic operators
#define OP_ADD 0
#define OP_SUB 1
#define OP_MUL 2
#define OP_DIV 3
#define OP_EXP 4
#define OP_MOD 5
#define OP_NEG 6

// moving phase operators
#define OP_INC 7
#define OP_DEC 8
#define OP_PHS 9

// trigonometric functions
#define OP_SIN 10
#define OP_COS 11
#define OP_TAN 12
#define OP_TANH 13
#define OP_ATAN 14
#define OP_ATAN2 15

// conditional operators
#define OP_LT 40
#define OP_LE 41
#define OP_GT 42
#define OP_GE 43
#define OP_EQ 44
#define OP_NE 45
#define OP_IF 46
#define OP_AND 47
#define OP_OR 48

// power and logarithmic functions
#define OP_SQRT 50
#define OP_LOG 51
#define OP_LOG2 52
#define OP_LOG10 53
#define OP_POW 54
#define OP_FABS 55
#define OP_FLOOR 56
#define OP_CEIL 57
#define OP_EEXP 58
#define OP_ROUND 59
#define OP_MIN 60
#define OP_MAX 61
#define OP_WRAP 62

// filters
#define OP_RPOLE 70
#define OP_RZERO 71
#define OP_DELAY 72
#define OP_CPOLE 73
#define OP_CZERO 74

// random functions
#define OP_RANDF 80
#define OP_RANDI 81

// sample-and-hold functions
#define OP_SAH 90

// math constants
#define OP_CONST 99
#define OP_PI 100
#define OP_TWOPI 101
#define OP_E 102
#define OP_SR 103

// complex number
#define OP_COMPLEX 120
#define OP_REAL 121
#define OP_IMAG 122

typedef struct t_expr {
    int type_op;
    int num;
    int *nodes;
    int *vars;
    int *input;
    int *output;
    MYFLT *values;
    MYFLT *previous;
    MYFLT result;
    MYFLT result2;
} expr;

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *variables;
    int count;
    MYFLT oneOverSr;
    MYFLT *input_buffer;
    MYFLT *output_buffer;
    expr lexp[1024];
    int modebuffer[2];
} Expr;

void 
clearexpr(expr ex)
{
    if (ex.nodes) { free(ex.nodes); }
    if (ex.vars) { free(ex.vars); }
    if (ex.input) { free(ex.input); }
    if (ex.output) { free(ex.output); }
    if (ex.values) { free(ex.values); }
    if (ex.previous) { free(ex.previous); }
}

expr
initexpr(const char *op, int size)
{
    expr ex;
    int i, value = -1, num = 0;
    if      (strcmp(op, "+") == 0) { value = OP_ADD; num = 2; }
    else if (strcmp(op, "-") == 0) { value = OP_SUB; num = 2; }
    else if (strcmp(op, "*") == 0) {value = OP_MUL; num = 2; }
    else if (strcmp(op, "/") == 0) {value = OP_DIV; num = 2; }
    else if (strcmp(op, "^") == 0) {value = OP_EXP; num = 2; }
    else if (strcmp(op, "%") == 0) {value = OP_MOD; num = 2; }
    else if (strcmp(op, "neg") == 0) {value = OP_NEG; num = 1; }
    else if (strcmp(op, "++") == 0) {value = OP_INC; num = 2; }
    else if (strcmp(op, "--") == 0) {value = OP_DEC; num = 2; }
    else if (strcmp(op, "~") == 0) {value = OP_PHS; num = 2; }
    else if (strcmp(op, "sin") == 0) {value = OP_SIN; num = 1; }
    else if (strcmp(op, "cos") == 0) {value = OP_COS; num = 1; }
    else if (strcmp(op, "tan") == 0) {value = OP_TAN; num = 1; }
    else if (strcmp(op, "tanh") == 0) {value = OP_TANH; num = 1; }
    else if (strcmp(op, "atan") == 0) {value = OP_ATAN; num = 1; }
    else if (strcmp(op, "atan2") == 0) {value = OP_ATAN2; num = 2; }
    else if (strcmp(op, "<") == 0) {value = OP_LT; num = 2; }
    else if (strcmp(op, "<=") == 0) {value = OP_LE; num = 2; }
    else if (strcmp(op, ">") == 0) {value = OP_GT; num = 2; }
    else if (strcmp(op, ">=") == 0) {value = OP_GE; num = 2; }
    else if (strcmp(op, "==") == 0) {value = OP_EQ; num = 2; }
    else if (strcmp(op, "!=") == 0) {value = OP_NE; num = 2; }
    else if (strcmp(op, "if") == 0) {value = OP_IF; num = 3; }
    else if (strcmp(op, "and") == 0) {value = OP_AND; num = 2; }
    else if (strcmp(op, "or") == 0) {value = OP_OR; num = 2; }
    else if (strcmp(op, "sqrt") == 0) {value = OP_SQRT; num = 1; }
    else if (strcmp(op, "log") == 0) {value = OP_LOG; num = 1; }
    else if (strcmp(op, "log2") == 0) {value = OP_LOG2; num = 1; }
    else if (strcmp(op, "log10") == 0) {value = OP_LOG10; num = 1; }
    else if (strcmp(op, "pow") == 0) {value = OP_POW; num = 2; }
    else if (strcmp(op, "abs") == 0) {value = OP_FABS; num = 1; }
    else if (strcmp(op, "floor") == 0) {value = OP_FLOOR; num = 1; }
    else if (strcmp(op, "ceil") == 0) {value = OP_CEIL; num = 1; }
    else if (strcmp(op, "exp") == 0) {value = OP_EEXP; num = 1; }
    else if (strcmp(op, "round") == 0) {value = OP_ROUND; num = 1; }
    else if (strcmp(op, "min") == 0) {value = OP_MIN; num = 2; }
    else if (strcmp(op, "max") == 0) {value = OP_MAX; num = 2; }
    else if (strcmp(op, "wrap") == 0) {value = OP_WRAP; num = 1; }
    else if (strcmp(op, "randf") == 0) {value = OP_RANDF; num = 2; }
    else if (strcmp(op, "randi") == 0) {value = OP_RANDI; num = 2; }
    else if (strcmp(op, "sah") == 0) {value = OP_SAH; num = 2; } // stream, trigger
    else if (strcmp(op, "rpole") == 0) {value = OP_RPOLE; num = 2; }
    else if (strcmp(op, "rzero") == 0) {value = OP_RZERO; num = 2; }
    else if (strcmp(op, "delay") == 0) {value = OP_DELAY; num = 1; }
    else if (strcmp(op, "cpole") == 0) {value = OP_CPOLE; num = 2; }
    else if (strcmp(op, "czero") == 0) {value = OP_CZERO; num = 2; }

    else if (strcmp(op, "complex") == 0) {value = OP_COMPLEX; num = 2; }
    else if (strcmp(op, "real") == 0) {value = OP_REAL; num = 1; }
    else if (strcmp(op, "imag") == 0) {value = OP_IMAG; num = 1; }

    else if (strcmp(op, "const") == 0) {value = OP_CONST; num = 1; }
    else if (strcmp(op, "pi") == 0) {value = OP_PI; num = 0; }
    else if (strcmp(op, "twopi") == 0) {value = OP_TWOPI; num = 0; }
    else if (strcmp(op, "e") == 0) {value = OP_E; num = 0; }
    else if (strcmp(op, "sr") == 0) {value = OP_SR; num = 0; }
    else if (size == 1) {value = OP_CONST; num = 1; }
    ex.type_op = value;
    ex.num = num;
    ex.nodes = (int *)malloc(num);
    ex.vars = (int *)malloc(num);
    ex.input = (int *)malloc(num);
    ex.output = (int *)malloc(num);
    ex.values = (MYFLT *)malloc(num);
    ex.previous = (MYFLT *)malloc(num);
    for (i=0; i<num; i++) {
        ex.nodes[i] = ex.vars[i] = -1;
        ex.input[i] = ex.output[i] = 1;
        ex.values[i] = ex.previous[i] = 0.0;
    }
    ex.result = 0.0;
    ex.result2 = 0.0;
    return ex;
}

void
print_expr(expr ex, int node)
{
    int i;
    PySys_WriteStdout("=== Node # %d ===\n", node);
    PySys_WriteStdout("Operator: %d\nNodes: ", ex.type_op);
    for (i=0; i<ex.num; i++) { PySys_WriteStdout("%d, ", ex.nodes[i]); }
    PySys_WriteStdout("\nVars: ");
    for (i=0; i<ex.num; i++) { PySys_WriteStdout("%d, ", ex.vars[i]); }
    PySys_WriteStdout("\nInputs: ");
    for (i=0; i<ex.num; i++) { PySys_WriteStdout("%d, ", ex.input[i]); }
    PySys_WriteStdout("\nOutputs: ");
    for (i=0; i<ex.num; i++) { PySys_WriteStdout("%d, ", ex.output[i]); }
    PySys_WriteStdout("\nValues: ");
    for (i=0; i<ex.num; i++) { PySys_WriteStdout("%f, ", ex.values[i]); }
    PySys_WriteStdout("\n\n");
}

static void
Expr_process(Expr *self) {
    int i, j, k, pos = 0;
    MYFLT tmp = 0.0, result = 0.0;
    MYFLT nextre = 0.0, lastre = 0.0, nextim = 0.0, lastim = 0.0, coefre = 0.0, coefim = 0.0; 
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->count == 0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = in[i];
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            self->input_buffer[i] = in[i];
            for (j=0; j<self->count; j++) {
                for (k=0; k<self->lexp[j].num; k++) {
                    if (self->lexp[j].nodes[k] != -1) {
                        self->lexp[j].values[k] = self->lexp[self->lexp[j].nodes[k]].result;
                    }
                    else if (self->lexp[j].vars[k] != -1) {
                        self->lexp[j].values[k] = self->lexp[self->lexp[j].vars[k]].result;
                    }
                    else if (self->lexp[j].input[k] < 1) {
                        pos = i + self->lexp[j].input[k];
                        if (pos < 0)
                            pos += self->bufsize;
                        self->lexp[j].values[k] = self->input_buffer[pos];
                    }
                    else if (self->lexp[j].output[k] < 0) {
                        pos = i + self->lexp[j].output[k];
                        if (pos < 0)
                            pos += self->bufsize;
                        self->lexp[j].values[k] = self->output_buffer[pos];
                    }
                }
                switch (self->lexp[j].type_op) {
                    case OP_ADD:
                        self->lexp[j].result = self->lexp[j].values[0] + self->lexp[j].values[1];
                        break;
                    case OP_SUB:
                        self->lexp[j].result = self->lexp[j].values[0] - self->lexp[j].values[1];
                        break;
                    case OP_MUL:
                        self->lexp[j].result = self->lexp[j].values[0] * self->lexp[j].values[1];
                        break;
                    case OP_DIV:
                        self->lexp[j].result = self->lexp[j].values[0] / self->lexp[j].values[1];
                        break;
                    case OP_EXP:
                        self->lexp[j].result = MYPOW(self->lexp[j].values[0], self->lexp[j].values[1]);
                        break;
                    case OP_MOD:
                        self->lexp[j].result = MYFMOD(self->lexp[j].values[0], self->lexp[j].values[1]);
                        break;
                    case OP_NEG:
                        self->lexp[j].result = -self->lexp[j].values[0];
                        break;
                    case OP_INC:
                        self->lexp[j].result = self->lexp[j].previous[0];
                        self->lexp[j].previous[0] = MYFMOD(self->lexp[j].previous[0] + self->lexp[j].values[0], self->lexp[j].values[1]);
                        break;
                    case OP_DEC:
                        self->lexp[j].result -= self->lexp[j].values[0];
                        if (self->lexp[j].result < 0) { self->lexp[j].result += self->lexp[j].values[1]; }
                        break;
                    case OP_PHS:
                        tmp = self->lexp[j].previous[0] + self->lexp[j].values[1];
                        if (tmp >= 1) { tmp -= 1.0; }
                        self->lexp[j].result = tmp;
                        self->lexp[j].previous[0] += (self->lexp[j].values[0] * self->oneOverSr);
                        if (self->lexp[j].previous[0] >= 1) { self->lexp[j].previous[0] -= 1.0; }
                        break;
                    case OP_SIN:
                        self->lexp[j].result = MYSIN(self->lexp[j].values[0]);
                        break;
                    case OP_COS:
                        self->lexp[j].result = MYCOS(self->lexp[j].values[0]);
                        break;
                    case OP_TAN:
                        self->lexp[j].result = MYTAN(self->lexp[j].values[0]);
                        break;
                    case OP_TANH:
                        self->lexp[j].result = MYTANH(self->lexp[j].values[0]);
                        break;
                    case OP_ATAN:
                        self->lexp[j].result = MYATAN(self->lexp[j].values[0]);
                        break;
                    case OP_ATAN2:
                        self->lexp[j].result = MYATAN2(self->lexp[j].values[0], self->lexp[j].values[1]);
                        break;
                    case OP_LT:
                        self->lexp[j].result = self->lexp[j].values[0] < self->lexp[j].values[1] ? 1.0 : 0.0;
                        break;
                    case OP_LE:
                        self->lexp[j].result = self->lexp[j].values[0] <= self->lexp[j].values[1] ? 1.0 : 0.0;
                        break;
                    case OP_GT:
                        self->lexp[j].result = self->lexp[j].values[0] > self->lexp[j].values[1] ? 1.0 : 0.0;
                        break;
                    case OP_GE:
                        self->lexp[j].result = self->lexp[j].values[0] >= self->lexp[j].values[1] ? 1.0 : 0.0;
                        break;
                    case OP_EQ:
                        self->lexp[j].result = self->lexp[j].values[0] == self->lexp[j].values[1] ? 1.0 : 0.0;
                        break;
                    case OP_NE:
                        self->lexp[j].result = self->lexp[j].values[0] != self->lexp[j].values[1] ? 1.0 : 0.0;
                        break;
                    case OP_IF:
                        self->lexp[j].result = self->lexp[j].values[0] != 0 ? self->lexp[j].values[1] : self->lexp[j].values[2];
                        break;
                    case OP_AND:
                        self->lexp[j].result = self->lexp[j].values[0] != 0 && self->lexp[j].values[1] != 0 ? 1.0 : 0.0;
                        break;
                    case OP_OR:
                        self->lexp[j].result = self->lexp[j].values[0] != 0 || self->lexp[j].values[1] != 0 ? 1.0 : 0.0;
                        break;
                    case OP_SQRT:
                        self->lexp[j].result = MYSQRT(self->lexp[j].values[0]);
                        break;
                    case OP_LOG:
                        self->lexp[j].result = MYLOG(self->lexp[j].values[0]);
                        break;
                    case OP_LOG2:
                        self->lexp[j].result = MYLOG2(self->lexp[j].values[0]);
                        break;
                    case OP_LOG10:
                        self->lexp[j].result = MYLOG10(self->lexp[j].values[0]);
                        break;
                    case OP_POW:
                        self->lexp[j].result = MYPOW(self->lexp[j].values[0], self->lexp[j].values[1]);
                        break;
                    case OP_FABS:
                        self->lexp[j].result = MYFABS(self->lexp[j].values[0]);
                        break;
                    case OP_FLOOR:
                        self->lexp[j].result = MYFLOOR(self->lexp[j].values[0]);
                        break;
                    case OP_CEIL:
                        self->lexp[j].result = MYCEIL(self->lexp[j].values[0]);
                        break;
                    case OP_EEXP:
                        self->lexp[j].result = MYEXP(self->lexp[j].values[0]);
                        break;
                    case OP_ROUND:
                        self->lexp[j].result = MYROUND(self->lexp[j].values[0]);
                        break;
                    case OP_MIN:
                        self->lexp[j].result = self->lexp[j].values[0] < self->lexp[j].values[1] ? self->lexp[j].values[0] : self->lexp[j].values[1];
                        break;
                    case OP_MAX:
                        self->lexp[j].result = self->lexp[j].values[0] > self->lexp[j].values[1] ? self->lexp[j].values[0] : self->lexp[j].values[1];
                        break;
                    case OP_WRAP:
                        tmp = self->lexp[j].values[0];
                        while (tmp < 0.0) {tmp += 1.0; }
                        while (tmp >= 1.0) {tmp -= 1.0; }
                        self->lexp[j].result = tmp;
                        break;
                    case OP_RANDF:
                        self->lexp[j].result = RANDOM_UNIFORM * (self->lexp[j].values[1] - self->lexp[j].values[0]) + self->lexp[j].values[0];
                        break;
                    case OP_RANDI:
                        self->lexp[j].result = MYFLOOR(RANDOM_UNIFORM * (self->lexp[j].values[1] - self->lexp[j].values[0]) + self->lexp[j].values[0]);
                        break;
                    case OP_SAH:
                        self->lexp[j].result = self->lexp[j].values[1] < self->lexp[j].previous[1] ? self->lexp[j].values[0] : self->lexp[j].result;
                        self->lexp[j].previous[1] = self->lexp[j].values[1];
                        break;
                    case OP_RPOLE:
                        self->lexp[j].result = self->lexp[j].values[0] + self->lexp[j].result * self->lexp[j].values[1];
                        break;
                    case OP_RZERO:
                        self->lexp[j].result = self->lexp[j].values[0] - self->lexp[j].previous[0] * self->lexp[j].values[1];
                        self->lexp[j].previous[0] = self->lexp[j].values[0];
                        break;
                    case OP_DELAY:
                        self->lexp[j].result = self->lexp[j].previous[0];
                        self->lexp[j].previous[0] = self->lexp[j].values[0];
                        break;
                    case OP_CZERO:
                        if (self->lexp[j].nodes[0] != -1) {
                            nextre = self->lexp[self->lexp[j].nodes[0]].result;
                            nextim = self->lexp[self->lexp[j].nodes[0]].result2;
                        } else {
                            nextre = 0.0;
                            nextim = 0.0;
                        }
                        if (self->lexp[j].nodes[1] != -1) {
                            coefre = self->lexp[self->lexp[j].nodes[1]].result;
                            coefim = self->lexp[self->lexp[j].nodes[1]].result2;
                        } else {
                            coefre = 0.0;
                            coefim = 0.0;
                        }
                        lastre = self->lexp[j].previous[0];
                        lastim = self->lexp[j].previous[1];
                        self->lexp[j].result = nextre - lastre * coefre + lastim * coefim;
                        self->lexp[j].result2 = nextim - lastre * coefim - lastim * coefre;
                        self->lexp[j].previous[0] = nextre;
                        self->lexp[j].previous[1] = nextim;
                        break;
                    case OP_CPOLE:
                        if (self->lexp[j].nodes[0] != -1) {
                            nextre = self->lexp[self->lexp[j].nodes[0]].result;
                            nextim = self->lexp[self->lexp[j].nodes[0]].result2;
                        } else {
                            nextre = 0.0;
                            nextim = 0.0;
                        }
                        if (self->lexp[j].nodes[1] != -1) {
                            coefre = self->lexp[self->lexp[j].nodes[1]].result;
                            coefim = self->lexp[self->lexp[j].nodes[1]].result2;
                        } else {
                            coefre = 0.0;
                            coefim = 0.0;
                        }
                        lastre = self->lexp[j].previous[0];
                        lastim = self->lexp[j].previous[1];
                        tmp = self->lexp[j].result = nextre + lastre * coefre - lastim * coefim;
                        self->lexp[j].previous[1] = self->lexp[j].result2 = nextim + lastre * coefim + lastim * coefre;
                        self->lexp[j].previous[0] = tmp;
                        break;
                    case OP_COMPLEX:
                        self->lexp[j].result = self->lexp[j].values[0];
                        self->lexp[j].result2 = self->lexp[j].values[1];
                        break;
                    case OP_REAL:
                        self->lexp[j].result = self->lexp[self->lexp[j].nodes[0]].result;
                        break;
                    case OP_IMAG:
                        self->lexp[j].result = self->lexp[self->lexp[j].nodes[0]].result2;
                        break;
                    case OP_CONST:
                        self->lexp[j].result = self->lexp[j].values[0];
                        break;
                    case OP_PI:
                        self->lexp[j].result = PI;
                        break;
                    case OP_TWOPI:
                        self->lexp[j].result = TWOPI;
                        break;
                    case OP_E:
                        self->lexp[j].result = E;
                        break;
                    case OP_SR:
                        self->lexp[j].result = self->sr;
                        break;
                }
                result = self->lexp[j].result;
            }
            self->data[i] = self->output_buffer[i] = result;
        }
    }
}

static void Expr_postprocessing_ii(Expr *self) { POST_PROCESSING_II };
static void Expr_postprocessing_ai(Expr *self) { POST_PROCESSING_AI };
static void Expr_postprocessing_ia(Expr *self) { POST_PROCESSING_IA };
static void Expr_postprocessing_aa(Expr *self) { POST_PROCESSING_AA };
static void Expr_postprocessing_ireva(Expr *self) { POST_PROCESSING_IREVA };
static void Expr_postprocessing_areva(Expr *self) { POST_PROCESSING_AREVA };
static void Expr_postprocessing_revai(Expr *self) { POST_PROCESSING_REVAI };
static void Expr_postprocessing_revaa(Expr *self) { POST_PROCESSING_REVAA };
static void Expr_postprocessing_revareva(Expr *self) { POST_PROCESSING_REVAREVA };

static void
Expr_setProcMode(Expr *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    self->proc_func_ptr = Expr_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Expr_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Expr_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Expr_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Expr_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Expr_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Expr_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Expr_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Expr_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Expr_postprocessing_revareva;
            break;
    }
}

static void
Expr_compute_next_data_frame(Expr *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Expr_traverse(Expr *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->variables);
    return 0;
}

static int
Expr_clear(Expr *self)
{
    int i;
    pyo_CLEAR
    for (i=0; i<self->count; i++) { 
        clearexpr(self->lexp[i]); 
    }
    free(self->input_buffer);
    free(self->output_buffer);
    Py_CLEAR(self->input);
    Py_CLEAR(self->variables);
    return 0;
}

static void
Expr_dealloc(Expr* self)
{
    pyo_DEALLOC
    Expr_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Expr_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *exprtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Expr *self;
    self = (Expr *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Expr_compute_next_data_frame);
    self->mode_func_ptr = Expr_setProcMode;

    self->oneOverSr = 1.0 / self->sr;
    
    self->variables = PyDict_New();

    static char *kwlist[] = {"input", "expr", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &exprtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (exprtmp) {
        PyObject_CallMethod((PyObject *)self, "setExpr", "O", exprtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->input_buffer = (MYFLT *)realloc(self->input_buffer, self->bufsize * sizeof(MYFLT));
    self->output_buffer = (MYFLT *)realloc(self->output_buffer, self->bufsize * sizeof(MYFLT));
    for (i=0; i<self->bufsize; i++) {
        self->input_buffer[i] = self->output_buffer[i] = 0.0;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Expr_getServer(Expr* self) { GET_SERVER };
static PyObject * Expr_getStream(Expr* self) { GET_STREAM };
static PyObject * Expr_setMul(Expr *self, PyObject *arg) { SET_MUL };
static PyObject * Expr_setAdd(Expr *self, PyObject *arg) { SET_ADD };
static PyObject * Expr_setSub(Expr *self, PyObject *arg) { SET_SUB };
static PyObject * Expr_setDiv(Expr *self, PyObject *arg) { SET_DIV };

static PyObject * Expr_play(Expr *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Expr_out(Expr *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Expr_stop(Expr *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Expr_multiply(Expr *self, PyObject *arg) { MULTIPLY };
static PyObject * Expr_inplace_multiply(Expr *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Expr_add(Expr *self, PyObject *arg) { ADD };
static PyObject * Expr_inplace_add(Expr *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Expr_sub(Expr *self, PyObject *arg) { SUB };
static PyObject * Expr_inplace_sub(Expr *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Expr_div(Expr *self, PyObject *arg) { DIV };
static PyObject * Expr_inplace_div(Expr *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Expr_setExpr(Expr *self, PyObject *arg)
{
    int i, j = 0, count = 0;
    PyObject *sentence = NULL, *exp = NULL, *explist = NULL, *tmpstr = NULL;
    PyObject *varDict = NULL, *waitingDict = NULL, *waitingList = NULL;
    Py_ssize_t len, start, end;

    PyDict_Clear(self->variables);
    varDict = PyDict_New();
    waitingDict = PyDict_New();

    if (PY_STRING_CHECK(arg)) {
        Py_INCREF(arg);
        Py_XDECREF(sentence);
        sentence = arg;
        len = PyUnicode_GetSize(sentence);
        if (len == 0) {
            Py_RETURN_NONE;            
        }
        if (PyUnicode_Count(sentence, PyUnicode_FromString(")"), 0, len) != PyUnicode_Count(sentence, PyUnicode_FromString("("), 0, len)) {
            PySys_WriteStdout("Expr: mismatched brackets, expression bypassed.\n");
            Py_RETURN_NONE;            
        }
        for (i=0; i<self->count; i++) { 
            clearexpr(self->lexp[i]); 
        }
        self->count = 0;
        while (PyUnicode_Find(sentence, PyUnicode_FromString(")"), 0, len, 1) != -1) {
            end = PyUnicode_Find(sentence, PyUnicode_FromString(")"), 0, len, 1) + 1;
            start = PyUnicode_Find(sentence, PyUnicode_FromString("("), 0, end, -1);
            exp = PySequence_GetSlice(sentence, start, end);
            if (PyUnicode_Contains(exp, PyUnicode_FromString("let ")) || PyUnicode_Contains(exp, PyUnicode_FromString("var "))) {
                sentence = PyUnicode_Concat(PySequence_GetSlice(sentence, 0, start), PySequence_GetSlice(sentence, end, len));
            }
            else {
                sentence = PyUnicode_Replace(sentence, exp, PyUnicode_Format(PyUnicode_FromString("_%d"), PyInt_FromLong(self->count)), 1);
            }
            exp = PyUnicode_Replace(exp, PyUnicode_FromString("("), PyUnicode_FromString(""), -1);
            exp = PyUnicode_Replace(exp, PyUnicode_FromString(")"), PyUnicode_FromString(""), -1);
            explist = PyUnicode_Split(exp, NULL, -1);
            // Prepare variable from "var" function
            if (PyUnicode_Compare(PyList_GetItem(explist, 0), PyUnicode_FromString("var")) == 0) {
                PyList_SetItem(explist, 0, PyUnicode_FromString("const"));
                PyDict_SetItem(self->variables, PyList_GetItem(explist, 1), PyInt_FromLong(self->count));
                PySequence_DelItem(explist, 1);
            }
            // Prepare variable from "let" function
            if (PyUnicode_Compare(PyList_GetItem(explist, 0), PyUnicode_FromString("let")) == 0) {
                PyList_SetItem(explist, 0, PyUnicode_FromString("const"));
                if (PyDict_GetItem(waitingDict, PyList_GetItem(explist, 1)) != NULL) {
                    waitingList = PyDict_GetItem(waitingDict, PyList_GetItem(explist, 1));
                    for (j=0; j<PyList_Size(waitingList); j++) {
                        count = PyInt_AsLong(PyTuple_GetItem(PyList_GetItem(waitingList, j), 0));
                        i = PyInt_AsLong(PyTuple_GetItem(PyList_GetItem(waitingList, j), 1));
                        self->lexp[count].vars[i] = self->count;
                    }
                    PyDict_DelItem(waitingDict, PyList_GetItem(explist, 1));
                }
                PyDict_SetItem(varDict, PyList_GetItem(explist, 1), PyInt_FromLong(self->count));
                PySequence_DelItem(explist, 1);
            }
            // Initialize expression node
            self->lexp[self->count] = initexpr(PY_STRING_AS_STRING(PyList_GetItem(explist, 0)), PyList_Size(explist));
            if (PyList_Size(explist) == 1 && self->lexp[self->count].type_op == OP_CONST) {
                PyList_Insert(explist, 0, PyUnicode_FromString("const"));
            }
            while (PyList_Size(explist) < (self->lexp[self->count].num+1)) {
                PyList_Append(explist, PyUnicode_FromString("0.0"));
            }
            for (i=0; i<self->lexp[self->count].num; i++) {
                if (PyUnicode_Contains(PyList_GetItem(explist, i+1), PyUnicode_FromString("_"))) {
                    tmpstr = PyUnicode_Replace(PyList_GetItem(explist, i+1), PyUnicode_FromString("_"), PyUnicode_FromString(""), -1);
                    self->lexp[self->count].nodes[i] = PyInt_AsLong(PyInt_FromString(PY_STRING_AS_STRING(tmpstr), NULL, 0));
                }
                else if (PyUnicode_Contains(PyList_GetItem(explist, i+1), PyUnicode_FromString("#"))) {
                    if (PyDict_GetItem(self->variables, PyList_GetItem(explist, i+1)) != NULL) {
                        self->lexp[self->count].vars[i] = PyInt_AsLong(PyDict_GetItem(self->variables, PyList_GetItem(explist, i+1)));
                    }
                    else if (PyDict_GetItem(varDict, PyList_GetItem(explist, i+1)) != NULL) {
                        self->lexp[self->count].vars[i] = PyInt_AsLong(PyDict_GetItem(varDict, PyList_GetItem(explist, i+1)));
                    }
                    else {
                        if (PyDict_GetItem(waitingDict, PyList_GetItem(explist, i+1)) == NULL)
                            waitingList = PyList_New(0);
                        else
                            waitingList = PyDict_GetItem(waitingDict, PyList_GetItem(explist, i+1));
                        PyList_Append(waitingList, PyTuple_Pack(2, PyInt_FromLong(self->count), PyInt_FromLong(i)));
                        PyDict_SetItem(waitingDict, PyList_GetItem(explist, i+1), waitingList);
                    }
                }
                else if (PyUnicode_Contains(PyList_GetItem(explist, i+1), PyUnicode_FromString("$x"))) {
                    tmpstr = PyUnicode_Replace(PyList_GetItem(explist, i+1), PyUnicode_FromString("$x"), PyUnicode_FromString(""), -1);
                    tmpstr = PyUnicode_Replace(tmpstr, PyUnicode_FromString("["), PyUnicode_FromString(""), -1);
                    tmpstr = PyUnicode_Replace(tmpstr, PyUnicode_FromString("]"), PyUnicode_FromString(""), -1);
                    self->lexp[self->count].input[i] = PyInt_AsLong(PyInt_FromString(PY_STRING_AS_STRING(tmpstr), NULL, 0));
                }
                else if (PyUnicode_Contains(PyList_GetItem(explist, i+1), PyUnicode_FromString("$y"))) {
                    tmpstr = PyUnicode_Replace(PyList_GetItem(explist, i+1), PyUnicode_FromString("$y"), PyUnicode_FromString(""), -1);
                    tmpstr = PyUnicode_Replace(tmpstr, PyUnicode_FromString("["), PyUnicode_FromString(""), -1);
                    tmpstr = PyUnicode_Replace(tmpstr, PyUnicode_FromString("]"), PyUnicode_FromString(""), -1);
                    self->lexp[self->count].output[i] = PyInt_AsLong(PyInt_FromString(PY_STRING_AS_STRING(tmpstr), NULL, 0));
                }
                else {
                    self->lexp[self->count].values[i] = PyFloat_AsDouble(PyFloat_FromString(PyList_GetItem(explist, i+1), NULL));
                }
            }
            len = PyUnicode_GetSize(sentence);
            self->count++;
        }

        explist = PyUnicode_Split(sentence, NULL, -1);
        if (PyList_Size(explist) == 1) 
            PyList_Insert(explist, 0, PyUnicode_FromString("const"));
        // Initialize last expression node
        self->lexp[self->count] = initexpr(PY_STRING_AS_STRING(PyList_GetItem(explist, 0)), PyList_Size(explist));
        for (i=0; i<self->lexp[self->count].num; i++) {
            if (PyUnicode_Contains(PyList_GetItem(explist, i+1), PyUnicode_FromString("_"))) {
                tmpstr = PyUnicode_Replace(PyList_GetItem(explist, i+1), PyUnicode_FromString("_"), PyUnicode_FromString(""), -1);
                self->lexp[self->count].nodes[i] = PyInt_AsLong(PyInt_FromString(PY_STRING_AS_STRING(tmpstr), NULL, 0));
            }
            else if (PyUnicode_Contains(PyList_GetItem(explist, i+1), PyUnicode_FromString("#"))) {
                self->lexp[self->count].vars[i] = PyInt_AsLong(PyDict_GetItem(varDict, PyList_GetItem(explist, i+1)));
            }
            else if (PyUnicode_Contains(PyList_GetItem(explist, i+1), PyUnicode_FromString("$x"))) {
                tmpstr = PyUnicode_Replace(PyList_GetItem(explist, i+1), PyUnicode_FromString("$x"), PyUnicode_FromString(""), -1);
                tmpstr = PyUnicode_Replace(tmpstr, PyUnicode_FromString("["), PyUnicode_FromString(""), -1);
                tmpstr = PyUnicode_Replace(tmpstr, PyUnicode_FromString("]"), PyUnicode_FromString(""), -1);
                self->lexp[self->count].input[i] = PyInt_AsLong(PyInt_FromString(PY_STRING_AS_STRING(tmpstr), NULL, 0));
            }
            else if (PyUnicode_Contains(PyList_GetItem(explist, i+1), PyUnicode_FromString("$y"))) {
                tmpstr = PyUnicode_Replace(PyList_GetItem(explist, i+1), PyUnicode_FromString("$y"), PyUnicode_FromString(""), -1);
                tmpstr = PyUnicode_Replace(tmpstr, PyUnicode_FromString("["), PyUnicode_FromString(""), -1);
                tmpstr = PyUnicode_Replace(tmpstr, PyUnicode_FromString("]"), PyUnicode_FromString(""), -1);
                self->lexp[self->count].output[i] = PyInt_AsLong(PyInt_FromString(PY_STRING_AS_STRING(tmpstr), NULL, 0));
            }
            else {
                self->lexp[self->count].values[i] = PyFloat_AsDouble(PyFloat_FromString(PyList_GetItem(explist, i+1), NULL));
            }
        }

        self->count++;
    }

    Py_XDECREF(sentence);
    Py_XDECREF(exp);
    Py_XDECREF(explist);
    Py_XDECREF(tmpstr);
    Py_XDECREF(varDict);
    Py_XDECREF(waitingDict);
    Py_XDECREF(waitingList);
    
	Py_RETURN_NONE;
}

static PyObject * 
Expr_setVar(Expr *self, PyObject *args, PyObject *kwds)
{
    int index;
    PyObject *varname = NULL, *value = NULL;
    static char *kwlist[] = {"varname", "value", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &varname, &value))
        Py_RETURN_NONE;

    if (PyDict_GetItem(self->variables, varname)) {
        index = PyInt_AsLong(PyDict_GetItem(self->variables, varname));
        self->lexp[index].values[0] = PyFloat_AsDouble(value);
    }
	Py_RETURN_NONE;
}

static PyObject * 
Expr_printNodes(Expr *self) { 
    int i;
    for (i=0; i<self->count; i++) {
        print_expr(self->lexp[i], i);
    }
	Py_RETURN_NONE;
};

static PyMemberDef Expr_members[] = {
    {"server", T_OBJECT_EX, offsetof(Expr, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Expr, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Expr, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Expr, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Expr, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Expr_methods[] = {
    {"getServer", (PyCFunction)Expr_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Expr_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Expr_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Expr_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Expr_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"printNodes", (PyCFunction)Expr_printNodes, METH_NOARGS, "Print the list of nodes."},
	{"setVar", (PyCFunction)Expr_setVar, METH_VARARGS|METH_KEYWORDS, "Sets a variable value."},
	{"setExpr", (PyCFunction)Expr_setExpr, METH_O, "Sets a new expression."},
	{"setMul", (PyCFunction)Expr_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Expr_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Expr_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Expr_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Expr_as_number = {
    (binaryfunc)Expr_add,                      /*nb_add*/
    (binaryfunc)Expr_sub,                 /*nb_subtract*/
    (binaryfunc)Expr_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)Expr_inplace_add,              /*inplace_add*/
    (binaryfunc)Expr_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Expr_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Expr_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Expr_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject ExprType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Expr_base",         /*tp_name*/
    sizeof(Expr),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Expr_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Expr_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Expr objects. Resolve a prefix notation sentence at audio rate.",           /* tp_doc */
    (traverseproc)Expr_traverse,   /* tp_traverse */
    (inquiry)Expr_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Expr_methods,             /* tp_methods */
    Expr_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Expr_new,                 /* tp_new */
};
