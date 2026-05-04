/**************************************************************************
 * Copyright 2014-2020 Olivier Belanger                                   *
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
#include <stdlib.h>

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

#include "Python.h"

#ifndef __m_pyo_h_
#define __m_pyo_h_

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
#define INLINE inline
extern "C" {
#else
#define INLINE
#endif

/* sub-interpreter counter, necessary to determine when to shut down Python */
static int py_instance_count = 0;
/* global interpreter boolean, used to initialize the global interpreter only once for all running [pyo~] objects */
static int py_global_initialized = 0;
/* global interpreter instance */
static PyThreadState *main_tstate = NULL;

/* Function prototypes to redirect Python's stdout to C
 * Needed because it is called before it is defined
*/
static inline void redirect_stdout_to_c(void);
static PyObject* PyInit_cstdout(void);

/*
** Creates a global python interpreter.
** This is called by every new [pyo~] object,
** but the global interpreter is initialized only once, using the py_global_initialized variable
*/
static void pyo_python_global_init(void)
{
    if (!py_global_initialized) {
		/* redirect Python's stdout to m_pyo.h from where we can get it in here */
		PyImport_AppendInittab("cstdout", PyInit_cstdout);

        Py_Initialize();

        /* Save main interpreter state */
        main_tstate = PyThreadState_Get();

        /* Release GIL */
        PyEval_ReleaseThread(main_tstate);

        py_global_initialized = 1;
    }
}

/*
** Creates a new python interpreter and starts a pyo server in it.
** Each instance of pyo, in order to be fully independent of other
** instances, must be started in its own interpreter. An instance
** can be an object in a programming language or a plugin in a daw.
**
** arguments:
**  sr : float, host sampling rate.
**  bufsize : int, host buffer size.
**  chnls : int, number of in/out channels of the pyo server.
**
** returns the new python thread's interpreter state.
*/
INLINE PyThreadState * pyo_new_interpreter(float sr, int bufsize, int ichnls, int ochnls) {
    char msg[128];
    PyThreadState *interp;

	pyo_python_global_init();

	PyEval_AcquireThread(main_tstate);

    interp = Py_NewInterpreter();   /* add a new sub-interpreter */
	redirect_stdout_to_c();

    /* On MacOS, trying to import wxPython in embedded python hang the process.
	 * On Linux, it crashes the host (at least in Pd)
	 */
    PyRun_SimpleString("import os; os.environ['PYO_GUI_WX'] = '0'");

    /* Force embedded audio server. */
    PyRun_SimpleString("os.environ['PYO_SERVER_AUDIO'] = 'embedded'");

    /* Set the default BPM (60 beat per minute). */
    PyRun_SimpleString("BPM = 60.0");

    PyRun_SimpleString("from pyo import *");
    sprintf(msg, "_s_ = Server(sr=%f, nchnls=%d, buffersize=%d, duplex=1, ichnls=%d)", sr, ochnls, bufsize, ichnls);
    PyRun_SimpleString(msg);
    PyRun_SimpleString("_s_.boot()\n_s_.start()\n_s_.setServer()");
    PyRun_SimpleString("_server_id_ = _s_.getServerID()");

    /*
    ** printf %p specifier behaves differently in Linux/MacOS and Windows.
    */

#if defined(_WIN32)
    PyRun_SimpleString("_in_address_ = '0x' + _s_.getInputAddr().lower()");
    PyRun_SimpleString("_out_address_ = '0x' + _s_.getOutputAddr().lower()");
    PyRun_SimpleString("_emb_callback_ = '0x' + _s_.getEmbedICallbackAddr().lower()");
#else
    PyRun_SimpleString("_in_address_ = _s_.getInputAddr()");
    PyRun_SimpleString("_out_address_ = _s_.getOutputAddr()");
    PyRun_SimpleString("_emb_callback_ = _s_.getEmbedICallbackAddr()");
#endif

    PyEval_ReleaseThread(interp);

	/* increase the sub-interpreter counter */
	py_instance_count++;

    return interp;
}

/*
** Returns the address, as unsigned long, of the pyo input buffer.
** Used this function if pyo's audio samples resolution is 32-bit.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**
** returns an "unsigned long" that should be recast to a float pointer.
*/
INLINE unsigned long pyo_get_input_buffer_address(PyThreadState *interp) {
    PyObject *module, *obj;
    const char *address;
    unsigned long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_in_address_");
    address = PyUnicode_AsUTF8(obj);
    uadd = strtoul(address, NULL, 0);
    PyEval_ReleaseThread(interp);
    return uadd;
}

/*
** Returns the address, as unsigned long long, of the pyo input buffer.
** Used this function if pyo's audio samples resolution is 64-bit.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**
** returns an "unsigned long long" that should be recast to a double pointer.
*/
INLINE unsigned long long pyo_get_input_buffer_address_64(PyThreadState *interp) {
    PyObject *module, *obj;
    const char *address;
    unsigned long long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_in_address_");
    address = PyUnicode_AsUTF8(obj);
    uadd = strtoull(address, NULL, 0);
    PyEval_ReleaseThread(interp);
    return uadd;
}

/*
** Returns the address, as unsigned long, of the pyo output buffer.
** Used this function if pyo's audio samples resolution is 32-bit.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**
** returns an "unsigned long" that should be recast to a float pointer.
*/
INLINE unsigned long pyo_get_output_buffer_address(PyThreadState *interp) {
    PyObject *module, *obj;
    const char *address;
    unsigned long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_out_address_");
    address = PyUnicode_AsUTF8(obj);
    uadd = strtoul(address, NULL, 0);
    PyEval_ReleaseThread(interp);
    return uadd;
}

/*
** Returns the address, as unsigned long long, of the pyo output buffer.
** Used this function if pyo's audio samples resolution is 64-bit.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**
** returns an "unsigned long long" that should be recast to a double pointer.
*/
INLINE unsigned long long pyo_get_output_buffer_address_64(PyThreadState *interp) {
    PyObject *module, *obj;
    const char *address;
    unsigned long long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_out_address_");
    address = PyUnicode_AsUTF8(obj);
    uadd = strtoull(address, NULL, 0);
    PyEval_ReleaseThread(interp);
    return uadd;
}

/*
** Returns the address, as unsigned long, of the pyo embedded callback.
** This callback must be called in the host's perform routine whenever
** pyo has to compute a new buffer of samples.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**
** returns an "unsigned long" that should be recast to a void pointer.
**
** The callback should be called with the server id (int) as argument.
**
** Prototype:
** void (*callback)(int);
*/
INLINE unsigned long pyo_get_embedded_callback_address(PyThreadState *interp) {
    PyObject *module, *obj;
    const char *address;
    unsigned long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_emb_callback_");
    address = PyUnicode_AsUTF8(obj);
    uadd = strtoul(address, NULL, 0);
    PyEval_ReleaseThread(interp);
    return uadd;
}

/*
** Returns the address, as unsigned long long, of the pyo embedded callback.
** This callback must be called in the host's perform routine whenever
** pyo has to compute a new buffer of samples.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**
** returns an "unsigned long long" that should be recast to a void pointer.
**
** The callback should be called with the server id (int) as argument.
**
** Prototype:
** void (*callback)(int);
*/
INLINE unsigned long long pyo_get_embedded_callback_address_64(PyThreadState *interp) {
    PyObject *module, *obj;
    const char *address;
    unsigned long long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_emb_callback_");
    address = PyUnicode_AsUTF8(obj);
    uadd = strtoull(address, NULL, 0);
    PyEval_ReleaseThread(interp);
    return uadd;
}

/*
** Returns the pyo server id of this thread, as an integer.
** The id must be pass as argument to the callback function.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**
** returns an integer.
*/
INLINE int pyo_get_server_id(PyThreadState *interp) {
    PyObject *module, *obj;
    int id;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_server_id_");
    id = PyLong_AsLong(obj);
    PyEval_ReleaseThread(interp);
    return id;
}

/*
** Closes the interpreter linked to the thread state given as argument.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
*/
INLINE void pyo_end_interpreter(PyThreadState *interp) {
    /* Clean up pyo's server. */
	if (interp !=NULL) {
		PyEval_AcquireThread(interp);
    	PyRun_SimpleString("_s_.setServer()\n_s_.stop()\n_s_.shutdown()");
    	PyEval_ReleaseThread(interp);

		//PyGILState_STATE state = PyGILState_Ensure();
    	PyGILState_Ensure();

		Py_EndInterpreter(interp);
	}

	/* decrement the sub-interpreter counter */
	if (py_instance_count > 0) py_instance_count--;
	/* if we delete the last sub-interpreter and the global interpreter is active
	 * finalize it all
	**/
	if (py_instance_count == 0 && py_global_initialized) {
		PyEval_AcquireThread(main_tstate);
        Py_FinalizeEx();
		py_global_initialized = 0;
		main_tstate = NULL;
	}
}

/*
** Shutdown and reboot the pyo server while keeping current in/out buffers.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
*/
INLINE void pyo_server_reboot(PyThreadState *interp) {
    PyEval_AcquireThread(interp);
    PyRun_SimpleString("_s_.setServer()\n_s_.stop()\n_s_.shutdown()");
    PyRun_SimpleString("_s_.boot(newBuffer=False).start()");
    PyEval_ReleaseThread(interp);
}

/*
** Reboot the pyo server with new sampling rate and buffer size.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**  sr : float, host sampling rate.
**  bufsize : int, host buffer size.
*/
INLINE void pyo_set_server_params(PyThreadState *interp, float sr, int bufsize) {
    char msg[64];
    PyEval_AcquireThread(interp);
    PyRun_SimpleString("_s_.setServer()\n_s_.stop()\n_s_.shutdown()");
    sprintf(msg, "_s_.setSamplingRate(%f)", sr);
    PyRun_SimpleString(msg);
    sprintf(msg, "_s_.setBufferSize(%d)", bufsize);
    PyRun_SimpleString(msg);
    PyRun_SimpleString("_s_.boot(newBuffer=False).start()");
    PyEval_ReleaseThread(interp);
}

/*
** This function can be used to pass the DAW's bpm value to the
** python interpreter. Changes the value of the BPM variable in
** the interpreter's global scope. (which defaults to 60).
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**  bpm : double, new BPM.
*/
INLINE void pyo_set_bpm(PyThreadState *interp, double bpm) {
    char msg[64];
    PyEval_AcquireThread(interp);
    sprintf(msg, "BPM = %f", bpm);
    PyRun_SimpleString(msg);
    PyEval_ReleaseThread(interp);
}

/*
** Add a MIDI event in the pyo server processing chain. When used in
** an embedded framework, pyo can't open MIDI ports by itself. MIDI
** inputs must be handled by the host program and sent to pyo with
** the pyo_add_midi_event function.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**  status : int, status byte.
**  data1 : int, first data byte.
**  data2 : int, second data byte.
*/
INLINE void pyo_add_midi_event(PyThreadState *interp, int status, int data1, int data2) {
    char msg[64];
    PyEval_AcquireThread(interp);
    sprintf(msg, "_s_.addMidiEvent(%d, %d, %d)", status, data1, data2);
    PyRun_SimpleString(msg);
    PyEval_ReleaseThread(interp);
}

/*
** Returns 1 if the pyo server is started for the given thread,
** Otherwise returns 0.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
*/
INLINE int pyo_is_server_started(PyThreadState *interp) {
    int started;
    PyObject *module, *obj;
    PyEval_AcquireThread(interp);
    PyRun_SimpleString("started = _s_.getIsStarted()");
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "started");
    started = PyLong_AsLong(obj);
    PyEval_ReleaseThread(interp);
    return started;
}

/*
** These two functions are used to execute code, either as a statement
** or as a loaded file.
** They are called by pyo_exec_file() and pyo_exec_statement()
*/
INLINE void copy_pyunicode_to_msg(PyObject *u, char *msg)
{
    if (!u) return;
    const char *s = PyUnicode_AsUTF8(u);
    if (s) {
        strcpy(msg, s);
    }
}

INLINE int pyo_exec_code(char *msg, const char *filename, int debug)
{
	int err = 0;
    PyObject *codeObj = Py_CompileString(msg, filename, Py_file_input);
    if (codeObj == NULL) {
        if (debug) {
            /* format compile error */
            PyObject *ptype=NULL,*pvalue=NULL,*ptraceback=NULL;
            PyErr_Fetch(&ptype,&pvalue,&ptraceback);
            PyErr_NormalizeException(&ptype,&pvalue,&ptraceback);

            PyObject *tbmod = PyImport_ImportModule("traceback");
            if (tbmod) {
                PyObject *fmt = PyObject_GetAttrString(tbmod, "format_exception");
                if (fmt && PyCallable_Check(fmt)) {
                    PyObject *tb_arg = ptraceback ? ptraceback : Py_None;
                    PyObject *exc_list = PyObject_CallFunctionObjArgs(fmt,
                        ptype?ptype:Py_None, pvalue?pvalue:Py_None, tb_arg, NULL);
                    if (exc_list) {
                        PyObject *sep = PyUnicode_FromString("");
                        PyObject *exc_str = PyUnicode_Join(sep, exc_list);
                        if (exc_str) { copy_pyunicode_to_msg(exc_str, msg); Py_DECREF(exc_str); }
                        Py_XDECREF(sep);
                        Py_DECREF(exc_list);
                    }
                    Py_XDECREF(fmt);
                }
                Py_DECREF(tbmod);
            }
            Py_XDECREF(ptype); Py_XDECREF(pvalue); Py_XDECREF(ptraceback);
        }
        err = 1;
    }
	else {
        PyObject *mainmod = PyImport_AddModule("__main__");  // borrowed
        PyObject *globals = PyModule_GetDict(mainmod);       // borrowed

        PyObject *res = PyEval_EvalCode((PyObject *)codeObj, globals, globals);
        if (res == NULL) {
            if (debug) {
				/* runtime error formatting */
                PyObject *ptype=NULL,*pvalue=NULL,*ptraceback=NULL;
                PyErr_Fetch(&ptype,&pvalue,&ptraceback);
                PyErr_NormalizeException(&ptype,&pvalue,&ptraceback);

                PyObject *tbmod = PyImport_ImportModule("traceback");
                if (tbmod) {
                    PyObject *fmt = PyObject_GetAttrString(tbmod, "format_exception");
                    if (fmt && PyCallable_Check(fmt)) {
                        PyObject *tb_arg = ptraceback ? ptraceback : Py_None;
                        PyObject *exc_list = PyObject_CallFunctionObjArgs(fmt,
								ptype?ptype:Py_None, pvalue?pvalue:Py_None, tb_arg, NULL);
                        if (exc_list) {
                            PyObject *sep = PyUnicode_FromString("");
                            PyObject *exc_str = PyUnicode_Join(sep, exc_list);
                            if (exc_str) { copy_pyunicode_to_msg(exc_str, msg); Py_DECREF(exc_str); }
                            Py_XDECREF(sep);
                            Py_DECREF(exc_list);
                        }
                        Py_XDECREF(fmt);
                    }
                    Py_DECREF(tbmod);
                }
                Py_XDECREF(ptype); Py_XDECREF(pvalue); Py_XDECREF(ptraceback);
            }
            err = 1;
        }
		else {
            Py_DECREF(res);
        }
        Py_DECREF(codeObj);
    }
	return err;
}

/*
** Execute a python script "file" in the given thread's interpreter (interp).
** A pre-allocated string "msg" must be given to create the python command
** used for error handling. An integer "add" is needed to indicate if the
** pyo server should be reboot or not.
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**  file : char *, filename to execute as a python script. The file is first
**                 searched in the current working directory. If not found,
**                 the module will try to open it as an absolute path.
**  msg : char *, pre-allocated string used to create the python command
**                used for error handling.
**  add, int, if positive, the commands in the file will be added to whatever
**            is already running in the pyo server. If 0, the server will be
**            shutdown and reboot before executing the file.
**
** returns 0 (no error), 1 (failed to open the file) or 2 (bad code in file).
*/
INLINE int pyo_exec_file(PyThreadState *interp, const char *file, char *msg, int add, int debug) {
    int ok, isrel, err = 0;
    PyObject *module;
    PyEval_AcquireThread(interp);
    sprintf(msg, "import os\n_isrel_ = True\n_ok_ = os.path.isfile('./%s')", file);
    PyRun_SimpleString(msg);
    sprintf(msg, "if not _ok_:\n    _isrel_ = False\n    _ok_ = os.path.isfile('%s')", file);
    PyRun_SimpleString(msg);
    module = PyImport_AddModule("__main__");
    ok = PyLong_AsLong(PyObject_GetAttrString(module, "_ok_"));
    isrel = PyLong_AsLong(PyObject_GetAttrString(module, "_isrel_"));
    if (ok) {
        if (!add) {
            PyRun_SimpleString("_s_.setServer()\n_s_.stop()\n_s_.shutdown()");
            PyRun_SimpleString("_s_.boot(newBuffer=False).start()");
        }
        if (isrel) {
            sprintf(msg, "exec(open('./%s').read())", file);
        } else {
            sprintf(msg, "exec(open('%s').read())", file);
        }
		err = pyo_exec_code(msg, file, debug);
		/* error code 1 means file not loaded
		 * so if we get an error in the code, we increment the error code by one
		 */
		if (err) err++;
    }
    else {
		err = 1;
    }
    PyEval_ReleaseThread(interp);
    return err;
}

/*
** Execute a python statement "msg" in the thread's interpreter "interp".
** If "debug" is true, the statement will be executed in a try - except
** block. The error message, if any, will be write back in the *msg
** pointer and the function will return 1. If no error occured, the
** function returned 0. If debug is false, the statement is executed
** without any error checking (unsafe but faster).
**
** arguments:
**  interp : pointer, pointer to the targeted Python thread state.
**  msg : char *, pointer to a string containing the statement to execute.
**                In debug mode, if an error occured, the output log will
**                be write back in this string.
**  debug, int, if positive, the commands will be executed in a try-except
**              statement. If 0, there will be no error checking, which is
**              much faster.
**
** returns 0 (no error) or 1 (bad code in file).
*/

INLINE int pyo_exec_statement(PyThreadState *interp, char *msg, int debug) {
    static long input_counter = 0;  /* REPL-style line counter */

    int err = 0;

    PyEval_AcquireThread(interp);

    char filename[64];
    snprintf(filename, sizeof(filename), "<python-input-%ld>", input_counter++);

	err = pyo_exec_code(msg, filename, debug);
    PyEval_ReleaseThread(interp);
    return err;
}

/*
** Functions to redirect Python's stdout in here
*/
/* --------- Message queue for stdout ---------- */

typedef struct MsgNode {
    char *msg;
    struct MsgNode *next;
} MsgNode;

static MsgNode *g_msg_head = NULL;
static MsgNode *g_msg_tail = NULL;
static pthread_mutex_t g_msg_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Add message to queue (called by Python thread in cstdout_write) */
static void pyo_enqueue_stdout(const char *s) {
    if (!s) return;

    MsgNode *node = (MsgNode*)malloc(sizeof(MsgNode));
    if (!node) return;

    node->msg = strdup(s);  /* copy string */
    node->next = NULL;

    pthread_mutex_lock(&g_msg_mutex);
    if (g_msg_tail) {
        g_msg_tail->next = node;
        g_msg_tail = node;
    } else {
        g_msg_head = g_msg_tail = node;
    }
    pthread_mutex_unlock(&g_msg_mutex);
}

/* Poll one message (called by host program, e.g. in update loop) */
/* Returns 1 if a message was dequeued, 0 otherwise */
INLINE int pyo_dequeue_stdout(char **out_msg) {
    pthread_mutex_lock(&g_msg_mutex);
    MsgNode *node = g_msg_head;
    if (!node) {
        pthread_mutex_unlock(&g_msg_mutex);
        return 0;
    }
    g_msg_head = node->next;
    if (!g_msg_head) g_msg_tail = NULL;
    pthread_mutex_unlock(&g_msg_mutex);

    *out_msg = node->msg;  /* transfer ownership */
    free(node);
    return 1;
}

/* --------- cstdout module hooks ---------- */
static PyObject* cstdout_write(PyObject *self, PyObject *args) {
    const char *s;
	(void)(self); /* unused */
	(void)(args);
    if (!PyArg_ParseTuple(args, "s", &s)) return NULL;

	pyo_enqueue_stdout(s);

   	Py_RETURN_NONE;
}

static PyObject* cstdout_flush(PyObject *self, PyObject *args) {
	(void)(self); /* unused */
	(void)(args);
    Py_RETURN_NONE;
}

/* Module method table */
static PyMethodDef cstdout_methods[] = {
    {"write",  cstdout_write, METH_VARARGS, "Write to C stdout"},
    {"flush",  cstdout_flush, METH_VARARGS, "Flush (no-op)"},
    {NULL, NULL, 0, NULL}
};

/* Module definition */
static struct PyModuleDef cstdout_moduledef = {
    PyModuleDef_HEAD_INIT,
    "cstdout",     /* m_name */
    "C stdout/stderr receiver", /* m_doc */
    -1,            /* m_size */
    cstdout_methods,
    NULL, NULL, NULL, NULL
};

static PyObject* PyInit_cstdout(void) {
    return PyModule_Create(&cstdout_moduledef);
}

/* Call this after Py_Initialize() while holding the GIL/threadstate. */
static inline void redirect_stdout_to_c(void) {
    /* Build and run Python code that sets sys.stdout/sys.stderr to call cstdout.write */
    const char *code =
        "import sys, cstdout\n"
        "class _CStdout:\n"
        "    def write(self, s):\n"
        "        # ensure only strings are forwarded\n"
        "        if s is None:\n"
        "            return\n"
        "        cstdout.write(str(s))\n"
        "    def flush(self):\n"
        "        cstdout.flush()\n"
        "sys.stdout = _CStdout()\n"
        "sys.stderr = _CStdout()\n";

    if (PyRun_SimpleString(code) != 0) {
        PyErr_Print();
    }
}
/*
** Functions to redirect Python's stdout in here done
*/

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
}
#endif

#endif /* __m_pyo_h_  */
