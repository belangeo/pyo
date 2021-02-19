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

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>
#include "pyomodule.h"
#include "servermodule.h"
#include "streammodule.h"
#include "pvstreammodule.h"
#include "dummymodule.h"
#include "tablemodule.h"
#include "matrixmodule.h"

/****** Algorithm utilities ******/
#define reducePoints_info \
"\nDouglas-Peucker curve reduction algorithm.\n\n\
This function receives a list of points as input and returns a simplified list by\neliminating redundancies.\n\n\
A point is a tuple (or a list) of two floats, time and value. A list of points looks like:\n\n        \
[ (0, 0), (0.1, 0.7), (0.2, 0.5), ... ] \n\n\
:Args:\n\n    \
pointlist: list of lists or list of tuples\n        List of points (time, value) to filter.\n    \
tolerance: float, optional\n        Normalized distance threshold under which a point is\n        excluded from the list. Defaults to 0.02."

typedef struct STACK_RECORD
{
    int nAnchorIndex;
    int nFloaterIndex;
    struct STACK_RECORD *precPrev;
} STACK_RECORD;

STACK_RECORD *m_pStack = NULL;

static void StackPush( int nAnchorIndex, int nFloaterIndex )
{
    STACK_RECORD *precPrev = m_pStack;
    m_pStack = (STACK_RECORD *)malloc( sizeof(STACK_RECORD) );
    m_pStack->nAnchorIndex = nAnchorIndex;
    m_pStack->nFloaterIndex = nFloaterIndex;
    m_pStack->precPrev = precPrev;
}

static int StackPop( int *pnAnchorIndex, int *pnFloaterIndex )
{
    STACK_RECORD *precStack = m_pStack;

    if ( precStack == NULL )
        return 0;

    *pnAnchorIndex = precStack->nAnchorIndex;
    *pnFloaterIndex = precStack->nFloaterIndex;
    m_pStack = precStack->precPrev;
    free( precStack );
    return 1;
}

static PyObject *
reducePoints(PyObject *self, PyObject *args, PyObject *kwds)
{
    int i, nPointsCount, nVertexIndex, nAnchorIndex, nFloaterIndex;
    MYFLT dSegmentVecLength;
    MYFLT dAnchorVecX, dAnchorVecY;
    MYFLT dAnchorUnitVecX, dAnchorUnitVecY;
    MYFLT dVertexVecLength;
    MYFLT dVertexVecX, dVertexVecY;
    MYFLT dProjScalar;
    MYFLT dVertexDistanceToSegment;
    MYFLT dMaxDistThisSegment;
    int nVertexIndexMaxDistance;
    PyObject *pointlist, *pPointsOut, *tup;
    MYFLT *pPointsX, *pPointsY;
    int *pnUseFlag;
    MYFLT dTolerance = .02;
    MYFLT xMax, yMin, yMax;

    static char *kwlist[] = {"pointlist", "tolerance", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_F, kwlist, &pointlist, &dTolerance))
        return PyLong_FromLong(-1);

    nPointsCount = PyList_Size(pointlist);

    // TODO: these malloc's are never freed.
    pPointsX = (MYFLT *)malloc(nPointsCount * sizeof(MYFLT));
    pPointsY = (MYFLT *)malloc(nPointsCount * sizeof(MYFLT));
    pnUseFlag = (int *)malloc(nPointsCount * sizeof(int));

    tup = PyList_GET_ITEM(pointlist, 0);

    if (PyTuple_Check(tup) == 1)
    {
        for (i = 0; i < nPointsCount; i++)
        {
            tup = PyList_GET_ITEM(pointlist, i);
            pPointsX[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 0));
            pPointsY[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
            pnUseFlag[i] = 0;
        }
    }
    else
    {
        for (i = 0; i < nPointsCount; i++)
        {
            tup = PyList_GET_ITEM(pointlist, i);
            pPointsX[i] = PyFloat_AsDouble(PyList_GET_ITEM(tup, 0));
            pPointsY[i] = PyFloat_AsDouble(PyList_GET_ITEM(tup, 1));
            pnUseFlag[i] = 0;
        }
    }

    // rescale points between 0. and 1.
    xMax = pPointsX[nPointsCount - 1];
    yMin = 9999999999.9;
    yMax = -999999.9;

    for (i = 0; i < nPointsCount; i++)
    {
        if (pPointsY[i] < yMin)
            yMin = pPointsY[i];
        else if (pPointsY[i] > yMax)
            yMax = pPointsY[i];
    }

    for (i = 0; i < nPointsCount; i++)
    {
        pPointsX[i] = pPointsX[i] / xMax;
        pPointsY[i] = (pPointsY[i] - yMin) / yMax;
    }

    // filter...
    pnUseFlag[0] = pnUseFlag[nPointsCount - 1] = 1;
    nAnchorIndex = 0;
    nFloaterIndex = nPointsCount - 1;
    StackPush( nAnchorIndex, nFloaterIndex );

    while ( StackPop( &nAnchorIndex, &nFloaterIndex ) )
    {
        // initialize line segment
        dAnchorVecX = pPointsX[ nFloaterIndex ] - pPointsX[ nAnchorIndex ];
        dAnchorVecY = pPointsY[ nFloaterIndex ] - pPointsY[ nAnchorIndex ];
        dSegmentVecLength = sqrt( dAnchorVecX * dAnchorVecX
                                  + dAnchorVecY * dAnchorVecY );
        dAnchorUnitVecX = dAnchorVecX / dSegmentVecLength;
        dAnchorUnitVecY = dAnchorVecY / dSegmentVecLength;
        // inner loop:
        dMaxDistThisSegment = 0.0;
        nVertexIndexMaxDistance = nAnchorIndex + 1;

        for ( nVertexIndex = nAnchorIndex + 1; nVertexIndex < nFloaterIndex; nVertexIndex++ )
        {
            //compare to anchor
            dVertexVecX = pPointsX[ nVertexIndex ] - pPointsX[ nAnchorIndex ];
            dVertexVecY = pPointsY[ nVertexIndex ] - pPointsY[ nAnchorIndex ];
            dVertexVecLength = sqrt( dVertexVecX * dVertexVecX
                                     + dVertexVecY * dVertexVecY );
            //dot product:
            dProjScalar = dVertexVecX * dAnchorUnitVecX + dVertexVecY * dAnchorUnitVecY;

            if ( dProjScalar < 0.0 )
                dVertexDistanceToSegment = dVertexVecLength;
            else
            {
                //compare to floater
                dVertexVecX = pPointsX[ nVertexIndex ] - pPointsX[ nFloaterIndex ];
                dVertexVecY = pPointsY[ nVertexIndex ] - pPointsY[ nFloaterIndex ];
                dVertexVecLength = sqrt( dVertexVecX * dVertexVecX
                                         + dVertexVecY * dVertexVecY );
                //dot product:
                dProjScalar = dVertexVecX * (-dAnchorUnitVecX) + dVertexVecY * (-dAnchorUnitVecY);

                if ( dProjScalar < 0.0 )
                    dVertexDistanceToSegment = dVertexVecLength;
                else //calculate perpendicular distance to line (pythagorean theorem):
                    dVertexDistanceToSegment =
                        sqrt( fabs( dVertexVecLength * dVertexVecLength - dProjScalar * dProjScalar ) );
            }

            if ( dMaxDistThisSegment < dVertexDistanceToSegment )
            {
                dMaxDistThisSegment = dVertexDistanceToSegment;
                nVertexIndexMaxDistance = nVertexIndex;
            }
        }

        if ( dMaxDistThisSegment <= dTolerance )   //use line segment
        {
            pnUseFlag[ nAnchorIndex ] = 1;
            pnUseFlag[ nFloaterIndex ] = 1;
        }
        else
        {
            StackPush( nAnchorIndex, nVertexIndexMaxDistance );
            StackPush( nVertexIndexMaxDistance, nFloaterIndex );
        }
    }

    pPointsOut = PyList_New(0);

    for (i = 0; i < nPointsCount; i++)
    {
        if (pnUseFlag[i] == 1)
        {
            PyList_Append(pPointsOut, PyList_GET_ITEM(pointlist, i));
        }
    }

    free(pPointsX);
    free(pPointsY);
    free(pnUseFlag);

    return pPointsOut;
}

#define distanceToSegment_info \
"\nFind the distance from a point to a line or line segment.\n\n\
This function returns the shortest distance from a point to a line segment\nnormalized between 0 and 1.\n\n\
A point is a tuple (or a list) of two floats, time and value. `p` is the point for which\nto find the distance from line `p1` to `p2`.\n\n\
:Args:\n\n    \
p: list or tuple\n        Point for which to find the distance.\n    \
p1: list or tuple\n        First point of the segment.\n    \
p2: list or tuple\n        Second point of the segment.\n    \
xmin: float, optional\n        Minimum value on the X axis.\n    \
xmax: float, optional\n        Maximum value on the X axis.\n    \
ymin: float, optional\n        Minimum value on the Y axis.\n    \
ymax: float, optional\n        Maximum value on the Y axis.\n    \
xlog: boolean, optional\n        Set this argument to True if X axis has a logarithmic scaling.\n    \
ylog: boolean, optional\n        Set this argument to True if Y axis has a logarithmic scaling."

static PyObject *
distanceToSegment(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *p, *p1, *p2, *pf, *pf1, *pf2;
    MYFLT xscale, yscale, xDelta, yDelta, u;
    MYFLT xmin = 0.0;
    MYFLT xmax = 1.0;
    MYFLT ymin = 0.0;
    MYFLT ymax = 1.0;
    int xlog = 0;
    int ylog = 0;
    MYFLT xp[2], xp1[2], xp2[2], closest[2];

    static char *kwlist[] = {"p", "p1", "p2", "xmin", "xmax", "ymin", "ymax", "xlog", "ylog", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OOO_FFFFII, kwlist, &p, &p1, &p2, &xmin, &xmax, &ymin, &ymax, &xlog, &ylog))
        return PyLong_FromLong(-1);

    pf = PySequence_Fast(p, NULL);
    pf1 = PySequence_Fast(p1, NULL);
    pf2 = PySequence_Fast(p2, NULL);

    if (xlog == 0)
    {
        xscale = xmax - xmin;
        xp[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 0)) / xscale;
        xp1[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 0)) / xscale;
        xp2[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 0)) / xscale;
    }
    else
    {
        xscale = MYLOG10(xmax / xmin);
        xp[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 0)) / xmin) / xscale;
        xp1[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 0)) / xmin) / xscale;
        xp2[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 0)) / xmin) / xscale;
    }

    if (ylog == 0)
    {
        yscale = ymax - ymin;
        xp[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 1)) / yscale;
        xp1[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 1)) / yscale;
        xp2[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 1)) / yscale;
    }
    else
    {
        yscale = MYLOG10(ymax / ymin);
        xp[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 1)) / ymin) / yscale;
        xp1[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 1)) / ymin) / yscale;
        xp2[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 1)) / ymin) / yscale;
    }

    xDelta = xp2[0] - xp1[0];
    yDelta = xp2[1] - xp1[1];
    u = ((xp[0] - xp1[0]) * xDelta + (xp[1] - xp1[1]) * yDelta) / (xDelta * xDelta + yDelta * yDelta);

    if (u < 0.0)
    {
        closest[0] = xp1[0];
        closest[1] = xp1[1];
    }
    else if (u > 1.0)
    {
        closest[0] = xp2[0];
        closest[1] = xp2[1];
    }
    else
    {
        closest[0] = xp1[0] + u * xDelta;
        closest[1] = xp1[1] + u * yDelta;
    }

    return PyFloat_FromDouble(MYSQRT(MYPOW(xp[0] - closest[0], 2.0) + MYPOW(xp[1] - closest[1], 2.0)));
}

#define linToCosCurve_info \
"\nCreates a cosinus interpolated curve from a list of points.\n\n\
A point is a tuple (or a list) of two floats, time and value.\n\n:Args:\n\n    \
data: list of points\n        Set of points between which will be inserted interpolated segments.\n    \
yrange: list of 2 floats, optional\n        Minimum and maximum values on the Y axis. Defaults to [0., 1.].\n    \
totaldur: float, optional\n        X axis duration. Defaults to 1.\n    \
points: int, optional\n        Number of points in the output list. Defaults to 1024.\n    \
log: boolean, optional\n        Set this value to True if the Y axis has a logarithmic scale. Defaults to False\n\n\
>>> s = Server().boot()\n\
>>> a = [(0,0), (0.25, 1), (0.33, 1), (1,0)]\n\
>>> b = linToCosCurve(a, yrange=[0, 1], totaldur=1, points=8192)\n\
>>> t = DataTable(size=len(b), init=[x[1] for x in b])\n\
>>> t.view()\n\n"

static PyObject *
linToCosCurve(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *data, *fdata, *out, *inout, *ftup, *yrange = NULL, *fyrange = NULL;
    int i, j, datasize, steps;
    double tmp, x1, x2, y1, y2, mu, ydiff, log10ymin, log10ymax;
    double *xdata, *ydata, *cxdata, *cydata;
    double totaldur = 1.0;
    double ymin = 0.0;
    double ymax = 1.0;
    int num = 1024;
    double inc = 1.0 / num;
    int log = 0;
    int count = 0;

    static char *kwlist[] = {"data", "yrange", "totaldur", "points", "log", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|Odii", kwlist, &data, &yrange, &totaldur, &num, &log))
        Py_RETURN_NONE;

    if (yrange)
    {
        fyrange = PySequence_Fast(yrange, NULL);
        ymin = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(fyrange, 0));
        ymax = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(fyrange, 1));
    }

    ydiff = ymax - ymin;
    log10ymin = log10(ymin);
    log10ymax = log10(ymax);

    fdata = PySequence_Fast(data, NULL);
    datasize = PySequence_Fast_GET_SIZE(fdata);
    xdata = (double *)malloc(datasize * sizeof(double));
    ydata = (double *)malloc(datasize * sizeof(double));

    /* acquire data + normalization */
    if (log == 0)
    {
        for (i = 0; i < datasize; i++)
        {
            ftup = PySequence_Fast(PySequence_Fast_GET_ITEM(fdata, i), NULL);
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 0));
            xdata[i] = tmp / totaldur;
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 1));
            ydata[i] = (tmp - ymin) / ydiff;
        }
    }
    else
    {
        for (i = 0; i < datasize; i++)
        {
            ftup = PySequence_Fast(PySequence_Fast_GET_ITEM(fdata, i), NULL);
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 0));
            xdata[i] = tmp / totaldur;
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 1));
            ydata[i] = log10(tmp / ymin) / log10(ymax / ymin);
        }
    }

    cxdata = (double *)malloc((num + 5) * sizeof(double));
    cydata = (double *)malloc((num + 5) * sizeof(double));

    /* generates cos interpolation */
    for (i = 0; i < (datasize - 1); i++)
    {
        x1 = xdata[i];
        x2 = xdata[i + 1];
        y1 = ydata[i];
        y2 = ydata[i + 1];
        steps = (int)((x2 - x1) * num);

        if (steps <= 0)
            continue;

        for (j = 0; j < steps; j++)
        {
            mu = (1.0 - cos(j / (float)steps * PI)) * 0.5;
            cxdata[count] = x1 + inc * j;
            cydata[count++] = y1 + (y2 - y1) * mu;
        }
    }

    cxdata[count] = xdata[datasize - 1];
    cydata[count++] = ydata[datasize - 1];

    /* denormalization */
    if (log == 0)
    {
        for (i = 0; i < count; i++)
        {
            cxdata[i] *= totaldur;
            cydata[i] = cydata[i] * ydiff + ymin;
        }
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            cxdata[i] *= totaldur;
            cydata[i] = pow(10.0, cydata[i] * (log10ymax - log10ymin) + log10ymin);
        }
    }

    /* output Python's list of lists */
    out = PyList_New(0);

    for (i = 0; i < count; i++)
    {
        inout = PyList_New(0);
        PyList_Append(inout, PyFloat_FromDouble(cxdata[i]));
        PyList_Append(inout, PyFloat_FromDouble(cydata[i]));
        PyList_Append(out, inout);
    }

    free(xdata);
    free(ydata);
    free(cxdata);
    free(cydata);

    return out;
}

#define rescale_info \
"\nConverts values from an input range to an output range.\n\n\
This function takes data in the range `xmin` - `xmax` and returns corresponding values\nin the range `ymin` - `ymax`.\n\n\
`data` can be either a number or a list. Return value is of the same type as `data`\nwith all values rescaled.\n\n\
:Argss:\n\n    \
data: float or list of floats\n        Values to convert.\n    \
xmin: float, optional\n        Minimum value of the input range.\n    \
xmax: float, optional\n        Maximum value of the input range.\n    \
ymin: float, optional\n        Minimum value of the output range.\n    \
ymax: float, optional\n        Maximum value of the output range.\n    \
xlog: boolean, optional\n        Set this argument to True if the input range has a logarithmic scaling.\n    \
ylog: boolean, optional\n        Set this argument to True if the output range has a logarithmic scaling.\n\n\
>>> a = 0.5\n\
>>> b = rescale(a, 0, 1, 20, 20000, False, True)\n\
>>> print(b)\n\
632.453369141\n\
>>> a = [0, .4, .8]\n\
>>> b = rescale(a, 0, 1, 20, 20000, False, True)\n\
>>> print(b)\n\
[20.000001907348633, 316.97738647460938, 5023.7705078125]\n\n"

static PyObject *
rescale(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *data, *out;
    MYFLT datascl, curscl, val;
    MYFLT xmin = 0.0;
    MYFLT xmax = 1.0;
    MYFLT ymin = 0.0;
    MYFLT ymax = 1.0;
    int xlog = 0;
    int ylog = 0;
    int i, cnt;
    int type; // 0 = float, 1 = list of floats

    static char *kwlist[] = {"data", "xmin", "xmax", "ymin", "ymax", "xlog", "ylog", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FFFFII, kwlist, &data, &xmin, &xmax, &ymin, &ymax, &xlog, &ylog))
        return PyLong_FromLong(-1);

    if (PyNumber_Check(data))
        type = 0;
    else if (PyList_Check(data))
        type = 1;
    else
    {
        Py_RETURN_NONE;
    }

    if (xlog == 0 && ylog == 0)
    {
        datascl = xmax - xmin;
        curscl = ymax - ymin;
        curscl /= datascl;

        if (type == 0)
        {
            val = PyFloat_AsDouble(data);
            return Py_BuildValue("d", (val - xmin) * curscl + ymin);
        }
        else if (type == 1)
        {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);

            for (i = 0; i < cnt; i++)
            {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
                PyList_SET_ITEM(out, i, PyFloat_FromDouble((val - xmin) * curscl + ymin));
            }

            return out;
        }
    }
    else if (xlog == 0 && ylog == 1)
    {
        if (xmin == 0)
            xmin = 0.000001;

        datascl = xmax - xmin;
        curscl = MYLOG10(ymax / ymin);
        ymin = MYLOG10(ymin);

        if (type == 0)
        {
            val = PyFloat_AsDouble(data);

            if (val == 0)
                val = 0.000001;

            val = (val - xmin) / datascl;
            return Py_BuildValue("d", MYPOW(10.0, val * curscl + ymin));
        }
        else if (type == 1)
        {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);

            for (i = 0; i < cnt; i++)
            {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));

                if (val == 0)
                    val = 0.000001;

                val = (val - xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(MYPOW(10.0, val * curscl + ymin)));
            }

            return out;
        }
    }
    else if (xlog == 1 && ylog == 0)
    {
        datascl = MYLOG10(xmax / xmin);
        curscl = ymax - ymin;

        if (type == 0)
        {
            val = PyFloat_AsDouble(data);
            val = MYLOG10(val / xmin) / datascl;
            return Py_BuildValue("d", val * curscl + ymin);
        }
        else if (type == 1)
        {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);

            for (i = 0; i < cnt; i++)
            {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
                val = MYLOG10(val / xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(val * curscl + ymin));
            }

            return out;
        }
    }
    else if (xlog == 1 && ylog == 1)
    {
        datascl = MYLOG10(xmax / xmin);
        curscl = MYLOG10(ymax / ymin);
        ymin = MYLOG10(ymin);

        if (type == 0)
        {
            val = PyFloat_AsDouble(data);
            val = MYLOG10(val / xmin) / datascl;
            return Py_BuildValue("d", MYPOW(10.0, val * curscl + ymin));
        }
        else if (type == 1)
        {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);

            for (i = 0; i < cnt; i++)
            {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
                val = MYLOG10(val / xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(MYPOW(10.0, val * curscl + ymin)));
            }

            return out;
        }
    }
    else
    {
        Py_RETURN_NONE;
    }

    Py_RETURN_NONE;
}

#define floatmap_info \
"\nConverts values from a 0-1 range to an output range.\n\n\
This function takes data in the range `0` - `1` and returns corresponding values\nin the range `min` - `max`.\n\n\
:Argss:\n\n    \
x: float\n        Value to convert, in the range 0 to 1.\n    \
min: float, optional\n        Minimum value of the output range. Defaults to 0.\n    \
max: float, optional\n        Maximum value of the output range. Defaults to 1.\n    \
exp: float, optional\n        Power factor (1 (default) is linear, les than 1 is logarithmic, greter than 1 is exponential).\n\n\
>>> a = 0.5\n\
>>> b = floatmap(a, 0, 1, 4)\n\
>>> print(b)\n\
0.0625\n\n"

static PyObject *
floatmap(PyObject *self, PyObject *args, PyObject *kwds)
{
    MYFLT x = 0.0;
    MYFLT min = 0.0;
    MYFLT max = 1.0;
    MYFLT exp = 1.0;

    static char *kwlist[] = {"x", "min", "max", "exp", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_FFF, kwlist, &x, &min, &max, &exp))
        return PyLong_FromLong(-1);

    if (x < 0.0)
        x = 0.0;
    else if (x > 1.0)
        x = 1.0;

    if (exp != 1.0)
        x = MYPOW(x, exp);

    return Py_BuildValue("d", x * (max - min) + min);
}

/****** Conversion utilities ******/
#define midiToHz_info \
"\nConverts a midi note value to frequency in Hertz.\n\n:Args:\n\n    \
x: int or float\n        Midi note. `x` can be a number, a list or a tuple, otherwise the function returns None.\n\n\
>>> a = (48, 60, 62, 67)\n\
>>> b = midiToHz(a)\n\
>>> print(b)\n\
(130.8127826503271, 261.62556530066814, 293.66476791748823, 391.9954359818656)\n\
>>> a = [48, 60, 62, 67]\n\
>>> b = midiToHz(a)\n\
>>> print(b)\n\
[130.8127826503271, 261.62556530066814, 293.66476791748823, 391.9954359818656]\n\
>>> b = midiToHz(60.0)\n\
>>> print(b)\n\
261.625565301\n\n"

static PyObject *
midiToHz(PyObject *self, PyObject *arg)
{
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("d", 440.0 * MYPOW(2.0, (PyFloat_AsDouble(arg) - 69) / 12.0));
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(440.0 * MYPOW(2.0, (x - 69) / 12.0)));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(440.0 * MYPOW(2.0, (x - 69) / 12.0)));
        }

        return newseq;
    }
    else
        Py_RETURN_NONE;
}

#define hzToMidi_info \
"\nConverts a frequency in Hertz to a midi note value.\n\n:Args:\n\n    \
x: float\n        Frequency in Hertz. `x` can be a number, a list or a tuple, otherwise the function returns None.\n\n\
>>> a = (110.0, 220.0, 440.0, 880.0)\n\
>>> b = hzToMidi(a)\n\
>>> print(b)\n\
(45.0, 57.0, 69.0, 81.0)\n\
>>> a = [110.0, 220.0, 440.0, 880.0]\n\
>>> b = hzToMidi(a)\n\
>>> print(b)\n\
[45.0, 57.0, 69.0, 81.0]\n\
>>> b = hzToMidi(440.0)\n\
>>> print(b)\n\
69.0\n\n"

static PyObject *
hzToMidi(PyObject *self, PyObject *arg)
{
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("d", 12.0 * MYLOG2(PyFloat_AsDouble(arg) / 440.0) + 69);
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(12.0 * MYLOG2(x / 440.0) + 69));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(12.0 * MYLOG2(x / 440.0) + 69));
        }

        return newseq;
    }
    else
        Py_RETURN_NONE;
}

#define midiToTranspo_info \
"\nConverts a midi note value to transposition factor (central key = 60).\n\n:Args:\n\n    \
x: int or float\n        Midi note. `x` can be a number, a list or a tuple, otherwise the function returns None.\n\n\
>>> a = (48, 60, 62, 67)\n\
>>> b = midiToTranspo(a)\n\
>>> print(b)\n    \
(0.49999999999997335, 1.0, 1.122462048309383, 1.4983070768767281)\n\
>>> a = [48, 60, 62, 67]\n\
>>> b = midiToTranspo(a)\n\
>>> print(b)\n\
[0.49999999999997335, 1.0, 1.122462048309383, 1.4983070768767281]\n\
>>> b = midiToTranspo(60.0)\n\
>>> print(b)\n\
1.0\n\n"

static PyObject *
midiToTranspo(PyObject *self, PyObject *arg)
{
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("d", pow(1.0594630943593, PyFloat_AsDouble(arg) - 60.0));
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(pow(1.0594630943593, x - 60.0)));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(pow(1.0594630943593, x - 60.0)));
        }

        return newseq;
    }
    else
        Py_RETURN_NONE;
}

#define sampsToSec_info \
"\nReturns the duration in seconds equivalent to the number of samples given as an argument.\n\n:Args:\n\n    \
x: int or float\n        Duration in samples. `x` can be a number, a list or a tuple, otherwise function returns None.\n\n\
>>> s = Server().boot()\n\
>>> a = (64, 128, 256)\n\
>>> b = sampsToSec(a)\n\
>>> print(b)\n\
(0.0014512471655328798, 0.0029024943310657597, 0.0058049886621315194)\n\
>>> a = [64, 128, 256]\n\
>>> b = sampsToSec(a)\n\
>>> print(b)\n\
[0.0014512471655328798, 0.0029024943310657597, 0.0058049886621315194]\n\
>>> b = sampsToSec(8192)\n\
>>> print(b)\n\
0.185759637188\n\n"

static PyObject *
sampsToSec(PyObject *self, PyObject *arg)
{
    PyObject *server = PyServer_get_server();

    if (server == NULL)
    {
        PySys_WriteStdout("Pyo error: A Server must be booted before calling `sampsToSec` function.\n");
        Py_RETURN_NONE;
    }

    double sr = PyFloat_AsDouble(PyObject_CallMethod(server, "getSamplingRate", NULL));
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("d", PyFloat_AsDouble(arg) / sr);
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(x / sr));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(x / sr));
        }

        return newseq;
    }
    else
        Py_RETURN_NONE;
}

#define secToSamps_info \
"\nReturns the number of samples equivalent to the duration in seconds given as an argument.\n\n:Args:\n\n    \
x: int or float\n        Duration in seconds. `x` can be a number, a list or a tuple, otherwise function returns None.\n\n\
>>> s = Server().boot()\n\
>>> a = (0.1, 0.25, 0.5, 1)\n\
>>> b = secToSamps(a)\n\
>>> print(b)\n\
(4410, 11025, 22050, 44100)\n\
>>> a = [0.1, 0.25, 0.5, 1]\n\
>>> b = secToSamps(a)\n\
>>> print(b)\n\
[4410, 11025, 22050, 44100]\n\
>>> b = secToSamps(2.5)\n\
>>> print(b)\n\
110250\n\n"

static PyObject *
secToSamps(PyObject *self, PyObject *arg)
{
    PyObject *server = PyServer_get_server();

    if (server == NULL)
    {
        PySys_WriteStdout("Pyo error: A Server must be booted before calling `secToSamps` function.\n");
        Py_RETURN_NONE;
    }

    double sr = PyFloat_AsDouble(PyObject_CallMethod(server, "getSamplingRate", NULL));
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("l", (long)(PyFloat_AsDouble(arg) * sr));
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyLong_FromLong((long)(x * sr)));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyLong_FromLong((long)(x * sr)));
        }

        return newseq;
    }
    else
        Py_RETURN_NONE;
}

/************* Server quieries *************/
#define serverCreated_info \
"\nReturns True if a Server object is already created, otherwise, returns False.\n\n\
>>> print(serverCreated())\n\
False\n\
>>> s = Server()\n\
>>> print(serverCreated())\n\
True\n\n"

static PyObject *
serverCreated(PyObject *self)
{
    if (PyServer_get_server() != NULL)
    {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else
    {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

#define serverBooted_info \
"\nReturns True if an already created Server is booted, otherwise, returns False.\n\n\
>>> s = Server()\n\
>>> print(serverBooted())\n\
False\n\
>>> s.boot()\n\
>>> print(serverBooted())\n\
True\n\n"

static PyObject *
serverBooted(PyObject *self)
{
    int boot;
    PyObject *server;

    if (PyServer_get_server() != NULL)
    {
        server = PyServer_get_server();
        boot = PyLong_AsLong(PyObject_CallMethod(server, "getIsBooted", NULL));

        if (boot == 0)
        {
            Py_INCREF(Py_False);
            return Py_False;
        }
        else
        {
            Py_INCREF(Py_True);
            return Py_True;
        }
    }
    else
    {
        PySys_WriteStdout("Pyo Warning: A Server must be created before calling `serverBooted` function.\n");
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyMethodDef pyo_functions[] =
{
    {"reducePoints", (PyCFunction)reducePoints, METH_VARARGS | METH_KEYWORDS, reducePoints_info},
    {"distanceToSegment", (PyCFunction)distanceToSegment, METH_VARARGS | METH_KEYWORDS, distanceToSegment_info},
    {"rescale", (PyCFunction)rescale, METH_VARARGS | METH_KEYWORDS, rescale_info},
    {"floatmap", (PyCFunction)floatmap, METH_VARARGS | METH_KEYWORDS, floatmap_info},
    {"linToCosCurve", (PyCFunction)linToCosCurve, METH_VARARGS | METH_KEYWORDS, linToCosCurve_info},
    {"midiToHz", (PyCFunction)midiToHz, METH_O, midiToHz_info},
    {"hzToMidi", (PyCFunction)hzToMidi, METH_O, hzToMidi_info},
    {"midiToTranspo", (PyCFunction)midiToTranspo, METH_O, midiToTranspo_info},
    {"sampsToSec", (PyCFunction)sampsToSec, METH_O, sampsToSec_info},
    {"secToSamps", (PyCFunction)secToSamps, METH_O, secToSamps_info},
    {"serverCreated", (PyCFunction)serverCreated, METH_NOARGS, serverCreated_info},
    {"serverBooted", (PyCFunction)serverBooted, METH_NOARGS, serverBooted_info},
    {NULL, NULL, 0, NULL},
};

#if PY_MAJOR_VERSION >= 3
// TODO: Pyo likely has a bunch of state stored in global variables right now, they should ideally be stored
// in an interpreter specific struct as described in https://docs.python.org/3/howto/cporting.html
static struct PyModuleDef pyo_moduledef =
{
    PyModuleDef_HEAD_INIT,
    LIB_BASE_NAME,/* m_name */
    "Python digital signal processing module.",/* m_doc */
    0,/* m_size */
    pyo_functions,/* m_methods */
    NULL,/* m_reload */
    NULL,/* m_traverse */
    NULL,/* m_clear */
    NULL,/* m_free */
};
#endif

static PyObject *
module_add_object(PyObject *module, const char *name, PyTypeObject *type)
{
    if (PyType_Ready(type) < 0)
        Py_RETURN_NONE;

    Py_INCREF(type);
    PyModule_AddObject(module, name, (PyObject *)type);
    Py_RETURN_NONE;
}

PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
#ifndef USE_DOUBLE
PyInit__pyo(void)
#else
PyInit__pyo64(void)
#endif
#else
#ifndef USE_DOUBLE
init_pyo(void)
#else
init_pyo64(void)
#endif
#endif

{
    PyObject *m;

#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&pyo_moduledef);
#else
    m = Py_InitModule3(LIB_BASE_NAME, pyo_functions, "Python digital signal processing module.");
#endif

    module_add_object(m, "Server_base", &ServerType);
    module_add_object(m, "Stream", &StreamType);
    module_add_object(m, "TriggerStream", &TriggerStreamType);
    module_add_object(m, "PVStream", &PVStreamType);
    module_add_object(m, "Dummy_base", &DummyType);
    module_add_object(m, "TriggerDummy_base", &TriggerDummyType);
    module_add_object(m, "TableStream", &TableStreamType);
    module_add_object(m, "MatrixStream", &MatrixStreamType);
    module_add_object(m, "ControlRec_base", &ControlRecType);
    module_add_object(m, "ControlRead_base", &ControlReadType);
    module_add_object(m, "NoteinRec_base", &NoteinRecType);
    module_add_object(m, "NoteinRead_base", &NoteinReadType);
    module_add_object(m, "Compare_base", &CompareType);
    module_add_object(m, "Mix_base", &MixType);
    module_add_object(m, "Sig_base", &SigType);
    module_add_object(m, "SigTo_base", &SigToType);
    module_add_object(m, "VarPort_base", &VarPortType);
    module_add_object(m, "InputFader_base", &InputFaderType);
    module_add_object(m, "Adsr_base", &AdsrType);
    module_add_object(m, "Linseg_base", &LinsegType);
    module_add_object(m, "Expseg_base", &ExpsegType);
    module_add_object(m, "HarmTable_base", &HarmTableType);
    module_add_object(m, "ChebyTable_base", &ChebyTableType);
    module_add_object(m, "HannTable_base", &HannTableType);
    module_add_object(m, "SincTable_base", &SincTableType);
    module_add_object(m, "WinTable_base", &WinTableType);
    module_add_object(m, "ParaTable_base", &ParaTableType);
    module_add_object(m, "LinTable_base", &LinTableType);
    module_add_object(m, "LogTable_base", &LogTableType);
    module_add_object(m, "CosLogTable_base", &CosLogTableType);
    module_add_object(m, "CosTable_base", &CosTableType);
    module_add_object(m, "CurveTable_base", &CurveTableType);
    module_add_object(m, "ExpTable_base", &ExpTableType);
    module_add_object(m, "DataTable_base", &DataTableType);
    module_add_object(m, "NewTable_base", &NewTableType);
    module_add_object(m, "TableRec_base", &TableRecType);
    module_add_object(m, "TableRecTimeStream_base", &TableRecTimeStreamType);
    module_add_object(m, "TableMorph_base", &TableMorphType);
    module_add_object(m, "TrigTableRec_base", &TrigTableRecType);
    module_add_object(m, "TrigTableRecTimeStream_base", &TrigTableRecTimeStreamType);
    module_add_object(m, "TableWrite_base", &TableWriteType);
    module_add_object(m, "TablePut_base", &TablePutType);
    module_add_object(m, "NewMatrix_base", &NewMatrixType);
    module_add_object(m, "MatrixPointer_base", &MatrixPointerType);
    module_add_object(m, "MatrixRec_base", &MatrixRecType);
    module_add_object(m, "MatrixRecLoop_base", &MatrixRecLoopType);
    module_add_object(m, "MatrixMorph_base", &MatrixMorphType);
    module_add_object(m, "Input_base", &InputType);
    module_add_object(m, "Trig_base", &TrigType);
    module_add_object(m, "NextTrig_base", &NextTrigType);
    module_add_object(m, "Metro_base", &MetroType);
    module_add_object(m, "Seqer_base", &SeqerType);
    module_add_object(m, "Seq_base", &SeqType);
    module_add_object(m, "Clouder_base", &ClouderType);
    module_add_object(m, "Cloud_base", &CloudType);
    module_add_object(m, "Beater_base", &BeaterType);
    module_add_object(m, "Beat_base", &BeatType);
    module_add_object(m, "BeatTapStream_base", &BeatTapStreamType);
    module_add_object(m, "BeatAmpStream_base", &BeatAmpStreamType);
    module_add_object(m, "BeatDurStream_base", &BeatDurStreamType);
    module_add_object(m, "BeatEndStream_base", &BeatEndStreamType);
    module_add_object(m, "Fader_base", &FaderType);
    module_add_object(m, "Randi_base", &RandiType);
    module_add_object(m, "Randh_base", &RandhType);
    module_add_object(m, "Choice_base", &ChoiceType);
    module_add_object(m, "RandDur_base", &RandDurType);
    module_add_object(m, "Xnoise_base", &XnoiseType);
    module_add_object(m, "XnoiseMidi_base", &XnoiseMidiType);
    module_add_object(m, "XnoiseDur_base", &XnoiseDurType);
    module_add_object(m, "RandInt_base", &RandIntType);
    module_add_object(m, "Urn_base", &UrnType);
    module_add_object(m, "Osc_base", &OscType);
    module_add_object(m, "OscLoop_base", &OscLoopType);
    module_add_object(m, "OscTrig_base", &OscTrigType);
    module_add_object(m, "OscBank_base", &OscBankType);
    module_add_object(m, "SumOsc_base", &SumOscType);
    module_add_object(m, "TableRead_base", &TableReadType);
    module_add_object(m, "Pulsar_base", &PulsarType);
    module_add_object(m, "Sine_base", &SineType);
    module_add_object(m, "FastSine_base", &FastSineType);
    module_add_object(m, "SineLoop_base", &SineLoopType);
    module_add_object(m, "Fm_base", &FmType);
    module_add_object(m, "CrossFm_base", &CrossFmType);
    module_add_object(m, "LFO_base", &LFOType);
    module_add_object(m, "Blit_base", &BlitType);
    module_add_object(m, "Rossler_base", &RosslerType);
    module_add_object(m, "RosslerAlt_base", &RosslerAltType);
    module_add_object(m, "Lorenz_base", &LorenzType);
    module_add_object(m, "LorenzAlt_base", &LorenzAltType);
    module_add_object(m, "ChenLee_base", &ChenLeeType);
    module_add_object(m, "ChenLeeAlt_base", &ChenLeeAltType);
    module_add_object(m, "Phasor_base", &PhasorType);
    module_add_object(m, "SuperSaw_base", &SuperSawType);
    module_add_object(m, "Pointer_base", &PointerType);
    module_add_object(m, "TableIndex_base", &TableIndexType);
    module_add_object(m, "Lookup_base", &LookupType);
    module_add_object(m, "Noise_base", &NoiseType);
    module_add_object(m, "PinkNoise_base", &PinkNoiseType);
    module_add_object(m, "BrownNoise_base", &BrownNoiseType);
    module_add_object(m, "Biquad_base", &BiquadType);
    module_add_object(m, "Biquadx_base", &BiquadxType);
    module_add_object(m, "Biquada_base", &BiquadaType);
    module_add_object(m, "EQ_base", &EQType);
    module_add_object(m, "Tone_base", &ToneType);
    module_add_object(m, "Atone_base", &AtoneType);
    module_add_object(m, "DCBlock_base", &DCBlockType);
    module_add_object(m, "Allpass_base", &AllpassType);
    module_add_object(m, "Allpass2_base", &Allpass2Type);
    module_add_object(m, "Phaser_base", &PhaserType);
    module_add_object(m, "Vocoder_base", &VocoderType);
    module_add_object(m, "Port_base", &PortType);
    module_add_object(m, "Denorm_base", &DenormType);
    module_add_object(m, "Disto_base", &DistoType);
    module_add_object(m, "Clip_base", &ClipType);
    module_add_object(m, "Mirror_base", &MirrorType);
    module_add_object(m, "Wrap_base", &WrapType);
    module_add_object(m, "Between_base", &BetweenType);
    module_add_object(m, "Degrade_base", &DegradeType);
    module_add_object(m, "Compress_base", &CompressType);
    module_add_object(m, "Gate_base", &GateType);
    module_add_object(m, "Balance_base", &BalanceType);
    module_add_object(m, "Delay_base", &DelayType);
    module_add_object(m, "SDelay_base", &SDelayType);
    module_add_object(m, "Waveguide_base", &WaveguideType);
    module_add_object(m, "AllpassWG_base", &AllpassWGType);
    module_add_object(m, "TrigRand_base", &TrigRandType);
    module_add_object(m, "TrigRandInt_base", &TrigRandIntType);
    module_add_object(m, "TrigVal_base", &TrigValType);
    module_add_object(m, "TrigChoice_base", &TrigChoiceType);
    module_add_object(m, "Iter_base", &IterType);
    module_add_object(m, "TrigEnv_base", &TrigEnvType);
    module_add_object(m, "TrigLinseg_base", &TrigLinsegType);
    module_add_object(m, "TrigExpseg_base", &TrigExpsegType);
    module_add_object(m, "TrigFunc_base", &TrigFuncType);
    module_add_object(m, "TrigXnoise_base", &TrigXnoiseType);
    module_add_object(m, "TrigXnoiseMidi_base", &TrigXnoiseMidiType);
    module_add_object(m, "Pattern_base", &PatternType);
    module_add_object(m, "CallAfter_base", &CallAfterType);
    module_add_object(m, "BandSplitter_base", &BandSplitterType);
    module_add_object(m, "BandSplit_base", &BandSplitType);
    module_add_object(m, "FourBandMain_base", &FourBandMainType);
    module_add_object(m, "FourBand_base", &FourBandType);
    module_add_object(m, "HilbertMain_base", &HilbertMainType);
    module_add_object(m, "Hilbert_base", &HilbertType);
    module_add_object(m, "Follower_base", &FollowerType);
    module_add_object(m, "Follower2_base", &Follower2Type);
    module_add_object(m, "ZCross_base", &ZCrossType);
    module_add_object(m, "SPanner_base", &SPannerType);
    module_add_object(m, "Panner_base", &PannerType);
    module_add_object(m, "Pan_base", &PanType);
    module_add_object(m, "SPan_base", &SPanType);
    module_add_object(m, "Switcher_base", &SwitcherType);
    module_add_object(m, "Switch_base", &SwitchType);
    module_add_object(m, "Selector_base", &SelectorType);
    module_add_object(m, "VoiceManager_base", &VoiceManagerType);
    module_add_object(m, "Mixer_base", &MixerType);
    module_add_object(m, "MixerVoice_base", &MixerVoiceType);
    module_add_object(m, "Counter_base", &CounterType);
    module_add_object(m, "Count_base", &CountType);
    module_add_object(m, "Thresh_base", &ThreshType);
    module_add_object(m, "Percent_base", &PercentType);
    module_add_object(m, "Timer_base", &TimerType);
    module_add_object(m, "Select_base", &SelectType);
    module_add_object(m, "Change_base", &ChangeType);
    module_add_object(m, "Score_base", &ScoreType);
    module_add_object(m, "Freeverb_base", &FreeverbType);
    module_add_object(m, "WGVerb_base", &WGVerbType);
    module_add_object(m, "Chorus_base", &ChorusType);
    module_add_object(m, "Convolve_base", &ConvolveType);
    module_add_object(m, "IRWinSinc_base", &IRWinSincType);
    module_add_object(m, "IRPulse_base", &IRPulseType);
    module_add_object(m, "IRAverage_base", &IRAverageType);
    module_add_object(m, "IRFM_base", &IRFMType);
    module_add_object(m, "Granulator_base", &GranulatorType);
    module_add_object(m, "Looper_base", &LooperType);
    module_add_object(m, "LooperTimeStream_base", &LooperTimeStreamType);
    module_add_object(m, "Harmonizer_base", &HarmonizerType);
    module_add_object(m, "Print_base", &PrintType);
    module_add_object(m, "M_Sin_base", &M_SinType);
    module_add_object(m, "M_Cos_base", &M_CosType);
    module_add_object(m, "M_Tan_base", &M_TanType);
    module_add_object(m, "M_Abs_base", &M_AbsType);
    module_add_object(m, "M_Sqrt_base", &M_SqrtType);
    module_add_object(m, "M_Log_base", &M_LogType);
    module_add_object(m, "M_Log2_base", &M_Log2Type);
    module_add_object(m, "M_Log10_base", &M_Log10Type);
    module_add_object(m, "M_Pow_base", &M_PowType);
    module_add_object(m, "M_Atan2_base", &M_Atan2Type);
    module_add_object(m, "M_Floor_base", &M_FloorType);
    module_add_object(m, "M_Ceil_base", &M_CeilType);
    module_add_object(m, "M_Round_base", &M_RoundType);
    module_add_object(m, "M_Tanh_base", &M_TanhType);
    module_add_object(m, "M_Exp_base", &M_ExpType);
    module_add_object(m, "Snap_base", &SnapType);
    module_add_object(m, "Interp_base", &InterpType);
    module_add_object(m, "SampHold_base", &SampHoldType);
    module_add_object(m, "DBToA_base", &DBToAType);
    module_add_object(m, "AToDB_base", &AToDBType);
    module_add_object(m, "Scale_base", &ScaleType);
    module_add_object(m, "CentsToTranspo_base", &CentsToTranspoType);
    module_add_object(m, "TranspoToCents_base", &TranspoToCentsType);
    module_add_object(m, "MToF_base", &MToFType);
    module_add_object(m, "FToM_base", &FToMType);
    module_add_object(m, "MToT_base", &MToTType);
    module_add_object(m, "FFTMain_base", &FFTMainType);
    module_add_object(m, "FFT_base", &FFTType);
    module_add_object(m, "IFFT_base", &IFFTType);
    module_add_object(m, "IFFTMatrix_base", &IFFTMatrixType);
    module_add_object(m, "CarToPol_base", &CarToPolType);
    module_add_object(m, "PolToCar_base", &PolToCarType);
    module_add_object(m, "FrameDeltaMain_base", &FrameDeltaMainType);
    module_add_object(m, "FrameDelta_base", &FrameDeltaType);
    module_add_object(m, "FrameAccum_base", &FrameAccumType);
    module_add_object(m, "FrameAccumMain_base", &FrameAccumMainType);
    module_add_object(m, "VectralMain_base", &VectralMainType);
    module_add_object(m, "Vectral_base", &VectralType);
    module_add_object(m, "Min_base", &MinType);
    module_add_object(m, "Max_base", &MaxType);
    module_add_object(m, "Delay1_base", &Delay1Type);
    module_add_object(m, "RCOsc_base", &RCOscType);
    module_add_object(m, "Yin_base", &YinType);
    module_add_object(m, "SVF_base", &SVFType);
    module_add_object(m, "SVF2_base", &SVF2Type);
    module_add_object(m, "Average_base", &AverageType);
    module_add_object(m, "Spectrum_base", &SpectrumType);
    module_add_object(m, "Reson_base", &ResonType);
    module_add_object(m, "Resonx_base", &ResonxType);
    module_add_object(m, "ButLP_base", &ButLPType);
    module_add_object(m, "ButHP_base", &ButHPType);
    module_add_object(m, "ButBP_base", &ButBPType);
    module_add_object(m, "ButBR_base", &ButBRType);
    module_add_object(m, "MoogLP_base", &MoogLPType);
    module_add_object(m, "PVAnal_base", &PVAnalType);
    module_add_object(m, "PVSynth_base", &PVSynthType);
    module_add_object(m, "PVTranspose_base", &PVTransposeType);
    module_add_object(m, "PVVerb_base", &PVVerbType);
    module_add_object(m, "PVGate_base", &PVGateType);
    module_add_object(m, "PVAddSynth_base", &PVAddSynthType);
    module_add_object(m, "PVCross_base", &PVCrossType);
    module_add_object(m, "PVMult_base", &PVMultType);
    module_add_object(m, "PVMorph_base", &PVMorphType);
    module_add_object(m, "PVFilter_base", &PVFilterType);
    module_add_object(m, "PVDelay_base", &PVDelayType);
    module_add_object(m, "PVBuffer_base", &PVBufferType);
    module_add_object(m, "PVShift_base", &PVShiftType);
    module_add_object(m, "PVAmpMod_base", &PVAmpModType);
    module_add_object(m, "PVFreqMod_base", &PVFreqModType);
    module_add_object(m, "PVBufLoops_base", &PVBufLoopsType);
    module_add_object(m, "PVBufTabLoops_base", &PVBufTabLoopsType);
    module_add_object(m, "PVMix_base", &PVMixType);
    module_add_object(m, "Granule_base", &GranuleType);
    module_add_object(m, "TableScale_base", &TableScaleType);
    module_add_object(m, "TrackHold_base", &TrackHoldType);
    module_add_object(m, "ComplexRes_base", &ComplexResType);
    module_add_object(m, "STReverb_base", &STReverbType);
    module_add_object(m, "STRev_base", &STRevType);
    module_add_object(m, "Pointer2_base", &Pointer2Type);
    module_add_object(m, "Centroid_base", &CentroidType);
    module_add_object(m, "AttackDetector_base", &AttackDetectorType);
    module_add_object(m, "SmoothDelay_base", &SmoothDelayType);
    module_add_object(m, "TrigBurster_base", &TrigBursterType);
    module_add_object(m, "TrigBurst_base", &TrigBurstType);
    module_add_object(m, "TrigBurstTapStream_base", &TrigBurstTapStreamType);
    module_add_object(m, "TrigBurstAmpStream_base", &TrigBurstAmpStreamType);
    module_add_object(m, "TrigBurstDurStream_base", &TrigBurstDurStreamType);
    module_add_object(m, "TrigBurstEndStream_base", &TrigBurstEndStreamType);
    module_add_object(m, "Scope_base", &ScopeType);
    module_add_object(m, "PeakAmp_base", &PeakAmpType);
    module_add_object(m, "MainParticle_base", &MainParticleType);
    module_add_object(m, "Particle_base", &ParticleType);
    module_add_object(m, "MainParticle2_base", &MainParticle2Type);
    module_add_object(m, "Particle2_base", &Particle2Type);
    module_add_object(m, "AtanTable_base", &AtanTableType);
    module_add_object(m, "Resample_base", &ResampleType);
    module_add_object(m, "Exprer_base", &ExprerType);
    module_add_object(m, "Expr_base", &ExprType);
    module_add_object(m, "PadSynthTable_base", &PadSynthTableType);
    module_add_object(m, "LogiMap_base", &LogiMapType);
    module_add_object(m, "SharedTable_base", &SharedTableType);
    module_add_object(m, "TableFill_base", &TableFillType);
    module_add_object(m, "TableScan_base", &TableScanType);
    module_add_object(m, "Expand_base", &ExpandType);
    module_add_object(m, "RMS_base", &RMSType);
    module_add_object(m, "MultiBandMain_base", &MultiBandMainType);
    module_add_object(m, "MultiBand_base", &MultiBandType);
    module_add_object(m, "M_Div_base", &M_DivType);
    module_add_object(m, "M_Sub_base", &M_SubType);
    module_add_object(m, "MMLMain_base", &MMLMainType);
    module_add_object(m, "MML_base", &MMLType);
    module_add_object(m, "MMLFreqStream_base", &MMLFreqStreamType);
    module_add_object(m, "MMLAmpStream_base", &MMLAmpStreamType);
    module_add_object(m, "MMLDurStream_base", &MMLDurStreamType);
    module_add_object(m, "MMLEndStream_base", &MMLEndStreamType);
    module_add_object(m, "MMLXStream_base", &MMLXStreamType);
    module_add_object(m, "MMLYStream_base", &MMLYStreamType);
    module_add_object(m, "MMLZStream_base", &MMLZStreamType);

    PyModule_AddStringConstant(m, "PYO_VERSION", PYO_VERSION);
#ifdef COMPILE_EXTERNALS
    EXTERNAL_OBJECTS
    PyModule_AddIntConstant(m, "WITH_EXTERNALS", 1);
#else
    PyModule_AddIntConstant(m, "WITH_EXTERNALS", 0);
#endif
#ifndef USE_DOUBLE
    PyModule_AddIntConstant(m, "USE_DOUBLE", 0);
#else
    PyModule_AddIntConstant(m, "USE_DOUBLE", 1);
#endif

#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}
