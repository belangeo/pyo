#include <stdlib.h>
#include <dlfcn.h>
#include "Python.h"

#ifndef __m_pyo_h_
#define __m_pyo_h_

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
#define INLINE inline
extern "C" {
#else
#define INLINE
#endif

/* Unicode/string handling. */
#if PY_MAJOR_VERSION >= 3
#define PyInt_AsLong PyLong_AsLong
#define PY_STRING_CHECK(a) PyUnicode_Check(arg) 
#define PY_STRING_AS_STRING(a) PyUnicode_AsUTF8(a)
#else
#define PY_STRING_CHECK(a) (PyUnicode_Check(a) || PyBytes_Check(a))
#define PY_STRING_AS_STRING(a) PyBytes_AsString(a)
#endif

/* libpython handle. libpython must be made available to the program loaded
** in a new interpreter. */
static void *libpython_handle;

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
INLINE PyThreadState * pyo_new_interpreter(float sr, int bufsize, int chnls) {
    char msg[64];
    PyThreadState *interp;
    if(!Py_IsInitialized()) {
        Py_InitializeEx(0);
        PyEval_InitThreads();
        PyEval_ReleaseLock();
    }

    /* This call hardcodes 2.7 as the python version to be used to embed pyo in
       a C or C++ program. This is not a good idea and must be fixed when everthing
       is stable.
    */
    if (libpython_handle == NULL) {
        libpython_handle = dlopen("libpython2.7.so", RTLD_LAZY | RTLD_GLOBAL);
    }

    PyEval_AcquireLock();              /* get the GIL */
    interp = Py_NewInterpreter();      /* add a new sub-interpreter */
    PyRun_SimpleString("from pyo import *");
    sprintf(msg, "_s_ = Server(%f, %d, %d, 1, 'embedded')", sr, chnls, bufsize);
    PyRun_SimpleString(msg);
    PyRun_SimpleString("_s_.boot()\n_s_.start()\n_s_.setServer()");
    PyRun_SimpleString("_in_address_ = _s_.getInputAddr()");
    PyRun_SimpleString("_out_address_ = _s_.getOutputAddr()");
    PyRun_SimpleString("_server_id_ = _s_.getServerID()");
    PyRun_SimpleString("_emb_callback_ = _s_.getEmbedICallbackAddr()");
    PyEval_ReleaseThread(interp);
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
    char *address;
    unsigned long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_in_address_");
    address = PY_STRING_AS_STRING(obj);
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
    char *address;
    unsigned long long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_in_address_");
    address = PY_STRING_AS_STRING(obj);
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
    char *address;
    unsigned long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_out_address_");
    address = PY_STRING_AS_STRING(obj);
    uadd = strtoul(address, NULL, 0);
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
    char *address;
    unsigned long uadd;
    PyEval_AcquireThread(interp);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_emb_callback_");
    address = PY_STRING_AS_STRING(obj);
    uadd = strtoul(address, NULL, 0);
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
    id = PyInt_AsLong(obj);
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
    PyEval_AcquireThread(interp);
    PyRun_SimpleString("_s_.setServer()\n_s_.stop()\n_s_.shutdown()");
    PyEval_ReleaseThread(interp);

    /* Old method (causing segfault) */
    //PyEval_AcquireThread(interp);
    //Py_EndInterpreter(interp);
    //PyEval_ReleaseLock();

    /* New method (seems to be ok) */
    PyThreadState_Swap(interp);
    PyThreadState_Swap(NULL);
    PyThreadState_Clear(interp);
    PyThreadState_Delete(interp);

    if (libpython_handle != NULL) {
        dlclose(libpython_handle);
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
    started = PyInt_AsLong(obj);
    PyEval_ReleaseThread(interp);
    return started;
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
*/
INLINE int pyo_exec_file(PyThreadState *interp, const char *file, char *msg, int add) {
    int ok, err = 0;
    PyObject *module, *obj;
    PyEval_AcquireThread(interp);
    sprintf(msg, "import os\n_ok_ = os.path.isfile('./%s')", file);
    PyRun_SimpleString(msg);
    sprintf(msg, "if not _ok_:\n    _ok_ = os.path.isfile('%s')", file);
    PyRun_SimpleString(msg);
    module = PyImport_AddModule("__main__");
    obj = PyObject_GetAttrString(module, "_ok_");
    ok = PyInt_AsLong(obj);
    if (ok) {
        sprintf(msg, "try:\n    exec(open('./%s').read())\nexcept:\n    exec(open('%s').read())",
                file, file);
        if (!add) {
            PyRun_SimpleString("_s_.setServer()\n_s_.stop()\n_s_.shutdown()");
            PyRun_SimpleString("_s_.boot(newBuffer=False).start()");
        }
        PyRun_SimpleString(msg);
    }
    else
        err = 1;
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
*/
INLINE int pyo_exec_statement(PyThreadState *interp, char *msg, int debug) {
    int err = 0;
    if (debug) {
        PyObject *module, *obj;
        char pp[26] = "_error_=None\ntry:\n    ";
        memmove(msg + strlen(pp), msg, strlen(msg)+1);
        memmove(msg, pp, strlen(pp));
        strcat(msg, "\nexcept Exception, _e_:\n    _error_=str(_e_)");
        PyEval_AcquireThread(interp);
        PyRun_SimpleString(msg);
        module = PyImport_AddModule("__main__");
        obj = PyObject_GetAttrString(module, "_error_");
        if (obj != Py_None) {
            strcpy(msg, PY_STRING_AS_STRING(obj));
            err = 1;
        }
        PyEval_ReleaseThread(interp);
    }
    else {
        PyEval_AcquireThread(interp);
        PyRun_SimpleString(msg);
        PyEval_ReleaseThread(interp);
    }
    return err;
}

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
}
#endif

#endif /* __m_pyo_h_  */
