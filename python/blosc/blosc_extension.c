/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted (faltet@pytables.org)
  Creation date: 2010-09-22

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/



#include "Python.h"
#include "blosc.h"


static PyObject *BloscError;

static void
blosc_error(int err, char *msg)
{
  PyErr_Format(BloscError, "Error %d %s", err, msg);
}


PyDoc_STRVAR(set_nthreads__doc__,
"set_nthreads(nthreads) -- Initialize a pool of threads for Blosc operation.\n"
             );

static PyObject *
PyBlosc_set_nthreads(PyObject *self, PyObject *args)
{
    int nthreads, old_nthreads;

    if (!PyArg_ParseTuple(args, "i:set_nthreads", &nthreads))
      return NULL;

    old_nthreads = blosc_set_nthreads(nthreads);

    return Py_BuildValue("i", old_nthreads);
}


PyDoc_STRVAR(free_resources__doc__,
"free_resources() -- Free possible memory temporaries and thread resources.\n"
             );

static PyObject *
PyBlosc_free_resources(PyObject *self)
{
    blosc_free_resources();

    return Py_None;
}


PyDoc_STRVAR(compress__doc__,
"compress(string[, typesize, clevel, shuffle]) -- Return compressed string.\n"
             );

static PyObject *
PyBlosc_compress(PyObject *self, PyObject *args)
{
    PyObject *result_str = NULL;
    void *input, *output;
    int clevel, shuffle, cbytes;
    Py_ssize_t nbytes, typesize;

    /* require Python string object, typesize, clevel and shuffle agrs */
    if (!PyArg_ParseTuple(args, "s#nii:compress", &input, &nbytes,
                          &typesize, &clevel, &shuffle))
      return NULL;

    /* Alloc memory for compression */
    output = malloc(nbytes+BLOSC_MAX_OVERHEAD);
    if (output == NULL) {
      PyErr_SetString(PyExc_MemoryError,
                      "Can't allocate memory to compress data");
      return NULL;
    }

    /* Compress */
    Py_BEGIN_ALLOW_THREADS;
    cbytes = blosc_compress(clevel, shuffle, typesize, nbytes,
                            input, output, nbytes+BLOSC_MAX_OVERHEAD);
    Py_END_ALLOW_THREADS;

    if (cbytes < 0) {
      blosc_error(cbytes, "while compressing data");
      free(output);
      return NULL;
    }

    /* This forces a copy of the output, but anyway */
    result_str = PyBytes_FromStringAndSize((char *)output, cbytes);

    /* Free the initial buffer */
    free(output);

    return result_str;
}

PyDoc_STRVAR(decompress__doc__,
"decompress(string) -- Return decompressed string.\n"
             );

static PyObject *
PyBlosc_decompress(PyObject *self, PyObject *args)
{
    PyObject *result_str;
    void *input, *output;
    int cbytes, err;
    size_t nbytes, cbytes2, blocksize;

    if (!PyArg_ParseTuple(args, "s#:decompress", &input, &cbytes))
      return NULL;

    /* Get the length of the uncompressed buffer */
    blosc_cbuffer_sizes(input, &nbytes, &cbytes2, &blocksize);

    if (cbytes != (int)cbytes2) {
      blosc_error(cbytes, ": not a Blosc buffer or header info is corrupted");
      return NULL;
    }

    /* Book memory for the result */
    if (!(result_str = PyBytes_FromStringAndSize(NULL, nbytes)))
      return NULL;
    output = PyBytes_AS_STRING(result_str);

    /* Do the decompression */
    Py_BEGIN_ALLOW_THREADS;
    err = blosc_decompress(input, output, nbytes);
    Py_END_ALLOW_THREADS;

    if (err < 0 || err != (int)nbytes) {
      blosc_error(err, "while decompressing data");
      Py_XDECREF(result_str);
      return NULL;
    }

    return result_str;

}


static PyMethodDef blosc_methods[] =
{
  {"compress", (PyCFunction)PyBlosc_compress,  METH_VARARGS,
   compress__doc__},
  {"decompress", (PyCFunction)PyBlosc_decompress, METH_VARARGS,
   decompress__doc__},
  {"free_resources", (PyCFunction)PyBlosc_free_resources, METH_VARARGS,
   free_resources__doc__},
  {"set_nthreads", (PyCFunction)PyBlosc_set_nthreads, METH_VARARGS,
   set_nthreads__doc__},
    {NULL, NULL, 0, NULL}
};


#if PY_MAJOR_VERSION < 3
/* Python 2 module initialization */
PyMODINIT_FUNC
initblosc_extension(void)
{
  PyObject *m;
  m = Py_InitModule("blosc_extension", blosc_methods);
  if (m == NULL)
    return;

  BloscError = PyErr_NewException("blosc_extension.error", NULL, NULL);
  if (BloscError != NULL) {
    Py_INCREF(BloscError);
    PyModule_AddObject(m, "error", BloscError);
  }

  /* Integer macros */
  PyModule_AddIntMacro(m, BLOSC_MAX_BUFFERSIZE);
  PyModule_AddIntMacro(m, BLOSC_MAX_THREADS);

  /* String macros */
  PyModule_AddStringMacro(m, BLOSC_VERSION_STRING);
  PyModule_AddStringMacro(m, BLOSC_VERSION_DATE);

}
# else
/* Python 3 module initialization */
static struct PyModuleDef blosc_def = {
  PyModuleDef_HEAD_INIT,
  "blosc_extension",
  NULL,
  -1,
  blosc_methods,
  NULL,
  NULL,
  NULL,
  NULL
};

PyMODINIT_FUNC
PyInit_blosc_extension(void) {
  PyObject *m = PyModule_Create(&blosc_def);

  /* Integer macros */
  PyModule_AddIntMacro(m, BLOSC_MAX_BUFFERSIZE);
  PyModule_AddIntMacro(m, BLOSC_MAX_THREADS);

  /* String macros */
  PyModule_AddStringMacro(m, BLOSC_VERSION_STRING);
  PyModule_AddStringMacro(m, BLOSC_VERSION_DATE);

  return m;
}
#endif
