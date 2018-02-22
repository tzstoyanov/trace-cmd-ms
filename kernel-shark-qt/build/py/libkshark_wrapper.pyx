""" Small Cython file to demonstrate the use of PyArray_SimpleNewFromData
in Cython to create an array from already allocated memory.

Cython enables mixing C-level calls and Python-level calls in the same
file with a Python-like syntax and easy type cohersion. See 
http://cython.org for more information
"""

# Author: Gael Varoquaux
# License: BSD

# Import the Python-level symbols of numpy
import numpy as np

# Import the C-level symbols of numpy
cimport numpy as np

cdef extern from "stdint.h":
    ctypedef unsigned long long uint8_t
    ctypedef unsigned long long uint16_t
    ctypedef unsigned long long uint64_t

# Declare the prototype of the C function we are interested in calling
cdef extern from "../../src/libkshark.c":
    size_t kshark_trace2matrix(const char *fname,
				   uint64_t **offset_array,
				   uint8_t **cpu_array,
				   uint64_t **ts_array,
			           uint16_t **pid_array,
			           int **event_array,
			           uint8_t **vis_array);

cdef extern from "../../src/libkshark.c":
   const char *kshark_get_task_lazy(uint16_t pid);

from libc.stdlib cimport free
from cpython cimport PyObject, Py_INCREF

# Numpy must be initialized. When using numpy from C or Cython you must
# _always_ do that, or you will have segfaults
np.import_array()

# We need to build an array-wrapper class to deallocate our array when
# the Python object is deleted.

cdef class ArrayWrapper:
    cdef void* data_ptr
    cdef int typenum
    cdef int size

    cdef set_data(self, int size, int typenum, void* data_ptr):
        """ Set the data of the array

        This cannot be done in the constructor as it must recieve C-level
        arguments.

        Parameters:
        -----------
        size: int
            Length of the array.
        data_ptr: void*
            Pointer to the data            

        """
        self.data_ptr = data_ptr
        self.typenum = typenum
        self.size = size

    def __array__(self):
        """ Here we use the __array__ method, that is called when numpy
            tries to get an array from the object."""
        cdef np.npy_intp shape[1]
        shape[0] = <np.npy_intp> self.size
        # Create a 1D array, of length 'size'
        ndarray = np.PyArray_SimpleNewFromData(1, shape,
                                               self.typenum, self.data_ptr)
        return ndarray

    def __dealloc__(self):
        """ Frees the array. This is called by Python when all the
        references to the object are gone. """
        free(<void*>self.data_ptr)


def load_data_file(fname):
    """ Python binding of the 'kshark_load_data_matrix' function in 'c_code.c' that does
        not copy the data allocated in C.
    """
    cdef uint64_t *ofst
    cdef uint8_t *cpu
    cdef uint64_t *ts
    cdef uint16_t *pid
    cdef int *evt
    cdef uint8_t *vis

    cdef np.ndarray ndarray_ofst
    cdef np.ndarray ndarray_cpu
    cdef np.ndarray ndarray_ts
    cdef np.ndarray ndarray_pid
    cdef np.ndarray ndarray_evt
    cdef np.ndarray ndarray_vis
 
    # Call the C function
    cdef int size = kshark_trace2matrix(fname, &ofst, &cpu, &ts, &pid, &evt, &vis)

    array_wrapper_ofst = ArrayWrapper()
    array_wrapper_cpu = ArrayWrapper()
    array_wrapper_ts = ArrayWrapper()
    array_wrapper_pid = ArrayWrapper()
    array_wrapper_evt = ArrayWrapper()
    array_wrapper_vis = ArrayWrapper()

    array_wrapper_ofst.set_data(size, np.NPY_UINT64,  <void*> ofst)
    array_wrapper_cpu.set_data(size, np.NPY_UINT8,  <void*> cpu)
    array_wrapper_ts.set_data(size, np.NPY_UINT64, <void*> ts)
    array_wrapper_pid.set_data(size, np.NPY_UINT16, <void*> pid)
    array_wrapper_evt.set_data(size, np.NPY_INT16, <void*> evt)
    array_wrapper_vis.set_data(size, np.NPY_INT16, <void*> vis)

    ndarray_ofst = np.array(array_wrapper_ofst, copy=False)
    ndarray_cpu = np.array(array_wrapper_cpu, copy=False)
    ndarray_ts = np.array(array_wrapper_ts, copy=False)
    ndarray_pid = np.array(array_wrapper_pid, copy=False)
    ndarray_evt = np.array(array_wrapper_evt, copy=False)
    ndarray_vis = np.array(array_wrapper_vis, copy=False)

    # Assign our object to the 'base' of the ndarray object
    ndarray_ofst.base = <PyObject*> array_wrapper_ofst
    ndarray_cpu.base = <PyObject*> array_wrapper_cpu
    ndarray_ts.base = <PyObject*> array_wrapper_ts
    ndarray_pid.base = <PyObject*> array_wrapper_pid
    ndarray_evt.base = <PyObject*> array_wrapper_evt
    ndarray_vis.base = <PyObject*> array_wrapper_vis

    # Increment the reference count, as the above assignement was done in
    # C, and Python does not know that there is this additional reference
    Py_INCREF(array_wrapper_ofst)
    Py_INCREF(array_wrapper_cpu)
    Py_INCREF(array_wrapper_ts)
    Py_INCREF(array_wrapper_pid)
    Py_INCREF(array_wrapper_evt)
    Py_INCREF(array_wrapper_vis)

    return ndarray_ofst, ndarray_cpu, ndarray_ts, ndarray_pid, ndarray_evt, ndarray_vis


def trace2matrix(fname):
    """ Python binding of the 'kshark_load_data_matrix' function in 'c_code.c' that does
        not copy the data allocated in C.
    """
    ofst, cpu, ts, pid, evt, vis = load_data_file(fname)
    return np.matrix(cpu, ts, pid)

class traceDataFile(object):
    """Abstracts a Tensorflow graph for a learning task.
    """
    def __init__(self, file_name):
        self.file_name = file_name
        self.load_data_file(file_name)

    def load_data_file(fname):
        load_data_file(fname)
