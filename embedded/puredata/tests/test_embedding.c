/* Exercise embedding without requiring an installed pyo package. */
#include <assert.h>
#include "../../m_pyo.h"

static int fail_import = 0;

static PyObject *test_check_available(PyObject *self, PyObject *args) {
    (void)self;
    (void)args;
    if (fail_import) {
        PyErr_SetString(PyExc_RuntimeError, "deliberate server initialization failure");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *PyInit_test_pyo(void) {
    static PyMethodDef methods[] = {
        {"_check_available", test_check_available, METH_NOARGS, NULL},
        {NULL, NULL, 0, NULL}
    };
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT, "pyo", NULL, -1, methods, NULL, NULL, NULL, NULL
    };
    const char *code =
        "class Server:\n"
        "    def __init__(self, **kwargs): _check_available()\n"
        "    def boot(self): return self\n"
        "    def start(self): return self\n"
        "    def stop(self): return self\n"
        "    def shutdown(self): return self\n"
        "    def setServer(self): pass\n"
        "    def getInputAddr(self): return '00000001ABCDEF00'\n"
        "    def getOutputAddr(self): return '0x1abcdef10'\n"
        "    def getEmbedICallbackAddr(self): return '00000001ABCDEF20'\n"
        "    def getServerAddr(self): return '00000001ABCDEF30'\n";
    PyObject *module, *result;
    if (fail_import) {
        PyErr_SetString(PyExc_ImportError, "deliberate import failure: 100% unavailable");
        return NULL;
    }
    module = PyModule_Create(&moduledef);
    if (module == NULL) return NULL;
    result = PyRun_String(code, Py_file_input, PyModule_GetDict(module), PyModule_GetDict(module));
    if (result == NULL) {
        Py_DECREF(module);
        return NULL;
    }
    Py_DECREF(result);
    return module;
}

static void check_addresses(PyThreadState *interp) {
    assert(pyo_get_input_buffer_address(interp) == (uintptr_t)0x1ABCDEF00ULL);
    assert(pyo_get_output_buffer_address(interp) == (uintptr_t)0x1ABCDEF10ULL);
    assert(pyo_get_embedded_callback_address(interp) == (uintptr_t)0x1ABCDEF20ULL);
    assert(pyo_get_server_address(interp) == (uintptr_t)0x1ABCDEF30ULL);
    assert(pyo_get_input_buffer_address_64(interp) == 0x1ABCDEF00ULL);
    assert(pyo_get_output_buffer_address_64(interp) == 0x1ABCDEF10ULL);
    assert(pyo_get_embedded_callback_address_64(interp) == 0x1ABCDEF20ULL);
}

int main(void) {
    PyThreadState *first, *second;
    char *msg;
    int reported_failure = 0;
    assert(sizeof(uintptr_t) == 8);
    assert(PyImport_AppendInittab("pyo", PyInit_test_pyo) == 0);

    /* Failed creation must restore the main thread and release its GIL. */
    fail_import = 1;
    assert(pyo_new_interpreter(44100, 64, 2, 2) == NULL);
    while (pyo_dequeue_stdout(&msg)) {
        if (strstr(msg, "deliberate import failure")) reported_failure = 1;
        free(msg);
    }
    assert(reported_failure);
    fail_import = 0;
    first = pyo_new_interpreter(44100, 64, 2, 2);
    assert(first != NULL);
    check_addresses(first);
    second = pyo_new_interpreter(44100, 64, 2, 2);
    assert(second != NULL);

    /* Invalid and missing attributes must report errors without dereferencing NULL. */
    PyEval_AcquireThread(first);
    assert(PyRun_SimpleString("_in_address_ = '0x0x1234'\n"
                              "_out_address_ = None\n"
                              "del _server_addr_") == 0);
    PyEval_ReleaseThread(first);
    assert(pyo_get_input_buffer_address(first) == 0);
    assert(pyo_get_output_buffer_address(first) == 0);
    assert(pyo_get_server_address(first) == 0);
    pyo_end_interpreter(first);
    check_addresses(second);

    /* Failure while another instance is alive must leave that instance usable.
     * Python may reuse a single-phase built-in module between subinterpreters,
     * so the fake server also checks availability during construction. */
    fail_import = 1;
    assert(pyo_new_interpreter(44100, 64, 2, 2) == NULL);
    check_addresses(second);
    fail_import = 0;
    pyo_end_interpreter(second);

    /* The last deletion finalizes Python; a new object must initialize it again. */
    first = pyo_new_interpreter(44100, 64, 2, 2);
    assert(first != NULL);
    check_addresses(first);
    pyo_end_interpreter(first);
    pyo_end_interpreter(NULL);
    while (pyo_dequeue_stdout(&msg)) free(msg);
    puts("Embedding regression checks passed.");
    return 0;
}
