// #define NPY_NO_DEPRECATED_API NPY_1_8_API_VERSION

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/ndarraytypes.h>
#include <numpy/ufuncobject.h>
#include <numpy/halffloat.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "matrix.h"
#include "utilities.h"
#include "data_structure.h"
#include "mathtool.h"

/** ====================================================================================================
 * test
 **/

static PyObject *
pyextension_test(PyObject *self, PyObject *args)
{
    /* 用来保存从 python 传入的参数 */
    PyObject *X_from_python = NULL;

    if (!PyArg_ParseTuple (args, "O", &X_from_python))
        return NULL;

    /**
     * 使用 Py_Array_FROM_OTF 将输入数据转化为 PyArrayObject。
     * OTF 的意思应该是 Object Type Flags（瞎猜的）。
     * 注意：Py_Array_FROM_OTF 宏会使用另一个宏 PyArray_FromAny。
     * 任何使用 PyArray_FromAny 宏的，都会增加对象的低层引用，
     * 因此要使用 Py_DECREF 来减少该对象的引用。
     **/
    PyArrayObject *X_numpy = (PyArrayObject *)PyArray_FROM_OTF(
        X_from_python,      /* 传入数据 */
        NPY_DOUBLE,         /* numpy 的数据类型 */
        NPY_ARRAY_IN_ARRAY  /* flag?? */
    );

    if (X_numpy == NULL) return NULL;

    /* 获取长度 */
    npy_intp X_len = PyArray_SIZE(X_numpy);

    /* 获取 numpy 数组数据 buffer 的指针 */
    double *X_np_data = (double *) PyArray_DATA (X_numpy);

    puts("\npython 端的数据 X");
    print_obj(X_from_python);

    puts("\nX 转化为 ndarray 后的数据");
    for (npy_intp i = 0; i < X_len; i++) {
        fprintf(stdout, "%5.3f\t", X_np_data[i]);
        fflush(stdout);
    }

    /* 分配一个数组的内存 */
    double *X_carray = malloc(X_len * sizeof (double));
    if (X_carray == NULL) goto fail;

    for (npy_intp i = 0; i < X_len; i++)
        X_carray[i] = (double) i * i;

    /* 为刚分配的数组建立一个 ndarray wrapper */
    int nd = 2;
    npy_intp dims[] = {2, 3};
    PyObject *out = PyArray_SimpleNewFromData(
        nd,          /* 维度数 */
        dims,        /* 各维长度 */
        NPY_DOUBLE,  /* 数据类型 */
        X_carray     /* 数组数据 */
    );

    double *X_ca_data = PyArray_DATA((PyArrayObject *)out);
    puts("\n新建数组中的数据");
    for (npy_intp i = 0; i < X_len; i++) {
        fprintf(stdout, "%5.3f\t", X_ca_data[i]);
        fflush(stdout);
    }
    printf("\n");

    /**
     * 计算
     **/

    /* 返回的数据 */
    PyObject *val = Py_BuildValue ("O", out);

    /**
     * 释放内存后，在 python 端 val 的 ndarray 变量可用，
     * 但数据已不可用
     **/

    free(X_carray);
    Py_DECREF (X_numpy);
    return val;

fail:  /* 错误 */
    Py_DECREF (X_numpy);
    return NULL;
}

static PyObject *
pyextension_test_dict(PyObject *self, PyObject *args)
{
    PyObject *X_python;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &X_python))
        return NULL;  /* 类型不为字典 */

    print_obj(X_python);

    PyObject *city_info = PyDict_New();
    PyDict_SetItemString(city_info, "ShangHai", PyUnicode_FromString("China"));
    PyDict_SetItemString(city_info, "HongKong", PyUnicode_FromString("China"));
    PyDict_SetItemString(city_info, "北京", PyUnicode_FromString("中国"));

    return city_info;
}

static PyObject *
pyextension_test_class(PyObject *self, PyObject *args)
{
    PyObject *X_python;
    if (!PyArg_ParseTuple(args, "O", &X_python))
        return NULL;  /* 类型不为字典 */

    if (strcmp(X_python->ob_type->tp_name, "City")) {
        PyErr_SetString(PyExc_ValueError, "输入必需为 City 类实例");
        return NULL;
    }

    print_obj(X_python);
    print_obj(PyObject_Type(X_python));
    printf(PyObject_GetAttr(X_python, PyUnicode_FromString("info"))->ob_type->tp_name);
    printf("\n");
    PyObject_SetAttr(X_python, PyUnicode_FromString("size"), PyLong_FromLong(6340));

    // PyObject *Y = X_python->ob_type->tp_new();

    return Py_None;
}

static PyObject *
pyextension_test_include(PyObject *self, PyObject *args)
{
    /* 用来保存从 python 传入的参数 */
    PyObject *X_from_python = NULL;

    if (!PyArg_ParseTuple (args, "O!", &PyDict_Type, &X_from_python))
        return NULL;

    // PyArrayObject *sum = (PyArrayObject *)npmath_shannon_entropy(X_from_python);
    // PyArrayObject *sum = (PyArrayObject *)PyArray_Sum((PyArrayObject *)X_from_python, 0, NPY_DOUBLE, NULL);
    X_from_python = test_include(X_from_python);

    double *cArray = (double *)malloc(6 * sizeof(double));

    for (npy_intp i = 0; i < 6; i++)
        cArray[i] = (double)(i * i);

    int nd = 2;
    npy_intp dims[] = {2, 3};
    PyArrayObject *npArray = (PyArrayObject *)PyArray_SimpleNewFromData(
        nd,          /* 维度数 */
        dims,        /* 各维长度 */
        NPY_DOUBLE,  /* 数据类型 */
        cArray       /* 数组数据 */
    );
    // npArray = npmath_test(npArray);

    print_obj((PyObject *)npArray);

    /* 返回的数据 */
    // PyObject *result = Py_BuildValue ("O", sum);

    return X_from_python;
}

/** ====================================================================================================
 * ufunc: logit 
 **/

static void logit_long_double(char     **args,
                              npy_intp  *dimensions,
                              npy_intp  *steps,
                              void      *data)
{
    npy_intp n = dimensions[0];
    char *in = args[0], *out=args[1];
    npy_intp in_step = steps[0], out_step = steps[1];

    long double tmp;

    for (npy_intp i = 0; i < n; i++) {
        /*BEGIN main ufunc computation*/
        tmp = *(long double *)in;
        tmp /= 1-tmp;
        *((long double *)out) = logl(tmp);
        /*END main ufunc computation*/

        in += in_step;
        out += out_step;
    }
}

static void logit_double(char     **args,
                         npy_intp  *dimensions,
                         npy_intp  *steps,
                         void      *data)
{
    npy_intp n = dimensions[0];
    char *in = args[0], *out = args[1];
    npy_intp in_step = steps[0], out_step = steps[1];

    double tmp;

    for (npy_intp i = 0; i < n; i++) {
        /*BEGIN main ufunc computation*/
        tmp = *(double *)in;
        tmp /= 1-tmp;
        *((double *)out) = log(tmp);
        /*END main ufunc computation*/

        in += in_step;
        out += out_step;
    }
}

static void logit_float(char     **args,
                        npy_intp  *dimensions,
                        npy_intp  *steps,
                        void      *data)
{
    npy_intp n = dimensions[0];
    char *in=args[0], *out = args[1];
    npy_intp in_step = steps[0], out_step = steps[1];

    float tmp;

    for (npy_intp i = 0; i < n; i++) {
        /*BEGIN main ufunc computation*/
        tmp = *(float *)in;
        tmp /= 1-tmp;
        *((float *)out) = logf(tmp);
        /*END main ufunc computation*/

        in += in_step;
        out += out_step;
    }
}

/* 指向 logit 系列函数的数组 */
PyUFuncGenericFunction logit_funcs[] = {&logit_float,
                                        &logit_double,
                                        &logit_long_double};

/* 定义 logit 系列函数输入输出的数组 */
static char logit_types[] = {NPY_FLOAT, NPY_FLOAT,
                             NPY_DOUBLE,NPY_DOUBLE,
                             NPY_LONGDOUBLE, NPY_LONGDOUBLE};

/* logit 的一些数据 */
static void *logit_data[] = {NULL, NULL, NULL};


/** ====================================================================================================
 * ufunc:
 **/

static PyObject*
pyextension_class_counter(PyObject *self, PyObject *args)
{
    PyObject *X_obj, *result;
    if (!PyArg_ParseTuple (args, "O!", &PyArray_Type, &X_obj))
        return NULL;

    if ( !PyArray_ISUNSIGNED( (PyArrayObject *)X_obj ) ) {  /*不是无符号整数的情况*/
        PyErr_SetString(PyExc_ValueError, "输入的类型必需为无符号整型！");
        return NULL;
    }

    PyArrayObject *X_nparr = (PyArrayObject *)PyArray_FROM_OTF(
        X_obj,      /* 传入数据 */
        NPY_ULONG,  /* 传入数据类型为 unsigned long */
        NPY_ARRAY_ALIGNED
    );

    if (X_nparr == NULL) {
        /* 处理意外情况 */
    }

    ClassCounter *counter = class_counter_create();
    npy_intp X_size = PyArray_SIZE(X_nparr);
    unsigned long *X_npdata = (unsigned long *)PyArray_DATA(X_nparr);

    for (Py_ssize_t i = 0; i < X_size; i++)
        if(class_counter_count(counter, X_npdata[i])) {
            PyErr_SetString(PyExc_ValueError, "类别数过大！");
            return NULL;
        }

    result = PyDict_New();
    for (size_t i = 0; i < counter->size; i++)
        PyDict_SetItem(result, PyLong_FromSize_t(i), PyLong_FromUnsignedLong(counter->count_result[i]));

    return result;
}


static PyObject*
pyextension_information_entropy(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple (args, "O!", &PyArray_Type, &obj))
        return NULL;

    if ( !PyArray_ISUNSIGNED( (PyArrayObject *)obj ) ) {  /*不是无符号整数的情况*/
        PyErr_SetString(PyExc_ValueError, "输入的类型必需为无符号整型！");
        return NULL;
    }

    PyArrayObject *arr_obj = (PyArrayObject *)PyArray_FROM_OTF(
        obj,        /* 传入数据 */
        NPY_ULONG,  /* 传入数据类型为 unsigned long */
        NPY_ARRAY_ALIGNED
    );

    if (arr_obj == NULL) {
        /* 处理意外情况 */
    }

    npy_intp size = PyArray_SIZE(arr_obj);
    unsigned long *data = (unsigned long *)PyArray_DATA(arr_obj);

    double entropy = information_entropy(data, (unsigned int)size);

    return Py_BuildValue("f", entropy);
}


static PyObject*
pyextension_info_entropy_discrete_prop(PyObject *self, PyObject *args)
{
    PyArrayObject *y, *properties;
    if ( !PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &y, &PyArray_Type, &properties) )
        return NULL;

    if ( !PyArray_ISUNSIGNED(y) ) {  /*不是无符号整数的情况*/
        PyErr_SetString(PyExc_ValueError, "输入的类型必需为无符号整型！");
        return NULL;
    }

    if ( !PyArray_ISUNSIGNED(properties) ) {  /*不是无符号整数的情况*/
        PyErr_SetString(PyExc_ValueError, "输入的类型必需为无符号整型！");
        return NULL;
    }

    npy_intp y_size = PyArray_SIZE(y);
    npy_intp properties_size = PyArray_SIZE(properties);

    if (y_size != properties_size) {
        PyErr_SetString(PyExc_ValueError, "输入的数组长度不相等！");
        return NULL;
    }

    unsigned long *y_data = (unsigned long *)PyArray_DATA(y);
    unsigned long *prop_data = (unsigned long *)PyArray_DATA(properties);

    double entropy = info_entropy_discrete_prop(y_data, prop_data, y_size);
    return Py_BuildValue("f", entropy);
}


/** ====================================================================================================
 * Module Define
 **/

static PyMethodDef
pyextension_methods[] = {
    {"test",                       pyextension_test,                           METH_VARARGS, "test(x, y, z)"},
    {"test_dict",                  pyextension_test_dict,                      METH_VARARGS, "test dict"},
    {"test_class",                 pyextension_test_class,                     METH_VARARGS, "test python class"},
    {"test_include",               pyextension_test_include,                   METH_VARARGS, "test python class"},
    {"class_counter",              pyextension_class_counter,                  METH_VARARGS, "计算一个字符数组的大小"},
    {"information_entropy",        pyextension_information_entropy,            METH_VARARGS, "计算一个字符数组的大小"},
    {"info_entropy_discrete_prop", pyextension_info_entropy_discrete_prop,  METH_VARARGS, "计算一个字符数组的大小"},
    {NULL, NULL, 0, NULL},
};


static PyModuleDef
pyextension_module = { 
    PyModuleDef_HEAD_INIT,
    "_fastmlcore",
    NULL,
    -1,
    pyextension_methods,
    NULL,
    NULL,
    NULL,
    NULL,
};


/* PyMODINIT_FUNC 最后定义 */

PyMODINIT_FUNC
PyInit_pyextension(void)
{
    PyObject *m, *logit, *d;
    m = PyModule_Create(&pyextension_module);
    if (!m) {
        return NULL;  /* 防止错误 */
    }

    import_array();
    import_umath();

    logit = PyUFunc_FromFuncAndData(logit_funcs,       /* function */
                                    logit_data,        /* data */
                                    logit_types,       /* types */
                                    3,                 /* ntypes */
                                    1,                 /* nin */
                                    1,                 /* nout */
                                    PyUFunc_None,      /* identity */
                                    "logit",           /* name */
                                    "logit_docstring", /* doc */
                                    0);                /* unused */

    d = PyModule_GetDict(m);
    PyDict_SetItemString(d, "logit", logit);
    Py_DECREF(logit);

    return m;
}