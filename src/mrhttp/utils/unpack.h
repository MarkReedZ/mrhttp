#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "Python.h"
 
PyObject* unpackc( char *p, int len );
