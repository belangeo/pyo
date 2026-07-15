/**************************************************
Declare a factory for each external object type.
The factory must create a per-interpreter heap type,
typically with PyType_FromModuleAndSpec(module, ...).
**************************************************/
PyTypeObject *PyoCreateGainType(PyObject *module);

/**********************************************************
This macro is called at runtime to include external objects
in "_pyo" module. Add a block for each external object that
creates the heap type for the current interpreter and adds
it to the module.
**********************************************************/
#define EXTERNAL_OBJECTS \
    do { \
        PyTypeObject *type = PyoCreateGainType(m); \
        if (type == NULL) \
            return NULL; \
        if (PyModule_AddObjectRef(m, "Gain_base", (PyObject *)type) < 0) { \
            Py_DECREF(type); \
            return NULL; \
        } \
        Py_DECREF(type); \
    } while (0); \
