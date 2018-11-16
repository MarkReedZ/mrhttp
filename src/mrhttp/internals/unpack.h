#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "Python.h"

PyObject* unpackc( unsigned char *p, int len );
void initmrpacker(void);

