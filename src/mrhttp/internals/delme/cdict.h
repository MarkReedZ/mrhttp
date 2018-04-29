#pragma once

#include <Python.h>

typedef struct {
  char* key;
  size_t key_length;
  char* value;
  size_t value_length;
} CDictEntry;

PyObject* cdict_to_dict(CDictEntry* entries, size_t length);
