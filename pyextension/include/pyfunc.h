#ifndef PYFUNC_H
#define PYFUNC_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "utils.h"

#define PyBool_AsCBool(x) (x == Py_True ? true : false)
#define print_obj(obj) PyObject_Print( ( (PyObject *)obj ), stdout, Py_PRINT_RAW ); printf("\n")
#define print_type_name(obj) printf("Python object type name: %s\n", ( (PyObject *)obj )->ob_type->tp_name)

PyObject *s_extract_keyword(PyObject *s_text, PyObject *d_keywords);
PyObject *s_cal_tfidf(PyObject *l_corpus, PyObject *d_keywords);
PyObject *s_reverse(PyObject *s_text);

#endif	/* PYFUNC_H */
