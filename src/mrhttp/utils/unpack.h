#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "Python.h"
 
#if defined(_MSC_VER)
#define inline __inline
#endif

PyObject* unpackc( unsigned char *p, int len );
void initmrpacker(void);
