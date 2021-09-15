
#pragma once

#include <Python.h>

static char to_64[64] = "mbo3uXFEWKAdCNS7LpJt-UzeOVfvyYasj4DlwBnQix1_hHRG59cP8r2TgM6Z0Iqk";
static int from_64[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,0,60,42,54,3,33,48,58,15,52,49,0,0,0,0,0,0,0,10,37,12,34,7,6,47,45,61,18,9,16,57,13,24,51,39,46,14,55,21,25,8,5,29,59,0,0,0,0,43,0,30,1,50,11,23,26,56,44,40,32,63,35,0,38,2,17,62,53,31,19,4,27,36,41,28,22};


PyObject* myrandint(PyObject* self, PyObject* args);
PyObject* escape_html(PyObject* self, PyObject* s);

PyObject *to64     (PyObject *self, PyObject *num);
PyObject *from64   (PyObject *self, PyObject *str);
PyObject *timesince(PyObject *self, PyObject *ts);
PyObject *hot      (PyObject *self, PyObject *args);

char *findchar(char *buf, char *buf_end, char *ranges, size_t ranges_size, int *found);

