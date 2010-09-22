/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted (faltet@pytables.org)
  Creation date: 2010-09-22

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/



#include "Python.h"
#include "blosc.h"


#define PyInit_blosc initblosc_extension

static PyObject *BloscError;

static void
blosc_error(int err, char *msg)
{
  PyErr_Format(BloscError, "Error %d %s", err, msg);
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

    /* require Python string object, optional 'level' arg */
    if (!PyArg_ParseTuple(args, "s#|nii:compress", &input, &nbytes,
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
    result_str = PyString_FromStringAndSize((char *)output, cbytes);

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
    if (!(result_str = PyString_FromStringAndSize(NULL, nbytes)))
      return NULL;
    output = PyString_AS_STRING(result_str);

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
    {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC
PyInit_blosc(void)
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
    PyModule_AddIntMacro(m, BLOSC_VERSION_MAJOR);
    PyModule_AddIntMacro(m, BLOSC_VERSION_MINOR);
    PyModule_AddIntMacro(m, BLOSC_VERSION_RELEASE);

    PyModule_AddIntMacro(m, BLOSC_VERSION_FORMAT);
    PyModule_AddIntMacro(m, BLOSCLZ_VERSION_FORMAT);
    PyModule_AddIntMacro(m, BLOSC_VERSION_CFORMAT);

    PyModule_AddIntMacro(m, BLOSC_MIN_HEADER_LENGTH);
    PyModule_AddIntMacro(m, BLOSC_MAX_OVERHEAD);

    PyModule_AddIntMacro(m, BLOSC_MAX_BUFFERSIZE);
    PyModule_AddIntMacro(m, BLOSC_MAX_TYPESIZE);
    PyModule_AddIntMacro(m, BLOSC_MAX_THREADS);

    PyModule_AddIntMacro(m, BLOSC_DOSHUFFLE);
    PyModule_AddIntMacro(m, BLOSC_MEMCPYED);

    /* String macros */
    PyModule_AddStringMacro(m, BLOSC_VERSION_STRING);
    PyModule_AddStringMacro(m, BLOSC_VERSION_REVISION);
    PyModule_AddStringMacro(m, BLOSC_VERSION_DATE);

}

